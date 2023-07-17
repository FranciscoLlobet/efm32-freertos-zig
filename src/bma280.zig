const std = @import("std");
const board = @import("microzig").board;
const config = @import("config.zig");

const c = @cImport({
    @cInclude("board_i2c_sensors.h");
});

handle: *c.bma2_dev,

pub fn init(self: *const @This()) void {
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
}

pub const sensor = @This(){ .handle = &c.board_bma280 };
