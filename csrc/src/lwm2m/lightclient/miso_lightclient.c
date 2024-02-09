/*******************************************************************************
 *
 * Copyright (c) 2013, 2014, 2015 Intel Corporation and others.
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v2.0
 * and Eclipse Distribution License v1.0 which accompany this distribution.
 *
 * The Eclipse Public License is available at
 *    http://www.eclipse.org/legal/epl-v20.html
 * The Eclipse Distribution License is available at
 *    http://www.eclipse.org/org/documents/edl-v10.php.
 *
 * Contributors:
 *    David Navarro, Intel Corporation - initial API and implementation
 *    Benjamin Cabé - Please refer to git log
 *    Fabien Fleutot - Please refer to git log
 *    Simon Bernard - Please refer to git log
 *    Julien Vermillard - Please refer to git log
 *    Axel Lorente - Please refer to git log
 *    Toby Jaffey - Please refer to git log
 *    Bosch Software Innovations GmbH - Please refer to git log
 *    Pascal Rieux - Please refer to git log
 *    Christian Renz - Please refer to git log
 *
 *******************************************************************************/

/*
 Copyright (c) 2013, 2014 Intel Corporation

 Redistribution and use in source and binary forms, with or without modification,
 are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
 this list of conditions and the following disclaimer.
 * Redistributions in binary form must reproduce the above copyright notice,
 this list of conditions and the following disclaimer in the documentation
 and/or other materials provided with the distribution.
 * Neither the name of Intel Corporation nor the names of its contributors
 may be used to endorse or promote products derived from this software
 without specific prior written permission.

 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 THE POSSIBILITY OF SUCH DAMAGE.

 David Navarro <david.navarro@intel.com>
 Bosch Software Innovations GmbH - Please refer to git log

 */
#include "lwm2m_client.h"

#include "liblwm2m.h"
#include "../connection.h"

//#include "simplelink.h"
#include <ctype.h>
#include <errno.h>
#include <inttypes.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#include "wifi_service.h"
#include "lwm2m_temperature.h"

/* Extern declarations */
extern lwm2m_object_t* get_object_device(void);
extern void free_object_device(lwm2m_object_t *objectP);
extern lwm2m_object_t* get_server_object(void);
extern void free_server_object(lwm2m_object_t *object);
extern lwm2m_object_t* get_security_object(void);
extern void free_security_object(lwm2m_object_t *objectP);
extern char* get_server_uri(lwm2m_object_t *objectP, uint16_t secObjInstID);
extern lwm2m_object_t* get_temperature_object(void);
extern void free_test_object(lwm2m_object_t *object);
extern void free_accelerometer_object(lwm2m_object_t *accelerometer);
extern void* get_accelerometer_object(void);

extern char * config_get_lwm2m_endpoint(void);

#define MAX_PACKET_SIZE			2048
#define LWM2M_STEP_TIMEOUT		pdMS_TO_TICKS(10000)

/* RX buffer */
static uint8_t buffer[MAX_PACKET_SIZE];

static uint8_t* get_rx_buffer(void)
{
	return &buffer[0];
}

int g_reboot = 0;

enum dtls_authentication_mode
{
	DTLS_AUTHENTICATION_MODE_PK = 0, DTLS_AUTHENTICATION_MODE_PSK = 1
};

typedef struct
{
	lwm2m_object_t *securityObjP;
	connection_t connList;
	void * param;
} client_data_t;

extern int mbedtls_connector_initialize(lwm2m_object_t *securityObjP,
		uint16_t secObjInstID);
extern void mbedtls_cleanup(void);
extern void mbedtls_connector_close(void);

int wait_for_rx(uint32_t wait_s);



