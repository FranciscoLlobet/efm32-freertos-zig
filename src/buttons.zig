const std = @import("std");
const microzig = @import("microzig");
const c = @cImport({
    @cInclude("board.h");
    @cInclude("sl_simple_button.h");
});
const leds = @import("leds");

pub const button_handle = [*c]const c.sl_button_t;

pub const button_error = error{
    invalid_button_handle,
};

handle: button_handle,

pub const state = enum(u32) {
    disabled = c.SL_SIMPLE_BUTTON_DISABLED,
    pressed = c.SL_SIMPLE_BUTTON_PRESSED,
    released = c.SL_SIMPLE_BUTTON_RELEASED,
};

pub fn getState(self: *const @This()) state {
    return @as(state, @enumFromInt(c.sl_simple_button_get_state(self.handle)));
}
pub fn getHandle(self: *const @This()) button_handle {
    return self.handle;
}

pub fn getInstance(handle: button_handle) *const @This() {
    if (handle == button1.handle) {
        return &button1;
    } else if (handle == button2.handle) {
        return &button2;
    } else {
        return undefined;
    }
}

pub const button1 = @This(){ .handle = &c.button1 };
pub const button2 = @This(){ .handle = &c.button2 };
