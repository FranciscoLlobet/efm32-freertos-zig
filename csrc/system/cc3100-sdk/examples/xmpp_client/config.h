/*
 * config.h - xmpp application configuration file
 *
 * Copyright (C) 2014 Texas Instruments Incorporated - http://www.ti.com/
 *
 *
 * All rights reserved. Property of Texas Instruments Incorporated.
 * Restricted rights to use, duplicate or disclose this code are
 * granted through contract.
 *
 * The program may not be used without the written permission of
 * Texas Instruments Incorporated or against the terms and conditions
 * stipulated in the agreement under which this program has been supplied,
 * and under no circumstances can it be used with non-TI connectivity device.
 *
 */

#ifndef DEMO_CONFIG_H
#define DEMO_CONFIG_H

#ifdef __cplusplus
extern "C"{
#endif /* __cplusplus */

/* Xmpp login credentials */

/* Username and password length should not exceed 32 characters */
#define XMPP_USER       "<username>"
#define XMPP_PWD        "<password>"
#define XMPP_DOMAIN     "gmail.com"
#define XMPP_RESOURCE   "work"

/* IP addressed of XMPP server. Should be in long format,
 * E.g: 0x40e9a77d == 64.233.167.125 */
#define XMPP_IP_ADDR    0x40e9a77d
#define XMPP_DST_PORT   5223

#define MAX_RCV_BUF_SIZE 250+1

/* Application specific status/error codes */
typedef enum{
    DEVICE_NOT_IN_STATION_MODE = -0x7D0,        /* Choosing this number to avoid overlap w/ host-driver's error codes */
    XMPP_SET_INVALID_APP_ID = DEVICE_NOT_IN_STATION_MODE - 1,
    XMPP_SET_INVALID_CASE = XMPP_SET_INVALID_APP_ID - 1,
    SERVER_INFO_INVALID_RESPONSE = XMPP_SET_INVALID_CASE - 1,
    QUERY_RESULT_INVALID_RESPONSE = SERVER_INFO_INVALID_RESPONSE - 1,
    BIND_FEATURE_INVALID_RESPONSE = QUERY_RESULT_INVALID_RESPONSE - 1,
    BIND_CONFIG_INVALID_RESPONSE = BIND_FEATURE_INVALID_RESPONSE - 1,
    ROSTER_INVALID_RESPONSE = BIND_CONFIG_INVALID_RESPONSE - 1,
    SESSION_CONFIG_INVALID_RESPONSE = ROSTER_INVALID_RESPONSE - 1,
    XMPP_SEND_ERROR = SESSION_CONFIG_INVALID_RESPONSE - 1,
    XMPP_RECV_ERROR = XMPP_SEND_ERROR - 1,
    XMPP_INVALID_RESPONSE = XMPP_RECV_ERROR - 1,
    CONNECTION_SM_INACTIVE_XMPP = XMPP_INVALID_RESPONSE - 1,
    XMPP_CONNECTION_ERROR = CONNECTION_SM_INACTIVE_XMPP - 1,

    STATUS_CODE_MAX = -0xBB8
}e_AppStatusCodes;

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* DEMO_CONFIG_H */
