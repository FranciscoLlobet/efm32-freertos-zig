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

#ifndef __TIMER_IF_H__
#define __TIMER_IF_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/*!
 *  \brief  Timers
 */
typedef enum
{
    TIMER1 = 0x01,
    TIMER2 = 0x02

}e_Timers;

/*!
 *  \brief  Timer modes
 */
typedef enum
{
    ONE_SHOT,
    PERIODIC

}e_TimerModes;

/* Timer Specific Error Codes */
typedef enum{
    TIMER_CREATION_FAILED_ERROR = -0xBB9,        /* Choosing this number to avoid overlap */
    TIMER_DELETION_FAILED_ERROR = TIMER_CREATION_FAILED_ERROR - 1
}e_TimerErrorCodes;

typedef void (*P_TIMER_CALLBACK)();

/*!
    \brief      This function configures the required timer
    \param[in]  timer: Timer to be initialized
    \param[in]  mode: Mode the timer shall be configured in
    \return     0 for success, -1 otherwise
*/
long Timer_IF_Init(e_Timers const timer, e_TimerModes const mode);

/*!
    \brief      This function deints the required timer
    \param[in]  timer: Timer to be deinitialized
    \return     0 for success, -1 otherwise
*/
long Timer_IF_DeInit(e_Timers const timer);

/*!
    \brief      Registers the interrupt-handler for the timer expiry
    \param[in]  timer: Timer number
    \param[in]  callback: Timer interrupt handler
    \return     0 for success, -1 otherwise
*/
long Timer_IF_IntSetup(e_Timers const timer, P_TIMER_CALLBACK callback);

/*!
    \brief      Clears the interrupt
    \param[in]  timer: Timer number
    \param[in]  callback: Timer interrupt handler
    \return     0 for success, -1 otherwise
*/
long Timer_IF_InterruptClear(e_Timers const timer);

/*!
    \brief      Starts the timer
    \param[in]  timer: Timer number
    \param[in]  value: Timeout value in ms
    \return     0 for success, -1 otherwise
*/
long Timer_IF_Start(e_Timers const timer, unsigned long value);

/*!
    \brief      Stops the timer
    \param[in]  timer: Timer number
    \return     0 for success, -1 otherwise
*/
long Timer_IF_Stop(e_Timers const timer);

long Timer_IF_Count(e_Timers const timer);
#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* __TIMER_IF_H__ */
