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

task: freertos.StaticTask(@This(), config.rtos_stack_depth_sensor, "sensor", sensorSampingTask) = undefined,
timer: freertos.StaticTimer(@This(), "sensor_timer", tempTimerCallback) = undefined,

fn tempTimerCallback(self: *@This()) void {
    self.task.notify(1, .eSetBits) catch {};
}

fn sensorSampingTask(self: *@This()) noreturn {
    var bme280_sensor = bme280.init(@ptrCast(board.bme280_dev)) catch unreachable;
    var bma280_sensor = bma280.init(@ptrCast(board.bma280_dev)) catch unreachable;
    _ = bma280_sensor;

    while (true) {
        var temp_data: bme280.bme280_data = undefined;

        if (self.task.waitForNotify(0, 0xFFFFFFFF, freertos.portMAX_DELAY) catch unreachable) |notification_value| {
            if (notification_value == 1) {
                bme280_sensor.readInForcedMode(&temp_data) catch unreachable;
                // Notify to event system
                _ = c.printf("Temp: %d, Stack: %d\n\r", @as(i32, @intFromFloat(100.0 * temp_data.temperature)), self.task.getStackHighWaterMark());

                network.lwm2m.service.updateTemperature(@as(f32, @as(f32, @floatCast(temp_data.temperature))));
            }
        }
    }
}

pub fn init(self: *@This()) !void {
    self.task.create(self, config.rtos_prio_sensor) catch unreachable;
    self.timer.create(1000, true, self) catch unreachable;
    self.timer.start(null) catch unreachable;
}

pub var service: @This() = undefined;
