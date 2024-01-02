const std = @import("std");
const microzig = @import("microzig");
const freertos = @import("freertos.zig");
const system = @import("system.zig");
const sensors = @import("sensors.zig");
const network = @import("network.zig");
const config = @import("config.zig");
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
    // @cInclude("sl_simple_led.h");
    @cInclude("miso_config.h");
});

const board = microzig.board;

extern fn xPortSysTickHandler() callconv(.C) void;

export fn vApplicationIdleHook() void {
    board.watchdogFeed();
}

/// Export the NVM3 handle
pub export const miso_nvm3_handle = &nvm.miso_nvm3;
pub export const miso_nvm3_init_handle = &nvm.miso_nvm3_init;

var timerTask: freertos.StaticTask(config.rtos_stack_depth_timer_task) = undefined;
var idleTask: freertos.StaticTask(config.rtos_stack_depth_idle_task) = undefined;

export fn vApplicationGetTimerTaskMemory(ppxTimerTaskTCBBuffer: **freertos.StaticTask_t, ppxTimerTaskStackBuffer: **freertos.StackType_t, pulTimerTaskStackSize: *c_uint) callconv(.C) void {
    ppxTimerTaskTCBBuffer.* = &timerTask.staticTask;
    ppxTimerTaskStackBuffer.* = &timerTask.stack[0];
    pulTimerTaskStackSize.* = timerTask.stack.len;
}

export fn vApplicationGetIdleTaskMemory(ppxIdleTaskTCBBuffer: **freertos.StaticTask_t, ppxIdleTaskStackBuffer: **freertos.StackType_t, pulIdleTaskStackSize: *c_uint) callconv(.C) void {
    ppxIdleTaskTCBBuffer.* = &idleTask.staticTask;
    ppxIdleTaskStackBuffer.* = &idleTask.stack[0];
    pulIdleTaskStackSize.* = idleTask.stack.len;
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
        pub fn PendSV() callconv(.Naked) void {
            asm volatile (
                \\mrs r0, psp
                \\isb
                \\
                \\ldr r3, pxCurrentTCBConst
                \\ldr r2, [r3]
                \\
                \\stmdb r0!, {r4-r11}
                \\str r0, [r2]
                \\
                \\stmdb sp!, {r3, r14}
                \\mov r0, %[priority]
                \\msr basepri, r0
                \\bl vTaskSwitchContext
                \\mov r0, #0
                \\msr basepri, r0
                \\ldmia sp!, {r3, r14}
                \\
                \\ldr r1, [r3]
                \\ldr r0, [r1]
                \\ldmia r0!, {r4-r11}
                \\msr psp, r0
                \\isb
                \\bx r14
                \\
                \\.align 4
                \\pxCurrentTCBConst: .word pxCurrentTCB
                :
                : [priority] "i" (freertos.c.configMAX_SYSCALL_INTERRUPT_PRIORITY),
            );
        }
        pub fn SVCall() callconv(.Naked) void {
            // Copy pasta the assembler code since the call to the naked function was not working
            asm volatile (
                \\ tst lr, #4                
                \\ ite eq                    
                \\ mrseq r0, msp             
                \\ mrsne r0, psp             
                \\ ldr r1, [r0, #24]   
                \\ ldrb r1, [r1, #-2]  
                \\ cmp r1, #0
                \\ beq SVC_Handler
                \\ cmp r1, #7
                \\ beq SVC7_Handler
                \\
                \\ bx lr
            );
        }
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

pub export fn SVC7_Handler() callconv(.Naked) void {
    asm volatile (
        \\nop
    );
}

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

// Initialisation of the C runtime.

extern fn __libc_init_array() callconv(.C) void;
extern fn SystemInit() callconv(.C) void;

pub export fn init() void {
    __libc_init_array();

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
