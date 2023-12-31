const std = @import("std");
const microzig = @import("microzig");

const c_flags = [_][]const u8{ "-DEFM32GG390F1024", "-O2", "-DSL_CATALOG_POWER_MANAGER_PRESENT=1", "-fdata-sections", "-ffunction-sections" };

const include_path = [_][]const u8{
    // FreeRTOS
    "csrc/system/FreeRTOS-Kernel/include",
    "csrc/system/FreeRTOS-Kernel/portable/GCC/ARM_CM3",
};

const source_paths = [_][]const u8{
    "csrc/system/FreeRTOS-Kernel/croutine.c",
    "csrc/system/FreeRTOS-Kernel/list.c",
    "csrc/system/FreeRTOS-Kernel/queue.c",
    "csrc/system/FreeRTOS-Kernel/stream_buffer.c",
    "csrc/system/FreeRTOS-Kernel/tasks.c",
    "csrc/system/FreeRTOS-Kernel/timers.c",
    "csrc/system/FreeRTOS-Kernel/portable/GCC/ARM_CM3/port.c",
    "csrc/system/FreeRTOS-Kernel/portable/MemMang/heap_4.c",
};

pub fn aggregate(exe: *microzig.Firmware) void {
    for (include_path) |path| {
        exe.addIncludePath(.{ .path = path });
    }

    for (source_paths) |path| {
        exe.addCSourceFile(.{ .file = .{ .path = path }, .flags = &c_flags });
    }
}
