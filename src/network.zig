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
    task: freertos.StaticTask(@This(), 900, "SimpleLinkSpawn", run),
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
        defer freertos.portYIELD();

        if (flags == 1) {
            try self.queue.send(&msg, 0);
        } else {
            try self.queue.send(&msg, null);
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
