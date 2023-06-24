/*
 * network.c
 *
 *  Created on: 8 abr 2023
 *      Author: Francisco
 */

#include "uiso.h"

#include "mbedtls/error.h"

#include "wifi_service.h"

//#include "simplelink.h"
#include "sl_sleeptimer.h"

#define NETWORK_MONITOR_TASK    (UBaseType_t)( uiso_task_runtime_services )

#define SIMPLELINK_MAX_SEND_MTU 	1472

typedef int (*_network_send_fn)(uiso_network_ctx_t ctx, unsigned char *buf, size_t len);
typedef int (*_network_read_fn)(uiso_network_ctx_t ctx, unsigned char *buf, size_t len);
typedef int (*_network_close_fn)(uiso_network_ctx_t ctx);

struct uiso_sockets_s
{
	int32_t sd; /* Socket Descriptor */
	int32_t protocol;

	_network_send_fn send_fn;
	_network_read_fn read_fn;
	_network_close_fn close_fn;

	/* Wait deadlines */
	uint32_t rx_wait_deadline_s;
	uint32_t tx_wait_deadline_s;

	/* RX-TX Wait */
	SemaphoreHandle_t rx_signal;
	SemaphoreHandle_t tx_signal;

	/* local address */
	SlSockAddrIn_t local;
	SlSocklen_t local_len;

	/* Peer address */
	SlSockAddrIn_t peer;
	SlSocklen_t peer_len;

	mbedtls_ssl_context *ssl_context;

	uint32_t last_send_time;
	uint32_t last_recv_time;

	void *app_ctx;
	uint32_t app_param;
};

static struct uiso_sockets_s system_sockets[wifi_service_max]; /* new system sockets */
static SemaphoreHandle_t network_monitor_mutex = NULL;

static SemaphoreHandle_t rx_tx_mutex = NULL;

static void select_task(void *param);
static void initialize_socket_management(void);
//static int register_deadline(struct socket_management_s * ctx, int sd, uint32_t timeout_s);
static void wifi_service_register_rx_socket(enum wifi_socket_id_e id, uint32_t timeout_s);
static void wifi_service_register_tx_socket(enum wifi_socket_id_e id, uint32_t timeout_s);

TaskHandle_t network_monitor_task_handle = NULL;

/* mbedTLS Support */
static int _network_send(uiso_network_ctx_t ctx, unsigned char *buf, size_t len);
static int _network_recv(uiso_network_ctx_t ctx, unsigned char *buf, size_t len);
static int _network_close(uiso_network_ctx_t ctx);

static int _send_udp(uiso_network_ctx_t ctx, unsigned char *buf, size_t len);
static int _send_tcp(uiso_network_ctx_t ctx, unsigned char *buf, size_t len);

static int _read_dtls(uiso_network_ctx_t ctx, uint8_t *buffer, size_t length);
static int _send_dtls(uiso_network_ctx_t ctx, uint8_t *buffer, size_t length);

/* mbedTLS BIO */
static int _send_bio_tcp(uiso_network_ctx_t ctx, unsigned char *buf, size_t len);
static int _send_bio_udp(uiso_network_ctx_t ctx, unsigned char *buf, size_t len);
static int _recv_bio_tcp(uiso_network_ctx_t ctx, unsigned char *buf, size_t len);
static int _recv_bio_udp(uiso_network_ctx_t ctx, unsigned char *buf, size_t len);

static int _set_mbedtls_bio(uiso_network_ctx_t ctx);

static inline uiso_network_ctx_t _get_network_ctx(enum wifi_socket_id_e id);
static int _initialize_network_ctx(uiso_network_ctx_t ctx);

static void initialize_socket_management(void)
{
	for (size_t i = 0; i < (size_t) wifi_service_max; i++)
	{
		_initialize_network_ctx(_get_network_ctx((enum wifi_socket_id_e) i));
	}
}

void wifi_service_register_rx_socket(enum wifi_socket_id_e id, uint32_t timeout_s)
{
	(void) xSemaphoreTake(network_monitor_mutex, portMAX_DELAY);

	_get_network_ctx(id)->rx_wait_deadline_s = (uint32_t) sl_sleeptimer_get_time() + timeout_s;

	(void) xSemaphoreGive(network_monitor_mutex);
}

