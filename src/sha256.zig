const std = @import("std");
const freertos = @import("freertos.zig");
const c = @cImport({
    @cDefine("MBEDTLS_CONFIG_FILE", "\"miso_mbedtls_config.h\"");
    @cInclude("board.h");
    @cInclude("mbedtls/base64.h");
    @cInclude("mbedtls/sha256.h");
});

ctx: c.mbedtls_sha256_context,

pub fn init() @This() {
    var self: @This() = undefined;

    self.initCtx();

    return self;
}

pub fn initCtx(self: *@This()) void {
    c.mbedtls_sha256_init(&self.ctx);
}

pub fn start(self: *@This()) bool {
    return (0 == c.mbedtls_sha256_starts(&self.ctx, 0));
}

pub fn update(self: *@This(), buffer: []u8, len: usize) bool {
    return (0 == c.mbedtls_sha256_update(&self.ctx, @ptrCast(buffer), len));
}

pub fn finish(self: *@This(), output: *[32]u8) bool {
    return (0 == c.mbedtls_sha256_finish(&self.ctx, output));
}

pub fn free(self: *@This()) void {
    c.mbedtls_sha256_free(&self.ctx);
}
