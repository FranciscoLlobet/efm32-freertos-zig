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
        "csrc/src/miso_ntp.c",
        "csrc/src/sntp_packet.c",
        "csrc/src/network.c",
        "csrc/src/wifi_service.c",
    };

    const c_flags = [_][]const u8{ "-O2", "-DEFM32GG390F1024", "-DSL_CATALOG_POWER_MANAGER_PRESENT=1", "-fdata-sections", "-ffunction-sections" };

    const microzig = @import("microzig").init(b, "microzig");

    const optimize = b.standardOptimizeOption(.{});

    const bootloader_target = .{
        .name = "miso",
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

    const firmware = microzig.addFirmware(b, bootloader_target);
    firmware.addSystemIncludePath(.{ .path = "toolchain/picolibc/include" });
    firmware.addObjectFile(.{ .path = "toolchain/picolibc/libc.a" });
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
    build_mcuboot.aggregate(firmware);

    const application = microzig.addFirmware(b, application_target);

    application.addSystemIncludePath(.{ .path = "toolchain/picolibc/include" });
    application.addObjectFile(.{ .path = "toolchain/picolibc/libc.a" });
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

    microzig.installFirmware(b, firmware, .{ .format = .elf });
    microzig.installFirmware(b, firmware, .{ .format = .bin });
    microzig.installFirmware(b, firmware, .{ .format = .hex });
    microzig.installFirmware(b, application, .{ .format = .elf });
    microzig.installFirmware(b, application, .{ .format = .hex });
    microzig.installFirmware(b, application, .{ .format = .bin });
}
