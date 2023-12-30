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

/// MQTT OK return code
/// Used internally to check the return code of the MQTTPacket functions
const mqtt_ok: c_int = 1;

/// Message types from MQTTPacket
const msgTypes = enum(c_int) { err_msg = -1, try_again = 0, connect = c.CONNECT, connack = c.CONNACK, publish = c.PUBLISH, puback = c.PUBACK, pubrec = c.PUBREC, pubrel = c.PUBREL, pubcomp = c.PUBCOMP, subscribe = c.SUBSCRIBE, suback = c.SUBACK, unsubscribe = c.UNSUBSCRIBE, unsuback = c.UNSUBACK, pingreq = c.PINGREQ, pingresp = c.PINGRESP, disconnect = c.DISCONNECT };

const state = enum(i32) {
    err = -1,
    not_connected = 0,
    connecting,
    connected,
    disconnecting,
};

//// Mqtt Error codes
pub const mqtt_error = error{
    packetlen,
    enqueue_failed,
    dequeue_failed,
    connect_failed,
    send_failed,
    connack_failed,
    parse_failed,
    publish_parse_failed,
    subscribe_qos_topic_count_mismatch, // The number of topics and qos do not match
    qos_not_supported,

    qos_packet_not_found,
    pubrel_packet_not_found,
    qos_packet_timeout,
};

/// Publish response type
const packet_response = @This().packet.publish_response;

/// MQTT String initializer
const MQTTString_initializer = MQTTString{ .cstring = null, .lenstring = .{ .len = 0, .data = null } };

/// Manually translated initializer
const MQTTPacket_willOptions_initializer = c.MQTTPacket_willOptions{
    .struct_id = [_]u8{ 'M', 'Q', 'T', 'W' },
    .struct_version = 0,
    .topicName = MQTTString_initializer,
    .message = MQTTString_initializer,
    .retained = 0,
    .qos = 0,
};

/// Manually translated initializer
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

pingTimer: freertos.Timer,
pubTimer: freertos.Timer, // delete this!
task: freertos.StaticTask(config.rtos_stack_depth_mqtt),
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

/// Authentification callback for mbedTLS connections
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

/// Process and send the contents of the txQueue
fn processSendQueue(self: *@This()) !void {
    while (self.packet.txQueue.receive(&txBuffer, 0)) |buf| {
        _ = try self.connection.send(buf);
    }
}

/// Quality of Service
const QoS = enum(c_int) { qos0 = 0, qos1 = 1, qos2 = 2 };

/// Queued message
const QueuedMessage = struct {
    packetId: ?u16,
    qos: QoS,
    dup: bool,
    retained: bool,
    topic: []u8,
    payload: []u8,
    deadline: u32,
    packetType: msgTypes,
};

