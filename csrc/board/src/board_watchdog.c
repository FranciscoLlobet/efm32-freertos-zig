/*
 * board_watchdog.c
 *
 *  Created on: 4 jun 2023
 *      Author: Francisco
 */
#include "board.h"

#include <em_wdog.h>

#define BOARD_WDOG    	DEFAULT_WDOG

void BOARD_Watchdog_Init(void)
{
	WDOG_Init_TypeDef wdog_init = WDOG_INIT_DEFAULT;
	wdog_init.clkSel = wdogClkSelLFXO;
	wdog_init.debugRun = false; // When in debug mode, the watchdog is disabled.
	wdog_init.perSel = wdogPeriod_256k; //(7s)
	wdog_init.lock = false;
	wdog_init.enable = false;
	wdog_init.em2Run = false;
	wdog_init.em3Run = false;
	wdog_init.em4Block = false;

	WDOGn_Init(BOARD_WDOG, &wdog_init);
}

void BOARD_Watchdog_Enable(void)
{
	WDOGn_Enable(BOARD_WDOG, true);
}

void BOARD_Watchdog_Feed(void)
{
	WDOGn_Feed(BOARD_WDOG);
}
