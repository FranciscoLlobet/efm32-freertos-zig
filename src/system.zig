const std = @import("std");
const board = @import("microzig").board;
const freertos = @import("freertos.zig");
const leds = @import("leds.zig");
const fatfs = @import("fatfs.zig");
const nvm = @import("nvm.zig");

const max_reset_delay: freertos.TickType_t = 2000;

pub const time = struct {
    /// Get current time in seconds
    pub fn now() u32 {
        return board.getTime();
    }

    pub fn calculateDeadline(timeout_s: u32) u32 {
        return now() + timeout_s;
    }
};

fn performReset(param1: ?*anyopaque, param2: u32) callconv(.C) noreturn {
    _ = param2;
    _ = param1;

    leds.red.on();
    leds.yellow.on();
    leds.orange.on();

    // Stop the SimpleLink Driver
    _ = board.c.sl_Stop(0xFFFF);

    // Add code to stop/close the FS
    fatfs.unmount("SD") catch {};

    _ = nvm.incrementResetCounter() catch {};

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
