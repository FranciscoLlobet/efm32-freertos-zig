const std = @import("std");
const board = @import("microzig").board;
const freertos = @import("freertos.zig");
const fatfs = @import("fatfs.zig");
const sha256 = @import("sha256.zig");
const pk = @import("pk.zig");
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

// open the configuration file, calculate the hash value and compare it with the nvm reference.

pub fn open_config_file(path: []const u8) !bool {
    var config_sha256: [32]u8 align(@alignOf(u32)) = undefined;

    const allocator = freertos.allocator;
    {
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

        _ = sha256_ctx.finish(&config_sha256);
    }
    // Read and compare the stored sha256

    var key_file = try fatfs.file.open("SD:CONFIG.PUB", @intFromEnum(fatfs.file.fMode.read));
    errdefer key_file.close() catch {};

    const key = try allocator.alloc(u8, key_file.size() + 1);
    defer allocator.free(key);

    @memset(key, 0);

    _ = try key_file.read(key, key.len);

    try key_file.close();

    var sig_file = try fatfs.file.open("SD:CONFIG.SIG", @intFromEnum(fatfs.file.fMode.read));
    defer sig_file.close() catch {};

    const sig = try allocator.alloc(u8, sig_file.size());
    defer allocator.free(sig);

    _ = try sig_file.read(sig, sig.len);

    var pk_ctx = pk.init();
    defer pk_ctx.free();

    var x = pk_ctx.parse(key);
    if (x == true) {
        x = pk_ctx.verify(&config_sha256, sig);
    }
    if (x == true) {
        _ = c.printf("CONFIG signature verified successfully\r\n");
    } else {
        _ = c.printf("CONFIG signature verification failed\r\n");
    }

    // if both are equal, then there is no need to reload the configuration

    // if not, then read the configuration file signature

    return true;
}
