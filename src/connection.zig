const std = @import("std");
const board = @import("microzig").board;
const freertos = @import("freertos.zig");
const config = @import("config.zig");
const system = @import("system.zig");
pub const mbedtls = @import("mbedtls.zig");

const c = @cImport({
    @cInclude("network.h");
    @cInclude("wifi_service.h");
    @cInclude("lwm2m_client.h");
});

pub const connection_error = error{
    /// Create Connection Error
    /// Could not create a connection to the host
    create_error,

    /// SSL Init error
    ssl_init_error,

    close_error,

    send_error,

    recieve_error,

    buffer_owerflow,
};

const network_ctx = c.miso_network_ctx_t;

/// Connections are based on a fixed connection pool
///
/// Each connection has an id mapped to the protocol used
pub const connection_id = enum(usize) {
    ntp = c.wifi_service_ntp_socket,
    lwm2m = c.wifi_service_lwm2m_socket,
    mqtt = c.wifi_service_mqtt_socket,
    http = c.wifi_service_http_socket,
};

pub const protocol = enum(u32) {
    no_protocol = c.miso_protocol_no_protocol,

    udp_ip4 = c.miso_protocol_udp_ip4,
    tcp_ip4 = c.miso_protocol_tcp_ip4,

    udp_ip6 = c.miso_protocol_udp_ip6,
    tcp_ip6 = c.miso_protocol_tcp_ip6,

    dtls_ip4 = c.miso_protocol_dtls_ip4,
    tls_ip4 = c.miso_protocol_tls_ip4,

    dtls_ip6 = c.miso_protocol_dtls_ip6,
    tls_ip6 = c.miso_protocol_tls_ip6,

    pub fn isSecure(self: @This()) bool {
        return switch (self) {
            .dtls_ip4, .tls_ip4, .dtls_ip6, .tls_ip6 => true,
            else => false,
        };
    }
};

pub const security_mode = enum(u32) {
    no_sec = 0,
    psk = 1,
    certificate_ec,
    certificate_rsa,
};

pub const schemes = enum(u32) {
    no_scheme = 0,
    // Unsecured schemes
    ntp = 1,
    http = 2,
    mqtt = 3,
    coap = 4,
    // Secured schemes
    https = 2 + 16,
    mqtts = 3 + 16,
    coaps = 4 + 16,

    const stringmap = std.ComptimeStringMap(@This(), .{ .{ "ntp", .ntp }, .{ "http", .http }, .{ "https", .https }, .{ "mqtt", .mqtt }, .{ "mqtts", .mqtts }, .{ "coap", .coap }, .{ "coaps", .coaps } });

    pub fn match(scheme: []const u8) ?@This() {
        return stringmap.get(scheme);
    }

    /// Get the underlying protocol for the proposed scheme
    pub fn getProtocol(self: @This()) protocol {
        return switch (self) {
            .ntp, .coap => protocol.udp_ip4,
            .coaps => protocol.dtls_ip4,
            .http, .mqtt => protocol.tcp_ip4,
            .https, .mqtts => protocol.tls_ip4,
            else => protocol.no_protocol,
        };
    }

    /// Test if the scheme is secure
    /// Note: This function is currently not used
    pub fn isSecure(self: @This()) bool {
        return self.getProtocol().isSecure();
    }
};

ctx: network_ctx,
proto: protocol,
id: connection_id,
ssl: mbedtls,

pub fn init(id: connection_id, credential_callback: ?mbedtls.credential_callback_fn, custom_ssl_init: ?mbedtls.custom_init_callback_fn) @This() {
    // Think about how to rewrite this for non-ssl connections
    return @This(){ .id = id, .ctx = c.miso_get_network_ctx(@as(c_uint, @intCast(@intFromEnum(id)))), .proto = undefined, .ssl = mbedtls.create(credential_callback, custom_ssl_init) };
}

pub fn connect(self: *@This(), uri: std.Uri, local_port: ?u16, proto: ?protocol, mode: ?security_mode) !void {
    _ = self;
    _ = uri;
    _ = local_port;
    _ = proto;
    _ = mode;
}

/// Create a connection to a host
///
pub fn create(self: *@This(), host: []const u8, port: u16, local_port: ?u16, proto: protocol, mode: ?security_mode) !void {
    self.proto = proto;

    if (self.proto.isSecure()) {
        _ = self.ssl.init(self, self.proto, mode.?) catch {
            return connection_error.ssl_init_error;
        };
    }

    if (0 != c.miso_create_network_connection(self.ctx, @as([*c]const u8, host.ptr), host.len, port, local_port orelse 0, @as(c.enum_miso_protocol, @intFromEnum(self.proto)))) {
        return connection_error.create_error;
    }
}

pub fn close(self: *@This()) !void {
    defer {
        if (self.proto.isSecure()) {
            _ = self.ssl.deinit();
        }
    }

    if (0 != c.miso_close_network_connection(self.ctx)) {
        return connection_error.close_error;
    }
}

/// Send Function
pub fn send(self: *@This(), buffer: []const u8) !usize {
    const len: isize = c.miso_network_send(self.ctx, @as([*c]const u8, buffer.ptr), buffer.len);
    return if (len <= 0) connection_error.send_error else @intCast(len);
}
/// Recieve Function
pub fn recieve(self: *@This(), buffer: []u8) ![]u8 {
    const len: isize = c.miso_network_read(self.ctx, buffer.ptr, buffer.len);
    return if (len <= 0)
        connection_error.recieve_error
    else if (@as(usize, @intCast(len)) > buffer.len)
        connection_error.buffer_owerflow
    else
        buffer[0..@intCast(len)];
}

pub fn waitRx(self: *@This(), timeout_s: u32) i32 {
    return c.wait_rx(self.ctx, timeout_s);
}
pub fn waitTx(self: *@This(), timeout_s: u32) i32 {
    return c.wait_tx(self.ctx, timeout_s);
}
