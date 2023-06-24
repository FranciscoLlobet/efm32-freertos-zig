/*
 * uiso_net_sockets.h
 *
 *  Created on: 18 nov 2022
 *      Author: Francisco
 */

#ifndef UISO_NET_SOCKETS_H_
#define UISO_NET_SOCKETS_H_

#include <time.h>
#include "network.h"

//#include "liblwm2m.h"

#include "mbedtls/ctr_drbg.h"
#include "mbedtls/timing.h"

#include "mbedtls/aes.h"
#include "mbedtls/base64.h"
#include "mbedtls/net_sockets.h"
#include "mbedtls/entropy.h"





enum
{
	UISO_NET_OK = 0,
	UISO_NET_GENERIC_ERROR = -1,
	UISO_NET_SOCKET_ERROR = -2,
	UISO_NET_CONNECT_ERROR = -3,
	UISO_NET_DNS_ERROR = -5,
};



typedef struct uiso_mbedtls_context_s
{
    struct uiso_mbedtls_context_s * next;
    int fd; /* Socket Descriptor */

    int32_t protocol; /* Protocol to use */
    SlSockAddrIn_t host_addr; /* Host Address */
    size_t host_addr_len;

    mbedtls_ssl_context * ssl_context;  /* Optional ssl context */
 //   lwm2m_context_t * lwm2m_context; /* Optional LWM2M Application context */

    void * app_context; /* Optional app context */
    void * app_param_p; /* Pointer App parameter */
    uint32_t app_param_u; /* App param */
    uint32_t bogo_param;

    time_t last_send_time;
    time_t last_read_time;
}
uiso_mbedtls_context_t;


/**
 * Compare two IPv4 addresses
 * @param address_1
 * @param address_2
 * @return
 */
bool uiso_net_compare_addresses_ipv4(SlSockAddrIn_t * address_1, SlSockAddrIn_t * address_2);
/* Support and utility */


/* Initialize the net context */
void uiso_mbedtls_net_init( uiso_mbedtls_context_t *ctx );


/**
 * Connects to a socket.
 */
int uiso_mbedtls_net_connect(uiso_mbedtls_context_t *ctx, const char *host, const char *port, int proto );

int uiso_mbedtls_ssl_udp_send( void *ctx, const unsigned char *buf, size_t len );
int uiso_mbedtls_ssl_recv( void *ctx, unsigned char *buf, size_t len );
int mbedtls_ssl_recv_timeout( void *ctx, unsigned char *buf, size_t len, uint32_t timeout );




#define UISO_MBED_TLS_SSL_SET_DTLS_BIO(ssl, p_bio)  mbedtls_ssl_set_bio(ssl, (void *)p_bio, uiso_mbedtls_ssl_udp_send, uiso_mbedtls_ssl_recv, mbedtls_ssl_recv_timeout)

#endif /* LWM2M_UISO_NET_SOCKETS_H_ */
