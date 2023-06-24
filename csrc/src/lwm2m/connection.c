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
#include "uiso_net_sockets.h"
#include "mbedtls/net_sockets.h"
#include <sys/types.h>

#define SIMPLE_LINK_MAX_SEND_MTU 	1472

extern mbedtls_ssl_context ssl_context;
extern uiso_mbedtls_context_t net_context;

#if 0
connection_t connection_find(connection_t connList, struct sockaddr_in *addr,
		size_t addrLen)
{
	connection_t connP;

	connP = connList;
	while (connP != NULL)
	{
		/* Modified this for IPv4 */
		if (uiso_net_compare_addresses_ipv4((SlSockAddrIn_t*) addr,
				&(connP->host_addr)))
		{
			return connP;
		}

		connP = connP->next;
	}

	return connP;
}

connection_t connection_new_incoming(connection_t connList,
		struct uiso_mbedtls_context_s *connection)
{
	connection_t connP;

	connP = (connection_t) lwm2m_malloc(sizeof(connection_t));
	if (connP != NULL)
	{
		/* Prepend to list */
		memmove(connP, connection, sizeof(struct uiso_mbedtls_context_s));
		connP->next = connList;
	}

	return connP;
}
#endif

connection_t connection_create(connection_t connList, char *host, char *port,
		int protocol)
{
	int ret = UISO_NET_GENERIC_ERROR;
	connection_t newConn = lwm2m_malloc(sizeof(struct connection_s));
	if(NULL != newConn)
	{
		ret = uiso_create_network_connection(uiso_get_network_ctx(wifi_service_lwm2m_socket), host, port, NULL, (enum uiso_protocol)protocol);
		if(ret >= 0)
		{
			/* Prepare */
			newConn->ctx = uiso_get_network_ctx(wifi_service_lwm2m_socket);
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
			ret = uiso_close_network_connection(connList->ctx);
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

	int nbSent = uiso_network_send(uiso_get_network_ctx(wifi_service_lwm2m_socket), buffer, length);
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
