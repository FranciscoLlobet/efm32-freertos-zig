/*
 * lwm2m_client.h
 *
 *  Created on: 4 dic 2022
 *      Author: Francisco
 */

#ifndef LWM2M_LIGHTCLIENT_LWM2M_CLIENT_H_
#define LWM2M_LIGHTCLIENT_LWM2M_CLIENT_H_

#include "miso.h"
#include "liblwm2m.h"

enum
{
	lwm2m_notify_registration = (1 << 0),
	lwm2m_notify_timestamp = (1 << 1),
	lwm2m_notify_temperature = (1 << 2),
	lwm2m_notify_accelerometer = (1 << 3),
	lwm2m_notify_message_reception = (1 << 4)
};
/**
 * Notify of temperature value change
 */
void lwm2m_client_update_temperature(float temperature);
void lwm2m_client_update_accel(float x, float y, float z);

int lwm2m_client_task_runner(void *param1);

#endif /* LWM2M_LIGHTCLIENT_LWM2M_CLIENT_H_ */
