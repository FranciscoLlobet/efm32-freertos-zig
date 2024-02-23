const std = @import("std");
const microzig = @import("microzig");

const c_flags = [_][]const u8{ "-DEFM32GG390F1024", "-O2", "-DSL_CATALOG_POWER_MANAGER_PRESENT=1", "-DEFM_DEBUG", "-DSL_RAMFUNC_DISABLE", "-DEM_MSC_RUN_FROM_FLASH", "-fdata-sections", "-ffunction-sections" };

const gecko_sdk_base_dir = "csrc/system/gecko_sdk/platform";

const include_path = [_][]const u8{
    // Gecko-SDK Defines
    gecko_sdk_base_dir ++ "/Device/SiliconLabs/EFM32GG/Include",
    gecko_sdk_base_dir ++ "/common/inc/",
    gecko_sdk_base_dir ++ "/common/errno/inc/",
    gecko_sdk_base_dir ++ "/emlib/inc/",
    gecko_sdk_base_dir ++ "/emlib/host/inc",
    gecko_sdk_base_dir ++ "/emdrv/common/inc",
    gecko_sdk_base_dir ++ "/emdrv/dmadrv/inc",
    gecko_sdk_base_dir ++ "/emdrv/emcode/inc",
    gecko_sdk_base_dir ++ "/emdrv/gpiointerrupt/inc",
    gecko_sdk_base_dir ++ "/emdrv/spidrv/inc",
    gecko_sdk_base_dir ++ "/emdrv/uartdrv/inc",
    gecko_sdk_base_dir ++ "/emdrv/nvm3/inc",
    gecko_sdk_base_dir ++ "/driver/button/inc",
    gecko_sdk_base_dir ++ "/driver/debug/inc",
    gecko_sdk_base_dir ++ "/driver/i2cspm/inc",
    gecko_sdk_base_dir ++ "/driver/leddrv/inc",
    gecko_sdk_base_dir ++ "/service/device_init/inc",
    gecko_sdk_base_dir ++ "/service/udelay/inc",
    gecko_sdk_base_dir ++ "/service/sleeptimer/inc",
    gecko_sdk_base_dir ++ "/service/iostream/inc",
    gecko_sdk_base_dir ++ "/service/power_manager/inc",
    gecko_sdk_base_dir ++ "/middleware/usb_gecko/inc",
    gecko_sdk_base_dir ++ "/middleware/usbxpress/inc/",
};

