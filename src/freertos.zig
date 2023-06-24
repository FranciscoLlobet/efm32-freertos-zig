const std = @import("std");

pub const c = @cImport({
    @cDefine("EFM32GG390F1024", "1");
    @cDefine("__PROGRAM_START", "__main");
    @cInclude("FreeRTOS.h");
    @cInclude("task.h");
    @cInclude("timers.h");
    @cInclude("semphr.h");
});

pub const BaseType_t = c.BaseType_t;
pub const TaskFunction_t = c.TaskFunction_t;
pub const UBaseType_t = c.UBaseType_t;
pub const TaskHandle_t = c.TaskHandle_t;
pub const QueueHandle_t = c.QueueHandle_t;
pub const TimerHandle_t = c.TimerHandle_t;
pub const TickType_t = c.TickType_t;
pub const PendedFunction_t = c.PendedFunction_t;

const TimerCallbackFunction_t = c.TimerCallbackFunction_t;

pub const portMAX_DELAY = c.portMAX_DELAY;
pub const pdMS_TO_TICKS = c.pdMS_TO_TICKS;

// Common FreeRTOS constants
pub const pdPASS = c.pdPASS;
pub const pdFAIL = c.pdPASS;
pub const pdTRUE = c.pdTRUE;
pub const pdFALSE = c.pdFALSE;

const FreeRtosError = error{
    pdFAIL,
    TaskCreationFailed,
    TaskHandleAlreadyExists,
    QueueCreationFailed,
    SemaphoreCreationFailed,
    TimerCreationFailed,

    TimerStartFailed,
};

// Kernel function
pub fn vTaskStartScheduler() noreturn {
    c.vTaskStartScheduler();
    unreachable;
}

pub fn vPortMalloc(xSize: usize) ?*anyopaque {
    return c.pvPortMalloc(xSize);
}

pub fn vPortFree(pv: ?*anyopaque) void {
    c.vPortFree(pv);
}

pub fn vTaskDelay(xTicksToDelay: TickType_t) void {
    c.vTaskDelay(xTicksToDelay);
}

pub fn xTimerPendFunctionCall(xFunctionToPend: PendedFunction_t, pvParameter1: ?anyopaque, ulParameter2: u32, xTicksToWait: TickType_t) bool {
    return (pdTRUE == xTimerPendFunctionCall(xFunctionToPend, pvParameter1, ulParameter2, xTicksToWait));
}

pub const eNotifyAction = enum(u32) {
    eSetBits = c.eSetBits,
    eIncrement = c.eIncrement,
    eSetValueWithOverwrite = c.eSetValueWithOverwrite,
    eSetValueWithoutOverwrite = c.eSetValueWithoutOverwrite,
    eNoAction = c.eNoAction,
};

