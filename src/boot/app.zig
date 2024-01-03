const std = @import("std");
//const microzig = @import("microzig");
const freertos = @import("../freertos.zig");
const system = @import("../system.zig");
const config = @import("../config.zig");
const leds = @import("../leds.zig");
const buttons = @import("../buttons.zig");
const usb = @import("../usb.zig");
const fatfs = @import("../fatfs.zig");
const nvm = @import("../nvm.zig");
const board = @import("microzig").board;
const firmware = @import("firmware.zig");

const c = @cImport({
    @cInclude("board.h");
    @cInclude("miso.h");
    @cInclude("miso_config.h");
});

task: freertos.StaticTask(2000),

fn prepareJump() void {
    board.jumpToApp();
}

fn taskFunction(pvParameters: ?*anyopaque) callconv(.C) void {
    const self = freertos.Task.getAndCastPvParameters(@This(), pvParameters);

    _ = nvm.init() catch 0;

    fatfs.mount("SD") catch unreachable;

    config.load_config_from_nvm() catch {
        _ = c.printf("Failed to load config from NVM\n");
    };

    // Load file
    firmware.checkFirmwareImage() catch {
        _ = c.printf("Failed to load firmware image\n");
    };

    while (true) {
        leds.red.on();
        self.task.delayTask(1);
        prepareJump();
    }
}

pub fn init(self: *@This()) void {
    self.task.create(taskFunction, "BootApp", self, config.rtos_prio_boot_app) catch unreachable;
    self.task.suspendTask();
}

pub var app: @This() = undefined;
