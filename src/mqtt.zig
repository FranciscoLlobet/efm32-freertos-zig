/// Basic MQTT Client implementation in Zig using FreeRTOS as OS
/// This is a work in progress and is not ready for production use
///
/// Not all aspects of the MQTT state machine were implemented
/// QoS 2 is partially supported
///
const std = @import("std");
const freertos = @import("freertos.zig");
const config = @import("config.zig");
const system = @import("system.zig");
const connection = @import("connection.zig");

const c = @cImport({
    @cDefine("MQTT_CLIENT", "1");
    @cInclude("MQTTPacket.h");
    @cInclude("miso_config.h");
});

const keepAliveInterval_s = 60;
const keepAliveInterval_ms = 1000 * keepAliveInterval_s;

const MQTTTransport = c.MQTTTransport;
const MQTTString = c.MQTTString;
const MQTTLenString = c.MQTTLenString;

const mqtt_ok: c_int = 1;

const msgTypes = enum(c_int) { err_msg = -1, try_again = 0, connect = c.CONNECT, connack = c.CONNACK, publish = c.PUBLISH, puback = c.PUBACK, pubrec = c.PUBREC, pubrel = c.PUBREL, pubcomp = c.PUBCOMP, subscribe = c.SUBSCRIBE, suback = c.SUBACK, unsubscribe = c.UNSUBSCRIBE, unsuback = c.UNSUBACK, pingreq = c.PINGREQ, pingresp = c.PINGRESP, disconnect = c.DISCONNECT };

const state = enum(i32) {
    err = -1,
    not_connected = 0,
    connecting,
    connected,
    disconnecting,
};

pub const mqtt_error = error{
    packetlen,
    enqueue_failed,
    dequeue_failed,
    connect_failed,
    send_failed,
    connack_failed,
    parse_failed,
    publish_parse_failed,
    qos_not_supported,

    qos_packet_not_found,
    qos_packet_timeout,
};

const packet_response = @This().packet.publish_response;

const MQTTString_initializer = MQTTString{ .cstring = null, .lenstring = .{ .len = 0, .data = null } };

// Manually translated initializer
const MQTTPacket_willOptions_initializer = c.MQTTPacket_willOptions{
    .struct_id = [_]u8{ 'M', 'Q', 'T', 'W' },
    .struct_version = 0,
    .topicName = MQTTString_initializer,
    .message = MQTTString_initializer,
    .retained = 0,
    .qos = 0,
};

// Manually translated initializer
const MQTTPacket_connectData_initializer = c.MQTTPacket_connectData{
    .struct_id = [_]u8{ 'M', 'Q', 'T', 'C' },
    .struct_version = 0,
    .MQTTVersion = 4,
    .clientID = MQTTString_initializer,
    .keepAliveInterval = keepAliveInterval_s,
    .cleansession = 1,
    .willFlag = 0,
    .will = MQTTPacket_willOptions_initializer,
    .username = MQTTString_initializer,
    .password = MQTTString_initializer,
};

connection: connection,
connectionCounter: usize,
disconnectionCounter: usize,
rxQueue: freertos.Queue,

pingTimer: freertos.Timer,
pubTimer: freertos.Timer, // delete this!
task: freertos.Task,
state: state,
packet: @This().packet,
uri_string: [*:0]u8,
device_id: [*:0]u8,

/// Qos1 tx queue
qosQueue: QueuedMessgeQueue,

/// QoS2 rx queue
qos2Queue: QueuedMessgeQueue,

var txBuffer: [256]u8 align(@alignOf(u32)) = undefined;
var rxBuffer: [512]u8 align(@alignOf(u32)) = undefined;
var workBuffer: [256]u8 align(@alignOf(u32)) = undefined;

/// Init a MQTT String using slice.
/// Has been tested in comptime
inline fn initMQTTString(data: []const u8) MQTTString {
    return MQTTString{ .cstring = null, .lenstring = .{ .len = @intCast(data.len), .data = @constCast(data.ptr) } };
}

