const std = @import("std");
const microzig = @import("microzig");
const c = @cImport({
    @cDefine("EFM32GG390F1024", "1");
    @cDefine("__PROGRAM_START", "__main");
    @cInclude("board.h");
});

pub const cpu_frequency = 48_000_000; // 48MHz

pub fn mcuReset() noreturn {
    c.BOARD_MCU_Reset();
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

// LED stuff
pub const led = struct {
    led_handle: *c.sl_led_t = undefined,
    pub fn init(self: *led) void {
        c.led_init(self.led_handle);
    }
    pub fn on(self: *led) void {
        c.sl_led_turn_on(self.led_handle);
    }
    pub fn off(self: *led) void {
        c.sl_led_turn_off(self.led_handle);
    }
    pub fn toggle(self: *led) void {
        c.sl_led_toggle(self.led_handle);
    }
};

pub var red = led{ .led_handle = &c.led_red };
pub var yellow = led{ .led_handle = &c.led_yellow };
pub var orange = led{ .led_handle = &c.led_orange };
