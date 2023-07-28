const std = @import("std");
const microzig = @import("microzig");
pub const c = @cImport({
    @cDefine("EFM32GG390F1024", "1");
    @cDefine("__PROGRAM_START", "__main");
    @cInclude("board.h");
    @cInclude("simplelink.h");
});

pub const cpu_frequency = 48_000_000; // 48MHz

pub fn mcuReset() noreturn {
    c.BOARD_MCU_Reset();
    unreachable;
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
