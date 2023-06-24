/*
 * mbedtls_connector.c
 *
 *  Created on: 11 dic 2022
 *      Author: Francisco
 */

#include "uiso.h"
#include "lwm2m_security.h"
#include "../connection.h"

#include "network.h"

#include "mbedtls/ctr_drbg.h"
#include "mbedtls/timing.h"

#include "mbedtls/aes.h"
#include "mbedtls/base64.h"
#include "mbedtls/net_sockets.h"
#include "mbedtls/entropy.h"


/* Certificate ciphersuites */
static const int ciphersuites_pk[] =
{
MBEDTLS_TLS_ECDHE_ECDSA_WITH_AES_128_CCM_8,
MBEDTLS_TLS_ECDHE_ECDSA_WITH_AES_128_CCM, 0 };

/* PSK ciphersuites */
static const int ciphersuites_psk[] =
{
MBEDTLS_TLS_PSK_WITH_AES_128_CCM_8,
MBEDTLS_TLS_PSK_WITH_AES_128_CCM, 0 };

static const uint16_t sig_algorithms[] =
{	MBEDTLS_TLS1_3_SIG_ECDSA_SECP256R1_SHA256,
	MBEDTLS_TLS1_3_SIG_NONE
};

static const uint16_t groups[] =
{ MBEDTLS_SSL_IANA_TLS_GROUP_SECP256R1, MBEDTLS_SSL_IANA_TLS_GROUP_NONE };

/* SSL Timer for the LWM2M connection */
uiso_mbedtls_timing_delay_t ssl_timer;

/* SSL Context */
mbedtls_ssl_context ssl_context;

/* SSL Config */
mbedtls_ssl_config ssl_config;

/* DRBG Context*/
mbedtls_ctr_drbg_context drbg_context;

/* Initialize entropy */
uint32_t entropy = 0x55555555;

/* Entropy Context */
mbedtls_entropy_context entropy_context;

/* Own certificate context */
mbedtls_x509_crt own_crt_ctx;

/* Peer certificate context */
mbedtls_x509_crt peer_crt_ctx;

/* Peer certificate revocation list context */
mbedtls_x509_crl peer_crl_ctx;

/* Own private key */
mbedtls_pk_context own_pk;

/* own connection id */
static unsigned char own_cid[MBEDTLS_SSL_CID_OUT_LEN_MAX] = {0};

const uint8_t own_cert[] =
{ "-----BEGIN CERTIFICATE-----\r\n"
		"MIICEDCCAbegAwIBAgIUJVNjKjWmCgniedGzfKbJpeFR+dIwCgYIKoZIzj0EAwIw\r\n"
		"XjELMAkGA1UEBhMCREUxEzARBgNVBAgMClNvbWUtU3RhdGUxITAfBgNVBAoMGElu\r\n"
		"dGVybmV0IFdpZGdpdHMgUHR5IEx0ZDEXMBUGA1UEAwwOd2FrYWFtYV94ZGsxMTAw\r\n"
		"HhcNMjIxMjA2MDkzNjI4WhcNMjMxMjAxMDkzNjI4WjBeMQswCQYDVQQGEwJERTET\r\n"
		"MBEGA1UECAwKU29tZS1TdGF0ZTEhMB8GA1UECgwYSW50ZXJuZXQgV2lkZ2l0cyBQ\r\n"
		"dHkgTHRkMRcwFQYDVQQDDA53YWthYW1hX3hkazExMDBZMBMGByqGSM49AgEGCCqG\r\n"
		"SM49AwEHA0IABJQHHTffNFksBWIvwwbNLVPeLY+A2gMY23YVKsshx8mFHbJRM8gu\r\n"
		"mjuQI9pqWYlq+ARJSGZ5eUeVfZfMH1/GcrGjUzBRMB0GA1UdDgQWBBTs5aOxib4/\r\n"
		"lK6cZ/ULwFyyc520FTAfBgNVHSMEGDAWgBTs5aOxib4/lK6cZ/ULwFyyc520FTAP\r\n"
		"BgNVHRMBAf8EBTADAQH/MAoGCCqGSM49BAMCA0cAMEQCIHXXG/ImpMhVLrXLlQl8\r\n"
		"7DTqEJgpZ8emSXH0zo/GgVjKAiARnrTtHnIuZLqpJHJ9W6JyUIvq4mZQZJQrDDZ8\r\n"
		"u/2K4g==\r\n"
		"-----END CERTIFICATE-----\r\n" };

