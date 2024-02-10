//! Simple Connection
//!
const std = @import("std");
const board = @import("microzig").board;
const freertos = @import("freertos.zig");
const c = @cImport({
    @cInclude("simplelink.h");
});

const connection = @import("connection.zig");

const conn_error = connection.connection_error;
// SimpleLink Wrapper

const SL_INVALID_SOCKET: i16 = -1;
const SL_OK: i16 = 0;
const SL_EAGAIN: i16 = c.SL_EAGAIN;
const SL_EALREADY: i16 = c.SL_EALREADY;

const SL_SOCK_DGRAM: i16 = c.SL_SOCK_DGRAM;
const SL_SOCK_STREAM: i16 = c.SL_SOCK_STREAM;

const SL_IPROTO_UDP: i16 = c.SL_IPPROTO_UDP;
const SL_IPROTO_TCP: i16 = c.SL_IPPROTO_TCP;

const SL_AF_INET: i16 = c.SL_AF_INET;

const SL_SEC_SOCKET: i16 = c.SL_SEC_SOCKET;
const SL_MAX_SOCKETS: i16 = @intCast(c.SL_MAX_SOCKETS);

const RET_VAL_OK: i32 = 0;
/// Address Family
const sl_fam = enum(i16) {
    inet = SL_AF_INET,
    inet6 = c.SL_AF_INET6,
};

/// Socket Type
const sl_type = enum(i16) {
    stream = SL_SOCK_STREAM,
    dgram = SL_SOCK_DGRAM,
};

/// Protocol
const sl_proto = enum(i16) {
    udp = SL_IPROTO_UDP,
    tcp = SL_IPROTO_TCP,
};

const sd_e = enum(i16) {
    invalid = SL_INVALID_SOCKET,
};

const ipv4Peer = struct {
    addr: c.SlSockAddrIn_t,
    len: usize,

    pub fn create(addr: u32, port: u16) @This() {
        return @This(){
            .addr = c.SlSockAddrIn_t{
                .sin_family = SL_AF_INET,
                .sin_port = @byteSwap(port),
                .sin_addr = .{
                    .s_addr = @byteSwap(addr),
                },
                .sin_zero = undefined,
            },
            .len = @sizeOf(c.SlSockAddrIn_t),
        };
    }
    pub inline fn getLen(self: *@This()) usize {
        return @sizeOf(@TypeOf(self.addr));
    }
    pub inline fn getAddrPtr(self: *@This()) *c.SlSockAddrIn_t {
        return @ptrCast(&self.addr);
    }
};

/// Experimental.
/// Do not use this for production code.
const ipv6Peer = struct {
    addr: c.SlSockAddrIn6_t,
    len: usize,

    pub fn create(addr: u128, port: u16) @This() {
        _ = addr;
        return @This(){
            .addr = c.SlSockAddrIn6_t{
                .sin6_family = c.SL_AF_INET6,
                .sin6_port = @byteSwap(port),
                .sin6_addr = undefined,
            },
            .len = @sizeOf(c.SlSockAddrIn6_t),
        };
    }
    pub fn getLen(self: *@This()) usize {
        return @sizeOf(self.addr);
    }
};

const peer = struct {
    addr: c.sl_SockAddr_t,
    len: usize,
};

/// Map the generic connection protocol to the SimpleLink protocol settings
fn mapConnProtoToSl(proto: connection.proto) struct { fam: sl_fam, sl_type: sl_type, proto: sl_proto } {
    return .{
        .fam = if (proto.isIp6()) sl_fam.inet6 else sl_fam.inet,
        .sl_type = if (proto.isStream()) sl_type.stream else sl_type.dgram,
        .proto = if (proto.isUdp()) sl_proto.udp else sl_proto.tcp,
    };
}

