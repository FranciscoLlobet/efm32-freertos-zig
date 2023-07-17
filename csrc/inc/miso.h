/*
 * miso.h
 *
 *  Created on: 12 nov 2022
 *      Author: Francisco
 */

#ifndef UISO_H_
#define UISO_H_

#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>

#include "board.h"

#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"
#include "semphr.h"


#include "sl_iostream.h"
#include "sl_iostream_swo.h"

enum
{
	miso_rtos_prio_idle = 0, /* Lowest priority. Reserved for idle task  */
	miso_rtos_prio_low = 1, /* Temperature collector, sensor task*/
	miso_rtos_prio_below_normal = 2,
	miso_rtos_prio_normal = 3, /* lwm2m task, mqtt task, select task */
	miso_rtos_prio_above_normal = 4, /* Wifi service, SL Spawn */
	miso_rtos_prio_high = 5, /* Timer Service */
	miso_rtos_prio_highest = 6
};

/* New */


/* LWM2M, MQTT */
/* Select task */
/* Wifi Service */
/* SL Spawn */
/* Timer */

enum
{
	miso_task_prio_idle = miso_rtos_prio_idle,
	miso_task_housekeeping_services = miso_rtos_prio_low,
	miso_task_runtime_services = miso_rtos_prio_normal,
	miso_task_event_services = miso_rtos_prio_above_normal,
	miso_task_connectivity_service = miso_rtos_prio_above_normal,
	miso_task_prio_timer_deamon = miso_rtos_prio_high, /* Reserved for the timer */
	miso_task_prio_hardware_service = miso_rtos_prio_highest,
};

/**
 * Application system reset
 *
 * This call is asynchronous. It enqueues a system reset request to the Timer Service task.
 */
void miso_reboot(void);

#endif /* UISO_H_ */
