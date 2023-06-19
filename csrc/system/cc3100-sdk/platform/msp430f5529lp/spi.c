/*
 * spi.c - msp430f5529 launchpad spi interface implementation
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

#ifndef SL_IF_TYPE_UART
#include <msp430.h>

#include "simplelink.h"
#include "spi.h"
#include "board.h"

#define ASSERT_CS()          (P2OUT &= ~BIT2)
#define DEASSERT_CS()        (P2OUT |= BIT2)

#ifdef ENABLE_DMA
unsigned long g_ucDinDout[20];
#define DMA_BUFF_SIZE_MIN               100
#define MAX_DMA_RECV_TRANSACTION_SIZE   1024
#endif

int spi_Close(Fd_t fd)
{
    /* Disable WLAN Interrupt ... */
    CC3100_InterruptDisable();
    
#ifdef SL_PLATFORM_MULTI_THREADED
    return OSI_OK;
#else
    return NONOS_RET_OK;
#endif    
}

Fd_t spi_Open(char *ifName, unsigned long flags)
{
    /* Select the SPI lines: MOSI/MISO on P3.0,1 CLK on P3.2 */
    P3SEL |= (BIT0 + BIT1);

    P3OUT |= BIT1;

    P3SEL |= BIT2;

    /* Enable pull up on P3.3, CC3100 UART RX */
    P3OUT |= BIT3;
    P3REN |= BIT3;

    UCB0CTL1 |= UCSWRST; /* Put state machine in reset */
    UCB0CTL0 = UCMSB + UCMST + UCSYNC + UCCKPH; /* 3-pin, 8-bit SPI master */

    UCB0CTL1 = UCSWRST + UCSSEL_2; /* Use SMCLK, keep RESET */

    /* Set SPI clock */
    UCB0BR0 = 0x02; /* f_UCxCLK = 25MHz/2 */
    UCB0BR1 = 0;
    UCB0CTL1 &= ~UCSWRST;
    
#ifdef ENABLE_DMA
    memset(g_ucDinDout,0xFF,sizeof(g_ucDinDout));

    DMACTL0 = DMA1TSEL_18+DMA0TSEL_19;    /* DMA0 - UCA0TXIFG */
#endif    

    /* P1.6 - WLAN enable full DS */
    P1SEL &= ~BIT6;
    P1OUT &= ~BIT6;
    P1DIR |= BIT6;
    
    /* Configure SPI IRQ line on P2.0 */
    P2DIR &= ~BIT0;
    P2SEL &= ~BIT0;

    P2REN |= BIT0;

    /* Configure the SPI CS to be on P2.2 */
    P2OUT |= BIT2;
    P2SEL &= ~BIT2;
    P2DIR |= BIT2;

    /* 50 ms delay */
    Delay(50);

    /* Enable WLAN interrupt */
    CC3100_InterruptEnable();

#ifdef SL_PLATFORM_MULTI_THREADED
    return OSI_OK;
#else
    return NONOS_RET_OK;
#endif    
}

#ifdef ENABLE_DMA
void SetupDMASend(unsigned char *ucBuff,int len)
{

    UCB0IFG &= ~UCTXIFG;

    /* Setup DMA0 */
    __data16_write_addr((unsigned short) &DMA0SA,(unsigned long) ucBuff);
                                            /* Source block address */

    __data16_write_addr((unsigned short) &DMA0DA,(unsigned long) &UCB0TXBUF);
                                            /* Destination address */

    DMA0SZ = len;                           /* Block size */
    DMA0CTL = DMADT_0 + DMASRCINCR_3 + DMASBDB + DMAEN + DMALEVEL;


    __data16_write_addr((unsigned short) &DMA1SA,(unsigned long) &UCB0RXBUF);
                                            /* Source block address */

    __data16_write_addr((unsigned short) &DMA1DA,(unsigned long) &g_ucDinDout[5]);
                                            /* Destination single address */

    DMA1SZ = len;                             /* Block size */

    DMA1CTL = DMADT_0 + DMADSTINCR_0 + DMASBDB + DMALEVEL + DMAEN;

    /* Signal to start xfer */
    UCB0IFG |=  UCTXIFG;

    /* Wait for completation of DMA xfer */
    while(!(DMA0CTL&DMAIFG) || !(DMA1CTL&DMAIFG));
}
#endif


