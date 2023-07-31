const std = @import("std");
const microzig = @import("deps/microzig/build.zig");

const include_path = [_][]const u8{
    "csrc/connectivity/MQTT-C/include",
    "csrc/connectivity/paho.mqtt.embedded-c/MQTTPacket/src/",
};

const base_src_path = "csrc/connectivity/MQTT-C/src/";
const paho_src_path = "csrc/connectivity/paho.mqtt.embedded-c/MQTTPacket/src/";

const source_path = [_][]const u8{
    base_src_path ++ "mqtt.c",
    paho_src_path ++ "MQTTFormat.c",
    paho_src_path ++ "MQTTPacket.c",
    paho_src_path ++ "MQTTSerializePublish.c",
    paho_src_path ++ "MQTTDeserializePublish.c",
    paho_src_path ++ "MQTTConnectClient.c",
    paho_src_path ++ "MQTTSubscribeClient.c",
    paho_src_path ++ "MQTTUnsubscribeClient.c",
};

const c_flags = [_][]const u8{ "-DMQTT_CLIENT=1", "-DMQTTC_PAL_FILE=miso_mqtt_pal.h", "-DMBEDTLS_CONFIG_FILE=\"miso_mbedtls_config.h\"", "-DEFM32GG390F1024", "-O2", "-fdata-sections", "-ffunction-sections" };

pub fn aggregate(exe: *microzig.EmbeddedExecutable) void {
    for (include_path) |path| {
        exe.addIncludePath(path);
    }

    for (source_path) |path| {
        exe.addCSourceFile(path, &c_flags);
    }
}
