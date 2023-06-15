const std = @import("std");
const microzig = @import("deps/microzig/build.zig");

pub const boards = @import("src/boards.zig");
pub const chips = @import("src/chips.zig");
// Although this function looks imperative, note that its job is to
// declaratively construct a build graph that will be executed by an external
// runner.
pub fn build(b: *std.build.Builder) void {
    const optimize = b.standardOptimizeOption(.{});
    inline for (@typeInfo(boards).Struct.decls) |decl| {
        if (!decl.is_pub)
            continue;

        const exe = microzig.addEmbeddedExecutable(b, .{
            .name = @field(boards, decl.name).name ++ ".minimal",
            .source_file = .{
                .path = "src/main.zig",
            },
            .backing = .{ .board = @field(boards, decl.name) },
            .optimize = optimize,
            // .linkerscript_source_file = .{ .path = "csrc/efm32gg.ld" },
        });
        exe.addSystemIncludePath("C:\\Program Files (x86)\\Arm GNU Toolchain arm-none-eabi\\12.2 mpacbti-rel1\\arm-none-eabi\\include");
        exe.addObjectFile("C:\\Program Files (x86)\\Arm GNU Toolchain arm-none-eabi\\12.2 mpacbti-rel1\\lib\\gcc\\arm-none-eabi\\12.2.1\\thumb\\v6-m\\nofp\\libc_nano.a");
        exe.addObjectFile("C:\\Program Files (x86)\\Arm GNU Toolchain arm-none-eabi\\12.2 mpacbti-rel1\\lib\\gcc\\arm-none-eabi\\12.2.1\\thumb\\v6-m\\nofp\\libgcc.a");
        exe.addIncludePath("csrc/system/config");
        exe.addIncludePath("csrc/system/cmsis");
        exe.addIncludePath("csrc/system/EFM32GG390F1024");
        exe.addIncludePath("csrc/system/gecko_sdk/common/inc/");
        exe.addIncludePath("csrc/system/gecko_sdk/common/errno/inc/");
        exe.addIncludePath("csrc/system/gecko_sdk/emlib/inc/");
        exe.addIncludePath("csrc/system/gecko_sdk/emlib/host/inc");

        exe.addIncludePath("csrc/system/gecko_sdk/emdrv/common/inc");
        exe.addIncludePath("csrc/system/gecko_sdk/emdrv/dmadrv/inc");
        exe.addIncludePath("csrc/system/gecko_sdk/emdrv/emcode/inc");
        exe.addIncludePath("csrc/system/gecko_sdk/emdrv/gpiointerrupt/inc");
        exe.addIncludePath("csrc/system/gecko_sdk/emdrv/spidrv/inc");
        exe.addIncludePath("csrc/system/gecko_sdk/emdrv/nvm3/inc");

        exe.addIncludePath("csrc/system/gecko_sdk/driver/button/inc");
        exe.addIncludePath("csrc/system/gecko_sdk/driver/debug/inc");
        exe.addIncludePath("csrc/system/gecko_sdk/driver/i2cspm/inc");
        exe.addIncludePath("csrc/system/gecko_sdk/driver/leddrv/inc");

        exe.addIncludePath("csrc/system/gecko_sdk/service/device_init/inc");
        exe.addIncludePath("csrc/system/gecko_sdk/service/udelay/inc");
        exe.addIncludePath("csrc/system/gecko_sdk/service/sleeptimer/inc");
        exe.addIncludePath("csrc/system/gecko_sdk/service/iostream/inc");
        exe.addIncludePath("csrc/system/gecko_sdk/service/power_manager/inc");

        exe.addIncludePath("csrc/board/inc");
        exe.addIncludePath("csrc/inc");

        exe.addIncludePath("csrc/system/FreeRTOS-Kernel/include");
        exe.addIncludePath("csrc/system/FreeRTOS-Kernel/portable/GCC/ARM_CM3");

        exe.addCSourceFile("csrc/system/newlib/assert.c", &[_][]const u8{"-DEFM32GG390F1024"});
        exe.addCSourceFile("csrc/system/newlib/exit.c", &[_][]const u8{"-DEFM32GG390F1024"});
        exe.addCSourceFile("csrc/system/newlib/sbrk.c", &[_][]const u8{"-DEFM32GG390F1024"});
        //exe.addCSourceFile("csrc/system/newlib/startup.c", &[_][]const u8{"-DEFM32GG390F1024"});
        exe.addCSourceFile("csrc/system/newlib/syscalls.c", &[_][]const u8{"-DEFM32GG390F1024"});

        exe.addCSourceFile("csrc/system/cortexm/initialize-hardware.c", &[_][]const u8{"-DEFM32GG390F1024"});
        exe.addCSourceFile("csrc/system/cortexm/exception-handlers.c", &[_][]const u8{"-DEFM32GG390F1024"});
        exe.addCSourceFile("csrc/system/cortexm/reset-hardware.c", &[_][]const u8{"-DEFM32GG390F1024"});

        exe.addCSourceFile("csrc/system/gecko_sdk/common/src/sl_assert.c", &[_][]const u8{"-DEFM32GG390F1024"});
        exe.addCSourceFile("csrc/system/gecko_sdk/common/src/sl_slist.c", &[_][]const u8{"-DEFM32GG390F1024"});
        exe.addCSourceFile("csrc/system/gecko_sdk/common/errno/src/sl_errno.c", &[_][]const u8{"-DEFM32GG390F1024"});
        // Adding EMLIB essentials
        exe.addCSourceFile("csrc/system/gecko_sdk/emlib/src/em_acmp.c", &[_][]const u8{"-DEFM32GG390F1024"});
        exe.addCSourceFile("csrc/system/gecko_sdk/emlib/src/em_adc.c", &[_][]const u8{"-DEFM32GG390F1024"});
        exe.addCSourceFile("csrc/system/gecko_sdk/emlib/src/em_aes.c", &[_][]const u8{"-DEFM32GG390F1024"});
        exe.addCSourceFile("csrc/system/gecko_sdk/emlib/src/em_burtc.c", &[_][]const u8{"-DEFM32GG390F1024"});
        exe.addCSourceFile("csrc/system/gecko_sdk/emlib/src/em_can.c", &[_][]const u8{"-DEFM32GG390F1024"});
        exe.addCSourceFile("csrc/system/gecko_sdk/emlib/src/em_cmu_fpga.c", &[_][]const u8{"-DEFM32GG390F1024"});
        exe.addCSourceFile("csrc/system/gecko_sdk/emlib/src/em_cmu.c", &[_][]const u8{"-DEFM32GG390F1024"});
        exe.addCSourceFile("csrc/system/gecko_sdk/emlib/src/em_core.c", &[_][]const u8{"-DEFM32GG390F1024"});
        exe.addCSourceFile("csrc/system/gecko_sdk/emlib/src/em_cryotimer.c", &[_][]const u8{"-DEFM32GG390F1024"});
        exe.addCSourceFile("csrc/system/gecko_sdk/emlib/src/em_crypto.c", &[_][]const u8{"-DEFM32GG390F1024"});
        exe.addCSourceFile("csrc/system/gecko_sdk/emlib/src/em_csen.c", &[_][]const u8{"-DEFM32GG390F1024"});
        exe.addCSourceFile("csrc/system/gecko_sdk/emlib/src/em_dac.c", &[_][]const u8{"-DEFM32GG390F1024"});
        exe.addCSourceFile("csrc/system/gecko_sdk/emlib/src/em_dbg.c", &[_][]const u8{"-DEFM32GG390F1024"});
        exe.addCSourceFile("csrc/system/gecko_sdk/emlib/src/em_dma.c", &[_][]const u8{"-DEFM32GG390F1024"});
        exe.addCSourceFile("csrc/system/gecko_sdk/emlib/src/em_ebi.c", &[_][]const u8{"-DEFM32GG390F1024"});
        exe.addCSourceFile("csrc/system/gecko_sdk/emlib/src/em_emu.c", &[_][]const u8{"-DEFM32GG390F1024"});
        exe.addCSourceFile("csrc/system/gecko_sdk/emlib/src/em_eusart.c", &[_][]const u8{"-DEFM32GG390F1024"});
        exe.addCSourceFile("csrc/system/gecko_sdk/emlib/src/em_gpcrc.c", &[_][]const u8{"-DEFM32GG390F1024"});
        exe.addCSourceFile("csrc/system/gecko_sdk/emlib/src/em_gpio.c", &[_][]const u8{"-DEFM32GG390F1024"});
        exe.addCSourceFile("csrc/system/gecko_sdk/emlib/src/em_i2c.c", &[_][]const u8{"-DEFM32GG390F1024"});
        exe.addCSourceFile("csrc/system/gecko_sdk/emlib/src/em_iadc.c", &[_][]const u8{"-DEFM32GG390F1024"});
        exe.addCSourceFile("csrc/system/gecko_sdk/emlib/src/em_idac.c", &[_][]const u8{"-DEFM32GG390F1024"});
        exe.addCSourceFile("csrc/system/gecko_sdk/emlib/src/em_lcd.c", &[_][]const u8{"-DEFM32GG390F1024"});
        exe.addCSourceFile("csrc/system/gecko_sdk/emlib/src/em_ldma.c", &[_][]const u8{"-DEFM32GG390F1024"});
        exe.addCSourceFile("csrc/system/gecko_sdk/emlib/src/em_lesense.c", &[_][]const u8{"-DEFM32GG390F1024"});
        exe.addCSourceFile("csrc/system/gecko_sdk/emlib/src/em_letimer.c", &[_][]const u8{"-DEFM32GG390F1024"});
        exe.addCSourceFile("csrc/system/gecko_sdk/emlib/src/em_leuart.c", &[_][]const u8{"-DEFM32GG390F1024"});
        exe.addCSourceFile("csrc/system/gecko_sdk/emlib/src/em_msc.c", &[_][]const u8{"-DEFM32GG390F1024"});
        exe.addCSourceFile("csrc/system/gecko_sdk/emlib/src/em_opamp.c", &[_][]const u8{"-DEFM32GG390F1024"});
        exe.addCSourceFile("csrc/system/gecko_sdk/emlib/src/em_pcnt.c", &[_][]const u8{"-DEFM32GG390F1024"});
        exe.addCSourceFile("csrc/system/gecko_sdk/emlib/src/em_pdm.c", &[_][]const u8{"-DEFM32GG390F1024"});
        exe.addCSourceFile("csrc/system/gecko_sdk/emlib/src/em_prs.c", &[_][]const u8{"-DEFM32GG390F1024"});
        exe.addCSourceFile("csrc/system/gecko_sdk/emlib/src/em_qspi.c", &[_][]const u8{"-DEFM32GG390F1024"});
        exe.addCSourceFile("csrc/system/gecko_sdk/emlib/src/em_rmu.c", &[_][]const u8{"-DEFM32GG390F1024"});
        exe.addCSourceFile("csrc/system/gecko_sdk/emlib/src/em_rtc.c", &[_][]const u8{"-DEFM32GG390F1024"});
        exe.addCSourceFile("csrc/system/gecko_sdk/emlib/src/em_rtcc.c", &[_][]const u8{"-DEFM32GG390F1024"});
        exe.addCSourceFile("csrc/system/gecko_sdk/emlib/src/em_se.c", &[_][]const u8{"-DEFM32GG390F1024"});
        exe.addCSourceFile("csrc/system/gecko_sdk/emlib/src/em_system.c", &[_][]const u8{"-DEFM32GG390F1024"});
        exe.addCSourceFile("csrc/system/gecko_sdk/emlib/src/em_timer.c", &[_][]const u8{"-DEFM32GG390F1024"});
        exe.addCSourceFile("csrc/system/gecko_sdk/emlib/src/em_usart.c", &[_][]const u8{"-DEFM32GG390F1024"});
        exe.addCSourceFile("csrc/system/gecko_sdk/emlib/src/em_vcmp.c", &[_][]const u8{"-DEFM32GG390F1024"});
        exe.addCSourceFile("csrc/system/gecko_sdk/emlib/src/em_vdac.c", &[_][]const u8{"-DEFM32GG390F1024"});
        exe.addCSourceFile("csrc/system/gecko_sdk/emlib/src/em_wdog.c", &[_][]const u8{"-DEFM32GG390F1024"});

        exe.addCSourceFile("csrc/system/gecko_sdk/emdrv/dmadrv/src/dmactrl.c", &[_][]const u8{"-DEFM32GG390F1024"});
        exe.addCSourceFile("csrc/system/gecko_sdk/emdrv/dmadrv/src/dmadrv.c", &[_][]const u8{"-DEFM32GG390F1024"});
        exe.addCSourceFile("csrc/system/gecko_sdk/emdrv/gpiointerrupt/src/gpiointerrupt.c", &[_][]const u8{"-DEFM32GG390F1024"});
        exe.addCSourceFile("csrc/system/gecko_sdk/emdrv/spidrv/src/spidrv.c", &[_][]const u8{"-DEFM32GG390F1024 -DSL_CATALOG_POWER_MANAGER_PRESENT=1"});

        exe.addCSourceFile("csrc/system/gecko_sdk/driver/button/src/sl_button.c", &[_][]const u8{"-DEFM32GG390F1024"});
        exe.addCSourceFile("csrc/system/gecko_sdk/driver/button/src/sl_simple_button.c", &[_][]const u8{"-DEFM32GG390F1024"});
        exe.addCSourceFile("csrc/system/gecko_sdk/driver/debug/src/sl_debug_swo.c", &[_][]const u8{"-DEFM32GG390F1024"});
        exe.addCSourceFile("csrc/system/gecko_sdk/driver/i2cspm/src/sl_i2cspm.c", &[_][]const u8{"-DEFM32GG390F1024"});
        exe.addCSourceFile("csrc/system/gecko_sdk/driver/leddrv/src/sl_led.c", &[_][]const u8{"-DEFM32GG390F1024"});
        exe.addCSourceFile("csrc/system/gecko_sdk/driver/leddrv/src/sl_simple_led.c", &[_][]const u8{"-DEFM32GG390F1024"});
        // Adding udelay Helper
        exe.addCSourceFile("csrc/system/gecko_sdk/service/udelay/src/sl_udelay.c", &[_][]const u8{"-DEFM32GG390F1024"});
        exe.addCSourceFile("csrc/system/gecko_sdk/service/udelay/src/sl_udelay_armv6m_gcc.S", &[_][]const u8{"-DEFM32GG390F1024"});
        //exe.add

        // Adding Device-Init Helpers
        exe.addCSourceFile("csrc/system/gecko_sdk/service/device_init/src/sl_device_init_emu_s0.c", &[_][]const u8{"-DEFM32GG390F1024"});
        exe.addCSourceFile("csrc/system/gecko_sdk/service/device_init/src/sl_device_init_hfrco.c", &[_][]const u8{"-DEFM32GG390F1024"});
        exe.addCSourceFile("csrc/system/gecko_sdk/service/device_init/src/sl_device_init_hfxo_s0.c", &[_][]const u8{"-DEFM32GG390F1024"});
        exe.addCSourceFile("csrc/system/gecko_sdk/service/device_init/src/sl_device_init_lfrco.c", &[_][]const u8{"-DEFM32GG390F1024"});
        exe.addCSourceFile("csrc/system/gecko_sdk/service/device_init/src/sl_device_init_lfxo_s0.c", &[_][]const u8{"-DEFM32GG390F1024"});
        exe.addCSourceFile("csrc/system/gecko_sdk/service/device_init/src/sl_device_init_nvic.c", &[_][]const u8{"-DEFM32GG390F1024"});
        exe.addCSourceFile("csrc/system/gecko_sdk/service/power_manager/src/sl_power_manager_debug.c", &[_][]const u8{"-DEFM32GG390F1024 -DSL_CATALOG_POWER_MANAGER_PRESENT=1"});
        exe.addCSourceFile("csrc/system/gecko_sdk/service/power_manager/src/sl_power_manager_hal_s0_s1.c", &[_][]const u8{"-DEFM32GG390F1024 -DSL_CATALOG_POWER_MANAGER_PRESENT=1"});
        exe.addCSourceFile("csrc/system/gecko_sdk/service/power_manager/src/sl_power_manager.c", &[_][]const u8{"-DEFM32GG390F1024 -DSL_CATALOG_POWER_MANAGER_PRESENT=1"});

        // Adding the sleeptimer service
        exe.addCSourceFile("csrc/system/gecko_sdk/service/sleeptimer/src/sl_sleeptimer_hal_burtc.c", &[_][]const u8{"-DEFM32GG390F1024 -DSL_CATALOG_POWER_MANAGER_PRESENT=1"});
        exe.addCSourceFile("csrc/system/gecko_sdk/service/sleeptimer/src/sl_sleeptimer_hal_rtc.c", &[_][]const u8{"-DEFM32GG390F1024 -DSL_CATALOG_POWER_MANAGER_PRESENT=1"});
        exe.addCSourceFile("csrc/system/gecko_sdk/service/sleeptimer/src/sl_sleeptimer.c", &[_][]const u8{"-DEFM32GG390F1024 -DSL_CATALOG_POWER_MANAGER_PRESENT=1"});

        // Adding the iostream
        exe.addCSourceFile("csrc/system/gecko_sdk/service/iostream/src/sl_iostream_debug.c", &[_][]const u8{"-DEFM32GG390F1024"});
        exe.addCSourceFile("csrc/system/gecko_sdk/service/iostream/src/sl_iostream_retarget_stdio.c", &[_][]const u8{"-DEFM32GG390F1024"});
        exe.addCSourceFile("csrc/system/gecko_sdk/service/iostream/src/sl_iostream_stdio.c", &[_][]const u8{"-DEFM32GG390F1024"});
        exe.addCSourceFile("csrc/system/gecko_sdk/service/iostream/src/sl_iostream_swo_itm_8.c", &[_][]const u8{"-DEFM32GG390F1024 -DSL_CATALOG_POWER_MANAGER_PRESENT=1"});
        exe.addCSourceFile("csrc/system/gecko_sdk/service/iostream/src/sl_iostream_swo.c", &[_][]const u8{"-DEFM32GG390F1024"});
        exe.addCSourceFile("csrc/system/gecko_sdk/service/iostream/src/sl_iostream.c", &[_][]const u8{"-DEFM32GG390F1024"});

        //exe.addCSourceFile("csrc/system/cmsis/startup_efm32gg.c", &[_][]const u8{"-DEFM32GG390F1024 -D__NO_SYSTEM_INIT=1"});

        exe.addCSourceFile("csrc/board/src/board.c", &[_][]const u8{"-DEFM32GG390F1024"});
        exe.addCSourceFile("csrc/board/src/board_leds.c", &[_][]const u8{"-DEFM32GG390F1024"});
        exe.addCSourceFile("csrc/board/src/board_buttons.c", &[_][]const u8{"-DEFM32GG390F1024"});
        exe.addCSourceFile("csrc/board/src/board_watchdog.c", &[_][]const u8{"-DEFM32GG390F1024"});
        exe.addCSourceFile("csrc/board/src/system_efm32gg.c", &[_][]const u8{"-DEFM32GG390F1024 -D__Vectors=\"VectorTable\""});

        exe.addCSourceFile("csrc/system/FreeRTOS-Kernel/croutine.c", &[_][]const u8{"-DEFM32GG390F1024"});
        exe.addCSourceFile("csrc/system/FreeRTOS-Kernel/list.c", &[_][]const u8{"-DEFM32GG390F1024"});
        exe.addCSourceFile("csrc/system/FreeRTOS-Kernel/queue.c", &[_][]const u8{"-DEFM32GG390F1024"});
        exe.addCSourceFile("csrc/system/FreeRTOS-Kernel/stream_buffer.c", &[_][]const u8{"-DEFM32GG390F1024"});
        exe.addCSourceFile("csrc/system/FreeRTOS-Kernel/tasks.c", &[_][]const u8{"-DEFM32GG390F1024"});
        exe.addCSourceFile("csrc/system/FreeRTOS-Kernel/timers.c", &[_][]const u8{"-DEFM32GG390F1024"});

        exe.addCSourceFile("csrc/system/FreeRTOS-Kernel/portable/GCC/ARM_CM3/port.c", &[_][]const u8{"-DEFM32GG390F1024"});
        exe.addCSourceFile("csrc/system/FreeRTOS-Kernel/portable/MemMang/heap_4.c", &[_][]const u8{"-DEFM32GG390F1024"});
        exe.installArtifact(b);
    }

    inline for (@typeInfo(chips).Struct.decls) |decl| {
        if (!decl.is_pub)
            continue;

        const exe = microzig.addEmbeddedExecutable(b, .{
            .name = @field(chips, decl.name).name ++ ".minimal",
            .source_file = .{
                .path = "test/programs/minimal.zig",
            },
            .backing = .{ .chip = @field(chips, decl.name) },
            .optimize = optimize,
        });
        exe.installArtifact(b);
    }
}
