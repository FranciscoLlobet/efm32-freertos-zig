const std = @import("std");
const microzig = @import("../deps/microzig/build.zig");
const Board = microzig.Board;

const chips = @import("chips.zig");

fn get_root_path() []const u8 {
    return std.fs.path.dirname(@src().file) orelse unreachable;
}

const root_path = get_root_path() ++ "/";

pub const xdk110 = Board{
    .name = "XDK110",
    .source = .{ .path = root_path ++ "boards/xdk110.zig" },
    .chip = chips.efm32gg390f1024,
};
