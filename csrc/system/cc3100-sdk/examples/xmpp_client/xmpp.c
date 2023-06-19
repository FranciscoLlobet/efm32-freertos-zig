/*
 * xmpp.c - function implementation to communicate with xmpp server
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

/*
 * Protocol Name     - Extensive Messaging and Presence Protocol
 * Protocol Overview - The Extensible Messaging and Presence Protocol (XMPP)
 *                     is an application profile of the Extensible Markup
 *                     Language [XML] that    enables the near-real-time exchange
 *                     of structured yet extensible data between any two or
 *                     more network entities.
 *                     The process whereby a client connects to a server,
 *                     exchanges XML stanzas, and ends the connection is:
 *
 *                   1.  Determine the IP address and port at which to connect, typically
 *                       based on resolution of a fully qualified domain name
 *                   2.  Open a Transmission Control Protocol [TCP] connection
 *                   3.  Open an XML stream over TCP
 *                   4.  Preferably negotiate Transport Layer Security [TLS] for channel
 *                       encryption
 *                   5.  Authenticate using a Simple Authentication and Security Layer
 *                       [SASL] mechanism
 *                   6.  Bind a resource to the stream
 *                   7.  Exchange an unbounded number of XML stanzas with other entities
 *                       on the network
 *                   8.  Close the XML stream
 *                   9.  Close the TCP connection
 *
 * Sample Streams
 *   +--------------------+--------------------+
 *   | INITIAL STREAM     |  RESPONSE STREAM   |
 *   +--------------------+--------------------+
 *   | <stream>           |                    |
 *   |--------------------|--------------------|
 *   |                    | <stream>           |
 *   |--------------------|--------------------|
 *   | <presence>         |                    |
 *   |   <show/>          |                    |
 *   | </presence>        |                    |
 *   |--------------------|--------------------|
 *   | <message to='foo'> |                    |
 *   |   <body/>          |                    |
 *   | </message>         |                    |
 *   |--------------------|--------------------|
 *   | <iq to='bar'       |                    |
 *   |     type='get'>    |                    |
 *   |   <query/>         |                    |
 *   | </iq>              |                    |
 *   |--------------------|--------------------|
 *   |                    | <iq from='bar'     |
 *   |                    |     type='result'> |
 *   |                    |   <query/>         |
 *   |                    | </iq>              |
 *   |--------------------|--------------------|
 *   | [ ... ]            |                    |
 *   |--------------------|--------------------|
 *   |                    | [ ... ]            |
 *   |--------------------|--------------------|
 *   | </stream>          |                    |
 *   |--------------------|--------------------|
 *   |                    | </stream>          |
 *   +--------------------+--------------------+
 *
 * Refer     1. http: *www.rfc-editor.org/rfc/rfc6120.txt
 *              2. http://www.rfc-editor.org/rfc/rfc6121.txt
*/

#include "xmpp.h"
#include "base64.h"
#include "config.h"
#include "sl_common.h"


#define SO_SECMETHOD    25  /* security method */
#define SO_SECURE_MASK  26  /* security mask 0 and mask 1 */
#define SL_SECURITY_ANY     (100)

#define PRESENCE_MESSAGE "<presence><priority>4</priority><status>Online</status><c xmlns='http://jabber.org/protocol/caps' node='http://jajc.jrudevels.org/caps' ver='0.0.8.125 (04.01.2012)'/></presence>"
#define JABBER_XMLNS_INFO "version='1.0' xmlns:stream='http://etherx.jabber.org/streams' xmlns='jabber:client'>"

#define XMPP_STATUS_SOCKET     (1 << 0)
#define XMPP_STATUS_USERNAME   (1 << 1)
#define XMPP_STATUS_PASSWORD   (1 << 2)
#define XMPP_STATUS_DOMAIN     (1 << 3)
#define XMPP_STATUS_RESOURCE   (1 << 4)

typedef enum
{
    XMPP_INACTIVE = 0,
    XMPP_INIT,
    FIRST_STREAM_SENT,
    FIRST_STREAM_RECV,
    STARTTLS_RESPONSE_RECV,
    AUTH_QUERY_SET,
    AUTH_RESULT_RECV,
    BIND_FEATURE_REQUEST,
    BIND_FEATURE_RESPONSE,
    BIND_CONFIG_SET,
    BIND_CONFIG_RECV,
    XMPP_SESSION_SET,
    XMPP_SESSION_RECV,
    PRESENCE_SET,
    CONNECTION_ESTABLISHED,
    ROSTER_REQUEST,
    ROSTER_RESPONSE
}_SlXMPPStatus_e;

