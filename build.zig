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
const build_mcuboot = @import("build_mcuboot.zig");
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
        "csrc/src/wifi_service.c",
    };

    const c_flags = [_][]const u8{ "-O2", "-DEFM32GG390F1024", "-DSL_CATALOG_POWER_MANAGER_PRESENT=1", "-fdata-sections", "-ffunction-sections", "-DMISO_APPLICATION" };

    const c_flags_boot = [_][]const u8{ "-O2", "-DEFM32GG390F1024", "-DSL_CATALOG_POWER_MANAGER_PRESENT=1", "-fdata-sections", "-ffunction-sections", "-DMISO_BOOTLOADER" };

    const microzig = @import("microzig").init(b, "microzig");

    const optimize = b.standardOptimizeOption(.{});

    const bootloader_target = .{
        .name = "boot",
        .target = .{
            .preferred_format = .elf,
            .chip = chips.bootloader,
            .board = boards.xdk110,
        },
        .optimize = optimize,
        .source_file = .{ .path = "src/boot.zig" },
    };

    const application_target = .{
        .name = "app",
        .target = .{
            .preferred_format = .elf,
            .chip = chips.application,
            .board = boards.xdk110,
        },
        .optimize = optimize,
        .source_file = .{ .path = "src/main.zig" },
    };

    const bootloader = microzig.addFirmware(b, bootloader_target);
    bootloader.addSystemIncludePath(.{ .path = "toolchain/clang-compiled/picolibc/include" });
    bootloader.addObjectFile(.{ .path = "toolchain/clang-compiled/picolibc/libc.a" });
    bootloader.addObjectFile(.{ .path = "csrc/system/gecko_sdk/emdrv/nvm3/lib/libnvm3_CM3_gcc.a" });

    for (include_path_array) |path| {
        bootloader.addIncludePath(.{ .path = path });
    }

    for (src_paths) |path| {
        bootloader.addCSourceFile(.{ .file = .{ .path = path }, .flags = &c_flags_boot });
    }

    build_ff.build_ff(bootloader);
    build_gecko_sdk.aggregate(bootloader);
    build_freertos.aggregate(bootloader);
    build_sensors.aggregate(bootloader);
    build_cc3100_sdk.aggregate(bootloader);
    build_mbedts.aggregate(bootloader);
    build_wakaama.aggregate(bootloader);
    build_mqtt.aggregate(bootloader);
    build_picohttpparser.aggregate(bootloader);
    build_mcuboot.aggregate(bootloader);

    const application = microzig.addFirmware(b, application_target);

    application.addSystemIncludePath(.{ .path = "toolchain/clang-compiled/picolibc/include" });
    application.addObjectFile(.{ .path = "toolchain/clang-compiled/picolibc/libc.a" });
    application.addObjectFile(.{ .path = "csrc/system/gecko_sdk/emdrv/nvm3/lib/libnvm3_CM3_gcc.a" });

    for (include_path_array) |path| {
        application.addIncludePath(.{ .path = path });
    }

    for (src_paths) |path| {
        application.addCSourceFile(.{ .file = .{ .path = path }, .flags = &c_flags });
    }

    build_ff.build_ff(application);
    build_gecko_sdk.aggregate(application);
    build_freertos.aggregate(application);
    build_sensors.aggregate(application);
    build_cc3100_sdk.aggregate(application);
    build_mbedts.aggregate(application);
    build_wakaama.aggregate(application);
    build_mqtt.aggregate(application);
    build_picohttpparser.aggregate(application);
    build_mcuboot.aggregate(application);

    microzig.installFirmware(b, bootloader, .{ .format = .elf });
    microzig.installFirmware(b, bootloader, .{ .format = .bin });
    microzig.installFirmware(b, bootloader, .{ .format = .hex });
    microzig.installFirmware(b, application, .{ .format = .elf });
    microzig.installFirmware(b, application, .{ .format = .hex });
    microzig.installFirmware(b, application, .{ .format = .bin });
}
