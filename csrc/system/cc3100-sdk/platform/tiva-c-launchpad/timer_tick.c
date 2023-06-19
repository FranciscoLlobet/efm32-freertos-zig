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
#include <stdint.h>
#include <inc/hw_types.h>
#include "inc/hw_nvic.h"
#include "driverlib/systick.h"
#include "timer_tick.h"

/*!
  \brief   Counter to count the number of interrupts that have been called.

  \note
*/

static volatile unsigned long g_Counter = 0;

/*!
  \brief   Increments the counter based on systick peripheral

  \note
*/

static void systick_handler(void)
{
    if (g_Counter < MAX_TIMER_TICKS)
    {
        g_Counter++;
    }
    else
    {
        g_Counter = 0;
    }
}

/*!
  \brief   Configures and starts the timer for simplelink time-stamping feature

  \note    This internal function is used to configure and start systick
*/

static void simplelink_timer_start(void)
{
    SysTickDisable();
    SysTickPeriodSet(TIMER_PRESCALAR);

    SysTickIntRegister(*systick_handler);
    SysTickIntEnable();

    SysTickEnable();
}

/*!
  \brief   returns the time stamp value

  \note    This API uses systick peripheral for the SL host driver, hence should not be altered
*/

unsigned long timer_GetCurrentTimestamp(void)
{
    //Verify timer has been started by checking that the control register was set
    if(NVIC_ST_CTRL_INTEN != (NVIC_ST_CTRL_INTEN & HWREG(NVIC_ST_CTRL)))
    {
        simplelink_timer_start();
    }

    return g_Counter;
}
