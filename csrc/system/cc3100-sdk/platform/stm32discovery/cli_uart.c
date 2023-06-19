/*
 * cli_uart.c - CC3100-STM32F4 console UART implementation
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

#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>

#include "stm32f4xx.h"
#include "stm32f407xx.h"
#include "stm32f4xx_hal.h"
#include "stm32f4_discovery.h"
#include "stm32f4xx_hal_gpio.h"

#include "cli_uart.h"

#define ASCII_ENTER     0x0D

/* Globals */
UART_HandleTypeDef UartHandle;

/* Static function declarations */
static void Error_Handler(void);

__IO uint8_t ReceBuf1[BuffSize];
__IO uint8_t ReceBuf2[BuffSize];
__IO uint8_t Count1_in = 0;
__IO uint8_t Count2_in = 0;

//*****************************************************************************
//
//! USART_NVICConfiguration
//!
//!     @brief  This function  initialization NVIC
//!
//!     @param  None
//!
//!     @return None
//!
//
//*****************************************************************************
void USART_NVICConfiguration(void)
{
}

//*****************************************************************************
//
//! USART_Configuration
//!
//!     @brief  This function  initialization UART
//!
//!     @param  None
//!
//!     @return None
//!
//
//*****************************************************************************
void CLI_Configure(void)
{
    /* Configure the UART peripheral */
    /* Put the USART peripheral in the Asynchronous mode (UART Mode) */
    /* UART1 configured as follow:
      - Word Length = 8 Bits
      - Stop Bit = One Stop bit
      - Parity = None
      - BaudRate = 115200 baud
      - Hardware flow control disabled (RTS and CTS signals) */
    UartHandle.Instance        = USART2;
    UartHandle.Init.BaudRate   = 9600;
    UartHandle.Init.WordLength = UART_WORDLENGTH_8B;
    UartHandle.Init.StopBits   = UART_STOPBITS_1;
    UartHandle.Init.Parity     = UART_PARITY_NONE;
    UartHandle.Init.HwFlowCtl  = UART_HWCONTROL_NONE;
    UartHandle.Init.Mode       = UART_MODE_TX_RX;

    if(HAL_UART_Init(&UartHandle) != HAL_OK)
    {
        Error_Handler();
    }
}

//*****************************************************************************
//
//! USART1_Write
//!
//!     @brief  The function write the data from UART
//!
//!     @param  unsigned char *inBuff
//!
//!     @return None
//!
//
//*****************************************************************************
char Buff[256];
char Temp[512];
void CLI_Write(unsigned char *pcFormat, ...)
{
    char *pcBuff = &Buff[0];
    char *pcTemp = &Temp[0];

    int iSize = 256;
    int iRet;

    memset(pcBuff, 0, 252);
    memset(pcTemp, 0, 512);

    va_list list;
    while(1)
    {
        va_start(list,pcFormat);
        iRet = vsnprintf(pcBuff,iSize,(char const *)pcFormat,list);
        va_end(list);
        if(iRet > -1 && iRet < iSize)
        {
            if(HAL_UART_Transmit(&UartHandle, (uint8_t*)pcBuff, strlen((const char *)pcBuff), 5000)!= HAL_OK)
            {
                Error_Handler();
            }

            break;
        }
        else
        {
            iSize*=2;
            pcBuff=pcTemp;
        }
    }
}

//*****************************************************************************
//
//! CLI_Read
//!  @brief  The function Read the data from UART
//!
//!  @param  inBuff    pointer to the read buffer
//!
//!  @return no of bytes read
//
//*****************************************************************************
int CLI_Read(unsigned char *pBuff)
{
    return 0;
}

//*****************************************************************************
//
//! fputc
//!  @brief
//!
//!  @param  int ch, FILE *f
//!
//!  @return int ch
//
//*****************************************************************************
int fputc(int ch, FILE *f)
{
    return 0;
}

//*****************************************************************************
//
//! USART1_IRQHandler
//!  @brief
//!
//!  @param  None
//!
//!  @return None
//
//*****************************************************************************
void USART1_IRQHandler(void)
{
}

/**
  * @brief  This function is executed in case of error occurrence.
  * @param  None
  * @retval None
  */
static void Error_Handler(void)
{
    while(1)
        ;
}
