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

task: freertos.StaticTask(@This(), config.rtos_stack_depth_lwm2m, "lwm2m", if (config.enable_lwm2m) taskFunction else dummyTaskFunction),
reg_update: freertos.StaticTimer(@This(), "lwm2m_reg_update", reg_update_function),
timer_update: freertos.StaticTimer(@This(), "lwm2m_timer_update", timer_update_function),

fn taskFunction(self: *@This()) void {
    var ret: i32 = 0;

    self.reg_update.start(null) catch unreachable;
    self.timer_update.start(null) catch unreachable;

    while (ret == 0) {
        ret = c.lwm2m_client_task_runner(self);

        self.task.delayTask(60 * 1000);
    }
    system.reset();
}

fn dummyTaskFunction(self: *@This()) void {
    while (true) {
        self.task.suspendTask();
    }
}

fn reg_update_function(self: *@This()) void {
    self.task.notify(c.lwm2m_notify_registration, .eSetBits) catch {};
}

fn timer_update_function(self: *@This()) void {
    self.task.notify(c.lwm2m_notify_timestamp, .eSetBits) catch {};
}

pub fn create(self: *@This()) void {
    self.task.create(self, config.rtos_prio_lwm2m) catch unreachable;
    self.task.suspendTask();

    if (config.enable_lwm2m) {
        self.reg_update.create((1 * 60 * 1000), true, self) catch unreachable;
        self.timer_update.create((60 * 1000), true, self) catch unreachable;
    }
}

pub fn updateTemperature(self: *@This(), temperature_in_celsius: f32) void {
    if (config.enable_lwm2m) {
        write_temperature(temperature_in_celsius);
        self.task.notify(c.lwm2m_notify_temperature, .eSetBits) catch {};
    }
}

/// Send suspend Task signal to LwM2M task
pub fn suspendTask(self: *@This()) void {
    self.task.notify(c.lwm2m_notify_suspend, .eSetBits) catch {};
}

pub fn getTaskHandle(self: *@This()) freertos.TaskHandle_t {
    return self.task.getHandle();
}

pub var service: @This() = undefined;
