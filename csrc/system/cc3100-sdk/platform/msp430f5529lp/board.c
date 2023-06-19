/*
 * board.c - msp430f5529 launchpad configuration
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

#define XT1_XT2_PORT_SEL            P5SEL
#define XT1_ENABLE                  (BIT4 + BIT5)
#define XT2_ENABLE                  (BIT2 + BIT3)
#define PMM_STATUS_ERROR  1
#define PMM_STATUS_OK     0

P_EVENT_HANDLER                 pIraEventHandler = 0;
BUTTON_PRESS_HANDLER            buttonIrqHandler = 0;

unsigned char IntIsMasked;
unsigned long sec_tick = 0;

#ifdef SL_IF_TYPE_UART
#define ASSERT_UART(expr) {  if (!(expr)) { while(1) ;}}

unsigned char error_overrun = FALSE;
_uartFlowctrl uartFlowctrl;
_uartFlowctrl *puartFlowctrl = &uartFlowctrl;
#endif

/*!
    \brief          Enable the IRQ for button S1 on the Launchpad

    \param[in]      none

    \return         none

    \note

    \warning
*/
static void enable_button_s1_irq()
{
    PADIR &= ~0x1080;
    P2OUT |= BIT1;
    P2DIR &= ~(BIT1);
    P2REN |= BIT1;
    P2IES &= ~(BIT1);
    P2IFG = 0;
    P2IE |= BIT1;
}

/*!
    \brief          Disable the IRQ for button S1 on the Launchpad

    \param[in]      none

    \return         none

    \note

    \warning
*/
static void disable_button_s1_irq()
{
    P2IE &= ~BIT1;
}

/*!
    \brief          Increase Vcore by one level

    \param[in]      level     Level to which Vcore needs to be increased

    \return         status

    \note

    \warning
*/
static uint16_t SetVCoreUp(uint8_t level)
{
    uint16_t PMMRIE_backup, SVSMHCTL_backup, SVSMLCTL_backup;

    /*The code flow for increasing the Vcore has been altered to work around
    * the erratum FLASH37.
    * Please refer to the Errata sheet to know if a specific device is affected
    * DO NOT ALTER THIS FUNCTION */

    /* Open PMM registers for write access */
    PMMCTL0_H = 0xA5;

    /* Disable dedicated Interrupts */
    /* Backup all registers */
    PMMRIE_backup = PMMRIE;
    PMMRIE &= ~(SVMHVLRPE | SVSHPE | SVMLVLRPE | SVSLPE | SVMHVLRIE |
                    SVMHIE | SVSMHDLYIE | SVMLVLRIE | SVMLIE | SVSMLDLYIE );
    SVSMHCTL_backup = SVSMHCTL;
    SVSMLCTL_backup = SVSMLCTL;

    /* Clear flags */
    PMMIFG = 0;

    /* Set SVM highside to new level and check if a VCore increase is possible */
    SVSMHCTL = SVMHE | SVSHE | (SVSMHRRL0 * level);

    /* Wait until SVM highside is settled */
    while ((PMMIFG & SVSMHDLYIFG) == 0);

    /* Clear flag */
    PMMIFG &= ~SVSMHDLYIFG;

    /* Check if a VCore increase is possible */
    if ((PMMIFG & SVMHIFG) == SVMHIFG)
    {
        /* -> Vcc is too low for a Vcore increase */
        /* recover the previous settings */
        PMMIFG &= ~SVSMHDLYIFG;
        SVSMHCTL = SVSMHCTL_backup;

        /* Wait until SVM highside is settled */
        while ((PMMIFG & SVSMHDLYIFG) == 0);

        /* Clear all Flags */
        PMMIFG &= ~(SVMHVLRIFG | SVMHIFG | SVSMHDLYIFG | SVMLVLRIFG | SVMLIFG |
                       SVSMLDLYIFG);

        PMMRIE = PMMRIE_backup;       /* Restore PMM interrupt enable register */
        PMMCTL0_H = 0x00;             /* Lock PMM registers for write access */
        return PMM_STATUS_ERROR;      /* return: voltage not set */
    }

    /* Set also SVS highside to new level */
    /* Vcc is high enough for a Vcore increase */
    SVSMHCTL |= (SVSHRVL0 * level);

    /* Wait until SVM highside is settled */
    while ((PMMIFG & SVSMHDLYIFG) == 0);

    /* Clear flag */
    PMMIFG &= ~SVSMHDLYIFG;

    /* Set VCore to new level */
    PMMCTL0_L = PMMCOREV0 * level;

    /* Set SVM, SVS low side to new level */
    SVSMLCTL = SVMLE | (SVSMLRRL0 * level) | SVSLE | (SVSLRVL0 * level);

    /* Wait until SVM, SVS low side is settled */
    while ((PMMIFG & SVSMLDLYIFG) == 0);

    /* Clear flag */
    PMMIFG &= ~SVSMLDLYIFG;
    /*SVS, SVM core and high side are now set to protect for the new core level*/

    /* Restore Low side settings */
    /* Clear all other bits _except_ level settings */
    SVSMLCTL &= (SVSLRVL0+SVSLRVL1+SVSMLRRL0+SVSMLRRL1+SVSMLRRL2);

    /* Clear level settings in the backup register,keep all other bits */
    SVSMLCTL_backup &= ~(SVSLRVL0+SVSLRVL1+SVSMLRRL0+SVSMLRRL1+SVSMLRRL2);

    /* Restore low-side SVS monitor settings */
    SVSMLCTL |= SVSMLCTL_backup;

    /* Restore High side settings */
    /* Clear all other bits except level settings */
    SVSMHCTL &= (SVSHRVL0+SVSHRVL1+SVSMHRRL0+SVSMHRRL1+SVSMHRRL2);

    /* Clear level settings in the backup register,keep all other bits */
    SVSMHCTL_backup &= ~(SVSHRVL0+SVSHRVL1+SVSMHRRL0+SVSMHRRL1+SVSMHRRL2);

    /* Restore backup */
    SVSMHCTL |= SVSMHCTL_backup;

    /* Wait until high side, low side settled */
    while (((PMMIFG & SVSMLDLYIFG) == 0) && ((PMMIFG & SVSMHDLYIFG) == 0));

    /* Clear all Flags */
    PMMIFG &= ~(SVMHVLRIFG | SVMHIFG | SVSMHDLYIFG | SVMLVLRIFG | SVMLIFG
                    | SVSMLDLYIFG);

    PMMRIE = PMMRIE_backup;     /* Restore PMM interrupt enable register */
    PMMCTL0_H = 0x00;           /* Lock PMM registers for write access */

    return PMM_STATUS_OK;
}