#define MAX_BUFF_SIZE 1024
#define BUF_SIZE 128

_u16 g_XMPPStatus = XMPP_INIT;

union
{
    _u8 Buf[2*MAX_BUFF_SIZE];
    _u32 demobuf[MAX_BUFF_SIZE/2];
} g_RecvBuf;

union
{
    _u8 Buf[MAX_BUFF_SIZE/2];
    _u32 demobuf[MAX_BUFF_SIZE/8];
} g_SendBuf;

union
{
    _u8 Buf[BUF_SIZE];
    _u32 demobuf[BUF_SIZE/4];
} g_MyBaseKey;

union
{
    _u8 Buf[BUF_SIZE];
    _u32 demobuf[BUF_SIZE/4];
} MyJid;

union
{
    _u8 Buf[BUF_SIZE];
    _u32 demobuf[BUF_SIZE/4];
} RemoteJid;

union
{
    _u8 Buf[BUF_SIZE];
    _u32 demobuf[BUF_SIZE/4];
} g_RosterJid;

union
{
    SlNetAppXmppOpt_t Buf;
    _u32 demobuf[9];
} g_XmppOpt;

union
{
    SlNetAppXmppUserName_t Buf;
    _u32 demobuf[9];
} g_XmppUserName;

union
{
    SlNetAppXmppPassword_t Buf;
    _u32 demobuf[9];
} g_XmppPassword;

union
{
    SlNetAppXmppDomain_t Buf;
    _u32 demobuf[9];
} g_XmppDomain;

union
{
    SlNetAppXmppResource_t Buf;
    _u32 demobuf[9];
} g_XmppResource;

_i16           g_SockID;
_u16    g_XmppSetStatus = 0;

/* Static function declarations */
static _i32 SlXmppConnectionSM(void);
static _i32 SlValidateServerInfo(void);
static _i32 SlValidateQueryResult(void);
static _i32 SlValidateBindFeature(void);
static _i32 SlBindingConfigure(void);
static _i32 SlValidateRoster(void);
static _i32 SlXMPPSessionConfig(void);

/*!
    \brief     A function for setting XMPP configurations

    \return    On success, zero is returned. On error, -1 is
               returned

    \param[in] AppId -  application id, should be SL_NET_APP_XMPP_ID

    \param[in] SetOptions - set option, could be one of the following: \n
                            NETAPP_XMPP_ADVANCED_OPT,\n
                            NETAPP_XMPP_USER_NAME,\n
                            NETAPP_XMPP_PASSWORD,\n
                            NETAPP_XMPP_DOMAIN,\n
                            NETAPP_XMPP_RESOURCE,\n
                            NETAPP_XMPP_ROSTER\n

    \param[in] OptionLen - option structure length

    \param[in] pOptionValues -   pointer to the option structure

    \sa

    \note

    \warning
*/
_i32 sl_NetAppXmppSet(_u8 AppId ,_u8 Option,
                       _u8 OptionLen, _u8 *pOptionValue)
{
    SlNetAppXmppOpt_t* pXmppOpt = 0;
    SlNetAppXmppUserName_t* pXmppUserName = 0;
    SlNetAppXmppPassword_t* pXmppPassword = 0;
    SlNetAppXmppDomain_t* pXmppDomain = 0;
    SlNetAppXmppResource_t* pXmppResource = 0;

    if (AppId != SL_NET_APP_XMPP_ID)
    {
        ASSERT_ON_ERROR(XMPP_SET_INVALID_APP_ID);
    }

    switch (Option)
    {
      case NETAPP_XMPP_ADVANCED_OPT:
        pXmppOpt = (SlNetAppXmppOpt_t*)pOptionValue;

        g_XmppOpt.Buf.Port = pXmppOpt->Port;
        g_XmppOpt.Buf.Family = pXmppOpt->Family;
        g_XmppOpt.Buf.SecurityMethod = pXmppOpt->SecurityMethod;
        g_XmppOpt.Buf.SecurityCypher = pXmppOpt->SecurityCypher;
        g_XmppOpt.Buf.Ip = pXmppOpt->Ip;

        g_XmppSetStatus+=XMPP_STATUS_SOCKET;
        break;

      case NETAPP_XMPP_USER_NAME:
        pXmppUserName = (SlNetAppXmppUserName_t*)pOptionValue;

        pal_Memcpy(g_XmppUserName.Buf.UserName, pXmppUserName->UserName, OptionLen);
        g_XmppUserName.Buf.Length = OptionLen;

        g_XmppSetStatus+=XMPP_STATUS_USERNAME;
        break;

      case NETAPP_XMPP_PASSWORD:
        pXmppPassword = (SlNetAppXmppPassword_t*)pOptionValue;

        pal_Memcpy(g_XmppPassword.Buf.Password, pXmppPassword->Password, OptionLen);
        g_XmppPassword.Buf.Length = OptionLen;

        g_XmppSetStatus+=XMPP_STATUS_PASSWORD;
        break;

      case NETAPP_XMPP_DOMAIN:
        pXmppDomain = (SlNetAppXmppDomain_t*)pOptionValue;

        pal_Memcpy(g_XmppDomain.Buf.DomainName, pXmppDomain->DomainName, OptionLen);
        g_XmppDomain.Buf.Length = OptionLen;

        g_XmppSetStatus+=XMPP_STATUS_DOMAIN;
        break;

      case NETAPP_XMPP_RESOURCE:
        pXmppResource = (SlNetAppXmppResource_t*)pOptionValue;
        pal_Memcpy(g_XmppResource.Buf.Resource, pXmppResource->Resource, OptionLen);
        g_XmppResource.Buf.Length = OptionLen;
        g_XmppSetStatus+=XMPP_STATUS_RESOURCE;
        break;
      default:
          ASSERT_ON_ERROR(XMPP_SET_INVALID_CASE);
    }

    return SUCCESS;
}

