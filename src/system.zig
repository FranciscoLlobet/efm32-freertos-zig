const std = @import("std");
const board = @import("microzig").board;
const freertos = @import("freertos.zig");
const leds = @import("leds.zig");

fn performReset(param1: ?*anyopaque, param2: u32) callconv(.C) noreturn {
    _ = param2;
    _ = param1;

    leds.red.on();
    leds.yellow.on();
    leds.orange.on();

    _ = board.c.sl_Stop(0xFFFF);

    // Add code to stop/close the FS

    freertos.c.taskENTER_CRITICAL();

    board.mcuReset();
}

pub fn reset() void {
    if (!freertos.xTimerPendFunctionCall(performReset, null, 0, freertos.portMAX_DELAY)) {
        performReset(null, 0);
    }
}

pub fn shutdown() void {
    reset();
}
