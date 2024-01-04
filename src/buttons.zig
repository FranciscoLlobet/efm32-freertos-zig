const std = @import("std");
const microzig = @import("microzig");
const board = microzig.board;
const c = @cImport({
    @cInclude("board.h");
    @cInclude("sl_simple_button.h");
});

pub const button_handle = [*c]const c.sl_button_t;

pub const button_error = error{
    invalid_button_handle,
};

handle: button_handle,

pub const name = enum(u32) { button1, button2 };

pub const state = enum(u32) {
    disabled = c.SL_SIMPLE_BUTTON_DISABLED,
    pressed = c.SL_SIMPLE_BUTTON_PRESSED,
    released = c.SL_SIMPLE_BUTTON_RELEASED,
};

fn initFromHandle(comptime handle: button_handle) @This() {
    return @This(){ .handle = handle };
}

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

pub fn getName(self: *const @This()) name {
    if (&button1 == self) {
        return name.button1;
    } else if (&button2 == self) {
        return name.button2;
    } else {
        return undefined;
    }
}

pub const button1 = @This().initFromHandle(@ptrCast(board.button1));
pub const button2 = @This().initFromHandle(@ptrCast(board.button2));
