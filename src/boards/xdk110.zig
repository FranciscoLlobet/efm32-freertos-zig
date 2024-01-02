const std = @import("std");
const microzig = @import("microzig");
pub const c = @cImport({
    @cInclude("board.h");
    @cInclude("simplelink.h");
    @cInclude("board_i2c_sensors.h");
});

pub const cpu_frequency = 48_000_000; // 48MHz

pub fn getTime() u32 {
    return c.sl_sleeptimer_get_time();
}

pub fn mcuReset() noreturn {
    c.BOARD_MCU_Reset();
    unreachable;
}

pub fn jumpToApp() noreturn {
    c.BOARD_JumpToAddress(@as(?*u32, @ptrFromInt(@as(usize, 0x78000))));
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

pub const bma280_dev = &c.board_bma280;
pub const bme280_dev = &c.board_bme280;
pub const bmg160_dev = &c.board_bmg160;
pub const bmi160_dev = &c.board_bmi160;
pub const bmm150_dev = &c.board_bmm150;

pub const button1 = &c.button1;
pub const button2 = &c.button2;

pub const led_red = &c.led_red;
pub const led_orange = &c.led_orange;
pub const led_yellow = &c.led_yellow;