/// Queued Message Queue
const QueuedMessgeQueue = struct {
    /// Message Queue
    message: std.ArrayList(QueuedMessage),

    /// Comptime Initializer
    pub fn init(comptime allocator: std.mem.Allocator) @This() {
        return @This(){ .message = std.ArrayList(QueuedMessage).init(allocator) };
    }

    /// Add a message to the queue
    pub fn addPublish(self: *@This(), packetId: u16, qos: QoS, dup: bool, retained: bool, topic: []const u8, payload: []const u8, deadline: u32) !void {
        var new_payload = try self.message.allocator.alloc(u8, payload.len);
        var new_topic = try self.message.allocator.alloc(u8, topic.len);

        @memcpy(new_payload, payload);
        @memcpy(new_topic, topic);

        try self.message.append(QueuedMessage{ .packetId = packetId, .qos = qos, .dup = dup, .retained = retained, .topic = new_topic, .payload = new_payload, .deadline = deadline, .packetType = .publish });
    }

    /// Add a pubrel message to the queue
    pub fn addPubRel(self: *@This(), packetId: u16, dup: bool, deadline: u32) !void {
        try self.message.append(QueuedMessage{ .packetId = packetId, .qos = .qos2, .dup = dup, .retained = false, .topic = undefined, .payload = undefined, .deadline = deadline, .packetType = .pubrel });
    }

    /// Scan the queue for messages that match the packetId
    pub fn acknowledge(self: *@This(), packetId: u16, packetType: msgTypes) !bool {
        for (self.message.items, 0..) |*item, idx| {
            if (item.packetId) |id| {
                if ((id == packetId) and (item.packetType == packetType)) {
                    // Remove the message from the list
                    var msg = self.message.orderedRemove(idx);

                    if (msg.packetType == .publish) {
                        // Clear topic and payload content
                        @memset(msg.topic, 0);
                        @memset(msg.payload, 0);

                        self.message.allocator.free(msg.topic);
                        self.message.allocator.free(msg.payload);
                    } else {
                        _ = c.printf("pubrel removed: %d\r\n", msg.packetId.?);
                    }

                    return true;
                }
            }
        }
        return false;
    }

    /// Remove all messages that have expired
    pub fn prune(self: *@This()) ?QueuedMessage {
        for (self.message.items, 0..) |*item, idx| {
            if (item.deadline < system.time.now()) {
                return self.message.orderedRemove(idx);
            }
        }
        return null;
    }

    /// Find and remove a message from the queue
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
    txQueue: freertos.StaticMessageBuffer(1024),

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
        }, .packetIdState = packetId, .workBufferMutex = undefined, .txQueue = undefined };
    }

    pub fn create(self: *@This(), conn: *connection) void {
        self.transport.sck = @ptrCast(conn);

        self.workBufferMutex = freertos.Mutex.create() catch unreachable;

        self.txQueue.create() catch unreachable; // Create message buffer
    }

    /// Generate Packet ID
    /// Uses XorShift Algorithm to generate the packet id
    fn generatePacketId(self: *@This()) u16 {
        if (self.packetIdState != 0) {
            self.packetIdState ^= self.packetIdState << 7;
            self.packetIdState ^= self.packetIdState >> 9;
            self.packetIdState ^= self.packetIdState << 8;
        } else {
            self.packetIdState = 1;
        }

        return self.packetIdState;
    }

    /// Get function for the MQTTTransport
    fn getFn(ptr: ?*anyopaque, buf: [*c]u8, buf_len: c_int) callconv(.C) c_int {
        var self = @as(*connection, @ptrCast(@alignCast(ptr))); // Cast ptr as connection

        const ret = self.recieve(buf[0..@intCast(buf_len)]) catch {
            return @intFromEnum(msgTypes.err_msg);
        };

        return @intCast(ret.len);
    }

    /// Read from the transport layer
    /// Uses the non-blocking (NB) version of the MQTTPacket read function
    fn read(self: *@This(), buffer: []u8) msgTypes {
        self.transport.state = 0;
        return @as(msgTypes, @enumFromInt(c.MQTTPacket_readnb(@ptrCast(&buffer[0]), @intCast(buffer.len), &self.transport)));
    }

    /// Deserialize a publish packet from buffer and converts it into a `publish_response`
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

    fn deserializePuback(self: *@This(), buffer: []u8) !struct { packetId: u16, dup: bool } {
        _ = self;
        var packetId: u16 = undefined;
        var dup: u8 = undefined;
        var packetType: u8 = undefined;

        if (mqtt_ok != c.MQTTDeserialize_ack(&packetType, &dup, &packetId, @ptrCast(&buffer[0]), @intCast(buffer.len))) {
            return mqtt_error.parse_failed;
        }
        if (packetType != c.PUBACK) {
            return mqtt_error.parse_failed; // Not actually a puback when expected
        }
        return .{ .packetId = packetId, .dup = (if (dup == 0) false else true) };
    }

    /// Deserialize a pubrel packet from buffer
    fn deserializePubrel(self: *@This(), buffer: []u8) !struct { packetId: u16, dup: bool } {
        _ = self;
        var packetId: u16 = undefined;
        var dup: u8 = undefined;
        var packetType: u8 = undefined;

        if (mqtt_ok != c.MQTTDeserialize_ack(&packetType, &dup, &packetId, @ptrCast(&buffer[0]), @intCast(buffer.len))) {
            return mqtt_error.parse_failed;
        }
        if (packetType != c.PUBREL) {
            return mqtt_error.parse_failed; // Not actually a pubrel when expected
        }
        return .{ .packetId = packetId, .dup = (if (dup == 0) false else true) };
    }

    /// Deserialize a pubrec packet from buffer
    fn deserializePubrec(self: *@This(), buffer: []u8) !u16 {
        _ = self;
        var packetId: u16 = undefined;
        var dup: u8 = undefined;
        var packetType: u8 = undefined;

        if (mqtt_ok != c.MQTTDeserialize_ack(&packetType, &dup, &packetId, @ptrCast(&buffer[0]), @intCast(buffer.len))) {
            return mqtt_error.parse_failed;
        }
        if (packetType != c.PUBREC) {
            return mqtt_error.parse_failed; // Not actually a pubrec when expected
        }

        return packetId;
    }

    /// Deserialize a pubcomp packet from buffer
    fn deserializePubcomp(self: *@This(), buffer: []u8) !u16 {
        _ = self;
        var packetId: u16 = undefined;
        var dup: u8 = undefined;
        var packetType: u8 = undefined;

        if (mqtt_ok != c.MQTTDeserialize_ack(&packetType, &dup, &packetId, @ptrCast(&buffer[0]), @intCast(buffer.len))) {
            return mqtt_error.parse_failed;
        }
        if (packetType != c.PUBCOMP) {
            return mqtt_error.parse_failed; // Not actually a pubcomp when expected
        }

        return packetId;
    }

    /// Deserialize a suback packet from buffer
    fn deserializeSubAck(self: *@This(), buffer: []u8, qos: []QoS) !struct { packetId: u16, qos: []QoS } {
        _ = self;
        var packetId: u16 = undefined;
        var count: c_int = 0;

        if (mqtt_ok != c.MQTTDeserialize_suback(&packetId, @intCast(qos.len), &count, @ptrCast(qos.ptr), @ptrCast(buffer.ptr), @intCast(buffer.len))) {
            return mqtt_error.parse_failed;
        }
        return .{ .packetId = packetId, .qos = qos[0..@as(usize, @intCast(count))] };
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

    /// Prepare a connect packet
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

    /// Prepare the puback packet and sends to TX Queue
    fn preparePubAckPacket(self: *@This(), packetId: u16) !u16 {
        _ = try self.workBufferMutex.take(null);
        defer self.workBufferMutex.give() catch {};

        return self.sendtoTxQueue(try serializeCheck(c.MQTTSerialize_puback(@ptrCast(&workBuffer[0]), workBuffer.len, packetId)), packetId);
    }

    /// Prepare the pubrec packet and sends to TX Queue
    fn preparePubRecPacket(self: *@This(), packetId: u16) !u16 {
        _ = try self.workBufferMutex.take(null);
        defer self.workBufferMutex.give() catch {};

        return self.sendtoTxQueue(try serializeCheck(c.MQTTSerialize_pubrec(@ptrCast(&workBuffer[0]), workBuffer.len, packetId)), packetId);
    }

    /// Prepare the pubcomp packet and sends to TX Queue
    fn preparePubCompPacket(self: *@This(), packetId: u16) !u16 {
        _ = try self.workBufferMutex.take(null);
        defer self.workBufferMutex.give() catch {};

        return self.sendtoTxQueue(try serializeCheck(c.MQTTSerialize_pubcomp(@ptrCast(&workBuffer[0]), workBuffer.len, packetId)), packetId);
    }

    /// Prepare the `pubrel` packet and sends to TX Queue
    fn preparePubRelPacket(self: *@This(), packetId: u16, dup: bool) !u16 {
        _ = try self.workBufferMutex.take(null);
        defer self.workBufferMutex.give() catch {};

        return self.sendtoTxQueue(try serializeCheck(c.MQTTSerialize_pubrel(@ptrCast(&workBuffer[0]), workBuffer.len, @intFromBool(dup), packetId)), packetId);
    }

    /// Prepare the subscribe packet and sends to TX Queue
    /// Both topicFilter and qos must have the same length
    fn prepareSubscribePacket(self: *@This(), topicFilter: []MQTTString, qos: []QoS) !u16 {
        _ = try self.workBufferMutex.take(null);
        defer self.workBufferMutex.give() catch {};

        const count = if (topicFilter.len == qos.len) topicFilter.len else return mqtt_error.subscribe_qos_topic_count_mismatch;
        const packetId = self.generatePacketId();
        const dup: u8 = 0;

        return self.sendtoTxQueue(try serializeCheck(c.MQTTSerialize_subscribe(@ptrCast(&workBuffer[0]), workBuffer.len, dup, packetId, @intCast(count), topicFilter.ptr, @ptrCast(qos.ptr))), packetId);
    }

    /// Prepare a ping packet and sends to TX Queue
    fn preparePingPacket(self: *@This()) !u16 {
        _ = try self.workBufferMutex.take(null);
        defer self.workBufferMutex.give() catch {};

        return self.sendtoTxQueue(try serializeCheck(c.MQTTSerialize_pingreq(@ptrCast(&workBuffer[0]), workBuffer.len)), null);
    }

    /// prepare a disconnect packet and sends to TX Queue
    fn prepareDisconnectPacket(self: *@This()) !u16 {
        _ = try self.workBufferMutex.take(null);
        defer self.workBufferMutex.give() catch {};

        return self.sendtoTxQueue(try serializeCheck(c.MQTTSerialize_disconnect(@ptrCast(&workBuffer[0]), workBuffer.len)), null);
    }

    /// Prepare a publish packet and sends to TX Queue
    pub fn preparePublishPacket(self: *@This(), topic: []const u8, payload: []const u8, qos: QoS, dup: bool, packetId: ?u16) !u16 {
        _ = try self.workBufferMutex.take(null);
        defer self.workBufferMutex.give() catch {};

        const topic_name = initMQTTString(topic);
        const retain: u8 = 0;
        const id = packetId orelse self.generatePacketId();

        return self.sendtoTxQueue(try serializeCheck(c.MQTTSerialize_publish(&workBuffer[0], workBuffer.len, if (dup) 1 else 0, @intFromEnum(qos), retain, id, topic_name, @constCast(payload.ptr), @intCast(payload.len))), id);
    }

    /// Process the connack packet
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
        // process the QoS1 tx queue
        while (self.qosQueue.prune()) |msg| {
            switch (msg.packetType) {
                .publish => {
                    try self.publish(msg.topic, msg.payload, msg.qos, true, msg.packetId, system.time.calculateDeadline(2000));
                },
                .pubrel => {
                    const ret = try self.packet.preparePubRelPacket(msg.packetId.?, true);
                    try self.qosQueue.addPubRel(ret, true, system.time.calculateDeadline(2000));
                },
                else => {},
            }
        }

        // process the QoS2 rx queue
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
                        try self.qos2Queue.addPublish(packetId, publish_response.qos, publish_response.dup, publish_response.retained, publish_response.topic, publish_response.payload, system.time.calculateDeadline(2000));
                    }
                },
                .puback => {
                    // Response for Client pub qos1
                    const resp = try self.packet.deserializePuback(&rxBuffer);

                    // Process the puback packet
                    // Acknowledges a QoS1 publish message
                    if (false == try self.qosQueue.acknowledge(resp.packetId, .publish)) {
                        return mqtt_error.qos_packet_not_found;
                    }
                },
                .pingresp => {
                    _ = c.printf("pingresp!\r\n");
                },
                .connack => try self.packet.processConnAck(&rxBuffer),
                .connect, .subscribe, .disconnect, .unsubscribe, .pingreq => break, // Broker messages
                .suback => {
                    var grantedQoSs: [2]QoS = .{ .qos0, .qos0 };

                    // Deserialize
                    const res = try self.packet.deserializeSubAck(&rxBuffer, &grantedQoSs);
                    _ = res;
                }, // To-do: process the sub-ack
                .unsuback => {
                    // currently unsuported
                },
                .pubrec => {
                    // generate pubrel package
                    // pubrec does not have a duplicate
                    const rx_packetId = try self.packet.deserializePubrec(&rxBuffer);

                    // Remove the publish message from the queue
                    if (false == try self.qosQueue.acknowledge(rx_packetId, .publish)) {
                        return mqtt_error.qos_packet_not_found;
                    }

                    // Search for duplicate packets in queue
                    const dup = try self.qosQueue.acknowledge(rx_packetId, .pubrel);

                    // Prepare the pubrel packet
                    const ret = try self.packet.preparePubRelPacket(rx_packetId, dup);

                    // Add to tx queue
                    try self.qosQueue.addPubRel(ret, false, system.time.calculateDeadline(2000));
                },
                .pubrel => {
                    // Recieved pubrel from broker
                    const ret = try self.packet.deserializePubrel(&rxBuffer);

                    if (self.qos2Queue.findAndRemove(ret.packetId)) |msg| {

                        // Send the pubcomp packet to the broker
                        _ = try self.packet.preparePubCompPacket(ret.packetId);
                        try self.processSendQueue();

                        var buf: [64]u8 = undefined;
                        @memset(&buf, 0);

                        var s = try std.fmt.bufPrint(&buf, "Pubrel recieved: {d}, {d} {s} {s}\r\n", .{ ret.packetId, @intFromEnum(msg.qos), msg.topic, msg.payload });

                        _ = c.printf("%s", s.ptr);

                        @memset(msg.topic, 0);
                        @memset(msg.payload, 0);

                        self.qos2Queue.message.allocator.free(msg.topic);
                        self.qos2Queue.message.allocator.free(msg.payload);
                    } else {
                        // No message found in the queue
                    }
                },
                .pubcomp => {
                    // publish complete recieved from broker
                    const resp = try self.packet.deserializePubcomp(&rxBuffer);

                    // Look for the pubrel package in the queue
                    if (false == try self.qosQueue.acknowledge(resp, .pubrel)) {
                        return mqtt_error.pubrel_packet_not_found;
                    } else {
                        _ = c.printf("pubcomp received for packetId\r\n", resp);
                    }
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

        self.loop(uri) catch |err| {
            if (err == connection.connection_error.create_error) {
                self.task.delayTask(1000);
            }
            _ = c.printf("Disconnected... reconnect: %d. %d \r\n", self.connectionCounter, @intFromError(err));
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

pub fn publish(self: *@This(), topic: []const u8, payload: []const u8, qos: QoS, dup: bool, packetId: ?u16, deadline: u32) !void {
    const ret = try self.packet.preparePublishPacket(topic, payload, qos, dup, packetId);
    try self.qosQueue.addPublish(ret, qos, dup, false, topic, payload, deadline);
}

fn pubTimer(xTimer: freertos.TimerHandle_t) callconv(.C) void {
    const self = freertos.Timer.getIdFromHandle(@This(), xTimer);
    const payload = "test";
    self.publish("zig/pub", payload, .qos2, false, null, system.time.calculateDeadline(2000)) catch unreachable;
}

pub fn create(self: *@This()) void {
    self.task.create(if (config.enable_mqtt) taskFunction else dummyTaskFunction, "mqtt", self, config.rtos_prio_mqtt) catch unreachable;
    self.task.suspendTask();
    if (config.enable_mqtt) {
        self.connection = connection.init(.mqtt, authCallback, null);
        self.pingTimer = freertos.Timer.create("mqttPing", 60000, true, @This(), self, pingTimer) catch unreachable;
        self.pubTimer = freertos.Timer.create("pubTimer", 10000, true, @This(), self, pubTimer) catch unreachable;
        self.packet.create(&self.connection);
    }
}

/// Connect to the MQTT broker
pub fn connect(self: *@This(), uri: std.Uri) !void {
    self.state = .connecting;
    var packetId: u16 = 0;

    self.connectionCounter += 1;
    errdefer {
        self.state = .err;
        self.connection.close() catch {};
    }

    try self.connection.create(uri.host.?, uri.port.?, null, connection.schemes.match(uri.scheme).?.getProtocol(), .psk);

    _ = try self.packet.prepareConnectPacket(self.device_id[0..c.strlen(self.device_id)], null, null);

    try self.processSendQueue();

    // Wait for the connack
    if (0 == self.connection.waitRx(5)) {
        if (msgTypes.connack == self.packet.read(&rxBuffer)) {
            try self.packet.processConnAck(&rxBuffer);
        } else {
            return mqtt_error.connect_failed;
        }
    }

    const subTopic = [_]MQTTString{ comptime initMQTTString(fw_update_topic), comptime initMQTTString(conf_update_topic) };
    const qos = [_]QoS{ QoS.qos1, QoS.qos2 };
    packetId = try self.packet.prepareSubscribePacket(@constCast(&subTopic), @constCast(&qos));

    try self.processSendQueue();

    // Wait for the connack
    if (0 == self.connection.waitRx(5)) {
        if (msgTypes.suback == self.packet.read(&rxBuffer)) {
            var grantedQoSs: [2]QoS = .{ .qos0, .qos0 };

            // Deserialize
            const res = try self.packet.deserializeSubAck(&rxBuffer, &grantedQoSs);
            if (res.packetId == packetId) {
                _ = c.printf("Suback received: %d..%d,%d\r\n", res.qos.len, @intFromEnum(res.qos[0]), @intFromEnum(res.qos[1]));
            }
        }
    }

    self.state = .connected;
    self.pingTimer.changePeriod(60000, null) catch unreachable;
    self.pubTimer.start(null) catch unreachable;
}

pub fn disconnect(self: *@This()) !void {
    self.pingTimer.stop(null) catch {};
    defer {
        self.connection.close() catch {};
        self.disconnectionCounter += 1;
    }

    _ = try self.packet.prepareDisconnectPacket();
    try self.processSendQueue();
}

pub fn resumeTask(self: *@This()) void {
    self.task.resumeTask();
}

pub fn getTaskHandle(self: *@This()) freertos.TaskHandle_t {
    return self.task.getHandle();
}

pub var service: @This() = init();
