/*
 * object_device_capability.c
 *
 *  Created on: 10 dic 2022
 *      Author: Francisco
 */
#include "uiso.h"
#include "liblwm2m.h"

#define LWM2M_DEV_CAP_MGMT		(15)

enum
{
	dev_cap_mgmt_res_property = 0,
	dev_cap_mgmt_res_group = 1,
	dev_cap_mgmt_res_description = 2,
	dev_cap_mgmt_res_attached = 3,
	dev_cap_mgmt_res_enabled = 4,
	dev_cap_mgmt_res_op_enabled = 5,
	dev_cap_mgmt_res_op_disable = 6,
	dev_cap_mgmt_res_notify_en = 7
};

enum
{
	group_sensor = 0,
	group_control = 1,
	group_connectivity = 2,
	group_navigation = 3,
	group_storage = 4,
	group_vision = 5,
	group_sound = 6,
	group_analog_input = 7,
	group_analog_output = 8
};

static const uint16_t resource_list[] =
{ 	(uint16_t)dev_cap_mgmt_res_property,
	(uint16_t)dev_cap_mgmt_res_group,
	(uint16_t)dev_cap_mgmt_res_description,
	// (uint16_t)dev_cap_mgmt_res_attached,
	(uint16_t)dev_cap_mgmt_res_enabled,
	(uint16_t)dev_cap_mgmt_res_op_enabled,
	(uint16_t)dev_cap_mgmt_res_op_disable,
	// (uint16_t)dev_cap_mgmt_res_notify_en
};

static const int num_resource_list = sizeof(resource_list) / sizeof(resource_list[0]);

/* static allocation */
static lwm2m_object_t dev_cap_mgmt_object;

static lwm2m_list_t dev_cap_mgmt_instances[1];


