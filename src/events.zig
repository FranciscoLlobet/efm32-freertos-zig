// Event manager
const config = @import("config.zig");
const freertos = @import("freertos.zig");
const std = @import("std");

const event_callback = *const fn (param1: ?*anyopaque, param2: u32) callconv(.C) void;

const list_element = struct {
    notify_value: u32,

    task: ?*freertos.Task,
    param1: ?*anyopaque,
};

pub const list = std.SinglyLinkedList(list_element);

fn executor(param1: ?*anyopaque, param2: u32) callconv(.C) void {
    _ = param2;
    var self: @This() = freertos.Task.getAndCastPvParameters(@This(), param1);
    _ = self;
}
