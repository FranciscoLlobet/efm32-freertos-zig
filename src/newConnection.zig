const std = @import("std");
const board = @import("microzig").board;
const freertos = @import("freertos.zig");
const config = @import("config.zig");
const system = @import("system.zig");
pub const mbedtls = @import("mbedtls.zig");

const c = @cImport({
    @cInclude("network.h");
    @cInclude("wifi_service.h");
    //    @cInclude("lwm2m_client.h");
});

const config_connection_pool_size: usize = 4;

const connection = @This();

// Connection pool
var connection_pool: [config_connection_pool_size]@This() = undefined;

// Connection Start-End Mutex
var connection_mutex: freertos.Semaphore = undefined;

// RX TX Access mutex
var tx_rx_mutex: freertos.Semaphore = undefined;

// RX Select Queue
var rx_queue: freertos.Queue = undefined;
var tx_queue: freertos.Queue = undefined;

// Select timeout message
const timeout_msg = struct {
    deadline: u32,
    ctx: *@This(),
};

const connect_fn = fn (self: *@This()) void;
const send_fn = fn (self: *@This(), buffer: []u8) i32;
const read_fn = fn (self: *@This(), buffer: []u8) i32;
const close_fn = fn (self: *@This()) void;

// Fields
sd: i32,
protocol: i32,
rx_semaphore: freertos.Semaphore,
tx_semaphore: freertos.Semaphore,

// Peer address

// Local address
send_fn: *send_fn,
read_fn: *read_fn,
close_fn: *close_fn,
ssl: *mbedtls,

// Local Address

// Peer Address
