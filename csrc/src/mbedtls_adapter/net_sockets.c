/*
 *  TCP/IP or UDP/IP networking functions
 *
 *  Copyright The Mbed TLS Contributors
 *  SPDX-License-Identifier: Apache-2.0
 *
 *  Licensed under the Apache License, Version 2.0 (the "License"); you may
 *  not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 *  WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */

/* Enable definition of getaddrinfo() even when compiling with -std=c99. Must
 * be set before mbedtls_config.h, which pulls in glibc's features.h indirectly.
 * Harmless on other platforms. */
#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200112L
#endif
#ifndef _XOPEN_SOURCE
#define _XOPEN_SOURCE 600 /* sockaddr_storage */
#endif

//#include "common.h"

#define MBEDTLS_NET_C
#if defined(MBEDTLS_NET_C)

#include "uiso_net_sockets.h"
#include "mbedtls/platform.h"

#include "mbedtls/net_sockets.h"
#include "mbedtls/error.h"

#include <string.h>
#include <stdio.h>

#if defined(MBEDTLS_HAVE_TIME)
#include <time.h>
#endif

#include <stdint.h>



#define UISO_NET_FALSE	((uint32_t)0)
#define UISO_NET_TRUE	((uint32_t)!UISO_NET_FALSE)

bool uiso_net_compare_addresses_ipv4(SlSockAddrIn_t *address_1,
		SlSockAddrIn_t *address_2)
{
	bool addresses_are_the_same = false;

	if ((SL_AF_INET == address_1->sin_family)
			&& (SL_AF_INET == address_2->sin_family))
	{
		if ((address_1->sin_port == address_2->sin_port)
				&& (address_1->sin_addr.s_addr == address_2->sin_addr.s_addr))
		{
			addresses_are_the_same = true;
		}
	}

	return addresses_are_the_same;
}

void uiso_mbedtls_net_init(uiso_mbedtls_context_t *ctx)
{
	ctx->fd = -1;
	ctx->protocol = (int32_t) uiso_protocol_no_protocol;
	(void) memset(&(ctx->host_addr), 0, sizeof(ctx->host_addr));
}

int uiso_mbedtls_net_connect(uiso_mbedtls_context_t *ctx, const char *host,
		const char *port, int proto)
{
	int ret = UISO_NET_GENERIC_ERROR;
	_i16 dns_status = -1;

	SlSockAddrIn_t *host_addr = &(ctx->host_addr);

	/* Only resolve for IPv4 */
	memset(host_addr, 0, sizeof(SlSockAddrIn_t));

	dns_status = sl_NetAppDnsGetHostByName((_i8*) host, strlen(host),
			&(host_addr->sin_addr.s_addr), SL_AF_INET);

	if (dns_status < 0)
	{
		return (UISO_NET_DNS_ERROR);
	}
	else
	{
		ctx->host_addr.sin_family = SL_AF_INET;
		ctx->host_addr.sin_port = __REV16(atoi(port));
		ctx->host_addr.sin_addr.s_addr = __REV(host_addr->sin_addr.s_addr);
		ctx->host_addr_len = sizeof(struct SlSockAddrIn_t);
	}

	_i16 type = 0;
	_i16 protocol = 0;

	switch (proto)
	{
	case uiso_protocol_dtls_ip4:
		type = SOCK_DGRAM;
		protocol = IPPROTO_UDP;
		break;
	case uiso_protocol_tls_ip4:
		type = SOCK_STREAM;
		protocol = IPPROTO_TCP;
		break;
	case uiso_protocol_udp_ip4:
		type = SOCK_DGRAM;
		protocol = IPPROTO_UDP;
		break;
	case uiso_protocol_tcp_ip4:
		type = SOCK_STREAM;
		protocol = IPPROTO_TCP;
		break;
	default:
		type = 0;
		protocol = 0;
		break;
	}

	ctx->protocol = proto;
	ctx->fd = (int) socket(SL_AF_INET, type, protocol);
	if (ctx->fd >= (int) 0)
	{
		ret = UISO_NET_OK;
	}
	else
	{
		ret = UISO_NET_SOCKET_ERROR;
	}

	if (ret == UISO_NET_OK)
	{
		SlSockNonblocking_t enableOption =
		{ .NonblockingEnabled = 1 };
		(void) sl_SetSockOpt(ctx->fd, SL_SOL_SOCKET, SL_SO_NONBLOCKING,
				(_u8*) &enableOption, sizeof(enableOption));
	}

	if (ret == UISO_NET_OK)
	{
		if (0
				< connect(ctx->fd, (SlSockAddr_t*) &(ctx->host_addr),
						sizeof(ctx->host_addr)))
		{
			ret = UISO_NET_CONNECT_ERROR;
			(void) close(ctx->fd);
			ctx->fd = -1;
		}
	}


	return ret;
}