const source_paths = [_][]const u8{
    // Adding EMLIB essentials
    gecko_sdk_base_dir ++ "/common/src/sl_assert.c",
    gecko_sdk_base_dir ++ "/common/src/sl_slist.c",
    //gecko_sdk_base_dir ++ "/common/errno/src/sl_errno.c",
    gecko_sdk_base_dir ++ "/emlib/src/em_acmp.c",
    gecko_sdk_base_dir ++ "/emlib/src/em_adc.c",
    gecko_sdk_base_dir ++ "/emlib/src/em_aes.c",
    gecko_sdk_base_dir ++ "/emlib/src/em_burtc.c",
    gecko_sdk_base_dir ++ "/emlib/src/em_can.c",
    gecko_sdk_base_dir ++ "/emlib/src/em_cmu_fpga.c",
    gecko_sdk_base_dir ++ "/emlib/src/em_cmu.c",
    gecko_sdk_base_dir ++ "/emlib/src/em_core.c",
    gecko_sdk_base_dir ++ "/emlib/src/em_cryotimer.c",
    gecko_sdk_base_dir ++ "/emlib/src/em_crypto.c",
    gecko_sdk_base_dir ++ "/emlib/src/em_csen.c",
    gecko_sdk_base_dir ++ "/emlib/src/em_dac.c",
    gecko_sdk_base_dir ++ "/emlib/src/em_dbg.c",
    gecko_sdk_base_dir ++ "/emlib/src/em_dma.c",
    gecko_sdk_base_dir ++ "/emlib/src/em_ebi.c",
    gecko_sdk_base_dir ++ "/emlib/src/em_emu.c",
    gecko_sdk_base_dir ++ "/emlib/src/em_eusart.c",
    gecko_sdk_base_dir ++ "/emlib/src/em_gpcrc.c",
    gecko_sdk_base_dir ++ "/emlib/src/em_gpio.c",
    gecko_sdk_base_dir ++ "/emlib/src/em_i2c.c",
    gecko_sdk_base_dir ++ "/emlib/src/em_iadc.c",
    gecko_sdk_base_dir ++ "/emlib/src/em_idac.c",
    gecko_sdk_base_dir ++ "/emlib/src/em_lcd.c",
    gecko_sdk_base_dir ++ "/emlib/src/em_ldma.c",
    gecko_sdk_base_dir ++ "/emlib/src/em_lesense.c",
    gecko_sdk_base_dir ++ "/emlib/src/em_letimer.c",
    gecko_sdk_base_dir ++ "/emlib/src/em_leuart.c",
    gecko_sdk_base_dir ++ "/emlib/src/em_msc.c",
    gecko_sdk_base_dir ++ "/emlib/src/em_opamp.c",
    gecko_sdk_base_dir ++ "/emlib/src/em_pcnt.c",
    gecko_sdk_base_dir ++ "/emlib/src/em_pdm.c",
    gecko_sdk_base_dir ++ "/emlib/src/em_prs.c",
    gecko_sdk_base_dir ++ "/emlib/src/em_qspi.c",
    gecko_sdk_base_dir ++ "/emlib/src/em_rmu.c",
    gecko_sdk_base_dir ++ "/emlib/src/em_rtc.c",
    gecko_sdk_base_dir ++ "/emlib/src/em_rtcc.c",
    gecko_sdk_base_dir ++ "/emlib/src/em_se.c",
    gecko_sdk_base_dir ++ "/emlib/src/em_system.c",
    gecko_sdk_base_dir ++ "/emlib/src/em_timer.c",
    gecko_sdk_base_dir ++ "/emlib/src/em_usart.c",
    gecko_sdk_base_dir ++ "/emlib/src/em_vcmp.c",
    gecko_sdk_base_dir ++ "/emlib/src/em_vdac.c",
    gecko_sdk_base_dir ++ "/emlib/src/em_wdog.c",

    gecko_sdk_base_dir ++ "/emdrv/dmadrv/src/dmactrl.c",
    gecko_sdk_base_dir ++ "/emdrv/dmadrv/src/dmadrv.c",
    gecko_sdk_base_dir ++ "/emdrv/gpiointerrupt/src/gpiointerrupt.c",
    gecko_sdk_base_dir ++ "/emdrv/spidrv/src/spidrv.c",
    gecko_sdk_base_dir ++ "/emdrv/uartdrv/src/uartdrv.c",

    //gecko_sdk_base_dir ++ "/emdrv/nvm3/src/nvm3_default.c",
    //gecko_sdk_base_dir ++ "/emdrv/nvm3/src/nvm3_default_common_linker.c",
    gecko_sdk_base_dir ++ "/emdrv/nvm3/src/nvm3_hal_flash.c",
    gecko_sdk_base_dir ++ "/emdrv/nvm3/src/nvm3_lock.c",

    gecko_sdk_base_dir ++ "/driver/button/src/sl_button.c",
    gecko_sdk_base_dir ++ "/driver/button/src/sl_simple_button.c",
    gecko_sdk_base_dir ++ "/driver/debug/src/sl_debug_swo.c",
    gecko_sdk_base_dir ++ "/driver/i2cspm/src/sl_i2cspm.c",
    gecko_sdk_base_dir ++ "/driver/leddrv/src/sl_led.c",
    gecko_sdk_base_dir ++ "/driver/leddrv/src/sl_simple_led.c",
    // Adding udelay Helper
    gecko_sdk_base_dir ++ "/service/udelay/src/sl_udelay.c",
    gecko_sdk_base_dir ++ "/service/udelay/src/sl_udelay_armv6m_gcc.S",
    // Adding Device-Init Helpers
    gecko_sdk_base_dir ++ "/service/device_init/src/sl_device_init_emu_s0.c",
    gecko_sdk_base_dir ++ "/service/device_init/src/sl_device_init_hfrco.c",
    gecko_sdk_base_dir ++ "/service/device_init/src/sl_device_init_hfxo_s0.c",
    gecko_sdk_base_dir ++ "/service/device_init/src/sl_device_init_lfrco.c",
    gecko_sdk_base_dir ++ "/service/device_init/src/sl_device_init_lfxo_s0.c",
    gecko_sdk_base_dir ++ "/service/device_init/src/sl_device_init_nvic.c",
    gecko_sdk_base_dir ++ "/service/power_manager/src/sl_power_manager_debug.c",
    gecko_sdk_base_dir ++ "/service/power_manager/src/sl_power_manager_hal_s0_s1.c",
    gecko_sdk_base_dir ++ "/service/power_manager/src/sl_power_manager.c",

    // Adding the sleeptimer service
    gecko_sdk_base_dir ++ "/service/sleeptimer/src/sl_sleeptimer_hal_burtc.c",
    gecko_sdk_base_dir ++ "/service/sleeptimer/src/sl_sleeptimer_hal_rtc.c",
    gecko_sdk_base_dir ++ "/service/sleeptimer/src/sl_sleeptimer.c",

    // Adding the iostream
    gecko_sdk_base_dir ++ "/service/iostream/src/sl_iostream_debug.c",
    gecko_sdk_base_dir ++ "/service/iostream/src/sl_iostream_retarget_stdio.c",
    gecko_sdk_base_dir ++ "/service/iostream/src/sl_iostream_stdio.c",
    gecko_sdk_base_dir ++ "/service/iostream/src/sl_iostream_swo_itm_8.c",
    gecko_sdk_base_dir ++ "/service/iostream/src/sl_iostream_swo.c",
    gecko_sdk_base_dir ++ "/service/iostream/src/sl_iostream.c",

    // Adding USB device
    gecko_sdk_base_dir ++ "/middleware/usb_gecko/src/em_usbhint.c",
    gecko_sdk_base_dir ++ "/middleware/usb_gecko/src/em_usbtimer.c",
    gecko_sdk_base_dir ++ "/middleware/usb_gecko/src/em_usbd.c",
    gecko_sdk_base_dir ++ "/middleware/usb_gecko/src/em_usbdch9.c",
    gecko_sdk_base_dir ++ "/middleware/usb_gecko/src/em_usbdep.c",
    gecko_sdk_base_dir ++ "/middleware/usb_gecko/src/em_usbdint.c",
    gecko_sdk_base_dir ++ "/middleware/usb_gecko/src/em_usbh.c",
    gecko_sdk_base_dir ++ "/middleware/usb_gecko/src/em_usbhal.c",
    gecko_sdk_base_dir ++ "/middleware/usb_gecko/src/em_usbhep.c",

    // Adding USB express
    gecko_sdk_base_dir ++ "/middleware/usbxpress/src/em_usbxpress_descriptors.c",
    gecko_sdk_base_dir ++ "/middleware/usbxpress/src/em_usbxpress.c",
    gecko_sdk_base_dir ++ "/middleware/usbxpress/src/em_usbxpress_callback.c",
};

pub fn aggregate(exe: *microzig.Firmware) void {
    for (include_path) |path| {
        exe.addIncludePath(.{ .path = path });
    }

    for (source_paths) |path| {
        exe.addCSourceFile(.{ .file = .{ .path = path }, .flags = &c_flags });
    }
}