const uint8_t server_cert[] =
{ "-----BEGIN CERTIFICATE-----\r\n"
		"MIICBzCCAa+gAwIBAgIUcVVVy8vLLI2v3pxlOoZ810gPyNYwCgYIKoZIzj0EAwIw\r\n"
		"WTELMAkGA1UEBhMCQVUxEzARBgNVBAgMClNvbWUtU3RhdGUxITAfBgNVBAoMGElu\r\n"
		"dGVybmV0IFdpZGdpdHMgUHR5IEx0ZDESMBAGA1UEAwwJbG9jYWxob3N0MCAXDTIx\r\n"
		"MDcxNTEzMzUxOFoYDzIxMjEwNjIxMTMzNTE4WjBZMQswCQYDVQQGEwJBVTETMBEG\r\n"
		"A1UECAwKU29tZS1TdGF0ZTEhMB8GA1UECgwYSW50ZXJuZXQgV2lkZ2l0cyBQdHkg\r\n"
		"THRkMRIwEAYDVQQDDAlsb2NhbGhvc3QwWTATBgcqhkjOPQIBBggqhkjOPQMBBwNC\r\n"
		"AATSbarzAiS5luCVFoABZyGTa9wQkG+25w8KXqGtlvjbgVooIO/X89nKQ2Ea+KPh\r\n"
		"tGXPj93ZQcK5gVBAj7j1Vb+Oo1MwUTAdBgNVHQ4EFgQUl3UTD4jWyM376aj6yaLN\r\n"
		"jWCUm0gwHwYDVR0jBBgwFoAUl3UTD4jWyM376aj6yaLNjWCUm0gwDwYDVR0TAQH/\r\n"
		"BAUwAwEB/zAKBggqhkjOPQQDAgNGADBDAh8QrcYR4drlHAbiztd/32Ecw4bKmUR5\r\n"
		"76jwP1qYrqlUAiBD0EJ2HfIyRbQkKLXUx4Awt9rZO65VhdRWhcGtiAJF7w==\r\n"
		"-----END CERTIFICATE-----\r\n" };

const uint8_t own_key[] =
{ "-----BEGIN EC PARAMETERS-----\r\n"
		"BggqhkjOPQMBBw==\r\n"
		"-----END EC PARAMETERS-----\r\n"
		"-----BEGIN EC PRIVATE KEY-----\r\n"
		"MHcCAQEEIJwbEl6dCqmE9Rii80z+DkTJlXC+y9OnVajavtvFnUwcoAoGCCqGSM49\r\n"
		"AwEHoUQDQgAElAcdN980WSwFYi/DBs0tU94tj4DaAxjbdhUqyyHHyYUdslEzyC6a\r\n"
		"O5Aj2mpZiWr4BElIZnl5R5V9l8wfX8ZysQ==\r\n"
		"-----END EC PRIVATE KEY-----\r\n" };



void mbedtls_cleanup(void);

