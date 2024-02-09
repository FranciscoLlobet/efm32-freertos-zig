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
 *
 *******************************************************************************/

#ifndef CONNECTION_H_
#define CONNECTION_H_

#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>
#include <liblwm2m.h>

#define LWM2M_STANDARD_PORT_STR "5683"
#define LWM2M_STANDARD_PORT      5683
#define LWM2M_DTLS_PORT_STR     "5684"
#define LWM2M_DTLS_PORT          5684
#define LWM2M_BSSERVER_PORT_STR "5685"
#define LWM2M_BSSERVER_PORT      5685

typedef struct connection_s *connection_t;

struct connection_s
{
		connection_t next;
		void * parent; // parent ptr
};

//connection_t connection_find(connection_t connList, struct sockaddr_in *addr, size_t addrLen);
//connection_t connection_new_incoming(connection_t connList,
//		struct miso_mbedtls_context_s *connection);

connection_t connection_create(connection_t connList, void * ctx);

void connection_free(connection_t connList);

extern int lwm2mservice_create_connection(void * conn, uint8_t * uri, uint16_t local_port, lwm2m_object_t * lwm2m_object, uint16_t sec_obj_inst_id);
extern void lwm2mservice_close_connection(void * conn);
extern int lwm2mservice_send_data(void * conn, uint8_t * buffer, size_t length);
extern int lwm2mservice_read_data(void * conn, uint8_t * buffer, size_t length);
extern int lwm2mservice_wait_data(void * conn, uint32_t timeout);
#endif
