const std = @import("std");
const microzig = @import("deps/microzig/build.zig");

const c_flags = [_][]const u8{ "-DEFM32GG390F1024", "-O2", "-DSL_CATALOG_POWER_MANAGER_PRESENT=1", "-DEFM_DEBUG", "-fdata-sections", "-ffunction-sections" };

const include_path = [_][]const u8{
    // Gecko-SDK Defines
    "csrc/system/EFM32GG390F1024",
    "csrc/system/gecko_sdk/common/inc/",
    "csrc/system/gecko_sdk/common/errno/inc/",
    "csrc/system/gecko_sdk/emlib/inc/",
    "csrc/system/gecko_sdk/emlib/host/inc",
    "csrc/system/gecko_sdk/emdrv/common/inc",
    "csrc/system/gecko_sdk/emdrv/dmadrv/inc",
    "csrc/system/gecko_sdk/emdrv/emcode/inc",
    "csrc/system/gecko_sdk/emdrv/gpiointerrupt/inc",
    "csrc/system/gecko_sdk/emdrv/spidrv/inc",
    "csrc/system/gecko_sdk/emdrv/uartdrv/inc",
    "csrc/system/gecko_sdk/emdrv/nvm3/inc",
    "csrc/system/gecko_sdk/driver/button/inc",
    "csrc/system/gecko_sdk/driver/debug/inc",
    "csrc/system/gecko_sdk/driver/i2cspm/inc",
    "csrc/system/gecko_sdk/driver/leddrv/inc",
    "csrc/system/gecko_sdk/service/device_init/inc",
    "csrc/system/gecko_sdk/service/udelay/inc",
    "csrc/system/gecko_sdk/service/sleeptimer/inc",
    "csrc/system/gecko_sdk/service/iostream/inc",
    "csrc/system/gecko_sdk/service/power_manager/inc",
    "csrc/system/gecko_sdk/middleware/usb_gecko/inc",
    "csrc/system/gecko_sdk/middleware/usbxpress/inc/",
};