/// Init a MQTT String using a pointer and asuming a 0-terminated C-String
inline fn initMQTTCstring(data: [*:0]u8) MQTTString {
    return MQTTString{ .cstring = @ptrCast(data), .lenstring = .{ .len = 0, .data = null } };
}

/// Get MQTT String as slice
inline fn getMQTTString(data: MQTTString) []u8 {
    if (data.cstring) |cstring| {
        return cstring[0..c.strlen(cstring)];
    } else {
        return data.lenstring.data[0..@intCast(data.lenstring.len)];
    }
}

fn init() @This() {
    return @This(){
        .connection = undefined,
        .connectionCounter = 0,
        .disconnectionCounter = 0,
        .rxQueue = undefined,
        .pingTimer = undefined,
        .pubTimer = undefined,
        .task = undefined,
        .state = .not_connected,
        .packet = packet.init(0x5555),
        .uri_string = undefined,
        .device_id = undefined,
        .qosQueue = QueuedMessgeQueue.init(freertos.allocator),
        .qos2Queue = QueuedMessgeQueue.init(freertos.allocator),
    };
}

fn authCallback(self: *connection.mbedtls, security_mode: connection.security_mode) connection.mbedtls.auth_error!void {
    if (security_mode == .psk) {
        var psk_buf: [64]u8 = undefined; // Request 64 Bytes for Base64 decoder

        const psk = connection.mbedtls.base64Decode(c.config_get_mqtt_psk_key(), &psk_buf) catch return connection.mbedtls.auth_error.generic_error;
        self.confPsk(psk, c.config_get_mqtt_psk_id()) catch return connection.mbedtls.auth_error.generic_error;

        @memset(&psk_buf, 0); // Sanitize the buffer to avoid the decoded psk to remain in stack
    } else {
        return connection.mbedtls.auth_error.unsuported_mode;
    }
}

fn processSendQueue(self: *@This()) !void {
    while (self.packet.txQueue.receive(&txBuffer, 0)) |buf| {
        _ = try self.connection.send(buf);
    }
}

const QoS = enum(u8) { qos0 = 0, qos1 = 1, qos2 = 2 };

const QueuedMessage = struct {
    packetId: ?u16,
    qos: QoS,
    dup: bool,
    retained: bool,
    deadline: u32,
    topic: []u8,
    payload: []u8,
};

const QueuedMessgeQueue = struct {
    message: std.ArrayList(QueuedMessage),

    pub fn init(comptime allocator: std.mem.Allocator) @This() {
        return @This(){ .message = std.ArrayList(QueuedMessage).init(allocator) };
    }

    pub fn add(self: *@This(), packetId: u16, qos: QoS, dup: bool, retained: bool, topic: []const u8, payload: []const u8, deadline: u32) !void {
        var new_payload = try self.message.allocator.alloc(u8, payload.len);
        var new_topic = try self.message.allocator.alloc(u8, topic.len);

        @memcpy(new_payload, payload);
        @memcpy(new_topic, topic);

        try self.message.append(QueuedMessage{ .packetId = packetId, .qos = qos, .dup = dup, .retained = retained, .topic = new_topic, .payload = new_payload, .deadline = deadline });
    }

    /// Scan the queue for messages that match the packetId
    pub fn acknowledge(self: *@This(), packetId: u16) !void {
        for (self.message.items, 0..) |*item, idx| {
            if (item.packetId) |id| {
                if (id == packetId) {
                    // Remove the message from the list
                    var msg = self.message.orderedRemove(idx);

                    // Clear topic and payload content
                    @memset(msg.topic, 0);
                    @memset(msg.payload, 0);

                    self.message.allocator.free(msg.topic);
                    self.message.allocator.free(msg.payload);

                    return;
                }
            }
        }
        return mqtt_error.qos_packet_not_found;
    }

    pub fn prune(self: *@This()) ?QueuedMessage {
        for (self.message.items, 0..) |*item, idx| {
            if (item.deadline < system.time.now()) {
                return self.message.orderedRemove(idx);
            }
        }
        return null;
    }

    pub fn findAndRemove(self: *@This(), packetId: u16) ?QueuedMessage {
        for (self.message.items, 0..) |*item, idx| {
            if (item.packetId) |id| {
                if (id == packetId) {
                    return self.message.orderedRemove(idx);
                }
            }
        }
        return null;
    }
};

