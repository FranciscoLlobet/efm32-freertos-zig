/*
 * board.c - CC3100-STM32F4 platform configuration functions
 *
 * Copyright (C) 2014 Texas Instruments Incorporated - http://www.ti.com/ 
 * 
 * 
 *  Redistribution and use in source and binary forms, with or without 
 *  modification, are permitted provided that the following conditions 
 *  are met:
 *
 *    Redistributions of source code must retain the above copyright 
 *    notice, this list of conditions and the following disclaimer.
 *
 *    Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the 
 *    documentation and/or other materials provided with the   
 *    distribution.
 *
 *    Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT 
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT 
 *  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, 
 *  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT 
 *  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT 
 *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE 
 *  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
*/

#include "stm32f4xx.h"
#include "stm32f407xx.h"
#include "stm32f4xx_hal.h"

#include "stm32f4xx_hal_tim.h"
#include "stm32f4_discovery.h"
#include "stm32f4xx_hal_gpio.h"

#include "board.h"

/* PA0 for HOST-IRQ*/
#define MCU_IRQ_PIN         GPIO_PIN_0
#define MCU_IRQ_PORT        GPIOA
#define MCU_nHIB_PORT       GPIOB
#define MCU_nHIB_PIN        GPIO_PIN_0

#define RTC_ASYNCH_PREDIV   0x7F
#define RTC_SYNCH_PREDIV    0x0130

#define PULSE1_VALUE        60000       /* 1s  */

/**/
typedef enum bool{
    FALSE,
    TRUE
}e_BOOL;

/**/
P_EVENT_HANDLER     pIrqEventHandler = 0;
RTC_HandleTypeDef   RTCHandle;
TIM_HandleTypeDef   TimHandle;
TIM_OC_InitTypeDef  sConfig;

/* Globals */
unsigned long g_ulTimerInts = 0;

/* Static function declarations */
static void Error_Handler();
static void SystemClock_Config();
static void EXTILine0_Config(e_BOOL enable);
static void GPIO_Init(GPIO_TypeDef  *GPIO_PORT, unsigned short GPIO_PIN);
static void GPIO_PinOutSet(GPIO_TypeDef  *GPIO_PORT, unsigned short GPIO_PIN);
static void GPIO_PinOutClear(GPIO_TypeDef  *GPIO_PORT, unsigned short GPIO_PIN);


void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin);

/*!
    \brief              This function inits the device
    \param              None
    \return             None
    \note
    \warning
*/
void initClk()
{
    HAL_Init();
    SystemClock_Config();
}

/*!
    \brief              This function stops The Watch Dog Timer
    \param              None
    \return             None
    \note
    \warning
*/
void stopWDT()
{
}

/*!
    \brief              This function initialize the CC3100 nHIB GPIO
    \param              None
    \return             None
    \note
    \warning
*/
void CC3100_nHIB_init()
{
	GPIO_Init(MCU_nHIB_PORT, MCU_nHIB_PIN);
}

/*!
    \brief              This function disables CC3100 device
    \param              None
    \return             None
    \note
    \warning
*/
void CC3100_disable()
{
    GPIO_PinOutClear(MCU_nHIB_PORT, MCU_nHIB_PIN);
}

/*!
    \brief              This function enables CC3100 device
    \param              None
    \return             None
    \note
    \warning
*/
void CC3100_enable()
{
    GPIO_PinOutSet(MCU_nHIB_PORT, MCU_nHIB_PIN);
}

/*!
    \brief              This function enables waln IrQ pin
    \param              None
    \return             None
    \note
    \warning
*/
void CC3100_InterruptEnable()
{
    /* Configure EXTI Line0 (connected to PA0 pin) in interrupt mode */
    EXTILine0_Config(TRUE);
}

/*!
    \brief              This function disables waln IrQ pin
    \param              None
    \return             None
    \note
    \warning
*/
void CC3100_InterruptDisable()
{
    EXTILine0_Config(FALSE);
}

/**
  * @brief EXTI line detection callbacks
  * @param GPIO_Pin: Specifies the pins connected EXTI line
  * @retval None
  */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    if((GPIO_Pin == MCU_IRQ_PIN) &&
       (NULL != pIrqEventHandler) )
    {
        pIrqEventHandler(0);
    }
}

/**/
int registerInterruptHandler(P_EVENT_HANDLER InterruptHdl , void* pValue)
{
    pIrqEventHandler = InterruptHdl;
    return 0;
}

/*!
    \brief              Induce delay in ms
    \param              delay: specifies the delay time length, in milliseconds.
    \return             None
    \note
    \warning
*/
void Delay(unsigned long delay)
{
    HAL_Delay(delay);
}

