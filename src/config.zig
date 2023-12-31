const std = @import("std");
const board = @import("microzig").board;
const freertos = @import("freertos.zig");
const fatfs = @import("fatfs.zig");
const sha256 = @import("sha256.zig");
const mbedtls = @import("mbedtls.zig");
const pk = @import("pk.zig");
const nvm = @import("nvm.zig");
pub const c = @cImport({
    @cInclude("miso_config.h");
});

const task_priorities = enum(freertos.BaseType_t) {
    rtos_prio_idle = 0, // Idle task priority
    rtos_prio_low = 1,
    rtos_prio_below_normal = 2,
    rtos_prio_normal = 3, // Normal Service Tasks
    rtos_prio_above_normal = 4,
    rtos_prio_high = 5,
    rtos_prio_highest = 6, // TimerService
};

pub const min_task_stack_depth: u16 = freertos.c.configMINIMAL_STACK_SIZE;

// Enable or Disable features at compile time
pub const enable_lwm2m = false;
pub const enable_mqtt = true;
pub const enable_http = true;

pub const rtos_prio_boot_app = @intFromEnum(task_priorities.rtos_prio_highest);

// SENSOR TASK
pub const rtos_prio_sensor = @intFromEnum(task_priorities.rtos_prio_low);
pub const rtos_stack_depth_sensor: u16 = 440;

// TIMER TASK
pub const rtos_stack_depth_timer_task: u16 = 512;

// IDLE Tasj
pub const rtos_stack_depth_idle_task: u16 = 200;

// LWM2M
pub const rtos_prio_lwm2m = @intFromEnum(task_priorities.rtos_prio_normal);
pub const rtos_stack_depth_lwm2m: u16 = if (enable_lwm2m) 2000 else min_task_stack_depth;

// MQTT
pub const rtos_prio_mqtt = @intFromEnum(task_priorities.rtos_prio_normal);
pub const rtos_stack_depth_mqtt: u16 = if (enable_mqtt) 1400 else min_task_stack_depth;

// HTTP
pub const rtos_prio_http = @intFromEnum(task_priorities.rtos_prio_normal);
pub const rtos_stack_depth_http: u16 = if (enable_http) 2000 else min_task_stack_depth;

pub const rtos_prio_user_task = @intFromEnum(task_priorities.rtos_prio_below_normal);
pub const rtos_stack_depth_user_task: u16 = 1500;

// version
pub const miso_version_mayor: u8 = 0;
pub const miso_version_minor: u8 = 0;
pub const miso_version_patch: u8 = 1;
pub const miso_version: u32 = (miso_version_mayor << 16) | (miso_version_minor << 8) | (miso_version_patch);

/// Default Firmware Bin Location
pub const fw_file_name = "SD:FW.BIN";

/// Default Firmware Signature Location
pub const fw_sig_file_name = "SD:FW.SIG";

/// Default Config Firmware Location
pub const config_file_name = "SD:CONFIG.TXT";

/// Default Config Signature Location
pub const config_sig_file_name = "SD:CONFIG.SIG";

/// Default Config Public Key Location
pub const config_pub_key_file_name = "SD:CONFIG.PUB";

/// Default APP backup Firmware Location
pub const app_backup_file_name = "SD:APP.BAK";

// Allocator used in the application
pub const allocator = freertos.allocator;

pub const config_local_error = error{
    signature_verification_failed,
};

/// Error Union in config
pub const config_error = (fatfs.frError || sha256.sha256_error || std.mem.Allocator.Error || pk.pk_error || mbedtls.mbedtls_error);

/// Const File block Size
const file_block_size: usize = 512;

/// Helper Getters
pub inline fn getHttpSigKey() []u8 {
    return c.config_get_http_sig_key()[0..c.strlen(c.config_get_http_sig_key())];
}

pub inline fn getHttpFwUri() []u8 {
    return c.config_get_http_uri()[0..c.strlen(c.config_get_http_uri())];
}

pub inline fn getHttpSigUri() []u8 {
    return c.config_get_http_sig_uri()[0..c.strlen(c.config_get_http_sig_uri())];
}

