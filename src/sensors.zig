const std = @import("std");
const board = @import("microzig").board;
const freertos = @import("freertos.zig");
const config = @import("config.zig");

const c = @cImport({
    @cDefine("EFM32GG390F1024", "1");
    @cDefine("__PROGRAM_START", "__main");
    @cInclude("board_i2c_sensors.h");
});

pub const sensor_errors = error{
    bme280_init_error,
    bme280_forced_read_error,
};

pub const bme280_data = c.struct_bme280_data;

pub const bma280 = bma2x{ .handle = &c.bma2_dev };
pub const bme280 = bme280_ctx{ .handle = &c.board_bme280 };

pub var task: freertos.Task = undefined;
pub var timer: freertos.Timer = undefined;


pub const bma2x = struct {
    handle: *c.bma2_dev,
    pub fn init(self: *const bma2x) void {
        var ret: i8 = -1;
        var self_test_result: i8 = -1;

        c.board_bma280_enable();

        ret = c.bma2_init(self.handle);
        if (c.BMA2_OK == ret) {
            ret = c.bma2_soft_reset(self.handle);
        }

        if (c.BMA2_OK == ret) {
            ret = c.bma2_perform_accel_selftest(&self_test_result, self.handle);
        }

        if (c.BMA2_OK == ret) {
            ret = c.bma2_set_power_mode(c.BMA2_STANDBY_MODE, self.handle);
        }
    }
};

pub const bme280_ctx = struct {
    handle: *c.bme280_dev,

    pub fn init(self: *const bme280_ctx) !void {
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
    pub fn readInForcedMode(self: *const bme280_ctx, data: *bme280_data) !void {
        var ret: i32 = -1;
        var bme280_delay: u32 = c.bme280_cal_meas_delay(&self.handle.settings);

        ret = c.bme280_set_sensor_mode(0x01, self.handle);

        if (ret == 0) {
            board.msDelay(bme280_delay);

            ret = c.bme280_get_sensor_data(0x07, data, self.handle);
        }

        if (ret != 0) return sensor_errors.bme280_forced_read_error;
    }
};

fn _tempTimerCallback(xTimer: freertos.TimerHandle_t) callconv(.C) void {
    _ = xTimer;
    _ = task.notify(1, freertos.eNotifyAction.eSetBits);
}

fn _sensorSampingTask(pvParameters: ?*anyopaque) callconv(.C) void {
    _ = pvParameters;

    bme280.init() catch unreachable;

    while (true) {
        var temp_data: bme280_data = undefined;
        var notification_value: u32 = 0;

        if (task.waitForNotify(0, 0xFFFFFFFF, &notification_value, freertos.portMAX_DELAY)) {
            if (notification_value == 1) {
                bme280.readInForcedMode(&temp_data) catch unreachable;
                // Notify to event system
                _ = c.printf("Temp: %d\n\r", @intFromFloat(u32, 100.0 * temp_data.temperature));

                //var test_string = "Test String";
                var sent: u32 = undefined;

                _ = c.strncpy(&c.usb_rx_buf[0], "Test String", c.strlen("Test String"));
                _ = c.USBX_blockWrite(&c.usb_rx_buf[0], c.strlen("Test String"), &sent);
            }
        }
        board.orange.toggle();
    }
}

pub fn init_sensor_service() !void {
    timer.create("tempTimer", 1000, freertos.pdTRUE, null, _tempTimerCallback) catch unreachable;
    task.create(_sensorSampingTask, "tempTask", config.rtos_stack_depth_sensor, null, config.rtos_prio_sensor) catch unreachable;
    timer.start(null) catch unreachable;
}
