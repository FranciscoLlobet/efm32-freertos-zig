const std = @import("std");
const freertos = @import("freertos.zig");
const config = @import("config.zig");
const system = @import("system.zig");
const connection = @import("connection.zig");
const file = @import("fatfs.zig").file;
const led = @import("leds.zig");

const c = @cImport({
    @cInclude("board.h");
    @cInclude("picohttpparser.h");
});

/// Connection instance
connection: connection,

/// Array to store parsed header information
headers: [24]c.phr_header,

/// TX Buffer mutex
tx_mutex: freertos.StaticMutex(),

/// Rx Buffer mutex
rx_mutex: freertos.StaticMutex(),

/// TX Buffer
tx_buffer: [256]u8 align(@alignOf(u32)),

/// RX Buffer
rx_buffer: [1536]u8 align(@alignOf(u32)),

// Etag
etag: [64]u8,

file: file,

const @"error" = error{
    rx_error,
    tx_error,

    /// PHR parse failed
    phr_library_parse_failed,

    parse_error,

    timeout,
    connection_error,

    /// Status Code Not OK (200)
    status_code_nok,

    /// Downloaded file size does not match the expected size
    file_size_mismatch,

    /// Advertized file size exceeds the maximum allowed size
    file_size_exceeded,

    /// Unexpected Status Code
    unexpected_status_code,

    range_response_parse_error,
};

/// HTTP Response from server
const rx_response = struct { payload: ?[]u8, headers: []c.phr_header, status: u32 };

/// Authentication callback function.
/// Used during the connection phase for providing PSK credentials
fn authCallback(self: *connection.mbedtls, security_mode: connection.security_mode) connection.mbedtls.auth_error!void {
    _ = self;
    if (security_mode == .psk) {
        // PSK authentication
    } else if (security_mode == .certificate_ec) {
        // EC Certificate
    } else {
        // Unsuported security mode
    }
}

/// Structure representing a parsed Content-Range response header.
const rangeResponse = struct {
    /// Start position of the range
    start: usize,

    /// End position of the range
    end: usize,

    /// Optional total size
    total: ?usize,

    /// Function to parse the Content-Range header
    ///
    /// - 206 partial-content: This function is currently parsing correctly a 206 response. Format: {start}-{end}/{total}
    /// - 416 Requested Range Not Satisfieable. Here the format is */{total} where {total} is the total size.
    ///
    /// This function will parse and assume the happy path (206 - Partial Content).
    fn match(header: c.phr_header) !?@This() {
        // Pre-fill the returned structure with default values
        var ret: @This() = .{ .start = 0, .end = 0, .total = null };

        // Check if the header contains the expected "bytes " string
        if (!std.mem.eql(u8, header.value[0.."bytes ".len], "bytes ")) {
            return @"error".range_response_parse_error; // Does not contain the expected "bytes " string
        } else if (std.mem.eql(u8, header.value[0.."bytes *".len], "bytes *")) {
            // Requested Range Not Satisfieable
            return null;
        } else {
            var iter = std.mem.splitAny(u8, header.value["bytes ".len..header.value_len], "-/");

            // Get the start position
            ret.start = try std.fmt.parseInt(usize, iter.first(), 10);
            if (iter.next()) |val| {
                ret.end = try std.fmt.parseInt(usize, val, 10);
            }
            if (iter.next()) |val| {
                ret.total = try std.fmt.parseInt(usize, val, 10);
            }
        }

        return ret;
    }
};

