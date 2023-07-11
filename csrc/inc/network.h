/*
 * network.h
 *
 *  Created on: 10 abr 2023
 *      Author: Francisco
 */

#ifndef NETWORK_H_
#define NETWORK_H_

#include "uiso.h"


#include "mbedtls/ssl.h"
/* Include Simplelink */
#include "simplelink.h"
#include "netapp.h"
#include "socket.h"

#define UISO_PROTOCOL_BIT		             (0)
#define UISO_PROTOCOL_BIT_MASK			     (1 << UISO_PROTOCOL_BIT)
#define UISO_UDP_SELECTION_BIT				 (1)
#define UISO_UDP_SELECTION_BIT_MASK			 (1 << UISO_UDP_SELECTION_BIT)
#define UISO_TCP_SELECTION_BIT	         	 (2)
#define UISO_TCP_SELECTION_BIT_MASK          (1 << UISO_TCP_SELECTION_BIT)
#define UISO_IPV4_IPV6_SELECTION_BIT         (3)
#define UISO_IPV4_IPV6_SELECTION_BIT_MASK    (1 << UISO_IPV4_IPV6_SELECTION_BIT)
#define UISO_SECURITY_BIT				     (4)
#define UISO_SECURITY_BIT_MASK			     (1 << UISO_SECURITY_BIT)

#define UISO_PROTOCOL_MASK    (UISO_PROTOCOL_BIT_MASK | UISO_UDP_SELECTION_BIT_MASK | UISO_TCP_SELECTION_BIT_MASK | UISO_IPV4_IPV6_SELECTION_BIT_MASK | UISO_SECURITY_BIT_MASK)

enum
{
	uiso_protocol_udp = (UISO_UDP_SELECTION_BIT_MASK | UISO_PROTOCOL_BIT_MASK),
	uiso_protocol_tcp = (UISO_TCP_SELECTION_BIT_MASK | UISO_PROTOCOL_BIT_MASK),

	uiso_protocol_ip4 = (0 << UISO_IPV4_IPV6_SELECTION_BIT)| UISO_PROTOCOL_BIT_MASK,
	uiso_protocol_ip6 = (1 << UISO_IPV4_IPV6_SELECTION_BIT)| UISO_PROTOCOL_BIT_MASK,
};

enum uiso_protocol
{
	uiso_protocol_no_protocol = 0,

	uiso_protocol_udp_ip4 = (uiso_protocol_udp | uiso_protocol_ip4),
	uiso_protocol_tcp_ip4 = (uiso_protocol_tcp | uiso_protocol_ip4),

	/* IPv6 - NOT SUPPORTED */
	uiso_protocol_udp_ip6 = (uiso_protocol_udp | uiso_protocol_ip6),
	uiso_protocol_tcp_ip6 = (uiso_protocol_tcp | uiso_protocol_ip6),

	/* IPv4 TLS */
	uiso_protocol_dtls_ip4 = (uiso_protocol_udp_ip4 | UISO_SECURITY_BIT_MASK) & UISO_PROTOCOL_MASK,
	uiso_protocol_tls_ip4 =  (uiso_protocol_tcp_ip4 | UISO_SECURITY_BIT_MASK) & UISO_PROTOCOL_MASK,

	/* IPv6 TLS - NOT SUPPORTED */
	uiso_protocol_dtls_ip6 = (uiso_protocol_udp_ip6 | UISO_SECURITY_BIT_MASK) & UISO_PROTOCOL_MASK,
	uiso_protocol_tls_ip6 =  (uiso_protocol_tcp_ip6 | UISO_SECURITY_BIT_MASK) & UISO_PROTOCOL_MASK,

	uiso_protocol_max = UISO_PROTOCOL_MASK,
};

enum wifi_socket_id_e
{
	wifi_service_ntp_socket = 0,
	wifi_service_lwm2m_socket = 1,
	wifi_service_mqtt_socket = 2,
	wifi_service_http_socket = 3,
	wifi_service_max
};

typedef struct uiso_sockets_s * uiso_network_ctx_t;

// socket id?

enum uiso_network_ret
{
	UISO_NETWORK_OK = 0,
	UISO_NETWORK_GENERIC_ERROR = INT32_MIN,
	UISO_NETWORK_SOCKET_ERROR ,
	UISO_NETWORK_BIND_ERROR,
	UISO_NETWORK_CONNECT_ERROR,
	UISO_NETWORK_DNS_ERROR,

	UISO_NETWORK_NULL_CTX,
	UISO_NETWORK_NEGATIVE_SD,
	UISO_NETWORK_UNKNOWN_PROTOCOL,

	UISO_NETWORK_RECV_ERROR,
	UISO_NETWORK_SEND_ERROR,

	UISO_NETWORK_DTLS_RENEGOCIATION_FAIL,
	UISO_NETWORK_UDP_SEND_ERROR,
};

#define UISO_NETWORK_INVALID_SOCKET		((int32_t)-1)

/**
 * Get Network Context for Service ID
 * @param id
 * @return
 */
uiso_network_ctx_t uiso_get_network_ctx(enum wifi_socket_id_e id);

/**
 * Create Connection and Handshake with peer
 * @param ctx
 * @param host
 * @param port
 * @param proto
 * @return
 */
int uiso_create_network_connection(uiso_network_ctx_t ctx, const char *host,
		const char *port, const char * local_port, enum uiso_protocol proto);

int uiso_close_network_connection(uiso_network_ctx_t ctx);

/**
 * Register SSL context with network context
 * @param ctx
 * @param ssl_ctx
 * @return
 */
int uiso_network_register_ssl_context(uiso_network_ctx_t ctx, mbedtls_ssl_context * ssl_ctx);

int uiso_network_read(uiso_network_ctx_t ctx, uint8_t *buffer, size_t length);
int uiso_network_send(uiso_network_ctx_t ctx, const uint8_t *buffer, size_t length);

int wait_rx(uiso_network_ctx_t ctx, uint32_t timeout_s);
int wait_tx(uiso_network_ctx_t ctx, uint32_t timeout_s);

mbedtls_ssl_context * uiso_network_get_ssl_ctx(uiso_network_ctx_t ctx);
#endif /* NETWORK_H_ */
