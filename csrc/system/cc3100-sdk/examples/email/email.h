/*
 * email.h - email header file
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

#include "simplelink.h"

#ifndef __EMAIL_H__
#define __EMAIL_H__

#ifdef __cplusplus
extern "C" {
#endif

/* SMTP defines */
#define SMTP_BUF_LEN               100

/* NetApp Email set/get options */
#define NETAPP_ADVANCED_OPT        (1)
#define NETAPP_SOURCE_EMAIL        (2)
#define NETAPP_PASSWORD            (3)
#define NETAPP_DEST_EMAIL          (4)
#define NETAPP_SUBJECT             (5)
#define NETAPP_MESSAGE             (6)

#define MAX_DEST_EMAIL_LEN         30
#define MAX_USERNAME_LEN           30
#define MAX_PASSWORD_LEN           30
#define MAX_SUBJECT_LEN            30
#define MAX_MESSAGE_LEN            64
#define BASEKEY_LEN                128

#define MAX_EMAIL_RCF_LEN        (MAX_DEST_EMAIL_LEN + 2)

/* NetApp Email protocol types */
#define SL_NET_APP_SMTP_PROTOCOL    (1)
typedef struct
{
    _u32  ProtocolSubType;
    _u32  Port;
    _u32  Family;
    _u32  SecurityMethod;
    _u32  SecurityCypher;
    _u32  Ip;                     /* IPv4 address or IPv6 first 4 bytes */
    _u32  Ip1OrPaadding;
    _u32  Ip2OrPaadding;
    _u32  Ip3OrPaadding;
}SlNetAppEmailOpt_t;

typedef struct
{
    _u8 Username[MAX_USERNAME_LEN];
}SlNetAppSourceEmail_t;

typedef struct
{
    _u8 Password[MAX_PASSWORD_LEN];
}SlNetAppSourcePassword_t;

typedef struct
{
    _u8 Email[MAX_DEST_EMAIL_LEN];
}SlNetAppDestination_t;

typedef struct
{
    _u8 Value[MAX_SUBJECT_LEN];
}SlNetAppEmailSubject_t;


/*!
    \brief          This function handles WLAN events

    \param[in]      command -   Command sent for processing
    \param[in]      pValueLen - Length of data to be processed
    \param[in]      pValue -    Data to be processed

    \return         0 for success, -1 otherwise

    \note

    \warning
*/
_i32 sl_NetAppEmailSet(_u8 command, _u8 pValueLen,
                      _u8 *pValue);

/*!
    \brief          Create a secure socket and connects to SMTP server

    \param[in]      none

    \return         0 if success and negative in case of error

    \note

    \warning
*/
_i32 sl_NetAppEmailConnect();

/*!
    \brief          Checks the connection status and sends the Email

    \param[in]      none

    \return         0 if success otherwise -1

    \note

    \warning
*/
_i32 sl_NetAppEmailSend();


#ifdef  __cplusplus
}
#endif /* __cplusplus */

#endif /* __NETAPP_H__ */