/// Structure representing a parsed HTTP response.
const parsedResponse = struct {
    /// Response status code
    status_code: u32,

    /// Content-type
    content_type: ?contentType,

    /// Content-length
    content_length: ?usize,

    /// Optional Content-Range
    range: ?rangeResponse,

    /// Optional Accept-Ranges
    accept_ranges: ?acceptRanges,

    /// Optional Keep-Alive response
    keep_alive: ?keepAlive,

    /// Optional Payload slice
    payload: ?[]const u8,

    /// Optional etag
    etag: ?[]const u8,

    /// Function to process the HTTP response headers.
    /// The function will parse the headers from rx response and store the values in the structure.
    /// The function will return the HTTP status code.
    fn processHeaders(self: *@This(), rx: rx_response) !u32 {

        // Set default values
        self.* = .{ .payload = rx.payload, .status_code = rx.status, .content_type = null, .content_length = null, .range = null, .accept_ranges = null, .etag = null, .keep_alive = null };

        // Process specific headers based on their type.
        for (rx.headers) |header| {
            if (responseHeaders.match(header)) |val| {
                switch (val) {
                    .contentRange => {
                        // Parse and store the Content-Range header details.
                        self.range = try rangeResponse.match(header);
                    },
                    .acceptRanges => {
                        // Parse and store the Accept-Ranges header value.
                        self.accept_ranges = try acceptRanges.match(header);
                    },
                    .contentLength => {
                        // Parse and store the Content-Length header value.
                        self.content_length = try std.fmt.parseInt(usize, header.value[0..header.value_len], 10);
                    },
                    .etag => {
                        // Convert ETag information into slice
                        self.etag = header.value[0..header.value_len];
                    },
                    .connection => {
                        // Determine if the connection should be kept alive or closed.
                        self.keep_alive = try keepAlive.match(header);
                    },
                    else => {
                        // ... other headers can be added and processed as needed ...
                    },
                }
            }
        }
        return self.status_code;
    }

    pub fn getEtag(self: *@This()) ?[]const u8 {
        return self.etag;
    }
};

/// Function to send an HTTP GET request to a specified URL.
pub fn sendGetRequest(self: *@This(), url: []const u8) !void {
    var uri = try std.Uri.parse(url);

    if (self.tx_mutex.take(null)) {
        const request = try std.fmt.bufPrint(&self.tx_buffer, "GET {s} HTTP/1.1\r\nHost: {s}\r\n\r\n", .{ uri.path, uri.host.? });
        try self.connection.send(@ptrCast(request), request.len);
        _ = self.tx_mutex.give();
    }
}

/// Function to send an HTTP GET request with a specific byte range.
/// The range is specified by the 'start' and 'end' parameters.
pub fn sendGetRangeRequest(self: *@This(), url: []const u8, start: usize, end: usize) !void {
    var uri = try std.Uri.parse(url);

    if (try self.tx_mutex.take(null)) {
        defer {
            self.tx_mutex.give() catch {};
        }
        const request = try std.fmt.bufPrint(&self.tx_buffer, "GET {s} HTTP/1.1\r\nHost: {s}\r\nRange: bytes={d}-{d}\r\n\r\n", .{ uri.path, uri.host.?, start, end });
        _ = try self.connection.send(request);
    }
}

/// Function to send an HTTP HEAD request to a specified URL.
/// HEAD requests retrieve the headers without the message body.
pub fn sendHeadRequest(self: *@This(), url: []const u8) !void {
    var uri = try std.Uri.parse(url);

    if (try self.tx_mutex.take(null)) {
        defer {
            self.tx_mutex.give() catch {};
        }
        const request = try std.fmt.bufPrint(&self.tx_buffer, "HEAD {s} HTTP/1.1\r\nHost: {s}\r\n\r\n", .{ uri.path, uri.host.? });
        _ = try self.connection.send(request);
    }
}