void wifi_service_register_tx_socket(enum wifi_socket_id_e id, uint32_t timeout_s)
{
	(void) xSemaphoreTake(network_monitor_mutex, portMAX_DELAY);

	_get_network_ctx(id)->tx_wait_deadline_s = (uint32_t) sl_sleeptimer_get_time() + timeout_s;

	(void) xSemaphoreGive(network_monitor_mutex);
}

int create_network_mediator(void)
{
	int ret = 0;

	if (pdFALSE == xTaskCreate(select_task, "SelectTask", configMINIMAL_STACK_SIZE + 4096, NULL,
	NETWORK_MONITOR_TASK, &network_monitor_task_handle))
	{
		ret = -1;
	}

	if (0 == ret)
	{
		initialize_socket_management();
	}
	if (0 == ret)
	{
		network_monitor_mutex = xSemaphoreCreateMutex();
		if (NULL == network_monitor_mutex)	ret = -1;
	}

	if (0 == ret)
	{
		rx_tx_mutex = xSemaphoreCreateMutex();
		if(NULL == rx_tx_mutex) ret = -1;
	}

	return ret;
}

int enqueue_select_rx(enum wifi_socket_id_e id, uint32_t timeout_s)
{
	int ret_value = -1;
	wifi_service_register_rx_socket(id, timeout_s);

	(void) xSemaphoreTake(_get_network_ctx(id)->rx_signal, 0);
	(void) xTaskNotifyIndexed(network_monitor_task_handle, 0, 1, eIncrement);

	if (pdTRUE == xSemaphoreTake(_get_network_ctx(id)->rx_signal, 1000 * (timeout_s + 2)))
	{
		ret_value = 0;
	}

	return ret_value;
}

int enqueue_select_tx(enum wifi_socket_id_e id, uint32_t timeout_s)
{
	int ret_value = -1;
	wifi_service_register_tx_socket(id, timeout_s);

	(void) xSemaphoreTake(_get_network_ctx(id)->tx_signal, 0);
	(void) xTaskNotifyIndexed(network_monitor_task_handle, 0, 1, eIncrement);

	if (pdTRUE == xSemaphoreTake(_get_network_ctx(id)->tx_signal, 1000 * (timeout_s + 2)))
	{
		ret_value = 0;
	}

	return ret_value;
}

