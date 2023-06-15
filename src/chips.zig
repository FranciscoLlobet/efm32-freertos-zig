const std = @import("std");
const microzig = @import("../deps/microzig/build.zig");
const Chip = microzig.Chip;
const MemoryRegion = microzig.MemoryRegion;

fn get_root_path() []const u8 {
    return std.fs.path.dirname(@src().file) orelse unreachable;
}

pub const efm32gg390f1024 = Chip.from_standard_paths(get_root_path(), .{
    .name = "EFM32GG390F1024",
    .cpu = microzig.cpus.cortex_m3,
    .memory_regions = &.{
        MemoryRegion{ .offset = 0x00000000, .length = 1024 * 1024, .kind = .flash },
        MemoryRegion{ .offset = 0x20000000, .length = 128 * 1024, .kind = .ram },
    },
});
