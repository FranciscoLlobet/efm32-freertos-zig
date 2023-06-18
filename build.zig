const std = @import("std");
const microzig = @import("deps/microzig/build.zig");
const build_ff = @import("build_ff.zig");
const build_gecko_sdk = @import("build_gecko_sdk.zig");
const build_freertos = @import("build_freertos.zig");
const build_sensors = @import("build_sensors.zig");

pub const boards = @import("src/boards.zig");
pub const chips = @import("src/chips.zig");

// Although this function looks imperative, note that its job is to
// declaratively construct a build graph that will be executed by an external
// runner.
pub fn build(b: *std.build.Builder) void {
    const optimize = b.standardOptimizeOption(.{});

    //const c_base_path = "csrc/":
    //const c_system_path = "csrc/system/";
    //const gecko_sdk_path = c_system_path ++ "gecko-sdk/";

    const include_path_array = [_][]const u8{
        // Configuration Files for miso
        "csrc/system/config",
        // C-Bootstraps
        "csrc/system/cmsis",
        // Board package
        "csrc/board/inc",
        "csrc/inc",

        "csrc/utils/jsmn",
        "csrc/inc",
    };
    const src_paths = [_][]const u8{
        "csrc/system/newlib/assert.c",
        "csrc/system/newlib/exit.c",
        "csrc/system/newlib/sbrk.c",
        "csrc/system/newlib/syscalls.c",

        "csrc/src/config.c",

        "csrc/board/src/board.c",
        "csrc/board/src/system_efm32gg.c",

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
    };

    const c_flags = [_][]const u8{"-DEFM32GG390F1024 -DSL_CATALOG_POWER_MANAGER_PRESENT=1 -D__Vectors=\"VectorTable\""};

    inline for (@typeInfo(boards).Struct.decls) |decl| {
        if (!decl.is_pub)
            continue;

        const exe = microzig.addEmbeddedExecutable(b, .{
            .name = @field(boards, decl.name).name ++ ".elf",
            .source_file = .{
                .path = "src/main.zig",
            },
            .backing = .{ .board = @field(boards, decl.name) },
            .optimize = optimize,
            // .linkerscript_source_file = .{ .path = "csrc/efm32gg.ld" },
        });
        exe.addSystemIncludePath("C:\\Program Files (x86)\\Arm GNU Toolchain arm-none-eabi\\12.2 mpacbti-rel1\\arm-none-eabi\\include");
        exe.addObjectFile("C:\\Program Files (x86)\\Arm GNU Toolchain arm-none-eabi\\12.2 mpacbti-rel1\\lib\\gcc\\arm-none-eabi\\12.2.1\\thumb\\v6-m\\nofp\\libc_nano.a");
        exe.addObjectFile("C:\\Program Files (x86)\\Arm GNU Toolchain arm-none-eabi\\12.2 mpacbti-rel1\\lib\\gcc\\arm-none-eabi\\12.2.1\\thumb\\v6-m\\nofp\\libgcc.a");

        for (include_path_array) |path| {
            exe.addIncludePath(path);
        }
        for (src_paths) |path| {
            exe.addCSourceFile(path, &c_flags);
        }

        build_ff.build_ff(exe);
        build_gecko_sdk.aggregate(exe);
        build_freertos.aggregate(exe);
        build_sensors.aggregate(exe);
        exe.installArtifact(b);
    }

    inline for (@typeInfo(chips).Struct.decls) |decl| {
        if (!decl.is_pub)
            continue;

        const exe = microzig.addEmbeddedExecutable(b, .{
            .name = @field(chips, decl.name).name ++ ".minimal",
            .source_file = .{
                .path = "test/programs/minimal.zig",
            },
            .backing = .{ .chip = @field(chips, decl.name) },
            .optimize = optimize,
        });
        exe.installArtifact(b);
    }
}