/* RX-TX-Monitor Task */
static void select_task(void *param)
{
	(void) param;

	// Add network mutex
	vTaskSuspend(NULL);

	fd_set read_fd_set;
	fd_set write_fd_set;
	uint32_t notification_counter = 0;

	do
	{
		// Reset the pointers
		fd_set *read_set_ptr = NULL;
		fd_set *write_fd_set_ptr = NULL;

		FD_ZERO(&read_fd_set);
		FD_ZERO(&write_fd_set);

		uint32_t timeout_min = MONITOR_MAX_RESPONSE_S;
		uint32_t current_time = sl_sleeptimer_get_time();

		// Start Cycle

		(void) xSemaphoreTake(network_monitor_mutex, portMAX_DELAY);

		for (size_t i = 0; i < (size_t) wifi_service_max; i++)
		{
			uint32_t current_timeout = 0;

			uiso_network_ctx_t ctx = &system_sockets[(size_t) i];

			if (ctx->sd >= 0)
			{
				if (ctx->rx_wait_deadline_s != 0)
				{
					if (ctx->rx_wait_deadline_s >= current_time)
					{
						current_timeout = (ctx->rx_wait_deadline_s - current_time);
						if (timeout_min > current_timeout)
						{
							timeout_min = current_timeout;
						}

						FD_SET((_i16) ctx->sd, &read_fd_set);
						read_set_ptr = &read_fd_set;
					} else if ((ctx->rx_wait_deadline_s + 1) == current_time)
					{
						timeout_min = 0;
						FD_SET((_i16) ctx->sd, &read_fd_set);
						read_set_ptr = &read_fd_set;
					} else
					{
						ctx->rx_wait_deadline_s = 0; /* Deadline expired */
					}
				} // rx deadlines
				if (ctx->tx_wait_deadline_s != 0)
				{
					if (ctx->tx_wait_deadline_s >= current_time)
					{
						current_timeout = (ctx->tx_wait_deadline_s - current_time);
						if (timeout_min > current_timeout)
						{
							timeout_min = current_timeout;
						}

						FD_SET((_i16) ctx->sd, &write_fd_set);
						write_fd_set_ptr = &write_fd_set;
					} else if ((ctx->tx_wait_deadline_s + 1) == current_time)
					{
						timeout_min = 0;
						FD_SET((_i16) ctx->sd, &write_fd_set);
						write_fd_set_ptr = &write_fd_set;
					} else
					{
						ctx->tx_wait_deadline_s = 0; /* Deadline expired */
					}
				}

			} // tx deadlines
		} // End for
		(void) xSemaphoreGive(network_monitor_mutex);


		if ((read_set_ptr != NULL) || (write_fd_set_ptr != NULL))
		{
			(void) xSemaphoreTake(rx_tx_mutex, portMAX_DELAY);

			// Start second cycle
			struct timeval tv =
			{ .tv_sec = timeout_min, .tv_usec = 0 };

			int result = sl_Select(FD_SETSIZE, read_set_ptr, write_fd_set_ptr, NULL, &tv);

			if (result > 0)
			{
				(void) xSemaphoreTake(network_monitor_mutex, portMAX_DELAY);
				if (NULL != read_set_ptr)
				{
					for (size_t i = 0; i < (size_t) wifi_service_max; i++)
					{
						uiso_network_ctx_t ctx = &system_sockets[(size_t) i];

						if (ctx->sd >= 0)
						{
							if (FD_ISSET(ctx->sd, read_set_ptr))
							{
								ctx->rx_wait_deadline_s = 0;
								(void) xSemaphoreGive(ctx->rx_signal);
							}
						}
					}
				}
				if (NULL != write_fd_set_ptr)
				{
					for (size_t i = 0; i < (size_t) wifi_service_max; i++)
					{
						uiso_network_ctx_t ctx = &system_sockets[(size_t) i];

						if (ctx->sd >= 0)
						{
							if (FD_ISSET(ctx->sd, write_fd_set_ptr))
							{
								ctx->tx_wait_deadline_s = 0;
								(void) xSemaphoreGive(ctx->tx_wait_deadline_s);
							}
						}
					}
				}
				(void) xSemaphoreGive(network_monitor_mutex);
			} else
			{
				//
			}
			(void) xSemaphoreGive(rx_tx_mutex);
			// End second cycle // Return mutex
		} else
		{
			// Return mutex
			(void) xTaskGenericNotifyWait(0, 0, UINT32_MAX, &notification_counter, portMAX_DELAY);
		}

	} while (1);
}

/* Basic network operations */
static int _network_connect(uiso_network_ctx_t ctx, const char *host, const char *port, const char * local_port,
		enum uiso_protocol proto)
{
	int ret = (int) UISO_NETWORK_GENERIC_ERROR;
	_i16 dns_status = -1;

	SlSockAddrIn_t *host_addr = &(ctx->peer);

	/* Only resolve for IPv4 */
	memset(host_addr, 0, sizeof(SlSockAddrIn_t));

	dns_status = sl_NetAppDnsGetHostByName((_i8*) host, strlen(host), &(host_addr->sin_addr.s_addr),
	SL_AF_INET);

	if (dns_status < 0)
	{
		return (int) UISO_NETWORK_DNS_ERROR;
	} else
	{
		ctx->peer.sin_family = SL_AF_INET;
		ctx->peer.sin_port = __REV16(atoi(port));
		ctx->peer.sin_addr.s_addr = __REV(host_addr->sin_addr.s_addr);
		ctx->peer_len = sizeof(struct SlSockAddrIn_t);
	}

	_i16 type = 0;
	_i16 protocol = 0;

	switch (proto)
	{
		case uiso_protocol_dtls_ip4:
			type = SL_SOCK_DGRAM;
			protocol = SL_IPPROTO_UDP;
			break;
		case uiso_protocol_tls_ip4:
			type = SL_SOCK_STREAM;
			protocol = SL_IPPROTO_TCP;
			break;
		case uiso_protocol_udp_ip4:
			type = SL_SOCK_DGRAM;
			protocol = SL_IPPROTO_UDP;
			break;
		case uiso_protocol_tcp_ip4:
			type = SL_SOCK_STREAM;
			protocol = SL_IPPROTO_TCP;
			break;
		default:
			type = 0;
			protocol = 0;
			break;
	}

