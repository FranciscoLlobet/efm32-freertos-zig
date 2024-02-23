const std = @import("std");
const microzig = @import("microzig");

const include_path = [_][]const u8{
    "csrc/mcuboot/boot/bootutil/include",
    "csrc/mcuboot/boot/zig-freertos",
};

const source_path = [_][]const u8{
    "csrc/mcuboot/boot/bootutil/src/boot_record.c",
    "csrc/mcuboot/boot/bootutil/src/bootutil_misc.c",
    "csrc/mcuboot/boot/bootutil/src/bootutil_public.c",
    "csrc/mcuboot/boot/bootutil/src/caps.c",
    "csrc/mcuboot/boot/bootutil/src/encrypted.c",
    "csrc/mcuboot/boot/bootutil/src/fault_injection_hardening.c",
    "csrc/mcuboot/boot/bootutil/src/fault_injection_hardening_delay_rng_mbedtls.c",
    "csrc/mcuboot/boot/bootutil/src/image_ecdsa.c",
    "csrc/mcuboot/boot/bootutil/src/image_validate.c",
    "csrc/mcuboot/boot/bootutil/src/loader.c",
    "csrc/mcuboot/boot/bootutil/src/swap_misc.c",
    "csrc/mcuboot/boot/bootutil/src/swap_move.c",
    "csrc/mcuboot/boot/bootutil/src/swap_scratch.c",
    "csrc/mcuboot/boot/bootutil/src/tlv.c",
};

const c_flags = [_][]const u8{ "-DEFM32GG390F1024", "-O2", "-fdata-sections", "-ffunction-sections" };

pub fn aggregate(exe: *microzig.Firmware) void {
    for (include_path) |path| {
        exe.addIncludePath(.{ .path = path });
    }

    for (source_path) |path| {
        exe.addCSourceFile(.{ .file = .{ .path = path }, .flags = &c_flags });
    }
}
