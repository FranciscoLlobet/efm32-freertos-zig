/*******************************************************************************
 *
 * Copyright (c) 2013, 2014, 2015 Intel Corporation and others.
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
 *    Bosch Software Innovations GmbH - Please refer to git log
 *    Pascal Rieux - Please refer to git log
 *    Scott Bertin, AMETEK, Inc. - Please refer to git log
 *
 *******************************************************************************/

/*
 *  Resources:
 *
 *          Name            | ID | Operations | Instances | Mandatory |  Type   |  Range  | Units |
 *  Server URI              |  0 |            |  Single   |    Yes    | String  |         |       |
 *  Bootstrap Server        |  1 |            |  Single   |    Yes    | Boolean |         |       |
 *  Security Mode           |  2 |            |  Single   |    Yes    | Integer |   0-3   |       |
 *  Public Key or ID        |  3 |            |  Single   |    Yes    | Opaque  |         |       |
 *  Server Public Key or ID |  4 |            |  Single   |    Yes    | Opaque  |         |       |
 *  Secret Key              |  5 |            |  Single   |    Yes    | Opaque  |         |       |
 *  SMS Security Mode       |  6 |            |  Single   |    Yes    | Integer |  0-255  |       |
 *  SMS Binding Key Param.  |  7 |            |  Single   |    Yes    | Opaque  |   6 B   |       |
 *  SMS Binding Secret Keys |  8 |            |  Single   |    Yes    | Opaque  | 32-48 B |       |
 *  Server SMS Number       |  9 |            |  Single   |    Yes    | Integer |         |       |
 *  Short Server ID         | 10 |            |  Single   |    No     | Integer | 1-65535 |       |
 *  Client Hold Off Time    | 11 |            |  Single   |    Yes    | Integer |         |   s   |
 *
 */

/*
 * Here we implement a very basic LWM2M Security Object which only knows NoSec security mode.
 */

#include "lwm2m_security.h"

#include "miso_config.h"
#include "mbedtls/base64.h"


typedef struct _security_instance_
{
    struct _security_instance_ * next;        // matches lwm2m_list_t::next
    uint16_t                     instanceId;  // matches lwm2m_list_t::id
    char                         uri[256];    // Server URI (255 Bytes) as COAP URI
    bool                         isBootstrap; // Is bootstrap
    uint32_t					 security_mode;

    char                         public_identity[128]; // rfc4279
    size_t  					 public_identity_len;

    uint8_t *                    server_identity;
    size_t                       server_identity_len;

    uint8_t                      secret_key[64]; // rfc4279
    size_t                       secret_key_len;

    uint16_t                     shortID;
    uint32_t                     clientHoldOffTime;
} security_instance_t;