/* ************************************************************************** */
/* ************************************************************************** */
/* ************************************************************************** */




#if 0
/**
 * \brief          Callback type: send data on the network.
 *
 * \note           That callback may be either blocking or non-blocking.
 *
 * \param ctx      Context for the send callback (typically a file descriptor)
 * \param buf      Buffer holding the data to send
 * \param len      Length of the data to send
 *
 * \return         The callback must return the number of bytes sent if any,
 *                 or a non-zero error code.
 *                 If performing non-blocking I/O, \c MBEDTLS_ERR_SSL_WANT_WRITE
 *                 must be returned when the operation would block.
 *
 * \note           The callback is allowed to send fewer bytes than requested.
 *                 It must always return the number of bytes actually sent.
 */
int uiso_mbedtls_ssl_udp_send(void *ctx, const unsigned char *buf, size_t len)
{
	int ret = MBEDTLS_ERR_ERROR_CORRUPTION_DETECTED;

	int fd = ((uiso_mbedtls_context_t*) ctx)->fd;
	SlSockAddrIn_t host_addr = ((uiso_mbedtls_context_t*) ctx)->host_addr;
	if (fd < 0)
	{
		return -1; // Invalid or uninitialized socket
	}

	ret = sl_SendTo(fd, buf, len, 0, (SlSockAddr_t*) &host_addr,
			sizeof(host_addr));
	if (ret < 0)
	{
		SlSockNonblocking_t enableOption =
		{ .NonblockingEnabled = 0 };
		(void) sl_GetSockOpt(fd, SL_SOL_SOCKET, SL_SO_NONBLOCKING,
				(_u8*) &enableOption, sizeof(enableOption));

		if (enableOption.NonblockingEnabled == 0)
		{
			ret = MBEDTLS_ERR_NET_SEND_FAILED;
		}
		else
		{
			ret = MBEDTLS_ERR_SSL_WANT_WRITE;
		}
	}

	return ret;
}

/**
 * \brief          Callback type: receive data from the network.
 *
 * \note           That callback may be either blocking or non-blocking.
 *
 * \param ctx      Context for the receive callback (typically a file
 *                 descriptor)
 * \param buf      Buffer to write the received data to
 * \param len      Length of the receive buffer
 *
 * \returns        If data has been received, the positive number of bytes received.
 * \returns        \c 0 if the connection has been closed.
 * \returns        If performing non-blocking I/O, \c MBEDTLS_ERR_SSL_WANT_READ
 *                 must be returned when the operation would block.
 * \returns        Another negative error code on other kinds of failures.
 *
 * \note           The callback may receive fewer bytes than the length of the
 *                 buffer. It must always return the number of bytes actually
 *                 received and written to the buffer.
 */
int uiso_mbedtls_ssl_recv(void *ctx, unsigned char *buf, size_t len)
{
	int ret = (size_t) len;
	uiso_mbedtls_context_t *udp_ctx = (uiso_mbedtls_context_t*) ctx;

	ret = sl_RecvFrom(udp_ctx->fd, (void*) buf, (_i16) len, 0,
			&(udp_ctx->host_addr), (SlSocklen_t) sizeof(udp_ctx->host_addr));
	if (ret < 0)
	{
		SlSockNonblocking_t enableOption =
		{ .NonblockingEnabled = 0 };
		(void) sl_GetSockOpt(udp_ctx->fd, SL_SOL_SOCKET, SL_SO_NONBLOCKING,
				(_u8*) &enableOption, sizeof(enableOption));

		if (enableOption.NonblockingEnabled == 0)
		{
			ret = MBEDTLS_ERR_NET_RECV_FAILED;
		}
		else
		{
			ret = MBEDTLS_ERR_SSL_WANT_READ;
		}
	}

	return ret;
}

