const std = @import("std");
const freertos = @import("freertos.zig");
const c = @cImport({
    @cDefine("MBEDTLS_CONFIG_FILE", "\"miso_mbedtls_config.h\"");
    @cInclude("board.h");
    @cInclude("mbedtls/base64.h");
    @cInclude("mbedtls/sha256.h");
});

ctx: c.mbedtls_sha256_context,

pub const sha256_error = error{
    start_error,
    update_error,
    finish_error,
};

pub fn init() @This() {
    var self: @This() = undefined;

    self.initCtx();

    return self;
}

inline fn initCtx(self: *@This()) void {
    c.mbedtls_sha256_init(&self.ctx);
}

pub fn start(self: *@This()) !void {
    if (0 != c.mbedtls_sha256_starts(&self.ctx, 0)) return sha256_error.start_error;
}

pub fn update(self: *@This(), buffer: []u8) !void {
    if (0 != c.mbedtls_sha256_update(&self.ctx, buffer.ptr, buffer.len)) return sha256_error.update_error;
}

pub fn finish(self: *@This(), output: *[32]u8) !void {
    if (0 != c.mbedtls_sha256_finish(&self.ctx, output)) return sha256_error.finish_error;
}

pub fn free(self: *@This()) void {
    c.mbedtls_sha256_free(&self.ctx);
}