/// Open file and calculate SHA256 hash
/// Precondition: The File System has been mounted (!)
pub fn calculateFileHash(path: [*:0]const u8, hash: *[32]u8) config_error![]u8 {
    var file = try fatfs.file.open(path, @intFromEnum(fatfs.file.fMode.read));
    defer file.close() catch {};

    const mem_block = try allocator.alloc(u8, if (file.size() >= file_block_size) file_block_size else file.size());
    defer allocator.free(mem_block);

    var sha256_ctx = sha256.init();
    defer sha256_ctx.free();

    try sha256_ctx.start();

    while (try file.readEof(mem_block)) |br| {
        try sha256_ctx.update(br);
    }

    try sha256_ctx.finish(hash);

    return hash[0..];
}

/// Calculate SHA256 hash of a slice in memory
pub fn calculateMemHash(slice: []u8, hash: *[32]u8) config_error![]u8 {
    // const mem_block = try allocator.alloc(u8, file_block_size);
    // defer allocator.free(mem_block);

    var sha256_ctx = sha256.init();
    defer sha256_ctx.free();

    try sha256_ctx.start();

    //ar current_pos: usize = 0;
    //while (current_pos < slice.len) {
    //    const start_pos = current_pos;
    //    const end_pos: usize = if ((start_pos + mem_block.len) > slice.len) slice.len else (start_pos + mem_block.len);

    try sha256_ctx.update(slice);
    //    current_pos = end_pos;
    //}

    try sha256_ctx.finish(hash);

    return hash[0..];
}

/// Load a public key im PEM format into PK context
fn load_public_key_from_file(path: [*:0]const u8, pk_ctx: *pk) config_error!void {
    //const allocator = freertos.allocator;

    var key_file = try fatfs.file.open(path, @intFromEnum(fatfs.file.fMode.read));
    defer key_file.close() catch {};

    const key = try allocator.alloc(u8, key_file.size() + @as(usize, 1));
    defer allocator.free(key);

    @memset(key, 0);

    _ = try key_file.read(key);

    try pk_ctx.parse(key);
}

// Load public key in pseudo-der format into PK context
pub fn load_public_key_from_nvm(nvm_str: []u8, pk_ctx: *pk) !void {
    const key = try allocator.alloc(u8, nvm_str.len);
    defer allocator.free(key);

    @memset(key, 0);

    try pk_ctx.parse(try mbedtls.base64Decode(@ptrCast(nvm_str.ptr), key));
}

/// Verify Firmware Candidate against the signature.
///
/// - fw_path : Path to the firmware file in SD card
/// - sig_path : Path to the signature file in SD card
/// - nvm_str : NVM string containing the public key in base64DER (stripped PEM) format
pub fn verifyFirmwareSignature(fw_path: [*:0]const u8, sig_path: [*:0]const u8, nvm_str: []u8) config_error!void {
    var fw_sha256: [32]u8 align(@alignOf(u32)) = undefined;

    @memset(&fw_sha256, 0);

    const hash = try calculateFileHash(fw_path, &fw_sha256); // open fw file and calculate the hash

    var sig_file = try fatfs.file.open(sig_path, @intFromEnum(fatfs.file.fMode.read));
    errdefer sig_file.close() catch {};

    const sig = try allocator.alloc(u8, sig_file.size());
    defer allocator.free(sig);

    @memset(sig, 0);

    _ = try sig_file.read(sig);

    try sig_file.close();

    var pk_ctx = pk.init();
    defer pk_ctx.free();

    try load_public_key_from_nvm(nvm_str, &pk_ctx);

    try pk_ctx.verify(hash, sig);
}

