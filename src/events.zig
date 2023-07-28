// Event manager
const config = @import("config.zig");
const freertos = @import("freertos.zig");

fn executor(param1: ?*anyopaque, param2: u32) callconv(.C) void {
    _ = param2;
    var self: @This() = @as(*@This(), @ptrCast(@alignCast(param1)));
    _ = self;
}