/*!
    \brief Decrease Vcore by one level

    \param[in]      level     Level to which Vcore needs to be decreased

    \return         status

    \note

    \warning
*/
static uint16_t SetVCoreDown(uint8_t level)
{
    uint16_t PMMRIE_backup, SVSMHCTL_backup, SVSMLCTL_backup;

    /* The code flow for decreasing the Vcore has been altered to work around
    * the erratum FLASH37.
    * Please refer to the Errata sheet to know if a specific device is affected
    * DO NOT ALTER THIS FUNCTION */

    /* Open PMM registers for write access */
    PMMCTL0_H = 0xA5;

    /* Disable dedicated Interrupts */
    /* Backup all registers */
    PMMRIE_backup = PMMRIE;
    PMMRIE &= ~(SVMHVLRPE | SVSHPE | SVMLVLRPE | SVSLPE | SVMHVLRIE |
                       SVMHIE | SVSMHDLYIE | SVMLVLRIE | SVMLIE | SVSMLDLYIE );
    SVSMHCTL_backup = SVSMHCTL;
    SVSMLCTL_backup = SVSMLCTL;

    /* Clear flags */
    PMMIFG &= ~(SVMHIFG | SVSMHDLYIFG | SVMLIFG | SVSMLDLYIFG);

    /* Set SVM, SVS high & low side to new settings in normal mode */
    SVSMHCTL = SVMHE | (SVSMHRRL0 * level) | SVSHE | (SVSHRVL0 * level);
    SVSMLCTL = SVMLE | (SVSMLRRL0 * level) | SVSLE | (SVSLRVL0 * level);

    /* Wait until SVM high side and SVM low side is settled */
    while ((PMMIFG & SVSMHDLYIFG) == 0 || (PMMIFG & SVSMLDLYIFG) == 0);

    /* Clear flags */
    PMMIFG &= ~(SVSMHDLYIFG + SVSMLDLYIFG);
    /*SVS, SVM core and high side are now set to protect for the new core level*/

    /* Set VCore to new level */
    PMMCTL0_L = PMMCOREV0 * level;

    /* Restore Low side settings */
    /* Clear all other bits _except_ level settings */
    SVSMLCTL &= (SVSLRVL0+SVSLRVL1+SVSMLRRL0+SVSMLRRL1+SVSMLRRL2);

    /* Clear level settings in the backup register,keep all other bits */
    SVSMLCTL_backup &= ~(SVSLRVL0+SVSLRVL1+SVSMLRRL0+SVSMLRRL1+SVSMLRRL2);

    /* Restore low-side SVS monitor settings */
    SVSMLCTL |= SVSMLCTL_backup;

    /* Restore High side settings */
    /* Clear all other bits except level settings */
    SVSMHCTL &= (SVSHRVL0+SVSHRVL1+SVSMHRRL0+SVSMHRRL1+SVSMHRRL2);

    /* Clear level settings in the backup register, keep all other bits */
    SVSMHCTL_backup &= ~(SVSHRVL0+SVSHRVL1+SVSMHRRL0+SVSMHRRL1+SVSMHRRL2);

    /* Restore backup */
    SVSMHCTL |= SVSMHCTL_backup;

    /* Wait until high side, low side settled */
    while (((PMMIFG & SVSMLDLYIFG) == 0) && ((PMMIFG & SVSMHDLYIFG) == 0));

    /* Clear all Flags */
    PMMIFG &= ~(SVMHVLRIFG | SVMHIFG | SVSMHDLYIFG | SVMLVLRIFG | SVMLIFG
                    | SVSMLDLYIFG);

    PMMRIE = PMMRIE_backup;           /* Restore PMM interrupt enable register */
    PMMCTL0_H = 0x00;                 /* Lock PMM registers for write access */
    return PMM_STATUS_OK;             /* Return: OK */
}

