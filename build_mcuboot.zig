const std = @import("std");
const microzig = @import("microzig");

const include_path = [_][]const u8{
    "csrc/mcuboot/mcuboot/boot/bootutil/include",
};

const source_path = [_][]const u8{
    "csrc/mcuboot/mcuboot/boot/bootutil/src/tlv.c",
};

const c_flags = [_][]const u8{ "-DEFM32GG390F1024", "-O2", "-fdata-sections", "-ffunction-sections" };

pub fn aggregate(exe: *microzig.Firmware) void {
    for (include_path) |path| {
        exe.addIncludePath(.{ .path = path });
    }

    for (source_path) |path| {
        exe.addCSourceFile(.{ .file = .{ .path = path }, .flags = &c_flags });
    }
}
