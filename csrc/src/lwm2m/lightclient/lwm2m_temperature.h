/*
 * lwm2m_temperature.h
 *
 *  Created on: 4 dic 2022
 *      Author: Francisco
 */

#ifndef LWM2M_LIGHTCLIENT_LWM2M_TEMPERATURE_H_
#define LWM2M_LIGHTCLIENT_LWM2M_TEMPERATURE_H_

#include "liblwm2m.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <limits.h>


#define LWM2M_OBJECT_TEMPERATURE   3303

// Resource
#define RESOURCE_ID_SENSOR_VALUE       5700 // Sensor Value, float
#define RESOURCE_ID_MIN_MEASURED_VALUE 5601 // Optional
#define RESOURCE_ID_MAX_MEASURED_VALUE 5602
#define RESOURCE_ID_MIN_RANGE_VALUE    5603
#define RESOURCE_ID_MAX_RANGE_VALUE    5604
#define RESOURCE_ID_SENSOR_UNITS       5701
#define RESOURCE_ID_APPLICATION_TYPE   5750
#define RESOURCE_ID_TIMESTAMP          5518

void write_temperature(float value);
void read_temperature(float value);

#endif /* LWM2M_LIGHTCLIENT_LWM2M_TEMPERATURE_H_ */
