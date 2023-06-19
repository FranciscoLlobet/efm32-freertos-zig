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
#include <msp430.h>
#include <string.h>

#include "timer_if.h"
#include "driverlib.h"

/*!
 *  \brief: Frequency of MCLK set to 1.5MHz
 */
#define ACLOCK_FREQ                    (32000)
// #define TIMER_PRESCALAR					TIMER_A_CLOCKSOURCE_DIVIDER_8
#define TIMER_PRESCALAR					TIMER_A_CLOCKSOURCE_DIVIDER_32
#define TIMER_CLK                      (ACLOCK_FREQ/TIMER_PRESCALAR) /* ACLK/prescaler */

#define MILLISECONDS_TO_TICKS(ms)       ((TIMER_CLK * ms)/1000)
#define TICKS_TO_MILLISECONDS(ticks)    ((ticks)/(TIMER_CLK/1000))
#define MAX_TIMER_TICKS                 (0xFFFF)

/*!
 *  \brief: Content of each timer
 */
typedef struct timerCtx{
    P_TIMER_CALLBACK    callback;
    unsigned short      timerMode;
}s_timerCtx;

s_timerCtx gTimer1Ctx = {0};
s_timerCtx gTimer2Ctx = {0};

/**/
long Timer_IF_Init(e_Timers const timer, e_TimerModes const mode)
{
    if( (TIMER1 != (timer & TIMER1)) &&
        (TIMER2 != (timer & TIMER2)) )
    {
        return -1;
    }

    if(TIMER1 == (timer & TIMER1))
    {
        switch(mode)
        {
            case PERIODIC:
            {
                gTimer1Ctx.timerMode = TIMER_A_CONTINUOUS_MODE;
            }
            break;

            case ONE_SHOT: /* Fall through */
            default:
                gTimer1Ctx.timerMode = TIMER_A_UP_MODE;
            break;
        }
    }

    if(TIMER2 == (timer & TIMER2))
    {
        switch(mode)
        {
            case PERIODIC:
            {
                gTimer2Ctx.timerMode = TIMER_A_CONTINUOUS_MODE;
            }
            break;

            case ONE_SHOT: /* Fall through */
            default:
                gTimer2Ctx.timerMode = TIMER_A_UP_MODE;
            break;
        }
    }

    return 0;
}

/**/
long Timer_IF_DeInit(e_Timers const timer)
{
    if( (TIMER1 != (timer & TIMER1)) &&
        (TIMER2 != (timer & TIMER2)) )
    {
        return -1;
    }

    if(TIMER1 == (timer & TIMER1)) memset(&gTimer1Ctx, 0, sizeof(gTimer1Ctx));
    if(TIMER2 == (timer & TIMER2)) memset(&gTimer2Ctx, 0, sizeof(gTimer1Ctx));

    return 0;
}

/**/
long  Timer_IF_IntSetup(e_Timers const timer, P_TIMER_CALLBACK callback)
{
    if( (TIMER1 != (timer & TIMER1)) &&
        (TIMER2 != (timer & TIMER2)) )
    {
        return -1;
    }
   
	if(TIMER1 == (timer & TIMER1))
		gTimer1Ctx.callback = callback;
	else
		gTimer2Ctx.callback = callback;

    return 0;
}

/**/
long Timer_IF_InterruptClear(e_Timers const timer)
{
    if( (TIMER1 != (timer & TIMER1)) &&
        (TIMER2 != (timer & TIMER2)) )
    {
        return -1;
    }

    return 0;
}

