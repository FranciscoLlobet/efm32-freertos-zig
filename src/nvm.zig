const std = @import("std");
const c = @cImport({
    @cInclude("board.h");
    @cInclude("nvm3.h");
    @cInclude("nvm3_hal_flash.h");
    @cInclude("string.h");
});

/// NVM Keys used in the App
/// NVM keys are 20-Bit unsigned integers
///
pub const app_nvm_keys = enum(u20) {
    /// APP Start Counter
    start_counter = 0x00000, // Boot Counter

    /// APP Reset Counter
    reset_counter = 0x00001,

    timestamp,

    device_uuid,

    // Configuration SHA256 hash checksum
    config_sha256 = 0x00010,
    config_pubkey,

    wifi_ssid,
    wifi_psk,

    /// NTP URI in format: sntp://host:port
    ntp_uri,
    // Secondary NTP uri

    /// MQTT URI in format: mqtt(s)://host:port
    mqtt_uri,

    /// PSK ID
    mqtt_psk_id,

    /// PSK Key
    mqtt_psk_key,

    /// Device ID
    mqtt_device_id,

    /// HTTP Firmware URI
    http_uri,

    /// HTTP Firmware sig
    http_sig_uri,

    /// HTTP Firmware public key
    http_sig_key,

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

    // Config uri
    config_uri,

    /// Bootloader Update Flag
    update_request,

    /// Bootloader Boot Request
    /// This flag is set by the bootloader after performing a firmware update
    /// and before booting the application. The application should clear this flag
    boot_request, //

    firmware_size,

    max_key = 0x0FFFF,

    fn toInt(self: @This()) u32 {
        return @as(u32, @intFromEnum(self));
    }
};

const nvmError = error{
    generic_error,
    buffer_size_error,
    invalid_object_type,
};

const ret = enum(u32) {
    nvm_ok = c.ECODE_NVM3_OK,

    /// Primitive/simple error checker and converter
    ///
    /// Takes the result error code from the NVM3 API and converts it into a Zig error
    fn check(ecode: c.Ecode_t) nvmError!void {
        if (@intFromEnum(@This().nvm_ok) != ecode) {
            return nvmError.generic_error;
        }
    }
};

//const page_size_alignment: usize = 4096;
const max_object_size: usize = 256;
const cache_len: usize = 48;

/// Silabs NVM3 handle
pub var miso_nvm3: c.nvm3_Handle_t = undefined;

/// NVM Size in Bytes
const nvm_size_in_bytes: usize = 16 * 4096;

/// NVM3 initial address.
/// To-do: Take this configuration from the linker script
const nvm_initial_address: c.nvm3_HalPtr_t = @ptrFromInt(0x000F0000); // 1024*1024 - 16*4096

/// NVM3 initial configuration
pub export const miso_nvm3_init: c.nvm3_Init_t = .{ .nvmAdr = nvm_initial_address, .nvmSize = nvm_size_in_bytes, .cachePtr = &cache, .cacheEntryCount = cache.len, .maxObjectSize = max_object_size, .repackHeadroom = 0, .halHandle = &c.nvm3_halFlashHandle };

/// NVM3 cache
var cache: [cache_len]c.nvm3_CacheEntry_t align(@alignOf(u32)) = undefined;

