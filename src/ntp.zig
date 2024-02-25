//! Simple Network Time Protocol (SNTP) Version 4 for IPv4, IPv6 and OSI
//! References:
//!
//! - <https://datatracker.ietf.org/doc/html/rfc4330>
//! - <https://datatracker.ietf.org/doc/html/rfc5905>
//!
const std = @import("std");
const connection = @import("connection.zig");
const system = @import("system.zig");
const simpleConnection = @import("simpleConnection.zig");

const sntp_error = error{
    invalid_server_version,
    invalid_packet,
    invalid_origin_timestamp,
    kiss_of_death,
    // connectivity issues
    send_error,
};

const li = enum(u8) {
    no_warning = (0 << 6),
    last_minute_has_61_seconds = (1 << 6),
    last_minute_has_59_seconds = (2 << 6),
    alarm_condition = (3 << 6),
};

const vn = enum(u8) {
    version_0 = (0 << 3),
    version_1 = (1 << 3),
    version_2 = (2 << 3),
    version_3 = (3 << 3),
    version_4 = (4 << 3),
};

const mode = enum(u8) {
    reserved = 0,
    symmetric_active = 1,
    symmetric_passive = 2,
    client = 3,
    server = 4,
    broadcast = 5,
    reserved_for_private_use = 6,
    reserved_for_ntp_control_message = 7,
};

const client_request = enum(u8) {
    request_v3 = @intFromEnum(li.no_warning) | @intFromEnum(vn.version_3) | @intFromEnum(mode.client),
    request_v4 = @intFromEnum(li.no_warning) | @intFromEnum(vn.version_4) | @intFromEnum(mode.client),
};

const server_response = enum(u8) {
    response_v3 = @intFromEnum(vn.version_3) | @intFromEnum(mode.server),
    response_v4 = @intFromEnum(vn.version_4) | @intFromEnum(mode.server),
};

const stratum = enum(u8) {
    kod = 0,
    primary_reference = 1,
    secondary_reference = 2,
    secondary_reference3 = 3,
    secondary_reference4 = 4,
    secondary_reference5 = 5,
    secondary_reference6 = 6,
    secondary_reference7 = 7,
    secondary_reference8 = 8,
    secondary_reference9 = 9,
    secondary_reference10 = 10,
    secondary_reference11 = 11,
    secondary_reference12 = 12,
    secondary_reference13 = 13,
    secondary_reference14 = 14,
    secondary_reference15 = 15,
    reserved = 16,
};

const poll_interval = enum(u8) {
    interval_4s,
    interval_8s = 3,
    /// Minimum supported poll interval for compatibility with most servers
    interval_16s = 4,
    interval_32s = 5,
    interval_64s = 6,
    interval_128s = 7,
    interval_256s = 8,
    interval_512s = 9,
    interval_1024s = 10,
    interval_2048s = 11,
    interval_4096s = 12,
    interval_8192s = 13,
    interval_16384s = 14,
    interval_32768s = 15,
    interval_65536s = 16,
    interval_131072s = 17,
};

const server_response_mask: u8 = (7 << 3) | (7 << 0);

pub const ntp_response = struct { timestamp_s: u32, timestamp_frac: u32, poll_interval: u32 };

/// Non public sntp v4 packet
const sntp_v4_packet = packed struct {
    leap_version_mode: u8,
    stratum: u8,
    poll_interval: u8,
    precision: u8,

    root_delay: u32,
    root_dispersion: u32,

    reference_identifier: u32,

    reference_timestamp_seconds: u32,
    reference_timestamp_fraction: u32,

    originate_timestamp_seconds: u32,
    originate_timestamp_fraction: u32,

    receive_timestamp_seconds: u32,
    recieve_timestamp_fraction: u32,

    transmit_timestamp_seconds: u32,
    transmit_timestamp_fraction: u32,

    pub fn createRequest(self: *@This(), originate_timestamp_s: u32, originate_timestamp_frac: u32) !void {
        // Zero the packet using memory magic
        @memset(self.slice(), 0);

        self.leap_version_mode = @intFromEnum(client_request.request_v4);
        self.transmit_timestamp_seconds = @byteSwap(originate_timestamp_s);
        self.transmit_timestamp_fraction = @byteSwap(originate_timestamp_frac);
    }

    /// Convert the packet to a slice of bytes.
    pub inline fn slice(self: *@This()) []u8 {
        return @as([*]u8, @ptrCast(self))[0..@sizeOf(@This())];
    }

    pub inline fn len() usize {
        return @sizeOf(@This());
    }

    /// This function is used to process a v4 server response packet.
    pub fn process_server_packet(self: *@This(), origin_timestamp_s: u32, origin_timestamp_frac: u32) sntp_error!ntp_response {

        // Check for the server response header
        if ((self.leap_version_mode & server_response_mask) != @intFromEnum(server_response.response_v4)) {
            return sntp_error.invalid_server_version;
        }
        // Check for the KoD indicator
        if (self.stratum == @intFromEnum(stratum.kod)) {
            return sntp_error.kiss_of_death;
        }
        // Check for zero transmit timestamp
        if ((self.transmit_timestamp_seconds == 0) and (self.transmit_timestamp_fraction == 0)) {
            return sntp_error.invalid_packet;
        }

        const server_timestamp_s = @byteSwap(self.receive_timestamp_seconds);
        const server_timestamp_frac = @byteSwap(self.recieve_timestamp_fraction);
        const server_origin_timestamp_s = @byteSwap(self.originate_timestamp_seconds);
        const server_origin_timestamp_frac = @byteSwap(self.originate_timestamp_fraction);
        const server_poll_interval = self.poll_interval;

        if ((server_origin_timestamp_s != origin_timestamp_s) or (server_origin_timestamp_frac != origin_timestamp_frac)) {
            return sntp_error.invalid_origin_timestamp;
        }

        // convert the poll interval response and conver it to a seconds value
        const next_poll_interval: u32 = @shlExact(@as(u32, 1), @as(u5, if (server_poll_interval < @intFromEnum(poll_interval.interval_16s))
            @intFromEnum(poll_interval.interval_16s)
        else if (server_poll_interval > @intFromEnum(poll_interval.interval_131072s))
            @intFromEnum(poll_interval.interval_131072s)
        else
            @intCast(server_poll_interval)));

        return .{ .timestamp_s = server_timestamp_s, .timestamp_frac = server_timestamp_frac, .poll_interval = next_poll_interval };
    }
};

var conn: connection.Connection(simpleConnection.SimpleLinkConnection(.udp_ip4)) = undefined;

/// Send an sNTP packet to the given connection.
fn send(c: *@TypeOf(conn), packet: *sntp_v4_packet) !void {
    if (c.send(packet.slice())) |len| {
        if (len != sntp_v4_packet.len()) return sntp_error.send_error;
    } else |_| {
        return sntp_error.send_error;
    }
}

/// Get the current time from an sNTP server using the given URI.
///
pub fn getTimeFromServer(uri: std.Uri) !ntp_response {
    //conn.init();

    var packet: sntp_v4_packet = undefined;

    const originate_timestamp_s: u32 = system.time.getNtpTime();
    const originate_timestamp_frac: u32 = 0;

    try conn.open(uri, 123);
    defer {
        conn.close() catch {};
    }

    try packet.createRequest(originate_timestamp_s, originate_timestamp_frac);

    try send(&conn, &packet);

    _ = try conn.waitRx(2);

    _ = try conn.recieve(packet.slice());

    var server_time = try packet.process_server_packet(originate_timestamp_s, originate_timestamp_frac);

    try system.time.setTimeFromNtp(server_time.timestamp_s);

    return server_time;
}
