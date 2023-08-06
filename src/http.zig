const std = @import("std");
const freertos = @import("freertos.zig");
const config = @import("config.zig");
const system = @import("system.zig");
const connection = @import("connection.zig");

connection: connection,
task: freertos.Task,
timer: freertos.Timer,

fn authCallback(self: *connection.mbedtls, security_mode: connection.security_mode) i32 {
    _ = self;
    _ = security_mode;

    return 0;
}

fn taskFunction(pvParameters: ?*anyopaque) callconv(.C) void {
    var self = freertos.getAndCastPvParameters(@This(), pvParameters);

    while (true) {
        self.task.suspendTask();
    }
}

fn dummyTaskFunction(pvParameters: ?*anyopaque) callconv(.C) void {
    var self = freertos.getAndCastPvParameters(@This(), pvParameters);

    while (true) {
        self.task.suspendTask();
    }
}

pub fn create(self: *@This()) void {
    self.task.create(if (config.enable_http) taskFunction else dummyTaskFunction, "http", config.rtos_stack_depth_http, self, config.rtos_prio_http) catch unreachable;
    self.task.suspendTask();

    if (config.enable_http) {
        self.connection = connection.init(.http, authCallback);
    }
}

pub var service: @This() = undefined;
