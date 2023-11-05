const std = @import("std");
const microzig = @import("microzig");

const include_path = [_][]const u8{
    "csrc/connectivity/paho.mqtt.embedded-c/MQTTPacket/src/",
};

const paho_src_path = "csrc/connectivity/paho.mqtt.embedded-c/MQTTPacket/src/";

const source_path = [_][]const u8{
    paho_src_path ++ "MQTTFormat.c",
    paho_src_path ++ "MQTTPacket.c",
    paho_src_path ++ "MQTTSerializePublish.c",
    paho_src_path ++ "MQTTDeserializePublish.c",
    paho_src_path ++ "MQTTConnectClient.c",
    paho_src_path ++ "MQTTSubscribeClient.c",
    paho_src_path ++ "MQTTUnsubscribeClient.c",
};

const c_flags = [_][]const u8{ "-DMQTT_CLIENT=1", "-O2", "-fdata-sections", "-ffunction-sections" };

pub fn aggregate(exe: *microzig.Firmware) void {
    for (include_path) |path| {
        exe.addIncludePath(.{ .path = path });
    }

    for (source_path) |path| {
        exe.addCSourceFile(.{ .file = .{ .path = path }, .flags = &c_flags });
    }
}
