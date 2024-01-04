const std = @import("std");
//const microzig = @import("microzig");
const freertos = @import("../freertos.zig");
const system = @import("../system.zig");
const config = @import("../config.zig");
const leds = @import("../leds.zig");
const buttons = @import("../buttons.zig");
const usb = @import("../usb.zig");
const fatfs = @import("../fatfs.zig");
const nvm = @import("../nvm.zig");
const board = @import("microzig").board;
const firmware = @import("firmware.zig");

const c = @cImport({
    @cInclude("board.h");
    @cInclude("miso.h");
    @cInclude("miso_config.h");
});

fn prepareJump() void {
    board.jumpToApp();
}

const update_phase = enum(usize) {
    check,
    backup,
    verify_backup,
    erase_flash,
    flash,
    verify_flash,
    complete,
};

task: freertos.StaticTask(2000),
phase: update_phase,

fn firmwareUpdate(self: *@This()) !void {
    self.phase = update_phase.check;
    var backup_len: usize = 0;
    var app_len: usize = 0;

    while (self.phase != update_phase.complete) {
        switch (self.phase) {
            update_phase.check => {
                // Check the firmware image in the SD card
                _ = c.printf("Checking firmware image\n");

                try firmware.checkFirmwareImage();

                self.phase = update_phase.backup; // next phase
            },
            update_phase.backup => {
                // Backup current image
                _ = c.printf("Backing up current image\n");

                backup_len = try firmware.backupFirmware(config.app_backup_file_name, null);

                self.phase = update_phase.verify_backup;
            },
            update_phase.verify_backup => {
                // Verify backup
                _ = c.printf("Verifying backup\n");

                try firmware.verifyBackup(config.app_backup_file_name, backup_len);

                self.phase = update_phase.erase_flash;
            },
            update_phase.erase_flash => {
                // Erase flash
                _ = c.printf("Erasing flash\n");

                try firmware.eraseFlash(backup_len);

                self.phase = update_phase.flash;
            },
            update_phase.flash => {
                // Write new image
                _ = c.printf("Writing new image\n");

                app_len = try firmware.flashFirmware(config.fw_file_name, null);

                self.phase = update_phase.verify_flash;
            },
            update_phase.verify_flash => {
                // Verify new image
                _ = c.printf("Verifying new image\n");

                try firmware.verifyBackup(config.fw_file_name, app_len);

                self.phase = update_phase.complete;
            },
            update_phase.complete => {
                return;
                //break update_phase.complete;
            },
        }
    }
}

fn taskFunction(pvParameters: ?*anyopaque) callconv(.C) void {
    const self = freertos.Task.getAndCastPvParameters(@This(), pvParameters);

    _ = nvm.init() catch 0;

    config.load_config_from_nvm() catch {
        _ = c.printf("Failed to load config from NVM\n");
    };

    while (true) {
        if (false) {
            //
        } else if (nvm.isUpdateRequested() catch false) {
            fatfs.mount("SD") catch unreachable;
            defer {
                fatfs.unmount("SD") catch {};
            }

            if (self.firmwareUpdate()) |_| {
                nvm.clearUpdateRequest() catch unreachable;
            } else |_| {
                // Error handling
                _ = c.printf("Firmware update failed\n");
                // Rollback
            }
            continue;
        }

        prepareJump();
    }
}

pub fn init(self: *@This()) void {
    self.task.create(taskFunction, "BootApp", self, config.rtos_prio_boot_app) catch unreachable;
    self.task.suspendTask();
}

pub var app: @This() = undefined;
