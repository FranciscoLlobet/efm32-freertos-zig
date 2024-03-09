// Copyright (c) 2023-2024 Francisco Llobet-Blandino and the "Miso Project".
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the “Software”), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
// WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

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
pub const RAM_SIZE: usize = 128 * KiB;

/// Size of the flash region in bytes.
pub const FLASH_SIZE: usize = 1024 * KiB;

/// Size of the available flash region in bytes.
const FLASH_AVAILABLE_SIZE: usize = FLASH_SIZE - NVM3_SIZE;

pub const FLASH_BOOTLOADER_SIZE: usize = 256 * KiB;

pub const FLASH_APP_SIZE: usize = (FLASH_SIZE - NVM3_SIZE - FLASH_BOOTLOADER_SIZE);

//const FLASH_RESERVE_SIZE: usize = FLASH_AVAILABLE_SIZE - FLASH_APP_SIZE;

pub const bootloader = .{
    .name = "EFM32GG390F1024",
    .cpu = .cortex_m3,
    .memory_regions = &.{
        .{ .offset = 0x00000000, .length = FLASH_BOOTLOADER_SIZE, .kind = .flash }, // Main Application
        .{ .offset = FLASH_BOOTLOADER_SIZE, .length = FLASH_APP_SIZE, .kind = .reserved }, // Main Application
        .{ .offset = FLASH_AVAILABLE_SIZE, .length = (NVM3_SIZE), .kind = .reserved }, // NVM3 space
        .{ .offset = 0x20000000, .length = RAM_SIZE, .kind = .ram }, // RAM
    },
    .register_definition = .{ .svd = .{ .cwd_relative = build_root_path ++ "/chips/EFM32GG390F1024.svd" } },
};

pub const application = .{
    .name = "EFM32GG390F1024",
    .cpu = .cortex_m3,
    .memory_regions = &.{
        .{ .offset = 0x00000000, .length = FLASH_BOOTLOADER_SIZE, .kind = .reserved }, // Main Application
        .{ .offset = FLASH_BOOTLOADER_SIZE + 0x80, .length = FLASH_APP_SIZE - 0x80, .kind = .flash }, // Main Application
        .{ .offset = FLASH_AVAILABLE_SIZE, .length = (NVM3_SIZE), .kind = .reserved }, // NVM3 space
        .{ .offset = 0x20000000, .length = RAM_SIZE, .kind = .ram }, // RAM
    },
    .register_definition = .{ .svd = .{ .cwd_relative = build_root_path ++ "/chips/EFM32GG390F1024.svd" } },
};
