/*
 * lwm2m_security.h
 *
 *  Created on: 15 dic 2022
 *      Author: Francisco
 */

#ifndef LWM2M_LIGHTCLIENT_LWM2M_SECURITY_H_
#define LWM2M_LIGHTCLIENT_LWM2M_SECURITY_H_

#include "liblwm2m.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

enum{
	lwm2m_security_uri = LWM2M_SECURITY_URI_ID,
	lwm2m_security_bootstrap = LWM2M_SECURITY_BOOTSTRAP_ID,
	lwm2m_security_mode = LWM2M_SECURITY_SECURITY_ID,
	lwm2m_security_own_public_identity = LWM2M_SECURITY_PUBLIC_KEY_ID,
	lwm2m_security_server_public_key = LWM2M_SECURITY_SERVER_PUBLIC_KEY_ID,
	lwm2m_security_own_secret_key = LWM2M_SECURITY_SECRET_KEY_ID,
	lwm2m_security_sms_security_mode = LWM2M_SECURITY_SMS_SECURITY_ID,
	lwm2m_security_sms_key_binding = LWM2M_SECURITY_SMS_KEY_PARAM_ID,
	lwm2m_security_sms_secret_key = LWM2M_SECURITY_SMS_SECRET_KEY_ID,
	lwm2m_security_sms_server_number = LWM2M_SECURITY_SMS_SERVER_NUMBER_ID,
	lwm2m_short_server_id = LWM2M_SECURITY_SHORT_SERVER_ID,
	lwm2m_client_hold_off_time = LWM2M_SECURITY_HOLD_OFF_ID,
	lwm2m_boostrap_server_timeout = LWM2M_SECURITY_BOOTSTRAP_TIMEOUT_ID,

	/* Security v1.1 extensions */
	lwm2m_matching_type = 13,
	lwm2m_server_name_indication = 14,
	lwm2m_certificate_usage = 15,
	lwm2m_dtls_tls_ciphersuite =  16,
	lwm2m_oscore_security_mode = 17,
};

enum lwm2m_security_mode_e {
	security_mode_psk = LWM2M_SECURITY_MODE_PRE_SHARED_KEY,
	security_mode_rpk = LWM2M_SECURITY_MODE_RAW_PUBLIC_KEY,
	security_mode_certificate = LWM2M_SECURITY_MODE_CERTIFICATE,
	security_mode_no_sec = LWM2M_SECURITY_MODE_NONE,
	security_mode_est = 4
};

enum {
	matching_type_exact_match = 0,
	matching_type_exact_sha256 = 1,
	matching_type_exact_sha384 = 2,
	matching_type_exact_sha512 = 3,
};

unsigned char* get_connection_psk(lwm2m_object_t *objectP,
		uint16_t secObjInstID, size_t *psk_len);

unsigned char* get_public_identiy(lwm2m_object_t *objectP,
		uint16_t secObjInstID, size_t *public_identity_len);

enum lwm2m_security_mode_e get_security_mode(lwm2m_object_t *objectP, uint16_t secObjInstID);

#endif /* LWM2M_LIGHTCLIENT_LWM2M_SECURITY_H_ */