const packet = struct {
    transport: MQTTTransport,
    packetIdState: u16,
    workBufferMutex: freertos.Mutex,
    txQueue: freertos.MessageBuffer = undefined,

    /// Publish packet response
    const publish_response = struct {
        packetId: u16,
        qos: QoS,
        dup: bool,
        retained: bool,
        topic: []u8,
        payload: []u8,
    };

    pub fn init(comptime packetId: u16) @This() {
        return @This(){ .transport = .{
            .getfn = getFn,
            .sck = undefined,
            .multiplier = undefined,
            .rem_len = undefined,
            .len = undefined,
            .state = undefined,
        }, .packetIdState = packetId, .workBufferMutex = undefined };
    }

    pub fn create(self: *@This(), conn: *connection) void {
        self.transport.sck = @ptrCast(conn);

        self.workBufferMutex = freertos.Mutex.create() catch unreachable;

        self.txQueue = freertos.MessageBuffer.create(1024) catch unreachable; // Create message buffer
    }

    // Use XorShift Algorithm to generate the packet id
    fn generatePacketId(self: *@This()) u16 {
        if (self.packetIdState != 0) {
            self.packetIdState ^= self.packetIdState << 7;
            self.packetIdState ^= self.packetIdState >> 9;
            self.packetIdState ^= self.packetIdState << 8;
        } else {
            self.packetIdState = 0x5555;
        }

        return self.packetIdState;
    }

    fn getFn(ptr: ?*anyopaque, buf: [*c]u8, buf_len: c_int) callconv(.C) c_int {
        var self = @as(*connection, @ptrCast(@alignCast(ptr))); // Cast ptr as connection

        const ret = self.recieve(buf[0..@intCast(buf_len)]) catch {
            return @intFromEnum(msgTypes.err_msg);
        };

        return @intCast(ret.len);
    }

    fn read(self: *@This(), buffer: []u8) msgTypes {
        self.transport.state = 0;
        return @as(msgTypes, @enumFromInt(c.MQTTPacket_readnb(@ptrCast(&buffer[0]), @intCast(buffer.len), &self.transport)));
    }

    fn deserializePublish(self: *@This(), buffer: []u8) !publish_response {
        _ = self;
        var topicName = MQTTString_initializer;
        var payload: [*c]u8 = undefined;
        var payloadLen: isize = undefined;
        var packetId: u16 = 0;
        var retained: u8 = undefined;
        var dup: u8 = undefined;
        var qos: c_int = undefined;

        if (mqtt_ok != c.MQTTDeserialize_publish(&dup, &qos, &retained, &packetId, &topicName, &payload, &payloadLen, buffer.ptr, @intCast(buffer.len))) {
            return mqtt_error.parse_failed;
        }

        const retQos: QoS = switch (qos) {
            0 => .qos0,
            1 => .qos1,
            2 => .qos2,
            else => return mqtt_error.qos_not_supported,
        };

        return publish_response{ .packetId = packetId, .qos = retQos, .dup = (if (dup == 0) false else true), .retained = (if (retained == 0) false else true), .topic = getMQTTString(topicName), .payload = payload[0..@intCast(payloadLen)] };
    }

    fn deserializeAck(self: *@This(), buffer: []u8, packetType: *u8, dup: *u8) !u16 {
        _ = self;
        var packetId: u16 = undefined;

        if (mqtt_ok != c.MQTTDeserialize_ack(packetType, dup, &packetId, @ptrCast(&buffer[0]), @intCast(buffer.len))) {
            return mqtt_error.parse_failed;
        }
        return packetId;
    }

    fn deserializeSubAck(self: *@This(), buffer: []u8, maxCount: c_int, count: *c_int, grantedQoSs: *c_int) !u16 {
        _ = self;
        var packetId: u16 = undefined;
        if (mqtt_ok != c.MQTTDeserialize_suback(&packetId, maxCount, count, grantedQoSs, @ptrCast(&buffer[0]), @intCast(buffer.len))) {
            return mqtt_error.parse_failed;
        }
        return packetId;
    }

    /// Check the output of the MQTTSerialize_xyz functions for error returns and avoids buffer overflows
    /// Returns a slice of the workBuffer
    fn serializeCheck(packetLen: isize) ![]u8 {
        if (packetLen <= 0) {
            return mqtt_error.packetlen; // Could not serialize packet
        } else if (packetLen > workBuffer.len) {
            return mqtt_error.packetlen; // Packet too big
        } else {
            return workBuffer[0..@intCast(packetLen)];
        }
    }

    fn prepareConnectPacket(self: *@This(), clientID: []const u8, username: ?[]const u8, password: ?[]const u8) !u16 {
        var connectPacket = MQTTPacket_connectData_initializer;
        connectPacket.clientID = initMQTTString(clientID);

        if (username) |un| {
            connectPacket.username = initMQTTString(un);
        }

        if (password) |pw| {
            connectPacket.password = initMQTTString(pw);
        }

        connectPacket.keepAliveInterval = 400;

        _ = try self.workBufferMutex.take(null);
        defer self.workBufferMutex.give() catch {};

        return self.sendtoTxQueue(try serializeCheck(c.MQTTSerialize_connect(&workBuffer[0], workBuffer.len, &connectPacket)), null);
    }

    fn preparePubAckPacket(self: *@This(), packetId: u16) !u16 {
        _ = try self.workBufferMutex.take(null);
        defer self.workBufferMutex.give() catch {};

        return self.sendtoTxQueue(try serializeCheck(c.MQTTSerialize_puback(@ptrCast(&workBuffer[0]), workBuffer.len, packetId)), packetId);
    }

    fn preparePubRecPacket(self: *@This(), packetId: u16) !u16 {
        _ = try self.workBufferMutex.take(null);
        defer self.workBufferMutex.give() catch {};

        return self.sendtoTxQueue(try serializeCheck(c.MQTTSerialize_pubrec(@ptrCast(&workBuffer[0]), workBuffer.len, packetId)), packetId);
    }

    fn preparePubCompPacket(self: *@This(), packetId: u16) !u16 {
        _ = try self.workBufferMutex.take(null);
        defer self.workBufferMutex.give() catch {};

        return self.sendtoTxQueue(try serializeCheck(c.MQTTSerialize_pubcomp(@ptrCast(&workBuffer[0]), workBuffer.len, packetId)), packetId);
    }

    fn preparePubRelPacket(self: *@This(), dup: u8, packetId: u16) !u16 {
        _ = try self.workBufferMutex.take(null);
        defer self.workBufferMutex.give() catch {};

        return self.sendtoTxQueue(try serializeCheck(c.MQTTSerialize_pubrel(@ptrCast(&workBuffer[0]), workBuffer.len, dup, packetId)), packetId);
    }

    fn prepareSubscribePacket(self: *@This(), count: usize, topicFilter: *MQTTString, qos: *c_int) !u16 {
        _ = try self.workBufferMutex.take(null);
        defer self.workBufferMutex.give() catch {};

        const packetId = self.generatePacketId();
        const dup: u8 = 0;

        return self.sendtoTxQueue(try serializeCheck(c.MQTTSerialize_subscribe(@ptrCast(&workBuffer[0]), workBuffer.len, dup, packetId, @intCast(count), topicFilter, qos)), packetId);
    }

    fn preparePingPacket(self: *@This()) !u16 {
        _ = try self.workBufferMutex.take(null);
        defer self.workBufferMutex.give() catch {};

        return self.sendtoTxQueue(try serializeCheck(c.MQTTSerialize_pingreq(@ptrCast(&workBuffer[0]), workBuffer.len)), null);
    }

    fn prepareDisconnectPacket(self: *@This()) !u16 {
        _ = try self.workBufferMutex.take(null);
        defer self.workBufferMutex.give() catch {};

        return self.sendtoTxQueue(try serializeCheck(c.MQTTSerialize_disconnect(@ptrCast(&workBuffer[0]), workBuffer.len)), null);
    }

    pub fn preparePublishPacket(self: *@This(), topic: []const u8, payload: []const u8, qos: QoS, dup: bool, packetId: ?u16) !u16 {
        _ = try self.workBufferMutex.take(null);
        defer self.workBufferMutex.give() catch {};

        const topic_name = initMQTTString(topic);
        const retain: u8 = 0;
        const id = packetId orelse self.generatePacketId();

        return self.sendtoTxQueue(try serializeCheck(c.MQTTSerialize_publish(&workBuffer[0], workBuffer.len, if (dup) 1 else 0, @intFromEnum(qos), retain, id, topic_name, @constCast(payload.ptr), @intCast(payload.len))), id);
    }

    pub fn processConnAck(self: *@This(), buffer: []u8) !void {
        _ = self;
        var sessionPresent: u8 = undefined;
        var connack_rc: u8 = undefined;

        if (mqtt_ok == c.MQTTDeserialize_connack(&sessionPresent, &connack_rc, @ptrCast(&buffer[0]), @intCast(buffer.len))) {
            if (connack_rc != c.MQTT_CONNECTION_ACCEPTED) {
                return mqtt_error.connack_failed;
            }
        } else {
            return mqtt_error.parse_failed;
        }
    }

    /// Send the content of the buffer to the txQueue.
    /// The optional packet ID is propagated to the next layer if the operation was succesful
    fn sendtoTxQueue(self: *@This(), buffer: []u8, packetId: ?u16) !u16 {
        if (buffer.len != self.txQueue.send(buffer, null)) {
            return mqtt_error.enqueue_failed;
        }

        return @intCast(packetId orelse 0);
    }
};

