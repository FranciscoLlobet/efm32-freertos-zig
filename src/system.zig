const std = @import("std");
const board = @import("microzig").board;
const freertos = @import("freertos");

fn _performReset(param1: ?anyopaque, param2: u32) callconv(.C) noreturn {
    _ = param2;
    _ = param1;

    board.red.on();
    board.yellow.on();
    board.orange.on();

    board.msDelay(1000);

    freertos.c.taskENTER_CRITICAL();

    board.mcuReset();
}

pub fn reset() void {
    if (!freertos.xTimerPendFunctionCall(_performReset, null, 0, freertos.portMAX_DELAY)) {
        _performReset(null, 0);
    }
}

pub fn shutdown() void {
    reset();
}

pub export fn system_reset() void {
    reset();
}