pub fn init() !u32 {
    const default_uuid: [36]u8 align(@alignOf(u32)) = .{0xFF} ** 36;
    const default_sha256: [32]u8 align(@alignOf(u32)) = .{0xFF} ** 32;

    var num_objects: usize = countObjects();

    if (num_objects < 9) {
        try eraseAll();
        try writeCounter(.start_counter, 0);
        try writeCounter(.reset_counter, 0);
        try writeCounter(.timestamp, 0);
        try writeCounter(.firmware_download_counter, 0);
        try writeData(.device_uuid, &default_uuid);
        try writeData(.config_sha256, &default_sha256);

        try writeCounter(.boot_request, 0);
        try writeCounter(.update_request, 0);
        try writeCounter(.firmware_size, 0);
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

    try ret.check(c.nvm3_getObjectInfo(&miso_nvm3, key.toInt(), &obj_type, &len));

    return .{ .object_type = @as(objectType, @enumFromInt(obj_type)), .object_len = len };
}

pub fn eraseAll() !void {
    try ret.check(c.nvm3_eraseAll(&miso_nvm3));
}

pub fn countObjects() usize {
    return c.nvm3_countObjects(&miso_nvm3);
}

/// Read data from NVM into a Zig slice
pub fn readData(key: app_nvm_keys, buffer: []u8) ![]u8 {
    var len: usize = 0;
    var object_type: u32 = undefined;

    try ret.check(c.nvm3_getObjectInfo(&miso_nvm3, key.toInt(), &object_type, &len));
    if (len > buffer.len) {
        return nvmError.buffer_size_error;
    } else if (object_type != @intFromEnum(objectType.data)) {
        return nvmError.invalid_object_type;
    }

    try ret.check(c.nvm3_readData(&miso_nvm3, key.toInt(), buffer.ptr, len));

    return buffer[0..len];
}

/// Read a 32-Bit counter value
pub fn readCounter(key: app_nvm_keys) !u32 {
    var value: u32 = 0;

    try ret.check(c.nvm3_readCounter(&miso_nvm3, key.toInt(), &value));

    return value;
}

/// Read a C-String from NVM and store it into given location
pub fn readCString(key: app_nvm_keys, value: [*c]u8) !void {
    var len: usize = 0;
    var object_type: u32 = undefined;

    try ret.check(c.nvm3_getObjectInfo(&miso_nvm3, key.toInt(), &object_type, &len));
    if (object_type != @intFromEnum(objectType.data)) {
        return nvmError.invalid_object_type;
    }

    try ret.check(c.nvm3_readData(&miso_nvm3, key.toInt(), value, len));
}

/// Write data into NVM using Zig slices
pub fn writeData(key: app_nvm_keys, value: []const u8) !void {
    try ret.check(c.nvm3_writeData(&miso_nvm3, key.toInt(), @ptrCast(value), value.len));
}

/// Write a C-String into NVM
pub fn writeDataCString(key: app_nvm_keys, value: [*c]const u8) !void {
    const len: usize = c.strlen(value) + @as(usize, 1);
    try ret.check(c.nvm3_writeData(&miso_nvm3, key.toInt(), value, len));
}

/// Write a 32-Bit counter value
fn writeCounter(key: app_nvm_keys, value: u32) !void {
    try ret.check(c.nvm3_writeCounter(&miso_nvm3, key.toInt(), value));
}

/// Increment counter value
fn incrementCounter(key: app_nvm_keys) !u32 {
    var newValue: u32 = 0;
    try ret.check(c.nvm3_incrementCounter(&miso_nvm3, key.toInt(), &newValue));

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

/// Set a Boot request from the Bootloader
pub inline fn setBootRequest() !void {
    _ = try incrementCounter(.boot_request);
}

/// Should be cleared by the application once it has booted successfully
pub inline fn clearBootRequest() !void {
    try writeCounter(.boot_request, 0);
}

/// Check if the Boot request flag is set
pub inline fn isBootRequested() !bool {
    return (try readCounter(.boot_request)) > 2;
}

pub inline fn setUpdateRequest() !void {
    _ = try incrementCounter(.update_request);
}

pub inline fn clearUpdateRequest() !void {
    try writeCounter(.update_request, 0);
}

pub inline fn isUpdateRequested() !bool {
    return (try readCounter(.update_request)) != 0;
}

pub inline fn setFirmwareSize(size: u32) !void {
    try writeCounter(.firmware_size, size);
}

pub inline fn getFirmwareSize() !u32 {
    return try readCounter(.firmware_size);
}