/*!
    \brief Initiates the XMPP connection process

    Connect to an XMPP server using all the predefined XMPP parameters.

    \return         On success, positive is returned.
                    On error, negative is returned

    \sa
    \note
    \warning
*/
_i32 sl_NetAppXmppConnect()
{
    SlSockAddrIn_t  Addr;
    _i16            AddrSize = 0;
    _i32            Status = 0;
    _u8             method = 0;
    _i32            cipher = 0;

    if (g_XmppSetStatus != 0x1F)
    {
        ASSERT_ON_ERROR(XMPP_CONNECTION_ERROR);
    }

    method = g_XmppOpt.Buf.SecurityMethod;
    cipher = g_XmppOpt.Buf.SecurityCypher;

    Addr.sin_family = g_XmppOpt.Buf.Family;
    Addr.sin_port = sl_Htons(g_XmppOpt.Buf.Port);
    Addr.sin_addr.s_addr = sl_Htonl(g_XmppOpt.Buf.Ip);

    AddrSize = sizeof(SlSockAddrIn_t);

    /* opens a secure socket */
    g_SockID = sl_Socket(SL_AF_INET,SL_SOCK_STREAM, SL_SECURITY_ANY);
    ASSERT_ON_ERROR(g_SockID);

    /*configure the socket security method */
    Status = sl_SetSockOpt(g_SockID, SL_SOL_SOCKET, SO_SECMETHOD,
                           &method, sizeof(method));
    if( Status < 0 )
    {
        sl_Close(g_SockID);
        ASSERT_ON_ERROR(Status);
    }

     /*configure the socket cipher */
    Status = sl_SetSockOpt(g_SockID, SL_SOL_SOCKET, SO_SECURE_MASK,
                           &cipher, sizeof(cipher));
    if( Status < 0 )
    {
        sl_Close(g_SockID);
        ASSERT_ON_ERROR(Status);
    }

    /* connect to the peer device */
    Status = sl_Connect(g_SockID, ( SlSockAddr_t *)&Addr, AddrSize);
    if( (Status < 0 ) && (SL_ESECSNOVERIFY != Status))
    {
        sl_Close(g_SockID);
        ASSERT_ON_ERROR(Status);
    }

    Status = SlXmppConnectionSM();

    return Status;
}