pub fn SimpleLinkConnection(comptime proto: connection.proto) type {
    const settings = mapConnProtoToSl(proto);

    return struct {
        /// Simple Link Socket Identifier
        sd: i16,
        /// Peer Address
        peer: ipv4Peer,
        /// Local Address
        local: ipv4Peer,

        pub fn create() @This() {
            return .{
                .sd = @intFromEnum(sd_e.invalid),
                .peer = undefined,
                .local = undefined,
            };
        }

        /// Open a connection to designated peer
        pub fn open(self: *@This(), uri: std.Uri, local_port: ?u16) !void {
            const host = uri.host.?;
            const port = uri.port.?; // If port is not provided, hang!

            // Resolve the ip address using DNS
            var host_ip: u32 = 0;
            const dns = c.sl_NetAppDnsGetHostByName(@as([*c]i8, @ptrCast(@constCast(host.ptr))), @intCast(host.len), &host_ip, @intCast(@intFromEnum(settings.fam)));
            if (dns >= 0) {
                self.peer = ipv4Peer.create(host_ip, port);
            } else {
                return conn_error.dns;
            }

            // Get Socket
            try self.socket();
            errdefer {
                self.close() catch {};
            }

            // Set Non-Blocking
            const enableOption: c.SlSockNonblocking_t = .{ .NonblockingEnabled = 1 };
            _ = c.sl_SetSockOpt(self.sd, c.SL_SOL_SOCKET, c.SL_SO_NONBLOCKING, &enableOption, @sizeOf(@TypeOf(enableOption)));

            // Remove RX Aggregation
            const RxAggrEnable: u8 = 0;
            _ = c.sl_NetCfgSet(c.SL_SET_HOST_RX_AGGR, 0, @sizeOf(@TypeOf(RxAggrEnable)), &RxAggrEnable);

            // Setup the local address
            self.local = ipv4Peer.create(0, local_port orelse 0);

            // Bind to local port
            if (local_port) |_| {
                _ = c.sl_Bind(self.sd, @ptrCast(self.local.getAddrPtr()), @intCast(self.local.getLen()));
            }

            try self.connect();

            // Register with the connection manager
        }

        /// Connect
        fn connect(self: *@This()) !void {
            var ret: i16 = SL_EALREADY;

            while (ret == SL_EALREADY) {
                ret = c.sl_Connect(self.sd, @ptrCast(self.peer.getAddrPtr()), @intCast(self.peer.getLen()));
                if (ret == SL_EALREADY) {
                    continue;
                } else if (ret < 0) {
                    return conn_error.connect;
                }
            }
        }

        /// Open a socket
        fn socket(self: *@This()) !void {
            self.sd = c.sl_Socket(@intFromEnum(settings.fam), @intFromEnum(settings.sl_type), @intFromEnum(settings.proto));
            if (self.sd < 0) {
                return conn_error.socket;
            }
        }

        /// Send
        pub fn send(self: *@This(), data: []u8) !usize {
            var ret: isize = undefined;
            if (comptime proto.isTcp()) {
                ret = c.sl_Send(self.sd, @ptrCast(data.ptr), @intCast(data.len), 0);
            } else {
                ret = c.sl_SendTo(self.sd, @ptrCast(data.ptr), @intCast(data.len), 0, @ptrCast(self.peer.getAddrPtr()), @intCast(self.peer.getLen()));
            }

            return if (ret <= 0) conn_error.send_error else @intCast(ret);
        }

        /// Basic C send
        pub fn send_c(self: *@This(), data: []const u8) isize {
            var len: c_int = undefined;
            if (comptime proto.isTcp()) {
                len = c.sl_Send(self.sd, @ptrCast(data.ptr), @intCast(data.len), 0);
            } else {
                len = c.sl_SendTo(self.sd, @ptrCast(data.ptr), @intCast(data.len), 0, @ptrCast(self.peer.getAddrPtr()), @intCast(self.peer.getLen()));
            }
            return if (len == SL_EAGAIN) connection.EAGAIN else len;
        }

        /// Basic C Recv
        pub fn recieve(self: *@This(), data: []u8) ![]u8 {
            var len: isize = 0;
            if (comptime proto.isTcp()) {
                len = c.sl_Recv(self.sd, @ptrCast(data.ptr), @intCast(data.len), 0);
            } else {
                var peer_len: c_ushort = @intCast(self.peer.len);
                len = c.sl_RecvFrom(self.sd, @ptrCast(data.ptr), @intCast(data.len), 0, @ptrCast(&self.peer), &peer_len);
                self.peer.len = @intCast(peer_len);
            }
            return if (len <= 0)
                conn_error.recieve_error
            else if (@as(usize, @intCast(len)) > data.len)
                conn_error.buffer_owerflow
            else
                data[0..@intCast(len)];
        }

        pub fn recieve_c(self: *@This(), data: []u8) isize {
            var len: isize = undefined;
            if (comptime proto.isTcp()) {
                len = c.sl_Recv(self.sd, @ptrCast(data.ptr), @intCast(data.len), 0);
            } else {
                var peer_len: c_ushort = @intCast(self.peer.len);
                len = c.sl_RecvFrom(self.sd, @ptrCast(data.ptr), @intCast(data.len), 0, @ptrCast(&self.peer), &peer_len);
                self.peer.len = @intCast(peer_len);
            }
            return if (len == SL_EAGAIN) connection.EAGAIN else len;
        }

        /// Close the connection
        pub fn close(self: *@This()) !void {
            defer {
                self.sd = @intFromEnum(sd_e.invalid);
            }
            if (0 != c.sl_Close(self.sd)) {
                return conn_error.close_error;
            }
        }
        /// Wait for data to be available
        pub fn waitRx(self: *@This(), timeout_s: u32) bool {
            _ = self;
            _ = timeout_s;

            return true;
        }
        pub fn getCtx(self: *@This()) *anyopaque {
            return @ptrCast(@alignCast(self));
        }
        pub fn getProto(self: *@This()) connection.proto {
            _ = self;
            return proto;
        }
    };
}
