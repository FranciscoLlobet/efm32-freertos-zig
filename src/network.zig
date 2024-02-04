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

pub fn start() void {
    // Create the wifi service state machine
    c.create_wifi_service_task();

    // Create the network service mediator
    _ = c.create_network_mediator();

    // Create the SimpleLink Spawn task
    _ = c.VStartSimpleLinkSpawnTask(@as(c_ulong, c.miso_task_connectivity_service));

    // Create the LWM2M service
    lwm2m.service.create();

    // Create the MQTT service
    mqtt.service.create();

    // Create the HTTP service
    http.service.create();
}
