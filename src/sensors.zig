const std = @import("std");
const board = @import("microzig").board;
const freertos = @import("freertos.zig");
const config = @import("config.zig");
const network = @import("network.zig");
const bme280 = @import("bme280.zig");
const bma280 = @import("bma280.zig");

const c = @cImport({
    @cInclude("board_i2c_sensors.h");
    @cInclude("FreeRTOS.h");
    @cInclude("task.h");
});

pub const service = struct {
    task: freertos.Task = undefined,
    timer: freertos.Timer = undefined,

    fn _tempTimerCallback(xTimer: freertos.TimerHandle_t) callconv(.C) void {
        _ = freertos.getAndCastTimerID(freertos.Task, xTimer).notify(1, .eSetBits);
    }

    fn _sensorSampingTask(pvParameters: ?*anyopaque) callconv(.C) void {
        var self = freertos.getAndCastPvParameters(@This(), pvParameters);

        bme280.sensor.init() catch unreachable;
        bma280.sensor.init();

        while (true) {
            var temp_data: bme280.bme280_data = undefined;
            var notification_value: u32 = 0;

            if (self.task.waitForNotify(0, 0xFFFFFFFF, &notification_value, freertos.portMAX_DELAY)) {
                if (notification_value == 1) {
                    bme280.sensor.readInForcedMode(&temp_data) catch unreachable;
                    // Notify to event system
                    _ = c.printf("Temp: %d, Stack: %d\n\r", @as(i32, @intFromFloat(100.0 * temp_data.temperature)), self.task.getStackHighWaterMark());

                    network.lwm2m.service.updateTemperature(@as(f32, @as(f32, @floatCast(temp_data.temperature))));
                }
            }
        }
    }
    pub fn init(self: *@This()) !void {
        self.task.create(_sensorSampingTask, "tempTask", config.rtos_stack_depth_sensor, self, config.rtos_prio_sensor) catch unreachable;
        self.timer.create("tempTimer", 1000, freertos.pdTRUE, self, _tempTimerCallback) catch unreachable;
        self.timer.start(null) catch unreachable;
    }
};
