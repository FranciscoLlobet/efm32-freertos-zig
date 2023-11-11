const std = @import("std");
const microzig = @import("microzig");
pub const c = @cImport({
    @cInclude("board.h");
    @cInclude("simplelink.h");
});

pub const cpu_frequency = 48_000_000; // 48MHz

pub fn getTime() u32 {
    return c.sl_sleeptimer_get_time();
}

pub fn mcuReset() noreturn {
    c.BOARD_MCU_Reset();
    unreachable;
}

pub fn getResetCause() u32 {
    return c.BOARD_MCU_GetResetCause();
}

pub fn init() void {
    c.BOARD_Init();
}

pub fn msDelay(ms: u32) void {
    c.BOARD_msDelay(ms);
}

pub fn usDelay(us: u32) void {
    c.BOARD_usDelay(us);
}

pub fn watchdogEnable() void {
    c.BOARD_Watchdog_Enable();
}

pub fn watchdogFeed() void {
    c.BOARD_Watchdog_Feed();
}
