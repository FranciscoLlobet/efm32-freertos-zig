const std = @import("std");
const mbedtls = @import("../mbedtls.zig");
const sha256 = @import("../sha256.zig");
const pk = @import("../pk.zig");
const board = @import("microzig").board;
const config = @import("../config.zig");
const fatfs = @import("../fatfs.zig");
const freertos = @import("../freertos.zig");
const c = @cImport({
    @cInclude("board.h");
    @cInclude("miso_config.h");
    @cInclude("bootutil/bootutil.h");
    @cInclude("bootutil/image.h");
    @cInclude("bootutil/sign_key.h");
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

pub fn verifyFirmware(path: [*:0]const u8) !void {
    var hdr: image_header = undefined;

    var img = fatfs.file.open(path, @intFromEnum(fatfs.file.fMode.read)) catch unreachable;
    defer img.close() catch {};

    // Prepare the flash area info
    var fa_p: flash_area = .{ .fa_id = 0, .fa_device_id = 0, .pad16 = 0, .fa_off = 0, .fa_size = (1024 * 1024), .fp = @ptrCast(@alignCast(&img)) };

    // Load the Boot Image Header
    try boot_image_load_header(&fa_p, &hdr); // Load image header

    var temp_buf = try freertos.allocator.alloc(u8, @as(usize, 512));
    defer freertos.allocator.free(temp_buf);

    try bootutil_img_validate(null, 0, &hdr, &fa_p, temp_buf.ptr, temp_buf.len, null, 0, null);
}

/// Check the firmware image signature
///
/// Error Cases:
/// - If the file size is larger than the allowed firmware size, then `flash_firmware_size_error` is returned
/// - If the signature verification fails, the process will be aborted and `firmware_candidate_not_valid` is returned
pub fn checkFirmwareImage(path: [*:0]const u8) !void {
    const cand_len = try checkFirmwareCandidateSize(path, null); // Either the file is not open-able or the size is too large

    if (verifyBackup(path, cand_len)) |_| {
        return firmware_error.firmware_already_in_system;
    } else |err| {
        if (err == firmware_error.hash_compare_mismatch) {
            // Perform the signature verification
            load_global_public_key();

            verifyFirmware(path) catch {
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
        const dest_slice = app_fw[current_pos..(current_pos + scratch_area.len)]; // Destination slice is the same size as the scratch area

        // If padding is needed
        if (br.len < scratch_area.len) {
            @memset(scratch_area[br.len..], 0xFF);
        }

        if (c.mscReturnOk != c.MSC_WriteWord(@as(*u32, @alignCast(@ptrCast(dest_slice.ptr))), &scratch_area, scratch_area.len)) {
            return firmware_error.flash_write_error;
        }

        current_pos = file.tell();
    }

    return file.size();
}

// C-interop
pub const image_header = c.struct_image_header;
pub const flash_area = c.struct_flash_area;

export var pub_key: [256]u8 = undefined;
export var pub_key_len: usize = 0;

export const bootutil_key_cnt = @as(c_int, 1); // Currently only one key is supported
export var bootutil_keys = [_]c.bootutil_key{.{ .key = @ptrCast(@alignCast(&pub_key)), .len = &pub_key_len }};

pub const mcuBootError = error{
    generic_error,
};

pub export fn flash_area_get_device_id(fa: [*c]const flash_area) callconv(.C) c_int {
    // Get device ID

    return fa.*.fa_device_id;
}

pub export fn flash_area_read(fa: [*c]const flash_area, off: u32, dst: ?*anyopaque, len: u32) callconv(.C) c_int {
    // Read flash area
    var dst_slice = @as([*]u8, @ptrCast(dst))[0..@as(usize, len)]; // Generate destination slice

    if (0 == c.flash_area_get_device_id(fa)) {
        var fil = @as(*fatfs.file, @ptrCast(@alignCast(fa.*.fp)));

        fil.lseek(off) catch unreachable;
        _ = fil.read(dst_slice) catch unreachable;
    } else {
        // Read from flash is easy using slice-math
        @memcpy(dst_slice, fw[off..(off + len)]);
    }

    return 0;
}

pub export fn flash_area_align(fa: [*c]const flash_area) c_int {
    _ = fa;
    // Align flash area
    return 1;
}

pub export fn boot_retrieve_public_key_hash(image_index: u8, public_key_hash: [*c]u8, key_hash_size: [*c]usize) callconv(.C) c_int {
    _ = image_index;

    const hash = config.calculateMemHash(pub_key[0..pub_key_len], @as(*[32]u8, @ptrCast(public_key_hash))) catch unreachable;

    if (key_hash_size != null) {
        key_hash_size.* = hash.len;
    }

    return 0;
}

/// Image Loader
fn boot_image_load_header(fa_p: [*c]const flash_area, hdr: [*c]image_header) !void {
    if (c.FIH_SUCCESS != c.boot_image_load_header(fa_p, hdr)) return mcuBootError.generic_error;
}

fn bootutil_img_validate(enc_state: ?*c.struct_enc_key_data, image_index: c_int, hdr: [*c]image_header, fap: [*c]const flash_area, tmp_buf: [*c]u8, tmp_buf_sz: u32, seed: [*c]u8, seed_len: c_int, out_hash: [*c]u8) !void {
    if (c.FIH_SUCCESS != c.bootutil_img_validate(enc_state, image_index, hdr, fap, tmp_buf, tmp_buf_sz, seed, seed_len, out_hash)) return mcuBootError.generic_error;
}

pub fn load_global_public_key() void {
    @memset(&pub_key, 0); // clear pub key
    const pub_key_slice = mbedtls.base64Decode(@ptrCast(config.getHttpSigKey()), pub_key[0..]) catch unreachable;
    pub_key_len = pub_key_slice.len;

    bootutil_keys[0].len = &pub_key_len;
    bootutil_keys[0].key = @ptrCast(@alignCast(&pub_key));
}
