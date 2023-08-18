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

const MQTTSerialize_ack = c.MQTTSerialize_ack;
const MQTTDeserialize_ack = c.MQTTDeserialize_ack;
const MQTTPacket_len = c.MQTTPacket_len;
const MQTTPacket_read = c.MQTTPacket_read;

const keepAliveInterval_s = 60;
const keepAliveInterval_ms = 1000 * keepAliveInterval_s;

const MQTTTransport = c.MQTTTransport;

const MQTTString = c.MQTTString;
const MQTTLenString = c.MQTTLenString;

const msgTypes = enum(c_int) { err_msg = -1, try_again = 0, connect = c.CONNECT, connack = c.CONNACK, publish = c.PUBLISH, puback = c.PUBACK, pubrec = c.PUBREC, pubrel = c.PUBREL, pubcomp = c.PUBCOMP, subscribe = c.SUBSCRIBE, suback = c.SUBACK, unsubscribe = c.UNSUBSCRIBE, unsuback = c.UNSUBACK, pingreq = c.PINGREQ, pingresp = c.PINGRESP, disconnect = c.DISCONNECT };

const state = enum(i32) {
    err = -1,
    not_connected = 0,
    connecting,
    connected,
    disconnecting,
};

const mqtt_error = error{
    packetlen,
    enqueue_failed,
    dequeue_failed,
    connect_failed,
    send_failed,
    connack_failed,
    parse_failed,
};

pub const MQTTString_initializer = MQTTString{ .cstring = null, .lenstring = .{ .len = 0, .data = null } };

// Manually translated initializer
pub const MQTTPacket_willOptions_initializer = c.MQTTPacket_willOptions{
    .struct_id = [_]u8{ 'M', 'Q', 'T', 'W' },
    .struct_version = 0,
    .topicName = MQTTString_initializer,
    .message = MQTTString_initializer,
    .retained = 0,
    .qos = 0,
};

