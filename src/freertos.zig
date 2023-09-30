const std = @import("std");
const cpu = @import("microzig").cpu;
pub const c = @cImport({
    @cInclude("FreeRTOS.h");
    @cInclude("task.h");
    @cInclude("timers.h");
    @cInclude("semphr.h");
    @cInclude("message_buffer.h");
});

pub const BaseType_t = c.BaseType_t;
pub const TaskFunction_t = c.TaskFunction_t;
pub const UBaseType_t = c.UBaseType_t;
pub const TaskHandle_t = c.TaskHandle_t;
pub const QueueHandle_t = c.QueueHandle_t;
pub const SemaphoreHandle_t = c.SemaphoreHandle_t;
pub const TimerHandle_t = c.TimerHandle_t;
pub const TickType_t = c.TickType_t;
pub const PendedFunction_t = c.PendedFunction_t;
pub const TimerCallbackFunction_t = c.TimerCallbackFunction_t;
pub const MessageBufferHandle_t = c.MessageBufferHandle_t;

pub const portMAX_DELAY = c.portMAX_DELAY;
pub const pdMS_TO_TICKS = c.pdMS_TO_TICKS;

// Common FreeRTOS constants
pub const pdPASS = c.pdPASS;
pub const pdFAIL = c.pdFAIL;

pub const pdTRUE = c.pdTRUE;
pub const pdFALSE = c.pdFALSE;

// Scheduler State
const eTaskSchedulerState = enum(BaseType_t) { taskSCHEDULER_NOT_STARTED = c.taskSCHEDULER_NOT_STARTED, taskSCHEDULER_SUSPENDED = c.taskSCHEDULER_SUSPENDED, taskSCHEDULER_RUNNING = c.taskSCHEDULER_RUNNING };

const FreeRtosError = error{
    pdFAIL,

    TaskCreationFailed,
    TaskHandleAlreadyExists,
    TaskNotifyFailed,

    QueueCreationFailed,
    QueueSendFailed,
    QueueReceiveFailed,

    SemaphoreCreationFailed,
    TimerCreationFailed,

    TimerStartFailed,
    TimerStopFailed,
    TimerChangePeriodFailed,
};

// Kernel function
pub inline fn vTaskStartScheduler() noreturn {
    c.vTaskStartScheduler();
    unreachable;
}

