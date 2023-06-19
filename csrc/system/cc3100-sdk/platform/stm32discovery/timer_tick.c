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
#include "stm32f4xx.h"
#include "timer_tick.h"

/*!
 *      Global Vars + Function Definitions
 */


/*!
  \brief   Configures and starts the timer for simplelink time-stamping feature
  
  \note    This internal function is used to configure and start TIM2
*/

// Init Time Base Handle Structure
TIM_HandleTypeDef       Timer_Base_Handle;

static void simplelink_timer_start()
{    
    //Enable TIM2 Clock
    __HAL_RCC_TIM2_CLK_ENABLE();
    
    //Configure Time Base Config Settings
    TIM_Base_InitTypeDef    Timer_Base_Config;
    Timer_Base_Config.Prescaler =              TIMER_PRESCALAR;
    Timer_Base_Config.CounterMode =            TIM_COUNTERMODE_UP;
    Timer_Base_Config.Period =                 MAX_TIMER_TICKS;
    Timer_Base_Config.ClockDivision =          TIM_CLOCKDIVISION_DIV1;
    Timer_Base_Config.RepetitionCounter =      0;
    
    //Complete Init for Timer_Base_Handle
    Timer_Base_Handle.Instance =                TIM2;
    Timer_Base_Handle.Init =                    Timer_Base_Config;
    HAL_TIM_Base_Init(&Timer_Base_Handle);
    
    //Configure + Init Clock Source for Timer
    TIM_ClockConfigTypeDef  Clock_Config;
    Clock_Config.ClockSource =                  TIM_CLOCKSOURCE_INTERNAL;
    HAL_TIM_ConfigClockSource(&Timer_Base_Handle, &Clock_Config);
    
    //Start TIM Base
    HAL_TIM_Base_Start(&Timer_Base_Handle);
}

/*!
  \brief   returns the time stamp value    
  
  \note    This API uses TIM2 peripheral for the SL host driver, hence should not be used by the application
*/

unsigned long timer_GetCurrentTimestamp()
{
    unsigned long timer_value = 0;
    
    //Verify timer has been started by checking that the control register was set
    if((TIM_CR1_CEN) != (((&Timer_Base_Handle)->Instance->CR1) & (TIM_CR1_CEN)))
    {
        simplelink_timer_start();
    }
    
    timer_value = __HAL_TIM_GET_COUNTER(&Timer_Base_Handle);
    return timer_value;
}