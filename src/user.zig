const std = @import("std");
const freertos = @import("freertos.zig");
const leds = @import("leds.zig");
const config = @import("config.zig");
const fatfs = @import("fatfs.zig");
const c = @cImport({
    @cInclude("board.h");
    @cInclude("miso.h");
    @cInclude("miso_config.h");
    @cInclude("wifi_service.h");
});
const pk = @import("pk.zig");
const sha256 = @import("sha256.zig");
const mbedtls = @import("mbedtls.zig");
const http = @import("http.zig");
const mqtt = @import("mqtt.zig");
const lwm2m = @import("lwm2m.zig");
const nvm = @import("nvm.zig");
const board = @import("microzig").board;
const system = @import("system.zig");
const firmware = @import("boot/firmware.zig");
const ntp = @import("ntp.zig");

const state = enum(usize) {
    verify_config = 0,
    start_connectivity,
    stop_connectivity,

    /// First NTP sync
    start_ntp_time,
    start_mqtt,
    start_lwm2m,
    get_config,
    perform_firmware_download,
    verify_firmware_download,

    working, // normal work mode
};

const notificationValues = enum(u32) {
    no = 0,

    connectivity_on = c.miso_connectivity_on,
    connectivity_off = c.miso_connectivity_off,

    ntp_sync = (1 << 2),
    user_timer = (1 << 3),

    pub fn isNotification(val: u32, notification: notificationValues) bool {
        return (val & @intFromEnum(notification)) == @intFromEnum(notification);
    }
};

/// User task handle
task: freertos.StaticTask(@This(), config.rtos_stack_depth_user_task, "user task", myUserTaskFunction),

/// User timer
timer: freertos.StaticTimer(@This(), "user timer", myTimerFunction),

/// NTP timer
ntpTimer: freertos.StaticTimer(@This(), "ntp timer", myNtpTimerFunction),
ntpSyncTime: u32,

/// User Statemachine state
state: state,

fn myTimerFunction(self: *@This()) void {
    self.task.notify(@intFromEnum(notificationValues.user_timer), .eSetBits) catch {};
}

fn myNtpTimerFunction(self: *@This()) void {
    self.task.notify(@intFromEnum(notificationValues.ntp_sync), .eSetBits) catch {};
}

fn myUserTaskFunction(self: *@This()) void {
    var wifi_task = freertos.Task.initFromHandle(@as(freertos.TaskHandle_t, @ptrCast(c.wifi_task_handle)));

    // Initialize the NVM
    _ = nvm.init() catch 0;

    fatfs.mount("SD") catch unreachable;

    // State machine pattern
    while (true) {
        var eventValue: u32 = 0;
        _ = eventValue;

        if (self.state == .verify_config) {
            config.load_config_from_nvm() catch {
                _ = c.printf("Failure!!\n\r");
            };

            _ = config.open_config_file(config.config_file_name) catch {
                _ = c.printf("Failure!!\n\r");
            };

            self.state = .start_connectivity;
        } else if (self.state == .start_connectivity) {

            // Change this to wait for connectivity
            wifi_task.resumeTask();

            // Wait for connectivity
            while (self.task.waitForNotify(0, @intFromEnum(notificationValues.connectivity_on), null) catch unreachable) |val| {
                if (notificationValues.isNotification(val, notificationValues.connectivity_on)) {
                    break;
                }
            }

            self.state = .start_ntp_time;
        } else if (self.state == .stop_connectivity) {
            _ = c.printf("Lost connectivity\r\n");

            self.ntpTimer.stop(null) catch unreachable; // stop NTP timer

            lwm2m.service.task.suspendTask();
            mqtt.service.task.suspendTask();

            self.state = .working; // go to the working state (?)
        } else if (self.state == .start_ntp_time) {
            // Get time from NTP Server

            const ntp_uri: std.Uri = std.Uri.parse("ntp://1.de.pool.ntp.org:123") catch unreachable;

            if (ntp.getTimeFromServer(ntp_uri)) |ntpResponse| {
                self.ntpSyncTime = ntpResponse.timestamp_s;

                // Calculate the next time to sync
                const nextSyncTime: u32 = if (ntpResponse.poll_interval > 60 * 60) @as(u32, 60 * 60 * 1000) else ntpResponse.poll_interval * 1000;

                _ = c.printf("NTP Sync: %d\r\n", system.time.now());
                self.ntpTimer.changePeriod(nextSyncTime, null) catch unreachable;
                self.state = .perform_firmware_download;
                self.state = .start_mqtt;
            } else |_| {
                self.ntpSyncTime = 0;
                self.task.delayTask(16000); // wait for 16
                self.state = .start_ntp_time;
            }
        } else if (self.state == .perform_firmware_download) {
            if (config.enable_http) {
                _ = c.printf("Performing firmware download\r\n");

                if (downloadAndVerify()) |_| {
                    // Happy path

                    nvm.setUpdateRequest() catch unreachable;

                    _ = c.printf("Firmware download complete\r\n");

                    self.task.delayTask(1000);

                    // reset
                } else |err| {
                    if (err == firmware.firmware_error.firmware_already_in_system) {
                        _ = c.printf("Firmware already in system\n\r");
                    } else {
                        _ = c.printf("Failure to download!!\n\r");
                    }
                }
            }

            // perform HTTP download
            if (comptime config.enable_lwm2m) {
                self.state = .start_lwm2m;
            } else if (comptime config.enable_mqtt) {
                self.state = .start_mqtt;
            } else {
                self.state = .working;
            }
        } else if (self.state == .start_mqtt) {
            mqtt.service.resumeTask();

            self.timer.start(null) catch unreachable;

            self.state = .working;
        } else if (self.state == .start_lwm2m) {
            lwm2m.service.task.resumeTask();

            self.state = .working;
        } else if (self.state == .working) {
            // recieve
            if (self.task.waitForNotify(0, 0xFFFFFFFF, null) catch unreachable) |val| {
                if (notificationValues.isNotification(val, notificationValues.connectivity_on)) {
                    self.state = .start_ntp_time;
                } else if (notificationValues.isNotification(val, notificationValues.connectivity_off)) {
                    self.state = .stop_connectivity;
                } else if (notificationValues.isNotification(val, notificationValues.ntp_sync)) {
                    // Execute the NTP sync

                    // self.ntpSyncTime = ntp.getTime();

                } else if (notificationValues.isNotification(val, notificationValues.user_timer)) {
                    leds.yellow.toggle();
                    _ = c.printf("UserTimer: %d\r\n", self.task.getStackHighWaterMark());
                }
            }
        } else {
            _ = c.printf("Unknown state\r\n");
        }

        // perform per-cycle checks
    }
}

fn downloadAndVerify() !bool {
    // Download the firmware
    try http.service.filedownload(config.getHttpFwUri(), config.fw_file_name, config.file_block_size, 1024 * 1024);

    try firmware.checkFirmwareImage(config.fw_file_name);

    return true;
}

pub fn create(self: *@This()) void {
    self.state = state.verify_config;
    self.task.create(self, config.rtos_prio_user_task) catch unreachable;
    self.timer.create(2000, true, self) catch unreachable;
    self.ntpTimer.create(4000, true, self) catch unreachable;
    self.ntpSyncTime = 0;
}

pub var user_task: @This() = .{ .timer = undefined, .state = undefined, .task = undefined, .ntpTimer = undefined, .ntpSyncTime = 0 };