/**/
long Timer_IF_Start(e_Timers const timer, unsigned long value)
{
	long timerPeriod;

    if( (TIMER1 != (timer & TIMER1)) &&
        (TIMER2 != (timer & TIMER2)) )
    {
        return -1;
    }

    	Timer_A_initUpModeParam	initUpParam = {0};
    	initUpParam.clockSource = TIMER_A_CLOCKSOURCE_ACLK;
    	initUpParam.clockSourceDivider = TIMER_PRESCALAR;
    	timerPeriod = MILLISECONDS_TO_TICKS(value);
    	if (timerPeriod > 0xFFFF)
    		initUpParam.timerPeriod = 0xFFFF;
    	else
    		initUpParam.timerPeriod = timerPeriod;
    	initUpParam.timerInterruptEnable_TAIE = TIMER_A_TAIE_INTERRUPT_DISABLE;
    	initUpParam.captureCompareInterruptEnable_CCR0_CCIE = TIMER_A_CCIE_CCR0_INTERRUPT_ENABLE;
		initUpParam.timerClear = TIMER_A_DO_CLEAR;
		initUpParam.startTimer = false;

    	if(TIMER1 == (timer & TIMER1))
		{	
   			Timer_A_initUpMode(TIMER_A0_BASE, &initUpParam); 
			Timer_A_clearTimerInterrupt(TIMER_A0_BASE);	
			Timer_A_clearCaptureCompareInterrupt(TIMER_A0_BASE,TIMER_A_CAPTURECOMPARE_REGISTER_0);
			Timer_A_startCounter(TIMER_A0_BASE, TIMER_A_UP_MODE);	
		}
		else {
   			Timer_A_initUpMode(TIMER_A1_BASE, &initUpParam); 
			Timer_A_clearTimerInterrupt(TIMER_A1_BASE);	
			Timer_A_clearCaptureCompareInterrupt(TIMER_A1_BASE,TIMER_A_CAPTURECOMPARE_REGISTER_0);
			Timer_A_startCounter(TIMER_A1_BASE, TIMER_A_UP_MODE);	
		}			
		

    return 0;
}

/**/
long Timer_IF_Stop(e_Timers const timer)
{
    if( (TIMER1 != (timer & TIMER1)) &&
        (TIMER2 != (timer & TIMER2)) )
    {
        return -1;
    }

    if(TIMER1 == (timer & TIMER1))
    {
        Timer_A_stop(TIMER_A0_BASE);
        Timer_A_disableInterrupt(TIMER_A0_BASE);
        Timer_A_disableCaptureCompareInterrupt(TIMER_A0_BASE,TIMER_A_CAPTURECOMPARE_REGISTER_0);
    }
	else {
        Timer_A_stop(TIMER_A1_BASE);
        Timer_A_disableInterrupt(TIMER_A1_BASE);
        Timer_A_disableCaptureCompareInterrupt(TIMER_A1_BASE,TIMER_A_CAPTURECOMPARE_REGISTER_0);
    }

    return 0;
}

long Timer_IF_Count(e_Timers const timer)
{
	long	count = 0;

    if( (TIMER1 != (timer & TIMER1)) &&
        (TIMER2 != (timer & TIMER2)) )
    {
        return -1;
    }

    if(TIMER1 == (timer & TIMER1))
    {
        count = (long) Timer_A_getCounterValue(TIMER_A0_BASE);
    }
	else {
	     count = (long) Timer_A_getCounterValue(TIMER_A1_BASE);
    }

    return count;
}

/* TIMER0_A0_VECTOR is used by FreeRTOS port.c */
#ifndef SL_PLATFORM_MULTI_THREADED
#if defined(__TI_COMPILER_VERSION__) || defined(__IAR_SYSTEMS_ICC__)
#pragma vector=TIMER0_A0_VECTOR
__interrupt
#elif defined(__GNUC__)
__attribute__((interrupt(TIMER0_A0_VECTOR)))
#endif
/*** Interrupt Service routines **/
void timer0_ccr0_ISR (void)
{
	/* check timer mode and stop, if needed */	
	if(gTimer1Ctx.timerMode == TIMER_A_UP_MODE)
	{
        Timer_A_stop(TIMER_A0_BASE);
        Timer_A_disableInterrupt(TIMER_A0_BASE);
        Timer_A_disableCaptureCompareInterrupt(TIMER_A0_BASE,TIMER_A_CAPTURECOMPARE_REGISTER_0);

	}
	// call back function
	if (gTimer1Ctx.callback)
		gTimer1Ctx.callback();
}
#endif // FREE_RTOS

#if defined(__TI_COMPILER_VERSION__) || defined(__IAR_SYSTEMS_ICC__)
#pragma vector=TIMER1_A0_VECTOR
__interrupt
#elif defined(__GNUC__)
__attribute__((interrupt(TIMER1_A0_VECTOR)))
#endif
/*** Interrupt Service routines **/
void timer2_ccr0_ISR (void)
{
	/* check timer mode and stop, if needed */	
	if(gTimer2Ctx.timerMode == TIMER_A_UP_MODE)
	{
        Timer_A_stop(TIMER_A1_BASE);
        Timer_A_disableInterrupt(TIMER_A1_BASE);
        Timer_A_disableCaptureCompareInterrupt(TIMER_A1_BASE,TIMER_A_CAPTURECOMPARE_REGISTER_0);
		
	}
	// call back function
	if(gTimer2Ctx.callback)
		gTimer2Ctx.callback();
}