fn loop(self: *@This(), uri: std.Uri) !void {
    try self.connect(uri);
    defer self.disconnect() catch {};

    while (true) {
        // process the qos tx queue
        while (self.qosQueue.prune()) |msg| {
            try self.publish(msg.topic, msg.payload, msg.qos, msg.dup, system.time.calculateDeadline(2000)); // Do resend
        }

        // process the qos tx queue
        while (self.qos2Queue.prune()) |msg| {
            @memset(msg.topic, 0);
            @memset(msg.payload, 0);
            self.qos2Queue.message.allocator.free(msg.topic);
            self.qos2Queue.message.allocator.free(msg.payload);
        }

        try self.processSendQueue();

        if (0 == self.connection.waitRx(1)) {
            var readRet = self.packet.read(&rxBuffer);
            switch (readRet) {
                .try_again => {},
                .publish => {
                    const publish_response = try self.packet.deserializePublish(&rxBuffer);

                    // prepare the response packets depwnding on the QOS
                    const packetId: u16 = switch (publish_response.qos) {
                        .qos0 => publish_response.packetId,
                        .qos1 => try self.packet.preparePubAckPacket(publish_response.packetId),
                        .qos2 => try self.packet.preparePubRecPacket(publish_response.packetId),
                    };

                    // Immediatly send the ACK to the broker in case of QOS 1
                    if (publish_response.qos == .qos1) {
                        try self.processSendQueue();
                    }

                    if (publish_response.qos != .qos2) {
                        // If QOS is 0 or 1, then we can send the message to the application layer
                        var buf: [64]u8 = undefined;
                        @memset(&buf, 0);

                        var s = try std.fmt.bufPrint(&buf, "publish recieved: {d}, {d} {s} {s}\r\n", .{ packetId, @intFromEnum(publish_response.qos), publish_response.topic, publish_response.payload });

                        _ = c.printf("%s", s.ptr);
                    } else {
                        // If the publish message has QOS2, then we should to wait for the pubrel
                        // The code should actually enqueue the message and process it later when the pubrel is received
                        // Enqueue the message into the QOS2 rx queue
                        try self.qos2Queue.add(packetId, publish_response.qos, publish_response.dup, publish_response.retained, publish_response.topic, publish_response.payload, system.time.calculateDeadline(2000));
                    }
                },
                .puback => {
                    // Response for Client pub qos1
                    var packetType: u8 = undefined;
                    var dup: u8 = undefined;

                    const rx_packetId = try self.packet.deserializeAck(&rxBuffer, &packetType, &dup);

                    // Process the puback packet
                    // Remove the packet from the qos1 tx queue
                    try self.qosQueue.acknowledge(rx_packetId);
                },
                .pingresp => {
                    _ = c.printf("pingresp!");
                },
                .connack => try self.packet.processConnAck(&rxBuffer),
                .connect, .subscribe, .disconnect, .unsubscribe, .pingreq => break, // Broker messages
                .suback => {
                    var maxCount: c_int = 1;
                    var count: c_int = 0;
                    var grantedQoSs: c_int = 0;

                    // Deserialize
                    const packetId = try self.packet.deserializeSubAck(&rxBuffer, maxCount, &count, &grantedQoSs);
                    _ = packetId;
                }, // To-do: process the sub-ack
                .unsuback => {
                    // currently unsuported
                },
                .pubrec => {
                    // generate pubrel package
                    var packetType: u8 = undefined;
                    var dup: u8 = undefined;

                    const rx_packetId = try self.packet.deserializeAck(&rxBuffer, &packetType, &dup);

                    _ = try self.packet.preparePubRelPacket(0, rx_packetId);
                },
                .pubrel => {
                    // generate pubcomp package
                    var packetType: u8 = undefined;
                    var dup: u8 = undefined;

                    const rx_packetId = try self.packet.deserializeAck(&rxBuffer, &packetType, &dup);

                    if (self.qos2Queue.findAndRemove(rx_packetId)) |msg| {
                        _ = try self.packet.preparePubCompPacket(rx_packetId);
                        try self.processSendQueue();

                        var buf: [64]u8 = undefined;
                        @memset(&buf, 0);

                        var s = try std.fmt.bufPrint(&buf, "Pubrel recieved: {d}, {d} {s} {s}\r\n", .{ rx_packetId, @intFromEnum(msg.qos), msg.topic, msg.payload });

                        _ = c.printf("%s", s.ptr);

                        @memset(msg.topic, 0);
                        @memset(msg.payload, 0);

                        self.qos2Queue.message.allocator.free(msg.topic);
                        self.qos2Queue.message.allocator.free(msg.payload);
                    }
                },
                .pubcomp => {
                    // publish complete
                    var packetType: u8 = undefined;
                    var dup: u8 = undefined;
                    var rx_packetId: u16 = undefined;

                    rx_packetId = try self.packet.deserializeAck(&rxBuffer, &packetType, &dup);

                    _ = c.printf("pubcomp received for packetId\r\n", rx_packetId);
                    // process the qos tx queue

                },
                .err_msg => break,
            }
        } else {
            _ = c.printf("mqtt: %d\r\n", self.task.getStackHighWaterMark());
        }

        // Process the qos1 to 2 tx queue

    }
}