	/* Open the socket */
	ctx->protocol = (int32_t) proto;
	ctx->sd = (int32_t) sl_Socket(SL_AF_INET, type, protocol);
	if (ctx->sd >= (int32_t) 0)
	{
		ret = (int) UISO_NETWORK_OK;
	} else
	{
		ret = (int) UISO_NETWORK_SOCKET_ERROR;
	}

	// Bind to local port
	if(ret == (int)UISO_NETWORK_OK)
	{
		if(NULL != local_port)
		{
			ctx->local.sin_family = SL_AF_INET;
			ctx->local.sin_port = __REV16(atoi(local_port));
			ctx->local.sin_addr.s_addr = (uint32_t)(0);
			ctx->local_len = sizeof(struct SlSockAddrIn_t);
			if(0 != sl_Bind(ctx->sd, (SlSockAddr_t *)&ctx->local, (_i16)ctx->local_len))
			{
				ret = (int) UISO_NETWORK_BIND_ERROR;
			}
		}
	}

	if (ret == (int) UISO_NETWORK_OK)
	{
	//	if((uint32_t)proto & UISO_UDP_SELECTION_BIT)
	//	{
			SlSockNonblocking_t enableOption =
			{ .NonblockingEnabled = 1 };
			(void) sl_SetSockOpt(ctx->sd, SL_SOL_SOCKET, SL_SO_NONBLOCKING, (_u8*) &enableOption,
					sizeof(enableOption));
	//	}
	}

	if (ret == (int) UISO_NETWORK_OK)
	{
		do{
			ret = sl_Connect(ctx->sd, (SlSockAddr_t*) &(ctx->peer), sizeof(ctx->peer));
			if(ret == SL_EALREADY)
			{
				continue;
			}
			else if (ret < 0)
			{
				ret = (int) UISO_NETWORK_CONNECT_ERROR;
				(void) sl_Close(ctx->sd);
							ctx->sd = UISO_NETWORK_INVALID_SOCKET;
			}
			else
			{
				ret = (int) UISO_NETWORK_OK;
			}

        }while(ret == SL_EALREADY);
	}

	return ret;
}

static int _send_bio_udp(uiso_network_ctx_t ctx, unsigned char *buf, size_t len)
{
	return (int) sl_SendTo((_i16) ctx->sd, (void*) buf, (_i16) len, (_i16) 0,
			(SlSockAddr_t*) &(ctx->peer), sizeof(SlSockAddrIn_t));
}

static int _send_bio_tcp(uiso_network_ctx_t ctx, unsigned char *buf, size_t len)
{
	return (int) sl_Send((_i16) ctx->sd, (void*) buf, (_i16) len, (_i16) 0);
}

static int _recv_bio_udp(uiso_network_ctx_t ctx, unsigned char *buf, size_t len)
{
	ctx->peer_len = sizeof(SlSockAddrIn_t);
	int ret = (int) sl_RecvFrom((_i16) ctx->sd, (void*) buf, (_i16) len, (_i16) 0,
			(SlSockAddr_t*) &(ctx->peer), (SlSocklen_t*) &(ctx->peer_len));
	if (ret == (int) SL_EAGAIN)
	{
		ret = (int) MBEDTLS_ERR_SSL_WANT_READ;
	}
	return ret;
}

static int _read_tcp(uiso_network_ctx_t ctx, unsigned char *buf, size_t len)
{
	int ret = 0;
	do
	{
		ret = (int) sl_Recv((_i16) ctx->sd, (char*) buf, (_i16) len, (_i16) 0);

	}while(ret == (int)SL_EAGAIN);
	return ret;
}

static int _recv_bio_tcp(uiso_network_ctx_t ctx, unsigned char *buf, size_t len)
{
	int ret = (int) sl_Recv((_i16) ctx->sd, (char*) buf, (_i16) len, (_i16) 0);
	if (ret == (int) SL_EAGAIN)
	{
		ret = (int) MBEDTLS_ERR_SSL_WANT_READ;
	}
	return ret;
}