static uint8_t prv_get_value(lwm2m_data_t * dataP,
                             security_instance_t * targetP)
{
	uint8_t ret = COAP_404_NOT_FOUND;

    switch (dataP->id)
    {
    case lwm2m_security_uri:
        lwm2m_data_encode_string(targetP->uri, dataP);
        ret = COAP_205_CONTENT;
        break;
    case lwm2m_security_bootstrap:
        lwm2m_data_encode_bool(targetP->isBootstrap, dataP);
        ret = COAP_205_CONTENT;
        break;
    case lwm2m_security_mode:
    	lwm2m_data_encode_int((int64_t)targetP->security_mode, dataP);
        ret = COAP_205_CONTENT;
        break;
    case lwm2m_security_own_public_identity:
    	if(targetP->public_identity_len > 0)
    	{
    		lwm2m_data_encode_opaque((uint8_t *)targetP->public_identity, targetP->public_identity_len, dataP);
    		ret = COAP_205_CONTENT;
    	}
    	else
    	{
      		uint8_t value_zero = 0;
       		lwm2m_data_encode_opaque(&value_zero, 1, dataP);
       		ret =  COAP_205_CONTENT;
    	}
        break;
    case lwm2m_security_server_public_key:
      	if(targetP->server_identity_len > 0)
       	{
       		lwm2m_data_encode_opaque(targetP->server_identity, targetP->server_identity_len, dataP);
       		ret =  COAP_205_CONTENT;
       	}
      	else
      	{
      		uint8_t value_zero = 0;
       		lwm2m_data_encode_opaque(&value_zero, 1, dataP);
       		ret =  COAP_205_CONTENT;
      	}
      	break;
    case lwm2m_security_own_secret_key:
        // Here we return an opaque of 1 byte containing 0
    	if(targetP->secret_key_len > 0)
    	{
    		lwm2m_data_encode_opaque(targetP->secret_key, targetP->secret_key_len, dataP);
    		ret = COAP_205_CONTENT;
    	}
    	else
    	{
    		uint8_t value_zero = 0;
    		lwm2m_data_encode_opaque(&value_zero, 1, dataP);
    		ret = COAP_205_CONTENT;
    	}
    	break;
    case lwm2m_security_sms_security_mode:
        lwm2m_data_encode_int(LWM2M_SECURITY_MODE_NONE, dataP);
        return COAP_205_CONTENT;


    case LWM2M_SECURITY_SMS_KEY_PARAM_ID:
        // Here we return an opaque of 6 bytes containing a buggy value
        {
            char * value = "12345";
            lwm2m_data_encode_opaque((uint8_t *)value, 6, dataP);
        }
        return COAP_205_CONTENT;

    case LWM2M_SECURITY_SMS_SECRET_KEY_ID:
        // Here we return an opaque of 32 bytes containing a buggy value
        {
            char * value = "1234567890abcdefghijklmnopqrstu";
            lwm2m_data_encode_opaque((uint8_t *)value, 32, dataP);
        }
        return COAP_205_CONTENT;

    case LWM2M_SECURITY_SMS_SERVER_NUMBER_ID:
        lwm2m_data_encode_int(0, dataP);
        return COAP_205_CONTENT;

    case LWM2M_SECURITY_SHORT_SERVER_ID:
        lwm2m_data_encode_int(targetP->shortID, dataP);
        return COAP_205_CONTENT;

    case LWM2M_SECURITY_HOLD_OFF_ID:
        lwm2m_data_encode_int(targetP->clientHoldOffTime, dataP);
        return COAP_205_CONTENT;

    default:
        ret = COAP_404_NOT_FOUND;
    }

    return ret;
}

static uint8_t prv_security_read(lwm2m_context_t * contextP,
                                 uint16_t instanceId,
                                 int * numDataP,
                                 lwm2m_data_t ** dataArrayP,
                                 lwm2m_object_t * objectP)
{
    security_instance_t * targetP;
    uint8_t result;
    int i;

    /* Unused parameter */
    (void)contextP;

    targetP = (security_instance_t *)lwm2m_list_find(objectP->instanceList, instanceId);
    if (NULL == targetP) return COAP_404_NOT_FOUND;

    // is the server asking for the full instance ?
    if (*numDataP == 0)
    {
        uint16_t resList[] = {LWM2M_SECURITY_URI_ID,
                              LWM2M_SECURITY_BOOTSTRAP_ID,
                              LWM2M_SECURITY_SECURITY_ID,
                              LWM2M_SECURITY_PUBLIC_KEY_ID,
                              LWM2M_SECURITY_SERVER_PUBLIC_KEY_ID,
                              LWM2M_SECURITY_SECRET_KEY_ID,
                              LWM2M_SECURITY_SMS_SECURITY_ID,
                              LWM2M_SECURITY_SMS_KEY_PARAM_ID,
                              LWM2M_SECURITY_SMS_SECRET_KEY_ID,
                              LWM2M_SECURITY_SMS_SERVER_NUMBER_ID,
                              LWM2M_SECURITY_SHORT_SERVER_ID,
                              LWM2M_SECURITY_HOLD_OFF_ID};
        int nbRes = sizeof(resList)/sizeof(uint16_t);

        *dataArrayP = lwm2m_data_new(nbRes);
        if (*dataArrayP == NULL) return COAP_500_INTERNAL_SERVER_ERROR;
        *numDataP = nbRes;
        for (i = 0 ; i < nbRes ; i++)
        {
            (*dataArrayP)[i].id = resList[i];
        }
    }

    i = 0;
    do
    {
        if ((*dataArrayP)[i].type == LWM2M_TYPE_MULTIPLE_RESOURCE)
        {
            result = COAP_404_NOT_FOUND;
        }
        else
        {
            result = prv_get_value((*dataArrayP) + i, targetP);
        }
        i++;
    } while (i < *numDataP && result == COAP_205_CONTENT);

    return result;
}