/**
 * \brief          Callback type: receive data from the network, with timeout
 *
 * \note           That callback must block until data is received, or the
 *                 timeout delay expires, or the operation is interrupted by a
 *                 signal.
 *
 * \param ctx      Context for the receive callback (typically a file descriptor)
 * \param buf      Buffer to write the received data to
 * \param len      Length of the receive buffer
 * \param timeout  Maximum number of milliseconds to wait for data
 *                 0 means no timeout (potentially waiting forever)
 *
 * \return         The callback must return the number of bytes received,
 *                 or a non-zero error code:
 *                 \c MBEDTLS_ERR_SSL_TIMEOUT if the operation timed out,
 *                 \c MBEDTLS_ERR_SSL_WANT_READ if interrupted by a signal.
 *
 * \note           The callback may receive fewer bytes than the length of the
 *                 buffer. It must always return the number of bytes actually
 *                 received and written to the buffer.
 */
int mbedtls_ssl_recv_timeout(void *ctx, unsigned char *buf, size_t len,
		uint32_t timeout)
{
	int ret = MBEDTLS_ERR_ERROR_CORRUPTION_DETECTED;
	uiso_mbedtls_context_t *udp_ctx = (uiso_mbedtls_context_t*) ctx;

	printf("SSL RECV\n\r");
	struct timeval tv;
	fd_set read_fds;
	int fd = udp_ctx->fd;

	SL_FD_ZERO(&read_fds);
	SL_FD_SET(fd, &read_fds);

	tv.tv_sec = timeout / 1000;
	tv.tv_usec = (timeout % 1000) * 1000;

	ret = select(FD_SETSIZE, &read_fds, NULL, NULL, timeout == 0 ? NULL : &tv);

	/* Zero fds ready means we timed out */
	if (ret == 0)
		return ( MBEDTLS_ERR_SSL_TIMEOUT);

	if (ret < 0)
	{
		return ( MBEDTLS_ERR_NET_RECV_FAILED);
	}

	return (uiso_mbedtls_ssl_recv(ctx, buf, len));
}
#endif

/*
 * Prepare for using the sockets interface
 */
static int net_prepare(void)
{
	return (0);
}

/*
 * Return 0 if the file descriptor is valid, an error otherwise.
 * If for_select != 0, check whether the file descriptor is within the range
 * allowed for fd_set used for the FD_xxx macros and the select() function.
 */
static int check_fd(int fd, int for_select)
{
	if (fd < 0)
		return ( MBEDTLS_ERR_NET_INVALID_CONTEXT);

	/* A limitation of select() is that it only works with file descriptors
	 * that are strictly less than FD_SETSIZE. This is a limitation of the
	 * fd_set type. Error out early, because attempting to call FD_SET on a
	 * large file descriptor is a buffer overflow on typical platforms. */
	if (for_select && fd >= FD_SETSIZE)
		return ( MBEDTLS_ERR_NET_POLL_FAILED);

	return (0);
}

/*
 * Initialize a context
 */
void mbedtls_net_init(mbedtls_net_context *ctx)
{
	ctx->fd = -1;
}

#if 0
int mbedtls_net_connect(mbedtls_net_context *ctx, const char *host,
		const char *port, int proto)
{
	int ret = MBEDTLS_ERR_ERROR_CORRUPTION_DETECTED;
	_i16 dns_status = -1;

	SlSockAddrIn_t host_addr;

	if ((ret = net_prepare()) != 0)
		return (ret);

	/* Only resolve for IPv4 */
	memset(&host_addr, 0, sizeof(host_addr));
	host_addr.sin_family = SL_AF_INET;

	dns_status = sl_NetAppDnsGetHostByName((_i8*) host, strlen(host),
			&(host_addr.sin_addr.s_addr), SL_AF_INET);

	if (dns_status < 0)
	{
		return ( MBEDTLS_ERR_NET_UNKNOWN_HOST);
	}
	else
	{
		host_addr.sin_family = SL_AF_INET;
		host_addr.sin_port = __REV16(atoi(port));
		host_addr.sin_addr.s_addr = __REV(host_addr.sin_addr.s_addr);
	}

	_i16 domain = proto == MBEDTLS_NET_PROTO_UDP ? SOCK_DGRAM : SOCK_STREAM;
	_i16 protocol = proto == MBEDTLS_NET_PROTO_UDP ? IPPROTO_UDP : IPPROTO_TCP;

	ctx->fd = (int) socket(SL_AF_INET, domain, protocol);
	if (ctx->fd < (int) 0)
	{
		return MBEDTLS_ERR_NET_SOCKET_FAILED;
	}

	if (0 == connect(ctx->fd, (SlSockAddr_t*) &host_addr, sizeof(host_addr)))
	{
		return 0;
	}

	(void) close(ctx->fd);
	ctx->fd = -1;
	return MBEDTLS_ERR_NET_CONNECT_FAILED;
}
#endif

