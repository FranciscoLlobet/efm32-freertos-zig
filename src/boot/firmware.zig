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

/// Check the firmware image signature
pub fn checkFirmwareImage() !void {
    try config.verifyFirmwareSignature(config.fw_file_name, config.fw_sig_file_name, config.getHttpSigKey());
}

const firmware_start_address: usize = 0x78000;
const firmware_max_size: usize = 0x78000;
const flash_page_size: usize = 4096;
const flash_page_addr_mask: u32 = !@as(u32, flash_page_size - 1); // 0xFFFFF000
const sd_page_size: usize = 512;

const firmware_error = error{
    hash_compare_mismatch,
    flash_erase_error,
};

// Describes the entire firmware image as a slice
const fw: []u8 = @as([*]u8, @ptrFromInt(firmware_start_address))[0..firmware_max_size];

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

pub fn eraseFlash(app_len: ?usize) !void {
    const app_fw = fw[0..(app_len orelse fw.len)];

    var current_pos: usize = 0;
    while (current_pos < app_fw.len) {
        const start_pos = current_pos;
        const end_pos: usize = if ((start_pos + flash_page_size) > app_fw.len) app_fw.len else (start_pos + flash_page_size);

        const addr = @as(*u32, @alignCast(@ptrCast(app_fw[start_pos..end_pos].ptr)));

        if (c.mscReturnOk != c.MSC_ErasePage(addr)) {
            return firmware_error.flash_erase_error;
        }

        current_pos = end_pos;
    }
}

var scratch_area: [flash_page_size]u8 = undefined;

pub fn flashFirmware(path: [*:0]const u8, app_len: ?usize) !usize {
    const app_fw = fw[0..(app_len orelse fw.len)];

    var file = try fatfs.file.open(path, @intFromEnum(fatfs.file.fMode.read));
    defer {
        file.close() catch {};
    }

    try file.sync();

    var current_pos: usize = 0;
    while (try file.readEof(scratch_area[0..])) |br| {
        const addr = @as(*u32, @alignCast(@ptrCast(app_fw[current_pos..].ptr)));

        if (c.mscReturnOk != c.MSC_WriteWord(addr, br.ptr, br.len)) {
            return firmware_error.flash_erase_error;
        }

        current_pos = file.tell();
    }

    return file.size();
}
