const std = @import("std");
const board = @import("microzig").board;
const freertos = @import("freertos.zig");
const config = @import("config.zig");

const c = @cImport({
    @cDefine("EFM32GG390F1024", "1");
    @cDefine("__PROGRAM_START", "__main");
    @cInclude("network.h");
    @cInclude("wifi_service.h");
});

pub fn start() void {
    c.create_wifi_service_task();
    _ = c.create_network_mediator();
    _ = c.VStartSimpleLinkSpawnTask(@as(c_ulong, c.uiso_task_connectivity_service));
}
