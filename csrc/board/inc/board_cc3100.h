/*
 * Copyright (c) 2022-2024 Francisco Llobet-Blandino and the "Miso Project".
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * board_cc3100.h
 *
 *  Created on: 11 nov 2022
 *      Author: Francisco
 *
 * Board support component for the CC3100 Wifi module
 *
 */

#ifndef BOARD_CC3100_H_
#define BOARD_CC3100_H_

#include "user.h"
void Board_CC3100_Init(void);

void CC3100_DeviceEnablePreamble(void);
void CC3100_DeviceEnable(void);
void CC3100_DeviceDisable(void);

#define CC3100_DEVICE_NAME "CC3100"

typedef int Fd_t;
typedef void (*P_EVENT_HANDLER)(void* pValue);

/* Interface Adaption */
int CC3100_IfOpen(const char* pIfName, unsigned long flags);
int CC3100_IfClose(Fd_t Fd);
int CC3100_IfWrite(Fd_t Fd, const uint8_t* pBuff, int Len);
int CC3100_IfRead(Fd_t Fd, uint8_t* pBuff, int Len);

void CC3100_IfRegIntHdlr(P_EVENT_HANDLER interruptHdl, void* pValue);
void CC3100_MaskIntHdlr(void);
void CC3100_UnmaskIntHdlr(void);
#endif /* BOARD_CC3100_H_ */
