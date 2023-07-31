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
    .keepAliveInterval = 60,
    .cleansession = 1,
    .willFlag = 0,
    .will = MQTTPacket_willOptions_initializer,
    .username = MQTTString_initializer,
    .password = MQTTString_initializer,
};

connection: connection,
rxQueue: freertos.Queue,
txQueue: freertos.MessageBuffer,
task: freertos.Task,
state: state,
packetIdState: u16,
transport: MQTTTransport,
connack_rc: u8, // Last CONNACK received
workBuffer: [256]u8 align(@alignOf(u32)),
txBuffer: [256]u8 align(@alignOf(u32)),
rxBuffer: [256]u8 align(@alignOf(u32)),

fn generatePacketId(self: *@This()) u16 {
    self.packetIdState ^= self.packetIdState << 7;
    self.packetIdState ^= self.packetIdState >> 9;
    self.packetIdState ^= self.packetIdState << 8;
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

    // Get to connect
    var ret = self.connect();

    while (ret == 0) {
        //var ret: i32 = 0;

        ret = self.processSendQueue();

        if (0 == self.connection.waitRx(1)) {
            var readRet = self.read();
            if (readRet == msgTypes.try_again) {
                // Try again
            } else if ((readRet == msgTypes.connect) or (readRet == msgTypes.subscribe)) {
                // error
            } else if (readRet == msgTypes.connack) {
                // readRet = c.MQTTDeserialize_connack(&sessionPresent, &connack_rc, buf, buflen);
            } else if (readRet == msgTypes.publish) {
                var dup: u8 = undefined;
                var qos: c_int = undefined;
                var retained: u8 = undefined;
                var packetId: u16 = undefined;
                var topicName = MQTTString_initializer;
                var payload: [*c]u8 = undefined;
                var payloadLen: isize = undefined;

                if (1 == c.MQTTDeserialize_publish(&dup, &qos, &retained, &packetId, &topicName, &payload, &payloadLen, @ptrCast(&self.rxBuffer[0]), self.rxBuffer.len)) {
                    ret = switch (qos) {
                        0 => 0,
                        1 => self.preparePubAckPacket(packetId),
                        2 => -1,
                        else => -1,
                    };
                    if (ret == 0) {
                        // notify application
                    }
                } else {
                    ret = -1; // error state
                }
            } else if (readRet == msgTypes.puback) {
                // Response for Client pub qos1
                // Deserialize
            } else if (readRet == msgTypes.pubrec) {
                // Response for client pub qos2
                // Deserialize
            } else if (readRet == msgTypes.pubrel) {
                // Response for server pub qos2
                // Serialize pubcomp
            } else if (readRet == msgTypes.pubcomp) {
                //
            } else if (readRet == msgTypes.suback) {

                // MQTTDeserialize_suback(&submsgid, 1, &subcount, &granted_qos, buf, buflen);
            } else if (readRet == msgTypes.unsubscribe) {
                // Client does not get unsubscribe
            } else if (readRet == msgTypes.unsuback) {
                // Handle unsuback
            } else if (readRet == msgTypes.pingreq) {
                // Client does not get ping request
            } else if (readRet == msgTypes.pingresp) {
                // Handle the ping response
            } else if (readRet == msgTypes.disconnect) {
                // Client does not disconnect
            } else {
                // Generic error
            }
        } else {
            _ = c.printf("mqtt\r\n");
        }
    }
}

fn dummyTaskFunction(pvParameters: ?*anyopaque) callconv(.C) void {
    var self = freertos.getAndCastPvParameters(@This(), pvParameters);

    while (true) {
        self.task.suspendTask();
    }
}

pub fn create(self: *@This()) void {
    self.txQueue.create(1024); // Create message buffer
    self.task.create(if (config.enable_mqtt) taskFunction else dummyTaskFunction, "mqtt", config.rtos_stack_depth_mqtt, self, config.rtos_prio_mqtt) catch unreachable;
    self.task.suspendTask();
    if (config.enable_mqtt) {
        self.connection = connection.init(.mqtt);
    }
    self.transport.getfn = getFn;
    self.transport.sck = @ptrCast(self);
    self.connack_rc = 255;
    self.packetIdState = 0x5555;
}

fn prepareConnectPacket(self: *@This()) i32 {
    var connectPacket = MQTTPacket_connectData_initializer;
    connectPacket.clientID.cstring = @constCast("ZigMaister");
    connectPacket.keepAliveInterval = 400;

    var packetLen: isize = c.MQTTSerialize_connect(@ptrCast(&self.workBuffer[0]), self.workBuffer.len, &connectPacket);
    if (packetLen == 0) return -1;

    return @intFromBool(!(@as(usize, @intCast(packetLen)) == self.txQueue.send(@ptrCast(&self.workBuffer[0]), @intCast(packetLen), null)));
}