int mbedtls_connector_initialize(lwm2m_object_t * securityObjP, uint16_t secObjInstID)
{
	int ret = -1;

	/* Start with mbedtls init */
	enum lwm2m_security_mode_e security_mode = get_security_mode(securityObjP, secObjInstID);

	uiso_mbedtls_init_timer(&ssl_timer);
	UISO_MBED_TLS_THREADING_SET_ALT();

	mbedtls_ssl_init(&ssl_context);

	mbedtls_ssl_config_init(&ssl_config);
	mbedtls_ctr_drbg_init(&drbg_context);
	mbedtls_entropy_init(&entropy_context);

	ret = mbedtls_ssl_config_defaults(&ssl_config, MBEDTLS_SSL_IS_CLIENT,
	MBEDTLS_SSL_TRANSPORT_DATAGRAM, MBEDTLS_SSL_PRESET_DEFAULT);
	if (0 == ret)
	{
		ret = mbedtls_ssl_conf_max_frag_len(&ssl_config,
		MBEDTLS_SSL_MAX_FRAG_LEN_1024);
	}

	if (0 == ret)
	{
		mbedtls_ssl_conf_authmode(&ssl_config, MBEDTLS_SSL_VERIFY_OPTIONAL);
		mbedtls_ssl_conf_read_timeout(&ssl_config, 5000);
		mbedtls_ssl_conf_rng(&ssl_config, mbedtls_ctr_drbg_random,
				&drbg_context);
		//mbedtls_entropy_add_source(&entropy_context, mbedtls_entropy_f_source_ptr f_source, void *p_source, size_t threshold, MBEDTLS_ENTROPY_SOURCE_STRONG );
		mbedtls_ctr_drbg_seed(&drbg_context, mbedtls_entropy_func, &entropy,
		NULL, 0);
	}

	if ((0 == ret) && (security_mode_certificate == security_mode))
	{
		mbedtls_ssl_conf_groups(&ssl_config, groups);
		mbedtls_ssl_conf_sig_algs(&ssl_config, sig_algorithms);
		mbedtls_pk_init(&own_pk);
		mbedtls_x509_crt_init(&own_crt_ctx);
		mbedtls_x509_crt_init(&peer_crt_ctx);
		mbedtls_x509_crl_init(&peer_crl_ctx);
	}

	if ((0 == ret) && (security_mode_psk == security_mode))
	{
		size_t psk_len = 0;
		unsigned char *psk = get_connection_psk(securityObjP,
				secObjInstID, &psk_len);

		size_t public_identity_len = 0;
		unsigned char *public_identity = get_public_identiy(securityObjP,
				secObjInstID, &public_identity_len);

		ret = mbedtls_ssl_conf_psk(&ssl_config, psk, psk_len, public_identity,
				strlen((char*) public_identity));
	}

	if ((0 == ret) && (security_mode_certificate == security_mode))
	{
		// Load certificates
		ret = mbedtls_x509_crt_parse(&own_crt_ctx, own_cert, sizeof(own_cert));

		if (0 == ret)
		{
			ret = mbedtls_x509_crt_parse(&peer_crt_ctx, server_cert,
					sizeof(server_cert));
		}

		if (0 == ret)
		{
			ret = mbedtls_pk_parse_key(&own_pk, (unsigned char*) &own_key,
					sizeof(own_key), (unsigned char*) NULL, (size_t) 0,
					mbedtls_ctr_drbg_random, &drbg_context);
		}

		if (0 == ret)
		{
			ret = mbedtls_ssl_conf_own_cert(&ssl_config, &own_crt_ctx, &own_pk);
		}

		if (0 == ret)
		{
			mbedtls_ssl_conf_ca_chain(&ssl_config, &peer_crt_ctx,
					&peer_crl_ctx);
		}
	}

	if (0 == ret)
	{
		if (security_mode_certificate == security_mode)
		{
			mbedtls_ssl_conf_ciphersuites(&ssl_config, ciphersuites_pk);
		}
		else if(security_mode_psk == security_mode)
		{
			mbedtls_ssl_conf_ciphersuites(&ssl_config, ciphersuites_psk);
		}
	}

	if (0 == ret)
	{
		mbedtls_ssl_conf_min_tls_version(&ssl_config,
				MBEDTLS_SSL_VERSION_TLS1_2);
		mbedtls_ssl_conf_renegotiation( &ssl_config, MBEDTLS_SSL_RENEGOTIATION_ENABLED );
		ret = mbedtls_ssl_conf_cid(&ssl_config, 6, MBEDTLS_SSL_UNEXPECTED_CID_FAIL);
	}

	if (0 == ret)
	{
		ret = mbedtls_ssl_setup(&ssl_context, &ssl_config);
	}

#ifdef MBEDTLS_SSL_DTLS_CONNECTION_ID
	if(0 == ret)
	{
		memset(own_cid, 0, sizeof(own_cid));

		ret = mbedtls_ctr_drbg_random(&drbg_context, own_cid, 8);
		*((uint64_t*)own_cid) ^= (uint64_t)lwm2m_gettime();

		ret = mbedtls_ssl_set_cid(&ssl_context, MBEDTLS_SSL_CID_DISABLED, NULL, 0);
	}
#endif

	if (0 == ret)
	{
		mbedtls_ssl_set_timer_cb(&ssl_context, &ssl_timer,
				uiso_mbedtls_timing_set_delay, uiso_mbedtls_timing_get_delay);
	}

	if(0 == ret)
	{
		ret = uiso_network_register_ssl_context(uiso_get_network_ctx(wifi_service_lwm2m_socket), &ssl_context);
	}

	return ret;
}


void mbedtls_cleanup(void)
{
	/* Free Certificate Chains */

	mbedtls_x509_crt_free(&own_crt_ctx);
	mbedtls_x509_crt_free(&peer_crt_ctx);
	mbedtls_x509_crl_free(&peer_crl_ctx);

	/* Free own PK */
	mbedtls_pk_free(&own_pk);

	uiso_mbedtls_deinit_timer(&ssl_timer);
	mbedtls_threading_free_alt();

	mbedtls_ctr_drbg_free(&drbg_context);
	mbedtls_entropy_free(&entropy_context);
	mbedtls_ssl_free(&ssl_context);
	mbedtls_ssl_config_free(&ssl_config);
}

void mbedtls_connector_close(void)
{
	mbedtls_ssl_close_notify(&ssl_context);
	//mbedtls_net_close(&net_context);
	mbedtls_cleanup();
}
