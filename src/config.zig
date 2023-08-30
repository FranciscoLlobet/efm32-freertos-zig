const std = @import("std");
const board = @import("microzig").board;
const freertos = @import("freertos.zig");
const fatfs = @import("fatfs.zig");
const sha256 = @import("sha256.zig");
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

pub const rtos_prio_sensor = @intFromEnum(task_priorities.rtos_prio_low);
pub const rtos_stack_depth_sensor: u16 = 440;

// LWM2M
pub const rtos_prio_lwm2m = @intFromEnum(task_priorities.rtos_prio_normal);
pub const rtos_stack_depth_lwm2m: u16 = if (enable_lwm2m) 2000 else min_task_stack_depth;

// MQTT
pub const rtos_prio_mqtt = @intFromEnum(task_priorities.rtos_prio_normal);
pub const rtos_stack_depth_mqtt: u16 = if (enable_mqtt) 2000 else min_task_stack_depth;

// HTTP
pub const rtos_prio_http = @intFromEnum(task_priorities.rtos_prio_normal);
pub const rtos_stack_depth_http: u16 = if (enable_http) 2000 else min_task_stack_depth;

pub const rtos_prio_user_task = @intFromEnum(task_priorities.rtos_prio_normal);
pub const rtos_stack_depth_user_task: u16 = 2000;

// version
pub const miso_version_mayor: u8 = 0;
pub const miso_version_minor: u8 = 0;
pub const miso_version_patch: u8 = 1;
pub const miso_version: u32 = (miso_version_mayor << 16) | (miso_version_minor << 8) | (miso_version_patch);

/// Open file and calculate SHA256 hash
fn calculate_config_hash(path: []const u8, hash: *[32]u8) !void {
    const allocator = freertos.allocator;

    var file = try fatfs.file.open(path, @intFromEnum(fatfs.file.fMode.read));
    defer file.close() catch {};

    const mem_block = try allocator.alloc(u8, 512);
    defer allocator.free(mem_block);

    var sha256_ctx = sha256.init();
    defer sha256_ctx.free();

    _ = sha256_ctx.start();

    while (!file.eof()) {
        var br = try file.read(mem_block, mem_block.len);
        _ = sha256_ctx.update(mem_block, br);
    }

    _ = sha256_ctx.finish(hash);
}

/// Load a public key im PEM format into PK context
fn load_public_key_from_file(path: []const u8, pk_ctx: *pk) !void {
    const allocator = freertos.allocator;

    var key_file = try fatfs.file.open(path, @intFromEnum(fatfs.file.fMode.read));
    defer key_file.close() catch {};

    const key = try allocator.alloc(u8, key_file.size() + 1);
    defer allocator.free(key);

    @memset(key, 0);

    _ = try key_file.read(key, key.len);

    _ = pk_ctx.parse(key);
}

/// Open the configuration file, calculate the hash value and compare it with the nvm reference.
pub fn open_config_file(path: []const u8) !bool {
    var config_sha256: [32]u8 align(@alignOf(u32)) = undefined;
    const allocator = freertos.allocator;

    var ref_config_sha256: [32]u8 = undefined;

    // Read hash from NVM
    nvm.readData(.config_sha256, &ref_config_sha256[0], ref_config_sha256.len) catch {
        @memset(&ref_config_sha256, 0);
    };

    try calculate_config_hash(path, &config_sha256); // open config file and calculate the hash

    if (std.mem.eql(u8, &ref_config_sha256, &config_sha256)) {
        return true;
    } else {
        var sig_file = try fatfs.file.open("SD:CONFIG.SIG", @intFromEnum(fatfs.file.fMode.read));
        errdefer sig_file.close() catch {};

        const sig = try allocator.alloc(u8, sig_file.size());
        defer allocator.free(sig);

        _ = try sig_file.read(sig, sig.len);

        try sig_file.close();

        // Load the Public Key
        var pk_ctx = pk.init();
        defer pk_ctx.free();

        load_public_key_from_file("SD:CONFIG.PUB", &pk_ctx) catch { // Load from NVM};

        };

        var x = pk_ctx.verify(&config_sha256, sig);

        if (x == true) {
            _ = c.printf("CONFIG signature verified successfully\r\n");

            c.miso_load_config(); // Process the config

            try nvm.writeData(.config_sha256, &config_sha256[0], config_sha256.len);

            return true;
        } else {
            _ = c.printf("CONFIG signature verification failed\r\n");
            return false;
        }
    }
    // if both are equal, then there is no need to reload the configuration

    // if not, then read the configuration file signature

    return true;
}

pub fn store_config_in_nvm() !void {
    // WIFI
    try nvm.writeDataCString(.wifi_ssid, c.config_get_wifi_ssid());
    try nvm.writeDataCString(.wifi_psk, c.config_get_wifi_key());

    // LWM2M
    try nvm.writeDataCString(.lwm2m_uri, c.config_get_lwm2m_uri());
    try nvm.writeDataCString(.lwm2m_psk_id, c.config_get_lwm2m_psk_id());
    try nvm.writeDataCString(.lwm2m_psk_key, c.config_get_lwm2m_psk_key());

    // MQTT
    try nvm.writeDataCString(.mqtt_uri, c.config_get_mqtt_url());
    try nvm.writeDataCString(.mqtt_psk_id, c.config_get_mqtt_psk_id());
    try nvm.writeDataCString(.mqtt_psk_key, c.config_get_mqtt_psk_key());
    try nvm.writeDataCString(.mqtt_device_id, c.config_get_mqtt_device_id());

    // HTTP
    try nvm.writeDataCString(.http_uri, c.config_get_http_uri());
}

pub fn load_config_from_nvm() !void {

    // Wifi
    nvm.readCString(.wifi_ssid, c.config_get_wifi_ssid()) catch {};
    nvm.readCString(.wifi_psk, c.config_get_wifi_key()) catch {};

    // LWM2M
    nvm.readCString(.lwm2m_uri, c.config_get_lwm2m_uri()) catch {};
    nvm.readCString(.lwm2m_psk_id, c.config_get_lwm2m_psk_id()) catch {};
    nvm.readCString(.lwm2m_psk_key, c.config_get_lwm2m_psk_key()) catch {};

    // MQTT
    nvm.readCString(.mqtt_uri, c.config_get_mqtt_url()) catch {};
    nvm.readCString(.mqtt_psk_id, c.config_get_mqtt_psk_id()) catch {};
    nvm.readCString(.mqtt_psk_key, c.config_get_mqtt_psk_key()) catch {};
    nvm.readCString(.mqtt_device_id, c.config_get_mqtt_device_id()) catch {};

    // HTTP
    nvm.readCString(.http_uri, c.config_get_http_uri()) catch {};
}
