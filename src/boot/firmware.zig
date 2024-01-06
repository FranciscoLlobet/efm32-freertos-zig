const std = @import("std");
const mbedtls = @import("../mbedtls.zig");
const sha256 = @import("../sha256.zig");
const pk = @import("../pk.zig");
const board = @import("microzig").board;
const config = @import("../config.zig");
const fatfs = @import("../fatfs.zig");
const c = @cImport({
    @cInclude("board.h");
    @cInclude("miso_config.h");
});

const firmware_start_address: usize = 0x78000;
const firmware_max_size: usize = 0x78000;
const flash_page_size: usize = 4096;
const flash_page_addr_mask: usize = @intCast(~@as(u32, flash_page_size - 1)); // 0xFFFFF000
const sd_page_size: usize = 512;

pub const firmware_error = error{
    firmware_already_in_system,
    firmware_candidate_not_valid,
    hash_compare_mismatch,
    flash_erase_error,
    flash_write_error,
    flash_firmware_size_error,
};

/// Describes the entire firmware image as a byte-slice
const fw: []u8 = @as([*]u8, @ptrFromInt(firmware_start_address))[0..firmware_max_size];

/// Scratch area for reading from the SD card
var scratch_area: [flash_page_size]u8 = undefined;

fn checkFirmwareCandidateSize(path: [*:0]const u8, app_len: ?usize) !usize {
    const app_fw = fw[0..(app_len orelse fw.len)];

    var fw_cand = try fatfs.file.open(path, @intFromEnum(fatfs.file.fMode.read));
    defer {
        fw_cand.close() catch {};
    }

    try fw_cand.sync();

    return if (fw_cand.size() > app_fw.len) firmware_error.flash_firmware_size_error else fw_cand.size();
}

/// Check the firmware image signature
///
/// Error Cases:
/// - If the file size is larger than the allowed firmware size, then `flash_firmware_size_error` is returned
/// - If the signature verification fails, the process will be aborted and `firmware_candidate_not_valid` is returned
pub fn checkFirmwareImage() !void {
    const cand_len = try checkFirmwareCandidateSize(config.fw_file_name, null); // Either the file is not open-able or the size is too large

    if (verifyBackup(config.fw_file_name, cand_len)) |_| {
        return firmware_error.firmware_already_in_system;
    } else |err| {
        if (err == firmware_error.hash_compare_mismatch) {
            config.verifyFirmwareSignature(config.fw_file_name, config.fw_sig_file_name, config.getHttpSigKey()) catch {
                return firmware_error.firmware_candidate_not_valid;
            };
        }
    }
}

/// Perform the firmware backup
/// Returns the size of the backup length
pub fn backupFirmware(path: [*:0]const u8, app_len: ?usize) !usize {
    const app_fw = fw[0..(app_len orelse fw.len)];

    var backup = try fatfs.file.open(path, @intFromEnum(fatfs.file.fMode.create_always) | @intFromEnum(fatfs.file.fMode.write));
    defer {
        backup.close() catch {};
    }

    try backup.sync();

    while (backup.tell() < app_fw.len) {
        const start_position: usize = backup.tell();
        const end_position: usize = if ((start_position + sd_page_size) > app_fw.len) app_fw.len else (start_position + sd_page_size);

        var wb = try backup.write(app_fw[start_position..end_position]);

        _ = wb;
        try backup.sync();
    }

    return backup.size();
}

pub fn verifyBackup(path: [*:0]const u8, app_len: ?usize) !void {
    const app_fw = fw[0..(app_len orelse fw.len)];

    var app_backup_hash: [32]u8 = undefined;
    var app_flash_hash: [32]u8 = undefined;

    const hash_backup = try config.calculateFileHash(path, &app_backup_hash);
    const hash_flash = try config.calculateMemHash(app_fw, &app_flash_hash);

    if (!std.mem.eql(u8, hash_backup, hash_flash)) {
        return firmware_error.hash_compare_mismatch;
    }
}

/// Erase the flash memory area used for the firmware storage.
/// Please be aware that this is a page-erase mechanism, so the entire page will be erased.
pub fn eraseFlash(app_len: ?usize) !void {
    const app_fw = fw[0..(app_len orelse fw.len)];

    var current_pos: usize = 0;
    while (current_pos < app_fw.len) {
        const start_pos = current_pos & flash_page_addr_mask;
        const end_pos: usize = if ((start_pos + flash_page_size) > app_fw.len) app_fw.len else (start_pos + flash_page_size);

        const dest_slice = app_fw[start_pos..end_pos];

        if (c.mscReturnOk != c.MSC_ErasePage(@as(*u32, @alignCast(@ptrCast(dest_slice.ptr))))) {
            return firmware_error.flash_erase_error;
        }

        current_pos = end_pos;
    }
}

/// Flash the firmware image
/// Returns the size of the flashed image
/// Error Cases:
/// - If the file size is larger than the allowed firmware size, then `flash_firmware_size_error` is returned
/// - If the flash write fails, then `flash_write_error` is returned
pub fn flashFirmware(path: [*:0]const u8, app_len: ?usize) !usize {
    const app_fw = fw[0..(app_len orelse fw.len)];

    var file = try fatfs.file.open(path, @intFromEnum(fatfs.file.fMode.read));
    defer {
        file.close() catch {};
    }

    try file.sync();

    if (file.size() > app_fw.len) {
        return firmware_error.flash_firmware_size_error;
    }

    var current_pos: usize = 0;
    while (try file.readEof(scratch_area[0..])) |br| {
        // Generate the destination slice
        const dest_slice = app_fw[current_pos..(current_pos + br.len)];

        if (c.mscReturnOk != c.MSC_WriteWord(@as(*u32, @alignCast(@ptrCast(dest_slice.ptr))), br.ptr, br.len)) {
            return firmware_error.flash_write_error;
        }

        current_pos = file.tell();
    }

    return file.size();
}
