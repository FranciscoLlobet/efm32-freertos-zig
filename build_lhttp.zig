const std = @import("std");
const microzig = @import("deps/microzig/build.zig");

const include_path = [_][]const u8{ "csrc/connectivity/llhttp/src/native", "csrc/connectivity/picohttpparser" };

const lhttp_src_path = "csrc/connectivity/llhttp/src/native/";
const picohttpp_src_path = "csrc/connectivity/picohttpparser/";

const source_path = [_][]const u8{ picohttpp_src_path ++ "picohttpparser.c", lhttp_src_path ++ "api.c", lhttp_src_path ++ "http.c", "csrc/src/llhttp.c" };

const c_flags = [_][]const u8{ "-DMQTT_CLIENT=1", "-O2", "-fdata-sections", "-ffunction-sections" };

pub fn aggregate(exe: *microzig.EmbeddedExecutable) void {
    for (include_path) |path| {
        exe.addIncludePath(.{ .path = path });
    }

    for (source_path) |path| {
        exe.addCSourceFile(.{ .file = .{ .path = path }, .flags = &c_flags });
    }
}