/*!
    \brief Validate the server

    \param[in]      none

    \return         0 for success, -1 otherwise

    \sa
    \note
    \warning
*/
static _i32 SlValidateServerInfo(void)
{
    _u8 *pServerStr = 0;

    if(!pal_Strstr(g_RecvBuf.Buf, "stream:stream"))
    {
        return SERVER_INFO_INVALID_RESPONSE;
    }

    pServerStr = (_u8 *)pal_Strstr(g_RecvBuf.Buf, "from=");
    if(pal_Strncmp(pServerStr+6,
               g_XmppDomain.Buf.DomainName, g_XmppDomain.Buf.Length))
    {
        ASSERT_ON_ERROR(SERVER_INFO_INVALID_RESPONSE);
    }

    return SUCCESS;
}

/*!
    \brief Validate the authentication query response

    \param[in]      none

    \return         0 for success, -1 otherwise

    \sa
    \note
    \warning
*/
static _i32 SlValidateQueryResult(void)
{
    if(pal_Strstr(g_RecvBuf.Buf, "<success") == 0)
        ASSERT_ON_ERROR(QUERY_RESULT_INVALID_RESPONSE);

    return SUCCESS;
}

/*!
    \brief Validate the bind feature query response

    \param[in]      none

    \return         0 for success, -1 otherwise

    \sa
    \note
    \warning
*/
static _i32 SlValidateBindFeature(void)
{
    if(pal_Strstr(g_RecvBuf.Buf, "xmpp-bind") == 0)
    {
        ASSERT_ON_ERROR(BIND_FEATURE_INVALID_RESPONSE);
    }

    return SUCCESS;
}

/*!
    \brief Validate the bind configuration query response

    \param[in]      none

    \return         0 for success, -1 otherwise

    \sa
    \note
    \warning
*/
static _i32 SlBindingConfigure(void)
{
    _u8 *pJid = 0;
    _u8 *pMyJid = MyJid.Buf;
    _i16 idx = 0;

    if(!pal_Strstr(g_RecvBuf.Buf, "result")
            || !pal_Strstr(g_RecvBuf.Buf, "bind"))
    {
        ASSERT_ON_ERROR(BIND_CONFIG_INVALID_RESPONSE);
    }

    pJid = (_u8 *)pal_Strstr(g_RecvBuf.Buf, "<jid>");
    if(!pJid)
        ASSERT_ON_ERROR(BIND_CONFIG_INVALID_RESPONSE);

    pJid += 5;

    while(*pJid != '<')
    {
        pMyJid[idx++] = *pJid;
        pJid ++;
    }

    pMyJid[idx] = '\0';

    return SUCCESS;
}

/*!
    \brief Validate the roster request query response

    \param[in]      none

    \return         0 for success, -1 otherwise

    \sa
    \note
    \warning
*/
static _i32 SlValidateRoster(void)
{
    _u8 *pJid = 0;
    _u8 *pMyJid = g_RosterJid.Buf;
    _i16  idx = 0;

    if(!pal_Strstr(g_RecvBuf.Buf, "jabber:iq:roster") ||
       !pal_Strstr(g_RecvBuf.Buf, "roster_1"))
    {
        ASSERT_ON_ERROR(ROSTER_INVALID_RESPONSE);
    }

    pJid = (_u8 *)pal_Strstr(g_RecvBuf.Buf, "item jid=");
    while (pJid != NULL)
    {
        pJid += 10;

        while(*pJid != ' ')
        {
            pMyJid[idx++] = *pJid;
            pJid ++;
        }

        pMyJid[idx-1] = '\0';
        idx = 0;
        pJid = (_u8 *)pal_Strstr(pJid, "item jid=");
    }

    return SUCCESS;
}

/*!
    \brief Validate the Session query response

    \param[in]      none

    \return         0 for success, -1 otherwise

    \sa
    \note
    \warning
*/
static _i32 SlXMPPSessionConfig(void)
{
    if(!pal_Strstr(g_RecvBuf.Buf, "result"))
    {
        ASSERT_ON_ERROR(SESSION_CONFIG_INVALID_RESPONSE);
    }

    return SUCCESS;
}

