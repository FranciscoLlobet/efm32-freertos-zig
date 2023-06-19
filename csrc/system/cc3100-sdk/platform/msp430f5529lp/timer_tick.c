/*
 *
 *   Copyright (C) 2015 Texas Instruments Incorporated
 *
 *   All rights reserved. Property of Texas Instruments Incorporated.
 *   Restricted rights to use, duplicate or disclose this code are
 *   granted through contract.
 *
 *   The program may not be used without the written permission of
 *   Texas Instruments Incorporated or against the terms and conditions
 *   stipulated in the agreement under which this program has been supplied,
 *   and under no circumstances can it be used with non-TI connectivity device.
 *
 */

#include "timer_tick.h"

/*!
  \brief   Configures and starts the timer for simplelink time-stamping feature
  
  \note    This internal function is used to configure and start timer A
*/

static void simplelink_timer_start(void)
{
	Timer_A_initUpModeParam	initUpParam = {0};
	initUpParam.clockSource = TIMER_A_CLOCKSOURCE_ACLK;
	initUpParam.clockSourceDivider = TIMER_A_CLOCKSOURCE_DIVIDER_32;
	initUpParam.timerPeriod = MAX_TIMER_TICKS;
	initUpParam.timerInterruptEnable_TAIE = TIMER_A_TAIE_INTERRUPT_DISABLE;
        initUpParam.captureCompareInterruptEnable_CCR0_CCIE = TIMER_A_CCIE_CCR0_INTERRUPT_DISABLE;
	initUpParam.timerClear = TIMER_A_DO_CLEAR;
	initUpParam.startTimer = false;

	Timer_A_initUpMode(TIMER_A0_BASE, &initUpParam); 
	Timer_A_startCounter(TIMER_A0_BASE, TIMER_A_UP_MODE);
}

/*!
  \brief   returns the time stamp value    
  
  \note    This API uses Timer A peripheral for the SL host driver, hence should not be used by the application
*/

unsigned long timer_GetCurrentTimestamp(void)
{
    unsigned long timer_value = 0;
    
    //Verify timer has been started by checking that the control register was set
    if((TIMER_A_UP_MODE) != ((HWREG16(TIMER_A0_BASE + OFS_TAxCTL)) & (TIMER_A_UP_MODE)))
    {
        simplelink_timer_start();
    }
    
    timer_value = (unsigned long) Timer_A_getCounterValue(TIMER_A0_BASE);
    return timer_value;
}