fn taskFunction(pvParameters: ?*anyopaque) callconv(.C) void {
    const self = freertos.Task.getAndCastPvParameters(@This(), pvParameters);

    // Clear the buffers
    @memset(&txBuffer, 0);
    @memset(&rxBuffer, 0);
    @memset(&workBuffer, 0);

    self.connectionCounter = 0;
    self.disconnectionCounter = 0;

    self.uri_string = c.config_get_mqtt_url();
    self.device_id = c.config_get_mqtt_device_id();

    // Get to connect
    while (true) {
        var uri = std.Uri.parse(self.uri_string[0..c.strlen(self.uri_string)]) catch unreachable;

        self.loop(uri) catch {
            _ = c.printf("Disconnected... reconnect: %d \r\n", self.connectionCounter);
        };
    }

    // Go to disconnect phase
}

fn dummyTaskFunction(pvParameters: ?*anyopaque) callconv(.C) void {
    const self = freertos.getAndCastPvParameters(@This(), pvParameters);

    while (true) {
        self.task.suspendTask();
    }
}

fn pingTimer(xTimer: freertos.TimerHandle_t) callconv(.C) void {
    const self = freertos.Timer.getIdFromHandle(@This(), xTimer);

    _ = self.packet.preparePingPacket() catch unreachable;
}

