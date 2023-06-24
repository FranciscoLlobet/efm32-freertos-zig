/*
 * board.c
 *
 *  Created on: 8 nov 2022
 *      Author: Francisco
 */

#include "board.h"
#include "board_sd_card.h"

#include "timer.h"

/* Silicon Labs Drivers */
#include "sl_iostream_debug.h"
#include "sl_debug_swo.h"
#include "sl_iostream_swo.h"
#include "sl_sleeptimer.h"
#include "sl_power_manager.h"
#include "dmadrv.h"
#include "em_usbxpress.h"

/* Silicon Labs Device Initializations */
#include "sl_device_init_emu.h"
#include "sl_device_init_nvic.h"
#include "sl_device_init_hfxo.h"
#include "sl_device_init_hfrco.h"
#include "sl_device_init_lfxo.h"

#include "FreeRTOS.h"
#include "task.h"


USBX_STRING_DESC(usb_product_string, 'X','D','K',' ','A','p','p','l','i','c','a','t','i','o','n');
USBX_STRING_DESC(usb_manufacturer_string, 'm','i','s','o');
USBX_STRING_DESC(usb_serial_string, '0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0');

USBX_Init_t usbx;

USBX_BUF(usb_rx_buf, 64);
USBX_BUF(usb_tx_buf, 64);

void BOARD_SysTick_Enable(void)
{
	SysTick_Config(SystemCoreClock / BOARD_SYSTICK_FREQUENCY);
}

void BOARD_SysTick_Disable(void)
{
	SysTick->CTRL = (SysTick_CTRL_CLKSOURCE_Msk | (0 & SysTick_CTRL_TICKINT_Msk)
			| (0 & SysTick_CTRL_ENABLE_Msk));

	NVIC_ClearPendingIRQ(SysTick_IRQn);
}

void BOARD_usDelay(uint32_t delay_in_us)
{
	sl_udelay_wait((unsigned) delay_in_us);
}


void BOARD_msDelay(uint32_t delay_in_ms)
{
	if (taskSCHEDULER_RUNNING == xTaskGetSchedulerState())
	{
		vTaskDelay(delay_in_ms);
	}
	else
	{
		sl_sleeptimer_delay_millisecond(delay_in_ms);
	}

}

void BOARD_USB_Callback(void)
{
	uint32_t intSource = USBX_getCallbackSource();
	if(intSource & USBX_RESET)
	{
		// USB Reset
	}
	if(intSource & USBX_TX_COMPLETE)
	{

	}
	if(intSource & USBX_DEV_OPEN)
	{

	}
	if(intSource & USBX_DEV_CLOSE)
	{

	}
	if(intSource & USBX_DEV_CONFIGURED)
	{

	}
	if(intSource & USBX_DEV_SUSPEND)
	{

	}
	if(intSource & USBX_RX_OVERRUN)
	{

	} 
}

