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
});

// Led instances
pub const yellow = @This().createFromHandle(@ptrCast(board.led_yellow));
pub const orange = @This().createFromHandle(@ptrCast(board.led_orange));
pub const red = @This().createFromHandle(@ptrCast(board.led_red));

const led_handle = [*c]const c.sl_led_t;

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

pub fn set(self: *const @This(), state: bool) void {
    if (state) {
        self.on();
    } else {
        self.off();
    }
}
