/*
 * bl_uart.h - msp430f5529 launchpad uart interface implementation
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

#ifndef __BL_UART_H__
#define __BL_UART_H__

#ifdef __cplusplus
extern "C" {
#endif

#define UART_BOOTLOADER_JITTER_BUFFER_SIZE       	64
#define SL_BOOTLDR_OPCODE_ACK_FIRST			   	0x00
#define SL_BOOTLDR_OPCODE_ACK_SECOND			0xCC

typedef struct
{
    unsigned char          JitterBuffer[UART_BOOTLOADER_JITTER_BUFFER_SIZE];
    unsigned char          JitterBufferWriteIdx;
    unsigned char          JitterBufferReadIdx;
    unsigned char          JitterBufferFreeBytes;
    unsigned char          bRtsSetByFlowControl;
    unsigned char          bActiveBufferIsJitterOne;
    unsigned char          *pActiveBuffer;
    unsigned short         ActiveBufferWriteCounter;
}_uartBootloaderctrl;

typedef enum
{
  size512KB = 0,
  size1MB,
  size2MB,
  size4MB,
  size8MB,
  size16MB
}sflashSize_e;



/*!
    \brief open uart communication port with baud rate 921600 to be used for 
	       communicating with a bootloader of the SimpleLink device

    \param[in]      none

    \return         none

    \sa             none
    \note
    \warning
*/

void bl_uart_Configure(void);


/*!
    \brief attempts to read up to len bytes from BL UART channel into a buffer
           starting at pBuff.

    \param[in]      pBuff  -    points to first location to start writing the
                    data

    \param[in]      len    -    number of bytes to read from the UART channel

    \return         upon successful completion, the function shall the number
    			  successfully read bytes

    \sa             uart_Open , uart_Write
    \note
    \warning
*/

int bl_uart_Read(unsigned char *pBuff, int len);


/*!
    \brief attempts to write up to len bytes to the BL UART channel

    \param[in]      pBuff     -    points to first location to start getting the
                    data from

    \param[in]      len       -    number of bytes to write to the UART channel

    \return         upon successful completion, the function shall return the number
    			  successfully written bytes

    \sa             uart_Open , uart_Read
    \note          
    \warning
*/
int bl_uart_Write(unsigned char *pBuff, int len);

/*!
    \brief setting break signal (i.e. assert to logic 0) on the UART line going to the device

    \param[in]      none

    \return         none

    \sa             none
    \note
    \warning
*/
void bl_uart_SetBreak(void);

/*!
    \brief read ACK packet from the device

    \param[in]      none

    \return         none

    \sa             none
    \note
    \warning
*/
void bl_read_ack(void);

/*!
    \brief formats the deviec accoring to the capacity

    \param[in]      flashSize     -    capacity size of the serial flash

    \return         none

    \sa             none
    \note
    \warning
*/
void bl_send_format(sflashSize_e flashSize);

/*!
    \brief disconnectes from the device.
    		At this phase, device exits boot loader phase and initializes

    \param[in]      none

    \return         none

    \sa             none
    \note
    \warning
*/
void bl_send_disconnect(void);


#ifdef  __cplusplus
}
#endif // __cplusplus

#endif  //__BL_UART_H__
