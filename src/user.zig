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
const http = @import("http.zig");
const mqtt = @import("mqtt.zig");
const lwm2m = @import("lwm2m.zig");
const nvm = @import("nvm.zig");
const board = @import("microzig").board;
const system = @import("system.zig");

const state = enum(usize) {
    verify_config = 0,
    start_connectivity,
    start_mqtt,
    start_lwm2m,
    get_config,
    perform_firmware_download,
    verify_firmware_download,

    working, // normal work mode

    pub fn newState(currentState: @This()) @This() {
        return switch (currentState) {
            .verify_config => .start_connectivity,
            .start_connectivity => .perform_firmware_download,
            .perform_firmware_download => .working,
            .working => .working,
        };
    }
};

task: freertos.StaticTask(config.rtos_stack_depth_user_task),
timer: freertos.Timer,
state: state,

fn myTimerFunction(xTimer: freertos.TimerHandle_t) callconv(.C) void {
    const self = freertos.Timer.getIdFromHandle(@This(), xTimer);
    var test_var: u32 = 0xAA55;

    self.task.notify(test_var, .eSetBits) catch {};
}

fn myUserTaskFunction(pvParameters: ?*anyopaque) callconv(.C) void {
    const self = freertos.Task.getAndCastPvParameters(@This(), pvParameters);

    var wifi_task = freertos.Task.initFromHandle(@as(freertos.TaskHandle_t, @ptrCast(c.wifi_task_handle)));

    _ = nvm.init() catch 0;

    fatfs.mount("SD") catch unreachable;

    while (true) {
        var eventValue: u32 = 0;

        if (self.state == .verify_config) {
            config.load_config_from_nvm() catch {
                _ = c.printf("Failure!!\n\r");
            };

            //board.watchdogFeed();
            _ = config.open_config_file(config.config_file_name) catch {
                _ = c.printf("Failure!!\n\r");
            };

            //config.store_config_in_nvm() catch {};

            // Next state
            self.state = .start_connectivity;
        } else if (self.state == .start_connectivity) {

            // Change this to wait for connectivity
            wifi_task.resumeTask();

            //eventValue = self.task.waitForNotify(0, 0xFFFFFFFF, null);
            while ((eventValue & c.miso_connectivity_on) != c.miso_connectivity_on) {
                if (self.task.waitForNotify(0, 0xFFFFFFFF, null) catch unreachable) |val| {
                    eventValue = val;
                }
            }

            self.state = .perform_firmware_download;
        } else if (self.state == .perform_firmware_download) {
            if (config.enable_http) {
                // get eTag from the NVM
                if (downloadAndVerify()) |_| {
                    // Happy path

                    nvm.setUpdateRequest() catch unreachable;

                    _ = c.printf("Firmware download complete\r\n");

                    self.task.delayTask(1000);

                    // system.reset();
                } else |_| {
                    _ = c.printf("Failure to download!!\n\r");
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

            if (self.task.waitForNotify(0, 0xFFFFFFFF, null) catch unreachable) |_| {
                //_ = val;
                leds.yellow.toggle();
                _ = c.printf("UserTask: %d\r\n", self.task.getStackHighWaterMark());
            }
        }
    }
}

fn downloadAndVerify() !bool {
    // Download the firmware
    try http.service.filedownload(config.getHttpFwUri(), config.fw_file_name, 512, 1024 * 1024);

    // Download the signature
    try http.service.filedownload(config.getHttpSigUri(), config.fw_sig_file_name, 512, 1024 * 1024);

    try config.verifyFirmwareSignature(config.fw_file_name, config.fw_sig_file_name, config.getHttpSigKey());

    return true;
}

pub fn create(self: *@This()) void {
    self.state = state.verify_config;
    self.task.create(myUserTaskFunction, "user_task", @constCast(self), config.rtos_prio_user_task) catch unreachable;
    self.timer = freertos.Timer.create("user_timer", 2000, true, @This(), self, myTimerFunction) catch unreachable;
}

pub var user_task: @This() = .{ .timer = undefined, .state = undefined, .task = undefined };
