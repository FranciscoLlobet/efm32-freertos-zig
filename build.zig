const std = @import("std");

pub const boards = @import("src/boards.zig");
pub const chips = @import("src/chips.zig");

const build_ff = @import("build_ff.zig");
const build_gecko_sdk = @import("build_gecko_sdk.zig");
const build_freertos = @import("build_freertos.zig");
const build_sensors = @import("build_sensors.zig");
const build_cc3100_sdk = @import("build_cc3100_sdk.zig");
const build_mbedts = @import("build_mbedtls.zig");
const build_wakaama = @import("build_wakaama.zig");
const build_mqtt = @import("build_mqtt.zig");
const build_picohttpparser = @import("build_picohttpparser.zig");
const builtin = @import("builtin");

pub fn build(b: *std.Build) !void {
    const include_path_array = [_][]const u8{
        // Configuration Files for miso
        "csrc/config",

        // C-Bootstraps
        "csrc/system/cmsis",

        // Board package
        "csrc/board/inc",

        // Core C-source includes
        "csrc/inc",

        // JSMN header lib
        "csrc/utils/jsmn",
    };
    const src_paths = [_][]const u8{
        // Newlib adapter
        //"csrc/system/newlib/assert.c",
        //"csrc/system/newlib/exit.c",
        //"csrc/system/newlib/sbrk.c",
        //"csrc/system/newlib/syscalls.c",

        // Board (support) package
        "csrc/board/src/system_efm32gg.c",
        "csrc/board/src/board.c",
        "csrc/board/src/board_leds.c",
        "csrc/board/src/board_buttons.c",
        "csrc/board/src/board_watchdog.c",
        "csrc/board/src/board_sd_card.c",
        "csrc/board/src/sdmm.c",
        "csrc/board/src/board_i2c_sensors.c",
        "csrc/board/src/board_bma280.c",
        "csrc/board/src/board_bme280.c",
        "csrc/board/src/board_bmg160.c",
        "csrc/board/src/board_bmi160.c",
        "csrc/board/src/board_bmm150.c",
        "csrc/board/src/board_CC3100.c",
        "csrc/board/src/board_em9301.c",
        "csrc/board/src/board_usb.c",

        // Core C-source
        "csrc/src/config.c",
        "csrc/src/miso_ntp.c",
        "csrc/src/sntp_packet.c",
        "csrc/src/network.c",
        "csrc/src/wifi_service.c",
    };

    const c_flags = [_][]const u8{ "-O2", "-DEFM32GG390F1024", "-DSL_CATALOG_POWER_MANAGER_PRESENT=1 -D__Vectors=\"VectorTable\"", "-fdata-sections", "-ffunction-sections" };

    const microzig = @import("microzig").init(b, "microzig");

    const optimize = b.standardOptimizeOption(.{});

    const firmware = microzig.addFirmware(b, .{
        .name = "miso",
        .target = .{ .preferred_format = .elf, .chip = chips.efm32gg390f1024, .board = boards.xdk110 },
        .optimize = optimize,
        .source_file = .{ .path = "src/main.zig" },
    });

    firmware.addSystemIncludePath(.{ .cwd_relative = "C:\\Program Files (x86)\\Arm GNU Toolchain arm-none-eabi\\12.2 mpacbti-rel1\\arm-none-eabi\\include" });
    firmware.addObjectFile(.{ .cwd_relative = "C:\\Program Files (x86)\\Arm GNU Toolchain arm-none-eabi\\12.2 mpacbti-rel1\\lib\\gcc\\arm-none-eabi\\12.2.1\\thumb\\v7-m\\nofp\\libc_nano.a" });
    firmware.addObjectFile(.{ .cwd_relative = "C:\\Program Files (x86)\\Arm GNU Toolchain arm-none-eabi\\12.2 mpacbti-rel1\\lib\\gcc\\arm-none-eabi\\12.2.1\\thumb\\v7-m\\nofp\\libgcc.a" });
    firmware.addObjectFile(.{ .path = "csrc/system/gecko_sdk/emdrv/nvm3/lib/libnvm3_CM3_gcc.a" });

    for (include_path_array) |path| {
        firmware.addIncludePath(.{ .path = path });
    }

    for (src_paths) |path| {
        firmware.addCSourceFile(.{ .file = .{ .path = path }, .flags = &c_flags });
    }

    build_ff.build_ff(firmware);
    build_gecko_sdk.aggregate(firmware);
    build_freertos.aggregate(firmware);
    build_sensors.aggregate(firmware);
    build_cc3100_sdk.aggregate(firmware);
    build_mbedts.aggregate(firmware);
    build_wakaama.aggregate(firmware);
    build_mqtt.aggregate(firmware);
    build_picohttpparser.aggregate(firmware);

    microzig.installFirmware(b, firmware, .{});
    microzig.installFirmware(b, firmware, .{ .format = .bin });
}
