const std = @import("std");
const microzig = @import("microzig");
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

const board = microzig.board;

// Enable or Disable features at compile time
pub const enable_lwm2m = true;
pub const enable_mqtt = false;
pub const enable_http = true;

extern fn xPortSysTickHandler() callconv(.C) void;

extern fn vPortSVCHandler() callconv(.Naked) void;
extern fn xPortPendSVHandler() callconv(.Naked) void;

export fn __stack_chk_fail() callconv(.C) noreturn {
    microzig.hang();
}

export fn vApplicationIdleHook() void {
    board.watchdogFeed();
}

/// Export the NVM3 handle
pub export const miso_nvm3_handle = &nvm.miso_nvm3;
pub export const miso_nvm3_init_handle = &nvm.miso_nvm3_init;

fn dummyTask(self: *@This()) void {
    _ = self;
    unreachable;
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

export fn hang() callconv(.C) void {
    microzig.hang();
}

pub const microzig_options = struct {
    pub const interrupts = struct {
        pub fn GPIO_EVEN() void {
            c.GPIO_EVEN_IRQHandler();
        }
        pub fn GPIO_ODD() void {
            c.GPIO_ODD_IRQHandler();
        }
        pub fn RTC() void {
            c.RTC_IRQHandler();
        }
        pub fn DMA() void {
            c.DMA_IRQHandler();
        }
        pub fn I2C0() void {
            c.I2C0_IRQHandler();
        }
        pub fn USB() void {
            //c.USB_IRQHandler();
        }
        pub fn TIMER0() void {
            c.TIMER0_IRQHandler();
        }
        pub fn SysTick() void {
            if (.taskSCHEDULER_NOT_STARTED != freertos.xTaskGetSchedulerState()) {
                xPortSysTickHandler();
            }
        }
        /// Redirecting the PendSV to the FreeRTOS handler
        pub const PendSV = xPortPendSVHandler;

        /// Redirecting the SVCall to the FreeRTOS handler
        pub const SVCall = vPortSVCHandler;

        pub fn HardFault() void {
            microzig.hang(); //c_board.BOARD_MCU_Reset();
        }
        pub fn MemManageFault() void {
            microzig.hang(); //c_board.BOARD_MCU_Reset();
        }
        pub fn BusFault() void {
            microzig.hang();
        }
        pub fn UsageFault() void {
            microzig.hang();
        }
    };
};

pub export fn system_reset() callconv(.C) void {
    system.reset();
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

//extern fn __stack_chk_init() callconv(.C) void;
extern fn SystemInit() callconv(.C) void;

pub export fn init() void {
    SystemInit();
}

// Button On-Change Callback
pub export fn sl_button_on_change(handle: buttons.button_handle) callconv(.C) void {
    const instance = buttons.getInstance(handle);
    const state = instance.getState();
    switch (instance.getName()) {
        .button1 => {
            switch (state) {
                .pressed => {
                    leds.red.on();
                },
                .released => {
                    leds.red.off();
                },
                else => {},
            }
        },
        .button2 => {
            switch (state) {
                .pressed => {
                    leds.orange.on();
                },
                .released => {
                    leds.orange.off();
                },
                else => {},
            }
        },
    }
}