uint16_t SetVCore(uint8_t level)
{
    uint16_t actlevel;
    uint16_t status = 0;

    level &= PMMCOREV_3;                       /* Set Mask for Max. level */
    actlevel = (PMMCTL0 & PMMCOREV_3);         /* Get actual VCore */
    /* step by step increase or decrease */
    while (((level != actlevel) && (status == 0)) || (level < actlevel))
    {
        if (level > actlevel)
        {
            status = SetVCoreUp(++actlevel);
        }
        else
        {
            status = SetVCoreDown(--actlevel);
        }
    }

    return status;
}

void LFXT_Start(uint16_t xtdrive)
{
    /* If the drive setting is not already set to maximum */
    /* Set it to max for LFXT startup */
    if ((UCSCTL6 & XT1DRIVE_3)!= XT1DRIVE_3)
    {
        /* Highest drive setting for XT1startup */
        UCSCTL6_L |= XT1DRIVE1_L + XT1DRIVE0_L;
    }

    while (SFRIFG1 & OFIFG)
    {   /* Check OFIFG fault flag */
        UCSCTL7 &= ~(DCOFFG+XT1LFOFFG+XT1HFOFFG+XT2OFFG);
        SFRIFG1 &= ~OFIFG;        /* Clear OFIFG fault flag */
    }

    UCSCTL6 = (UCSCTL6 & ~(XT1DRIVE_3)) | (xtdrive); /*set requested Drive mode */
}


