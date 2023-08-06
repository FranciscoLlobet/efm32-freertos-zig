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

const network_ctx = c.miso_network_ctx_t;

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
};

pub const security_mode = enum(u32) {
    no_sec,
    psk,
    certificate_ec,
    certificate_rsa,
};

ctx: network_ctx,
id: connection_id,
ssl: mbedtls,

pub fn init(id: connection_id, auth_callback: ?mbedtls.auth_callback_fn) @This() {
    return @This(){ .id = id, .ctx = c.miso_get_network_ctx(@as(c_uint, @intCast(@intFromEnum(id)))), .ssl = mbedtls.create(auth_callback) };
}

pub fn create(self: *@This(), host: []const u8, port: []const u8, local_port: ?[]const u8, proto: protocol, mode: ?security_mode) i32 {
    var c_local_port: [*c]const u8 = undefined;
    var ret: i32 = 0;
    if (local_port) |l_port| {
        c_local_port = @as([*c]const u8, @ptrCast(l_port));
    }

    if (mode) |s_mode| {
        ret = self.ssl.init(proto, s_mode);
        if (ret == 0) {
            ret = c.miso_network_register_ssl_context(self.ctx, @as(*c.mbedtls_ssl_context, @ptrCast(&self.ssl.context)));
        }
    }

    if (ret == 0) {
        ret = @as(i32, c.miso_create_network_connection(self.ctx, @as([*c]const u8, @ptrCast(host)), @as([*c]const u8, @ptrCast(port)), c_local_port, @as(c.enum_miso_protocol, @intFromEnum(proto))));
    }

    return ret;
}

pub fn close(self: *@This()) i32 {
    return c.miso_close_network_connection(self.ctx);
}
pub fn send(self: *@This(), buffer: *const u8, length: usize) i32 {
    return c.miso_network_send(self.ctx, @as([*c]const u8, @ptrCast(buffer)), length);
}
pub fn recieve(self: *@This(), buffer: *u8, length: usize) i32 {
    return c.miso_network_read(self.ctx, buffer, length);
}
pub fn waitRx(self: *@This(), timeout_s: u32) i32 {
    return c.wait_rx(self.ctx, timeout_s);
}
pub fn waitTx(self: *@This(), timeout_s: u32) i32 {
    return c.wait_tx(self.ctx, timeout_s);
}
