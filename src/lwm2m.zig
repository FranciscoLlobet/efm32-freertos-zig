const std = @import("std");
const board = @import("microzig").board;
const freertos = @import("freertos.zig");
const config = @import("config.zig");
const system = @import("system.zig");
const mqtt = @import("mqtt.zig");

const c = @cImport({
    @cInclude("network.h");
    @cInclude("wifi_service.h");
    @cInclude("lwm2m_client.h");
});

extern fn write_temperature(temperature: f32) callconv(.C) void;

task: freertos.Task,
reg_update: freertos.Timer,
timer_update: freertos.Timer,

fn taskFunction(pvParameters: ?*anyopaque) callconv(.C) void {
    var self = freertos.getAndCastPvParameters(@This(), pvParameters);
    var ret: i32 = 0;
    self.task.suspendTask();
    self.reg_update.start(null) catch unreachable;
    self.timer_update.start(null) catch unreachable;

    while (ret == 0) {
        ret = c.lwm2m_client_task_runner(self);

        board.msDelay(60 * 1000); // Retry in 60 seconds
    }
    system.reset();
}

fn dummyTaskFunction(pvParameters: ?*anyopaque) callconv(.C) void {
    var self = freertos.getAndCastPvParameters(@This(), pvParameters);

    while (true) {
        self.task.suspendTask();
    }
}

// test conditional compilation
const lwm2mTaskFunction = if (config.enable_lwm2m) taskFunction else dummyTaskFunction;

fn reg_update_function(xTimer: freertos.TimerHandle_t) callconv(.C) void {
    _ = freertos.getAndCastTimerID(@This(), xTimer).task.notify(c.lwm2m_notify_registration, .eSetBits);
}

fn timer_update_function(xTimer: freertos.TimerHandle_t) callconv(.C) void {
    _ = freertos.getAndCastTimerID(@This(), xTimer).task.notify(c.lwm2m_notify_timestamp, .eSetBits);
}

pub fn create(self: *@This()) void {
    self.task.create(lwm2mTaskFunction, "lwm2m", config.rtos_stack_depth_lwm2m, self, config.rtos_prio_lwm2m) catch unreachable;

    if (config.enable_lwm2m) {
        self.reg_update.create("lwm2m_reg_update", (5 * 60 * 1000), freertos.pdTRUE, self, reg_update_function) catch unreachable;
        self.timer_update.create("lwm2m_timer_update", (60 * 1000), freertos.pdTRUE, self, timer_update_function) catch unreachable;
    }
}

pub fn updateTemperature(self: *@This(), temperature_in_celsius: f32) void {
    if (config.enable_lwm2m) {
        write_temperature(temperature_in_celsius);
        _ = self.task.notify(c.lwm2m_notify_temperature, .eSetBits);
    }
}

pub fn getTaskHandle(self: *@This()) freertos.TaskHandle_t {
    return self.task.getHandle();
}

pub var service: @This() = undefined;
