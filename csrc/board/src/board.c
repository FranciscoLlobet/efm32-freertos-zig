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

/* Silicon Labs Device Initializations */
#include "sl_device_init_emu.h"
#include "sl_device_init_nvic.h"
#include "sl_device_init_hfxo.h"
#include "sl_device_init_hfrco.h"
#include "sl_device_init_lfxo.h"

#include "FreeRTOS.h"
#include "task.h"

extern nvm3_Handle_t * miso_nvm3_handle;
extern nvm3_Init_t  * miso_nvm3_init_handle;

volatile uint32_t reset_cause = (uint32_t)0;

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
void BOARD_DeInit(void)
{
	(void)nvm3_close(miso_nvm3_handle);
	MSC_Deinit();
}


void BOARD_Init(void)
{
	CHIP_Init();
	MSC_Init();

	reset_cause = RMU_ResetCauseGet();

	/* Initialize mcu peripherals */
	sl_device_init_nvic();
	//sl_device_init_hfxo();
	sl_device_init_hfrco();
	sl_device_init_lfxo();
	sl_device_init_emu();

	(void)nvm3_open(miso_nvm3_handle, miso_nvm3_init_handle);

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
	BOARD_EM9301_Init();

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

	RMU_ResetCauseClear();

	//BOARD_USB_Init();
}


void BOARD_MCU_Reset(void)
{
	//USBX_disable();
	BOARD_DeInit();
	
	/* Disable the IRQs that may interfere now */
	__disable_irq();

	// Disable all interrupts
	NVIC->ICER[0] = 0xFFFFFFFF;
	NVIC->ICER[1] = 0xFFFFFFFF;
	NVIC->ICER[2] = 0xFFFFFFFF;
	NVIC->ICER[3] = 0xFFFFFFFF;
	NVIC->ICER[4] = 0xFFFFFFFF;
	NVIC->ICER[5] = 0xFFFFFFFF;
	NVIC->ICER[6] = 0xFFFFFFFF;
	NVIC->ICER[7] = 0xFFFFFFFF;

	// Clear all pending interrupts
	NVIC->ICPR[0] = 0xFFFFFFFF;
	NVIC->ICPR[1] = 0xFFFFFFFF;
	NVIC->ICPR[2] = 0xFFFFFFFF;
	NVIC->ICPR[3] = 0xFFFFFFFF;
	NVIC->ICPR[4] = 0xFFFFFFFF;
	NVIC->ICPR[5] = 0xFFFFFFFF;
	NVIC->ICPR[6] = 0xFFFFFFFF;
	NVIC->ICPR[7] = 0xFFFFFFFF;

	//BOARD_SysTick_Disable();
	SysTick->CTRL = 0 ;
	SCB->ICSR |= SCB_ICSR_PENDSTCLR_Msk ;
	
	__DSB();
	__ISB();

	/* Perform Chip Reset*/
	CHIP_Reset();
}

uint32_t BOARD_MCU_GetResetCause(void)
{
	return reset_cause;
}

#include <sys/time.h>
// Gets called by time()

extern int gettimeofday(struct timeval* ptimeval, void * ptimezone);
/* implementing system time */
int gettimeofday(struct timeval* ptimeval, void * ptimezone)
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

typedef void (*jumpFunction)(void);

// Jump is assigned outside of the stack
static jumpFunction jump = NULL;

