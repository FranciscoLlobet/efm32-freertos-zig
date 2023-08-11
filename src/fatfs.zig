const std = @import("std");
const freertos = @import("freertos.zig");
const c = @cImport({
    @cInclude("board.h");
    @cInclude("ff.h");
});

const FSIZE_t = c.FSIZE_t;
const UINT = c.UINT;
const FR_OK = c.FR_OK;
const FILINFO = c.FILINFO;
pub const FATFS = c.FATFS;

const fRet = enum(usize) {
    fr_ok = c.FR_OK,
    fr_disk_err = c.FR_DISK_ERR,
    fr_int_err = c.FR_INT_ERR,
    fr_not_ready = c.FR_NOT_READY,
    fr_no_file = c.FR_NO_FILE,
    fr_no_path = c.FR_NO_PATH,
    fr_invalid_name = c.FR_INVALID_NAME,
    fr_denied = c.FR_DENIED,
    fr_exist = c.FR_EXIST,
    fr_invalid_object = c.FR_INVALID_OBJECT,
    fr_write_protected = c.FR_WRITE_PROTECTED,
    fr_invalid_drive = c.FR_INVALID_DRIVE,
    fr_not_enabled = c.FR_NOT_ENABLED,
    fr_no_filesystem = c.FR_NO_FILESYSTEM,
    fr_mkfs_aborted = c.FR_MKFS_ABORTED,
    fr_timeout = c.FR_TIMEOUT,
    fr_locked = c.FR_LOCKED,
    fr_not_enough_core = c.FR_NOT_ENOUGH_CORE,
    fr_too_many_open_files = c.FR_TOO_MANY_OPEN_FILES,
    fr_invalid_parameter = c.FR_INVALID_PARAMETER,

    const fr_error = error{ FR_DISK_ERR, FR_INT_ERR, FR_NOT_READY, FR_NO_FILE, FR_NO_PATH, FR_INVALID_NAME, FR_DENIED, FR_EXIST, FR_INVALID_OBJECT, FR_WRITE_PROTECTED, FR_INVALID_DRIVE, FR_NOT_ENABLED, FR_NO_FILESYSTEM, FR_MKFS_ABORTED, FR_TIMEOUT, FR_LOCKED, FR_NOT_ENOUGH_CORE, FR_TOO_MANY_OPEN_FILES, FR_INVALID_PARAMETER, generic_error };
};

pub export var fileSystem: c.FATFS = undefined;

pub const file = struct {
    handle: c.FIL,

    pub const fMode = enum(u8) {
        read = c.FA_READ,
        write = c.FA_WRITE,
        open_existing = c.FA_OPEN_EXISTING,
        create_new = c.FA_CREATE_NEW,
        create_always = c.FA_CREATE_ALWAYS,
        open_always = c.FA_OPEN_ALWAYS,
        open_append = c.FA_OPEN_APPEND,
    };

    pub fn open(path: []const u8, mode: u8) !@This() {
        var self: @This() = undefined;
        return if (FR_OK == c.f_open(&self.handle, @ptrCast(path), mode)) self else fRet.fr_error.generic_error;
    }

    pub fn close(self: *@This()) !void {
        if (FR_OK != c.f_close(&self.handle)) {
            return fRet.fr_error.generic_error;
        }
    }

    pub fn read(self: *@This(), buf: []u8, bytesToRead: u32) !usize {
        var bytesRead: usize = 0;

        var ret = c.f_read(&self.handle, @ptrCast(buf), if (bytesToRead > buf.len) buf.len else bytesToRead, &bytesRead);

        return if (ret == FR_OK) bytesRead else fRet.fr_error.generic_error;
    }

    pub fn write(self: *@This(), buf: []const u8, bytesToWrite: u32) !usize {
        var bytesWritten: usize = 0;

        var ret = c.f_write(&self.handle, @ptrCast(buf), if (bytesToWrite > buf.len) buf.len else bytesToWrite, &bytesWritten);
        return if (ret == FR_OK) bytesWritten else fRet.fr_error.generic_error;
    }

    pub fn sync(self: *@This()) bool {
        return (FR_OK == c.f_sync(&self.handle));
    }

    pub fn size(self: *@This()) usize {
        return c.f_size(&self.handle);
    }

    pub fn lseek(self: *@This(), offset: usize) bool {
        return (FR_OK == c.f_lseek(&self.handle, offset));
    }

    pub fn tell(self: *@This()) usize {
        return c.f_tell(&self.handle);
    }

    pub fn rewind(self: *@This()) bool {
        return self.lseek(0);
    }

    pub fn eof(self: *@This()) bool {
        return (0 != self.f_eof(&self.handle));
    }
};

pub fn mount() !bool {
    var ret = c.f_mount(&fileSystem, "SD", 1);

    return (FR_OK == ret);
}

pub fn unmount() bool {
    return (FR_OK == c.f_unmount("SD"));
}