/*
 * Create a listening socket on bind_ip:port
 */
int mbedtls_net_bind(mbedtls_net_context *ctx, const char *bind_ip,
		const char *port, int proto)
{
	int ret = MBEDTLS_ERR_ERROR_CORRUPTION_DETECTED;
	_i16 dns_status = -1;

	SlSockAddrIn_t host_addr;

	if ((ret = net_prepare()) != 0)
		return (ret);

	/* Only resolve for IPv4 */
	memset(&host_addr, 0, sizeof(host_addr));
	host_addr.sin_family = SL_AF_INET;

	dns_status = sl_NetAppDnsGetHostByName((_i8*) bind_ip, strlen(bind_ip),
			&(host_addr.sin_addr.s_addr), SL_AF_INET);

	if (dns_status < 0)
	{
		return ( MBEDTLS_ERR_NET_UNKNOWN_HOST);
	}
	else
	{
		host_addr.sin_family = SL_AF_INET;
		host_addr.sin_port = __REV16(atoi(port));
		host_addr.sin_addr.s_addr = __REV(host_addr.sin_addr.s_addr);
	}

	_i16 domain = proto == MBEDTLS_NET_PROTO_UDP ? SOCK_DGRAM : SOCK_STREAM;
	_i16 protocol = proto == MBEDTLS_NET_PROTO_UDP ? IPPROTO_UDP : IPPROTO_TCP;

	ctx->fd = (int) socket(SL_AF_INET, domain, protocol);
	if (ctx->fd < (int) 0)
	{
		return MBEDTLS_ERR_NET_SOCKET_FAILED;
	}

	if ( bind(ctx->fd, &host_addr, sizeof(host_addr)) != 0)
	{
		close(ctx->fd);
		ctx->fd = -1;
		ret = MBEDTLS_ERR_NET_BIND_FAILED;
	}
	else
	{
		ret = 0;
	}

	if (ret == 0)
	{
		/* Listen only makes sense for TCP */
		if (proto == MBEDTLS_NET_PROTO_TCP)
		{
			if ( listen(ctx->fd, MBEDTLS_NET_LISTEN_BACKLOG) != 0)
			{
				close(ctx->fd);
				ctx->fd = -1;
				ret = MBEDTLS_ERR_NET_LISTEN_FAILED;
			}
		}
	}
	return (ret);

}

#if 0
/*
 * Accept a connection from a remote client
 */
