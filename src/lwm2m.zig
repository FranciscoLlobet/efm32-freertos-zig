const std = @import("std");
const board = @import("microzig").board;
const freertos = @import("freertos.zig");
const config = @import("config.zig");
const system = @import("system.zig");
const mqtt = @import("mqtt.zig");
const mbedtls = @import("mbedtls.zig");
const connection = @import("connection.zig");

const c = @cImport({
    @cInclude("network.h");
    @cInclude("wifi_service.h");
    @cInclude("lwm2m_client.h");
    @cInclude("lightclient/lwm2m_security.h");
});

extern fn write_temperature(temperature: f32) callconv(.C) void;

task: freertos.StaticTask(@This(), config.rtos_stack_depth_lwm2m, "lwm2m", if (config.enable_lwm2m) taskFunction else dummyTaskFunction),
reg_update: freertos.StaticTimer(@This(), "lwm2m_reg_update", reg_update_function),
timer_update: freertos.StaticTimer(@This(), "lwm2m_timer_update", timer_update_function),
connection: connection.Connection(.lwm2m, mbedtls.TlsContext(@This(), .psk)),
lwm2m_object: *c.lwm2m_object_t = undefined,
lwm2m_sec_obj_inst_id: u16 = undefined,

export fn lwm2mservice_create_connection(param: ?*anyopaque, uri: [*c]u8, local_port: u16, lwm2m_object: *c.lwm2m_object_t, sec_obj_inst_id: u16) callconv(.C) c_int {
    var self: *@This() = @ptrCast(@alignCast(param));
    const serviceUri = std.Uri.parse(uri[0..c.strlen(uri)]) catch return -1;

    self.lwm2m_object = lwm2m_object;
    self.lwm2m_sec_obj_inst_id = sec_obj_inst_id;

    self.connection.create(serviceUri, local_port) catch return -1;
    return 0;
}

export fn lwm2mservice_close_connection(param: ?*anyopaque) callconv(.C) void {
    @as(*@This(), @ptrCast(@alignCast(param))).connection.close() catch {};
}

export fn lwm2mservice_send_data(param: ?*anyopaque, data: [*c]u8, len: usize) callconv(.C) c_int {
    return @intCast(@as(*@This(), @ptrCast(@alignCast(param))).connection.send(data[0..len]) catch {
        return @as(c_int, -1);
    });
}

export fn lwm2mservice_read_data(param: ?*anyopaque, data: [*c]u8, len: usize) callconv(.C) c_int {
    const ret = @as(*@This(), @ptrCast(@alignCast(param))).connection.recieve(data[0..len]) catch return -1;
    return @intCast(ret.len);
}

/// Authentification callback for mbedTLS connections
fn authCallback(self: *@This(), security_mode: connection.security_mode) mbedtls.auth_error!void {
    if (security_mode == .psk) {
        var public_identity_len: usize = 0;

        var psk_len: usize = 0;
        const psk_ptr: [*]u8 = c.get_connection_psk(self.lwm2m_object, self.lwm2m_sec_obj_inst_id, &psk_len);
        const public_identity: [*:0]u8 = c.get_public_identiy(self.lwm2m_object, self.lwm2m_sec_obj_inst_id, &public_identity_len);

        self.connection.ssl.confPsk(psk_ptr[0..psk_len], public_identity) catch return mbedtls.auth_error.generic_error;
    } else {
        return mbedtls.auth_error.unsuported_mode;
    }
}

fn taskFunction(self: *@This()) void {
    var ret: i32 = 0;

    self.reg_update.start(null) catch unreachable;
    self.timer_update.start(null) catch unreachable;

    while (ret == 0) {
        ret = c.lwm2m_client_task_runner(self);

        if (ret == 1) {
            c.miso_notify_event(c.miso_lwm2m_suspended);
            self.task.suspendTask();
        }

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
        self.reg_update.create((5 * 60 * 1000), true, self) catch unreachable;
        self.timer_update.create((60 * 1000), true, self) catch unreachable;
        self.connection.init();
        self.connection.ssl = @TypeOf(self.connection.ssl).create(self, authCallback, null, null);
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
