const std = @import("std");
const freertos = @import("freertos.zig");
const c = @cImport({
    @cInclude("board.h");
    @cInclude("nvm3.h");
    @cInclude("nvm3_hal_flash.h");
});
const main = @import("main.zig");

const app_nvm_keys = enum(u32) {
    start_counter = 0x00000,
    reset_counter = 0x00001,

    firmware_update_trigger,

    timestamp,

    device_uuid,

    // Configuration SHA256 hash checksum
    config_sha256 = 0x00010,

    wifi_ssid,
    wifi_psk,

    ntp_uri,

    // MQTT URI
    mqtt_uri,
    mqtt_psk_id,
    mqtt_psk_key,

    // Firmware download URI
    http_uri,
    http_psk_id,
    http_psk_key,

    // LWM2M Default URI
    lwm2m_uri,
    lwm2m_psk_id,
    lwmem_psk_key,

    // Firmware update info

    // Firmware SHA256 hash checksum
    firmware_sha256,
    firmware_update_state,

    max_key = 0x0FFFF,
};

const page_size_alignment: usize = 4096;
const nvm_storage_size: usize = 12 * page_size_alignment;
const max_object_size: usize = 256;
const cache_len: usize = 24;

const default_uuid: []const u8 = "xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx";
const default_sha256: [32]u8 = .{0xFF} ** 32;

pub var miso_nvm3: c.nvm3_Handle_t = undefined;
pub const miso_nvm3_init: c.nvm3_Init_t = .{ .nvmAdr = @ptrCast(@constCast(&nvm3Storage[0])), .nvmSize = nvm3Storage.len, .cachePtr = &cache, .cacheEntryCount = cache.len, .maxObjectSize = max_object_size, .repackHeadroom = 0, .halHandle = &c.nvm3_halFlashHandle };

export const nvm3Storage: [nvm_storage_size]u8 align(page_size_alignment) linksection(".flash") = .{0xFF} ** (nvm_storage_size);
var cache: [cache_len]c.nvm3_CacheEntry_t align(@alignOf(u32)) = undefined;

pub fn init() !u32 {
    var num_objects: usize = c.nvm3_countObjects(&miso_nvm3);

    if (num_objects < 2) {
        try writeCounter(.start_counter, 0);
        try writeCounter(.reset_counter, 0);
        //try writeData(.device_uuid, &default_uuid[0], default_uuid.len);
        //try writeData(.firmware_sha256, &default_sha256[0], default_sha256.len);
        //try writeData(.config_sha256, &default_sha256[0], default_sha256.len);
    }

    return 0;
}

pub fn close() void {
    _ = c.nvm3_close(&miso_nvm3);
}

fn readData(key: app_nvm_keys, value: *anyopaque, len: usize) !void {
    _ = c.nvm3_readData(&miso_nvm3, @intFromEnum(key), value, len);
}

fn writeData(key: app_nvm_keys, value: *const anyopaque, len: usize) !void {
    _ = c.nvm3_writeData(&miso_nvm3, @intFromEnum(key), value, len);
}

fn writeCounter(key: app_nvm_keys, value: u32) !void {
    _ = c.nvm3_writeCounter(&miso_nvm3, @intFromEnum(key), value);
}

fn incrementCounter(key: app_nvm_keys) !u32 {
    var newValue: u32 = 0;
    var counter = c.nvm3_incrementCounter(&miso_nvm3, @intFromEnum(key), &newValue);
    if (counter != 0) {
        // convert to error
    }
    return newValue;
}

pub fn incrementAppCounter() !u32 {
    return incrementCounter(app_nvm_keys.start_counter);
}
