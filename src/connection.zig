const std = @import("std");
const freertos = @import("freertos.zig");
const config = @import("config.zig");

const c = @cImport({
    @cInclude("network.h");
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

pub fn Connection(comptime sslType: type) type {
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
        }
        pub fn open(self: *@This(), uri: std.Uri, local_port: ?u16) !void {
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
        pub fn waitRx(self: *@This(), timeout_s: u32) !bool {
            return self.ssl.waitRx(timeout_s);
        }
    };
}

fn run(self: *@This()) noreturn {
    var read_fd_set: c.SlFdSet_t = undefined;
    var write_fd_set: c.SlFdSet_t = undefined;

    c.SL_FD_ZERO(&read_fd_set);
    c.SL_FD_ZERO(&write_fd_set);

    self.mutex.give() catch {};

    while (true) {
        // Start the loop
        var read_set_ptr: ?*c.SlFdSet_t = null;
        var write_set_ptr: ?*c.SlFdSet_t = null;
        var nfsd: i16 = -1;

        c.SL_FD_ZERO(&read_fd_set);
        c.SL_FD_ZERO(&write_fd_set);

        const current_time = freertos.xTaskGetTickCount();

        // process the connection pool
        for (&self.connections) |*conn| {
            if (conn.sd >= 0) {
                if (conn.rx_queue.recieve(0)) |msg| {
                    if (msg.deadline > conn.rx_wait_deadline_ms) {
                        conn.rx_wait_deadline_ms = msg.deadline;
                    }
                }
                if (conn.tx_queue.recieve(0)) |msg| {
                    if (msg.deadline > conn.tx_wait_deadline_ms) {
                        conn.tx_wait_deadline_ms = msg.deadline;
                    }
                }

                if (conn.rx_wait_deadline_ms >= current_time) {
                    c.SL_FD_SET(conn.sd, &read_fd_set);
                    if (nfsd < conn.sd) {
                        nfsd = conn.sd;
                    }
                    read_set_ptr = &read_fd_set;
                } else {
                    conn.rx_wait_deadline_ms = 0; // reset deadline
                    conn.rx_signal.give() catch {};
                }

                if (conn.tx_wait_deadline_ms >= current_time) {
                    c.SL_FD_SET(conn.sd, &write_fd_set);
                    if (nfsd < conn.sd) {
                        nfsd = conn.sd;
                    }
                    write_set_ptr = &write_fd_set;
                } else {
                    conn.tx_wait_deadline_ms = 0; // reset deadline
                    conn.tx_signal.give() catch {};
                }
            }
        }

        if ((read_set_ptr != null) or (write_set_ptr != null)) {
            var res: i16 = 0;
            // Sem
            if (self.mutex.take(null)) |_| {
                const tv = c.SlTimeval_t{ .tv_sec = 0, .tv_usec = 0 };

                res = c.sl_Select(nfsd + 1, read_set_ptr, write_set_ptr, null, @constCast(&tv));

                self.mutex.give() catch unreachable;
            } else |_| {
                //Ignore
            }

            var waiting_count: usize = 0;
            if (res > 0) {
                if (read_set_ptr) |read_set| {
                    for (&self.connections) |*conn| {
                        if (conn.sd >= 0) {
                            if (1 == c.SL_FD_ISSET(conn.sd, read_set)) {
                                conn.sd = -1;
                                conn.rx_wait_deadline_ms = 0;
                                conn.rx_signal.give() catch unreachable;
                            } else {
                                waiting_count += 1;
                            }
                        }
                    }
                }
            } else if (res == 0) {
                if (read_set_ptr) |_| {
                    for (&self.connections) |*conn| {
                        if (conn.sd >= 0) {
                            waiting_count += 1;
                        }
                    }
                }

                // self.task.delayTask(10);
            } else {
                // Error
                @breakpoint();
            }

            if (waiting_count > 0) {
                self.task.delayTask(100);
            }
        } else {
            // no deadlines
            if (self.task.waitForNotify(0, 0xFFFFFFFF, 2000)) |_| {
                // Do something
            } else |_| {
                // Do nothing
            }
        }
        // Do something
    }
}

const timeout_msg = struct {
    deadline: u32,
};

const connectionManagerElement = struct {
    sd: i16 = -1,
    rx_wait_deadline_ms: u32 = 0,
    tx_wait_deadline_ms: u32 = 0,
    rx_signal: freertos.StaticBinarySemaphore(),
    tx_signal: freertos.StaticBinarySemaphore(),
    rx_queue: freertos.StaticQueue(timeout_msg, 1),
    tx_queue: freertos.StaticQueue(timeout_msg, 1),

    pub fn init(self: *@This()) void {
        self.sd = -1;
        self.rx_wait_deadline_ms = 0;
        self.tx_wait_deadline_ms = 0;
        self.rx_signal.create() catch unreachable;
        self.tx_signal.create() catch unreachable;
        self.rx_queue.create() catch unreachable;
        self.tx_queue.create() catch unreachable;
    }
};

task: freertos.StaticTask(@This(), 1200, "select_task", run),
mutex: freertos.StaticMutex(),
connections: [4]connectionManagerElement = undefined,

/// Look if socket is already in the pool
fn findSocket(self: *@This(), sd: i16) ?*connectionManagerElement {
    for (&self.connections) |*conn| {
        if (conn.sd == sd) {
            return conn;
        }
    }
    return null;
}

/// Look for the first free position in the pool
fn findFreeSocket(self: *@This(), sd: i16) ?*connectionManagerElement {
    for (&self.connections) |*conn| {
        if (conn.sd < 0) {
            conn.sd = sd;
            return conn;
        }
    }
    return null;
}

pub fn initializeConnectionsManager(self: *@This()) void {
    for (&self.connections) |*conn| {
        conn.init();
    }
}

pub fn init(self: *@This()) !void {
    try self.task.create(self, 4);
    try self.mutex.create();
    self.initializeConnectionsManager();
    self.task.suspendTask();
}

pub fn wait_rx(self: *@This(), sd: i16, timeout_s: u32) !bool {
    const timeout_ms = timeout_s * 1000;
    var current_time = freertos.xTaskGetTickCount();
    const deadline_ms = current_time + timeout_ms + 1000;

    const msg: timeout_msg = .{ .deadline = deadline_ms };

    // Find a connection in the pool.
    var conn: *connectionManagerElement = self.findSocket(sd) orelse (self.findFreeSocket(sd) orelse unreachable);

    _ = conn.rx_signal.take(0) catch false; // Dummy take to block the signal

    try conn.rx_queue.send(&msg, timeout_ms);

    self.task.notify(1, .eIncrement) catch {};

    current_time = freertos.xTaskGetTickCount();
    if (current_time <= deadline_ms) {
        if (conn.rx_signal.take(deadline_ms - current_time)) |val| {
            return val;
        } else |err| {
            return if (err == freertos.FreeRtosError.SemaphoreTakeTimeout) false else err;
        }
    }

    return false;
}

pub var connectionManager: @This() = undefined;

/// Handle the mbedtls threading
extern fn miso_mbedtls_set_treading_alt() callconv(.C) void;

export fn create_network_mediator() callconv(.C) c_int {
    connectionManager.init() catch unreachable;
    miso_mbedtls_set_treading_alt();
    return 0;
}

export fn suspend_network_mediator() callconv(.C) void {
    connectionManager.task.suspendTask();
}

export fn resume_network_mediator() callconv(.C) void {
    connectionManager.task.resumeTask();
}

pub fn network_mediator_wait_rx(sd: i16, timeout_s: u32) !bool {
    return connectionManager.wait_rx(sd, timeout_s);
}
