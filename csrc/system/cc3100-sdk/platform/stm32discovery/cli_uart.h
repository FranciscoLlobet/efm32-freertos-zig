/*
 * cli_uart.h - CC3100-STM32F4 console UART implementation
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

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __USART_H
#define __USART_H

/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx.h"
#include <stdio.h>
#include <string.h>

#define BuffSize    50

extern __IO uint8_t ReceBuf1[BuffSize]; /* receive buffer */
extern __IO uint8_t ReceBuf2[BuffSize]; /* receive buffer */
extern __IO uint8_t Count1_in;//
extern __IO uint8_t Count2_in;//

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
void CLI_Configure(void);

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
void CLI_Write(unsigned char *inBuff, ...);

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
int CLI_Read(unsigned char *pBuff);

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
int fputc(int ch, FILE *f);

#endif
/******************* (C) COPYRIGHT 2012 STMicroelectronics *****END OF FILE****/
