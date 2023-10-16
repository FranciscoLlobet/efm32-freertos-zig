const std = @import("std");
const microzig = @import("microzig");

const include_path = [_][]const u8{
    "csrc/system/ff15/source",
};

const source_path = [_][]const u8{
    "csrc/system/ff15/source/ff.c",
    "csrc/system/ff15/source/ffunicode.c",
    "csrc/system/ff15/custom/ffsystem.c",
};

const c_flags = [_][]const u8{ "-DEFM32GG390F1024", "-O2", "-fdata-sections", "-ffunction-sections" };

pub fn build_ff(exe: *microzig.Firmware) void {
    for (include_path) |path| {
        exe.addIncludePath(.{ .path = path });
    }

    for (source_path) |path| {
        exe.addCSourceFile(.{ .file = .{ .path = path }, .flags = &c_flags });
    }
}
