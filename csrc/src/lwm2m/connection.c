/*******************************************************************************
 *
 * Copyright (c) 2013, 2014 Intel Corporation and others.
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
 *    Pascal Rieux - Please refer to git log
 *
 *******************************************************************************/

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "connection.h"
#include "commandline.h"
#include "mbedtls/net_sockets.h"
#include <sys/types.h>

#define SIMPLE_LINK_MAX_SEND_MTU 	1472

extern mbedtls_ssl_context ssl_context;
//extern miso_mbedtls_context_t net_context;

connection_t connection_create(connection_t connList, char *host, char *port,
		int protocol)
{
	int ret = UISO_NETWORK_GENERIC_ERROR;
	connection_t newConn = lwm2m_malloc(sizeof(struct connection_s));
	if(NULL != newConn)
	{
		ret = miso_create_network_connection(miso_get_network_ctx(wifi_service_lwm2m_socket), host, strlen(host), atoi(port), 0, (enum miso_protocol)protocol);
		if(ret >= 0)
		{
			/* Prepare */
			newConn->ctx = miso_get_network_ctx(wifi_service_lwm2m_socket);
			newConn->next = NULL;

			if(NULL != connList)
			{
				connection_t connList_traversal = connList;
				while(connList_traversal->next != NULL)
				{
					connList_traversal = connList_traversal->next;
				}
				connList_traversal->next = newConn;
			}
		}
		else
		{
			lwm2m_free(newConn);
			newConn = NULL;
		}
	}

	return newConn;
}

void connection_free(connection_t connList)
{
	int ret = UISO_NETWORK_OK;
	if(connList != NULL)
	{
		do{
			// connection free
			ret = miso_close_network_connection(connList->ctx);
			if(ret != UISO_NETWORK_OK)
			{
				printf("Error: %d\n\r");
			}

			if(NULL != connList->next)
			{
				connList = connList->next;
			}
		}
		while(NULL != connList->next);
	}
}

int connection_send(connection_t connP, uint8_t *buffer, size_t length)
{
	(void)connP;

	int nbSent = miso_network_send(miso_get_network_ctx(wifi_service_lwm2m_socket), buffer, length);
	if(0 > nbSent)
	{
		return -1;
	}
	return nbSent;
}


uint8_t lwm2m_buffer_send(void *sessionH, uint8_t *buffer, size_t length,
		void *userdata)
{
	connection_t connP = (connection_t) sessionH;

	(void) userdata; /* unused */

	if (connP == NULL)
	{
		return COAP_500_INTERNAL_SERVER_ERROR ;
	}

	if (-1 == connection_send(connP, buffer, length))
	{
		return COAP_500_INTERNAL_SERVER_ERROR ;
	}

	return COAP_NO_ERROR ;
}

bool lwm2m_session_is_equal(void *session1, void *session2, void *userData)
{
	(void) userData; /* unused */

	return true; //(session1 == session2);
}
