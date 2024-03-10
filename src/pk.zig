// Copyright (c) 2023-2024 Francisco Llobet-Blandino and the "Miso Project".
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the “Software”), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
// WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

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