/*!
    \brief Send a XMPP message


    \param[in] pRemoteJid -  The remote user ID that sent us the XMPP message

    \param[in] Jidlen - The size of the pRemoteJid buffer

    \param[in] pMessage - The buffer which will hold the retreived message

    \param[in] Msglen - The maximal size of the message buffer


    \return         On success, positive is returned.
                    On error, negative is returned
    \sa
    \note
    \warning    The Send function doesn't check the Jid validity or its' status
                (on-line, off line, etc.)
*/
_i32 sl_NetAppXmppSend(_u8* pRemoteJid, _u16 Jidlen,
                      _u8* pMessage, _u16 Msglen)
{
    /*
     * <message
     *        to='romeo@example.net'
     *        from='juliet@example.com/balcony'
     *        type='chat'
     *        xml:lang='en'>
     *      <body>Wherefore art thou, Romeo?</body>
     *    </message>
     */
    _u8 *ccPtr = 0;
    _i32 Status = 0;

    pal_Memset(g_SendBuf.Buf, 0, sizeof(g_SendBuf.Buf));
    ccPtr = g_SendBuf.Buf;

    pal_Memcpy(ccPtr, "<message to='", 13);
    ccPtr += 13;

    pal_Memcpy(ccPtr, pRemoteJid, Jidlen);
    ccPtr += Jidlen;

    pal_Memcpy(ccPtr, "' type='chat' from='", 20);
    ccPtr += 20;

    pal_Memcpy(ccPtr, MyJid.Buf, pal_Strlen(MyJid.Buf));
    ccPtr += pal_Strlen(MyJid.Buf);

    pal_Memcpy(ccPtr, "'><body>", 8);
    ccPtr += 8;

    pal_Memcpy(ccPtr, pMessage, Msglen);
    ccPtr += Msglen;

    pal_Memcpy(ccPtr, "</body><active xmlns='http://jabber.org/protocol/chatstates'/></message>", 72);
    ccPtr += 72;

    *ccPtr= '\0';

    Status = sl_Send(g_SockID, g_SendBuf.Buf, pal_Strlen(g_SendBuf.Buf), 0);
    if(Status <= 0)
        ASSERT_ON_ERROR(XMPP_SEND_ERROR);

    return SUCCESS;
}

/*!
    \brief Retrieve XMPP message


    \param[out] pRemoteJid -  The remote user ID that sent us the XMPP message

    \param[in] Jidlen - The size of the pRemoteJid buffer

    \param[out] pMessage - The buffer which will hold the retreived message

    \param[in] Msglen - The maximal size of the message buffer


    \return         On success, positive is returned.
                    -1 - error, General error, problem in parsing XMPP buffer
                    -2 - error, Jid buffer too small to hold the remote user id
                    -3 - error, Message buffer too small to hold the received
                                message
    \sa
    \note
    \warning
*/
_i32 sl_NetAppXmppRecv(_u8* pRemoteJid, _u16 Jidlen,
                      _u8* pMessage, _u16 Msglen)
{
    _u8 *pBasePtr = 0;
    _u8 *pBodyPtr = 0;
    _u8 *pFromPtr = 0;
    _i16 nonBlocking = 1;
    _i32 Status = 0;

    Status = setsockopt(g_SockID, SOL_SOCKET, SO_NONBLOCKING,
                 &nonBlocking, sizeof(nonBlocking));
    ASSERT_ON_ERROR(Status);

    pal_Memset(g_RosterJid.Buf, 0, sizeof(g_RosterJid.Buf));
    pal_Memset(g_RecvBuf.Buf,'\0',sizeof(g_RecvBuf.Buf));

    Status = sl_Recv(g_SockID, g_RecvBuf.Buf, sizeof(g_RecvBuf.Buf), 0);
    if(Status <= 0)
        ASSERT_ON_ERROR(XMPP_RECV_ERROR);

    pBasePtr = (_u8 *)pal_Strstr(g_RecvBuf.Buf, "<body>");
    if(!pBasePtr)
    {
        ASSERT_ON_ERROR(XMPP_INVALID_RESPONSE);
    }

    pBasePtr += 6;
    pBodyPtr = pBasePtr;
    while(*pBodyPtr != '<')
    {
        pBodyPtr ++;
    }

    /* null the body string */
    *pBodyPtr = '\0';
    if (pal_Strlen(pBasePtr)+1 < Msglen)
    {
        pal_Memcpy(pMessage, pBasePtr, pal_Strlen(pBasePtr)+1);
    }
    else
    {
        ASSERT_ON_ERROR(XMPP_INVALID_RESPONSE);
    }

    pBodyPtr = (_u8 *)pal_Strstr(g_RecvBuf.Buf, "from=");

    /* Increment the pointer by 6 (from=") */
    pBodyPtr += 6;
    pFromPtr = pBodyPtr;
    while(*pBodyPtr != '"')
    {
        pBodyPtr ++;
    }

    /* null the body string */
    *pBodyPtr = '\0';

    if (pal_Strlen(pFromPtr)+1 < Jidlen)
    {
        pal_Memcpy(pRemoteJid, pFromPtr, pal_Strlen(pFromPtr)+1);
        pal_Memcpy(RemoteJid.Buf, pFromPtr, pal_Strlen(pFromPtr)+1);
        pal_Memcpy(g_RosterJid.Buf, RemoteJid.Buf, pal_Strlen(RemoteJid.Buf)+1);
    }
    else
    {
        ASSERT_ON_ERROR(XMPP_INVALID_RESPONSE);
    }


    return Status;
}

