const std = @import("std");
const freertos = @import("freertos.zig");
const system = @import("system.zig");

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

    /// DNS error (peer not found)
    dns,
    /// Unavailable socket
    socket,
    /// Connect error
    connect,

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

/// Protocol Bit
const protocol_bit: u32 = (1 << 0);
/// Select TCP or UDP
const udp_tcp_bit: u32 = (1 << 1);
/// Select IP4 or IP6
const ip4_ip6_bit: u32 = (1 << 2);
/// Select if the connection is secure
const secure_bit: u32 = (1 << 3);

/// New protocol Enum
pub const proto = enum(u32) {
    no_protocol = 0,
    udp_ip4 = protocol_bit | udp_tcp_bit,
    tcp_ip4 = protocol_bit,
    udp_ip6 = protocol_bit | ip4_ip6_bit | udp_tcp_bit,
    tcp_ip6 = protocol_bit | ip4_ip6_bit,
    dtls_ip4 = protocol_bit | secure_bit | udp_tcp_bit,
    tls_ip4 = protocol_bit | secure_bit,
    dtls_ip6 = protocol_bit | secure_bit | ip4_ip6_bit | udp_tcp_bit,
    tls_ip6 = protocol_bit | secure_bit | ip4_ip6_bit,

    /// Is the protocol secure
    pub fn isSecure(self: @This()) bool {
        return (@intFromEnum(self) & secure_bit) != 0;
    }
    /// Is the protocol IP6
    pub fn isIp6(self: @This()) bool {
        return (@intFromEnum(self) & ip4_ip6_bit) != 0;
    }
    /// Is the protocol TCP
    pub fn isTcp(self: @This()) bool {
        return (@intFromEnum(self) & udp_tcp_bit) == 0;
    }
    /// Is the protocol UDP
    pub fn isUdp(self: @This()) bool {
        return (@intFromEnum(self) & udp_tcp_bit) != 0;
    }
    /// Is the protocol DTLS
    pub fn isDtls(self: @This()) bool {
        return self.isSecure() and self.isUdp();
    }
    /// Is the protocol TLS
    pub fn isTls(self: @This()) bool {
        return self.isSecure() and self.isTcp();
    }
    pub fn isStream(self: @This()) bool {
        return self.isTcp();
    }
};

pub const EAGAIN: isize = -11;

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
    pub fn getProtocol(self: @This()) proto {
        return switch (self) {
            .ntp, .coap => .udp_ip4,
            .coaps => .dtls_ip4,
            .http, .mqtt => .tcp_ip4,
            .https, .mqtts => .tls_ip4,
            else => .no_protocol,
        };
    }

    /// Test if the scheme is secure
    /// Note: This function is currently not used
    pub fn isSecure(self: @This()) bool {
        return self.getProtocol().isSecure();
    }
};

pub fn Connection(comptime id: connection_id, comptime sslType: type) type {
    _ = id;
    // Compile time checks
    if (sslType != void) {
        if (!@hasDecl(sslType, "init")) {
            @compileError("SSL Type must have an init function");
        }
        if (!@hasDecl(sslType, "deinit")) {
            @compileError("SSL Type must have a deinit function");
        }
    }

    return struct {
        ssl: sslType,

        /// Initialize the connection
        pub fn init(self: *@This()) void {
            _ = self;
            //self.ssl.init();
        }
        pub fn create(self: *@This(), uri: std.Uri, local_port: ?u16) !void {
            try self.ssl.open(uri, local_port);
        }
        pub fn close(self: *@This()) !void {
            try self.ssl.close();
        }
        pub fn send(self: *@This(), buffer: []const u8) !usize {
            return self.ssl.send(buffer);
        }
        pub fn recieve(self: *@This(), buffer: []u8) ![]u8 {
            return self.ssl.recieve(buffer);
        }
        pub fn waitRx(self: *@This(), timeout_s: u32) bool {
            return self.ssl.waitRx(timeout_s);
        }
    };
}