void* lwm2m_connect_server(uint16_t secObjInstID, void *userData)
{
	client_data_t *dataP = (client_data_t*) userData;
	connection_t newConnP = NULL;
	volatile int ret = -1;

	char * uri = get_server_uri(dataP->securityObjP, secObjInstID);
	if (uri == NULL)
		return NULL;

	fprintf(stdout, "Connecting to %s\r\n", uri);

	ret = lwm2mservice_create_connection(dataP->param, (uint8_t *) get_server_uri(dataP->securityObjP, secObjInstID), 0, dataP->securityObjP, secObjInstID); // secObjP, secObjInstID);
	if (0 == ret)
	{
		newConnP = connection_create(dataP->connList, dataP->param);
	}

	if (newConnP == NULL)
	{
		fprintf(stderr, "Connection creation failed.\r\n");
	}
	else if(NULL == dataP->connList)
	{
		dataP->connList = newConnP;
	}

	exit: lwm2m_free(uri);
	return (void*) newConnP;
}

void lwm2m_close_connection(void *sessionH, void *userData)
{
	client_data_t *app_data;
	connection_t targetP;

	app_data = (client_data_t*) userData;
	targetP = (connection_t) sessionH;

	// Add stuff to close the connection

	if (targetP == app_data->connList)
	{
		app_data->connList = targetP->next;
		lwm2m_free(targetP);
	}
	else
	{
		connection_t parentP;

		parentP = app_data->connList;
		while (parentP != NULL && parentP->next != targetP)
		{
			parentP = parentP->next;
		}
		if (parentP != NULL)
		{
			parentP->next = targetP->next;
			lwm2m_free(targetP);
		}
	}
}

void print_state(lwm2m_context_t *lwm2mH)
{
	lwm2m_server_t *targetP;

	fprintf(stderr, "State: ");
	switch (lwm2mH->state)
	{
	case STATE_INITIAL:
		fprintf(stderr, "STATE_INITIAL");
		break;
	case STATE_BOOTSTRAP_REQUIRED:
		fprintf(stderr, "STATE_BOOTSTRAP_REQUIRED");
		break;
	case STATE_BOOTSTRAPPING:
		fprintf(stderr, "STATE_BOOTSTRAPPING");
		break;
	case STATE_REGISTER_REQUIRED:
		fprintf(stderr, "STATE_REGISTER_REQUIRED");
		break;
	case STATE_REGISTERING:
		fprintf(stderr, "STATE_REGISTERING");
		break;
	case STATE_READY:
		fprintf(stderr, "STATE_READY");
		break;
	default:
		fprintf(stderr, "Unknown !");
		break;
	}
	fprintf(stderr, "\r\n");

	targetP = lwm2mH->bootstrapServerList;

	if (lwm2mH->bootstrapServerList == NULL)
	{
		fprintf(stderr, "No Bootstrap Server.\r\n");
	}
	else
	{
		fprintf(stderr, "Bootstrap Servers:\r\n");
		for (targetP = lwm2mH->bootstrapServerList; targetP != NULL; targetP =
				targetP->next)
		{
			fprintf(stderr, " - Security Object ID %d", targetP->secObjInstID);
			fprintf(stderr, "\tHold Off Time: %lu s",
					(unsigned long) targetP->lifetime);
			fprintf(stderr, "\tstatus: ");
			switch (targetP->status)
			{
			case STATE_DEREGISTERED:
				fprintf(stderr, "DEREGISTERED\r\n");
				break;
			case STATE_BS_HOLD_OFF:
				fprintf(stderr, "CLIENT HOLD OFF\r\n");
				break;
			case STATE_BS_INITIATED:
				fprintf(stderr, "BOOTSTRAP INITIATED\r\n");
				break;
			case STATE_BS_PENDING:
				fprintf(stderr, "BOOTSTRAP PENDING\r\n");
				break;
			case STATE_BS_FINISHED:
				fprintf(stderr, "BOOTSTRAP FINISHED\r\n");
				break;
			case STATE_BS_FAILED:
				fprintf(stderr, "BOOTSTRAP FAILED\r\n");
				break;
			default:
				fprintf(stderr, "INVALID (%d)\r\n", (int) targetP->status);
			}
			fprintf(stderr, "\r\n");
		}
	}

	if (lwm2mH->serverList == NULL)
	{
		fprintf(stderr, "No LWM2M Server.\r\n");
	}
	else
	{
		fprintf(stderr, "LWM2M Servers:\r\n");
		for (targetP = lwm2mH->serverList; targetP != NULL;
				targetP = targetP->next)
		{
			fprintf(stderr, " - Server ID %d", targetP->shortID);
			fprintf(stderr, "\tstatus: ");
			switch (targetP->status)
			{
			case STATE_DEREGISTERED:
				fprintf(stderr, "DEREGISTERED\r\n");
				break;
			case STATE_REG_PENDING:
				fprintf(stderr, "REGISTRATION PENDING\r\n");
				break;
			case STATE_REGISTERED:
				fprintf(stderr,
						"REGISTERED\tlocation: \"%s\"\tLifetime: %lus\r\n",
						targetP->location, (unsigned long) targetP->lifetime);
				break;
			case STATE_REG_UPDATE_PENDING:
				fprintf(stderr, "REGISTRATION UPDATE PENDING\r\n");
				break;
			case STATE_REG_UPDATE_NEEDED:
				fprintf(stderr, "REGISTRATION UPDATE REQUIRED\r\n");
				break;
			case STATE_DEREG_PENDING:
				fprintf(stderr, "DEREGISTRATION PENDING\r\n");
				break;
			case STATE_REG_FAILED:
				fprintf(stderr, "REGISTRATION FAILED\r\n");
				break;
			default:
				fprintf(stderr, "INVALID (%d)\r\n", (int) targetP->status);
			}
			fprintf(stderr, "\r\n");
		}
	}
}

