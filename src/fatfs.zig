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
const FATFS = c.FATFS;

var fileSystem: c.FATFS = undefined;

fp: *c.FIL,

pub export fn miso_getFs() callconv(.C) *FATFS {
    return &fileSystem;
}

pub fn setFs(fs: *FATFS) void {
    fileSystem = fs;
}

pub fn mount() bool {
    return (FR_OK == c.f_mount(&fileSystem, "SD", 1));
}

pub fn unmount() bool {
    return (FR_OK == c.f_unmount("SD"));
}

pub fn open(self: *@This(), path: []u8, mode: u8) bool {
    return (FR_OK == c.f_open(self.fp, path, mode));
}

pub fn close(self: *@This()) bool {
    return (FR_OK == c.f_close(self.fp));
}

pub fn read(self: *@This(), buf: ?anyopaque, btr: u32, br: *u32) bool {
    return (FR_OK == c.f_read(self, buf, btr, br));
}
