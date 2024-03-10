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

pub const sensor_errors = error{
    bme280_init_error,
    bme280_forced_read_error,
};

pub const bme280_data = c.struct_bme280_data;
pub const bme280_handle = *c.bme280_dev;

handle: bme280_handle,

pub fn init(handle: bme280_handle) !@This() {
    var self: @This() = .{ .handle = handle };

    var ret: i32 = -1;

    c.board_bme280_enable();

    ret = c.bme280_init(self.handle);

    if (ret == 0) {
        ret = c.bme280_set_sensor_mode(0, self.handle);
    }

    if (ret == 0) {
        ret = c.bme280_get_sensor_settings(self.handle);

        self.handle.settings.filter = c.BME280_FILTER_COEFF_OFF;
        self.handle.settings.standby_time = c.BME280_STANDBY_TIME_0_5_MS;
        self.handle.settings.osr_h = 0x5;
        self.handle.settings.osr_p = 0x5;
        self.handle.settings.osr_t = 0x5;

        if (ret == 0) {
            ret = c.bme280_set_sensor_settings(0x1F, self.handle);
        }
    }

    return if (ret == 0) self else sensor_errors.bme280_init_error;
}
pub fn readInForcedMode(self: *const @This(), data: *bme280_data) !void {
    var ret: i32 = -1;
    var bme280_delay = c.bme280_cal_meas_delay(&self.handle.settings);

    ret = c.bme280_set_sensor_mode(0x01, self.handle);

    if (ret == 0) {
        board.msDelay(bme280_delay);

        ret = c.bme280_get_sensor_data(0x07, data, self.handle);
    }

    if (ret != 0) return sensor_errors.bme280_forced_read_error;
}
