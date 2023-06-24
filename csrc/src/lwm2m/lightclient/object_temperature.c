/*
 * object_temperature.c
 *
 *  Created on: 10 dic 2022
 *      Author: Francisco
 */
#include "uiso.h"
#include "liblwm2m.h"

#define LWM2M_TEMPERATURE_OBJECT_ID		(3303)

enum
{
	lwm2m_temp_sensor_value = 5700,
	lwm2m_temp_min_measured_value = 5601,
	lwm2m_temp_max_measured_value = 5602,
	lwm2m_temp_min_range_value = 5603,
	lwm2m_temp_max_range_value = 5604,
	lwm2m_temp_sensor_units = 5701,
	lwm2m_temp_reset_min_max_measured_values = 5605,
	lwm2m_temp_application_type = 5750,
	lwm2m_temp_timestamp = 5518,
	lwm2m_temp_fractional_timestamp = 6050,
	lwm2m_temp_measurement_quality_indicator = 6042,
	lwm2m_temp_measurement_quality_level = 6049,
};

static const uint16_t resource_list[] =
{ lwm2m_temp_sensor_value, lwm2m_temp_sensor_units, lwm2m_temp_application_type};

static const int num_resource_list = sizeof(resource_list) / sizeof(resource_list[0]);

/* static allocation */
static lwm2m_object_t temperature_object;
static lwm2m_list_t temperature_instance[1];

static struct
{
	float value;
	float min_value;
	float max_value;

}temperature;

void write_temperature(float value)
{
	temperature.value = value;
}

static uint8_t _read(lwm2m_context_t *contextP, uint16_t instanceId,
		int *numDataP, lwm2m_data_t **dataArrayP, lwm2m_object_t *objectP)
{
	(void)contextP;

	uint8_t ret = COAP_NO_ERROR;
	int i = -1; /* Iterator */

	lwm2m_list_t * instance = lwm2m_list_find(objectP->instanceList, instanceId);

	if (NULL == instance)
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
			case lwm2m_temp_sensor_value:
				lwm2m_data_encode_float(temperature.value, data_point);
				ret = COAP_205_CONTENT;
				break;
			case lwm2m_temp_sensor_units:
				lwm2m_data_encode_string("degree Celsius", data_point);
				ret = COAP_205_CONTENT;
				break;
			case lwm2m_temp_application_type:
				lwm2m_data_encode_string("Bosch Sensortec BME280", data_point);
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

void* get_temperature_object(void)
{
	memset(&temperature_object, 0, sizeof(temperature_object));
	memset(temperature_instance, 0, sizeof(temperature_instance));

	temperature_instance[0].id = 0;

	temperature_object.instanceList = lwm2m_list_add(&temperature_instance[0], NULL);

	temperature_object.objID = LWM2M_TEMPERATURE_OBJECT_ID;

	temperature_object.versionMajor = 1;
	temperature_object.versionMinor = 1;

	temperature_object.createFunc = NULL;
	temperature_object.deleteFunc = NULL;
	temperature_object.discoverFunc = NULL;
	temperature_object.executeFunc = NULL;
	temperature_object.readFunc = _read;
	temperature_object.writeFunc = NULL;

	return (void*)&temperature_object;
}