// Manually translated initializer
pub const MQTTPacket_connectData_initializer = c.MQTTPacket_connectData{
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
uri_string: [*c]u8,
device_id: [*c]u8,

var txBuffer: [256]u8 align(@alignOf(u32)) = undefined;
var rxBuffer: [512]u8 align(@alignOf(u32)) = undefined;

fn init() @This() {
    return @This(){ .connection = undefined, .connectionCounter = 0, .disconnectionCounter = 0, .rxQueue = undefined, .pingTimer = undefined, .pubTimer = undefined, .task = undefined, .state = .not_connected, .packet = packet.init(0x5555), .uri_string = undefined, .device_id = undefined };
}

fn authCallback(self: *connection.mbedtls, security_mode: connection.security_mode) i32 {
    var ret: i32 = 0;

    if (security_mode == .psk) {
        @memset(&self.psk, 0);
        ret = connection.mbedtls.c.mbedtls_base64_decode(&self.psk, self.psk.len, &self.psk_len, config.c.config_get_mqtt_psk_key(), config.c.strlen(config.c.config_get_mqtt_psk_key()));
        if (ret == 0) {
            ret = connection.mbedtls.c.mbedtls_ssl_conf_psk(&self.config, &self.psk, self.psk_len, config.c.config_get_mqtt_psk_id(), config.c.strlen(config.c.config_get_mqtt_psk_id()));
        }
    }

    return 0;
}

fn processSendQueue(self: *@This()) !void {
    var ret: i32 = 1;

    while (ret > 0) {
        var buf_len = packet.txQueue.receive(@ptrCast(&txBuffer[0]), txBuffer.len, 0);
        if (buf_len != 0) {
            ret = self.connection.send(&txBuffer[0], buf_len);
        } else {
            ret = 0;
        }
    }

    if (0 > ret) {
        return mqtt_error.send_failed;
    }
}

const packet = struct {
    transport: MQTTTransport,
    packetIdState: u16,

    var workBuffer: [256]u8 align(@alignOf(u32)) = undefined;
    var workBufferMutex: freertos.Semaphore = undefined;
    var txQueue: freertos.MessageBuffer = undefined;

    pub fn init(comptime packetId: u16) @This() {
        return @This(){ .transport = .{
            .getfn = getFn,
            .sck = undefined,
            .multiplier = undefined,
            .rem_len = undefined,
            .len = undefined,
            .state = undefined,
        }, .packetIdState = packetId };
    }

    pub fn create(self: *@This(), conn: *connection) void {
        self.transport.sck = @ptrCast(conn);

        workBufferMutex.createMutex() catch unreachable;
        txQueue.create(1024); // Create message buffer
        @memset(&workBuffer, 0);
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
        var self = @as(*connection, @ptrCast(@alignCast(ptr)));

        var ret = self.recieve(buf, @intCast(buf_len));
        if (ret < 0) ret = @intFromEnum(msgTypes.err_msg);

        return ret;
    }

    fn read(self: *@This(), buffer: []u8) msgTypes {
        self.transport.state = 0;
        return @as(msgTypes, @enumFromInt(c.MQTTPacket_readnb(@ptrCast(&buffer[0]), @intCast(buffer.len), &self.transport)));
    }

    fn deserializeAck(self: *@This(), buffer: []u8, packetType: *u8, dup: *u8) !u16 {
        _ = self;
        var packetId: u16 = undefined;

        if (1 != c.MQTTDeserialize_ack(packetType, dup, &packetId, @ptrCast(&buffer[0]), @intCast(buffer.len))) {
            // return
        }
        return packetId;
    }

    fn deserializeSubAck(self: *@This(), buffer: []u8, maxCount: c_int, count: *c_int, grantedQoSs: *c_int) !u16 {
        _ = self;
        var packetId: u16 = undefined;
        if (1 != c.MQTTDeserialize_suback(&packetId, maxCount, count, grantedQoSs, @ptrCast(&buffer[0]), @intCast(buffer.len))) {
            return mqtt_error.parse_failed;
        }
        return packetId;
    }

    fn prepareConnectPacket(self: *@This(), clientID: [*:0]const u8, username: ?[*:0]const u8, password: ?[*:0]const u8) !u16 {
        var connectPacket = MQTTPacket_connectData_initializer;
        connectPacket.clientID.cstring = @constCast(clientID);

        if (username != null) {
            connectPacket.username = MQTTString{ .cstring = @constCast(username), .lenstring = .{ .len = 0, .data = null } };
        }

        if (password != null) {
            connectPacket.password = MQTTString{ .cstring = @constCast(password), .lenstring = .{ .len = 0, .data = null } };
        }

        connectPacket.keepAliveInterval = 400;

        _ = workBufferMutex.take(null);

        var packetLen: isize = c.MQTTSerialize_connect(@ptrCast(&workBuffer[0]), workBuffer.len, &connectPacket);

        return self.sendtoTxQueue(packetLen, null);
    }

    fn preparePubAckPacket(self: *@This(), packetId: u16) !u16 {
        _ = workBufferMutex.take(null);

        var packetLen: isize = c.MQTTSerialize_puback(@ptrCast(&workBuffer[0]), workBuffer.len, packetId);

        return self.sendtoTxQueue(packetLen, null);
    }

    fn preparePubCompPacket(self: *@This(), packetId: u16) !u16 {
        _ = workBufferMutex.take(null);

        var packetLen: isize = c.MQTTSerialize_pubcomp(@ptrCast(&workBuffer[0]), workBuffer.len, packetId);

        return self.sendtoTxQueue(packetLen, packetId);
    }

    fn preparePubRelPacket(self: *@This(), dup: u8, packetId: u16) !u16 {
        _ = workBufferMutex.take(null);

        var packetLen: isize = c.MQTTSerialize_pubrel(@ptrCast(&workBuffer[0]), workBuffer.len, dup, packetId);

        return self.sendtoTxQueue(packetLen, packetId);
    }

    fn prepareSubscribePacket(self: *@This(), count: usize, topicFilter: *MQTTString, qos: *c_int) !u16 {
        _ = workBufferMutex.take(null);

        var packetId = self.generatePacketId();
        var dup: u8 = 0;

        var packetLen: isize = c.MQTTSerialize_subscribe(@ptrCast(&workBuffer[0]), workBuffer.len, dup, packetId, @intCast(count), topicFilter, qos);

        return self.sendtoTxQueue(packetLen, packetId);
    }

    fn preparePingPacket(self: *@This()) !u16 {
        _ = workBufferMutex.take(null);

        var packetLen: isize = c.MQTTSerialize_pingreq(@ptrCast(&workBuffer[0]), workBuffer.len);

        return self.sendtoTxQueue(packetLen, null);
    }

    fn prepareDisconnectPacket(self: *@This()) !u16 {
        _ = workBufferMutex.take(null);

        var packetLen: isize = c.MQTTSerialize_disconnect(@ptrCast(&workBuffer[0]), workBuffer.len);

        return self.sendtoTxQueue(packetLen, null);
    }

    pub fn preparePublishPacket(self: *@This(), topic: [*:0]const u8, payload: [*:0]const u8, payload_len: usize, qos: u8) !u16 {
        _ = workBufferMutex.take(null);

        var topic_name = MQTTString{ .cstring = @constCast(topic), .lenstring = .{ .len = 0, .data = null } };
        var dup: u8 = 0;
        var retain: u8 = 0;
        var packetId = self.generatePacketId();

        var packetLen: isize = c.MQTTSerialize_publish(&workBuffer[0], workBuffer.len, dup, qos, retain, packetId, topic_name, @ptrCast(@constCast(payload)), @intCast(payload_len));

        return self.sendtoTxQueue(packetLen, packetId);
    }

    pub fn processConnAck(self: *@This(), buffer: []u8) !void {
        _ = self;
        var sessionPresent: u8 = undefined;
        var connack_rc: u8 = undefined;

        if (1 == c.MQTTDeserialize_connack(&sessionPresent, &connack_rc, @ptrCast(&buffer[0]), @intCast(buffer.len))) {
            if (connack_rc != c.MQTT_CONNECTION_ACCEPTED) {
                return mqtt_error.connack_failed;
            }
        } else {
            return mqtt_error.parse_failed;
        }
    }

    fn sendtoTxQueue(self: *@This(), packetLen: isize, packetId: ?u16) !u16 {
        _ = self;

        if (packetLen <= 0) {
            return mqtt_error.packetlen;
        } else if (!(@as(usize, @intCast(packetLen)) == txQueue.send(@ptrCast(&workBuffer[0]), @intCast(packetLen), null))) {
            return mqtt_error.enqueue_failed;
        }
        _ = workBufferMutex.give();

        return @intCast(packetId orelse 0);
    }
};

fn loop(self: *@This(), uri: std.Uri) !void {
    try self.connect(uri);
    defer self.disconnect() catch {};

    while (true) {
        try self.processSendQueue();

        if (0 == self.connection.waitRx(1)) {
            var readRet = self.packet.read(&rxBuffer);
            if (readRet == msgTypes.try_again) {} else if ((readRet == msgTypes.connect) or (readRet == msgTypes.subscribe)) {
                break;
            } else if (readRet == msgTypes.connack) {
                try self.packet.processConnAck(&rxBuffer);
            } else if (readRet == msgTypes.publish) {
                var dup: u8 = undefined;
                var qos: c_int = undefined;
                var retained: u8 = undefined;

                var topicName = MQTTString_initializer;
                var payload: [*c]u8 = undefined;
                var payloadLen: isize = undefined;

                var rx_packetId: u16 = undefined;

                if (1 == c.MQTTDeserialize_publish(&dup, &qos, &retained, &rx_packetId, &topicName, &payload, &payloadLen, @ptrCast(&rxBuffer[0]), rxBuffer.len)) {
                    var packetId = switch (qos) {
                        0 => 0,
                        1 => try self.packet.preparePubAckPacket(rx_packetId),
                        else => 0,
                    };

                    _ = packetId;
                    _ = c.printf("publish recieved\r\n");
                } else {
                    break; // error state
                }
            } else if (readRet == msgTypes.puback) {
                // Response for Client pub qos1
                var packetType: u8 = undefined;
                var dup: u8 = undefined;
                var rx_packetId: u16 = undefined;

                rx_packetId = self.packet.deserializeAck(&rxBuffer, &packetType, &dup) catch {
                    1;
                };
            } else if (readRet == msgTypes.pubrec) {
                break;
            } else if (readRet == msgTypes.pubrel) {
                break;
            } else if (readRet == msgTypes.pubcomp) {
                break;
            } else if (readRet == msgTypes.suback) {
                break;
                // MQTTDeserialize_suback(&submsgid, 1, &subcount, &granted_qos, buf, buflen);
            } else if (readRet == msgTypes.unsubscribe) {
                break;
            } else if (readRet == msgTypes.unsuback) {
                // unsuback
            } else if (readRet == msgTypes.pingreq) {
                break;
            } else if (readRet == msgTypes.pingresp) {
                // process the ping response
                _ = c.printf("pingresp!");
            } else if (readRet == msgTypes.disconnect) {
                break;
            } else {
                break;
                // Generic error
            }
        } else {
            _ = c.printf("mqtt: %d\r\n", self.task.getStackHighWaterMark());
        }
    }
}

fn taskFunction(pvParameters: ?*anyopaque) callconv(.C) void {
    var self = freertos.getAndCastPvParameters(@This(), pvParameters);

    @memset(&txBuffer, 0);
    @memset(&rxBuffer, 0);
    @memset(&packet.workBuffer, 0);

    self.connectionCounter = 0;
    self.disconnectionCounter = 0;

    self.uri_string = c.config_get_mqtt_url();
    self.device_id = c.config_get_mqtt_device_id();

    // Get to connect
    while (true) {
        var uri = std.Uri.parse(self.uri_string[0..c.strlen(self.uri_string)]) catch unreachable;

        self.loop(uri) catch {};

        _ = c.printf("Disconnected... reconnect: %d", self.connectionCounter);
    }

    // Go to disconnect phase
}

fn dummyTaskFunction(pvParameters: ?*anyopaque) callconv(.C) void {
    var self = freertos.getAndCastPvParameters(@This(), pvParameters);

    while (true) {
        self.task.suspendTask();
    }
}

fn pingTimer(xTimer: freertos.TimerHandle_t) callconv(.C) void {
    var self = freertos.getAndCastTimerID(@This(), xTimer);

    _ = self.packet.preparePingPacket() catch {};
}

fn pubTimer(xTimer: freertos.TimerHandle_t) callconv(.C) void {
    var self = freertos.getAndCastTimerID(@This(), xTimer);
    const payload = "test";
    _ = self.packet.preparePublishPacket("zig/pub", payload, payload.len, 1) catch {};
}

pub fn create(self: *@This()) void {
    self.task.create(if (config.enable_mqtt) taskFunction else dummyTaskFunction, "mqtt", config.rtos_stack_depth_mqtt, self, config.rtos_prio_mqtt) catch unreachable;
    self.task.suspendTask();
    if (config.enable_mqtt) {
        self.connection = connection.init(.mqtt, authCallback);
        self.pingTimer.create("mqttPing", 60000, freertos.pdTRUE, self, pingTimer) catch unreachable;
        self.pubTimer.create("pubTimer", 10000, freertos.pdTRUE, self, pubTimer) catch unreachable;
        self.packet.create(&self.connection);
    }
}

/// Add
pub fn connect(self: *@This(), uri: std.Uri) !void {
    self.state = .connecting;
    //var connRet: i32 = 0;
    var packetId: u16 = 0;

    self.connectionCounter += 1;

    try self.connection.create(uri.host.?, uri.port.?, null, connection.schemes.stringmap.get(uri.scheme).?.getProtocol(), .psk);
    errdefer {
        self.connection.close() catch {};
    }

    _ = try self.packet.prepareConnectPacket(self.device_id, null, null);

    try self.processSendQueue();

    // Wait for the connack
    if (0 == self.connection.waitRx(5)) {
        if (msgTypes.connack == self.packet.read(&rxBuffer)) {
            try self.packet.processConnAck(&rxBuffer);
        } else {
            //
        }
    }

    var subTopic = MQTTString{ .cstring = @constCast("zig/test"), .lenstring = .{ .len = 0, .data = null } };
    var qos: c_int = 1;
    packetId = try self.packet.prepareSubscribePacket(1, &subTopic, &qos);

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
