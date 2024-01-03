const std = @import("std");
const microzig = @import("root").dependencies.imports.microzig;
const Chip = microzig.Chip;
const MemoryRegion = microzig.MemoryRegion;

fn get_root_path() []const u8 {
    return std.fs.path.dirname(@src().file) orelse ".";
}

const build_root_path = get_root_path();

/// Number of bytes in a kilobyte.
const KiB: usize = 1024;

/// Size of the NVM3 region in bytes.
const NVM3_SIZE: usize = 64 * KiB;

/// Size of the RAM region in bytes.
const RAM_SIZE: usize = 128 * KiB;

/// Size of the flash region in bytes.
const FLASH_SIZE: usize = 1024 * KiB;

/// Size of the available flash region in bytes.
const FLASH_AVAILABLE_SIZE: usize = FLASH_SIZE - NVM3_SIZE;

const FLASH_APP_SIZE: usize = 480 * KiB;

const FLASH_RESERVE_SIZE: usize = FLASH_AVAILABLE_SIZE - FLASH_APP_SIZE;

pub const bootloader = .{
    .name = "EFM32GG390F1024",
    .cpu = .cortex_m3,
    .memory_regions = &.{
        .{ .offset = 0x00000000, .length = FLASH_APP_SIZE, .kind = .flash }, // Main Application
        .{ .offset = FLASH_APP_SIZE, .length = FLASH_RESERVE_SIZE, .kind = .reserved }, // Main Application
        .{ .offset = FLASH_AVAILABLE_SIZE, .length = (NVM3_SIZE), .kind = .reserved }, // NVM3 space
        .{ .offset = 0x20000000, .length = RAM_SIZE, .kind = .ram }, // RAM
    },
    .register_definition = .{ .svd = .{ .cwd_relative = build_root_path ++ "/chips/EFM32GG390F1024.svd" } },
};

pub const application = .{
    .name = "EFM32GG390F1024",
    .cpu = .cortex_m3,
    .memory_regions = &.{
        .{ .offset = 0x00000000, .length = FLASH_APP_SIZE, .kind = .reserved }, // Main Application
        .{ .offset = FLASH_APP_SIZE, .length = FLASH_RESERVE_SIZE, .kind = .flash }, // Main Application
        .{ .offset = FLASH_AVAILABLE_SIZE, .length = (NVM3_SIZE), .kind = .reserved }, // NVM3 space
        .{ .offset = 0x20000000, .length = RAM_SIZE, .kind = .ram }, // RAM
    },
    .register_definition = .{ .svd = .{ .cwd_relative = build_root_path ++ "/chips/EFM32GG390F1024.svd" } },
};
