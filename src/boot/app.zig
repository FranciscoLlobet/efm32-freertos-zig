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
    board.jumpToApp(0x78000 + 0x80);
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

    /// Firmware Update is complete
    complete,

    /// Firmware Update failed, but backup was restored
    backup_restored,
};

task: freertos.StaticTask(@This(), 2000, "bootApp", taskFunction),

fn firmwareUpdate() !firmware_update_outcome {
    return firmwareUpdateStateMachine(update_phase.check);
}

fn firmwareRestore() !firmware_update_outcome {
    return firmwareUpdateStateMachine(update_phase.restore_backup);
}

fn checkFirmwareCandidate() !update_phase {
    _ = c.printf("Checking firmware image\n");

    try firmware.checkFirmwareImage(config.fw_file_name); // If this fails, we can't continue.

    return update_phase.backup; // next phase
}

fn performFirmwareBackup() !struct {
    phase: update_phase,
    backup_len: usize,
} {
    const fw_len = nvm.getFirmwareSize() catch 0;

    _ = c.printf("Backing up current image\n");

    const backup_len = try firmware.backupFirmware(config.app_backup_file_name, if (fw_len != 0) fw_len else null);

    _ = c.printf("Verifying backup\n");

    try firmware.verifyBackup(config.app_backup_file_name, backup_len);

    // If we fail here, we can loop for some time and try again to backup the firmware
    // The current logic would fail the whole process if the backup is not successful, which should be ok
    return .{ .phase = update_phase.erase_flash, .backup_len = backup_len };
}

fn eraseFlash(backup_len: usize) !update_phase {
    _ = backup_len;
    _ = c.printf("Erasing flash\n");

    try firmware.eraseFlash(null);
    return update_phase.flash;
}

fn eraseFlashBeforeRestore(backup_len: usize) !update_phase {
    _ = backup_len;
    // Erase flash
    _ = c.printf("Erasing flash\n");

    try firmware.eraseFlash(null); // Erase flash ?
    return update_phase.restore_backup;
}

fn restoreBackup() !struct { phase: update_phase, app_len: usize } {
    var app_len: usize = 0;
    var phase = update_phase.verify_backup;

    _ = c.printf("Restoring backup\n");

    if (firmware.flashFirmware(config.app_backup_file_name, null)) |val| {
        app_len = val;
        phase = update_phase.verify_backup;
    } else |err| {
        if (err == firmware.firmware_error.flash_write_error) {
            phase = update_phase.erase_flash_before_restore;
        } else {
            return err;
        }
    }

    return .{ .phase = phase, .app_len = app_len };
}

fn verifyImage(app_len: usize) !update_phase {
    _ = c.printf("Verifying new image\n");

    firmware.verifyBackup(config.fw_file_name, app_len) catch |err| {
        if (err == firmware.firmware_error.hash_compare_mismatch) {
            // go to erase flash
            return update_phase.erase_flash;
        } else {
            return err;
        }
    };

    nvm.setFirmwareSize(app_len) catch {};

    return update_phase.complete;
}

fn verifyBackup(app_len: usize) !update_phase {
    // Verify backup
    _ = c.printf("Verifying backup\n");

    firmware.verifyBackup(config.app_backup_file_name, app_len) catch |err| {
        if (err == firmware.firmware_error.hash_compare_mismatch) {
            // go to erase flash
            return update_phase.erase_flash_before_restore;
        } else {
            return err;
        }
    };

    return update_phase.backup_restored;
}

fn flashImage() !struct {
    phase: update_phase,
    app_len: usize,
} {
    var app_len: usize = 0;
    var phase = update_phase.restore_backup;

    if (firmware.flashFirmware(config.fw_file_name, null)) |val| {
        app_len = val;
        phase = update_phase.verify_flash;
    } else |err| {
        if (err == firmware.firmware_error.flash_firmware_size_error) {
            phase = update_phase.restore_backup; // This should not be reachable
        } else if (err == firmware.firmware_error.flash_write_error) {
            phase = update_phase.erase_flash;
        } else {
            return err;
        }
    }

    return .{ .phase = phase, .app_len = app_len };
}

const update_state = struct { phase: update_phase, app_len: usize, backup_len: usize };

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

    var state: update_state = .{ .phase = start_phase orelse update_phase.check, .app_len = 0, .backup_len = 0 };
    _ = state;

    while (true) {
        phase = switch (phase) {
            update_phase.check => try checkFirmwareCandidate(),
            update_phase.backup => blk: {
                // Backup current image
                const res = try performFirmwareBackup();

                backup_len = res.backup_len;
                break :blk res.phase;
            },
            update_phase.erase_flash => try eraseFlash(backup_len),
            update_phase.flash => blk: {
                // Write new image
                const res = try flashImage();

                app_len = res.app_len;
                break :blk res.phase;
            },
            update_phase.verify_flash => try verifyImage(app_len),
            update_phase.erase_flash_before_restore => try eraseFlashBeforeRestore(backup_len),
            update_phase.restore_backup => blk: {
                // Restore backup
                const res = try restoreBackup();

                app_len = res.app_len;
                break :blk res.phase;
            },
            update_phase.verify_backup => try verifyBackup(app_len),
            update_phase.complete => {
                return firmware_update_outcome.success;
            },
            update_phase.backup_restored => {
                return firmware_update_outcome.backup_restore;
            },
        };
    }

    return outcome;
}

fn taskFunction(self: *@This()) void {
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
    self.task.create(self, config.rtos_prio_boot_app) catch unreachable;
    self.task.suspendTask();
}

pub var app: @This() = undefined;
