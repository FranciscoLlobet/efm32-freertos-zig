/*
 * uiso.h
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

#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"
#include "semphr.h"


#include "sl_iostream.h"
#include "sl_iostream_swo.h"

enum
{
	uiso_rtos_prio_idle = 0, /* Lowest priority. Reserved for idle task  */
	uiso_rtos_prio_low = 1, /* Temperature collector, sensor task*/
	uiso_rtos_prio_below_normal = 2,
	uiso_rtos_prio_normal = 3, /* lwm2m task, mqtt task, select task */
	uiso_rtos_prio_above_normal = 4, /* Wifi service, SL Spawn */
	uiso_rtos_prio_high = 5, /* Timer Service */
	uiso_rtos_prio_highest = 6
};

/* New */


/* LWM2M, MQTT */
/* Select task */
/* Wifi Service */
/* SL Spawn */
/* Timer */

enum
{
	uiso_task_prio_idle = uiso_rtos_prio_idle,
	uiso_task_housekeeping_services = uiso_rtos_prio_low,
	uiso_task_runtime_services = uiso_rtos_prio_normal,
	uiso_task_event_services = uiso_rtos_prio_above_normal,
	uiso_task_connectivity_service = uiso_rtos_prio_above_normal,
	uiso_task_prio_timer_deamon = uiso_rtos_prio_high, /* Reserved for the timer */
	uiso_task_prio_hardware_service = uiso_rtos_prio_highest,
};

/**
 * Application system reset
 *
 * This call is asynchronous. It enqueues a system reset request to the Timer Service task.
 */
void uiso_reboot(void);

#endif /* UISO_H_ */