static int _send_udp(uiso_network_ctx_t ctx, unsigned char *buf, size_t len)
{
	size_t offset = 0;
	int n_bytes_sent = 0;

	while (offset != len)
	{
		n_bytes_sent = _send_bio_udp(ctx, buf + offset, len - offset);
		if (0 >= n_bytes_sent)
		{
			return n_bytes_sent;
		}
		offset += n_bytes_sent;
	}

	return offset;
}

static int _send_tcp(uiso_network_ctx_t ctx, unsigned char *buf, size_t len)
{
	size_t offset = 0;
	int n_bytes_sent = 0;

	while (offset != len)
	{
		n_bytes_sent = _send_bio_tcp(ctx, buf + offset, len - offset);
		if (0 >= n_bytes_sent)
		{
			return n_bytes_sent;
		}
		offset += n_bytes_sent;
	}

	return offset;
}

static int _set_mbedtls_bio(uiso_network_ctx_t ctx)
{
	if (NULL == ctx)
	{
		return (int) UISO_NETWORK_NULL_CTX;
	} else if (NULL == ctx->ssl_context)
	{
		return (int) UISO_NETWORK_NULL_CTX;
	}

	if (ctx->protocol == (int32_t) uiso_protocol_dtls_ip4)
	{
		mbedtls_ssl_set_bio(ctx->ssl_context, (void*) ctx, (mbedtls_ssl_send_t*) _send_bio_udp,
				(mbedtls_ssl_recv_t*) _recv_bio_udp, (mbedtls_ssl_recv_timeout_t*) NULL);
	} else if (ctx->protocol == (int32_t) uiso_protocol_tls_ip4)
	{
		mbedtls_ssl_set_bio(ctx->ssl_context, (void*) ctx, (mbedtls_ssl_send_t*) _send_bio_tcp,
				(mbedtls_ssl_recv_t*) _recv_bio_tcp, (mbedtls_ssl_recv_timeout_t*) NULL);
	}

	mbedtls_ssl_set_mtu(ctx->ssl_context, SIMPLELINK_MAX_SEND_MTU);

	return UISO_NETWORK_OK;
}

/*
 * Close the connection
 */
static int _network_close(uiso_network_ctx_t ctx)
{
	if (NULL == ctx)
	{
		return UISO_NETWORK_NULL_CTX;
	} else if (0 > (ctx->sd))
	{
		return UISO_NETWORK_NEGATIVE_SD;
	}

	int ret = (int) sl_Close((_i16) ctx->sd);

	ctx->sd = UISO_NETWORK_INVALID_SOCKET;

	return ret;
}

int uiso_network_register_ssl_context(uiso_network_ctx_t ctx, mbedtls_ssl_context *ssl_ctx)
{
	if (NULL == ctx)
	{
		return UISO_NETWORK_NULL_CTX;
	} else if (NULL == ssl_ctx)
	{
		return UISO_NETWORK_NULL_CTX;
	}

	ctx->ssl_context = ssl_ctx;
	return UISO_NETWORK_OK;
}

int uiso_create_network_connection(uiso_network_ctx_t ctx, const char *host, const char *port, const char * local_port,
		enum uiso_protocol proto)
{
	int ret = (int) UISO_NETWORK_OK;

	(void) xSemaphoreTake(rx_tx_mutex, portMAX_DELAY);

	/* Prepare connection */
	if ((UISO_SECURITY_BIT_MASK & (int32_t) proto) == UISO_SECURITY_BIT_MASK)
	{
		ret = _set_mbedtls_bio(ctx);
	}

	switch (proto)
	{
		case uiso_protocol_udp_ip4:
			ctx->read_fn = _recv_bio_udp;
			ctx->send_fn = _send_udp;
			ctx->close_fn = _network_close;
			break;
		case uiso_protocol_tcp_ip4:
			ctx->read_fn = _read_tcp;
			ctx->send_fn = _send_tcp;
			ctx->close_fn = _network_close;
			break;
		case uiso_protocol_dtls_ip4:
			ctx->read_fn = _read_dtls;
			ctx->send_fn = _send_dtls;
			ctx->close_fn = NULL;
			break;
		case uiso_protocol_tls_ip4:
			ctx->read_fn = _read_dtls;
			ctx->send_fn = _send_dtls;
			ctx->close_fn = NULL;
			break;
		default:
			break;
	}

