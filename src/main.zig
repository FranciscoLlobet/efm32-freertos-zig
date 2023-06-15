const std = @import("std");
const microzig = @import("microzig");

const c_board = @cImport({
    @cDefine("EFM32GG390F1024", "1");
    @cDefine("__PROGRAM_START", "__main");
    @cDefine("SL_CATALOG_POWER_MANAGER_PRESENT", "1");
    @cInclude("board.h");
    @cInclude("sl_simple_led.h");
});

const free_rtos = @cImport({
    @cDefine("EFM32GG390F1024", "1");
    @cDefine("__PROGRAM_START", "__main");
    @cInclude("FreeRTOS.h");
    @cInclude("task.h");
    @cInclude("timers.h");
    @cInclude("semphr.h");
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
            if (free_rtos.taskSCHEDULER_NOT_STARTED != free_rtos.xTaskGetSchedulerState()) {
                xPortSysTickHandler();
            }
        }
        pub fn PendSV() void {
            free_rtos.PendSV_Handler();
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

pub export fn main() void {
    c_board.BOARD_Init();

    free_rtos.vTaskStartScheduler();
}
