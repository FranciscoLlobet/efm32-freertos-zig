const std = @import("std");
//const microzig = @import("microzig");
const freertos = @import("freertos.zig");
const system = @import("system.zig");
const config = @import("config.zig");
const leds = @import("leds.zig");
const buttons = @import("buttons.zig");
const usb = @import("usb.zig");
const fatfs = @import("fatfs.zig");
const nvm = @import("nvm.zig");
const board = @import("microzig").board;

task: freertos.StaticTask(2000),

pub fn prepareJump() void {
    board.jumpToApp();
}

fn taskFunction(pvParameters: ?*anyopaque) callconv(.C) void {
    const self = freertos.Task.getAndCastPvParameters(@This(), pvParameters);

    while (true) {
        leds.red.on();
        self.task.delayTask(1000);
        prepareJump();
    }
}

pub fn init(self: *@This()) void {
    self.task.create(taskFunction, "BootApp", self, config.rtos_prio_boot_app) catch unreachable;
    self.task.suspendTask();
}

pub var boot_app: @This() = undefined;
