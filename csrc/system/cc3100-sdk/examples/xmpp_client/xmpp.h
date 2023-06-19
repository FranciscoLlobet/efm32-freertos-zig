/*
 * xmpp.h
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

#ifndef __XMPP_H__
#define __XMPP_H__

#ifdef __cplusplus
extern "C" {
#endif

/* NetApp Xmpp IDs */
#define SL_NET_APP_XMPP_ID           (4)

/* NetApp XMPP set/get options */
#define NETAPP_XMPP_ADVANCED_OPT     (1)
#define NETAPP_XMPP_USER_NAME        (2)
#define NETAPP_XMPP_PASSWORD         (3)
#define NETAPP_XMPP_DOMAIN           (4)
#define NETAPP_XMPP_RESOURCE         (5)
#define NETAPP_XMPP_ROSTER           (6)

#define SO_SECMETHOD_SSLV3           0  /* security metohd SSL v3 */
#define SECURE_MASK_SSL_RSA_WITH_RC4_128_SHA                    (1 << 0)
#define SECURE_MASK_SSL_RSA_WITH_RC4_128_MD5                    (1 << 1)

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
}SlNetAppXmppOpt_t;

typedef struct
{
    _u8 UserName[32];
    _u8 Length;
}SlNetAppXmppUserName_t;

typedef struct
{
    _u8 Password[32];
    _u8 Length;
}SlNetAppXmppPassword_t;

typedef struct
{
    _u8 DomainName[32];
    _u8 Length;
}SlNetAppXmppDomain_t;

typedef struct
{
    _u8 Resource[32];
    _u8 Length;
}SlNetAppXmppResource_t;


/*!
    \brief          A function for setting XMPP configurations

    \return         On success, zero is returned. On error, -1 is
                    returned

    \param[in]      AppId -  application id, should be SL_NET_APP_XMPP_ID

    \param[in]      SetOptions - set option, could be one of the following:
                            NETAPP_XMPP_ADVANCED_OPT,\n
                            NETAPP_XMPP_USER_NAME,\n
                            NETAPP_XMPP_PASSWORD,\n
                            NETAPP_XMPP_DOMAIN,\n
                            NETAPP_XMPP_RESOURCE,\n
                            NETAPP_XMPP_ROSTER\n

    \param[in]  OptionLen - option structure length

    \param[in]      pOptionValues -   pointer to the option structure

    \sa

    \note

    \warning
*/
_i32 sl_NetAppXmppSet(_u8 AppId ,_u8 Option ,
        _u8 OptionLen, _u8 *pOptionValue);


/*!
    \brief          Initiates the XMPP connection process

    Connect to an XMPP server using the all the predefined XMPP parameters.

    \return         On success, positive is returned.
                    On error, negative is returned

    \sa
    \note
    \warning
*/
_i32 sl_NetAppXmppConnect();


/*!
    \brief          Retrieve XMPP message


    \param[out]     pRemoteJid -  The remote user ID that sent us the XMPP
                                  message

    \param[in]      Jidlen - The size of the pRemoteJid buffer

    \param[out]     pMessage - The buffer which will hold the retrieved message

    \param[in]      Msglen - The maximal size of the message buffer


    \return         On success, positive is returned.
                    -1 - error, General error, problem in parsing XMPP buffer
                    -2 - error, Jid buffer too small to hold the remote user id
                    -3 - error, Message buffer too small to hold the received
                                message
    \sa
    \note
    \warning
*/
_i32 sl_NetAppXmppRecv(_u8* pRemoteJid, _u16 Jidlen, _u8* pMessage,
                                                            _u16 Msglen);


/*!
    \brief          Send a XMPP message

    \param[in]      pRemoteJid -  The remote user ID that sent us the XMPP
                                  message

    \param[in]      Jidlen - The size of the pRemoteJid buffer

    \param[in]      pMessage - The buffer which will hold the retrieved message

    \param[in]      Msglen - The maximal size of the message buffer


    \return         On success, positive is returned.
                    On error, negative is returned
    \sa
    \note
    \warning        The Send function doesn't check the Jid validity or its'
                    status (online, off line, etc.)
*/
_i32 sl_NetAppXmppSend(_u8* pRemoteJid, _u16 Jidlen, _u8* pMessage,
                                                            _u16 Msglen);



#ifdef  __cplusplus
}
#endif /* __cplusplus */

#endif /* __NETAPP_H__ */