/// Recieve an HTTP response from the server.
fn recieveResponse(self: *@This()) !rx_response {
    var rx_count: usize = 0;
    var pret: i32 = -2; // Incomplete request
    var prevbuflen: usize = 0;

    var minor_version: i32 = undefined;
    var status: i32 = undefined;
    var msg: [*c]u8 = undefined;
    var msg_len: usize = undefined;
    var num_headers: usize = self.headers.len;
    var payload_len: usize = undefined;
    var payload: ?[]u8 = null;
    if (try self.rx_mutex.take(null)) {
        defer {
            self.rx_mutex.give() catch {};
        }
        while ((pret == -2) and (rx_count < self.rx_buffer.len)) {
            if (0 == self.connection.waitRx(5)) {
                var rec = try self.connection.recieve(self.rx_buffer[rx_count..(self.rx_buffer.len)]);

                // returns number of bytes consumed if successful, -2 if request is partial, -1 if failed
                pret = c.phr_parse_response(rec.ptr, rec.len, &minor_version, &status, &msg, &msg_len, &self.headers, &num_headers, prevbuflen);
                prevbuflen = rx_count;
                rx_count += rec.len;
            } else {
                // rx Timeout
            }
        }

        if (pret > 0) {
            payload_len = rx_count - @as(usize, @intCast(pret));
            payload = if (payload_len != 0) self.rx_buffer[(rx_count - payload_len)..rx_count] else null;
        } else if (pret == -1) {
            // PHR parse response failed for some reason
            return @"error".phr_library_parse_failed;
        } else {
            return @"error".rx_error;
        }
    } else {
        return @"error".timeout; // Could not take the mutex
    }

    return .{ .payload = payload, .headers = self.headers[0..num_headers], .status = @intCast(status) };
}

/// Calculate the end position of the range request
fn calcRequestEnd(file_size: usize, comptime block_size: usize, current_position: usize) usize {
    var requestEnd = current_position + (block_size - 1);
    return if (requestEnd > (file_size - 1)) (file_size - 1) else requestEnd;
}

/// File Download using HTTP
pub fn filedownload(self: *@This(), url: []const u8, file_name: [*:0]const u8, comptime block_size: usize, comptime max_file_size: usize) !void {
    var parsed_response: parsedResponse = undefined;

    // Parse the URI
    var uri = try std.Uri.parse(url);
    errdefer {
        self.file.close() catch {};
        self.connection.close() catch {};
    }

    // Change this to support TLS on HTTP
    const proto = connection.schemes.match(uri.scheme).?.getProtocol();

    try self.connection.create(uri, null, if (proto.isSecure()) .psk else null);
    defer {
        self.connection.close() catch {};
    }

    try self.sendHeadRequest(url);

    if (200 != try parsed_response.processHeaders(try self.recieveResponse())) {
        return @"error".status_code_nok;
    }

    const fileSize: usize = parsed_response.content_length.?;
    if (fileSize > max_file_size) {
        return @"error".file_size_exceeded;
    }

    if (parsed_response.getEtag()) |etag| {
        @memcpy(self.etag[0..].ptr, etag);
    }

    // Open the file for writing
    self.file = try file.open(file_name, @intFromEnum(file.fMode.create_always) | @intFromEnum(file.fMode.write));
    defer {
        self.file.close() catch {};
    }

    try self.file.sync(); // Perfom sync to reduce chances of critical errors

    while (self.file.tell() < fileSize) {
        // Calculate the end position of the request
        const requestEnd = calcRequestEnd(fileSize, block_size, self.file.tell());

        try self.sendGetRangeRequest(url, self.file.tell(), requestEnd);

        // We expect a HTTP code 206 Partial Content.
        if (206 == try parsed_response.processHeaders(try self.recieveResponse())) {

            // We compare the start position of the response with current file pointer position
            // If they do not match, we need to rewind the file a previous position
            // If the start position is smaller than the current position, we need to rewind to the start of the file in order to avoid holes and file corruption.
            if (self.file.tell() != parsed_response.range.?.start) {
                if (self.file.tell() > parsed_response.range.?.start) {
                    // Rewind to a previous position.
                    try self.file.lseek(parsed_response.range.?.start);
                } else {
                    // Rewind to file start
                    // This code will effectively rewind the file and restart the transfer.
                    try self.file.rewind();
                    try self.file.sync();
                    continue;
                }
            }

            if (requestEnd != parsed_response.range.?.end) {
                // Request end position does not match with the expected value
                // Not so tragic...
            }

            // Write the payload to the file
            // We store the current file position.
            // If the amount of bytes written into the file does not match with the expected length, we rewind to the previous position.
            const current_position = self.file.tell();
            if (parsed_response.payload.?.len != try self.file.write(parsed_response.payload.?)) {
                // Test if the bytes written match the payload.
                // current error handling will rewind to previous position
                try self.file.lseek(current_position);
            }

            // Move current position to the current file pointer
            // This is probably not necessary since the current position can be calculated from the block size.
            // However three things can happen:
            //   1. The response length is smaller than the requested block size.
            //   2. The write function could not write the full block.
            //   3. The response start position is not equal to requested start position.
            //
            // Here the code avoids throwing a failure cases and performs a re-synchronisation of the current pointers.
            //
            // A case that is not taken into account is when the current position is no longer aligned with the block size.
            // In this case, due to block misalignement the write operation might take longer than expected.

            // Perform sync to reduce chances of critical errors
            try self.file.sync();
        } else {
            return @"error".unexpected_status_code;
        }
        if (parsed_response.keep_alive) |kA| {
            if (kA == .close) {
                // Reconnect logic
                try self.connection.close();

                try self.connection.create(uri, null, if (proto.isSecure()) .psk else null);
            } else {
                //  Keep Alive
            }
        }
    }

    if (self.file.tell() != fileSize) {
        return @"error".file_size_mismatch;
    }

    // return the etag ?
}

