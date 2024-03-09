/*
 * Copyright (c) 2004 Francisco Llobet-Blandino and the "Miso Project".
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
 * lwm2m_client.h
 *
 *  Created on: 4 dic 2022
 *      Author: Francisco
 */

#ifndef LWM2M_LIGHTCLIENT_LWM2M_CLIENT_H_
#define LWM2M_LIGHTCLIENT_LWM2M_CLIENT_H_

#include "liblwm2m.h"
#include "miso.h"

enum
{
    lwm2m_notify_registration      = (1 << 0),
    lwm2m_notify_timestamp         = (1 << 1),
    lwm2m_notify_temperature       = (1 << 2),
    lwm2m_notify_accelerometer     = (1 << 3),
    lwm2m_notify_message_reception = (1 << 4),
    lwm2m_notify_suspend           = (1 << 5)
};
/**
 * Notify of temperature value change
 */
void lwm2m_client_update_temperature(float temperature);
void lwm2m_client_update_accel(float x, float y, float z);

int lwm2m_client_task_runner(void *param1);

#endif /* LWM2M_LIGHTCLIENT_LWM2M_CLIENT_H_ */