const source_paths = [_][]const u8{
    // Adding EMLIB essentials
    "csrc/system/gecko_sdk/common/src/sl_assert.c",
    "csrc/system/gecko_sdk/common/src/sl_slist.c",
    //"csrc/system/gecko_sdk/common/errno/src/sl_errno.c",
    "csrc/system/gecko_sdk/emlib/src/em_acmp.c",
    "csrc/system/gecko_sdk/emlib/src/em_adc.c",
    "csrc/system/gecko_sdk/emlib/src/em_aes.c",
    "csrc/system/gecko_sdk/emlib/src/em_burtc.c",
    "csrc/system/gecko_sdk/emlib/src/em_can.c",
    "csrc/system/gecko_sdk/emlib/src/em_cmu_fpga.c",
    "csrc/system/gecko_sdk/emlib/src/em_cmu.c",
    "csrc/system/gecko_sdk/emlib/src/em_core.c",
    "csrc/system/gecko_sdk/emlib/src/em_cryotimer.c",
    "csrc/system/gecko_sdk/emlib/src/em_crypto.c",
    "csrc/system/gecko_sdk/emlib/src/em_csen.c",
    "csrc/system/gecko_sdk/emlib/src/em_dac.c",
    "csrc/system/gecko_sdk/emlib/src/em_dbg.c",
    "csrc/system/gecko_sdk/emlib/src/em_dma.c",
    "csrc/system/gecko_sdk/emlib/src/em_ebi.c",
    "csrc/system/gecko_sdk/emlib/src/em_emu.c",
    "csrc/system/gecko_sdk/emlib/src/em_eusart.c",
    "csrc/system/gecko_sdk/emlib/src/em_gpcrc.c",
    "csrc/system/gecko_sdk/emlib/src/em_gpio.c",
    "csrc/system/gecko_sdk/emlib/src/em_i2c.c",
    "csrc/system/gecko_sdk/emlib/src/em_iadc.c",
    "csrc/system/gecko_sdk/emlib/src/em_idac.c",
    "csrc/system/gecko_sdk/emlib/src/em_lcd.c",
    "csrc/system/gecko_sdk/emlib/src/em_ldma.c",
    "csrc/system/gecko_sdk/emlib/src/em_lesense.c",
    "csrc/system/gecko_sdk/emlib/src/em_letimer.c",
    "csrc/system/gecko_sdk/emlib/src/em_leuart.c",
    "csrc/system/gecko_sdk/emlib/src/em_msc.c",
    "csrc/system/gecko_sdk/emlib/src/em_opamp.c",
    "csrc/system/gecko_sdk/emlib/src/em_pcnt.c",
    "csrc/system/gecko_sdk/emlib/src/em_pdm.c",
    "csrc/system/gecko_sdk/emlib/src/em_prs.c",
    "csrc/system/gecko_sdk/emlib/src/em_qspi.c",
    "csrc/system/gecko_sdk/emlib/src/em_rmu.c",
    "csrc/system/gecko_sdk/emlib/src/em_rtc.c",
    "csrc/system/gecko_sdk/emlib/src/em_rtcc.c",
    "csrc/system/gecko_sdk/emlib/src/em_se.c",
    "csrc/system/gecko_sdk/emlib/src/em_system.c",
    "csrc/system/gecko_sdk/emlib/src/em_timer.c",
    "csrc/system/gecko_sdk/emlib/src/em_usart.c",
    "csrc/system/gecko_sdk/emlib/src/em_vcmp.c",
    "csrc/system/gecko_sdk/emlib/src/em_vdac.c",
    "csrc/system/gecko_sdk/emlib/src/em_wdog.c",

    "csrc/system/gecko_sdk/emdrv/dmadrv/src/dmactrl.c",
    "csrc/system/gecko_sdk/emdrv/dmadrv/src/dmadrv.c",
    "csrc/system/gecko_sdk/emdrv/gpiointerrupt/src/gpiointerrupt.c",
    "csrc/system/gecko_sdk/emdrv/spidrv/src/spidrv.c",
    "csrc/system/gecko_sdk/emdrv/uartdrv/src/uartdrv.c",

    "csrc/system/gecko_sdk/driver/button/src/sl_button.c",
    "csrc/system/gecko_sdk/driver/button/src/sl_simple_button.c",
    "csrc/system/gecko_sdk/driver/debug/src/sl_debug_swo.c",
    "csrc/system/gecko_sdk/driver/i2cspm/src/sl_i2cspm.c",
    "csrc/system/gecko_sdk/driver/leddrv/src/sl_led.c",
    "csrc/system/gecko_sdk/driver/leddrv/src/sl_simple_led.c",
    // Adding udelay Helper
    "csrc/system/gecko_sdk/service/udelay/src/sl_udelay.c",
    "csrc/system/gecko_sdk/service/udelay/src/sl_udelay_armv6m_gcc.S",
    // Adding Device-Init Helpers
    "csrc/system/gecko_sdk/service/device_init/src/sl_device_init_emu_s0.c",
    "csrc/system/gecko_sdk/service/device_init/src/sl_device_init_hfrco.c",
    "csrc/system/gecko_sdk/service/device_init/src/sl_device_init_hfxo_s0.c",
    "csrc/system/gecko_sdk/service/device_init/src/sl_device_init_lfrco.c",
    "csrc/system/gecko_sdk/service/device_init/src/sl_device_init_lfxo_s0.c",
    "csrc/system/gecko_sdk/service/device_init/src/sl_device_init_nvic.c",
    "csrc/system/gecko_sdk/service/power_manager/src/sl_power_manager_debug.c",
    "csrc/system/gecko_sdk/service/power_manager/src/sl_power_manager_hal_s0_s1.c",
    "csrc/system/gecko_sdk/service/power_manager/src/sl_power_manager.c",

    // Adding the sleeptimer service
    "csrc/system/gecko_sdk/service/sleeptimer/src/sl_sleeptimer_hal_burtc.c",
    "csrc/system/gecko_sdk/service/sleeptimer/src/sl_sleeptimer_hal_rtc.c",
    "csrc/system/gecko_sdk/service/sleeptimer/src/sl_sleeptimer.c",

    // Adding the iostream
    "csrc/system/gecko_sdk/service/iostream/src/sl_iostream_debug.c",
    "csrc/system/gecko_sdk/service/iostream/src/sl_iostream_retarget_stdio.c",
    "csrc/system/gecko_sdk/service/iostream/src/sl_iostream_stdio.c",
    "csrc/system/gecko_sdk/service/iostream/src/sl_iostream_swo_itm_8.c",
    "csrc/system/gecko_sdk/service/iostream/src/sl_iostream_swo.c",
    "csrc/system/gecko_sdk/service/iostream/src/sl_iostream.c",

    // Adding USB device
    "csrc/system/gecko_sdk/middleware/usb_gecko/src/em_usbhint.c",
    "csrc/system/gecko_sdk/middleware/usb_gecko/src/em_usbtimer.c",
    "csrc/system/gecko_sdk/middleware/usb_gecko/src/em_usbd.c",
    "csrc/system/gecko_sdk/middleware/usb_gecko/src/em_usbdch9.c",
    "csrc/system/gecko_sdk/middleware/usb_gecko/src/em_usbdep.c",
    "csrc/system/gecko_sdk/middleware/usb_gecko/src/em_usbdint.c",
    "csrc/system/gecko_sdk/middleware/usb_gecko/src/em_usbh.c",
    "csrc/system/gecko_sdk/middleware/usb_gecko/src/em_usbhal.c",
    "csrc/system/gecko_sdk/middleware/usb_gecko/src/em_usbhep.c",

    // Adding USB express
    "csrc/system/gecko_sdk/middleware/usbxpress/src/em_usbxpress_descriptors.c",
    "csrc/system/gecko_sdk/middleware/usbxpress/src/em_usbxpress.c",
    "csrc/system/gecko_sdk/middleware/usbxpress/src/em_usbxpress_callback.c",
};

pub fn aggregate(exe: *microzig.EmbeddedExecutable) void {
    for (include_path) |path| {
        exe.addIncludePath(path);
    }

    for (source_paths) |path| {
        exe.addCSourceFile(path, &c_flags);
    }
}