pub fn eTag(self: *@This()) ?[]const u8 {
    return self.etag;
}

pub fn create(self: *@This()) void {
    if (config.enable_http) {
        //self.connection = connection.init(.http, authCallback, null); // mem corruption
        self.tx_mutex.create() catch unreachable;
        self.rx_mutex.create() catch unreachable;
    }
}

/// HTTP Response Header Types
const responseHeaders = enum(usize) {
    contentType,
    contentLength,
    contentRange,
    connection,
    contentLocation,
    contentEncoding,
    acceptRanges,
    etag,

    const stringMap = std.ComptimeStringMap(@This(), .{ .{ "Content-Type", .contentType }, .{ "Content-Length", .contentLength }, .{ "Content-Range", .contentRange }, .{ "Connection", .connection }, .{ "Content-Location", .contentLocation }, .{ "Content-Encoding", .contentEncoding }, .{ "Accept-Ranges", .acceptRanges }, .{ "ETag", .etag } });

    /// Match a response header to the stringmap
    fn match(header: c.phr_header) ?@This() {
        return stringMap.get(header.name[0..(header.name_len)]);
    }
};

/// Parse the accept ranges header
const acceptRanges = enum(usize) {
    none = @intFromBool(false),
    bytes = @intFromBool(true),

    const stringMap = std.ComptimeStringMap(@This(), .{ .{ "bytes", .bytes }, .{ "none", .none } });

    fn match(header: c.phr_header) !@This() {
        return (stringMap.get(header.value[0..(header.value_len)])) orelse @"error".parse_error;
    }
};

const keepAlive = enum(usize) {
    keep_alive = @intFromBool(true),
    close = @intFromBool(false),

    const stringMap = std.ComptimeStringMap(@This(), .{ .{ "keep-alive", .keep_alive }, .{ "close", .close } });

    fn match(header: c.phr_header) !@This() {
        return (stringMap.get(header.value[0..(header.value_len)])) orelse @"error".parse_error;
    }
};

const contentType = enum(usize) {
    unknown = 0,
    text_html,
    text_plain,
    application_json,
    application_xml,
    application_javascript,
    text_css,
    image_jpeg,
    application_octet_stream,
    application_pdf,
    application_zip,
    multipart_byteranges,

    const strings = [_][]const u8{ "Unknown", "text/html", "text/plain", "application/json", "application/xml", "application/javascript", "text/css", "image/jpeg", "application/octet-stream", "application/pdf", "application/zip", "multipart/byteranges" };

    /// Match content to Content-Type(s)
    fn match(header: c.phr_header) !@This() {
        for (strings, 0..) |ct, idx| {
            if (header.value_len >= ct.len) {
                if (std.mem.eql(u8, header.value[0..ct.len], ct)) {
                    return @enumFromInt(idx);
                }
            }
        }

        return @"error".parse_error;
    }

    fn getString(id: @This()) []const u8 {
        return strings[@intFromEnum(id)];
    }
};

pub var service: @This() = undefined;
