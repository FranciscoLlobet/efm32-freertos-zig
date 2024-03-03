const std = @import("std");
const microzig = @import("microzig");
const board = microzig.board;
const freertos = @import("freertos.zig");
const leds = @import("leds.zig");
const fatfs = @import("fatfs.zig");
const nvm = @import("nvm.zig");
const buttons = @import("buttons.zig");

const c = @cImport({
    @cInclude("board.h");
    @cInclude("miso.h");
    @cInclude("miso_config.h");
});

const max_reset_delay: freertos.TickType_t = 5000;

// FreeRTOS Port Handlers
extern fn xPortSysTickHandler() callconv(.C) void;
extern fn vPortSVCHandler() callconv(.Naked) void;
extern fn xPortPendSVHandler() callconv(.Naked) void;

/// C Library Stack Check Fail when Stack Protector is enabled
export fn __stack_chk_fail() callconv(.C) noreturn {
    microzig.hang();
}

pub const time = struct {
    /// Get current time in seconds
    pub fn now() u32 {
        return board.getTime();
    }

    pub fn setTimeFromNtp(timestamp: u32) !void {
        try board.setTimeFromNtp(timestamp);
    }

    pub fn getNtpTime() u32 {
        return board.getNtpTime();
    }

    pub fn calculateDeadline(timeout_s: u32) u32 {
        return now() + timeout_s;
    }
};

fn performReset(param1: ?*anyopaque, param2: u32) callconv(.C) noreturn {
    _ = param2;
    _ = param1;

    leds.red.on();
    leds.yellow.on();
    leds.orange.on();

    // Add code to stop/close the FS
    fatfs.unmount("SD") catch {};

    _ = nvm.incrementResetCounter() catch {};

    freertos.c.taskENTER_CRITICAL();

    board.mcuReset();

    unreachable;
}

pub fn reset() void {
    freertos.xTimerPendFunctionCall(performReset, null, 0, max_reset_delay) catch {
        performReset(null, 0);
    };
}

pub export fn system_reset() callconv(.C) void {
    reset();
}

pub fn shutdown() void {
    reset();
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
            c.USB_IRQHandler();
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