int mbedtls_net_accept( mbedtls_net_context *bind_ctx,
                        mbedtls_net_context *client_ctx,
                        void *client_ip, size_t buf_size, size_t *ip_len )
{
    int ret = MBEDTLS_ERR_ERROR_CORRUPTION_DETECTED;
    int type;

    struct sockaddr_storage client_addr;

#if defined(__socklen_t_defined) || defined(_SOCKLEN_T) ||  \
    defined(_SOCKLEN_T_DECLARED) || defined(__DEFINED_socklen_t) || \
    defined(socklen_t) || (defined(_POSIX_VERSION) && _POSIX_VERSION >= 200112L)
    socklen_t n = (socklen_t) sizeof( client_addr );
    socklen_t type_len = (socklen_t) sizeof( type );
#else
    int n = (int) sizeof( client_addr );
    int type_len = (int) sizeof( type );
#endif

    /* Is this a TCP or UDP socket? */
    if( getsockopt( bind_ctx->fd, SOL_SOCKET, SO_TYPE,
                    (void *) &type, &type_len ) != 0 ||
        ( type != SOCK_STREAM && type != SOCK_DGRAM ) )
    {
        return( MBEDTLS_ERR_NET_ACCEPT_FAILED );
    }

    if( type == SOCK_STREAM )
    {
        /* TCP: actual accept() */
        ret = client_ctx->fd = (int) accept( bind_ctx->fd,
                                             (struct sockaddr *) &client_addr, &n );
    }
    else
    {
        /* UDP: wait for a message, but keep it in the queue */
        char buf[1] = { 0 };

        ret = (int) recvfrom( bind_ctx->fd, buf, sizeof( buf ), MSG_PEEK,
                        (struct sockaddr *) &client_addr, &n );

#if defined(_WIN32)
        if( ret == SOCKET_ERROR &&
            WSAGetLastError() == WSAEMSGSIZE )
        {
            /* We know buf is too small, thanks, just peeking here */
            ret = 0;
        }
#endif
    }

    if( ret < 0 )
    {
        if( net_would_block( bind_ctx ) != 0 )
            return( MBEDTLS_ERR_SSL_WANT_READ );

        return( MBEDTLS_ERR_NET_ACCEPT_FAILED );
    }

    /* UDP: hijack the listening socket to communicate with the client,
     * then bind a new socket to accept new connections */
    if( type != SOCK_STREAM )
    {
        struct sockaddr_storage local_addr;
        int one = 1;

        if( connect( bind_ctx->fd, (struct sockaddr *) &client_addr, n ) != 0 )
            return( MBEDTLS_ERR_NET_ACCEPT_FAILED );

        client_ctx->fd = bind_ctx->fd;
        bind_ctx->fd   = -1; /* In case we exit early */

        n = sizeof( struct sockaddr_storage );
        if( getsockname( client_ctx->fd,
                         (struct sockaddr *) &local_addr, &n ) != 0 ||
            ( bind_ctx->fd = (int) socket( local_addr.ss_family,
                                           SOCK_DGRAM, IPPROTO_UDP ) ) < 0 ||
            setsockopt( bind_ctx->fd, SOL_SOCKET, SO_REUSEADDR,
                        (const char *) &one, sizeof( one ) ) != 0 )
        {
            return( MBEDTLS_ERR_NET_SOCKET_FAILED );
        }

        if( bind( bind_ctx->fd, (struct sockaddr *) &local_addr, n ) != 0 )
        {
            return( MBEDTLS_ERR_NET_BIND_FAILED );
        }
    }

    if( client_ip != NULL )
    {
        if( client_addr.ss_family == AF_INET )
        {
            struct sockaddr_in *addr4 = (struct sockaddr_in *) &client_addr;
            *ip_len = sizeof( addr4->sin_addr.s_addr );

            if( buf_size < *ip_len )
                return( MBEDTLS_ERR_NET_BUFFER_TOO_SMALL );

            memcpy( client_ip, &addr4->sin_addr.s_addr, *ip_len );
        }
        else
        {
            struct sockaddr_in6 *addr6 = (struct sockaddr_in6 *) &client_addr;
            *ip_len = sizeof( addr6->sin6_addr._S6_un );

            if( buf_size < *ip_len )
                return( MBEDTLS_ERR_NET_BUFFER_TOO_SMALL );

            memcpy( client_ip, &addr6->sin6_addr._S6_un, *ip_len);
        }
    }

    return( 0 );
}
#endif

/*
 * Set the socket blocking or non-blocking
 */
int mbedtls_net_set_block(mbedtls_net_context *ctx)
{
	return 0;
}

int mbedtls_net_set_nonblock(mbedtls_net_context *ctx)
{
	return 0;
}

/*
 * Check if data is available on the socket
 */

int mbedtls_net_poll(mbedtls_net_context *ctx, uint32_t rw, uint32_t timeout)
{
	int ret = MBEDTLS_ERR_ERROR_CORRUPTION_DETECTED;
	struct timeval tv;

	fd_set read_fds;
	fd_set write_fds;

	int fd = ctx->fd;

	ret = check_fd(fd, 1);
	if (ret != 0)
		return (ret);

#if defined(__has_feature)
#if __has_feature(memory_sanitizer)
    /* Ensure that memory sanitizers consider read_fds and write_fds as
     * initialized even on platforms such as Glibc/x86_64 where FD_ZERO
     * is implemented in assembly. */
    memset( &read_fds, 0, sizeof( read_fds ) );
    memset( &write_fds, 0, sizeof( write_fds ) );
#endif
#endif

	FD_ZERO(&read_fds);
	if (rw & MBEDTLS_NET_POLL_READ)
	{
		rw &= ~MBEDTLS_NET_POLL_READ;
		FD_SET(fd, &read_fds);
	}

	FD_ZERO(&write_fds);
	if (rw & MBEDTLS_NET_POLL_WRITE)
	{
		rw &= ~MBEDTLS_NET_POLL_WRITE;
		FD_SET(fd, &write_fds);
	}

	if (rw != 0)
		return ( MBEDTLS_ERR_NET_BAD_INPUT_DATA);

	tv.tv_sec = timeout / 1000;
	tv.tv_usec = (timeout % 1000) * 1000;

	do
	{
		ret = select(fd + 1, &read_fds, &write_fds, NULL,
				timeout == (uint32_t) -1 ? NULL : &tv);
	} while (0);

	if (ret < 0)
		return ( MBEDTLS_ERR_NET_POLL_FAILED);

	ret = 0;
	if (FD_ISSET(fd, &read_fds))
		ret |= MBEDTLS_NET_POLL_READ;
	if (FD_ISSET(fd, &write_fds))
		ret |= MBEDTLS_NET_POLL_WRITE;

	return (ret);
}

