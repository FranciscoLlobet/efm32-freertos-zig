const std = @import("std");
const mbedtls = @import("../mbedtls.zig");
const sha256 = @import("../sha256.zig");
const pk = @import("../pk.zig");
const board = @import("microzig").board;
const config = @import("../config.zig");

/// Check the firmware image signature
pub fn checkFirmwareImage() !void {
    try config.verifyFirmwareSignature(config.fw_file_name, config.fw_sig_file_name, config.getHttpSigKey());
}

/// Perform the firmware backup
pub fn backupFirmware() !void {
    //
}
