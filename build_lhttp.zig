const std = @import("std");
const microzig = @import("deps/microzig/build.zig");

const include_path = [_][]const u8{"csrc/connectivity/picohttpparser"};

const picohttpp_src_path = "csrc/connectivity/picohttpparser/";

const source_path = [_][]const u8{picohttpp_src_path ++ "picohttpparser.c"};

const c_flags = [_][]const u8{ "-O2", "-fdata-sections", "-ffunction-sections" };

pub fn aggregate(exe: *microzig.EmbeddedExecutable) void {
    for (include_path) |path| {
        exe.addIncludePath(.{ .path = path });
    }

    for (source_path) |path| {
        exe.addCSourceFile(.{ .file = .{ .path = path }, .flags = &c_flags });
    }
}
