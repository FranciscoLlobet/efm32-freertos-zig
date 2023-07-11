const std = @import("std");
const board = @import("microzig").board;
const freertos = @import("freertos.zig");

const task_priorities = enum(freertos.BaseType_t) {
    rtos_prio_idle = 0, // Idle task priority
    rtos_prio_low = 1,
    rtos_prio_below_normal = 2,
    rtos_prio_normal = 3, // Normal Service Tasks
    rtos_prio_above_normal = 4,
    rtos_prio_high = 5,
    rtos_prio_highest = 6, // TimerService
};

pub const rtos_prio_sensor = @intFromEnum(task_priorities.rtos_prio_low);
pub const rtos_stack_depth_sensor: u16 = 440;

pub const rtos_prio_lwm2m = @intFromEnum(task_priorities.rtos_prio_normal);
pub const rtos_stack_depth_lwm2m: u16 = 2000;

pub const rtos_prio_mqtt = @intFromEnum(task_priorities.rtos_prio_normal);
pub const rtos_stack_depth_mqtt: u16 = 2000;