void Init_FLL(uint16_t fsystem, uint16_t ratio)
{
    uint16_t d, dco_div_bits;
    uint16_t mode = 0;

    /*Save actual state of FLL loop control, then disable it. This is needed to
    * prevent the FLL from acting as we are making fundamental modifications to
    * the clock setup. */
    uint16_t srRegisterState = __get_SR_register() & SCG0;
    __bic_SR_register(SCG0);

    d = ratio;
    dco_div_bits = FLLD__2;        /* Have at least a divider of 2 */

    if (fsystem > 16000)
    {
        d >>= 1 ;
        mode = 1;
    }
    else
    {
        fsystem <<= 1;               /* fsystem = fsystem * 2 */
    }

    while (d > 512)
    {
        dco_div_bits = dco_div_bits + FLLD0;  /* Set next higher div level */
        d >>= 1;
    }

    UCSCTL0 = 0x0000;              /* Set DCO to lowest Tap */

    UCSCTL2 &= ~(0x03FF);          /* Reset FN bits */
    UCSCTL2 = dco_div_bits | (d - 1);

    if (fsystem <= 630)            /*           fsystem < 0.63MHz */
        UCSCTL1 = DCORSEL_0;
    else if (fsystem <  1250)      /* 0.63MHz < fsystem < 1.25MHz */
        UCSCTL1 = DCORSEL_1;
    else if (fsystem <  2500)      /* 1.25MHz < fsystem <  2.5MHz */
        UCSCTL1 = DCORSEL_2;
    else if (fsystem <  5000)      /* 2.5MHz  < fsystem <    5MHz */
        UCSCTL1 = DCORSEL_3;
    else if (fsystem <  10000)     /* 5MHz    < fsystem <   10MHz */
        UCSCTL1 = DCORSEL_4;
    else if (fsystem <  20000)     /* 10MHz   < fsystem <   20MHz */
        UCSCTL1 = DCORSEL_5;
    else if (fsystem <  40000)     /* 20MHz   < fsystem <   40MHz */
        UCSCTL1 = DCORSEL_6;
    else
        UCSCTL1 = DCORSEL_7;

    while (SFRIFG1 & OFIFG)
    {                               /* Check OFIFG fault flag */
        UCSCTL7 &= ~(DCOFFG+XT1LFOFFG+XT1HFOFFG+XT2OFFG);/*Clear OSC flaut Flags */
        SFRIFG1 &= ~OFIFG;                              /*Clear OFIFG fault flag */
    }

    if (mode == 1)
    {                                        /* fsystem > 16000 */
        SELECT_MCLK_SMCLK(SELM__DCOCLK + SELS__DCOCLK);     /* Select DCOCLK */
    }
    else
    {
        SELECT_MCLK_SMCLK(SELM__DCOCLKDIV + SELS__DCOCLKDIV);/*Select DCODIVCLK*/
    }

    __bis_SR_register(srRegisterState);            /* Restore previous SCG0 */
}

void Init_FLL_Settle(uint16_t fsystem, uint16_t ratio)
{
    volatile uint16_t x = ratio * 32;

    Init_FLL(fsystem, ratio);

    while (x--)
    {
        __delay_cycles(30);
    }
}

void platform_timer_init()
{
    
    /* Initialize the timer that ticks every second. */
    TA2CCR0 = 32700;
    TA2CTL = TASSEL_1 + MC_1 + ID_0;
    TA2CCTL0 = CCIE;

    P1DIR = BIT0;
    P4DIR = BIT7;

    __bis_SR_register(GIE); /* Enable maskable interrupts */
}

int registerInterruptHandler(P_EVENT_HANDLER InterruptHdl , void* pValue)
{
    pIraEventHandler = InterruptHdl;

    return 0;
}


void CC3100_disable()
{
    P1OUT &= ~BIT6;
}


void CC3100_enable()
{
    P1OUT |= BIT6;
}

void CC3100_InterruptEnable(void)
{
    P2IES &= ~BIT0;
    P2IE |= BIT0;

#ifdef SL_IF_TYPE_UART
    UCA0IE |= UCRXIE;
#endif

}

void CC3100_InterruptDisable()
{
    P2IE &= ~BIT0;
#ifdef SL_IF_TYPE_UART
    UCA0IE &= ~UCRXIE;
#endif
}

void MaskIntHdlr()
{
    IntIsMasked = TRUE;
}

void UnMaskIntHdlr()
{
    IntIsMasked = FALSE;
}

void set_rts()
{
    P1OUT |= BIT4;
}

void clear_rts()
{
    P1OUT &= ~BIT4;
}

void initClk()
{
    /* Setup XT1 and XT2 */
    XT1_XT2_PORT_SEL |= XT1_ENABLE + XT2_ENABLE;

    /* Set Vcore to accomodate for max. allowed system speed */
    SetVCore(3);
    /* Use 32.768kHz XTAL as reference */
    LFXT_Start(XT1DRIVE_0);

    /* Set system clock to max (25MHz) */
    Init_FLL_Settle(25000, 762);
    SFRIFG1 = 0;
    SFRIE1 |= OFIE;

    /* Globally enable interrupts */
    __enable_interrupt();
}

void stopWDT()
{
    WDTCTL = WDTPW + WDTHOLD;
}