const fw_update_topic = "zig/fw";
const conf_update_topic = "zig/conf";

pub fn publish(self: *@This(), topic: []const u8, payload: []const u8, qos: QoS, dup: bool, deadline: u32) !void {
    const packetId = try self.packet.preparePublishPacket(topic, payload, qos, dup, null);
    try self.qosQueue.add(packetId, qos, dup, false, topic, payload, deadline);
}

fn pubTimer(xTimer: freertos.TimerHandle_t) callconv(.C) void {
    const self = freertos.Timer.getIdFromHandle(@This(), xTimer);
    const payload = "test";
    self.publish("zig/pub", payload, .qos1, true, system.time.calculateDeadline(2000)) catch unreachable;
}

pub fn create(self: *@This()) void {
    self.task = freertos.Task.create(if (config.enable_mqtt) taskFunction else dummyTaskFunction, "mqtt", config.rtos_stack_depth_mqtt, self, config.rtos_prio_mqtt) catch unreachable;
    self.task.suspendTask();
    if (config.enable_mqtt) {
        self.connection = connection.init(.mqtt, authCallback, null);
        self.pingTimer = freertos.Timer.create("mqttPing", 60000, true, @This(), self, pingTimer) catch unreachable;
        self.pubTimer = freertos.Timer.create("pubTimer", 10000, true, @This(), self, pubTimer) catch unreachable;
        self.packet.create(&self.connection);
    }
}