/// Open the configuration file, calculate the hash value and compare it with the nvm reference.
pub fn open_config_file(path: [*:0]const u8) !void {
    var config_sha256: [32]u8 align(@alignOf(u32)) = undefined;
    var ref_config_sha256: [32]u8 align(@alignOf(u32)) = undefined;

    @memset(&config_sha256, 0);
    @memset(&ref_config_sha256, 0);

    // Read hash from NVM
    _ = nvm.readData(.config_sha256, &ref_config_sha256) catch {
        @memset(&ref_config_sha256, 0);
    };

    const hash = try calculateFileHash(path, &config_sha256); // open config file and calculate the hash

    if (!std.mem.eql(u8, &ref_config_sha256, hash)) {
        var sig_file = try fatfs.file.open(config_sig_file_name, @intFromEnum(fatfs.file.fMode.read));
        errdefer sig_file.close() catch {};

        const sig = try allocator.alloc(u8, sig_file.size());
        defer allocator.free(sig);

        _ = try sig_file.read(sig);

        try sig_file.close();

        // Load the Public Key
        var pk_ctx = pk.init();
        defer pk_ctx.free();

        load_public_key_from_file(config_pub_key_file_name, &pk_ctx) catch {};

        pk_ctx.verify(hash, sig) catch |err| {
            _ = c.printf("CONFIG signature verification failed\r\n");
            return err;
        };

        _ = c.printf("CONFIG signature verified successfully\r\n");

        c.miso_load_config(); // Process the config

        try store_config_in_nvm();

        try nvm.writeData(.config_sha256, hash);
    } else {
        _ = c.printf("CONFIG Hash as in NVM\r\n");
    }
}

pub fn store_config_in_nvm() !void {
    // WIFI
    try nvm.writeDataCString(.wifi_ssid, c.config_get_wifi_ssid());
    try nvm.writeDataCString(.wifi_psk, c.config_get_wifi_key());

    // LWM2M
    try nvm.writeDataCString(.lwm2m_uri, c.config_get_lwm2m_uri());
    try nvm.writeDataCString(.lwm2m_device_id, c.config_get_lwm2m_endpoint());
    try nvm.writeDataCString(.lwm2m_psk_id, c.config_get_lwm2m_psk_id());
    try nvm.writeDataCString(.lwm2m_psk_key, c.config_get_lwm2m_psk_key());

    // MQTT
    try nvm.writeDataCString(.mqtt_uri, c.config_get_mqtt_url());
    try nvm.writeDataCString(.mqtt_psk_id, c.config_get_mqtt_psk_id());
    try nvm.writeDataCString(.mqtt_psk_key, c.config_get_mqtt_psk_key());
    try nvm.writeDataCString(.mqtt_device_id, c.config_get_mqtt_device_id());

    // HTTP
    try nvm.writeDataCString(.http_uri, c.config_get_http_uri());
    try nvm.writeDataCString(.http_sig_uri, c.config_get_http_sig_uri());
    try nvm.writeDataCString(.http_sig_key, c.config_get_http_sig_key());
}

pub fn load_config_from_nvm() !void {
    // Wifi
    nvm.readCString(.wifi_ssid, c.config_get_wifi_ssid()) catch {};
    nvm.readCString(.wifi_psk, c.config_get_wifi_key()) catch {};

    // LWM2M
    nvm.readCString(.lwm2m_uri, c.config_get_lwm2m_uri()) catch {};
    nvm.readCString(.lwm2m_device_id, c.config_get_lwm2m_endpoint()) catch {};
    nvm.readCString(.lwm2m_psk_id, c.config_get_lwm2m_psk_id()) catch {};
    nvm.readCString(.lwm2m_psk_key, c.config_get_lwm2m_psk_key()) catch {};

    // MQTT
    nvm.readCString(.mqtt_uri, c.config_get_mqtt_url()) catch {};
    nvm.readCString(.mqtt_psk_id, c.config_get_mqtt_psk_id()) catch {};
    nvm.readCString(.mqtt_psk_key, c.config_get_mqtt_psk_key()) catch {};
    nvm.readCString(.mqtt_device_id, c.config_get_mqtt_device_id()) catch {};

    // HTTP
    nvm.readCString(.http_uri, c.config_get_http_uri()) catch {};
    nvm.readCString(.http_sig_uri, c.config_get_http_sig_uri()) catch {};
    nvm.readCString(.http_sig_key, c.config_get_http_sig_key()) catch {};
}