void initLEDs()
{
    P1OUT &= ~BIT0;
    P4OUT &= ~BIT7;
    P1DIR |= BIT0;
    P4DIR |= BIT7;
    P1SEL &= ~BIT0;
    P4SEL &= ~BIT7;
}

void turnLedOn(char ledNum)
{
    switch(ledNum)
    {
      case LED1:
        P1OUT |= BIT0;
        break;
      case LED2:
        P4OUT |= BIT7;
        break;
    }
}

void turnLedOff(char ledNum)
{
    switch(ledNum)
    {
      case LED1:
        P1OUT &= ~BIT0;
        break;
      case LED2:
        P4OUT &= ~BIT7;
        break;
    }
}


void toggleLed(char ledNum)
{
    switch(ledNum)
    {
      case LED1:
        P1OUT ^= BIT0;
        break;
      case LED2:
        P4OUT ^= BIT7;
        break;
    }

}

unsigned char GetLEDStatus()
{
  unsigned char status = 0;

  if(P1OUT & BIT0)
    status |= (1 << 0);
  if(P4OUT & BIT7)
    status |= (1 << 1);

  return status;
}

void enableButtonIrq(void)
{
    enable_button_s1_irq();
}

void disableButtonIrq(void)
{
    disable_button_s1_irq();
}

int registerButtonIrqHandler(BUTTON_PRESS_HANDLER InterruptHdl , void* pValue)
{
    buttonIrqHandler = InterruptHdl;

    /* Enable Switch interrupt*/
    enableButtonIrq();

    return 0;
}

void initAntSelGPIO()
{
    P2OUT &= ~BIT5;
    P2OUT |=  BIT4; /* Select Antenna 1 */
    P2SEL &= ~(BIT4 + BIT5);
    P2DIR |= (BIT4 + BIT5);
}

void SelAntenna(int antenna)
{
    switch(antenna)
   {
        case ANT1:
            P2OUT &= ~BIT5;
            P2OUT |=  BIT4;
            break;
        case ANT2:
            P2OUT &= ~BIT4;
            P2OUT |=  BIT5;
            break;
   }
}

#if defined(__TI_COMPILER_VERSION__) || defined(__IAR_SYSTEMS_ICC__)
#pragma vector=PORT1_VECTOR
__interrupt
#elif defined(__GNUC__)
__attribute__((interrupt(PORT1_VECTOR)))
#endif
void Port1_ISR(void)
{
    /* Context save interrupt flag before calling interrupt vector. */
    /* Reading interrupt vector generator will automatically clear IFG flag */

    switch (__even_in_range(P1IV, P1IV_P1IFG7))
    {
        /* Vector  P1IV_NONE:  No Interrupt pending */
        case  P1IV_NONE:
            break;

        /* Vector  P1IV_P1IFG0:  P1IV P1IFG.0 */
        case  P1IV_P1IFG0:
            break;

        /* Vector  P1IV_P1IFG1:  P1IV P1IFG.1 */
        case  P1IV_P1IFG1:
            break;

        /* Vector  P1IV_P1IFG2:  P1IV P1IFG.2 */
        case  P1IV_P1IFG2:
            break;

        /* Vector  P1IV_P1IFG3:  P1IV P1IFG.3 */
        case  P1IV_P1IFG3:
            break;

        /* Vector  P1IV_P1IFG4:  P1IV P1IFG.4 */
        case  P1IV_P1IFG4:
            break;

        /* Vector  P1IV_P1IFG5:  P1IV P1IFG.5 */
        case  P1IV_P1IFG5:
            break;

        /* Vector  P1IV_P1IFG1:  P1IV P1IFG.6 */
        case  P1IV_P1IFG6:
            break;

        /* Vector  P1IV_P1IFG7:  P1IV P1IFG.7 */
        case  P1IV_P1IFG7:
            break;

        /* Default case */
        default:
            break;
    }
}

void Delay(unsigned long interval)
{
    while(interval > 0)
    {
        __delay_cycles(25000);
        interval--;
    }
}

unsigned long platform_get_time_in_secs()
{
    return sec_tick;
}

