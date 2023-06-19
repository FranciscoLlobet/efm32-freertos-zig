/*
 * bl_uart.c - msp430f5529 launchpad uart interface implementation
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

#include "simplelink.h"
#include "board.h"
#include <msp430.h>
#include "bl_uart.h"

_uartBootloaderctrl uartBootloader;
_uartBootloaderctrl *pUartBootloader = &uartBootloader;

_u8 formatCommand[6][24] = {{0x00, 0x17, 0xac, 0x28, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02},
							{0x00, 0x17, 0x2d, 0x28, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02},
							{0x00, 0x17, 0x2e, 0x28, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02},
							{0x00, 0x17, 0x30, 0x28, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02},
							{0x00, 0x17, 0x34, 0x28, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02},
							{0x00, 0x17, 0x3c, 0x28, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02}};

_u8 disconnectCommand[4] = {0x00, 0x03, 0x26, 0x26};


/*!
    \brief          The UART A0 interrupt handler for boot loader

    \param[in]      none

    \return         none

    \note

    \warning
*/
#ifdef FORMAT_ENABLE
#pragma vector=USCI_A0_VECTOR
__interrupt void CC3100_UART_BOOTLOADER_ISR(void)
{
    switch(__even_in_range(UCA0IV,0x08))
    {
        case 0:break;                             /* Vector 0 - no interrupt */
        case 2:                                   /* Vector 2 - RXIF */

        {
            unsigned char byteRead;

            while((UCA0IFG & UCRXIFG) != 0);

            if(UCRXERR & UCA1STAT)
            {
                if(UCOE & UCA1STAT)
                {
                	while(1) ;	/* error: overrun */
                }
            }

            byteRead = UCA0RXBUF;

            if(pUartBootloader->bActiveBufferIsJitterOne == TRUE)
            {
                if(pUartBootloader->JitterBufferFreeBytes > 0)
                {
                    pUartBootloader->JitterBuffer[pUartBootloader->JitterBufferWriteIdx] = byteRead;
                    pUartBootloader->JitterBufferFreeBytes--;
                    pUartBootloader->JitterBufferWriteIdx++;
                }
                else
                {
                    while(1) ;	/* error: jitter buffer is full */
                }

                if(pUartBootloader->JitterBufferWriteIdx > (UART_BOOTLOADER_JITTER_BUFFER_SIZE - 1))
                {
                    pUartBootloader->JitterBufferWriteIdx = 0;
                }
            }
            else
            {
                pUartBootloader->pActiveBuffer[pUartBootloader->ActiveBufferWriteCounter++] = byteRead;
            }
        }

            break;
        case 4:break;                             /* Vector 4 - TXIFG */
        default: break;
    }
}
#endif		/* FORMAT_ENABLE */

void bl_uart_Configure(void)
{
    unsigned char index;
	
    pUartBootloader->JitterBufferFreeBytes = UART_BOOTLOADER_JITTER_BUFFER_SIZE;
    pUartBootloader->JitterBufferWriteIdx = 0;

    pUartBootloader->pActiveBuffer = pUartBootloader->JitterBuffer;
    pUartBootloader->bActiveBufferIsJitterOne = TRUE;

    pUartBootloader->JitterBufferReadIdx = 0xFF;

    for(index = 0; index < UART_BOOTLOADER_JITTER_BUFFER_SIZE; index++)
    {
    	pUartBootloader->JitterBuffer[index] = 0xFF;
    }

    /* Configure Pin 3.3/3.4 for RX/TX */
    P3SEL |= BIT3 + BIT4;               /* P4.4,5 = USCI_A0 TXD/RXD */

    UCA0CTL1 |= UCSWRST;                /* Put state machine in reset */
    UCA0CTL0 = 0x00;
    UCA0CTL1 = UCSSEL__SMCLK + UCSWRST; /* Use SMCLK, keep RESET */
    UCA0BR0 = 0x1B;         /* 25MHz/115200 = 217.01 =0xD9 (see User's Guide) */
    UCA0BR1 = 0x00;         /* 25MHz/921600 = 27 =0x1B (see User's Guide) */

    UCA0MCTL = UCBRS_3 + UCBRF_0;       /* Modulation UCBRSx=3, UCBRFx=0 */

    UCA0CTL1 &= ~UCSWRST;               /* Initialize USCI state machine */

    /* Enable RX Interrupt on UART */
    UCA0IFG &= ~ (UCRXIFG | UCRXIFG);
    UCA0IE |= UCRXIE;
}



int bl_uart_Read(unsigned char *pBuff, int len)
{
    /* Disable interrupt to protect reorder of bytes */
    __disable_interrupt();

    pUartBootloader->pActiveBuffer = pBuff;
    pUartBootloader->bActiveBufferIsJitterOne = FALSE;
    pUartBootloader->ActiveBufferWriteCounter = 0;

    /* Copy data received in Jitter buffer to the user buffer */
    while((pUartBootloader->JitterBufferFreeBytes != UART_BOOTLOADER_JITTER_BUFFER_SIZE) && (pUartBootloader->ActiveBufferWriteCounter < len))
    {
        if(pUartBootloader->JitterBufferReadIdx == (UART_BOOTLOADER_JITTER_BUFFER_SIZE - 1))
        {
            pUartBootloader->JitterBufferReadIdx = 0;
        }
        else
        {
            pUartBootloader->JitterBufferReadIdx++;
        }

        pUartBootloader->pActiveBuffer[pUartBootloader->ActiveBufferWriteCounter++] = pUartBootloader->JitterBuffer[pUartBootloader->JitterBufferReadIdx];

        pUartBootloader->JitterBufferFreeBytes ++;
    }

    __enable_interrupt();

    /* wait till all remaining bytes are received */
    while(pUartBootloader->ActiveBufferWriteCounter < len);

    pUartBootloader->bActiveBufferIsJitterOne = TRUE;

    return len;
}


int bl_uart_Write(unsigned char *pBuff, int len)
{
    int returnLen = len;

    while (len)
    {
        while (!(UCA0IFG & UCTXIFG)) ;
        UCA0TXBUF = *pBuff;
        len--;
        pBuff++;
    }
	
    return returnLen;
}

void bl_uart_SetBreak(void)
{

    P3DIR |= BIT3;
    P3SEL &= ~BIT3;
    P3OUT &= ~BIT3;
}

void bl_read_ack(void)
{
	unsigned char ackBuffer[2] = {0xFF,0xFF};
	unsigned char newByte;

	while(		(ackBuffer[0]!= SL_BOOTLDR_OPCODE_ACK_FIRST	)	||
				(ackBuffer[1]!= SL_BOOTLDR_OPCODE_ACK_SECOND	)	)
	{
		bl_uart_Read(&newByte, 1);

		ackBuffer[0] = ackBuffer[1];
		ackBuffer[1] = newByte;
	}
}

void bl_send_format(sflashSize_e flashSize)
{
	bl_uart_Write(formatCommand[flashSize], sizeof(formatCommand[flashSize]));
}

void bl_send_disconnect(void)
{
	bl_uart_Write(disconnectCommand, sizeof(disconnectCommand));
}

