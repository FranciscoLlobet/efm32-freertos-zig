const std = @import("std");
const freertos = @import("freertos.zig");
const c = @cImport({
    @cInclude("board.h");
    @cInclude("nvm3.h");
    @cInclude("nvm3_hal_flash.h");
    @cInclude("string.h");
});
const main = @import("main.zig");

/// NVM Keys used in the App
pub const app_nvm_keys = enum(u20) {
    /// APP Start Counter
    start_counter = 0x00000,

    /// APP Reset Counter
    reset_counter = 0x00001,

    timestamp,

    device_uuid,

    // Configuration SHA256 hash checksum
    config_sha256 = 0x00010,
    config_pubkey,

    wifi_ssid,
    wifi_psk,

    ntp_uri,

    /// MQTT URI in format: mqtt(s)://host:port
    mqtt_uri,

    /// PSK ID
    mqtt_psk_id,

    /// PSK Key
    mqtt_psk_key,

    /// Device ID
    mqtt_device_id,

    // Firmware download URI
    http_uri,
    http_psk_id,
    http_psk_key,

    // LWM2M Default URI
    lwm2m_uri,
    lwm2m_psk_id,
    lwm2m_psk_key,
    lwm2m_device_id,

    // Firmware update info

    // Firmware SHA256 hash checksum
    firmware_etag,
    firmware_update_trigger,
    firmware_download_counter,

    firmware_update_state,

    max_key = 0x0FFFF,
};

const nvmError = error{generic_error};

const ret = enum(u32) {
    nvm_ok = 0,

    fn check(ecode: c.Ecode_t) nvmError!void {
        if (ecode != c.ECODE_NVM3_OK) {
            return nvmError.generic_error;
        }
    }
};

//const page_size_alignment: usize = 4096;
const max_object_size: usize = 256;
const cache_len: usize = 48;

pub var miso_nvm3: c.nvm3_Handle_t = undefined;
pub const miso_nvm3_init: c.nvm3_Init_t = .{ .nvmAdr = @ptrCast(@as([*c]u8, @ptrFromInt(0x000F0000))), .nvmSize = 16 * 4096, .cachePtr = &cache, .cacheEntryCount = cache.len, .maxObjectSize = max_object_size, .repackHeadroom = 0, .halHandle = &c.nvm3_halFlashHandle };

//export const nvm3Storage: [nvm_storage_size]u8 align(page_size_alignment) linksection(".NVM") = .{0xFF} ** (nvm_storage_size);
var cache: [cache_len]c.nvm3_CacheEntry_t align(@alignOf(u32)) = undefined;

pub fn init() !u32 {
    const default_uuid: [36]u8 = .{0xFF} ** 36;
    const default_sha256: [32]u8 = .{0xFF} ** 32;

    var num_objects: usize = countObjects();

    if (num_objects < 4) {
        try eraseAll();
        try writeCounter(.start_counter, 0);
        try writeCounter(.reset_counter, 0);
        try writeCounter(.timestamp, 0);
        try writeCounter(.firmware_download_counter, 0);
        try writeData(.device_uuid, &default_uuid[0], default_uuid.len);
        try writeData(.config_sha256, &default_sha256[0], default_sha256.len);
    }

    return 0;
}

pub fn close() void {
    _ = c.nvm3_close(&miso_nvm3);
}

const objectType = enum(u32) {
    data = c.NVM3_OBJECTTYPE_DATA,
    counter = c.NVM3_OBJECTTYPE_COUNTER,
};

pub fn getObjectInfo(key: app_nvm_keys) !struct { object_type: objectType, object_len: usize } {
    var len: usize = 0;
    var obj_type: u32 = 0;

    try ret.check(c.nvm3_getObjectInfo(&miso_nvm3, @intFromEnum(key), &obj_type, &len));

    return .{ .object_type = @as(objectType, @enumFromInt(obj_type)), .object_len = len };
}

pub fn eraseAll() !void {
    try ret.check(c.nvm3_eraseAll(&miso_nvm3));
}

pub fn countObjects() usize {
    return c.nvm3_countObjects(&miso_nvm3);
}

pub fn readData(key: app_nvm_keys, value: [*c]u8, len: usize) !void {
    try ret.check(c.nvm3_readData(&miso_nvm3, @intFromEnum(key), value, len));
}

pub fn readCString(key: app_nvm_keys, value: [*c]u8) !void {
    var len: usize = 0;
    var object_type: u32 = undefined;

    try ret.check(c.nvm3_getObjectInfo(&miso_nvm3, @intFromEnum(key), &object_type, &len));
    try ret.check(c.nvm3_readData(&miso_nvm3, @intFromEnum(key), value, len));
}

/// Write data into NVM
pub fn writeData(key: app_nvm_keys, value: [*c]const u8, len: usize) !void {
    try ret.check(c.nvm3_writeData(&miso_nvm3, @intFromEnum(key), value, len));
}

/// Write a C-String into NVM
pub fn writeDataCString(key: app_nvm_keys, value: [*c]const u8) !void {
    const len: usize = c.strlen(value) + @as(usize, 1);
    try ret.check(c.nvm3_writeData(&miso_nvm3, @intFromEnum(key), value, len));
}

/// Write a 32-Bit counter value
fn writeCounter(key: app_nvm_keys, value: u32) !void {
    try ret.check(c.nvm3_writeCounter(&miso_nvm3, @intFromEnum(key), value));
}

/// Increment counter value
fn incrementCounter(key: app_nvm_keys) !u32 {
    var newValue: u32 = 0;
    try ret.check(c.nvm3_incrementCounter(&miso_nvm3, @intFromEnum(key), &newValue));

    return newValue;
}

/// Increment the APP start counter
pub fn incrementAppCounter() !u32 {
    return try incrementCounter(.start_counter);
}

/// Increment the reset counter
pub fn incrementResetCounter() !u32 {
    return try incrementCounter(.reset_counter);
}
