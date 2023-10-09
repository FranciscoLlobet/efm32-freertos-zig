/// Light FatFs http://elm-chan.org/fsw/ff/00index_e.html) facade for use in Zig
///
/// This facade wraps most, but not all of the functions provided by FatFs.
/// For simplicity, it also does not go into the Hardware access interface (diskio.h)
/// It is responsibility of the systems programmer to implement the corresponding adapting code.
///
/// Read http://elm-chan.org/fsw/ff/doc/appnote.html for the application notes.
///
/// Tested on FreeRTOS with a standard SD card
///
const c = @cImport({
    @cInclude("ff.h");
});

const FSIZE_t = c.FSIZE_t;
const UINT = c.UINT;
const FR_OK = c.FR_OK;
const FILINFO = c.FILINFO;
const FRESULT = c.FRESULT;

pub const FATFS = c.FATFS;

pub const frError = error{ FR_DISK_ERR, FR_INT_ERR, FR_NOT_READY, FR_NO_FILE, FR_NO_PATH, FR_INVALID_NAME, FR_DENIED, FR_EXIST, FR_INVALID_OBJECT, FR_WRITE_PROTECTED, FR_INVALID_DRIVE, FR_NOT_ENABLED, FR_NO_FILESYSTEM, FR_MKFS_ABORTED, FR_TIMEOUT, FR_LOCKED, FR_NOT_ENOUGH_CORE, FR_TOO_MANY_OPEN_FILES, FR_INVALID_PARAMETER, generic_error, buffer_overflow };

const fRet = enum(c_uint) {
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
    generic_error,

    fn check(ret: FRESULT) frError!void {
        return switch (@as(@This(), @enumFromInt(ret))) {
            .fr_ok => {},
            .fr_disk_err => frError.FR_DISK_ERR,
            .fr_int_err => frError.FR_INT_ERR,
            .fr_not_ready => frError.FR_NOT_READY,
            .fr_no_file => frError.FR_NO_FILE,
            .fr_no_path => frError.FR_NO_PATH,
            .fr_invalid_name => frError.FR_INVALID_NAME,
            .fr_denied => frError.FR_DENIED,
            .fr_exist => frError.FR_EXIST,
            .fr_invalid_object => frError.FR_INVALID_OBJECT,
            .fr_write_protected => frError.FR_WRITE_PROTECTED,
            .fr_invalid_drive => frError.FR_INVALID_DRIVE,
            .fr_not_enabled => frError.FR_NOT_ENABLED,
            .fr_no_filesystem => frError.FR_NO_FILESYSTEM,
            .fr_mkfs_aborted => frError.FR_MKFS_ABORTED,
            .fr_timeout => frError.FR_TIMEOUT,
            .fr_locked => frError.FR_LOCKED,
            .fr_not_enough_core => frError.FR_NOT_ENOUGH_CORE,
            .fr_too_many_open_files => frError.FR_TOO_MANY_OPEN_FILES,
            .fr_invalid_parameter => frError.FR_INVALID_PARAMETER,
            else => frError.generic_error,
        };
    }
};

// Global variable for FS
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

    pub fn open(path: [*:0]const u8, mode: u8) frError!@This() {
        var self: @This() = undefined;

        try fRet.check(c.f_open(&self.handle, path, mode));

        return self;
    }

    pub fn close(self: *@This()) frError!void {
        try fRet.check(c.f_close(&self.handle));
    }

    pub fn read(self: *@This(), buf: []u8) frError![]u8 {
        var bytesRead: usize = 0;

        try fRet.check(c.f_read(&self.handle, buf.ptr, buf.len, &bytesRead));

        return if (bytesRead > buf.len) frError.buffer_overflow else buf[0..bytesRead];
    }

    pub fn write(self: *@This(), buf: []const u8) frError!usize {
        var bytesWritten: usize = 0;

        try fRet.check(c.f_write(&self.handle, buf.ptr, buf.len, &bytesWritten));

        return bytesWritten;
    }

    pub fn sync(self: *@This()) frError!void {
        return fRet.check(c.f_sync(&self.handle));
    }

    pub fn size(self: *@This()) usize {
        return c.f_size(&self.handle);
    }

    pub fn lseek(self: *@This(), offset: usize) frError!void {
        try fRet.check(c.f_lseek(&self.handle, offset));
    }

    pub fn tell(self: *@This()) usize {
        return c.f_tell(&self.handle);
    }

    pub fn rewind(self: *@This()) frError!void {
        try self.lseek(0);
    }

    pub fn eof(self: *@This()) bool {
        return (0 != c.f_eof(&self.handle));
    }
};

pub fn mount(volume: [*:0]const u8) frError!void {
    try fRet.check(c.f_mount(&fileSystem, volume, 1));
}

pub fn unmount(volume: [*:0]const u8) frError!void {
    try fRet.check(c.f_unmount(volume));
}
