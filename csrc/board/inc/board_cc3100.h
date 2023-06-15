/*
 * board_cc3100.h
 *
 *  Created on: 11 nov 2022
 *      Author: Francisco
 */

#ifndef BOARD_CC3100_H_
#define BOARD_CC3100_H_

#include "user.h"
void Board_CC3100_Init(void);

void CC3100_DeviceEnablePreamble(void);
void CC3100_DeviceEnable(void);
void CC3100_DeviceDisable(void);

#define CC3100_DEVICE_NAME   "CC3100"

typedef int Fd_t;
typedef void (*P_EVENT_HANDLER)(void* pValue);

/* Interface Adaption */
int CC3100_IfOpen(char* pIfName , unsigned long flags);
int CC3100_IfClose(Fd_t Fd);
int CC3100_IfWrite(Fd_t Fd , uint8_t * pBuff , int Len);
int CC3100_IfRead(Fd_t Fd , uint8_t * pBuff , int Len);

void CC3100_IfRegIntHdlr(P_EVENT_HANDLER interruptHdl, void * pValue);
void CC3100_MaskIntHdlr(void);
void CC3100_UnmaskIntHdlr(void);
#endif /* BOARD_CC3100_H_ */
