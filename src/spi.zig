//! SPI object

const c = @cImport({
    @cInclude("board.h");
    @cInclude("miso.h");
    @cInclude("miso_config.h");
});

const freertos = @import("freertos.zig");

const ecode = c.Ecode_t;
//const spiDrvHandle
const txferElement = struct {
    status: ecode, // status
    items: isize, // number of items transfered
};

pub fn Spi() type {
    return struct {
        spidrvHandle: c.SPIDRV_HandleData_t,

        txQueue: freertos.StaticQueue(txferElement, 1),
        rxQueue: freertos.StaticQueue(txferElement, 1),
        //spiDrvHandle: c.SPIDRV_HandleData_t,

        fn recieve_callback(handle: c.SPIDRV_HandleData_t, buffer: []u8, n: usize) callconv(.C) void {
            var xHigherPriorityTaskWoken: freertos.BaseType_t = freertos.pdFALSE;
            const transfer_status_information = txferElement{ .status = c.ECODE_EMDRV_SPIDRV_OK, .items = n };

            // self.xQueueSendFromISR(&transfer_status_information, &xHigherPriorityTaskWoken);

            freertos.portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
        }
        fn send_callback(handle: c.SPIDRV_HandleData_t, buffer: []u8, n: usize) callconv(.C) void {
            var xHigherPriorityTaskWoken: freertos.BaseType_t = freertos.pdFALSE;
            const transfer_status_information = txferElement{ .status = c.ECODE_EMDRV_SPIDRV_OK, .items = n };

            // self.xQueueSendFromISR(&transfer_status_information, &xHigherPriorityTaskWoken);

            freertos.portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
        }

        pub fn init(self: *@This(), spiDrvHandle: c.SPIDRV_HandleData_t) void {
            //self.spidrvHandle = spiDrvHandle;
            //self.txQueue = freertos.StaticQueue(txferElement, 1);
            //self.rxQueue = freertos.StaticQueue(txferElement, 1);
        }
        pub fn deinit(self: *@This()) void {
            //self.txQueue.deinit();
            //self.rxQueue.deinit();
        }

        pub fn send(self: *@This(), data: []const u8, timeout_ticks: ?u32) !usize {
            var itemsTransfered: usize = 0;

            const spiEcode = c.SPIDRV_MTransmit(&self.spiDrvHandle, @ptrCast(data.ptr), @intCast(data.len), send_callback);
            if (spiEcode == c.ECODE_EMDRV_SPIDRV_OK) {
                if (self.txQueue.receive(timeout_ticks)) |val| {
                    if (val.status == c.ECODE_EMDRV_SPIDRV_OK) {
                        itemsTransfered = val.items;
                    }
                } else |err| {
                    return err;
                }
            }
            return itemsTransfered;
        }
        pub fn receive(self: *@This(), data: []u8, timeout_ticks: ?u32) ![]u8 {
            var itemsTransfered: usize = 0;

            const spiEcode = c.SPIDRV_MReceive(&self.spiDrvHandle, @ptrCast(data.ptr), @intCast(data.len), recieve_callback);
            if (spiEcode == c.ECODE_EMDRV_SPIDRV_OK) {
                if (self.rxQueue.receive(timeout_ticks)) |val| {
                    if (val.status == c.ECODE_EMDRV_SPIDRV_OK) {
                        itemsTransfered = val.items;
                    }
                } else |err| {
                    return err;
                }
            }
            return data[0..itemsTransfered];
        }
    };
}
