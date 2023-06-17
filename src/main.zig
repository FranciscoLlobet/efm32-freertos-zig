const std = @import("std");
const microzig = @import("microzig");
const freertos = @import("freertos.zig");

const c_board = @cImport({
    @cDefine("EFM32GG390F1024", "1");
    @cDefine("__PROGRAM_START", "__main");
    @cDefine("SL_CATALOG_POWER_MANAGER_PRESENT", "1");
    @cInclude("board.h");
    @cInclude("sl_simple_led.h");
    @cInclude("uiso_config.h");
});

extern fn xPortSysTickHandler() callconv(.C) void;

export fn vApplicationMallocFailedHook() void {}
export fn vApplicationIdleHook() void {}
export fn vApplicationDaemonTaskStartupHook() void {
    c_board.sl_led_turn_on(&c_board.led_red);

    c_board.BOARD_msDelay(500);

    c_board.sl_led_turn_on(&c_board.led_orange);

    c_board.BOARD_msDelay(500);

    c_board.sl_led_turn_on(&c_board.led_yellow);

    c_board.BOARD_msDelay(500);

    c_board.sl_led_turn_off(&c_board.led_red);

    c_board.BOARD_msDelay(500);

    c_board.sl_led_turn_off(&c_board.led_orange);

    c_board.BOARD_msDelay(500);

    c_board.sl_led_turn_off(&c_board.led_yellow);

    //c_board.uiso_load_config();
}

export fn vApplicationStackOverflowHook() void {}
export fn vApplicationTickHook() void {}
extern fn microzig_main() noreturn;

extern fn PendSV_Handler() void;
extern fn SVC_Handler() void;

pub const microzig_options = struct {
    pub const interrupts = struct {
        pub fn GPIO_EVEN() void {
            c_board.GPIO_EVEN_IRQHandler();
        }
        pub fn GPIO_ODD() void {
            c_board.GPIO_ODD_IRQHandler();
        }
        pub fn RTC() void {
            c_board.RTC_IRQHandler();
        }
        pub fn DMA() void {
            c_board.DMA_IRQHandler();
        }
        pub fn SysTick() void {
            if (freertos.c.taskSCHEDULER_NOT_STARTED != freertos.c.xTaskGetSchedulerState()) {
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
        // pub fn EMU() void {
        //     c_board.EMU_IRQHandler();
        // }
        //pub fn I2C0() void {
        //    c_board.I2C0_IRQHandler();
        //}
        //pub fn I2C1() void {
        //    c_board.I2C1_IRQHandler();
        //}
        //pub fn USART0_RX() void {
        //    c_board.USART0_RX_IRQHandler();
        // }
        //pub fn USART0_TX() void {
        //    c_board.USART0_TX_IRQHandler();
        //}
        //pub fn USART1_RX() void {
        //    c_board.USART1_RX_IRQHandler();
        //}
        //pub fn USART1_TX() void {
        //    c_board.USART1_TX_IRQHandler();
        //}
    };
};

// Types

var my_user_task: freertos.Task = undefined;
var my_user_task_queue: freertos.Queue = undefined;
const stack_Depth: u16 = 2000;
const task_name: [*c]const u8 = "TestTask";
const taskPriority: freertos.c.UBaseType_t = 3;
var my_timer: freertos.Timer = undefined;
const timer_name: [*c]const u8 = "TestTimer";

fn myUserTaskFunction(pvParameters: ?*anyopaque) callconv(.C) void {
    _ = pvParameters;

    c_board.uiso_load_config();

    while (true) {
        var test_var: u32 = 0;

        if (my_user_task_queue.receive(@ptrCast(*void, &test_var), freertos.c.portMAX_DELAY)) {
            c_board.sl_led_toggle(&c_board.led_yellow);
        }
    }
}

fn myTimerFunction(xTimer: freertos.c.TimerHandle_t) callconv(.C) void {
    _ = xTimer;
    var test_var: u32 = 0xAA55;

    _ = my_user_task_queue.send(@ptrCast(*void, &test_var), freertos.c.portMAX_DELAY);
}

pub export fn main() void {
    c_board.BOARD_Init();

    my_user_task.init(myUserTaskFunction, task_name, stack_Depth, null, taskPriority) catch unreachable;

    my_user_task_queue.init(4, 1) catch unreachable;

    my_timer.create(timer_name, 2000, freertos.c.pdTRUE, null, myTimerFunction) catch unreachable;

    my_timer.start(null) catch unreachable;

    freertos.vTaskStartScheduler();
}
