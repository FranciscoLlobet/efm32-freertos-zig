const std = @import("std");
const mbedtls = @import("../mbedtls.zig");
const sha256 = @import("../sha256.zig");
const pk = @import("../pk.zig");
const board = @import("microzig").board;
const config = @import("../config.zig");

pub fn checkFirmwareImage() !void {
    var firmwareImageHashSpace: [32]u8 = undefined;
    @memset(&firmwareImageHashSpace, 0);

    const firmwareHash = try config.calculateFileHash(config.fw_file_name, &firmwareImageHashSpace);

    _ = firmwareHash;
}
