const std = @import("std");
const board = @import("microzig").board;
const freertos = @import("freertos.zig");
const config = @import("config.zig");
const system = @import("system.zig");
const mqtt = @import("mqtt.zig");
const http = @import("http.zig");
pub const lwm2m = @import("lwm2m.zig");

const c = @cImport({
    @cInclude("network.h");
    @cInclude("wifi_service.h");
    @cInclude("lwm2m_client.h");
});

/// SimpleLink Spawn Queue Executor
const simpleLinkSpawn = struct {
    task: freertos.StaticTask(@This(), 900, "SimpleLinkSpawnTask", run),
    queue: freertos.StaticQueue(c.tSimpleLinkSpawnMsg, 4),
    stack: usize,

    /// Initialize the SimpleLink Spawn task
    fn init(self: *@This()) !void {
        try self.task.create(self, @as(c_ulong, 5));
        try self.queue.create();
    }

    /// SimpleLink Spawn task
    fn run(self: *@This()) noreturn {
        while (true) {
            if (self.queue.recieve(null)) |msg| {
                _ = msg.pEntry.?(msg.pValue);
            }
        }
    }

    /// Send a message to the SimpleLink Spawn task
    fn sendMsg(self: *@This(), pEntry: c.P_OSI_SPAWN_ENTRY, pValue: ?*anyopaque, flags: c_ulong) !void {
        const msg: c.tSimpleLinkSpawnMsg = .{ .pEntry = pEntry, .pValue = pValue };
        var xHigherPriorityTaskWoken: freertos.BaseType_t = freertos.pdFALSE;

        if (flags == 1) {
            defer freertos.portYIELD_FROM_ISR(xHigherPriorityTaskWoken);

            try self.queue.sendFromIsr(&msg, &xHigherPriorityTaskWoken);
        } else {
            try self.queue.send(&msg, null);
            defer freertos.portYIELD();
        }
    }
};

export fn osi_Spawn(pEntry: c.P_OSI_SPAWN_ENTRY, pValue: ?*anyopaque, flags: c_ulong) callconv(.C) c.OsiReturnVal_e {
    simpleLinkSpawnTask.sendMsg(pEntry, pValue, flags) catch return c.OSI_OPERATION_FAILED;

    return c.OSI_OK;
}

var simpleLinkSpawnTask: simpleLinkSpawn = undefined;

pub fn start() void {
    // Create the wifi service state machine
    c.create_wifi_service_task();

    // Create the network service mediator
    _ = c.create_network_mediator();

    // Create the SimpleLink Spawn task
    simpleLinkSpawnTask.init() catch unreachable;

    // Create the LWM2M service
    lwm2m.service.create();

    // Create the MQTT service
    mqtt.service.create();

    // Create the HTTP service
    http.service.create();
}
