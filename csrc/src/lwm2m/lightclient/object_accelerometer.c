/*
 * object_accelerometer.c
 *
 *  Created on: 5 dic 2022
 *      Author: Francisco
 */

#include "miso.h"
#include "liblwm2m.h"

#define LWM2M_ACCELEROMETER_OBJECT_ID	(3313)

enum
{
	lwm2m_accel_x_value = 5702,
	lwm2m_accel_y_value = 5703,
	lwm2m_accel_z_value = 5704,
	lwm2m_accel_sensor_units = 5701,
	lwm2m_accel_min_range_value = 5603,
	lwm2m_accel_max_range_value = 5604,
	lwm2m_accel_application_type = 5750,
	lwm2m_accel_timestamp = 5518,
	lwm2m_accel_fractional_timestamp = 6050,
	lwm2m_accel_quality_indicator = 6042,
	lwm2m_accel_quality_level = 6049,
};

static const uint16_t resource_list[] =
{ lwm2m_accel_x_value, lwm2m_accel_y_value, lwm2m_accel_z_value,
		lwm2m_accel_sensor_units, lwm2m_accel_application_type};

static const int num_resource_list = sizeof(resource_list) / sizeof(resource_list[0]);

struct accelerometer_values_s
{
	float x;
	float y;
	float z;
	uint32_t timestamp;
};

volatile struct accelerometer_values_s accelerometer_values;

void update_accelerometer_values(float x, float y, float z)
{
	accelerometer_values.x = x;
	accelerometer_values.y = y;
	accelerometer_values.z = z;
}


uint8_t accel_read(lwm2m_context_t *contextP, uint16_t instanceId,
		int *numDataP, lwm2m_data_t **dataArrayP, lwm2m_object_t *objectP)
{
	uint8_t ret = COAP_NO_ERROR;

	struct accelerometer_values_s * values = &accelerometer_values;

	int i = -1; /* Iterator */

	if (instanceId > 0)
	{
		ret = COAP_404_NOT_FOUND;
	}

	if (COAP_NO_ERROR == ret)
	{
		if (*numDataP == 0)
		{
			// Peer asks for full object
			*dataArrayP = lwm2m_data_new(num_resource_list);
			if (NULL == *dataArrayP)
			{
				return COAP_500_INTERNAL_SERVER_ERROR ;
			}

			*numDataP = num_resource_list;

			/* Populate resource list */
			for (i = 0; i < num_resource_list; i++)
			{
				(*dataArrayP)[i].id = resource_list[i];
			}
		}
		else if (*numDataP < 0)
		{
			ret = COAP_500_INTERNAL_SERVER_ERROR;
		}
	}

	if (COAP_NO_ERROR == ret)
	{
		for (i = 0; i < *numDataP; i++)
		{
			lwm2m_data_t *data_point = (*dataArrayP) + i;

			switch (data_point->id)
			{
			case lwm2m_accel_x_value:
				lwm2m_data_encode_float(values->x, data_point);
				ret = COAP_205_CONTENT;
				break;
			case lwm2m_accel_y_value:
				lwm2m_data_encode_float(values->y, data_point);
				ret = COAP_205_CONTENT;
				break;
			case lwm2m_accel_z_value:
				lwm2m_data_encode_float(values->z, data_point);
				ret = COAP_205_CONTENT;
				break;
			case lwm2m_accel_sensor_units:
				lwm2m_data_encode_string("LSB", data_point);
				ret = COAP_205_CONTENT;
				break;
			case lwm2m_accel_application_type:
				lwm2m_data_encode_string("Bosch Sensortec BMA280", data_point);
				ret = COAP_205_CONTENT;
				break;
			default:
				ret = COAP_404_NOT_FOUND;
				break;
			}

			if (COAP_205_CONTENT != ret)
				break;
		}
	}

	return ret;
}

uint8_t accel_discover(lwm2m_context_t *contextP, uint16_t instanceId,
		int *numDataP, lwm2m_data_t **dataArrayP, lwm2m_object_t *objectP)
{
	uint8_t ret = COAP_NO_ERROR;
	int i = -1;

	if (0 == *numDataP)
	{
		// Peer asks for full object
		*dataArrayP = lwm2m_data_new(num_resource_list);
		if (NULL == *dataArrayP)
		{
			return COAP_500_INTERNAL_SERVER_ERROR ;
		}

		*numDataP = num_resource_list;

		/* Populate resource list */
		for (i = 0; i < num_resource_list; i++)
		{
			(*dataArrayP)[i].id = resource_list[i];
		}

		ret = COAP_205_CONTENT;
	}
	else
	{
		for (i = 0; i < *numDataP; i++)
		{
			lwm2m_data_t *data_point = (*dataArrayP) + i;
			switch (data_point->id)
			{
			case lwm2m_accel_x_value:
				ret = COAP_205_CONTENT;
				break;
			case lwm2m_accel_y_value:
				ret = COAP_205_CONTENT;
				break;
			case lwm2m_accel_z_value:
				ret = COAP_205_CONTENT;
				break;
			case lwm2m_accel_sensor_units:
				ret = COAP_205_CONTENT;
				break;
			case lwm2m_accel_application_type:
				ret = COAP_205_CONTENT;
				break;
			default:
				ret = COAP_404_NOT_FOUND;
				break;
			}
			if (COAP_205_CONTENT != ret)
				break;
		}
	}

	return ret;
}

void free_accelerometer_object(lwm2m_object_t * accelerometer)
{
	if(accelerometer != NULL)
	{
		if(accelerometer->instanceList!= NULL)
		{
			lwm2m_free(accelerometer->instanceList);

		}
		lwm2m_free(accelerometer);
	}


}

void* get_accelerometer_object(void)
{
	lwm2m_object_t *accelerometer = lwm2m_malloc(sizeof(lwm2m_object_t));
	if (NULL != accelerometer)
	{
		memset(accelerometer, 0, sizeof(lwm2m_object_t));
	}
	else
	{
		return NULL;
	}

	accelerometer->instanceList = lwm2m_malloc(sizeof(lwm2m_list_t));
	if (NULL != accelerometer->instanceList)
	{
		memset(accelerometer->instanceList, 0, sizeof(lwm2m_list_t));
	}
	else
	{
		lwm2m_free(accelerometer);
		return NULL;
	}

	accelerometer->objID = LWM2M_ACCELEROMETER_OBJECT_ID;

	accelerometer_values.x = 0.0F;
	accelerometer_values.y = 0.0F;
	accelerometer_values.z = 0.0F;
	accelerometer_values.timestamp = 0;

	accelerometer->userData = &accelerometer_values;
	accelerometer->versionMajor = 1;
	accelerometer->versionMinor = 1;

	accelerometer->createFunc = NULL;
	accelerometer->deleteFunc = NULL;
	accelerometer->discoverFunc = accel_discover;
	accelerometer->executeFunc = NULL;
	accelerometer->readFunc = accel_read;
	accelerometer->writeFunc = NULL;

	return (void*) accelerometer;
}
