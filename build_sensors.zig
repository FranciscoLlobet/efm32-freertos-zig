const std = @import("std");
const microzig = @import("microzig");

const include_path = [_][]const u8{
    "csrc/sensors/BMA2-Sensor-API",
    "csrc/sensors/BME280_driver",
    "csrc/sensors/BMG160_driver",
    "csrc/sensors/BMI160_driver",
    "csrc/sensors/BMM150-Sensor-API",
};

const source_path = [_][]const u8{
    "csrc/sensors/BMA2-Sensor-API/bma2.c",
    "csrc/sensors/BME280_driver/bme280.c",
    "csrc/sensors/BMG160_driver/bmg160.c",
    "csrc/sensors/BMI160_driver/bmi160.c",
    "csrc/sensors/BMM150-Sensor-API/bmm150.c",
};

const c_flags = [_][]const u8{ "-O2", "-DEFM32GG390F1024", "-fdata-sections", "-ffunction-sections" };

pub fn aggregate(exe: *microzig.Firmware) void {
    for (include_path) |path| {
        exe.addIncludePath(.{ .path = path });
    }

    for (source_path) |path| {
        exe.addCSourceFile(.{ .file = .{ .path = path }, .flags = &c_flags });
    }
}
