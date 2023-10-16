const std = @import("std");
const microzig = @import("microzig");
const BoardDefinition = microzig.BoardDefinition;

const chips = @import("chips.zig");

fn get_root_path() []const u8 {
    return std.fs.path.dirname(@src().file) orelse unreachable;
}

const root_path = get_root_path() ++ "/";

pub const xdk110 = BoardDefinition{
    .name = "XDK110",
    .url = null,
    .source_file = .{ .cwd_relative = root_path ++ "/boards/xdk110.zig" },
};
