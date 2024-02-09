/*
 * mbedtls_connector.c
 *
 *  Created on: 11 dic 2022
 *      Author: Francisco
 */

#include "miso.h"
#include "lwm2m_security.h"
#include "../connection.h"

#include "network.h"

#include "mbedtls/ctr_drbg.h"
#include "mbedtls/timing.h"

#include "mbedtls/aes.h"
#include "mbedtls/base64.h"
#include "mbedtls/net_sockets.h"
#include "mbedtls/entropy.h"





void mbedtls_cleanup(void);

int mbedtls_connector_initialize(lwm2m_object_t * securityObjP, uint16_t secObjInstID)
{
	int ret = -1;

	/* Start with mbedtls init */
	enum lwm2m_security_mode_e security_mode = get_security_mode(securityObjP, secObjInstID);

		size_t psk_len = 0;
		unsigned char *psk = get_connection_psk(securityObjP, secObjInstID, &psk_len);

		size_t public_identity_len = 0;
		unsigned char *public_identity = get_public_identiy(securityObjP, secObjInstID, &public_identity_len);

	__BKPT(0);

	return ret;
}


void mbedtls_cleanup(void)
{

}

void mbedtls_connector_close(void)
{

}
