const std = @import("std");
const microzig = @import("root").dependencies.imports.microzig;
const Chip = microzig.Chip;
const MemoryRegion = microzig.MemoryRegion;

fn get_root_path() []const u8 {
    return std.fs.path.dirname(@src().file) orelse ".";
}

const build_root_path = get_root_path();

pub const efm32gg390f1024 = .{ .name = "EFM32GG390F1024", .cpu = .cortex_m3, .memory_regions = &.{
    .{ .offset = 0x00000000, .length = (1024 * 1024) - (16 * 4096), .kind = .flash },
    .{ .offset = 0x000F0000, .length = (16 * 4096), .kind = .reserved },
    .{ .offset = 0x20000000, .length = 128 * 1024, .kind = .ram },
}, .register_definition = .{ .zig = .{ .cwd_relative = build_root_path ++ "/chips/EFM32GG390F1024.zig" } } };