pub fn xTaskGetSchedulerState() eTaskSchedulerState {
    return @enumFromInt(c.xTaskGetSchedulerState());
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

pub fn xTaskGetTickCount() TickType_t {
    return c.xTaskGetTickCount();
}

pub inline fn portYIELD_FROM_ISR(xSwitchRequired: BaseType_t) void {
    if (xSwitchRequired != pdFALSE) {
        cpu.regs.ICSR.modify(.{ .PENDSVSET = 1 });

        cpu.dsb();
        cpu.isb();
    }
}

pub fn xTimerPendFunctionCall(xFunctionToPend: PendedFunction_t, pvParameter1: ?*anyopaque, ulParameter2: u32, xTicksToWait: ?TickType_t) bool {
    return (pdTRUE == c.xTimerPendFunctionCall(xFunctionToPend, pvParameter1, ulParameter2, xTicksToWait orelse portMAX_DELAY));
}

/// Helper function that can be used to cast a timerID pointer to a reference
pub inline fn getAndCastTimerID(comptime T: type, xTimer: TimerHandle_t) *T {
    return @as(*T, @ptrCast(@alignCast(c.pvTimerGetTimerID(xTimer))));
}

/// Helper function that can be used to cast the pvParameters pointer to a reference
pub inline fn getAndCastPvParameters(comptime T: type, pvParameters: ?*anyopaque) *T {
    return @as(*T, @ptrCast(@alignCast(pvParameters)));
}

pub const Task = struct {
    pub const eNotifyAction = enum(u32) {
        eSetBits = c.eSetBits,
        eIncrement = c.eIncrement,
        eSetValueWithOverwrite = c.eSetValueWithOverwrite,
        eSetValueWithoutOverwrite = c.eSetValueWithoutOverwrite,
        eNoAction = c.eNoAction,
    };

    handle: TaskHandle_t = undefined,

    pub fn initFromHandle(task_handle: TaskHandle_t) @This() {
        return @This(){ .handle = task_handle };
    }
    pub fn create(pxTaskCode: TaskFunction_t, pcName: [*:0]const u8, usStackDepth: u16, pvParameters: ?*anyopaque, uxPriority: UBaseType_t) !@This() {
        var self: @This() = undefined;

        if (c.pdPASS == c.xTaskCreate(pxTaskCode, pcName, usStackDepth, pvParameters, uxPriority, @constCast(&self.handle))) {
            return self;
        } else {
            return FreeRtosError.TaskCreationFailed;
        }
    }
    pub fn setHandle(self: *const @This(), handle: TaskHandle_t) !void {
        if (self.handle != undefined) {
            self.handle = handle;
        } else {
            return FreeRtosError.TaskHandleAlreadyExists;
        }
    }
    pub fn getHandle(self: *const @This()) TaskHandle_t {
        return self.handle;
    }
    pub fn resumeTask(self: *const @This()) void {
        c.vTaskResume(self.handle);
    }
    pub fn suspendTask(self: *const @This()) void {
        c.vTaskSuspend(self.handle);
    }
    pub inline fn resumeTaskFromIsr(self: *const @This()) BaseType_t {
        return c.xTaskResumeFromISR(self.handle);
    }

    // Notify Task with given value
    pub fn notify(self: *const @This(), ulValue: u32, eAction: eNotifyAction) FreeRtosError!void {
        return if (pdPASS == c.xTaskGenericNotify(self.handle, c.tskDEFAULT_INDEX_TO_NOTIFY, ulValue, @intFromEnum(eAction), null)) return FreeRtosError.TaskNotifyFailed;
    }
    pub inline fn notifyFromISR(self: *const @This(), ulValue: u32, eAction: eNotifyAction, pxHigherPriorityTaskWoken: *BaseType_t) bool {
        return (pdPASS == c.xTaskNotifyFromISR(self.handle, ulValue, eAction, pxHigherPriorityTaskWoken));
    }
    pub fn waitForNotify(self: *const @This(), ulBitsToClearOnEntry: u32, ulBitsToClearOnExit: u32, xTicksToWait: ?TickType_t) ?u32 {
        _ = self;
        var pulNotificationValue: u32 = undefined;
        return if (pdPASS == c.xTaskNotifyWait(ulBitsToClearOnEntry, ulBitsToClearOnExit, &pulNotificationValue, xTicksToWait orelse portMAX_DELAY)) pulNotificationValue else null;
    }
    pub fn getStackHighWaterMark(self: *const @This()) u32 {
        return @as(u32, c.uxTaskGetStackHighWaterMark(self.handle));
    }
    pub fn delayTask(self: *const @This(), xTicksToDelay: TickType_t) void {
        _ = self;
        c.vTaskDelay(xTicksToDelay);
    }
};

pub const Queue = struct {
    handle: QueueHandle_t = undefined,

    pub fn create(comptime T: type, len: usize) !@This() {
        var self: @This() = .{ .handle = c.xQueueCreate(@sizeOf(T), len) };
        if (self.handle == null) {
            return FreeRtosError.QueueCreationFailed;
        }
        return self;
    }
    pub fn send(self: *@This(), comptime T: type, item: *const T, comptime ticks_to_wait: ?TickType_t) bool {
        return (pdTRUE == c.xQueueSend(self.handle, @ptrCast(@constCast(item)), ticks_to_wait orelse portMAX_DELAY));
    }
    pub fn receive(self: *@This(), comptime T: type, item: *const T, comptime ticks_to_wait: ?TickType_t) bool {
        return (pdTRUE == c.xQueueReceive(self.handle, @ptrCast(@as(*T, @alignCast(item))), ticks_to_wait orelse portMAX_DELAY));
    }
    pub fn delete(self: *@This()) void {
        c.xQueueDelete(self.handle);
        self.handle = undefined;
    }
};

pub const TypedQueue = struct {
    handle: QueueHandle_t,
    T: type,

    pub fn create(comptime T: type) @This() {
        var self: @This() = .{ .handle = undefined, .T = T };

        return self;
    }
    pub fn init(self: *@This(), len: usize) !void {
        self.handle = .xQueueCreate(@sizeOf(self.T), len);
        if (self.handle == null) {
            return FreeRtosError.QueueCreationFailed;
        }
    }

    pub fn send(self: *@This(), comptime item: *const self.T, comptime ticks_to_wait: ?TickType_t) bool {
        return (pdTRUE == c.xQueueSend(self.handle, @ptrCast(@constCast(item)), ticks_to_wait orelse portMAX_DELAY));
    }
    pub fn receive(self: *@This(), comptime item: *const self.T, comptime ticks_to_wait: ?TickType_t) bool {
        return (pdTRUE == c.xQueueReceive(self.handle, @ptrCast(@as(*self.T, @alignCast(item))), ticks_to_wait orelse portMAX_DELAY));
    }
    pub fn delete(self: *@This()) void {
        c.xQueueDelete(self.handle);
        self.handle = undefined;
    }
};

pub const Mutex = struct {
    handle: SemaphoreHandle_t = undefined,

    pub fn create() !@This() {
        var self = @This(){ .handle = c.xSemaphoreCreateMutex() };

        if (self.handle == null) {
            return FreeRtosError.SemaphoreCreationFailed;
        }
        return self;
    }

    pub fn take(self: *@This(), xTicksToWait: ?TickType_t) !bool {
        return if (pdTRUE == c.xSemaphoreTake(self.handle, xTicksToWait orelse portMAX_DELAY)) true else FreeRtosError.pdFAIL;
    }

    pub fn give(self: *@This()) !void {
        return if (pdTRUE == c.xSemaphoreGive(self.handle)) {} else FreeRtosError.pdFAIL;
    }

    pub inline fn giveFromIsr(self: *@This(), pxHigherPriorityTaskWoken: *BaseType_t) !void {
        return if (pdTRUE == c.xSemaphoreGiveFromISR(self.handle, pxHigherPriorityTaskWoken)) {} else FreeRtosError.pdFAIL;
    }
};

pub const Semaphore = struct {
    handle: SemaphoreHandle_t = undefined,

    pub fn createBinary(self: *@This()) !void {
        self.handle = c.xSemaphoreCreateBinary();
        if (self.handle == null) {
            return FreeRtosError.SemaphoreCreationFailed;
        }
    }
    pub fn createCountingSemaphore(self: *@This(), uxMaxCount: u32, uxInitialCount: u32) !void {
        self.handle = c.xSemaphoreCreateCounting(uxMaxCount, uxInitialCount);
        if (self.handle == null) {
            return FreeRtosError.SemaphoreCreationFailed;
        }
    }
    pub fn take(self: *@This(), xTicksToWait: ?TickType_t) bool {
        return (pdTRUE == c.xSemaphoreTake(self.handle, xTicksToWait orelse portMAX_DELAY));
    }
    pub fn give(self: *@This()) bool {
        return (pdTRUE == c.xSemaphoreGive(self.handle));
    }
    pub inline fn giveFromIsr(self: *@This(), pxHigherPriorityTaskWoken: *BaseType_t) bool {
        return (pdTRUE == c.xSemaphoreGiveFromISR(self.handle, pxHigherPriorityTaskWoken));
    }
    pub fn initFromHandle(handle: SemaphoreHandle_t) @This() {
        return @This(){ .handle = handle };
    }
};

pub const Timer = struct {
    handle: TimerHandle_t = undefined,

    pub fn create(pcTimerName: [*:0]const u8, xTimerPeriodInTicks: TickType_t, xAutoReload: BaseType_t, pvTimerID: ?*anyopaque, pxCallbackFunction: TimerCallbackFunction_t) !@This() {
        var self: @This() = .{ .handle = c.xTimerCreate(pcTimerName, xTimerPeriodInTicks, xAutoReload, pvTimerID, pxCallbackFunction) };
        if (self.handle == null) {
            return FreeRtosError.TimerCreationFailed;
        }
        return self;
    }
    pub fn start(self: *const @This(), xTicksToWait: ?TickType_t) !void {
        if (pdFAIL == c.xTimerGenericCommand(self.handle, c.tmrCOMMAND_START, c.xTaskGetTickCount(), null, xTicksToWait orelse portMAX_DELAY)) {
            return FreeRtosError.TimerStartFailed;
        }
    }
    pub fn stop(self: *const @This(), xTicksToWait: ?TickType_t) !void {
        if (pdFAIL == c.xTimerGenericCommand(self.handle, c.tmrCOMMAND_STOP, @as(TickType_t, 0), null, xTicksToWait orelse portMAX_DELAY)) {
            return FreeRtosError.TimerStopFailed;
        }
    }
    pub fn changePeriod(self: *const @This(), xNewPeriod: TickType_t, xBlockTime: ?TickType_t) !void {
        if (pdFAIL == c.xTimerGenericCommand(self.handle, c.tmrCOMMAND_CHANGE_PERIOD, xNewPeriod, null, xBlockTime orelse portMAX_DELAY)) {
            return FreeRtosError.TimerChangePeriodFailed;
        }
    }
    pub fn delete(self: *const @This(), xTicksToWait: TickType_t) BaseType_t {
        return c.xTimerDelete(self.handle, xTicksToWait);
    }
    pub fn reset(self: *const @This(), xTicksToWait: TickType_t) BaseType_t {
        return c.xTimerReset(self.handle, xTicksToWait);
    }
    pub fn getId(self: *const @This()) ?*anyopaque {
        return c.pvTimerGetTimerID(self.handle);
    }
    pub fn initFromHandle(handle: TimerHandle_t) @This() {
        return @This(){ .handle = handle };
    }
};

pub const MessageBuffer = struct {
    handle: MessageBufferHandle_t = undefined,
    pub fn create(self: *@This(), xBufferSizeBytes: usize) void {
        self.handle = c.xStreamBufferGenericCreate(xBufferSizeBytes, 0, pdTRUE, null, null);
    }
    pub fn send(self: *@This(), pvTxData: ?*const anyopaque, xDataLengthBytes: usize, comptime xTicksToWait: ?TickType_t) usize {
        return c.xMessageBufferSend(self.handle, pvTxData, xDataLengthBytes, xTicksToWait orelse portMAX_DELAY);
    }
    pub fn receive(self: *@This(), pvRxData: ?*anyopaque, xDataLengthBytes: usize, ticks_to_wait: ?TickType_t) usize {
        return c.xMessageBufferReceive(self.handle, pvRxData, xDataLengthBytes, ticks_to_wait orelse portMAX_DELAY);
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
    return @as(?[*]u8, @ptrCast(vPortMalloc(len)));
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
