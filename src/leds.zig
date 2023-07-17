const std = @import("std");
const microzig = @import("microzig");
const c = @cImport({
    @cInclude("board.h");
});

pub const led_handle = [*c]const c.sl_led_t;

// Led instances
pub const yellow = @This().createFromHandle(&c.led_yellow);
pub const orange = @This().createFromHandle(&c.led_orange);
pub const red = @This().createFromHandle(&c.led_red);

// LED stuff
handle: led_handle,

pub fn createFromHandle(comptime handle: led_handle) @This() {
    return @This(){ .handle = handle };
}

pub fn on(self: *const @This()) void {
    c.sl_led_turn_on(self.handle);
}

pub fn off(self: *const @This()) void {
    c.sl_led_turn_off(self.handle);
}

pub fn toggle(self: *const @This()) void {
    c.sl_led_toggle(self.handle);
}
