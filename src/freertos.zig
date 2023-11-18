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
    SemaphoreTakeTimeout,
    TimerCreationFailed,

    TimerStartFailed,
    TimerStopFailed,
    TimerChangePeriodFailed,

    MessageBufferCreationFailed,
    MessageBufferReceiveFailed,
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

        return if (pdPASS == c.xTaskCreate(pxTaskCode, pcName, usStackDepth, pvParameters, uxPriority, @constCast(&self.handle))) self else FreeRtosError.TaskCreationFailed;
    }
    pub fn setHandle(self: *const @This(), handle: TaskHandle_t) !void {
        if (self.handle != undefined) {
            self.handle = handle;
        } else {
            return FreeRtosError.TaskHandleAlreadyExists;
        }
    }
    /// Helper function that can be used to cast the pvParameters pointer to a reference
    pub inline fn getAndCastPvParameters(comptime T: type, pvParameters: ?*anyopaque) *T {
        return @as(*T, @ptrCast(@alignCast(pvParameters)));
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
        return @intCast(c.uxTaskGetStackHighWaterMark(self.handle));
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
        return if (self.handle == null) FreeRtosError.QueueCreationFailed else self;
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

pub const Semaphore = struct {
    handle: SemaphoreHandle_t = undefined,

    /// Create a binary Semaphore
    pub fn createBinary() !@This() {
        const self = @This(){ .handle = c.xSemaphoreCreateBinary() };
        if (self.handle == null) {
            return FreeRtosError.SemaphoreCreationFailed;
        }
    }

    /// Create a counting semaphore
    pub fn createCountingSemaphore(self: *@This(), uxMaxCount: u32, uxInitialCount: u32) !void {
        self.handle = c.xSemaphoreCreateCounting(uxMaxCount, uxInitialCount);
        if (self.handle == null) {
            return FreeRtosError.SemaphoreCreationFailed;
        }
    }

    /// Create a mutex
    pub fn createMutex() !@This() {
        const self = @This(){ .handle = c.xSemaphoreCreateMutex() };

        return if (self.handle == null) FreeRtosError.SemaphoreCreationFailed else self;
    }

    /// Take the semaphore
    pub fn take(self: *const @This(), xTicksToWait: ?TickType_t) !bool {
        return if (pdTRUE == c.xSemaphoreTake(self.handle, xTicksToWait orelse portMAX_DELAY)) true else FreeRtosError.SemaphoreTakeTimeout;
    }

    /// Give the semaphore
    pub fn give(self: *const @This()) !void {
        return if (pdTRUE == c.xSemaphoreGive(self.handle)) {} else FreeRtosError.pdFAIL;
    }

    /// Give the semaphore from ISR
    pub inline fn giveFromIsr(self: *const @This(), pxHigherPriorityTaskWoken: *BaseType_t) !void {
        return if (pdTRUE == c.xSemaphoreGiveFromISR(self.handle, pxHigherPriorityTaskWoken)) {} else FreeRtosError.pdFAIL;
    }

    /// Initialize Semaphore from handle
    pub fn initFromHandle(handle: SemaphoreHandle_t) @This() {
        return @This(){ .handle = handle };
    }
};

pub const Mutex = struct {
    semaphore: Semaphore = undefined,

    pub fn create() !@This() {
        var self: @This() = undefined;

        self.semaphore = try Semaphore.createMutex();

        return self;
    }

    pub fn take(self: *const @This(), xTicksToWait: ?TickType_t) !bool {
        return self.semaphore.take(xTicksToWait);
    }

    pub fn give(self: *const @This()) !void {
        return self.semaphore.give();
    }

    pub inline fn giveFromIsr(self: *const @This(), pxHigherPriorityTaskWoken: *BaseType_t) !void {
        return self.semaphore.giveFromIsr(pxHigherPriorityTaskWoken);
    }
};

/// Timer
pub const Timer = struct {
    handle: TimerHandle_t = undefined,

    /// Get the timer ID from the timer handle and cast it into the desired (referenced) type
    pub fn getIdFromHandle(comptime T: type, xTimer: TimerHandle_t) *T {
        return @as(*T, @ptrCast(@alignCast(c.pvTimerGetTimerID(xTimer))));
    }

    /// Create a FreeRTOS timer
    pub fn create(pcTimerName: [*:0]const u8, xTimerPeriodInTicks: TickType_t, autoReload: bool, comptime T: type, pvTimerID: *T, pxCallbackFunction: TimerCallbackFunction_t) !@This() {
        var self: @This() = .{ .handle = c.xTimerCreate(pcTimerName, xTimerPeriodInTicks, if (autoReload) pdTRUE else pdFALSE, @ptrCast(@alignCast(pvTimerID)), pxCallbackFunction) };

        return if (self.handle == null) FreeRtosError.TimerCreationFailed else self;
    }

    /// Get the timer ID from the own timer
    pub fn getId(self: *const @This(), comptime T: type) ?*T {
        return @as(?*T, @ptrCast(@alignCast(c.pvTimerGetTimerID(self.timer))));
    }

    /// Start the timer
    pub fn start(self: *const @This(), xTicksToWait: ?TickType_t) !void {
        if (pdFAIL == c.xTimerGenericCommand(self.handle, c.tmrCOMMAND_START, c.xTaskGetTickCount(), null, xTicksToWait orelse portMAX_DELAY)) {
            return FreeRtosError.TimerStartFailed;
        }
    }

    /// Stop the timer
    pub fn stop(self: *const @This(), xTicksToWait: ?TickType_t) !void {
        if (pdFAIL == c.xTimerGenericCommand(self.handle, c.tmrCOMMAND_STOP, @as(TickType_t, 0), null, xTicksToWait orelse portMAX_DELAY)) {
            return FreeRtosError.TimerStopFailed;
        }
    }

    /// Change the period of the timer
    pub fn changePeriod(self: *const @This(), xNewPeriod: TickType_t, xBlockTime: ?TickType_t) !void {
        if (pdFAIL == c.xTimerGenericCommand(self.handle, c.tmrCOMMAND_CHANGE_PERIOD, xNewPeriod, null, xBlockTime orelse portMAX_DELAY)) {
            return FreeRtosError.TimerChangePeriodFailed;
        }
    }

    /// Delete the timer
    pub fn delete(self: *const @This(), xTicksToWait: TickType_t) BaseType_t {
        return c.xTimerDelete(self.handle, xTicksToWait);
    }

    /// Reset the timer
    pub fn reset(self: *const @This(), xTicksToWait: TickType_t) BaseType_t {
        return c.xTimerReset(self.handle, xTicksToWait);
    }

    /// Initialize Timer from handle
    pub fn initFromHandle(handle: TimerHandle_t) @This() {
        return @This(){ .handle = handle };
    }
};

/// Message Buffer
pub const MessageBuffer = struct {
    /// Handle to the message buffer
    handle: MessageBufferHandle_t = undefined,

    /// Create a message buffer with the given size
    pub fn create(xBufferSizeBytes: usize) !@This() {
        var self: @This() = undefined;

        self.handle = c.xStreamBufferGenericCreate(xBufferSizeBytes, 0, pdTRUE, null, null);

        return if (self.handle != null) self else FreeRtosError.MessageBufferCreationFailed;
    }

    /// Send a message to the message buffer
    pub fn send(self: *const @This(), txData: []u8, comptime xTicksToWait: ?TickType_t) usize {
        return c.xMessageBufferSend(self.handle, txData.ptr, txData.len, xTicksToWait orelse portMAX_DELAY);
    }

    /// Receive a message from the message buffer
    pub fn receive(self: *const @This(), rxData: []const u8, ticks_to_wait: ?TickType_t) ?[]u8 {
        var rx_len: usize = c.xMessageBufferReceive(self.handle, @constCast(rxData.ptr), rxData.len, ticks_to_wait orelse portMAX_DELAY);

        return if (rx_len != 0) @constCast(rxData)[0..rx_len] else null;
    }
};

/// Allocator to use in a FreeRTOS application
pub const allocator = std.mem.Allocator{ .ptr = undefined, .vtable = &allocator_vtable };

/// VTable for the FreeRTOS allocator
const allocator_vtable = std.mem.Allocator.VTable{
    .alloc = freertos_alloc,
    .resize = freertos_resize,
    .free = freertos_free,
};

/// FreeRTOS allocator
fn freertos_alloc(
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
