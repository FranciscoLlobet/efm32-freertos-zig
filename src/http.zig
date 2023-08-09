const std = @import("std");
const freertos = @import("freertos.zig");
const config = @import("config.zig");
const system = @import("system.zig");
const connection = @import("connection.zig");

const c = @cImport({
    @cInclude("board.h");
    @cInclude("picohttpparser.h");
});

connection: connection,
task: freertos.Task,
timer: freertos.Timer,
headers: [24]c.phr_header,
tx_buffer: [256]u8 align(@alignOf(u32)),
rx_buffer: [1024]u8 align(@alignOf(u32)),

// "content-type"

fn authCallback(self: *connection.mbedtls, security_mode: connection.security_mode) i32 {
    _ = self;
    _ = security_mode;

    return 0;
}

const parsedResponse = struct {
    // Parsed code
    // Parsed content-type
    status_code: i32,
    content_type: ?contentType,
    content_length: ?isize,
    content_range_start: ?isize,
    content_range_end: ?isize,
    accept_ranges: ?bool,
    //

    payload: ?*u8,
    payload_length: usize,
};

fn taskFunction(pvParameters: ?*anyopaque) callconv(.C) void {
    var self = freertos.getAndCastPvParameters(@This(), pvParameters);

    const url = "http://192.168.50.133:80/test.json";

    var parsed = std.Uri.parse(url) catch unreachable;

    var zigHeader = std.http.Headers{ .allocator = freertos.allocator };

    // Prepare Header
    zigHeader.append("Accept", contentType.getString(.application_json)) catch unreachable;
    zigHeader.append("User-Agent", "Miso (FreeRTOS)") catch unreachable;

    // Write Header

    zigHeader.deinit();

    while (true) {
        var ret = self.connection.create("192.168.50.133", "80", null, .tcp_ip4, null);

        const request = std.fmt.bufPrint(&self.tx_buffer, "HEAD {s} HTTP/1.1\r\nHost: {s}\r\n\r\n", .{ parsed.path, parsed.host.? }) catch unreachable;

        if (ret == 0) {
            ret = self.connection.send(@ptrCast(request), request.len);
        }

        @memset(&self.rx_buffer, 0);
        if (0 == self.connection.waitRx(5)) {
            var parsed_response: parsedResponse = undefined;

            var rx_count: usize = 0;
            var pret: i32 = -2; // Incomplete request
            var prevbuflen: usize = 0;
            ret = 0;

            var minor_version: i32 = undefined;
            var status: i32 = undefined;
            var msg: [*c]u8 = undefined;
            var meg_len: usize = undefined;
            //var last_len: usize = @intCast(ret);
            var num_headers: usize = 24;

            while (pret == -2) {
                ret = self.connection.recieve(&self.rx_buffer[rx_count], self.rx_buffer.len - rx_count);
                if (ret > 0) {
                    pret = c.phr_parse_response(&self.rx_buffer[prevbuflen], @intCast(ret), &minor_version, &status, &msg, &meg_len, &self.headers, &num_headers, prevbuflen);
                    prevbuflen = rx_count;
                    rx_count += @intCast(ret);
                } else {
                    break;
                }
            } // return tx_count - pret

            if ((pret > 0) and (ret >= 0)) {
                parsed_response.payload_length = rx_count - @as(usize, @intCast(pret));
                parsed_response.payload = if (parsed_response.payload_length != 0) &self.rx_buffer[rx_count - parsed_response.payload_length] else null;

                // process the response status
                parsed_response.status_code = status;
                parsed_response.content_type = null;
                parsed_response.content_length = null;
                parsed_response.content_range_start = null;
                parsed_response.content_range_end = null;
                parsed_response.accept_ranges = null;

                // Parse GET response headers
                for (self.headers[0..num_headers]) |header| {
                    if (responseHeaders.getResponseType(header)) |val| {
                        if (val == .contentType) {
                            parsed_response.content_type = contentType.match(header);
                        } else if (val == .contentLength) {
                            parsed_response.content_length = std.fmt.parseInt(i32, header.value[0..header.value_len], 10) catch unreachable;
                        } else if (val == .contentRange) {
                            // process content range
                        } else if (val == .acceptRanges) {
                            // Convert this into a stringmap loopkup which should be safer
                            if (std.mem.eql(u8, "bytes", header.value[0..header.value_len])) {
                                parsed_response.accept_ranges = true;
                            } else if (std.mem.eql(u8, "none", header.value[0..header.value_len])) {
                                parsed_response.accept_ranges = false;
                            }
                        }
                    }
                }
                _ = c.printf("Content-Length: %d\r\n", parsed_response.content_length.?);
            }
        }

        ret = self.connection.close();

        self.task.delayTask(5000);

        _ = c.printf("HTTP: %d\n\r", self.task.getStackHighWaterMark());
    }
}

fn dummyTaskFunction(pvParameters: ?*anyopaque) callconv(.C) void {
    var self = freertos.getAndCastPvParameters(@This(), pvParameters);

    while (true) {
        self.task.suspendTask();
    }
}

pub fn create(self: *@This()) void {
    self.task.create(if (config.enable_http) taskFunction else dummyTaskFunction, "http", config.rtos_stack_depth_http, self, config.rtos_prio_http) catch unreachable;
    self.task.suspendTask();

    if (config.enable_http) {
        self.connection = connection.init(.http, authCallback);
    }
}

pub fn getTaskHandle(self: *@This()) freertos.TaskHandle_t {
    return self.task.getHandle();
}

pub var service: @This() = undefined;

const httpHeaderIndexes = enum(i32) {
    http_get,
    http_post,
    http_put,
};

const method_strings = [_][]u8{ "GET", "POST", "PUT", "DELETE", "HEAD", "OPTIONS", "PATCH" };
const header_vals = 0;
//const header_kvs = struct{}

const responseHeaders = enum(usize) {
    contentType = 0,
    contentLength = 1,
    contentRange = 2,
    connection = 3,
    contentLocation = 4,
    contentEncoding = 5,
    acceptRanges = 6,

    const stringMap = std.ComptimeStringMap(@This(), .{ .{ "Content-Type", .contentType }, .{ "Content-Length", .contentLength }, .{ "Content-Range", .contentRange }, .{ "Connection", .connection }, .{ "Content-Location", .contentLocation }, .{ "Content-Encoding", .contentEncoding }, .{ "Accept-Ranges", .acceptRanges } });

    fn getResponseType(header: c.phr_header) ?@This() {
        return stringMap.get(header.name[0..(header.name_len)]);
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
    not_found = 255,

    const strings = [_][]const u8{ "Unknown", "text/html", "text/plain", "application/json", "application/xml", "application/javascript", "text/css", "image/jpeg", "application/octet-stream", "application/pdf", "application/zip", "multipart/byteranges" };

    fn match(header: c.phr_header) @This() {
        // Match content length to Content-Type(s)
        var idx: usize = 0;
        for (strings) |ct| {
            if (header.value_len >= ct.len) {
                if (std.mem.eql(u8, header.value[0..ct.len], ct)) {
                    return @enumFromInt(@as(isize, @intCast(idx)));
                }
            }
            idx = idx + 1;
        }

        return .unknown;
    }

    fn getString(id: @This()) []const u8 {
        return strings[@intFromEnum(id)];
    }
};
