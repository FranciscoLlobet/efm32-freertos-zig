const std = @import("std");
const microzig = @import("deps/microzig/build.zig");

const include_path = [_][]const u8{
    "csrc/system/ff15/source",
};

const source_path = [_][]const u8{
    "csrc/system/ff15/source/ff.c",
    "csrc/system/ff15/source/ffunicode.c",
    "csrc/system/ff15/custom/ffsystem.c",
};

const c_flags = [_][]const u8{"-DEFM32GG390F1024"};

pub fn build_ff(exe: *microzig.EmbeddedExecutable) void {
    for (include_path) |path| {
        exe.addIncludePath(path);
    }

    for (source_path) |path| {
        exe.addCSourceFile(path, &c_flags);
    }
}
