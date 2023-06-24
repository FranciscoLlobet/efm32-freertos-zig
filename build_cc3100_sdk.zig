const std = @import("std");
const microzig = @import("deps/microzig/build.zig");

const include_path = [_][]const u8{
    "csrc/system/cc3100-sdk/simplelink/include",
    "csrc/system/cc3100-sdk/oslib",
    "csrc/system/cc3100-sdk/netapps",
    // "csrc/system/cc3100-sdk/netapps/http/client",
    // "csrc/system/cc3100-sdk/netapps/mqtt/include",
    // "csrc/system/cc3100-sdk/netapps/mqtt/common",
    // "csrc/system/cc3100-sdk/netapps/mqtt/client",
    // "csrc/system/cc3100-sdk/netapps/mqtt/platform",
};

const source_path = [_][]const u8{
    "csrc/system/cc3100-sdk/oslib/osi_freertos.c",
    "csrc/system/cc3100-sdk/simplelink/source/device.c",
    "csrc/system/cc3100-sdk/simplelink/source/driver.c",
    "csrc/system/cc3100-sdk/simplelink/source/flowcont.c",
    "csrc/system/cc3100-sdk/simplelink/source/fs.c",
    "csrc/system/cc3100-sdk/simplelink/source/netapp.c",
    "csrc/system/cc3100-sdk/simplelink/source/netcfg.c",
    "csrc/system/cc3100-sdk/simplelink/source/nonos.c",
    "csrc/system/cc3100-sdk/simplelink/source/socket.c",
    "csrc/system/cc3100-sdk/simplelink/source/spawn.c",
    "csrc/system/cc3100-sdk/simplelink/source/wlan.c",
};

const c_flags = [_][]const u8{ "-DEFM32GG390F1024", "-D__OSI__=1", "-DOS_USE_TRACE_ITM", "-fdata-sections", "-ffunction-sections" };

pub fn aggregate(exe: *microzig.EmbeddedExecutable) void {
    for (include_path) |path| {
        exe.addIncludePath(path);
    }

    for (source_path) |path| {
        exe.addCSourceFile(path, &c_flags);
    }
}
