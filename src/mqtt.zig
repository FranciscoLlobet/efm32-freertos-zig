const std = @import("std");
const freertos = @import("freertos.zig");
const config = @import("config.zig");
const system = @import("system.zig");
const connection = @import("connection.zig");

const c = @cImport({
    @cDefine("MQTT_CLIENT", "1");
    @cInclude("MQTTPacket.h");
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
rxQueue: freertos.Queue,
txQueue: freertos.MessageBuffer,
pingTimer: freertos.Timer,
pubTimer: freertos.Timer, // delete this!
workBufferMutex: freertos.Semaphore,
task: freertos.Task,
state: state,
packetIdState: u16,
transport: MQTTTransport,
connack_rc: u8, // Last CONNACK received
workBuffer: [256]u8 align(@alignOf(u32)),
txBuffer: [256]u8 align(@alignOf(u32)),
rxBuffer: [512]u8 align(@alignOf(u32)),

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
    var self = @as(*@This(), @ptrCast(@alignCast(ptr)));

    var ret = self.connection.recieve(buf, @intCast(buf_len));
    if (ret < 0) ret = @intFromEnum(msgTypes.err_msg);

    return ret;
}

fn processSendQueue(self: *@This()) i32 {
    var ret: i32 = 1;

    while (ret > 0) {
        var buf_len = self.txQueue.receive(@ptrCast(&self.txBuffer[0]), self.txBuffer.len, 0);
        if (buf_len != 0) {
            ret = self.connection.send(&self.txBuffer[0], buf_len);
        } else {
            ret = 0;
        }
    }
    return ret;
}

fn read(self: *@This()) msgTypes {
    self.transport.state = 0;
    return @as(msgTypes, @enumFromInt(c.MQTTPacket_readnb(@ptrCast(&self.rxBuffer[0]), self.rxBuffer.len, &self.transport)));
}

fn taskFunction(pvParameters: ?*anyopaque) callconv(.C) void {
    var self = freertos.getAndCastPvParameters(@This(), pvParameters);

    self.connectionCounter = 0;

    // Get to connect
    while (true) {
        var packetId: i32 = undefined;
        var ret = self.connect();

        if (ret == 0) {
            self.connectionCounter += 1;
            // connection time
        }

        while (ret == 0) {
            ret = self.processSendQueue();

            if (0 == self.connection.waitRx(1)) {
                var readRet = self.read();
                if (readRet == msgTypes.try_again) {
                    ret = 0;
                } else if ((readRet == msgTypes.connect) or (readRet == msgTypes.subscribe)) {
                    ret = -1;
                } else if (readRet == msgTypes.connack) {
                    ret = self.processConnAck();
                } else if (readRet == msgTypes.publish) {
                    var dup: u8 = undefined;
                    var qos: c_int = undefined;
                    var retained: u8 = undefined;
                    var rx_packetId: u16 = undefined;
                    var topicName = MQTTString_initializer;
                    var payload: [*c]u8 = undefined;
                    var payloadLen: isize = undefined;

                    if (1 == c.MQTTDeserialize_publish(&dup, &qos, &retained, &rx_packetId, &topicName, &payload, &payloadLen, @ptrCast(&self.rxBuffer[0]), self.rxBuffer.len)) {
                        packetId = switch (qos) {
                            0 => 0,
                            1 => self.preparePubAckPacket(rx_packetId),
                            else => -1,
                        };
                        if (packetId >= 0) {
                            _ = c.printf("publish recieved\r\n");
                            ret = 0;
                        } else {
                            ret = -1;
                        }
                    } else {
                        ret = -1; // error state
                    }
                } else if (readRet == msgTypes.puback) {
                    // Response for Client pub qos1
                    var packetType: u8 = undefined;
                    var dup: u8 = undefined;
                    var rx_packetId: u16 = undefined;

                    if (1 == c.MQTTDeserialize_ack(&packetType, &dup, &rx_packetId, @ptrCast(&self.rxBuffer[0]), self.rxBuffer.len)) {
                        // notify application about received ack ?
                        _ = c.printf("puback recieved\r\n");
                        ret = 0;
                    } else {
                        ret = -1;
                    }
                } else if (readRet == msgTypes.pubrec) {
                    ret = -1;
                } else if (readRet == msgTypes.pubrel) {
                    ret = -1;
                } else if (readRet == msgTypes.pubcomp) {
                    ret = -1;
                } else if (readRet == msgTypes.suback) {
                    ret = 0; // The suback
                    // MQTTDeserialize_suback(&submsgid, 1, &subcount, &granted_qos, buf, buflen);
                } else if (readRet == msgTypes.unsubscribe) {
                    ret = -1;
                } else if (readRet == msgTypes.unsuback) {
                    ret = 0;
                } else if (readRet == msgTypes.pingreq) {
                    ret = -1;
                } else if (readRet == msgTypes.pingresp) {
                    // process the ping response
                    ret = 0;
                    _ = c.printf("pingresp!");
                } else if (readRet == msgTypes.disconnect) {
                    ret = -1;
                } else {
                    ret = -1;
                    // Generic error
                }
            } else {
                _ = c.printf("mqtt: %d\r\n", self.task.getStackHighWaterMark());
            }
        }

        ret = self.disconnect();

        _ = c.printf("Disconnected... reconnect: %d", ret, self.connectionCounter);
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

    _ = self.preparePingPacket();
}

fn pubTimer(xTimer: freertos.TimerHandle_t) callconv(.C) void {
    var self = freertos.getAndCastTimerID(@This(), xTimer);
    const payload = "test";
    _ = self.preparePublishPacket("zig/pub", payload, payload.len, 1);
}

pub fn create(self: *@This()) void {
    self.task.create(if (config.enable_mqtt) taskFunction else dummyTaskFunction, "mqtt", config.rtos_stack_depth_mqtt, self, config.rtos_prio_mqtt) catch unreachable;
    self.task.suspendTask();
    if (config.enable_mqtt) {
        self.txQueue.create(1024); // Create message buffer
        self.connection = connection.init(.mqtt);
        self.pingTimer.create("mqttPing", 60000, freertos.pdTRUE, self, pingTimer) catch unreachable;
        self.pubTimer.create("pubTimer", 1000, freertos.pdTRUE, self, pubTimer) catch unreachable;
        self.workBufferMutex.createMutex() catch unreachable;
    }
    self.transport.getfn = getFn;
    self.transport.sck = @ptrCast(self);
    self.connack_rc = 255;
    self.packetIdState = 0x5555;
}

fn sendtoTxQueue(self: *@This(), packetLen: isize, packetId: ?u16) i32 {
    if (packetLen <= 0) {
        return -1;
    } else if (!(@as(usize, @intCast(packetLen)) == self.txQueue.send(@ptrCast(&self.workBuffer[0]), @intCast(packetLen), null))) {
        return -1;
    }
    _ = self.workBufferMutex.give();

    return @intCast(packetId orelse 0);
}

fn prepareConnectPacket(self: *@This(), clientID: [*:0]const u8, username: ?[*:0]const u8, password: ?[*:0]const u8) i32 {
    var connectPacket = MQTTPacket_connectData_initializer;
    connectPacket.clientID.cstring = @constCast(clientID);

    if (username != null) {
        connectPacket.username = MQTTString{ .cstring = @constCast(username), .lenstring = .{ .len = 0, .data = null } };
    }

    if (password != null) {
        connectPacket.password = MQTTString{ .cstring = @constCast(password), .lenstring = .{ .len = 0, .data = null } };
    }

    connectPacket.keepAliveInterval = 400;

    _ = self.workBufferMutex.take(null);

    var packetLen: isize = c.MQTTSerialize_connect(@ptrCast(&self.workBuffer[0]), self.workBuffer.len, &connectPacket);

    return self.sendtoTxQueue(packetLen, null);
}

fn preparePubAckPacket(self: *@This(), packetId: u16) i32 {
    _ = self.workBufferMutex.take(null);

    var packetLen: isize = c.MQTTSerialize_puback(@ptrCast(&self.workBuffer[0]), self.workBuffer.len, packetId);

    return self.sendtoTxQueue(packetLen, null);
}

fn preparePubCompPacket(self: *@This(), packetId: u16) i32 {
    _ = self.workBufferMutex.take(null);

    var packetLen: isize = c.MQTTSerialize_pubcomp(@ptrCast(&self.workBuffer[0]), self.workBuffer.len, packetId);

    return self.sendtoTxQueue(packetLen, packetId);
}

fn preparePubRelPacket(self: *@This(), dup: u8, packetId: u16) i32 {
    _ = self.workBufferMutex.take(null);

    var packetLen: isize = c.MQTTSerialize_pubrel(@ptrCast(&self.workBuffer[0]), self.workBuffer.len, dup, packetId);

    return self.sendtoTxQueue(packetLen, packetId);
}

fn prepareSubscribePacket(self: *@This(), count: usize, topicFilter: *MQTTString, qos: *c_int) i32 {
    _ = self.workBufferMutex.take(null);

    var packetId = self.generatePacketId();
    var dup: u8 = 0;

    var packetLen: isize = c.MQTTSerialize_subscribe(@ptrCast(&self.workBuffer[0]), self.workBuffer.len, dup, packetId, @intCast(count), topicFilter, qos);

    return self.sendtoTxQueue(packetLen, packetId);
}

fn preparePingPacket(self: *@This()) i32 {
    _ = self.workBufferMutex.take(null);

    var packetLen: isize = c.MQTTSerialize_pingreq(@ptrCast(&self.workBuffer[0]), self.workBuffer.len);

    return self.sendtoTxQueue(packetLen, null);
}

fn prepareDisconnectPacket(self: *@This()) i32 {
    _ = self.workBufferMutex.take(null);

    var packetLen: isize = c.MQTTSerialize_disconnect(@ptrCast(&self.workBuffer[0]), self.workBuffer.len);

    return self.sendtoTxQueue(packetLen, null);
}

pub fn preparePublishPacket(self: *@This(), topic: [*:0]const u8, payload: [*:0]const u8, payload_len: usize, qos: u8) i32 {
    _ = self.workBufferMutex.take(null);

    var topic_name = MQTTString{ .cstring = @constCast(topic), .lenstring = .{ .len = 0, .data = null } };
    var dup: u8 = 0;
    var retain: u8 = 0;
    var packetId = self.generatePacketId();

    var packetLen: isize = c.MQTTSerialize_publish(&self.workBuffer[0], self.workBuffer.len, dup, qos, retain, packetId, topic_name, @ptrCast(@constCast(payload)), @intCast(payload_len));

    return self.sendtoTxQueue(packetLen, packetId);
}

pub fn processConnAck(self: *@This()) i32 {
    var connRet: i32 = undefined;
    var sessionPresent: u8 = undefined;

    if (1 == c.MQTTDeserialize_connack(&sessionPresent, &self.connack_rc, &self.rxBuffer[0], self.rxBuffer.len)) {
        if (self.connack_rc == c.MQTT_CONNECTION_ACCEPTED) {
            connRet = 0;
        } else {
            connRet = -1;
        }
    } else {
        connRet = -1;
    }

    return connRet;
}

pub fn connect(self: *@This()) i32 {
    self.state = .connecting;
    var connRet = self.connection.initSsl(.tls_ip4, .psk);
    //var connRet: i32 = 0;
    var packetId: i32 = 0;

    if (connRet == 0) {
        connRet = self.connection.create("192.168.50.133", "8883", null, .tls_ip4);
        //connRet = self.connection.create("192.168.50.133", "1883", null, .tcp_ip4);
    }

    if (connRet == 0) {
        connRet = self.prepareConnectPacket("zigMQTT", null, null);
    }

    if (connRet == 0) {
        connRet = self.processSendQueue();
    }

    // Wait for the connack
    if (0 == self.connection.waitRx(5)) {
        if (msgTypes.connack == self.read()) {
            connRet = self.processConnAck();
        }
    }

    if (connRet == 0) {
        var subTopic = MQTTString{ .cstring = @constCast("zig/test"), .lenstring = .{ .len = 0, .data = null } };
        var qos: c_int = 1;
        packetId = self.prepareSubscribePacket(1, &subTopic, &qos);
        if (packetId < 0) {
            connRet = -1;
        }
    }

    if (connRet == 0) {
        connRet = self.processSendQueue();
    }

    // Wait for the connack
    if (0 == self.connection.waitRx(5)) {
        if (msgTypes.suback == self.read()) {
            // process the suback
            var rx_packetId: u16 = undefined;
            var maxCount: c_int = 1;
            var count: c_int = 0;
            var grantedQoSs: c_int = 0;
            if (1 == c.MQTTDeserialize_suback(&rx_packetId, maxCount, &count, &grantedQoSs, &self.rxBuffer[0], self.rxBuffer.len)) {
                if (packetId != @as(i32, @intCast(rx_packetId))) {
                    connRet = -1; // wrong packet id
                }
                _ = c.printf("Suback received\r\n");
            } else {
                connRet = -1; //
            }
        }
    }

    if (connRet == 0) {
        self.state = .connected;
        self.pingTimer.changePeriod(60000, null) catch unreachable;
        self.pubTimer.start(null) catch unreachable;
    } else {
        self.state = .err;
    }

    // wait for connack
    return connRet;
}

pub fn disconnect(self: *@This()) i32 {
    var ret: i32 = 0;
    self.pingTimer.stop(null) catch {};

    ret = self.prepareDisconnectPacket();
    if (ret == 0) {
        ret = self.processSendQueue();
    }

    if (ret == 0) {
        ret = self.connection.close();
    } else {
        _ = self.connection.close();
    }

    if (ret == 0) {
        ret = self.connection.ssl.deinit();
    } else {
        _ = self.connection.ssl.deinit();
    }

    return ret;
}

pub fn getTaskHandle(self: *@This()) freertos.TaskHandle_t {
    return self.task.getHandle();
}

pub var service: @This() = undefined;