void BOARD_Init(void)
{
	CHIP_Init();
	MSC_Init();

	/* Initialize mcu peripherals */
	sl_device_init_nvic();
	sl_device_init_hfxo();
	sl_device_init_hfrco();
	sl_device_init_lfxo();
	sl_device_init_emu();

	NVIC_SetPriorityGrouping((uint32_t) 3);/* Set priority grouping to group 4*/

	/* Set core NVIC priorities */
	NVIC_SetPriority(SVCall_IRQn, 0);
	NVIC_SetPriority(DebugMonitor_IRQn, 0);
	NVIC_SetPriority(PendSV_IRQn, 0);
	NVIC_SetPriority(SysTick_IRQn, 0);

	CMU_OscillatorEnable(cmuOsc_HFXO, true, true);
	CMU_ClockSelectSet(cmuClock_HF, cmuSelect_HFXO);
	CMU_OscillatorEnable(cmuOsc_HFRCO, false, false);
	CMU_ClockEnable(cmuClock_HFPER, true);
	CMU_OscillatorEnable(cmuOsc_LFXO, true, true);
	CMU_ClockSelectSet(cmuClock_LFA, cmuSelect_LFXO);
	CMU_ClockEnable(cmuClock_HFLE, true);

	CMU_ClockEnable(cmuClock_GPIO, true);


	/* ENABLE SWO */
	sl_debug_swo_init();

	sl_iostream_swo_init(); // Send printf to swo

	/* */
	CMU_ClockEnable(cmuClock_GPIO, true);

	{
		/* Setup Interrupts */
		NVIC_SetPriority(GPIO_ODD_IRQn, 5);
		NVIC_SetPriority(GPIO_EVEN_IRQn, 5);

		GPIOINT_Init();
	}

	/* Wifi NRSET port */
	GPIO_PinModeSet(gpioPortA, 15, gpioModeWiredAnd, 0);

	/* Enable 2v5 */
	GPIO_PinModeSet(gpioPortF, 5, gpioModeWiredOr, 0);

	/* Enable 3v3 supply */
	GPIO_PinModeSet(gpioPortC, 11, gpioModePushPull, 0);

	GPIO_DriveModeSet(gpioPortE, gpioDriveModeLow);
	GPIO_DriveModeSet(gpioPortD, gpioDriveModeHigh);

	GPIO_PinOutClear(PWR_2V5_SNOOZE_PORT, PWR_2V5_SNOOZE_PIN);
	GPIO_PinOutSet(PWR_3V3_EN_PORT, PWR_3V3_EN_PIN);

	/* LED Set Group */
	sl_led_init(&led_red);
	sl_led_init(&led_orange);
	sl_led_init(&led_yellow);

	sl_led_turn_off(&led_red);
	sl_led_turn_off(&led_orange);
	sl_led_turn_off(&led_yellow);

	/* BUTTON Set Group */
	sl_button_init(&button1);
	sl_button_init(&button2);

	DMADRV_Init();

	/* Initialize SPI peripherals */
	BOARD_SD_Card_Init();
	Board_CC3100_Init();

	/* Initialize I2C peripheral */
	board_i2c_init();

	BOARD_Watchdog_Init();

	SysTick_Config(SystemCoreClock / TIMER_FREQUENCY_HZ);

	/* Start Services */
	sl_power_manager_init();

	sl_sleeptimer_date_t date;

	(void)sl_sleeptimer_build_datetime(
			&date,
			(2022 - 1900), /* DEFAULT YEAR */
            MONTH_NOVEMBER, /* DEFAULT MONTH */
			11, /* DEFAULT DAY */
            10, /* DEFAULT HOUR */
            0, /* DEFAULT MINUTES */
            0, /* DEFAULT SECONDS */
            1); /* DEFAULT TZ */

	(void)sl_sleeptimer_set_datetime(&date);

#if 0

	usbx.serialString= (void*)&usb_serial_string;
	usbx.productString= (void *)&usb_product_string;
	usbx.manufacturerString =(void*)&usb_manufacturer_string;
	usbx.vendorId = (uint16_t)0x108C;
	usbx.productId = (uint16_t)0x017B;
	usbx.maxPower = (uint8_t)0xFA;
	usbx.powerAttribute = (uint8_t)0x80;
	usbx.releaseBcd = (uint16_t)0x01;
	usbx.useFifo = 0;
	USBX_init(&usbx);

	USBX_apiCallbackEnable((USBX_apiCallback_t)BOARD_USB_Callback);
#endif 
}


void BOARD_MCU_Reset(void)
{
	/* Disable the IRQs that may interfere now */
	__disable_irq();

	/* Data Synch Barrier */
	__DSB();

	/* Instruction Synch Barrier */
	__ISB();

	/* Perform Chip Reset*/
	CHIP_Reset();
}

#include <sys/time.h>
extern int _gettimeofday(struct timeval* ptimeval, void * ptimezone);
/* implementing system time */
int _gettimeofday(struct timeval* ptimeval, void * ptimezone)
{
	struct timezone * timezone_ptr = (struct timezone * )ptimezone;

	if(NULL != ptimeval)
	{
		ptimeval->tv_sec = sl_sleeptimer_get_time();
		ptimeval->tv_usec = 0;
	}

	if(NULL != timezone_ptr)
	{
		timezone_ptr->tz_dsttime = 0;
		timezone_ptr->tz_minuteswest = 0;
	}

	return 0;
}