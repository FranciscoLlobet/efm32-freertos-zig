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
const sha256 = @import("sha256.zig");
const http = @import("http.zig");
const mqtt = @import("mqtt.zig");

const state = enum(usize) {
    verify_config = 0,
    start_connectivity,
    start_mqtt,
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
task: freertos.Task,
timer: freertos.Timer,
queue: freertos.Queue,
state: state,

fn myTimerFunction(xTimer: freertos.TimerHandle_t) callconv(.C) void {
    var self = freertos.getAndCastTimerID(@This(), xTimer);
    var test_var: u32 = 0xAA55;

    _ = self.queue.send(@as(*void, @ptrCast(&test_var)), 0); // This is causing a deadlock (!)
}

fn myUserTaskFunction(pvParameters: ?*anyopaque) callconv(.C) void {
    var self = freertos.getAndCastPvParameters(@This(), pvParameters);

    var wifi_task = freertos.Task.initFromHandle(@as(freertos.TaskHandle_t, @ptrCast(c.wifi_task_handle)));

    _ = fatfs.mount() catch unreachable;

    // file = fatfs.file.open("SD:/CONFIG.TXT", @intFromEnum(fatfs.fMode.read)) catch null;

    //  var rb: usize = 0;
    //  const slice = allocator.alloc(u8, file.?.size()) catch unreachable;

    //   var hash_value: [32]u8 = undefined;

    //   hash.initCtx();

    //  rb = file.?.read(slice, slice.len) catch unreachable;
    //  _ = hash.start();
    //   _ = hash.update(slice, slice.len);
    //   _ = hash.finish(&hash_value);

    //   _ = file.?.close() catch unreachable;
    //   allocator.free(slice);

    self.timer.start(null) catch unreachable;

    while (true) {

        //var eventValue: u32 = undefined;
        //if (self.task.waitForNotify(0, 0xFFFFFFFF, &eventValue, 0)) {
        //    if ((eventValue & c.miso_connectivity_on) == c.miso_connectivity_on) {
        //        //
        //    }
        //    if ((eventValue & c.miso_connectivity_off) == c.miso_connectivity_off) {
        //
        //    }
        //}
        var eventValue: u32 = 0;

        if (self.state == .verify_config) {
            c.miso_load_config();

            // Next state
            self.state = .start_connectivity;
        } else if (self.state == .start_connectivity) {

            // Change this to wait for connectivity
            wifi_task.resumeTask();

            while ((eventValue & c.miso_connectivity_on) != c.miso_connectivity_on) {
                _ = self.task.waitForNotify(0, 0xFFFFFFFF, &eventValue, null);
            }

            self.state = .perform_firmware_download;
        } else if (self.state == .perform_firmware_download) {
            if (config.enable_http) {
                http.service.filedownload("http://192.168.50.133:80/XDK110.bin", "SD:FW.BIN", 512) catch {};
            }

            _ = c.printf("Firmware download complete\r\n");
            // perform HTTP download

            self.state = .start_mqtt;
        } else if (self.state == .start_mqtt) {
            mqtt.service.resumeTask();

            self.state = .working;
        } else if (self.state == .working) {
            var test_var: u32 = 0;

            if (self.queue.receive(@as(*void, @ptrCast(&test_var)), null)) {
                leds.yellow.toggle();
                _ = c.printf("UserTask: %d\r\n", self.task.getStackHighWaterMark());
            }
        }
    }
}

pub fn create(self: *@This()) void {
    self.task.create(myUserTaskFunction, "user_task", config.rtos_stack_depth_user_task, self, config.rtos_prio_user_task) catch unreachable;
    self.queue.create(4, 1) catch unreachable;
    self.timer.create("user_timer", 2000, freertos.pdTRUE, self, myTimerFunction) catch unreachable;
}

pub var user_task: @This() = undefined;
