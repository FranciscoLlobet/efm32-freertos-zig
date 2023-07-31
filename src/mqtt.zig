const std = @import("std");
const freertos = @import("freertos.zig");
const network = @import("network.zig");
const config = @import("config.zig");
const system = @import("system.zig");
const board = @import("microzig").board;
const connection = @import("connection.zig");
const MQTTPacket = @import("MQTTPacket.zig");

const c = @cImport({
    @cDefine("MQTTC_PAL_FILE", "miso_mqtt_pal.h");
    @cInclude("network.h");
    @cInclude("wifi_service.h");
    @cInclude("lwm2m_client.h");
    @cInclude("mqtt.h");
});

// Setting alias for types
const mqtt_client = c.mqtt_client;
const printf = c.printf;
const MQTT_OK: i32 = @intCast(c.MQTT_OK);

pub export fn mqtt_pal_sendall(fd: ?*anyopaque, buf: [*c]u8, len: usize, flags: i32) callconv(.C) isize {
    _ = flags;
    return @as(*connection, @ptrCast(@alignCast(fd))).send(buf, len);
}

pub export fn mqtt_pal_recvall(fd: ?*anyopaque, buf: [*c]u8, len: usize, flags: i32) callconv(.C) isize {
    _ = flags;
    return @as(*connection, @ptrCast(@alignCast(fd))).recieve(buf, len);
}

var send_buf: [1024]u8 align(@alignOf(u32)) = undefined; // Alignment must be set to 32bit
var recv_buf: [1024]u8 align(@alignOf(u32)) = undefined;

connection: connection,
task: freertos.Task,
client: mqtt_client,
ping_timer: freertos.Timer,
publish_timer: freertos.Timer,
mqttpacket: MQTTPacket,

pub const publish_qos = enum(u8) {
    qos_0 = c.MQTT_PUBLISH_QOS_0,
    qos_1 = c.MQTT_PUBLISH_QOS_1,
    qos_2 = c.MQTT_PUBLISH_QOS_2,
};

pub const errors = error{
    send_error,
    recieve_error,
};

pub fn getTaskHandle(self: *@This()) freertos.TaskHandle_t {
    return self.mqttpacket.task.getHandle();
}

pub fn publish(self: *@This(), topic: [*:0]const u8, message: [*:0]const u8, message_size: usize, qos: publish_qos, retain: bool) void {
    var flags = @intFromEnum(qos);

    if (retain) {
        flags |= @as(u8, @intCast(c.MQTT_PUBLISH_RETAIN));
    }

    if (c.MQTT_OK == c.mqtt_publish(&self.client, topic, message, message_size, flags)) {
        self.notifyPendingSend();
    }
}

pub fn subscribe(self: *@This(), topic: [*:0]const u8, max_qos_level: i32) void {
    if (c.MQTT_OK == c.mqtt_subscribe(&self.client, topic, @as(c_int, max_qos_level))) {
        self.notifyPendingSend();
    }
}

fn send(self: *@This()) !void {
    var ret: i32 = @intCast(c.__mqtt_send(&self.client));
    if (ret != MQTT_OK) {
        ret = c.mqtt_clear_error(&self.client);
    }
    if (ret != MQTT_OK) {
        return errors.send_error;
    }
}

fn receive(self: *@This()) !void {
    var ret: i32 = @intCast(c.__mqtt_recv(&self.client));
    if (ret != MQTT_OK) {
        ret = c.mqtt_clear_error(&self.client);
    }
    if (ret != MQTT_OK) {
        return errors.recieve_error;
    }
}

fn notifyPendingSend(self: *@This()) void {
    _ = self.task.notify((1 << 1), .eSetBits);
}
fn notifyPendingReceive(self: *@This()) void {
    _ = self.task.notify((1 << 0), .eSetBits);
}

fn publishResponseCallback(state: [*c]?*anyopaque, publish_response: [*c]c.struct_mqtt_response_publish) callconv(.C) void {
    _ = state;

    _ = printf("Topic: %s\r\n", publish_response.*.topic_name);
}

fn reconnectCallback(client: [*c]c.struct_mqtt_client, state: [*c]?*anyopaque) callconv(.C) void {
    _ = client;
    _ = state;
}

fn initMqtt(self: *@This()) void {
    _ = c.mqtt_init(&self.client, &self.connection, @as([*c]u8, @ptrCast(&send_buf[0])), send_buf.len, @as([*c]u8, @ptrCast(&recv_buf[0])), recv_buf.len, publishResponseCallback);
}

fn timer_update_function(xTimer: freertos.TimerHandle_t) callconv(.C) void {
    _ = @as(*@This(), @as(*@This(), @ptrCast(@alignCast(freertos.c.pvTimerGetTimerID(xTimer))))).task.notify((1 << 2), .eSetBits);
}

fn taskFunction(pvParameters: ?*anyopaque) callconv(.C) void {
    var self = freertos.getAndCastPvParameters(@This(), pvParameters);

    self.initMqtt();

    const temp = struct { temp: f32, timestamp: u32 };

    var string = std.ArrayList(u8).init(freertos.allocator);

    var local_temp = temp{ .temp = 0.001, .timestamp = 1 };

    _ = string;
    _ = local_temp;

    while (true) {
        _ = self.innerLoop();
    }
}

fn innerLoop(self: *@This()) i32 {
    var ret: i32 = 0;

    var connRet = self.connection.initSsl();

    if (connRet == 0) {
        connRet = self.connection.create("192.168.50.133", "8883", null, .tls_ip4);
    }

    _ = c.printf("connRet: %d\n", connRet);

    const connect_flags: u8 = c.MQTT_CONNECT_CLEAN_SESSION;

    var test_ret = c.mqtt_connect(&self.client, "zigMaster", null, null, 0, null, null, connect_flags, 400);

    _ = c.printf("test_ret: %d\n", test_ret);

    self.subscribe("zig/ret/#", 1);

    self.publish_timer.start(null) catch unreachable;

    ret = MQTT_OK;
    while (ret == MQTT_OK) {
        const test_message: [*:0]const u8 = "Test message";
        //"Test message {s}", counter;

        _ = c.printf("MQTT %d\n\r", self.task.getStackHighWaterMark());

        var notification: u32 = 0;
        if (self.task.waitForNotify(0x0, 0xFFFFFFFF, &notification, 0)) {
            if ((1 << 2) == ((1 << 2) & notification)) {
                self.publish("zig/application", test_message, c.strlen(test_message), .qos_1, false);
                _ = c.printf("Publish!\n\r");
            }

            if ((1 << 0) == ((1 << 0) & notification)) {
                self.receive() catch break;
            }

            if ((1 << 1) == ((1 << 1) & notification)) {
                self.send() catch break;
            }
        }

        if (0 == self.connection.waitRx(1)) {
            self.notifyPendingReceive();
        }

        // get out of issues
    }

    self.publish_timer.stop(null) catch unreachable;
    _ = self.connection.close();
    _ = self.connection.ssl.deinit();

    return ret;
}

pub fn create(self: *@This()) void {
    self.mqttpacket.create();
}

pub var service: @This() = undefined;
