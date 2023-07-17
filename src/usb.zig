const std = @import("std");
const board = @import("microzig").board;
const freertos = @import("freertos.zig");

const c = @cImport({
    @cInclude("board.h");
});

export fn callback() callconv(.C) void {
    var source = c.USBX_getCallbackSource();
    var pxHigherPriorityTaskWoken = freertos.pdFALSE;

    if (source == c.USBX_TX_COMPLETE) {
        _ = usb.tx_semaphore.giveFromIsr(&pxHigherPriorityTaskWoken);
    }

    freertos.portYIELD_FROM_ISR(pxHigherPriorityTaskWoken);
}

const Usb = struct {
    tx_semaphore: freertos.Semaphore,
    rx_semaphore: freertos.Semaphore,

    pub fn init(self: *@This()) void {
        self.tx_semaphore.createMutex() catch unreachable;
        self.rx_semaphore.createMutex() catch unreachable;

        c.USBX_apiCallbackEnable(callback);
    }
    pub fn write(self: *@This(), block: [*c]const u8, numBytes: usize) usize {
        var countPtr: u32 = 0;
        _ = self;
        //_ = self.tx_semaphore.take(0);

        var res = c.USBX_blockWrite(@constCast(block), @intCast(numBytes), &countPtr);

        _ = res;
        // _ = self.tx_semaphore.take(null);

        return @intCast(countPtr);
    }
};

pub var usb: Usb = undefined;
