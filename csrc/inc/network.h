/*
 * network.h
 *
 *  Created on: 10 abr 2023
 *      Author: Francisco
 */

#ifndef NETWORK_H_
#define NETWORK_H_

#include "miso.h"


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

/**
 * Protocol Selection Bit Masks
 * 
 * Do not use directly
 */
enum
{
	miso_protocol_udp = (UISO_UDP_SELECTION_BIT_MASK | UISO_PROTOCOL_BIT_MASK),
	miso_protocol_tcp = (UISO_TCP_SELECTION_BIT_MASK | UISO_PROTOCOL_BIT_MASK),

	miso_protocol_ip4 = (0 << UISO_IPV4_IPV6_SELECTION_BIT)| UISO_PROTOCOL_BIT_MASK,
	miso_protocol_ip6 = (1 << UISO_IPV4_IPV6_SELECTION_BIT)| UISO_PROTOCOL_BIT_MASK,
};

/**
 * MISO Protocol selction
*/
enum miso_protocol
{
	miso_protocol_no_protocol = 0,

	miso_protocol_udp_ip4 = (miso_protocol_udp | miso_protocol_ip4),
	miso_protocol_tcp_ip4 = (miso_protocol_tcp | miso_protocol_ip4),

	/* IPv6 - NOT SUPPORTED */
	miso_protocol_udp_ip6 = (miso_protocol_udp | miso_protocol_ip6),
	miso_protocol_tcp_ip6 = (miso_protocol_tcp | miso_protocol_ip6),

	/* IPv4 TLS */
	miso_protocol_dtls_ip4 = (miso_protocol_udp_ip4 | UISO_SECURITY_BIT_MASK) & UISO_PROTOCOL_MASK,
	miso_protocol_tls_ip4 =  (miso_protocol_tcp_ip4 | UISO_SECURITY_BIT_MASK) & UISO_PROTOCOL_MASK,

	/* IPv6 TLS - NOT SUPPORTED */
	miso_protocol_dtls_ip6 = (miso_protocol_udp_ip6 | UISO_SECURITY_BIT_MASK) & UISO_PROTOCOL_MASK,
	miso_protocol_tls_ip6 =  (miso_protocol_tcp_ip6 | UISO_SECURITY_BIT_MASK) & UISO_PROTOCOL_MASK,

	miso_protocol_max = UISO_PROTOCOL_MASK, // Do not use
};

/**
 * miso security mode
 */
enum miso_security_mode{
	miso_security_mode_none = 0,
	miso_security_mode_psk = 1,
	miso_security_mode_ec = 2,
	miso_security_mode_rsa = 3,
};


/**
 * Socket ID
 * 
 * Currently only 4 sockets are supported in the pool.
 * 
 * Each socket is identified by an ID and associated with a service.
 * 
 */
enum wifi_socket_id_e
{
	wifi_service_ntp_socket = 0,
	wifi_service_lwm2m_socket = 1,
	wifi_service_mqtt_socket = 2,
	wifi_service_http_socket = 3,
	wifi_service_max
};

/**
 * Network Context
 * 
 * Forward declaration of the network context.
 */
typedef struct miso_sockets_s * miso_network_ctx_t;

enum miso_network_ret
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
miso_network_ctx_t miso_get_network_ctx(enum wifi_socket_id_e id);

/**
 * Create Connection and Handshake with peer
 * @param ctx
 * @param host
 * @param port
 * @param proto
 * @return
 */
int miso_create_network_connection(miso_network_ctx_t ctx, const char *host, size_t host_len, uint16_t port, uint16_t local_port,
								   enum miso_protocol proto);

/**
 * Close Connection
 * 
 */
int miso_close_network_connection(miso_network_ctx_t ctx);

/**
 * Register SSL context with network context
 * @param ctx
 * @param ssl_ctx
 * @return
 */
int miso_network_register_ssl_context(miso_network_ctx_t ctx, mbedtls_ssl_context * ssl_ctx);

/** 
 * Read from socket
*/
int miso_network_read(miso_network_ctx_t ctx, unsigned char *buffer, size_t length);

/**
 * Write (send) to socket
*/
int miso_network_send(miso_network_ctx_t ctx, const unsigned char *buffer, size_t length);

int miso_network_get_socket(miso_network_ctx_t ctx);

int wait_rx(miso_network_ctx_t ctx, uint32_t timeout_s);
int wait_tx(miso_network_ctx_t ctx, uint32_t timeout_s);

mbedtls_ssl_context * miso_network_get_ssl_ctx(miso_network_ctx_t ctx);
#endif /* NETWORK_H_ */
