/*
 * daignostic.h
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

#ifndef __DIAG_H__
#define __DIAG_H__

#include "board.h"
#include "cli_uart.h"

/*!
    \brief      attempts to write up to len bytes to the Uart channel
    
    \param      pBuff - pointer to the first location of a buffer that contains
                        the data to send over the communication channel

    \param      len   - number of bytes to write to the communication channel
                    
    \sa         sl_IfClose , sl_IfOpen , sl_IfRead

    \note 
                The prototype of the function is as follow:
                    void UartWrite(unsigned char* pBuff , int Len);

    \warning
*/
#define UartWrite       CLI_Write

/*!
    \brief      attempts to Open the Uart channel
    
    
    \note       The prototype of the function is as follow:
                    void UartConfig();
*/
#define UartConfig      DispatcherUARTConfigure

/*!
    \brief      Initialize the System clock
    
    
    \note       The prototype of the function is as follow:
                    void Init_Clk();
*/
#define Init_Clk        initClk

/*!
    \brief Stops the WDT
    
    
    \note       The prototype of the function is as follow:
                    void StopWDT();
*/

#define StopWDT        stopWDT

#endif

