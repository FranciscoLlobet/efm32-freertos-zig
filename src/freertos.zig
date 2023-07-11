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
pub const pdFAIL = c.pdFAIL;
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

pub fn xTimerPendFunctionCall(xFunctionToPend: PendedFunction_t, pvParameter1: ?*anyopaque, ulParameter2: u32, xTicksToWait: TickType_t) bool {
    return (pdTRUE == c.xTimerPendFunctionCall(xFunctionToPend, pvParameter1, ulParameter2, xTicksToWait));
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

    pub fn initFromHandle(task_handle: ?TaskHandle_t) @This() {
        return @This(){
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
    pub fn getStackHighWaterMark(self: *Task) u32 {
        return @as(u32, c.uxTaskGetStackHighWaterMark(self.task_handle));
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
    pub fn getId(self: *Timer) ?*anyopaque {
        return c.pvTimerGetTimerID(self.handle);
    }
    pub fn initFromHandle(handle: TimerHandle_t) @This() {
        return @This(){ .handle = handle };
    }
};

pub const allocator = std.mem.Allocator{ .ptr = undefined, .vtable = &allocator_vtable };

pub const allocator_vtable = std.mem.Allocator.VTable{
    .alloc = freertos_alloc,
    .resize = freertos_resize,
    .free = freertos_free,
};

pub fn freertos_alloc(
    _: *anyopaque,
    len: usize,
    log2_ptr_align: u8,
    ret_addr: usize,
) ?[*]u8 {
    _ = ret_addr;
    _ = log2_ptr_align;
    //std.debug.assert(log2_ptr_align <= comptime std.math.log2_int(usize, @alignOf(std.c.max_align_t)));
    // Note that this pointer cannot be aligncasted to max_align_t because if
    // len is < max_align_t then the alignment can be smaller. For example, if
    // max_align_t is 16, but the user requests 8 bytes, there is no built-in
    // type in C that is size 8 and has 16 byte alignment, so the alignment may
    // be 8 bytes rather than 16. Similarly if only 1 byte is requested, malloc
    // is allowed to return a 1-byte aligned pointer.
    return @ptrCast(?[*]u8, vPortMalloc(len));
}

fn freertos_resize(
    _: *anyopaque,
    buf: []u8,
    log2_old_align: u8,
    new_len: usize,
    ret_addr: usize,
) bool {
    _ = log2_old_align;
    _ = ret_addr;
    return new_len <= buf.len;
}

fn freertos_free(
    _: *anyopaque,
    buf: []u8,
    log2_old_align: u8,
    ret_addr: usize,
) void {
    _ = log2_old_align;
    _ = ret_addr;
    vPortFree(buf.ptr);
}