/*!
    \brief Flushes the xmpp receive buffer

    \param[in]      none

    \return         none

    \sa
    \note
    \warning
*/
void FlushSendRecvBuffer(void)
{
    pal_Memset(g_RecvBuf.Buf, '\0', sizeof(g_RecvBuf.Buf));
    g_RecvBuf.Buf[0] = '\0';

    pal_Memset(g_SendBuf.Buf, '\0', sizeof(g_SendBuf.Buf));
    g_SendBuf.Buf[0] = '\0';
}

/*!
    \brief Convert the string to Base64 format needed for authentication

    \param[in]      none

    \return         none

    \sa
    \note
    \warning
*/
void GenerateBase64Key(void)
{
    _u8 OneByteArray[1] = {0x00};
    _u8 InputStr[BUF_SIZE] = {'\0'};
    _u8 *pIn = (_u8 *)InputStr;
    _i16 length = 0;

    /* Generate Base64 Key. The original key is "username@domain" +
    *  '\0' + "username" + '\0' + "password" */
    pal_Memcpy(pIn, g_XmppUserName.Buf.UserName, g_XmppUserName.Buf.Length);
    pIn += g_XmppUserName.Buf.Length;

    pal_Memcpy(pIn, "@", 1);
    pIn ++;

    pal_Memcpy(pIn, g_XmppDomain.Buf.DomainName, g_XmppDomain.Buf.Length);
    pIn += g_XmppDomain.Buf.Length;

    pal_Memcpy(pIn, OneByteArray, 1);
    pIn ++;

    pal_Memcpy(pIn,  g_XmppUserName.Buf.UserName, g_XmppUserName.Buf.Length);
    pIn +=  g_XmppUserName.Buf.Length;

    pal_Memcpy(pIn, OneByteArray, 1);
    pIn ++;

    pal_Memcpy(pIn, g_XmppPassword.Buf.Password, g_XmppPassword.Buf.Length);
    length = g_XmppUserName.Buf.Length*2;
    length += 3 + g_XmppDomain.Buf.Length +  g_XmppPassword.Buf.Length;

    ConvertToBase64(g_MyBaseKey.Buf, (void *)InputStr, length);
}

