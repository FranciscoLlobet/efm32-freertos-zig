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

const c = @cImport({
    @cInclude("board.h");
});

tx_semaphore: freertos.StaticBinarySemaphore(),
rx_semaphore: freertos.StaticBinarySemaphore(),
txLen: usize,

pub export fn usb_write_char(wc: u8) callconv(.C) void {
    usb.writeChar(wc);
}

export fn callback() callconv(.C) void {
    var source = c.USBX_getCallbackSource();
    var pxHigherPriorityTaskWoken = freertos.pdFALSE;

    // USB Reset Interrupt has occurred
    if ((source & c.USBX_RESET) == c.USBX_RESET) {
        //
    }

    // Transmit Complete Interrupt has occurred
    if ((source & c.USBX_TX_COMPLETE) == c.USBX_TX_COMPLETE) {
        usb.tx_semaphore.giveFromIsr(&pxHigherPriorityTaskWoken) catch {};
    }

    // Receive Complete Interrupt has occurred
    if ((source & c.USBX_RX_COMPLETE) == c.USBX_RX_COMPLETE) {
        usb.rx_semaphore.giveFromIsr(&pxHigherPriorityTaskWoken) catch {};
    }

    // Receive and Transmit FIFO's were purged
    if ((source & c.USBX_FIFO_PURGE) == c.USBX_FIFO_PURGE) {
        //
    }

    // Device Instance Opened on host side
    if ((source & c.USBX_DEV_OPEN) == c.USBX_DEV_OPEN) {
        //
    }

    // Device Instance Closed on host side
    if ((source & c.USBX_DEV_CLOSE) == c.USBX_DEV_CLOSE) {
        //
    }

    // Device has entered configured state
    if ((source & c.USBX_DEV_CONFIGURED) == c.USBX_DEV_CONFIGURED) {
        //
    }

    // USB suspend signaling present on bus
    if ((source & c.USBX_DEV_SUSPEND) == c.USBX_DEV_SUSPEND) {
        //
    }

    // Data received with no place to put it
    if ((source & c.USBX_RX_OVERRUN) == c.USBX_RX_OVERRUN) {
        //
    }

    freertos.portYIELD_FROM_ISR(pxHigherPriorityTaskWoken);
}

pub fn init(self: *@This()) void {
    self.tx_semaphore.create() catch unreachable;
    self.rx_semaphore.create() catch unreachable;

    c.USBX_apiCallbackEnable(callback);
}

pub fn writeChar(self: *@This(), wc: u8) void {
    self.txLen = 0;

    const t: u8 align(4) = wc;
    _ = c.USBX_blockWrite(@constCast(&t), 1, &self.txLen);
    _ = self.tx_semaphore.take(null) catch unreachable;
}

pub fn write(self: *@This(), block: []const u8) usize {
    self.txLen = 0;

    var count: usize = 0;
    while (count < block.len) {
        const t: u8 align(4) = block[count];

        _ = c.USBX_blockWrite(@constCast(&t), 1, &self.txLen);
        _ = self.tx_semaphore.take(null) catch unreachable;
        count += 1;
    }

    return @intCast(0);
}

pub var usb: @This() = undefined;
