// Copyright (c) 2023-2024 Francisco Llobet-Blandino and the "Miso Project".
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the “Software”), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
// WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

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

pub inline fn getState(self: *const @This()) state {
    return @as(state, @enumFromInt(c.sl_simple_button_get_state(self.handle)));
}
pub inline fn getHandle(self: *const @This()) button_handle {
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