	if (ret >= 0)
	{
		ret = _network_connect(ctx, host, port, local_port, proto);
	}

	if (ret >= 0)
	{
		if ((UISO_SECURITY_BIT_MASK & (int32_t) proto) == UISO_SECURITY_BIT_MASK)
		{
			/* Perform mbedTLS handshake */
			do
			{
				ret = mbedtls_ssl_handshake(ctx->ssl_context);
			} while ((MBEDTLS_ERR_SSL_WANT_READ == ret) || (MBEDTLS_ERR_SSL_WANT_WRITE == ret));
		}
	}

	if (ret >= 0)
	{
		ctx->last_recv_time = sl_sleeptimer_get_time();
		ctx->last_send_time = ctx->last_recv_time;
	}

	(void) xSemaphoreGive(rx_tx_mutex);
	return ret;
}

int uiso_close_network_connection(uiso_network_ctx_t ctx)
{
	int ret = UISO_NETWORK_OK;

	(void) xSemaphoreTake(rx_tx_mutex, portMAX_DELAY);

	if (ctx->protocol & UISO_SECURITY_BIT_MASK)
	{
		do
		{
			ret = mbedtls_ssl_close_notify(ctx->ssl_context);
		} while ((MBEDTLS_ERR_SSL_WANT_READ == ret) || (MBEDTLS_ERR_SSL_WANT_WRITE == ret));
	}

	if (ret == UISO_NETWORK_OK)
	{
		ret = _network_close(ctx);
	} else
	{
		(void) _network_close(ctx);
	}

	(void) xSemaphoreGive(rx_tx_mutex);

	return ret;
}

static int _read_dtls(uiso_network_ctx_t ctx, uint8_t *buffer, size_t length)
{
	int numBytes = -1;
	int result = 0;

	do
	{
		numBytes = mbedtls_ssl_read(ctx->ssl_context, buffer, length);
	} while ((numBytes == MBEDTLS_ERR_SSL_WANT_WRITE)
			|| (numBytes == MBEDTLS_ERR_SSL_ASYNC_IN_PROGRESS)
			|| (numBytes == MBEDTLS_ERR_SSL_CRYPTO_IN_PROGRESS)
			|| (numBytes == MBEDTLS_ERR_SSL_CLIENT_RECONNECT));

	if (0 > numBytes)
	{
		/* Close DTLS session and perform handshake */
		do
		{
			result = mbedtls_ssl_close_notify(ctx->ssl_context);
		} while ((MBEDTLS_ERR_SSL_WANT_READ == result) || (MBEDTLS_ERR_SSL_WANT_WRITE == result));

		result = mbedtls_ssl_session_reset(ctx->ssl_context);

		if (0 == result)
		{
			do
			{
				result = mbedtls_ssl_handshake(ctx->ssl_context);
			} while ((MBEDTLS_ERR_SSL_WANT_READ == result) || (MBEDTLS_ERR_SSL_WANT_WRITE == result));
		}
	}

	return numBytes;
}


int uiso_network_read(uiso_network_ctx_t ctx, uint8_t *buffer, size_t length)
{
	(void) xSemaphoreTake(rx_tx_mutex, portMAX_DELAY);
	int ret = ctx->read_fn(ctx, buffer, length);
	(void) xSemaphoreGive(rx_tx_mutex);

	return ret;
}

int uiso_network_send(uiso_network_ctx_t ctx, uint8_t *buffer, size_t length)
{
	(void) xSemaphoreTake(rx_tx_mutex, portMAX_DELAY);
    int ret = ctx->send_fn(ctx, buffer, length);
    (void) xSemaphoreGive(rx_tx_mutex);

    return ret;
}

