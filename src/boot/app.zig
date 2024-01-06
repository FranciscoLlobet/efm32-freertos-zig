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

const firmware_update_outcome = enum {
    incomplete,
    success,
    backup_restore,
};

fn prepareJump() void {
    board.jumpToApp();
}

const update_phase = enum(usize) {
    check,
    backup,
    erase_flash,
    flash,
    verify_flash,
    erase_flash_before_restore,
    restore_backup,
    verify_backup,
    complete,
};

task: freertos.StaticTask(2000),

fn firmwareUpdate() !firmware_update_outcome {
    return firmwareUpdateStateMachine(update_phase.check);
}

fn firmwareRestore() !firmware_update_outcome {
    return firmwareUpdateStateMachine(update_phase.restore_backup);
}

/// This function implements the firmware update state machine.
/// It is responsible for checking the firmware image in the SD card,
/// backing up the current image, erasing the flash, writing the new image,
/// verifying the new image, and finally setting the new image size in the NVM.
/// If any of the steps fail, the state machine will try to restore the backup.
/// If the backup is not successful, the state machine will loop and try to backup the firmware again.
/// If the backup is successful, the state machine will try to erase the flash and restore the backup.
/// If the restore is successful, the state machine will try to verify the backup.
fn firmwareUpdateStateMachine(start_phase: ?update_phase) !firmware_update_outcome {
    var outcome = firmware_update_outcome.incomplete;
    var phase = start_phase orelse update_phase.check;
    var backup_len: usize = 0;
    var app_len: usize = 0;

    while (phase != update_phase.complete) {
        switch (phase) {
            update_phase.check => {
                // Check the firmware image in the SD card
                _ = c.printf("Checking firmware image\n");

                try firmware.checkFirmwareImage(); // If this fails, we can't continue.

                phase = update_phase.backup; // next phase
            },
            update_phase.backup => {
                // Backup current image
                const fw_len = nvm.getFirmwareSize() catch 0;

                _ = c.printf("Backing up current image\n");

                {
                    backup_len = try firmware.backupFirmware(config.app_backup_file_name, if (fw_len != 0) fw_len else null);
                    try firmware.verifyBackup(config.app_backup_file_name, backup_len);

                    // If we fail here, we can loop for some time and try again to backup the firmware
                    // The current logic would fail the whole process if the backup is not successful, which should be ok
                }

                _ = c.printf("Verifying backup\n");
                phase = update_phase.erase_flash;
            },
            update_phase.erase_flash => {
                // Erase flash
                _ = c.printf("Erasing flash\n");

                try firmware.eraseFlash(backup_len); // Erase flash ?

                phase = update_phase.flash;
            },
            update_phase.flash => {
                // Write new image
                _ = c.printf("Writing new image\n");

                if (firmware.flashFirmware(config.fw_file_name, null)) |val| {
                    app_len = val;
                } else |err| {
                    if (err == firmware.firmware_error.flash_firmware_size_error) {
                        // Restore Backup.
                        phase = update_phase.restore_backup; // This should not be reachable
                        continue;
                    } else if (err == firmware.firmware_error.flash_write_error) {
                        // Erase flash
                        // self.phase = update_phase.erase_flash;
                        // continue;
                    } else {
                        // File issues or unknown issues.
                    }
                }

                phase = update_phase.verify_flash;
            },
            update_phase.verify_flash => {
                // Verify new image
                _ = c.printf("Verifying new image\n");

                firmware.verifyBackup(config.fw_file_name, app_len) catch |err| {
                    if (err == firmware.firmware_error.hash_compare_mismatch) {
                        // go to erase flash
                        phase = update_phase.erase_flash;
                        continue;
                    } else {
                        // File issues or unknown issues.
                    }
                };

                nvm.setFirmwareSize(app_len) catch {};

                outcome = firmware_update_outcome.success;
                phase = update_phase.complete;
            },
            update_phase.erase_flash_before_restore => {
                // Erase flash
                _ = c.printf("Erasing flash\n");

                try firmware.eraseFlash(backup_len); // Erase flash ?

                phase = update_phase.restore_backup;
            },
            update_phase.restore_backup => {
                // Restore backup
                _ = c.printf("Restoring backup\n");

                if (firmware.flashFirmware(config.app_backup_file_name, null)) |val| {
                    app_len = val;
                } else |err| {
                    if (err == firmware.firmware_error.flash_firmware_size_error) {
                        // This should not be reachable
                        return err;
                    } else if (err == firmware.firmware_error.flash_write_error) {
                        // Erase flash
                        phase = update_phase.erase_flash_before_restore;
                        continue;
                    } else {
                        // File issues or unknown issues.
                    }
                }

                phase = update_phase.complete;
            },
            update_phase.verify_backup => {
                // Verify backup
                _ = c.printf("Verifying backup\n");

                firmware.verifyBackup(config.app_backup_file_name, app_len) catch |err| {
                    if (err == firmware.firmware_error.hash_compare_mismatch) {
                        // go to erase flash
                        phase = update_phase.erase_flash_before_restore;
                        continue;
                    } else {
                        // File issues or unknown issues.
                    }
                };
                outcome = firmware_update_outcome.backup_restore;
                phase = update_phase.complete;
            },
            update_phase.complete => {
                break;
            },
        }
    }
    return outcome;
}

fn taskFunction(pvParameters: ?*anyopaque) callconv(.C) void {
    const self = freertos.Task.getAndCastPvParameters(@This(), pvParameters);
    _ = self;

    _ = nvm.init() catch 0;

    config.load_config_from_nvm() catch {
        _ = c.printf("Failed to load config from NVM\n");
    };

    while (nvm.isUpdateRequested() catch false) {
        fatfs.mount("SD") catch break;
        defer {
            fatfs.unmount("SD") catch {};
        }

        if (firmwareUpdate()) |val| {
            nvm.clearUpdateRequest() catch unreachable;
            if (val == firmware_update_outcome.backup_restore) {
                // Firmware update failed, but backup was restored
                // Try to boot the app
            } else {
                // Happy path
            }
        } else |err| {
            if (err == firmware.firmware_error.firmware_candidate_not_valid) {
                // Do nothing
                // Invalid candidate
                nvm.clearUpdateRequest() catch unreachable;
            } else if (err == firmware.firmware_error.firmware_already_in_system) {
                // Do nothing
                _ = c.printf("Firmware already in system\n");
                nvm.clearUpdateRequest() catch unreachable;
            } else {
                if (firmwareRestore()) |_| {
                    nvm.clearUpdateRequest() catch unreachable;
                } else |_| {
                    // error case
                }
            }
        }
    }

    prepareJump();
    unreachable;
}

pub fn init(self: *@This()) void {
    self.task.create(taskFunction, "BootApp", self, config.rtos_prio_boot_app) catch unreachable;
    self.task.suspendTask();
}

pub var app: @This() = undefined;
