const std = @import("std");
const microzig = @import("deps/microzig/build.zig");

const include_path = [_][]const u8{
    "csrc/connectivity/wakaama/include",
    "csrc/connectivity/wakaama/core",
    "csrc/connectivity/wakaama/coap",
    "csrc/src/lwm2m/",
};

const base_src_path = "csrc/connectivity/wakaama/";
const usr_src_path = "csrc/src/lwm2m/";

const source_path = [_][]const u8{
    base_src_path ++ "coap/block.c",
    base_src_path ++ "coap/transaction.c",
    base_src_path ++ "coap/er-coap-13/er-coap-13.c",
    base_src_path ++ "core/bootstrap.c",
    base_src_path ++ "core/discover.c",
    base_src_path ++ "core/liblwm2m.c",
    base_src_path ++ "core/list.c",
    base_src_path ++ "core/management.c",
    base_src_path ++ "core/objects.c",
    base_src_path ++ "core/observe.c",
    base_src_path ++ "core/packet.c",
    base_src_path ++ "core/registration.c",
    base_src_path ++ "core/uri.c",
    base_src_path ++ "core/utils.c",
    base_src_path ++ "data/data.c",
    base_src_path ++ "data/json_common.c",
    base_src_path ++ "data/json.c",
    base_src_path ++ "data/senml_json.c",
    base_src_path ++ "data/tlv.c",

    usr_src_path ++ "server_cert.c",
    usr_src_path ++ "platform.c",
    usr_src_path ++ "memtrace.c",
    usr_src_path ++ "connection.c",
    usr_src_path ++ "commandline.c",
    usr_src_path ++ "lightclient/mbedtls_connector.c",
    usr_src_path ++ "lightclient/object_accelerometer.c",
    usr_src_path ++ "lightclient/object_device_capability.c",
    usr_src_path ++ "lightclient/object_device.c",
    usr_src_path ++ "lightclient/object_security.c",
    usr_src_path ++ "lightclient/object_server.c",
    usr_src_path ++ "lightclient/object_temperature.c",
    usr_src_path ++ "lightclient/uiso_lightclient.c",
};

const c_flags = [_][]const u8{ "-DMBEDTLS_CONFIG_FILE=\"miso_mbedtls_config.h\"", "-DLWM2M_CLIENT_MODE=1","-DLWM2M_COAP_DEFAULT_BLOCK_SIZE=512", "-DLWM2M_SUPPORT_JSON=1", "-DLWM2M_SUPPORT_SENML_JSON=1", "-DEFM32GG390F1024", "-O2", "-fdata-sections", "-ffunction-sections" };

pub fn aggregate(exe: *microzig.EmbeddedExecutable) void {
    for (include_path) |path| {
        exe.addIncludePath(path);
    }

    for (source_path) |path| {
        exe.addCSourceFile(path, &c_flags);
    }
}