void BOARD_JumpToAddress(uint32_t * addr)
{
	jump = (jumpFunction)(addr[1]);

	BOARD_DeInit();

	__DSB();
	__ISB();

	// Set to privileged mode
	if( CONTROL_nPRIV_Msk & __get_CONTROL( ) )
	{
		//__ASM volatile ("svc 0x07");
	}

	__ISB();
	__disable_irq();

	// Disable all interrupts
	NVIC->ICER[0] = 0xFFFFFFFF;
	NVIC->ICER[1] = 0xFFFFFFFF;
	NVIC->ICER[2] = 0xFFFFFFFF;
	NVIC->ICER[3] = 0xFFFFFFFF;
	NVIC->ICER[4] = 0xFFFFFFFF;
	NVIC->ICER[5] = 0xFFFFFFFF;
	NVIC->ICER[6] = 0xFFFFFFFF;
	NVIC->ICER[7] = 0xFFFFFFFF;

	// Clear all pending interrupts
	NVIC->ICPR[0] = 0xFFFFFFFF;
	NVIC->ICPR[1] = 0xFFFFFFFF;
	NVIC->ICPR[2] = 0xFFFFFFFF;
	NVIC->ICPR[3] = 0xFFFFFFFF;
	NVIC->ICPR[4] = 0xFFFFFFFF;
	NVIC->ICPR[5] = 0xFFFFFFFF;
	NVIC->ICPR[6] = 0xFFFFFFFF;
	NVIC->ICPR[7] = 0xFFFFFFFF;

	__DSB();
	__ISB();

	// Set some of the peripherals to reset values
	RTC->CTRL         = _RTC_CTRL_RESETVALUE;
	RTC->COMP0        = _RTC_COMP0_RESETVALUE;
	RTC->IEN          = _RTC_IEN_RESETVALUE;
	/* Disable RTC clock */
	CMU->LFACLKEN0    = _CMU_LFACLKEN0_RESETVALUE;
	CMU->LFCLKSEL     = _CMU_LFCLKSEL_RESETVALUE;
	/* Disable LFRCO */
	CMU->OSCENCMD     = CMU_OSCENCMD_LFRCODIS;
	/* Disable LE interface */
	CMU->HFCORECLKEN0 = _CMU_HFCORECLKEN0_RESETVALUE;
	/* Reset clocks */
	CMU->HFPERCLKDIV  = _CMU_HFPERCLKDIV_RESETVALUE;
	CMU->HFPERCLKEN0  = _CMU_HFPERCLKEN0_RESETVALUE;
	
	// Disable the SysTick
	SysTick->CTRL = 0 ;
	SCB->ICSR |= SCB_ICSR_PENDSTCLR_Msk;

	__DSB();
	__ISB();

	// Disable all Faultmasks
	SCB->SHCSR &= ~( SCB_SHCSR_USGFAULTENA_Msk | SCB_SHCSR_BUSFAULTENA_Msk | SCB_SHCSR_MEMFAULTENA_Msk ) ;

	if( CONTROL_SPSEL_Msk & __get_CONTROL( ) )
	{  /* MSP is not active */
  		__set_MSP( __get_PSP( ) ) ;
  		__set_CONTROL( __get_CONTROL( ) & ~CONTROL_SPSEL_Msk ) ;
	}

	__DSB();
	__ISB();
	
	// Set the vector table offset
	SCB->VTOR = ( uint32_t )addr;
	
	// Set the main stack pointer
	// From here we lose the variables in stack
	__set_MSP(addr[0]);
	__set_PSP(__get_MSP());

	// Perform the Jump using the C-Style function pointer
	if(NULL != jump)
	{
		__set_CONTROL(0x00);
		__ISB();
		
		jump();
	}
	// unreachable
}


static int miso_putc(char c, FILE *file)
{
	(void)file;
	
	(void)sl_iostream_putchar(SL_IOSTREAM_STDOUT, c);

	return 1;
}

static int miso_getc(FILE *file)
{
	char c = EOF;
	(void)file;
	(void) sl_iostream_getchar(SL_IOSTREAM_STDIN, &c);

	return c;
}

static int miso_flush(FILE *file)
{
	(void)file;

	return 0;
}

static FILE __stdio = FDEV_SETUP_STREAM(miso_putc, miso_getc, miso_flush, _FDEV_SETUP_RW);


#ifdef __strong_reference
#define STDIO_ALIAS(x) __strong_reference(stdin, x);
#else
#define STDIO_ALIAS(x) FILE *const x = &__stdio;
#endif

FILE *const stdin = &__stdio;
STDIO_ALIAS(stdout);
STDIO_ALIAS(stderr);

int write(int handle, const unsigned char * buffer, size_t size)
{
	(void)handle;

	for (size_t i = 0; i < size; i++)
	{
		(void)sl_iostream_putchar(SL_IOSTREAM_STDOUT, buffer[i]);
	}

	return size;
}