fn preparePubAckPacket(self: *@This(), packetId: u16) i32 {
    var packetLen: isize = c.MQTTSerialize_puback(@ptrCast(&self.workBuffer[0]), self.workBuffer.len, packetId);
    if (packetLen == 0) return -1;

    return @intFromBool(!(@as(usize, @intCast(packetLen)) == self.txQueue.send(@ptrCast(&self.workBuffer[0]), @intCast(packetLen), null)));
}

fn preparePubCompPacket(self: *@This(), packetId: u16) i32 {
    var packetLen: isize = c.MQTTSerialize_pubcomp(@ptrCast(&self.workBuffer[0]), self.workBuffer.len, packetId);
    if (packetLen == 0) return -1;

    return @intFromBool(!(@as(usize, @intCast(packetLen)) == self.txQueue.send(@ptrCast(&self.workBuffer[0]), @intCast(packetLen), null)));
}

fn preparePubRelPacket(self: *@This(), dup: u8, packetId: u16) i32 {
    var packetLen: isize = c.MQTTSerialize_pubrel(@ptrCast(&self.workBuffer[0]), self.workBuffer.len, dup, packetId);
    if (packetLen == 0) return -1;

    return @intFromBool(!(@as(usize, @intCast(packetLen)) == self.txQueue.send(@ptrCast(&self.workBuffer[0]), @intCast(packetLen), null)));
}

fn prepareSubscribePacket(self: *@This(), count: usize, topicFilter: *MQTTString, qos: *c_int) i32 {
    var packetId = self.generatePacketId();
    var dup: u8 = 0;

    var packetLen: isize = c.MQTTSerialize_subscribe(@ptrCast(&self.workBuffer[0]), self.workBuffer.len, dup, packetId, @intCast(count), topicFilter, qos);
    if (packetLen == 0) return -1;

    return @intFromBool(!(@as(usize, @intCast(packetLen)) == self.txQueue.send(@ptrCast(&self.workBuffer[0]), @intCast(packetLen), null)));
}

pub fn preparePublishPacket(self: *@This(), topic: [*:0]const u8, payload: [*:0]const u8, payload_len: usize, qos: u8) i32 {
    var topic_name = MQTTString{ .cstring = topic, .lenstring = .{ .len = 0, .data = null } };
    var dup: u8 = 0;
    var retain: u8 = 0;
    var packetId = self.generatePacketId();

    var packetLen: isize = c.MQTTSerialize_publish(&self.workBuffer[0], self.workBuffer.len, dup, qos, retain, packetId, &topic_name, payload, payload_len);

    if (packetLen < 0) return -1;

    return @intFromBool(!(@as(usize, @intCast(packetLen)) == self.txQueue.send(@ptrCast(&self.workBuffer[0]), @intCast(packetLen), null)));
}

pub fn connect(self: *@This()) i32 {
    self.state = .connecting;
    var connRet = self.connection.initSsl();

    if (connRet == 0) {
        connRet = self.connection.create("192.168.50.133", "8883", null, .tls_ip4);
    }

    if (connRet == 0) {
        connRet = self.prepareConnectPacket();
    }

    if (connRet == 0) {
        connRet = self.processSendQueue();
    }

    // Wait for the connack
    if (0 == self.connection.waitRx(5)) {
        if (msgTypes.connack == self.read()) {
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
        }
    }

    if (connRet == 0) {
        var subTopic = MQTTString{ .cstring = @constCast("zig/#"), .lenstring = .{ .len = 0, .data = null } };
        var qos: c_int = 1;
        connRet = self.prepareSubscribePacket(1, &subTopic, &qos);
    }

    if (connRet == 0) {
        connRet = self.processSendQueue();
    }

    // Wait for the connack
    if (0 == self.connection.waitRx(5)) {
        if (msgTypes.suback == self.read()) {
            // process the suback
            var packetId: u16 = undefined;
            var maxCount: c_int = 1;
            var count: c_int = 0;
            var grantedQoSs: c_int = 0;
            if (1 == c.MQTTDeserialize_suback(&packetId, maxCount, &count, &grantedQoSs, &self.rxBuffer[0], self.rxBuffer.len)) {
                _ = c.printf("Suback received\r\n");
            } else {
                connRet = -1;
            }
        }
    }

    if (connRet == 0) {
        self.state = .connected;
    } else {
        self.state = .err;
    }

    // wait for connack
    return connRet;
}
