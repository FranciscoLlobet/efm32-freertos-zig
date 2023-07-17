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

handle: *c.bme280_dev,

pub fn init(self: *const @This()) !void {
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

    if (ret != 0) return sensor_errors.bme280_init_error;
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

pub const sensor = @This(){ .handle = &c.board_bme280 };
