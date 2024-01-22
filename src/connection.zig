const std = @import("std");
const board = @import("microzig").board;
const freertos = @import("freertos.zig");
const config = @import("config.zig");
const system = @import("system.zig");
//pub const mbedtls = @import("mbedtls.zig");

const c = @cImport({
    @cInclude("network.h");
    @cInclude("wifi_service.h");
    @cInclude("lwm2m_client.h");
});

/// Connection Errors
pub const connection_error = error{
    /// Create Connection Error
    /// Could not create a connection to the host
    create_error,

    /// SSL Init error
    ssl_init_error,

    /// Close Connection Error
    close_error,

    /// Send Error
    send_error,

    /// Recieve Error
    recieve_error,

    /// Possible buffer overflow
    buffer_owerflow,
};

/// Network Context from C
const network_ctx = c.miso_network_ctx_t;

/// Connections are based on a fixed connection pool
///
/// Each connection has an id mapped to the protocol used
///
pub const connection_id = enum(usize) {
    ntp = c.wifi_service_ntp_socket,
    lwm2m = c.wifi_service_lwm2m_socket,
    mqtt = c.wifi_service_mqtt_socket,
    http = c.wifi_service_http_socket,
};

/// Protocol Enumerations
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

    /// Checks if the protocol is secure (TLS/DTLS)
    pub fn isSecure(self: @This()) bool {
        return switch (self) {
            .dtls_ip4, .tls_ip4, .dtls_ip6, .tls_ip6 => true,
            else => false,
        };
    }
};

/// Security Mode Enumerations
///
pub const security_mode = enum(u32) {
    /// No security
    no_sec = c.miso_security_mode_none,
    /// Pre-Shared Key
    psk = c.miso_security_mode_psk,
    /// Certificate EC
    certificate_ec = c.miso_security_mode_ec,
    /// Certificate RSA
    certificate_rsa = c.miso_security_mode_rsa,
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

pub fn Connection(comptime id: connection_id, comptime sslType: type) type {
    return struct {
        ctx: network_ctx,
        proto: protocol,
        ssl: sslType,

        /// Initialize the connection
        pub fn init(self: *@This()) void {
            self.ctx = c.miso_get_network_ctx(@as(c_uint, @intCast(@intFromEnum(id))));
            self.proto = protocol.no_protocol;
        }

        pub fn create(self: *@This(), uri: std.Uri, local_port: ?u16) !void {
            self.proto = schemes.match(uri.scheme).?.getProtocol();

            const host = uri.host.?;
            const port = uri.port.?;

            if (sslType != void) {
                _ = self.ssl.init(self.proto) catch {
                    return connection_error.ssl_init_error;
                };
                _ = c.miso_network_register_ssl_context(@ptrCast(self.ctx), @ptrCast(&self.ssl.context));
            }

            if (0 != c.miso_create_network_connection(self.ctx, @as([*c]const u8, host.ptr), host.len, port, local_port orelse 0, @as(c.enum_miso_protocol, @intFromEnum(self.proto)))) {
                return connection_error.create_error;
            }
        }
        pub fn close(self: *@This()) !void {
            defer {
                if (sslType != void) {
                    _ = self.ssl.deinit();
                }
            }

            if (0 != c.miso_close_network_connection(self.ctx)) {
                return connection_error.close_error;
            }
        }
        pub fn send(self: *@This(), buffer: []const u8) !usize {
            const len: isize = c.miso_network_send(self.ctx, @as([*c]const u8, buffer.ptr), buffer.len);
            return if (len <= 0) connection_error.send_error else @intCast(len);
        }
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
    };
}