/// Add
pub fn connect(self: *@This(), uri: std.Uri) !void {
    self.state = .connecting;
    var packetId: u16 = 0;

    self.connectionCounter += 1;

    try self.connection.create(uri.host.?, uri.port.?, null, connection.schemes.match(uri.scheme).?.getProtocol(), .psk);
    errdefer {
        self.connection.close() catch {};
    }

    _ = try self.packet.prepareConnectPacket(self.device_id[0..c.strlen(self.device_id)], null, null);

    try self.processSendQueue();

    // Wait for the connack
    if (0 == self.connection.waitRx(5)) {
        if (msgTypes.connack == self.packet.read(&rxBuffer)) {
            try self.packet.processConnAck(&rxBuffer);
        } else {
            //
        }
    }

    const subTopic = [_]MQTTString{ comptime initMQTTString(fw_update_topic), comptime initMQTTString(conf_update_topic) };
    const qos = [_]c_int{ 1, 2 };
    packetId = try self.packet.prepareSubscribePacket(2, @constCast(&subTopic[0]), @constCast(&qos[0]));

    try self.processSendQueue();

    // Wait for the connack
    if (0 == self.connection.waitRx(5)) {
        if (msgTypes.suback == self.packet.read(&rxBuffer)) {
            var maxCount: c_int = 1;
            var count: c_int = 0;
            var grantedQoSs: c_int = 0;

            // Deserialize
            if (packetId == try self.packet.deserializeSubAck(&rxBuffer, maxCount, &count, &grantedQoSs)) {
                _ = c.printf("Suback received\r\n");
            }
        }
    }

    self.state = .connected;
    self.pingTimer.changePeriod(60000, null) catch unreachable;
    self.pubTimer.start(null) catch unreachable;

    errdefer {
        self.state = .err;
    }
}

pub fn disconnect(self: *@This()) !void {
    self.pingTimer.stop(null) catch {};

    _ = try self.packet.prepareDisconnectPacket();
    try self.processSendQueue();

    defer {
        self.connection.close() catch {};
        _ = self.connection.ssl.deinit();
        self.disconnectionCounter += 1;
    }
}

pub fn resumeTask(self: *@This()) void {
    self.task.resumeTask();
}

pub fn getTaskHandle(self: *@This()) freertos.TaskHandle_t {
    return self.task.getHandle();
}

pub var service: @This() = init();