/*
 * Read at most 'len' characters
 */
int mbedtls_net_recv(void *ctx, unsigned char *buf, size_t len)
{
	int ret = MBEDTLS_ERR_ERROR_CORRUPTION_DETECTED;
	int fd = ((uiso_mbedtls_context_t*) ctx)->fd;
	SlSockAddrIn_t * host_addr = &((uiso_mbedtls_context_t*) ctx)->host_addr;

	ret = check_fd(fd, 0);
	if (ret != 0)
		return (ret);

	if (len > 16000)
	{
		len = 16000;
	}

	SlSocklen_t from_len = sizeof(SlSockAddrIn_t);
	ret = (int) sl_RecvFrom(fd, (char*)buf, (int)len, 0, (SlSockAddr_t *)host_addr,
			&from_len);
	if (ret < 0)
	{
		if (ret == SL_EAGAIN)
		{
			ret = MBEDTLS_ERR_SSL_WANT_READ;
		}
		else
		{
			ret = MBEDTLS_ERR_NET_RECV_FAILED;
		}
	}

	return (ret);
}

#if 0
/*
 * Read at most 'len' characters, blocking for at most 'timeout' ms
 */
int mbedtls_net_recv_timeout(void *ctx, unsigned char *buf, size_t len,
		uint32_t timeout)
{
	int ret = MBEDTLS_ERR_ERROR_CORRUPTION_DETECTED;
	struct timeval tv;
	fd_set read_fds;
	int fd = ((uiso_mbedtls_context_t*) ctx)->fd;

	ret = check_fd(fd, 1);
	if (ret != 0)
		return (ret);

	SL_FD_ZERO(&read_fds);
	SL_FD_SET(fd, &read_fds);

	tv.tv_sec = timeout / 1000;
	tv.tv_usec = (timeout % 1000) * 1000;

	ret = select(FD_SETSIZE, &read_fds, NULL, NULL, timeout == 0 ? NULL : &tv);

	/* Zero fds ready means we timed out */
	if (ret == 0)
		return ( MBEDTLS_ERR_SSL_TIMEOUT);

	if (ret < 0)
	{
		return ( MBEDTLS_ERR_NET_RECV_FAILED);
	}

	/* This call will not block */
	return (mbedtls_net_recv(ctx, buf, len));
}
#endif

/*
 * Write at most 'len' characters
 */
int mbedtls_net_send(void *ctx, const unsigned char *buf, size_t len)
{
	int ret = MBEDTLS_ERR_ERROR_CORRUPTION_DETECTED;
	int fd = ((uiso_mbedtls_context_t*) ctx)->fd;
	SlSockAddrIn_t * to_addr = &(((uiso_mbedtls_context_t*) ctx)->host_addr);
	ret = check_fd(fd, 0);
	if (ret != 0)
		return (ret);

	ret = sl_SendTo(fd, (void*) buf, (_i16) len, (_i16) 0,  (SlSockAddr_t *)to_addr,
			sizeof(SlSockAddrIn_t));
	if (ret < 0)
	{
		return ( MBEDTLS_ERR_NET_SEND_FAILED);
	}

	return (ret);
}

/*
 * Close the connection
 */
void mbedtls_net_close(mbedtls_net_context *ctx)
{
	if (ctx->fd == -1)
		return;

	(void) sl_Close(ctx->fd);
	ctx->fd = -1;
}

/*
 * Gracefully close the connection
 */
void mbedtls_net_free(mbedtls_net_context *ctx)
{
	if (ctx->fd == -1)
		return;

	sl_Close(ctx->fd);
	ctx->fd = -1;
}

#endif /* MBEDTLS_NET_C */