/*!
    \brief          The IntSpiGPIOHandler interrupt handler

    \param[in]      none

    \return         none

    \note

    \warning
*/
#if defined(__TI_COMPILER_VERSION__) || defined(__IAR_SYSTEMS_ICC__)
#pragma vector=PORT2_VECTOR
__interrupt
#elif defined(__GNUC__)
__attribute__((interrupt(PORT2_VECTOR)))
#endif
void IntSpiGPIOHandler(void)
{
    switch(__even_in_range(P2IV, P2IV_P2IFG7))
    {
      case P2IV_P2IFG0:
#ifndef SL_IF_TYPE_UART
        if (pIraEventHandler)
        {
            pIraEventHandler(0);
        }
#else
        if(puartFlowctrl->bRtsSetByFlowControl == FALSE)
        {
            clear_rts();
        }

#endif
        break;

      case P2IV_P2IFG1:
        if(buttonIrqHandler)
        {
            disableButtonIrq();
            buttonIrqHandler();
        }
      break;

    default:
        break;
    }
}

#if defined(__TI_COMPILER_VERSION__) || defined(__IAR_SYSTEMS_ICC__)
#pragma vector=TIMER2_A0_VECTOR
__interrupt
#elif defined(__GNUC__)
__attribute__((interrupt(USCI_A0_VECTOR)))
#endif
void TIMER_A2_ISR(void)
{
    sec_tick++;
}

/*!
    \brief          The UART A0 interrupt handler

    \param[in]      none

    \return         none

    \note

    \warning
*/
#if defined(__TI_COMPILER_VERSION__) || defined(__IAR_SYSTEMS_ICC__)
#pragma vector=USCI_A0_VECTOR
__interrupt
#elif defined(__GNUC__)
__attribute__((interrupt(USCI_A0_VECTOR)))
#endif
void CC3100_UART_ISR(void)
{
    switch(__even_in_range(UCA0IV,0x08))
    {
        case 0:break;                             /* Vector 0 - no interrupt */
        case 2:                                   /* Vector 2 - RXIF */
#ifdef SL_IF_TYPE_UART
        {
            unsigned char ByteRead;

            while((UCA0IFG & UCRXIFG) != 0);

            if(UCRXERR & UCA1STAT)
            {
                if(UCOE & UCA1STAT)
                {
                    error_overrun = TRUE;
                }
                ASSERT_UART(0);
            }

            ByteRead = UCA0RXBUF;

            if(puartFlowctrl->bActiveBufferIsJitterOne == TRUE)
            {
                if(puartFlowctrl->JitterBufferFreeBytes > 0)
                {
                    puartFlowctrl->JitterBuffer[puartFlowctrl->JitterBufferWriteIdx] = ByteRead;
                    puartFlowctrl->JitterBufferFreeBytes--;
                    puartFlowctrl->JitterBufferWriteIdx++;

                    if((FALSE == IntIsMasked) && (NULL != pIraEventHandler))
                    {
                        pIraEventHandler(0);
                    }
                }
                else
                {
                    if(P1OUT & BIT3)
                    {
                        ASSERT_UART(0);
                    }
                }

                if(puartFlowctrl->JitterBufferFreeBytes <= UART_READ_JITTER_RTS_GUARD)
                {
                    set_rts();
                    puartFlowctrl->bRtsSetByFlowControl = TRUE;
                }

                if(puartFlowctrl->JitterBufferWriteIdx > (UART_READ_JITTER_BUFFER_SIZE - 1))
                {
                    puartFlowctrl->JitterBufferWriteIdx = 0;
                }
            }
            else
            {
                puartFlowctrl->pActiveBuffer[puartFlowctrl->ActiveBufferWriteCounter++] = ByteRead;
            }
        }
#endif
            break;
        case 4:break;                             /* Vector 4 - TXIFG */
        default: break;
    }
}

/* TIMER0_A0_VECTOR is used by FreeRTOS port.c */

/* Catch interrupt vectors that are not initialized. */
#ifdef __CCS__
#pragma vector=WDT_VECTOR, ADC12_VECTOR, USCI_B1_VECTOR, \
    TIMER1_A1_VECTOR, TIMER0_A1_VECTOR, \
    TIMER2_A1_VECTOR, COMP_B_VECTOR, USB_UBM_VECTOR, UNMI_VECTOR,DMA_VECTOR, \
    TIMER0_B0_VECTOR, TIMER0_B1_VECTOR,SYSNMI_VECTOR, USCI_B0_VECTOR, RTC_VECTOR
__interrupt void Trap_ISR(void)
{
    while(1);
}

#endif

