const std = @import("std");
const freertos = @import("freertos.zig");
const leds = @import("leds.zig");
const config = @import("config.zig");
const fatfs = @import("fatfs.zig");
const c = @cImport({
    @cInclude("board.h");
    @cInclude("miso_config.h");
    @cInclude("wifi_service.h");
});
const sha256 = @import("sha256.zig");

task: freertos.Task,
timer: freertos.Timer,
queue: freertos.Queue,

fn myTimerFunction(xTimer: freertos.TimerHandle_t) callconv(.C) void {
    var self = freertos.getAndCastTimerID(@This(), xTimer);
    var test_var: u32 = 0xAA55;

    _ = self.queue.send(@as(*void, @ptrCast(&test_var)), null);
}

//var file: ?fatfs.file = undefined;
//var hash: sha256 = undefined;
//var readBuffer: [24]u8 = undefined;
//const allocator = freertos.allocator;

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

    c.miso_load_config();

    wifi_task.resumeTask();

    self.timer.start(null) catch unreachable;

    while (true) {
        var test_var: u32 = 0;

        if (self.queue.receive(@as(*void, @ptrCast(&test_var)), null)) {
            leds.yellow.toggle();
            _ = c.printf("UserTask: %d\r\n", self.task.getStackHighWaterMark());
        }
    }
}

pub fn create(self: *@This()) void {
    self.task.create(myUserTaskFunction, "user_task", config.rtos_stack_depth_user_task, self, config.rtos_prio_user_task) catch unreachable;
    self.queue.create(4, 1) catch unreachable;
    self.timer.create("user_timer", 2000, freertos.pdTRUE, self, myTimerFunction) catch unreachable;
}

pub var user_task: @This() = undefined;
