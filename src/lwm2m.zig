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

task: freertos.StaticTask(config.rtos_stack_depth_lwm2m),
reg_update: freertos.Timer,
timer_update: freertos.Timer,

fn taskFunction(pvParameters: ?*anyopaque) callconv(.C) void {
    const self = freertos.getAndCastPvParameters(@This(), pvParameters);
    var ret: i32 = 0;

    self.reg_update.start(null) catch unreachable;
    self.timer_update.start(null) catch unreachable;

    while (ret == 0) {
        ret = c.lwm2m_client_task_runner(self);

        self.task.delayTask(60 * 1000);
    }
    system.reset();
}

fn dummyTaskFunction(pvParameters: ?*anyopaque) callconv(.C) void {
    const self = freertos.Task.getAndCastPvParameters(@This(), pvParameters);

    while (true) {
        self.task.suspendTask();
    }
}

fn reg_update_function(xTimer: freertos.TimerHandle_t) callconv(.C) void {
    freertos.getAndCastTimerID(@This(), xTimer).task.notify(c.lwm2m_notify_registration, .eSetBits) catch unreachable;
}

fn timer_update_function(xTimer: freertos.TimerHandle_t) callconv(.C) void {
    freertos.getAndCastTimerID(@This(), xTimer).task.notify(c.lwm2m_notify_timestamp, .eSetBits) catch unreachable;
}

pub fn create(self: *@This()) void {
    self.task.create(if (config.enable_lwm2m) taskFunction else dummyTaskFunction, "lwm2m", self, config.rtos_prio_lwm2m) catch unreachable;

    self.task.suspendTask();

    if (config.enable_lwm2m) {
        self.reg_update = freertos.Timer.create("lwm2m_reg_update", (5 * 60 * 1000), freertos.pdTRUE, self, reg_update_function) catch unreachable;
        self.timer_update = freertos.Timer.create("lwm2m_timer_update", (60 * 1000), freertos.pdTRUE, self, timer_update_function) catch unreachable;
    }
}

pub fn updateTemperature(self: *@This(), temperature_in_celsius: f32) void {
    if (config.enable_lwm2m) {
        write_temperature(temperature_in_celsius);
        self.task.notify(c.lwm2m_notify_temperature, .eSetBits) catch {};
    }
}

pub fn getTaskHandle(self: *@This()) freertos.TaskHandle_t {
    return self.task.getHandle();
}

pub var service: @This() = undefined;