/*!
    \brief XMPP connection station machine

    \param[in]      none

    \return         none

    \sa
    \note
    \warning
*/
static _i32 SlXmppConnectionSM(void)
{
    _i16 NeedToSend = 0;
    _i16 RecvRetVal = 0;
    _i16 NeedToReceive = 0;
    _u8 *ccPtr = 0;
    _i16 len = 0;

    /* 3 steps for XMPP connection establishment
            1) initialization
            2) authentication
            3) binding
    */

    pal_Memset(g_MyBaseKey.Buf, '\0', sizeof(g_MyBaseKey.Buf));
    pal_Memset(MyJid.Buf, '\0', sizeof(MyJid.Buf));
    pal_Memset(RemoteJid.Buf, '\0', sizeof(RemoteJid.Buf));
    pal_Memset(g_RosterJid.Buf, '\0', sizeof(g_RosterJid.Buf));

    if(g_XMPPStatus == XMPP_INACTIVE)
    {
        ASSERT_ON_ERROR(CONNECTION_SM_INACTIVE_XMPP);
    }

    /* encode the username, password and domain in Base64 */
    GenerateBase64Key();

    while(g_XMPPStatus != CONNECTION_ESTABLISHED)
    {
        FlushSendRecvBuffer();

        ccPtr = (_u8*)g_SendBuf.Buf;

        if(NeedToReceive == 1)
        {
            pal_Memset(g_RecvBuf.Buf, '\0', sizeof(g_RecvBuf.Buf));
            RecvRetVal = sl_Recv(g_SockID, g_RecvBuf.Buf,
                                 sizeof(g_RecvBuf.Buf), 0);
            if(RecvRetVal <= 0)
                ASSERT_ON_ERROR(XMPP_RECV_ERROR);
        }

        if(NeedToReceive == 1 && pal_Strlen(g_RecvBuf.Buf) <= 0)
            continue;

        switch(g_XMPPStatus)
        {
          case XMPP_INIT:
            /*
             *     <stream:stream
             *         xmlns='jabber:client'
             *         xmlns:stream='http://etherx.jabber.org/streams'
             *         to='example.com'
             *         version='1.0'>
             */
            pal_Memcpy(ccPtr, "<stream:stream to='", 19);
            ccPtr += 19;
            pal_Memcpy(ccPtr, g_XmppDomain.Buf.DomainName, g_XmppDomain.Buf.Length);
            ccPtr += g_XmppDomain.Buf.Length;
            pal_Memcpy(ccPtr, "' ", 2);
            ccPtr += 2;
            pal_Memcpy(ccPtr, JABBER_XMLNS_INFO, pal_Strlen(JABBER_XMLNS_INFO));
            ccPtr += pal_Strlen(JABBER_XMLNS_INFO);
            *ccPtr= '\0';

            NeedToSend = 1;
            NeedToReceive = 1;

            g_XMPPStatus = FIRST_STREAM_SENT;
            break;

          case FIRST_STREAM_SENT:
            if(RecvRetVal > 0 && SlValidateServerInfo() == SUCCESS)
            {
                NeedToReceive = 0;
                g_XMPPStatus = FIRST_STREAM_RECV;
            }
            else
            {
                NeedToReceive = 1;
            }
            break;

          case FIRST_STREAM_RECV:
            /*
             *  <stream:stream
             *      xmlns='jabber:client'
             *      xmlns:stream='http://etherx.jabber.org/streams'
             *      id='c2s_234'
             *      from='example.com'
             *      version='1.0'>
             */

            g_XMPPStatus = STARTTLS_RESPONSE_RECV;
            break;

          case STARTTLS_RESPONSE_RECV:
            /*
             *  <auth xmlns='urn:ietf:params:xml:ns:xmpp-sasl'
             *      mechanism='PLAIN'/>
             */
            pal_Memcpy(ccPtr, "<auth xmlns='urn:ietf:params:xml:ns:xmpp-sasl' mechanism='PLAIN'>", 65);
            ccPtr += 65;
            pal_Memcpy(ccPtr, g_MyBaseKey.Buf, pal_Strlen(g_MyBaseKey.Buf));
            ccPtr += pal_Strlen(g_MyBaseKey.Buf);
            pal_Memcpy(ccPtr, "</auth>", 7);
            ccPtr += 7;
            *ccPtr= '\0';

            NeedToSend = 1;
            NeedToReceive = 1;

            g_XMPPStatus = AUTH_QUERY_SET;
            break;

          case AUTH_QUERY_SET:
            if((RecvRetVal > 0) && (SlValidateQueryResult() == SUCCESS))
            {
                NeedToReceive = 0;

                g_XMPPStatus = AUTH_RESULT_RECV;
            }
            else
            {
                NeedToReceive = 1;
            }
            break;

          case AUTH_RESULT_RECV:
            pal_Memcpy(ccPtr, "<stream:stream to='", 19);
            ccPtr += 19;
            pal_Memcpy(ccPtr, g_XmppDomain.Buf.DomainName, g_XmppDomain.Buf.Length);
            ccPtr += g_XmppDomain.Buf.Length;
            pal_Memcpy(ccPtr, "' ", 2);
            ccPtr += 2;
            pal_Memcpy(ccPtr, JABBER_XMLNS_INFO, pal_Strlen(JABBER_XMLNS_INFO));
            ccPtr += pal_Strlen(JABBER_XMLNS_INFO);
            *ccPtr= '\0';

            NeedToSend = 1;
            NeedToReceive = 1;

            g_XMPPStatus = BIND_FEATURE_RESPONSE;
            break;

          case BIND_FEATURE_REQUEST:
            NeedToSend = 0;
            NeedToReceive = 0;

            g_XMPPStatus = BIND_FEATURE_RESPONSE;
            break;

          case BIND_FEATURE_RESPONSE:
            if(RecvRetVal > 0 && SlValidateBindFeature() == SUCCESS)
            {
                NeedToReceive = 0;
                g_XMPPStatus = BIND_CONFIG_SET;
            }
            else
            {
                NeedToReceive = 1;
            }
            break;

          case BIND_CONFIG_SET:
            /*
             * <iq type='set' id='bind_2'>
             *   <bind xmlns='urn:ietf:params:xml:ns:xmpp-bind'>
             *     <resource>someresource</resource>
             *   </bind>
             * </iq>
             */
            pal_Memcpy(ccPtr, "<iq id='JAJSBind' type='set'><bind xmlns='urn:ietf:params:xml:ns:xmpp-bind'><resource>", 86);
            ccPtr += 86;
            pal_Memcpy(ccPtr,g_XmppResource.Buf.Resource,g_XmppResource.Buf.Length);
            ccPtr += g_XmppResource.Buf.Length;
            pal_Memcpy(ccPtr, "</resource></bind></iq>", 23);
            ccPtr += 23;
            *ccPtr= '\0';

            NeedToSend = 1;
            NeedToReceive = 1;

            g_XMPPStatus = BIND_CONFIG_RECV;
            break;

          case BIND_CONFIG_RECV:
            if(RecvRetVal > 0 && SlBindingConfigure() == SUCCESS)
            {
                NeedToReceive = 0;

                g_XMPPStatus = XMPP_SESSION_SET;
            }
            else
            {
                NeedToSend = 0;
                NeedToReceive = 0;
                g_XMPPStatus = XMPP_INIT;
            }
            break;

          case XMPP_SESSION_SET:
            /*
             * <iq to='example.com'
             *     type='set'
             *     id='sess_1'>
             *   <session xmlns='urn:ietf:params:xml:ns:xmpp-session'/>
             * </iq>
             */
            pal_Memcpy(ccPtr, "<iq type='set' id='2'><session xmlns='urn:ietf:params:xml:ns:xmpp-session'/></iq>", 81);
            ccPtr += 81;
            *ccPtr= '\0';

            NeedToSend = 1;
            NeedToReceive = 1;

            g_XMPPStatus = XMPP_SESSION_RECV;
            break;

          case XMPP_SESSION_RECV:
            if(RecvRetVal > 0 && SlXMPPSessionConfig() == SUCCESS)
            {
                NeedToReceive = 0;
                g_XMPPStatus = PRESENCE_SET;
            }
            else
            {
                NeedToReceive = 1;
            }
            break;

          case PRESENCE_SET:

            NeedToSend = 1;
            NeedToReceive = 0;

            pal_Memcpy(ccPtr, PRESENCE_MESSAGE, pal_Strlen(PRESENCE_MESSAGE));
            ccPtr += pal_Strlen(PRESENCE_MESSAGE);
            *ccPtr= '\0';

            g_XMPPStatus = ROSTER_REQUEST;
            break;

          case ROSTER_REQUEST:
            /*
             * <iq from='juliet@example.com/balcony' type='get' id='roster_1'>
             *   <query xmlns='jabber:iq:roster'/>
             * </iq>
             */
            pal_Memcpy(ccPtr, "<iq type='get' id='roster_1' from='", 35);
            ccPtr += 35;
            pal_Memcpy(ccPtr, MyJid.Buf, pal_Strlen(MyJid.Buf));
            ccPtr += pal_Strlen(MyJid.Buf);
            pal_Memcpy(ccPtr, "'> <query xmlns='jabber:iq:roster'/></iq>", 50);
            ccPtr += 50;
            *ccPtr= '\0';

            NeedToSend = 1;
            NeedToReceive = 0;

            g_XMPPStatus = ROSTER_RESPONSE;

            break;

          case ROSTER_RESPONSE:
            NeedToSend = 1;
            NeedToReceive = 1;

            if (SlValidateRoster() == SUCCESS)
            {
                g_XMPPStatus = CONNECTION_ESTABLISHED;
                NeedToReceive = 0;
                NeedToSend = 0;
            }

            break;
        }

        if(NeedToSend ==  1)
        {
            NeedToSend = 0;
            len = pal_Strlen(g_SendBuf.Buf);

            if(len > 0)
            {
                if(sl_Send(g_SockID, g_SendBuf.Buf, len, 0) <= 0)
                    ASSERT_ON_ERROR(XMPP_SEND_ERROR);
            }
        }
    }
    pal_Memset(g_RecvBuf.Buf,'\0',sizeof(g_RecvBuf.Buf));

    return SUCCESS;
}
