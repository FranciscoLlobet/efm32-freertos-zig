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
const microzig = @import("microzig");
const board = microzig.board;

const freertos = @import("freertos.zig");
const system = @import("system.zig");
const sensors = @import("sensors.zig");
const network = @import("network.zig");
const leds = @import("leds.zig");
const buttons = @import("buttons.zig");
const usb = @import("usb.zig");
const user = @import("user.zig");
const events = @import("events.zig");
const fatfs = @import("fatfs.zig");
const nvm = @import("nvm.zig");

const c = @cImport({
    @cInclude("board.h");
    @cInclude("miso.h");
    @cInclude("miso_config.h");
});

// Enable or Disable features at compile time
pub const enable_lwm2m = true;
pub const enable_mqtt = false;
pub const enable_http = true;

/// Export the NVM3 handle
pub export const miso_nvm3_handle = &nvm.miso_nvm3;
pub export const miso_nvm3_init_handle = &nvm.miso_nvm3_init;

/// Redirecting the common system IRQ handlers
pub const microzig_options = system.microzig_options;

extern fn SystemInit() callconv(.C) void;

export fn vApplicationIdleHook() void {
    board.watchdogFeed();
}

export fn vApplicationDaemonTaskStartupHook() void {
    appStart();
}

export fn vApplicationStackOverflowHook() noreturn {
    microzig.hang();
}

export fn vApplicationMallocFailedHook() noreturn {
    microzig.hang();
}

export fn vApplicationTickHook() void {
    //
}

export fn vApplicationGetRandomHeapCanary(pxHeapCanary: [*c]u32) void {
    pxHeapCanary.* = @as(u32, 0xdeadbeef);
}

pub export fn miso_notify_event(event: c.miso_event) callconv(.C) void {
    user.user_task.task.notify(event, .eSetBits) catch {};
}

/// Application entry point
pub export fn main() noreturn {
    board.init();

    const appCounter = nvm.incrementAppCounter() catch 0;

    user.user_task.create();

    sensors.service.init() catch unreachable;

    network.start();

    usb.usb.init(); // Fixme

    _ = c.printf("--- MISO starting FreeRTOS %d---\n\r", appCounter);

    // Start the FreeRTOS scheduler
    freertos.vTaskStartScheduler();

    unreachable;
}

pub export fn appStart() void {
    _ = c.printf("--- FreeRTOS Scheduler Started ---\n\rReset Cause: %d\n\r", board.getResetCause());

    leds.red.on();

    board.msDelay(500);

    leds.orange.on();

    board.msDelay(500);

    leds.yellow.on();

    board.msDelay(500);

    leds.red.off();

    board.msDelay(500);

    leds.orange.off();

    board.msDelay(500);

    leds.yellow.off();

    board.watchdogEnable();
}

pub export fn init() void {
    SystemInit();
}
