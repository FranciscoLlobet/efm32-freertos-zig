const std = @import("std");
const board = @import("microzig").board;
const freertos = @import("freertos.zig");
const config = @import("config.zig");
const system = @import("system.zig");
const mqtt = @import("mqtt.zig");

const c = @cImport({
    @cDefine("EFM32GG390F1024", "1");
    @cDefine("__PROGRAM_START", "__main");
    @cInclude("network.h");
    @cInclude("wifi_service.h");
    @cInclude("lwm2m_client.h");
});

extern fn write_temperature(temperature: f32) callconv(.C) void;

// LWM2M Wrapper
pub const lwm2m = struct {
    task: freertos.Task,
    reg_update: freertos.Timer,
    timer_update: freertos.Timer,

    fn task_function(pvParameters: ?*anyopaque) callconv(.C) void {
        var self: *@This() = @ptrCast(*@This(), @alignCast(@alignOf(*@This()), pvParameters));
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

    fn reg_update_function(xTimer: freertos.TimerHandle_t) callconv(.C) void {
        _ = @as(*@This(), @ptrCast(*@This(), @alignCast(@alignOf(*@This()), freertos.c.pvTimerGetTimerID(xTimer)))).task.notify((1 << 0), freertos.eNotifyAction.eSetBits);
    }
    fn timer_update_function(xTimer: freertos.TimerHandle_t) callconv(.C) void {
        _ = @as(*@This(), @ptrCast(*@This(), @alignCast(@alignOf(*@This()), freertos.c.pvTimerGetTimerID(xTimer)))).task.notify((1 << 1), freertos.eNotifyAction.eSetBits);
    }

    pub fn create(self: *@This()) void {
        self.task.create(task_function, "lwm2m", config.rtos_stack_depth_lwm2m, self, config.rtos_prio_lwm2m) catch unreachable;
        self.reg_update.create("lwm2m_reg_update", (5 * 60 * 1000), freertos.pdTRUE, self, reg_update_function) catch unreachable;
        self.timer_update.create("lwm2m_timer_update", (60 * 1000), freertos.pdTRUE, self, timer_update_function) catch unreachable;
    }

    pub fn updateTemperature(self: *@This(), temperature_in_celsius: f32) void {
        write_temperature(temperature_in_celsius);
        _ = self.task.notify(c.lwm2m_notify_temperature, freertos.eNotifyAction.eSetBits);
    }

    pub fn getTaskHandle(self: *@This()) freertos.TaskHandle_t {
        return self.task.getHandle();
    }
};

pub var lwm2m_service: lwm2m = lwm2m{ .task = undefined, .reg_update = undefined, .timer_update = undefined };

pub fn start() void {
    // Create the wifi service state machine
    c.create_wifi_service_task();

    // Create the network service mediator
    _ = c.create_network_mediator();

    // Create the SimpleLink Spawn task
    _ = c.VStartSimpleLinkSpawnTask(@as(c_ulong, c.uiso_task_connectivity_service));

    // Create the LWM2M service
    lwm2m_service.create();

    // Create the MQTT service
    mqtt.service.init();
}

pub export fn get_lwm2m_task_handle() callconv(.C) freertos.TaskHandle_t {
    return lwm2m_service.getTaskHandle();
}

pub export fn get_mqtt_task_handle() callconv(.C) freertos.TaskHandle_t {
    return mqtt.service.getTaskHandle();
}

const network_ctx = c.uiso_network_ctx_t;

pub const connection_id = enum(usize) {
    ntp = c.wifi_service_ntp_socket,
    lwm2m = c.wifi_service_lwm2m_socket,
    mqtt = c.wifi_service_mqtt_socket,
    http = c.wifi_service_http_socket,
};

pub const connection = struct {
    ctx: network_ctx,

    pub const protocol = enum(u32) {
        no_protocol = c.uiso_protocol_no_protocol,
        udp_ip4 = c.uiso_protocol_udp_ip4,
        tcp_ip4 = c.uiso_protocol_tcp_ip4,
        udp_ip6 = c.uiso_protocol_udp_ip6,
        tcp_ip6 = c.uiso_protocol_tcp_ip6,
        dtls_ip4 = c.uiso_protocol_dtls_ip4,
        tls_ip4 = c.uiso_protocol_tls_ip4,

        dtls_ip6 = c.uiso_protocol_dtls_ip6,
        tls_ip6 = c.uiso_protocol_tls_ip6,
    };

    pub fn init(id: connection_id) @This() {
        return @This(){ .ctx = c.uiso_get_network_ctx(@intCast(c_uint, @intFromEnum(id))) };
    }

    pub fn create(self: *@This(), host: []const u8, port: []const u8, local_port: ?[]const u8, proto: protocol) i32 {
        var c_local_port: [*c]const u8 = undefined;
        if (local_port) |l_port| {
            c_local_port = @ptrCast([*c]const u8, l_port);
        }

        return @as(i32, c.uiso_create_network_connection(self.ctx, @ptrCast([*c]const u8, host), @ptrCast([*c]const u8, port), c_local_port, @as(c.enum_uiso_protocol, @intFromEnum(proto))));
    }
    pub fn close(self: *@This()) i32 {
        return c.uiso_close_network_connection(self.ctx);
    }
    pub fn send(self: *@This(), buffer: *const u8, length: usize) i32 {
        return c.uiso_network_send(self.ctx, @ptrCast([*c]const u8, buffer), length);
    }
    pub fn recieve(self: *@This(), buffer: *u8, length: usize) i32 {
        return c.uiso_network_read(self.ctx, buffer, length);
    }
    pub fn waitRx(self: *@This(), timeout_s: u32) i32 {
        return c.wait_rx(self.ctx, timeout_s);
    }
    pub fn waitTx(self: *@This(), timeout_s: u32) i32 {
        return c.wait_tx(self.ctx, timeout_s);
    }
};
