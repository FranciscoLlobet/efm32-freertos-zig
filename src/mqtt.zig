const std = @import("std");
const freertos = @import("freertos.zig");
const network = @import("network.zig");
const config = @import("config.zig");
const system = @import("system.zig");
const board = @import("microzig").board;
const connection = @import("connection.zig");
const mbedtls_psk = @import("mbedtls_psk.zig");

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

pub const publish_qos = enum(u8) {
    qos_0 = c.MQTT_PUBLISH_QOS_0,
    qos_1 = c.MQTT_PUBLISH_QOS_1,
    qos_2 = c.MQTT_PUBLISH_QOS_2,
};

pub const errorEnum = enum(i32) {
    ok = c.MQTT_OK,
    unknown = c.MQTT_ERROR_UNKNOWN,
    nullptr = c.MQTT_ERROR_NULLPTR,
    controlForbiddenType = c.MQTT_ERROR_CONTROL_FORBIDDEN_TYPE,
    controlInvalidFlags = c.MQTT_ERROR_CONTROL_INVALID_FLAGS,
    controlWrongType = c.MQTT_ERROR_CONTROL_WRONG_TYPE,
    controlConnectClientIdRefused = c.MQTT_ERROR_CONNECT_CLIENT_ID_REFUSED,
    connectNullWillMessage = c.MQTT_ERROR_CONNECT_NULL_WILL_MESSAGE,
    connectForbiddenWillQos = c.MQTT_ERROR_CONNECT_FORBIDDEN_WILL_QOS,
    connackForbiddenFlags = c.MQTT_ERROR_CONNACK_FORBIDDEN_FLAGS,
    connackForbiddenCode = c.MQTT_ERROR_CONNACK_FORBIDDEN_CODE,
    publishForbiddenQos = c.MQTT_ERROR_PUBLISH_FORBIDDEN_QOS,
    subscribeTooManyTopics = c.MQTT_ERROR_SUBSCRIBE_TOO_MANY_TOPICS,
    malformedResponse = c.MQTT_ERROR_MALFORMED_RESPONSE,
    subscribeTooManyTopics = c.MQTT_ERROR_UNSUBSCRIBE_TOO_MANY_TOPICS,
    responseInvalidControlType = c.MQTT_ERROR_RESPONSE_INVALID_CONTROL_TYPE,
    connectNotCalled = c.MQTT_ERROR_CONNECT_NOT_CALLED,
    sendBufferIsFull = c.MQTT_ERROR_SEND_BUFFER_IS_FULL,
    socketError = c.MQTT_ERROR_SOCKET_ERROR,
    malformedRequest = c.MQTT_ERROR_MALFORMED_REQUEST,
    recvBufferTooSmall = c.MQTT_ERROR_RECV_BUFFER_TOO_SMALL,
    ackOfUnknown = c.MQTT_ERROR_ACK_OF_UNKNOWN,
    notImplemented = c.MQTT_ERROR_NOT_IMPLEMENTED,
    connectionRefused = c.MQTT_ERROR_CONNECTION_REFUSED,
    subscribeFailed = c.MQTT_ERROR_SUBSCRIBE_FAILED,
    connectionClosed = c.MQTT_ERROR_CONNECTION_CLOSED,
    initialReconnect = c.MQTT_ERROR_INITIAL_RECONNECT,
    invalidRemainingLength = c.MQTT_ERROR_INVALID_REMAINING_LENGTH,
    cleanSessionIsRequired = c.MQTT_ERROR_CLEAN_SESSION_IS_REQUIRED,
    reconnectFailed = c.MQTT_ERROR_RECONNECT_FAILED,
    reconnecting = c.MQTT_ERROR_RECONNECTING,
};

pub fn getHandle(self: *@This()) *@This() {
    return self;
}

pub fn getTaskHandle(self: *@This()) freertos.TaskHandle_t {
    return self.task.getHandle();
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

fn connect(self: *@This()) void {
    _ = self;

    //pub extern fn mqtt_connect(client: [*c]struct_mqtt_client, client_id: [*c]const u8, will_topic: [*c]const u8, will_message: ?*const anyopaque, will_message_size: usize, user_name: [*c]const u8, password: [*c]const u8, connect_flags: u8, keep_alive: u16) enum_MQTTErrors;

    //_ = mqtt_init(&self.client, self.connection., sendbuf: [*c]u8, sendbufsz: usize, recvbuf: [*c]u8, recvbufsz: usize, publish_response_callback: ?*const fn ([*c]?*anyopaque, [*c]struct_mqtt_response_publish) callconv(.C) void) ;

}

fn send(self: *@This()) void {
    if (c.MQTT_OK != c.__mqtt_send(&self.client)) {
        c.mqtt_clear_error(&self.client);
    }
}

fn receive(self: *@This()) void {
    if (c.MQTT_OK != c.__mqtt_recv(&self.client)) {
        c.mqtt_clear_error(&self.client);
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

    var connRet = self.connection.initSsl(); // Think about making ssl a subclass of connection

    if (connRet == 0) {
        connRet = self.connection.create("192.168.50.133", "8883", null, .tls_ip4);
    }

    _ = c.printf("connRet: %d\n", connRet);

    const connect_flags: u8 = c.MQTT_CONNECT_CLEAN_SESSION;

    var test_ret = c.mqtt_connect(&self.client, "zigMaster", null, null, 0, null, null, connect_flags, 400);

    _ = c.printf("test_ret: %d\n", test_ret);

    // start connection
    self.send();

    self.subscribe("zig/ret/#", 1);

    self.send();

    self.publish_timer.start(null) catch unreachable;

    while (true) {
        const test_message: [*:0]const u8 = "Test message";
        //"Test message {s}", counter;

        _ = c.printf("MQTT %d\n\r", self.task.getStackHighWaterMark());

        var notification: u32 = 0;
        if (self.task.waitForNotify(0x0, 0xFFFFFFFF, &notification, 0)) {
            if ((1 << 2) == ((1 << 2) & notification)) {
                self.publish("zig/application", test_message, c.strlen(test_message), .qos_1, false);
            }

            if ((1 << 0) == ((1 << 0) & notification)) {
                self.receive();
            }

            if ((1 << 1) == ((1 << 1) & notification)) {
                self.send();
            }
        }

        if (0 == self.connection.waitRx(1)) {
            self.notifyPendingReceive();
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
    self.task.create(if (config.enable_mqtt) taskFunction else dummyTaskFunction, "mqtt", config.rtos_stack_depth_mqtt, self, config.rtos_prio_mqtt) catch unreachable;
    self.task.suspendTask();
    if (config.enable_mqtt) {
        self.connection = connection.init(.mqtt);
        self.publish_timer.create("mqtt_update", (1 * 1000), freertos.pdTRUE, self, timer_update_function) catch unreachable;
    }
}

pub var service: @This() = undefined;
