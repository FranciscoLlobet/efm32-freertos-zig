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
#ifndef MISO_H_
#define MISO_H_

#include <FreeRTOS.h>
#include <semphr.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <task.h>
#include <timers.h>

#include "board.h"
#include "sl_iostream.h"
#include "sl_iostream_swo.h"

enum
{
    miso_rtos_prio_idle         = 0, /* Lowest priority. Reserved for idle task  */
    miso_rtos_prio_low          = 1, /* Temperature collector, sensor task*/
    miso_rtos_prio_below_normal = 2,
    miso_rtos_prio_normal       = 3, /* lwm2m task, mqtt task, select task */
    miso_rtos_prio_above_normal = 4, /* Wifi service, SL Spawn */
    miso_rtos_prio_high         = 5, /* Timer Service */
    miso_rtos_prio_highest      = 6
};

enum
{
    miso_task_prio_idle             = miso_rtos_prio_idle,
    miso_task_housekeeping_services = miso_rtos_prio_low,
    miso_task_runtime_services      = miso_rtos_prio_normal,
    miso_task_event_services        = miso_rtos_prio_above_normal,
    miso_task_connectivity_service  = miso_rtos_prio_above_normal,
    miso_task_prio_timer_deamon     = miso_rtos_prio_high, /* Reserved for the timer */
    miso_task_prio_hardware_service = miso_rtos_prio_highest,
};

/**
 * Application system reset
 *
 * This call is asynchronous. It enqueues a system reset request to the Timer Service task.
 */
void miso_reboot(void);

enum miso_event
{
    miso_connectivity_on  = (1 << 0),
    miso_connectivity_off = (1 << 1),
    miso_lwm2m_suspended  = (1 << 2),
    miso_ntp_sync         = (1 << 3),
    miso_user_timer       = (1 << 4),

};

extern void miso_notify_event(enum miso_event event);

void miso_mqtt_resume(void);
void miso_mqtt_suspend(void);

void miso_lwm2m_resume(void);
void miso_lwm2m_suspend(void);

#endif /* MISO_H_ */
