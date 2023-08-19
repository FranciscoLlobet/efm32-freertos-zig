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

    max_key = 0x0FFFF,
};

const page_size_alignment: usize = 4096;
const nvm_storage_size: usize = 12 * page_size_alignment;

pub var miso_nvm3: c.nvm3_Handle_t = undefined;
pub var miso_nvm3_init: c.nvm3_Init_t = .{ .nvmAdr = @ptrCast(@constCast(&nvm3Storage[0])), .nvmSize = nvm3Storage.len, .cachePtr = &cache, .cacheEntryCount = cache.len, .maxObjectSize = 256, .repackHeadroom = 0, .halHandle = &c.nvm3_halFlashHandle };

export const nvm3Storage: [nvm_storage_size]u8 align(page_size_alignment) linksection(".flash") = .{0xFF} ** (nvm_storage_size);
var cache: [24]c.nvm3_CacheEntry_t align(@alignOf(u32)) = undefined;

pub fn init() !u32 {
    var num_objects: usize = c.nvm3_countObjects(&miso_nvm3);

    if (num_objects < 2) {
        _ = c.nvm3_writeCounter(&miso_nvm3, @intFromEnum(app_nvm_keys.start_counter), 0);
        _ = c.nvm3_writeCounter(&miso_nvm3, @intFromEnum(app_nvm_keys.reset_counter), 0);
    }

    return 0;
}

pub fn incrementCounter(key: app_nvm_keys) !u32 {
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
