const std = @import("std");
const microzig = @import("microzig");

const c_board = @cImport({
    @cDefine("EFM32GG390F1024", "1");
    @cDefine("__PROGRAM_START", "__main");
    @cDefine("SL_CATALOG_POWER_MANAGER_PRESENT", "1");
    @cInclude("board.h");
    @cInclude("sl_simple_led.h");
    @cInclude("uiso_config.h");
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
            if (free_rtos.taskSCHEDULER_NOT_STARTED != free_rtos.xTaskGetSchedulerState()) {
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
                : [priority] "i" (free_rtos.configMAX_SYSCALL_INTERRUPT_PRIORITY),
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
pub const BaseType_t = free_rtos.BaseType_t;
pub const TaskFunction_t = free_rtos.TaskFunction_t;
pub const UBaseType_t = free_rtos.UBaseType_t;
pub const TaskHandle_t = free_rtos.TaskHandle_t;
pub const QueueHandle_t = free_rtos.QueueHandle_t;
pub const TimerHandle_t = free_rtos.TimerHandle_t;
pub const TickType_t = free_rtos.TickType_t;
const TimerCallbackFunction_t = free_rtos.TimerCallbackFunction_t;

pub const task = struct {
    task_handle: TaskHandle_t,
    pub fn xTaskCreate(self: *task, pxTaskCode: TaskFunction_t, pcName: [*c]const u8, usStackDepth: u16, pvParameters: ?*anyopaque, uxPriority: UBaseType_t) BaseType_t {
        return free_rtos.xTaskCreate(pxTaskCode, pcName, usStackDepth, pvParameters, uxPriority, &self.task_handle);
    }
};

pub const Queue = struct {
    handle: QueueHandle_t,

    pub fn init(item_size: UBaseType_t, queue_length: UBaseType_t) Queue {
        return Queue{ .handle = free_rtos.xQueueCreate(item_size, queue_length) };
    }
    pub fn send(self: *Queue, item_to_queue: *const void, ticks_to_wait: UBaseType_t) BaseType_t {
        return free_rtos.xQueueSend(self.handle, item_to_queue, ticks_to_wait);
    }
    pub fn receive(self: *Queue, buffer: *void, ticks_to_wait: UBaseType_t) BaseType_t {
        return free_rtos.xQueueReceive(self.handle, buffer, ticks_to_wait);
    }
    pub fn delete(self: *Queue) void {
        free_rtos.xQueueDelete(self.handle);
    }
};

pub const Timer = struct {
    handle: TimerHandle_t = undefined,

    pub fn create(self: *Timer, pcTimerName: [*c]const u8, xTimerPeriodInTicks: TickType_t, xAutoReload: BaseType_t, pvTimerID: ?*anyopaque, pxCallbackFunction: TimerCallbackFunction_t) BaseType_t {
        self.handle = free_rtos.xTimerCreate(pcTimerName, xTimerPeriodInTicks, xAutoReload, pvTimerID, pxCallbackFunction);
        if (self.handle == null) {
            return free_rtos.pdFAIL;
        } else {
            return free_rtos.pdPASS;
        }
    }

    //pub fn start(self: *Timer, xTicksToWait: TickType_t) BaseType_t {
    //     return free_rtos.xTimerStart(self.handle, xTicksToWait);
    // }
    pub fn start(self: *Timer, xTicksToWait: TickType_t) BaseType_t {
        var pxHigherPriorityTaskWoken: BaseType_t = undefined;
        return free_rtos.xTimerGenericCommand(self.handle, free_rtos.tmrCOMMAND_START, free_rtos.xTaskGetTickCount(), &pxHigherPriorityTaskWoken, xTicksToWait);
    }

    pub fn stop(self: *Timer, xTicksToWait: TickType_t) BaseType_t {
        return free_rtos.xTimerStop(self.handle, xTicksToWait);
    }

    pub fn delete(self: *Timer, xTicksToWait: TickType_t) BaseType_t {
        return free_rtos.xTimerDelete(self.handle, xTicksToWait);
    }

    pub fn reset(self: *Timer, xTicksToWait: TickType_t) BaseType_t {
        return free_rtos.xTimerReset(self.handle, xTicksToWait);
    }
};

var my_user_task: task = undefined;
var my_user_task_queue: Queue = undefined;
const stack_Depth: u16 = 1600;
const task_name: [*c]const u8 = "TestTask";
const taskPriority: UBaseType_t = 3;
var my_timer: Timer = undefined;
const timer_name: [*c]const u8 = "TestTimer";

fn myUserTaskFunction(pvParameters: ?*anyopaque) callconv(.C) void {
    _ = pvParameters;

    c_board.uiso_load_config();

    while (true) {
        var test_var: u32 = 0;

        if (free_rtos.pdPASS == my_user_task_queue.receive(@ptrCast(*void, &test_var), free_rtos.portMAX_DELAY)) {
            c_board.sl_led_toggle(&c_board.led_yellow);
        }
    }
}

fn myTimerFunction(xTimer: TimerHandle_t) callconv(.C) void {
    _ = xTimer;
    var test_var: u32 = 0xAA55;

    _ = my_user_task_queue.send(@ptrCast(*void, &test_var), free_rtos.portMAX_DELAY);
}

pub export fn main() void {
    c_board.BOARD_Init();

    _ = my_user_task.xTaskCreate(myUserTaskFunction, task_name, stack_Depth, null, taskPriority);
    my_user_task_queue = Queue.init(4, 1);
    _ = my_timer.create(timer_name, 2000, free_rtos.pdTRUE, null, myTimerFunction);

    _ = my_timer.start(free_rtos.portMAX_DELAY);
    free_rtos.vTaskStartScheduler();
}
