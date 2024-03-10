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
const board = @import("microzig").board;
const config = @import("config.zig");

const c = @cImport({
    @cInclude("board_i2c_sensors.h");
});

/// BMA280 handle type
pub const bma280_handle = *c.bma2_dev;

handle: bma280_handle,

pub fn init(handle: bma280_handle) !@This() {
    var self: @This() = .{ .handle = handle };

    var ret: i8 = -1;
    var self_test_result: i8 = -1;

    c.board_bma280_enable();

    ret = c.bma2_init(self.handle);
    if (0 == ret) {
        ret = c.bma2_soft_reset(self.handle);
    }

    if (0 == ret) {
        ret = c.bma2_perform_accel_selftest(&self_test_result, self.handle);
    }

    if (0 == ret) {
        ret = c.bma2_set_power_mode(0x0C, self.handle);
    }

    return self;
}