int spi_Write(Fd_t fd, unsigned char *pBuff, int len)
{
    int len_to_return = 0;
    
#ifdef ENABLE_DMA    
    unsigned int pBuffAddr = (unsigned int)pBuff;
#endif    

    ASSERT_CS();
    
#ifdef ENABLE_DMA

    if( len > DMA_BUFF_SIZE_MIN && ((pBuffAddr % 2 == 0)))
    {
        SetupDMASend(pBuff,len);
        len_to_return += len;
    }
    else
    {      
#endif
        len_to_return = len;
        while (len)
        {
            while (!(UCB0IFG&UCTXIFG));
            UCB0TXBUF = *pBuff;
            while (!(UCB0IFG&UCRXIFG));
            UCB0RXBUF;
            len --;
            pBuff++;
        }
#ifdef ENABLE_DMA
    }
#endif

    /* At lower SPI clock frequencies the clock may not be in idle state
     * soon after exiting the above loop. Therefore, the user should poll for 
     * for the clock pin (P3.2) to go to idle state(low) before de-asserting 
     * the Chip Select.
     * 
     * while (P3IN & BIT2);
     */

    DEASSERT_CS();
    return len_to_return;
}

#ifdef ENABLE_DMA
void SetupDMAReceive(unsigned char *ucBuff,int len)
{
    UCB0IFG &= ~UCTXIFG;

    /* Setup DMA1 */
    __data16_write_addr((unsigned short) &DMA1SA,(unsigned long) &UCB0RXBUF);
                                            /* Source block address */

    __data16_write_addr((unsigned short) &DMA1DA,(unsigned long) ucBuff);
                                            /* Destination single address */

    DMA1SZ = len;                           /* Block size */

    DMA1CTL = DMADT_0 + DMADSTINCR_3 + DMASBDB + DMALEVEL + DMAEN;


    // Set up DMA0
    __data16_write_addr((unsigned short) &DMA0SA,(unsigned long) &g_ucDinDout[10]);
                                            /* Source block address */

    __data16_write_addr((unsigned short) &DMA0DA,(unsigned long) &UCB0TXBUF);
                                            /* Destination address */

    DMA0SZ = len;                           /* Block size */
    DMA0CTL = DMADT_0 + DMASRCINCR_0 + DMASBDB + DMALEVEL + DMAEN;

    /* Signal DMA xfer */
    UCB0IFG |=  UCTXIFG;

    /* Wait for completation of DMA xfer */
    while(!(DMA0CTL&DMAIFG) || !(DMA1CTL&DMAIFG));
}
#endif

int spi_Read(Fd_t fd, unsigned char *pBuff, int len)
{
    int i = 0;
#ifdef ENABLE_DMA
    int read_size = 0;
    unsigned int pBuffAddr = (unsigned int)pBuff;
#endif    
    
    ASSERT_CS();

#ifdef ENABLE_DMA
    
    if(len>DMA_BUFF_SIZE_MIN && ((pBuffAddr % 2) == 0) )
    {
        while (len>0)
        {
            if( len < MAX_DMA_RECV_TRANSACTION_SIZE)
            {
                SetupDMAReceive(&pBuff[read_size],len);
                read_size += len;
                len = 0;
            }
            else
            {
                SetupDMAReceive(&pBuff[read_size],MAX_DMA_RECV_TRANSACTION_SIZE);
                read_size += MAX_DMA_RECV_TRANSACTION_SIZE;
                len -= MAX_DMA_RECV_TRANSACTION_SIZE;
            }       
        }
        len = read_size;
    }
    else
    {
#endif      
    for (i = 0; i < len; i ++)
    {
        while (!(UCB0IFG&UCTXIFG));
        UCB0TXBUF = 0xFF;
        while (!(UCB0IFG&UCRXIFG));
        pBuff[i] = UCB0RXBUF;
    }
#ifdef ENABLE_DMA
  }    
#endif

    /* At lower SPI clock frequencies the clock may not be in idle state
     * soon after exiting the above loop. Therefore, the user should poll for 
     * for the clock pin (P3.2) to go to idle state(low) before de-asserting 
     * the Chip Select.
     * 
     * while (P3IN & BIT2);
     */

    DEASSERT_CS();

    return len;
}
#endif /* SL_IF_TYPE_UART */
