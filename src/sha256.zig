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

pub inline fn start(self: *@This()) !void {
    if (0 != c.mbedtls_sha256_starts(&self.ctx, 0)) return sha256_error.start_error;
}

pub inline fn update(self: *@This(), buffer: []u8) !void {
    if (0 != c.mbedtls_sha256_update(&self.ctx, buffer.ptr, buffer.len)) return sha256_error.update_error;
}

pub inline fn finish(self: *@This(), output: *[32]u8) !void {
    if (0 != c.mbedtls_sha256_finish(&self.ctx, output)) return sha256_error.finish_error;
}

pub inline fn free(self: *@This()) void {
    c.mbedtls_sha256_free(&self.ctx);
}
