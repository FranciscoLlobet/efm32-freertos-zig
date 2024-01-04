const std = @import("std");
const freertos = @import("freertos.zig");
const c = @cImport({
    @cDefine("MBEDTLS_CONFIG_FILE", "\"miso_mbedtls_config.h\"");
    @cInclude("board.h");
    @cInclude("mbedtls/sha256.h");
    @cInclude("mbedtls/ecp.h");
    @cInclude("mbedtls/ecdsa.h");
    @cInclude("mbedtls/pem.h");
    @cInclude("mbedtls/x509_crt.h");
});

ctx: c.mbedtls_pk_context,

pub const pk_error = error{
    parse_key_error,
    verify_error,
};

pub fn init() @This() {
    var self: @This() = undefined;

    self.initCtx();

    return self;
}

inline fn initCtx(self: *@This()) void {
    c.mbedtls_pk_init(&self.ctx);
}

pub fn parse(self: *@This(), in: []const u8) !void {
    if (0 != c.mbedtls_pk_parse_public_key(&self.ctx, in.ptr, in.len)) return pk_error.parse_key_error;
}

pub fn verify(self: *@This(), hash: []u8, sig: []const u8) !void {
    if (0 != c.mbedtls_pk_verify(&self.ctx, c.MBEDTLS_MD_SHA256, hash.ptr, hash.len, sig.ptr, sig.len)) return pk_error.verify_error;
}

pub fn free(self: *@This()) void {
    c.mbedtls_pk_free(&self.ctx);
}