/**
  * @brief  System Clock Configuration
  *         The system Clock is configured as follow :
  *            System Clock source            = PLL (HSE)
  *            SYSCLK(Hz)                     = 168000000
  *            HCLK(Hz)                       = 168000000
  *            AHB Prescaler                  = 1
  *            APB1 Prescaler                 = 4
  *            APB2 Prescaler                 = 2
  *            HSE Frequency(Hz)              = 8000000
  *            PLL_M                          = 8
  *            PLL_N                          = 336
  *            PLL_P                          = 2
  *            PLL_Q                          = 7
  *            VDD(V)                         = 3.3
  *            Main regulator output voltage  = Scale1 mode
  *            Flash Latency(WS)              = 5
  * @param  None
  * @retval None
  */
static void SystemClock_Config(void)
{
    RCC_ClkInitTypeDef RCC_ClkInitStruct;
    RCC_OscInitTypeDef RCC_OscInitStruct;

    /* Enable Power Control clock */
    __PWR_CLK_ENABLE();

    /* The voltage scaling allows optimizing the power consumption when the device is
       clocked below the maximum system frequency, to update the voltage scaling value
       regarding system frequency refer to product datasheet.  */
    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

    /* Enable HSE Oscillator and activate PLL with HSE as source */
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct.HSEState = RCC_HSE_ON;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
    RCC_OscInitStruct.PLL.PLLM = 8;
    RCC_OscInitStruct.PLL.PLLN = 336;
    RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
    RCC_OscInitStruct.PLL.PLLQ = 7;
    if(HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
    {
        Error_Handler();
    }

    /* Select PLL as system clock source and configure the HCLK, PCLK1 and PCLK2
       clocks dividers */
    RCC_ClkInitStruct.ClockType = (RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2);
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;
    if(HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK)
    {
        Error_Handler();
    }
}

/**
  * @brief  This function is executed in case of error occurrence.
  * @param  None
  * @retval None
  */
static void Error_Handler(void)
{
    while(1)
    {
    }
}

/**
  * @brief  Configures EXTI Line0 (connected to PA0 pin) in interrupt mode
  * @param  None
  * @retval None
  */
static void EXTILine0_Config(e_BOOL enable)
{
    GPIO_InitTypeDef   GPIO_InitStructure;

    if (TRUE == enable)
    {
        /* Enable GPIOA clock */
        __GPIOA_CLK_ENABLE();

        /* Configure PA0 pin as input floating */
        GPIO_InitStructure.Mode = GPIO_MODE_IT_RISING;
        GPIO_InitStructure.Pull = GPIO_NOPULL;
        GPIO_InitStructure.Pin = MCU_IRQ_PIN;
        HAL_GPIO_Init(MCU_IRQ_PORT, &GPIO_InitStructure);

        /* Enable and set EXTI Line0 Interrupt to the lowest priority */
        HAL_NVIC_SetPriority(EXTI0_IRQn, 2, 0);
        HAL_NVIC_EnableIRQ(EXTI0_IRQn);
    }
    else if(FALSE == enable)
    {
        HAL_NVIC_DisableIRQ(EXTI0_IRQn);
    }
}

static void GPIO_Init(GPIO_TypeDef  *GPIO_PORT,
                      unsigned short GPIO_PIN)
{
    GPIO_InitTypeDef   GPIO_InitStructure;

    /* Enable GPIOx clock */
    if(GPIO_PORT == GPIOA)
        __GPIOA_CLK_ENABLE();
    else if(GPIO_PORT == GPIOB)
        __GPIOB_CLK_ENABLE();
    else if(GPIO_PORT == GPIOC)
        __GPIOC_CLK_ENABLE();
    else if(GPIO_PORT == GPIOD)
        __GPIOD_CLK_ENABLE();

    /* Configure pin as input floating */
    GPIO_InitStructure.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStructure.Pull = GPIO_PULLUP;
    GPIO_InitStructure.Pin = GPIO_PIN;
    GPIO_InitStructure.Speed = GPIO_SPEED_FAST;

    HAL_GPIO_Init(GPIO_PORT, &GPIO_InitStructure);
}

static void GPIO_PinOutSet(GPIO_TypeDef  *GPIO_PORT,
                           unsigned short GPIO_PIN)
{
    HAL_GPIO_WritePin(GPIO_PORT, GPIO_PIN, GPIO_PIN_SET);
}

static void GPIO_PinOutClear(GPIO_TypeDef  *GPIO_PORT,
                            unsigned short GPIO_PIN)
{
    HAL_GPIO_WritePin(GPIO_PORT, GPIO_PIN, GPIO_PIN_RESET);
}