pub const Task = struct {
    task_handle: TaskHandle_t = undefined,

    pub fn initFromHandle(task_handle: ?TaskHandle_t) Task {
        return Task{
            .task_handle = task_handle orelse c.xTaskGetCurrentTaskHandle(),
        };
    }
    pub fn create(self: *Task, pxTaskCode: TaskFunction_t, pcName: [*c]const u8, usStackDepth: u16, pvParameters: ?*anyopaque, uxPriority: UBaseType_t) !void {
        if (c.pdPASS != c.xTaskCreate(pxTaskCode, pcName, usStackDepth, pvParameters, uxPriority, &self.task_handle)) {
            return FreeRtosError.TaskCreationFailed;
        }
    }
    pub fn setHandle(self: *Task, handle: TaskHandle_t) !void {
        if (self.task_handle != undefined) {
            self.task_handle = handle;
        } else {
            return FreeRtosError.TaskHandleAlreadyExists;
        }
    }
    pub fn getHandle(self: *Task) TaskHandle_t {
        return self.task_handle;
    }
    pub fn resumeTask(self: *Task) void {
        c.vTaskResume(self.task_handle);
    }
    pub fn suspendTask(self: *Task) void {
        c.vTaskSuspend(self.task_handle);
    }
    pub fn resumeTaskFromIsr(self: *Task) BaseType_t {
        return c.xTaskResumeFromISR(self.task_handle);
    }
    pub fn notify(self: *Task, ulValue: u32, eAction: eNotifyAction) bool {
        return (pdPASS == c.xTaskGenericNotify(self.task_handle, c.tskDEFAULT_INDEX_TO_NOTIFY, ulValue, @intFromEnum(eAction), null));
    }
    pub fn notifyFromISR(self: *Task, ulValue: u32, eAction: eNotifyAction, pxHigherPriorityTaskWoken: *BaseType_t) bool {
        return (pdPASS == c.xTaskNotifyFromISR(self.task_handle, ulValue, eAction, pxHigherPriorityTaskWoken));
    }
    pub fn waitForNotify(self: *Task, ulBitsToClearOnEntry: u32, ulBitsToClearOnExit: u32, pulNotificationValue: *u32, xTicksToWait: TickType_t) bool {
        _ = self;
        return (pdPASS == c.xTaskNotifyWait(ulBitsToClearOnEntry, ulBitsToClearOnExit, pulNotificationValue, xTicksToWait));
    }
};

pub const Queue = struct {
    handle: QueueHandle_t = undefined,

    pub fn create(self: *Queue, item_size: UBaseType_t, queue_length: UBaseType_t) !void {
        self.handle = c.xQueueCreate(item_size, queue_length);
        if (self.handle == null) {
            return FreeRtosError.QueueCreationFailed;
        }
    }
    pub fn send(self: *Queue, item_to_queue: *const void, comptime ticks_to_wait: ?UBaseType_t) bool {
        var ticks = ticks_to_wait orelse portMAX_DELAY;
        return (pdTRUE == c.xQueueSend(self.handle, item_to_queue, ticks));
    }
    pub fn receive(self: *Queue, buffer: *void, comptime ticks_to_wait: ?UBaseType_t) bool {
        var ticks = ticks_to_wait orelse portMAX_DELAY;
        return (pdTRUE == c.xQueueReceive(self.handle, buffer, ticks));
    }
    pub fn delete(self: *Queue) void {
        c.xQueueDelete(self.handle);
        self.handle = undefined;
    }
};

pub const Timer = struct {
    handle: TimerHandle_t = undefined,

    pub fn create(self: *Timer, pcTimerName: [*c]const u8, xTimerPeriodInTicks: TickType_t, xAutoReload: BaseType_t, pvTimerID: ?*anyopaque, pxCallbackFunction: TimerCallbackFunction_t) !void {
        self.handle = c.xTimerCreate(pcTimerName, xTimerPeriodInTicks, xAutoReload, pvTimerID, pxCallbackFunction);
        if (self.handle == null) {
            return FreeRtosError.TimerCreationFailed;
        }
    }
    pub fn start(self: *Timer, xTicksToWait: ?TickType_t) !void {
        var pxHigherPriorityTaskWoken: BaseType_t = undefined;
        var ticks = xTicksToWait orelse portMAX_DELAY;
        if (c.pdFAIL == c.xTimerGenericCommand(self.handle, c.tmrCOMMAND_START, c.xTaskGetTickCount(), &pxHigherPriorityTaskWoken, ticks)) {
            return FreeRtosError.TimerStartFailed;
        }
    }
    pub fn stop(self: *Timer, xTicksToWait: TickType_t) BaseType_t {
        return c.xTimerStop(self.handle, xTicksToWait);
    }
    pub fn delete(self: *Timer, xTicksToWait: TickType_t) BaseType_t {
        return c.xTimerDelete(self.handle, xTicksToWait);
    }
    pub fn reset(self: *Timer, xTicksToWait: TickType_t) BaseType_t {
        return c.xTimerReset(self.handle, xTicksToWait);
    }
};
