const std = @import("std");
const board = @import("microzig").board;
const freertos = @import("freertos.zig");

const c = @cImport({
    @cInclude("board.h");
});

tx_semaphore: freertos.Mutex,
rx_semaphore: freertos.Mutex,

export fn callback() callconv(.C) void {
    var source = c.USBX_getCallbackSource();
    var pxHigherPriorityTaskWoken = freertos.pdFALSE;

    if (source == c.USBX_TX_COMPLETE) {
        usb.tx_semaphore.giveFromIsr(&pxHigherPriorityTaskWoken) catch {};
    }

    freertos.portYIELD_FROM_ISR(pxHigherPriorityTaskWoken);
}

pub fn init() @This() {
    var self: @This() = undefined;

    self.tx_semaphore = freertos.Mutex.create() catch unreachable;
    self.rx_semaphore = freertos.Mutex.create() catch unreachable;

    c.USBX_apiCallbackEnable(callback);

    return self;
}

pub fn write(self: *@This(), block: []const u8) usize {
    var countPtr: u32 = 0;

    var res = c.USBX_blockWrite(@constCast(block.ptr), @intCast(block.len), &countPtr);
    _ = res;
    _ = self;

    return @intCast(countPtr);
}

pub var usb: @This() = undefined;