#define OBJ_COUNT 5

#include "FreeRTOS.h"
#include "task.h"

client_data_t data;

int lwm2m_client_task_runner(void *param1)
{


	lwm2m_context_t *lwm2mH = NULL;
	lwm2m_object_t *objArray[OBJ_COUNT];

	int suspend = 0;
	int result;
	int opt;

	/* Reset and clear the recieve buffer */
	memset(buffer, 0, sizeof(buffer));

	/* Reset the client_data_object */
	memset(&data, 0, sizeof(client_data_t));

	data.param = param1;
	data.connList = (connection_t) NULL;

	/*
	 * Now the main function fill an array with each object, this list will be later passed to liblwm2m.
	 * Those functions are located in their respective object file.
	 */
	objArray[0] = get_security_object();
	if (NULL == objArray[0])
	{
		fprintf(stderr, "Failed to create security object\r\n");
		return -1;
	}
	data.securityObjP = objArray[0];

	objArray[1] = get_server_object();
	if (NULL == objArray[1])
	{
		fprintf(stderr, "Failed to create server object\r\n");
		return -1;
	}

	objArray[2] = get_object_device();
	if (NULL == objArray[2])
	{
		fprintf(stderr, "Failed to create Device object\r\n");
		return -1;
	}

	objArray[3] = get_temperature_object();
	if (NULL == objArray[3])
	{
		fprintf(stderr, "Failed to create Test object\r\n");
		return -1;
	}

	objArray[4] = get_accelerometer_object();

	/*
	 * The liblwm2m library is now initialized with the functions that will be in
	 * charge of communication
	 */
	lwm2mH = lwm2m_init(&data);
	if (NULL == lwm2mH)
	{
		fprintf(stderr, "lwm2m_init() failed\r\n");
		return -1;
	}

	/*
	 * We configure the liblwm2m library with the name of the client - which shall be unique for each client -
	 * the number of objects we will be passing through and the objects array
	 */
	result = lwm2m_configure(lwm2mH, config_get_lwm2m_endpoint(), NULL, NULL, OBJ_COUNT, objArray);
	if (result != 0)
	{
		fprintf(stderr, "lwm2m_configure() failed: 0x%X\r\n", result);
		return -1;
	}

	uint32_t lwm2m_timeout = 0;

	while (1)
	{
		uint32_t notification_value = 0;

		if (pdTRUE == xTaskNotifyWait(0, UINT32_MAX, &notification_value, 0))
		{
			if (notification_value & (uint32_t) lwm2m_notify_registration)
			{
				// Registration Timer
				lwm2m_update_registration(lwm2mH, 0, false);
			}
			if (notification_value & (uint32_t) lwm2m_notify_timestamp)
			{
				/* Update timestamp */
				lwm2m_uri_t uri =
				{ .objectId = LWM2M_DEVICE_OBJECT_ID, .instanceId = 0,
						.resourceId = 13 };
				lwm2m_resource_value_changed(lwm2mH, &uri);
			}
			if (notification_value & (uint32_t) lwm2m_notify_temperature)
			{
				/* Update temperature object */
				lwm2m_uri_t uri =
				{ .objectId = LWM2M_OBJECT_TEMPERATURE, .instanceId = 0,
						.resourceId = RESOURCE_ID_SENSOR_VALUE };
				lwm2m_resource_value_changed(lwm2mH, &uri);
			}
			if (notification_value & (uint32_t) lwm2m_notify_accelerometer)
			{
				/* Update accelerometer object */
				lwm2m_uri_t uri =
				{ .objectId = 3313, .instanceId = 0, .resourceId = 5702 };
				lwm2m_resource_value_changed(lwm2mH, &uri);

				uri.resourceId = 5703;
				lwm2m_resource_value_changed(lwm2mH, &uri);

				uri.resourceId = 5704;
				lwm2m_resource_value_changed(lwm2mH, &uri);
			}
			if (notification_value & (uint32_t) lwm2m_notify_message_reception)
			{
				/* Handle reception */
				int numBytes = lwm2mservice_read_data(data.param, &buffer[0], sizeof(buffer));
				if (numBytes != -1)
				{
					connection_t connP = data.connList;

					/* Let liblwm2m respond to the query depending on the context */
					lwm2m_handle_packet(lwm2mH, get_rx_buffer(),
							(size_t) numBytes, connP);
				}
			}
			if(notification_value & (uint32_t) lwm2m_notify_suspend)
			{
				suspend = 1;
				break;
			}
		}
		else
		{
			// No notifications pending
		}

		print_state(lwm2mH);

		/* Perform LWM2M step*/
		time_t timeout_val = 60;

		result = lwm2m_step(lwm2mH, &timeout_val);
		if (result != 0)
		{
			fprintf(stderr, "lwm2m_step() failed: 0x%X\r\n", result);
			// Go to error condition
			break;
		}
		else
		{
			if(timeout_val > 1)
			{
				timeout_val = 1;
			}

			wait_for_rx(timeout_val);
		}

		printf("LWM2M: %d\n\r", uxTaskGetStackHighWaterMark(NULL));
	} 

	/*
	 * Finally when the loop is left, we unregister our client from it
	 */
	lwm2m_close(lwm2mH);
	//close(data.sock);
	connection_free(data.connList);

	free_security_object(objArray[0]);
	free_server_object(objArray[1]);
	free_object_device(objArray[2]); /* Device object */
	free_accelerometer_object(objArray[4]); /* Accelerometer */

	fprintf(stdout, "\r\n\n");

	return (suspend == 1);
}

extern void update_accelerometer_values(float x, float y, float z);

void lwm2m_client_update_accel(float x, float y, float z)
{
	update_accelerometer_values(x, y, z);

//	xTaskNotify(user_task_handle, (uint32_t )lwm2m_notify_accelerometer,
//			eSetBits);
}

int wait_for_rx(uint32_t wait_s)
{
	int ret = enqueue_select_rx(wifi_service_lwm2m_socket, wait_s);
	if(0 == ret)
	{
		xTaskNotify(xTaskGetCurrentTaskHandle(), (uint32_t )lwm2m_notify_message_reception, eSetBits);
	}

	return ret;
}