lwm2m_object_t * get_security_object()
{
    lwm2m_object_t * securityObj;

    securityObj = (lwm2m_object_t *)lwm2m_malloc(sizeof(lwm2m_object_t));

    if (NULL != securityObj)
    {
        security_instance_t * targetP;

        memset(securityObj, 0, sizeof(lwm2m_object_t));

        securityObj->objID = 0;
        securityObj->versionMajor = 1;
        securityObj->versionMinor = 1;

        // Manually create an hardcoded instance
        targetP = (security_instance_t *)lwm2m_malloc(sizeof(security_instance_t));
        if (NULL == targetP)
        {
            lwm2m_free(securityObj);
            return NULL;
        }

        memset(targetP, 0, sizeof(security_instance_t));
        targetP->instanceId = 0;

        strncpy(targetP->uri, config_get_lwm2m_uri(), strlen(config_get_lwm2m_uri()));
        strncpy(targetP->public_identity, config_get_lwm2m_psk_id(), strlen(config_get_lwm2m_psk_id()));
        targetP->public_identity_len = strlen(targetP->public_identity) + 1;
        mbedtls_base64_decode((unsigned char *)&(targetP->secret_key), sizeof(targetP->secret_key), &(targetP->secret_key_len), config_get_lwm2m_psk_key(), strlen(config_get_lwm2m_psk_key()));

        targetP->isBootstrap = false;
        targetP->shortID = 123;
        targetP->clientHoldOffTime = 10;
        targetP->security_mode = (uint32_t)security_mode_psk;

        securityObj->instanceList = LWM2M_LIST_ADD(securityObj->instanceList, targetP);

        securityObj->readFunc = prv_security_read;
#ifdef LWM2M_BOOTSTRAP
        securityObj->writeFunc = prv_security_write;
        securityObj->createFunc = prv_security_create;
        securityObj->deleteFunc = prv_security_delete;
#endif

    }

    return securityObj;
}

void free_security_object(lwm2m_object_t * objectP)
{
    while (objectP->instanceList != NULL)
    {
        security_instance_t * securityInstance = (security_instance_t *)objectP->instanceList;
        objectP->instanceList = objectP->instanceList->next;

        lwm2m_free(securityInstance);
    }
    lwm2m_free(objectP);
}

char * get_server_uri(lwm2m_object_t * objectP,
                      uint16_t secObjInstID)
{
    security_instance_t * targetP = (security_instance_t *)LWM2M_LIST_FIND(objectP->instanceList, secObjInstID);

    if (NULL != targetP)
    {
        return lwm2m_strdup(targetP->uri);
    }

    return NULL;
}

unsigned char * get_connection_psk(lwm2m_object_t * objectP, uint16_t secObjInstID, size_t * psk_len)
{
	uint8_t * psk = NULL;
    security_instance_t * targetP = (security_instance_t *)LWM2M_LIST_FIND(objectP->instanceList, secObjInstID);

    if (NULL != targetP)
    {
        psk = (unsigned char*)targetP->secret_key;
        *psk_len = targetP->secret_key_len;
    }

	return psk;
}

unsigned char * get_public_identiy(lwm2m_object_t * objectP, uint16_t secObjInstID, size_t * public_identity_len)
{
	uint8_t * psk = NULL;
    security_instance_t * targetP = (security_instance_t *)LWM2M_LIST_FIND(objectP->instanceList, secObjInstID);

    if (NULL != targetP)
    {
        psk = (unsigned char*)targetP->public_identity;
        *public_identity_len = targetP->public_identity_len;
    }

	return psk;
}

enum lwm2m_security_mode_e get_security_mode(lwm2m_object_t *objectP, uint16_t secObjInstID)
{
	enum lwm2m_security_mode_e security_mode = security_mode_no_sec;

	 security_instance_t * targetP = (security_instance_t *)LWM2M_LIST_FIND(objectP->instanceList, secObjInstID);
	 if(NULL != targetP)
	 {
		 security_mode = (enum lwm2m_security_mode_e)targetP->security_mode;
	 }

	return security_mode;
}
