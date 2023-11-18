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
    @cInclude("sl_simple_led.h");
    @cInclude("miso_config.h");
});

const board = microzig.board;

extern fn xPortSysTickHandler() callconv(.C) void;

export fn vApplicationIdleHook() void {
    board.watchdogFeed();
}

var xTimerTaskTCP: freertos.StaticTask_t = undefined;
var xIdleTaskTCP: freertos.StaticTask_t = undefined;

var uxTimerTaskStack: [config.rtos_stack_depth_timer_task]freertos.StackType_t align(@alignOf(freertos.StackType_t)) = undefined;
var uxIdleTaskStack: [config.rtos_stack_depth_idle_task]freertos.StackType_t align(@alignOf(freertos.StackType_t)) = undefined;

export fn vApplicationGetTimerTaskMemory(ppxTimerTaskTCBBuffer: **freertos.StaticTask_t, ppxTimerTaskStackBuffer: **freertos.StackType_t, pulTimerTaskStackSize: *c_uint) callconv(.C) void {
    ppxTimerTaskTCBBuffer.* = &xTimerTaskTCP;
    ppxTimerTaskStackBuffer.* = &uxTimerTaskStack[0];
    pulTimerTaskStackSize.* = uxTimerTaskStack.len;
}

export fn vApplicationGetIdleTaskMemory(ppxIdleTaskTCBBuffer: **freertos.StaticTask_t, ppxIdleTaskStackBuffer: **freertos.StackType_t, pulIdleTaskStackSize: *c_uint) callconv(.C) void {
    ppxIdleTaskTCBBuffer.* = &xIdleTaskTCP;
    ppxIdleTaskStackBuffer.* = &uxIdleTaskStack[0];
    pulIdleTaskStackSize.* = uxIdleTaskStack.len;
}

export fn vApplicationDaemonTaskStartupHook() void {
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
                \\ldr r3, pxCurrentTCBConst2  /* Restore the context. */
                \\ldr r1, [r3]                /* Use pxCurrentTCBConst to get the pxCurrentTCB address. */
                \\ldr r0, [r1]                /* The first item in pxCurrentTCB is the task top of stack. */
                \\ldmia r0!, {r4-r11}         /* Pop the registers that are not automatically saved on exception entry and the critical nesting count. */
                \\msr psp, r0                 /* Restore the task stack pointer. */
                \\isb                         
                \\mov r0, #0                  
                \\msr basepri, r0             
                \\orr r14, #0xd               
                \\bx r14                      
                \\                            
                \\.align 4                  
                \\pxCurrentTCBConst2: .word pxCurrentTCB
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

pub export fn system_reset() callconv(.C) void {
    system.reset();
}

pub export const miso_nvm3_handle = &nvm.miso_nvm3;
pub export const miso_nvm3_init_handle = &nvm.miso_nvm3_init;

pub export fn miso_notify_event(event: c.miso_event) callconv(.C) void {
    user.user_task.task.notify(event, .eSetBits) catch {};
}

pub export fn main() void {
    board.init();

    const appCounter = nvm.incrementAppCounter() catch 0;

    user.user_task.create();

    sensors.service.init() catch unreachable;

    network.start();

    _ = c.printf("--- MISO starting FreeRTOS %d---\n\r", appCounter);

    freertos.vTaskStartScheduler();

    unreachable;
}

/// Mini heap used for sbrk. Mainly used by printf()
var mini_heap: [1050]u8 align(@alignOf(u32)) = undefined;
var heap_end align(@alignOf(u32)) = &mini_heap[0];

/// Current Zig-based implementation of the sbrk() function
pub export fn _sbrk(inc: isize) callconv(.C) ?*anyopaque {
    if (heap_end == undefined) {
        heap_end = &mini_heap[0];
    }

    const prev_heap_end: isize = @intCast(@intFromPtr(heap_end));
    const new_heap_end: isize = prev_heap_end + inc;

    if (new_heap_end > @as(isize, @intCast(@intFromPtr(&mini_heap[mini_heap.len - 1])))) {
        return null;
    }

    // Update the heap end
    heap_end = @ptrFromInt(@as(usize, @intCast(new_heap_end)));

    return @as(*anyopaque, @ptrCast(@as(@TypeOf(heap_end), @ptrFromInt(@as(usize, @intCast(prev_heap_end))))));
}