int _send_dtls(uiso_network_ctx_t ctx, uint8_t *buffer, size_t length)
{
	int n_bytes_sent = 0;
	int ret = (int) UISO_NETWORK_OK;
	size_t offset = 0;

	uint32_t current_time = sl_sleeptimer_get_time();

	int cid_enabled = MBEDTLS_SSL_CID_DISABLED;
	ret = mbedtls_ssl_get_peer_cid(ctx->ssl_context, &cid_enabled, NULL, 0);
	if (MBEDTLS_SSL_CID_DISABLED == cid_enabled)
	{
		if ((current_time - ctx->last_send_time) > 120)
		{
			/* Attempt re-negotiation */
			do
			{
				ret = mbedtls_ssl_renegotiate(ctx->ssl_context);
			} while ((MBEDTLS_ERR_SSL_WANT_READ == ret) || (MBEDTLS_ERR_SSL_WANT_WRITE == ret)
					|| (MBEDTLS_ERR_SSL_CRYPTO_IN_PROGRESS == ret));

			if (0 != ret)
			{
				/* This code tries to handle issues seen when the server does not support renegociation */
				ret = mbedtls_ssl_close_notify(ctx->ssl_context);
				if (0 == ret)
				{
					ret = mbedtls_ssl_session_reset(ctx->ssl_context);
				} else
				{
					(void) mbedtls_ssl_session_reset(ctx->ssl_context);
				}

				if (0 == ret)
				{
					do
					{
						ret = mbedtls_ssl_handshake(ctx->ssl_context);
					} while ((MBEDTLS_ERR_SSL_WANT_READ == ret)
							|| (MBEDTLS_ERR_SSL_WANT_WRITE == ret));
				}
			}

			if (0 != ret)
			{
				ret = (int) UISO_NETWORK_DTLS_RENEGOCIATION_FAIL;
			}
		}
	} else
	{
		ret = 0;
	}

	/* Actual sending */
	if (0 == ret)
	{
		while (offset != length)
		{
			n_bytes_sent = mbedtls_ssl_write(ctx->ssl_context, buffer + offset, length - offset);
			if ((MBEDTLS_ERR_SSL_WANT_READ == n_bytes_sent)
					|| (MBEDTLS_ERR_SSL_WANT_WRITE == n_bytes_sent)
					|| (MBEDTLS_ERR_SSL_ASYNC_IN_PROGRESS == n_bytes_sent)
					|| (MBEDTLS_ERR_SSL_CRYPTO_IN_PROGRESS == n_bytes_sent))
			{
				/* These mbedTLS return codes mean that the write operation must be retried */
				n_bytes_sent = 0;
			} else if (0 > n_bytes_sent)
			{
				ret = UISO_NETWORK_GENERIC_ERROR;
				do
				{
					ret = mbedtls_ssl_close_notify(ctx->ssl_context);
				} while ((MBEDTLS_ERR_SSL_WANT_READ == ret) || (MBEDTLS_ERR_SSL_WANT_WRITE == ret));

				ret = mbedtls_ssl_session_reset(ctx->ssl_context);
				if (0 == ret)
				{
					do
					{
						ret = mbedtls_ssl_handshake(ctx->ssl_context);
					} while ((MBEDTLS_ERR_SSL_WANT_READ == ret)
							|| (MBEDTLS_ERR_SSL_WANT_WRITE == ret));
				}

				if (0 == ret)
				{
					n_bytes_sent = 0;
				} else
				{
					break;
				}
			}
			offset += n_bytes_sent;
		}

		if (0 == ret)
		{
			ret = offset;
		}
	}

	if (ret >= 0)
	{
		ctx->last_send_time = sl_sleeptimer_get_time();
	}
	return ret;
}

uiso_network_ctx_t uiso_get_network_ctx(enum wifi_socket_id_e id)
{
	return &system_sockets[(size_t) id];
}

static inline uiso_network_ctx_t _get_network_ctx(enum wifi_socket_id_e id)
{
	return &system_sockets[(size_t) id];
}

static int _initialize_network_ctx(uiso_network_ctx_t ctx)
{
	memset(ctx, 0, sizeof(struct uiso_sockets_s));

	ctx->sd = UISO_NETWORK_INVALID_SOCKET;
	ctx->ssl_context = NULL;

	ctx->app_ctx = NULL;
	ctx->app_param = 0;
	ctx->last_recv_time = 0;
	ctx->last_send_time = 0;
	ctx->protocol = (int32_t) uiso_protocol_no_protocol;
	ctx->rx_wait_deadline_s = 0;
	ctx->tx_wait_deadline_s = 0;
	ctx->rx_signal = xSemaphoreCreateBinary();
	ctx->tx_signal = xSemaphoreCreateBinary();

	return 0;
}

mbedtls_ssl_context* uiso_network_get_ssl_ctx(uiso_network_ctx_t ctx)
{
	return ctx->ssl_context;
}
