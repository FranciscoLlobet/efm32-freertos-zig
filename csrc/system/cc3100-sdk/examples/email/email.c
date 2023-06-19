/*
 * email.c - function implementation to send email
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
 * Protocol Name     -  Simple Mail Transfer Protocol
 * Protocol Overview -  The objective of the Simple Mail Transfer Protocol (SMTP) is to
 *                      transfer mail reliably and efficiently.SMTP is independent of the
 *                      particular transmission subsystem and requires only a reliable
 *                      ordered data stream channel.
 *                      Refer: https://www.ietf.org/rfc/rfc2821.txt
 */

#include "email.h"
#include "base64.h"
#include "config.h"
#include "sl_common.h"

/*NetApp Email protocol types */
#define SL_NET_APP_SMTP_PROTOCOL        (1)
#define INVALID_SOCKET_DESC             0xFFFFFFFF

/* Strings used to configure the Email msg in proper format */
const _u8 smtp_helo[] = {'H','E','L','O','\0'};
const _u8 smtp_mail_from[]={'M','A','I','L',' ','F','R','O','M',':',' ','\0'};
const _u8 smtp_rcpt[] = {'R','C','P','T',' ','T','O',':',' ','\0'};
const _u8 smtp_data[] = "DATA";
const _u8 smtp_crlf[] = "\r\n";
const _u8 smtp_dcrlf[] = "\r\n\r\n";
const _u8 smtp_subject[] = "Subject: ";
const _u8 smtp_to[] = "To: ";
const _u8 smtp_from[] = "From: ";
/* <CRLF>.<CRLF> Terminates the data portion */
const _u8 smtp_data_end[] = {'\r','\n','.','\r','\n','\0'};
const _u8 smtp_quit[] = {'Q','U','I','T','\r','\n','\0'};

/* Return Codes */
const _u8 smtp_code_ready[] = {'2','2','0','\0'};
const _u8 smtp_ok_reply[] = {'2','5','0','\0'};
const _u8 smtp_intermed_reply[] = {'3','5','4','\0'};
const _u8 smtp_auth_reply[] = {'3','3','4','\0'};
const _u8 smtp_auth_success[] = {'2','3','5','\0'};

/* states for smtp state machine */
typedef enum
{
    smtpINACTIVE = 0,
    smtpINIT,
    smtpHELO,
    smtpAUTH,
    smtpFROM,
    smtpRCPT,
    smtpDATA,
    smtpMESSAGE,
    smtpQUIT,
    smtpERROR
}_SlsmtpStatus_e;

/* Initialize SMTP State Machine */
_u16 g_smtpStatus = smtpINIT;

_u16 g_EmailSetStatus = 0;

/* error handling flags for smtp state machine */
typedef enum
{
    smtpNOERROR = 0,
    atINIT,
    atHELO,
    atAUTH,
    atFROM,
    atRCPT,
    atDATA,
    atMESSAGE,
    atQUIT
}_SlsmtpERROR_e;

/* Initialize Error handling flag */
_u16 g_smtpErrorInfo = smtpNOERROR;
_i32 smtpSocket;
_u8 g_cmdBuf[SMTP_BUF_LEN];
_u8 buf[SMTP_BUF_LEN];
_u8 basekey1[BASEKEY_LEN];
_u8 basekey2[BASEKEY_LEN];
_u8 message[MAX_MESSAGE_LEN];

SlNetAppEmailOpt_t g_EmailOpt;
SlNetAppSourceEmail_t g_Email;
SlNetAppSourcePassword_t g_SourcePass;
SlNetAppDestination_t g_Destination;
SlNetAppEmailSubject_t g_Subject;

_u8 email_rfc[MAX_EMAIL_RCF_LEN];

static _i32 _smtpConnect(void);
static _i32 _smtpSend(void);
static void _smtpHandleERROR(_u8 * servermessage);
static _i32 _sendSMTPCommand(_i32 socket, _u8 * cmd, _u8 * cmdparam, _u8 * respBuf);
static void _generateBase64Key(_u8 * basekey1, _u8 * basekey2);


/*!
    \brief This function sets the email parameters

    \param[in]      command -   Command send for processing
    \param[in]      pValueLen - Length of data to be processed
    \param[in]      pValue -     Data to be processed

    \return         0 for success, -1 otherwise

    \note

    \warning
*/
_i32 sl_NetAppEmailSet(_u8 command ,_u8 pValueLen,
                      _u8 *pValue)
{

    SlNetAppEmailOpt_t* pEmailOpt = 0;
    SlNetAppSourceEmail_t* pSourceEmail = NULL;
    SlNetAppSourcePassword_t* pSourcePassword = NULL;
    SlNetAppDestination_t* pDestinationEmail = NULL;
    SlNetAppEmailSubject_t* pSubject = NULL;

    switch (command)
    {
      case NETAPP_ADVANCED_OPT:
        pEmailOpt = (SlNetAppEmailOpt_t*)pValue;

        g_EmailOpt.Port = pEmailOpt->Port;
        g_EmailOpt.Family = pEmailOpt->Family;
        g_EmailOpt.SecurityMethod = pEmailOpt->SecurityMethod;
        g_EmailOpt.SecurityCypher = pEmailOpt->SecurityCypher;
        g_EmailOpt.Ip = pEmailOpt->Ip;

        g_EmailSetStatus+=1;
        break;

      case NETAPP_SOURCE_EMAIL:
        pSourceEmail = (SlNetAppSourceEmail_t*)pValue;
        pal_Memset(g_Email.Username, '\0', MAX_USERNAME_LEN);
        pal_Memcpy(g_Email.Username, pSourceEmail->Username, pValueLen);

        g_EmailSetStatus+=2;

        break;

      case NETAPP_PASSWORD:
        pSourcePassword = (SlNetAppSourcePassword_t*)pValue;
        pal_Memset(g_SourcePass.Password, '\0', MAX_PASSWORD_LEN);
        pal_Memcpy(g_SourcePass.Password, pSourcePassword->Password, pValueLen);

        g_EmailSetStatus+=4;

        break;

      case NETAPP_DEST_EMAIL:
        pDestinationEmail=(SlNetAppDestination_t*)pValue;
        pal_Memset(g_Destination.Email, '\0', MAX_DEST_EMAIL_LEN);
        pal_Memcpy(g_Destination.Email, pDestinationEmail->Email, pValueLen);

        g_EmailSetStatus+=8;

        break;

      case NETAPP_SUBJECT:
        pSubject=(SlNetAppEmailSubject_t*)pValue;
        pal_Memset(g_Subject.Value, '\0', MAX_SUBJECT_LEN);
        pal_Memcpy(g_Subject.Value, pSubject->Value, pValueLen);

        g_EmailSetStatus+=16;

        break;

      case NETAPP_MESSAGE:
        if(pValueLen > (MAX_MESSAGE_LEN - 1))
        {
            ASSERT_ON_ERROR(EMAIL_SET_INVALID_MESSAGE);
        }
        pal_Memset(message, '\0', MAX_MESSAGE_LEN);
        pal_Memcpy(message ,pValue, pValueLen);
        break;

      default:
        CLI_Write((_u8*)"\n\rError:Default case\n\r");
        ASSERT_ON_ERROR(EMAIL_SET_INVALID_CASE);
    }

    return SUCCESS;
}

/*!
    \brief Create a secure socket and connects to SMTP server

    \param[in]      none

    \return         0 if success and negative in case of error

    \note

    \warning
*/
_i32 sl_NetAppEmailConnect()
{
    if (!(g_EmailSetStatus >= 0x07))
    {
        CLI_Write((_u8*)"\n\rError:Email and Subject is not configured\n\r");
        ASSERT_ON_ERROR(EMAIL_CONNECT_INVALID_CONFIURATION);
    }

    return _smtpConnect();
}

/*!
    \brief Check the connection status and sends the Email

    \param[in]      none

    \return         0 if success otherwise -1

    \note

    \warning
*/
_i32 sl_NetAppEmailSend()
{
    _i32 retVal = -1;
    retVal = _smtpSend();
    if(retVal < 0)
        sl_Close(smtpSocket);

    return retVal;
}


/*!
    \brief Creates a secure socket and connects to SMTP server

    \param[in]      none

    \return         0 if success and negative in case of error

    \note

    \warning
*/
static _i32 _smtpConnect(void)
{
    SlSockAddrIn_t  LocalAddr;
    SlTimeval_t     tTimeout;
    _i32           cipher = 0;
    _i32           LocalAddrSize = 0;
    _i8            method = 0;
    _i32            Status = 0;

    LocalAddr.sin_family = g_EmailOpt.Family;
    LocalAddr.sin_port = sl_Htons(g_EmailOpt.Port);
    LocalAddr.sin_addr.s_addr = sl_Htonl(g_EmailOpt.Ip);
    LocalAddrSize = sizeof(SlSockAddrIn_t);

    /* If TLS is required */
    if(g_EmailOpt.SecurityMethod <= 5)
    {
        /* Create secure socket */
        smtpSocket = sl_Socket(SL_AF_INET, SL_SOCK_STREAM, SL_SEC_SOCKET);
        ASSERT_ON_ERROR(smtpSocket);

        tTimeout.tv_sec = 10;
        tTimeout.tv_usec = 90000;
        Status = sl_SetSockOpt(smtpSocket, SOL_SOCKET, SL_SO_RCVTIMEO,
                               &tTimeout, sizeof(SlTimeval_t));
        ASSERT_ON_ERROR(Status);

        method = g_EmailOpt.SecurityMethod;
        cipher = g_EmailOpt.SecurityCypher;

        /* Set Socket Options that were just defined */
        Status = sl_SetSockOpt(smtpSocket, SL_SOL_SOCKET, SL_SO_SECMETHOD,
                               &method, sizeof(method));
        if( Status < 0 )
        {
            sl_Close(smtpSocket);
            ASSERT_ON_ERROR(Status);
        }
        Status = sl_SetSockOpt(smtpSocket, SL_SOL_SOCKET, SL_SO_SECURE_MASK,
                               &cipher, sizeof(cipher));
        if( Status < 0 )
        {
            sl_Close(smtpSocket);
            ASSERT_ON_ERROR(Status);
        }
    }
    /* If no TLS required */
    else
    {
        /* Create socket */
        smtpSocket = sl_Socket(SL_AF_INET, SL_SOCK_STREAM, SL_IPPROTO_TCP);
        ASSERT_ON_ERROR(smtpSocket);
    }

    /* connect to socket */
    Status = sl_Connect(smtpSocket, (SlSockAddr_t *)&LocalAddr, LocalAddrSize);
    if((Status < 0) && (SL_ESECSNOVERIFY != Status))
    {
        ASSERT_ON_ERROR(Status);
    }

    return SUCCESS;
}

/*!
    \brief This function will send the email

    \param[in]      none

    \return         0 if success otherwise -1

    \note

    \warning
*/
static _i32 _smtpSend(void)
{
    _i16 exit = 0;
    _i32 retVal = -1;

    /* If socket has been opened, check for acknowledge from SMTP server */
    while(exit == 0)
    {
        switch(g_smtpStatus)
        {
            /*
             * An SMTP session is initiated when a client opens a connection to
             * a server and the server responds with an opening message.
             */
            case smtpINIT:
                /* Create buffer, Read so we can check for 220 'OK' from server */
                pal_Memset(buf, 0, SMTP_BUF_LEN);

                retVal = sl_Recv(smtpSocket, buf, sizeof(buf), 0);
                if(retVal <= 0)
                {
                    ASSERT_ON_ERROR(TCP_RECV_ERROR);
                }

                /* If buffer has 220, set state to HELO */
                if(buf[0] == smtp_code_ready[0] &&
                   buf[1] == smtp_code_ready[1] &&
                   buf[2] == smtp_code_ready[2])
                {
                    g_smtpStatus = smtpHELO;
                }
                /* Else error, set state to ERROR */
                else
                {
                    g_smtpStatus = smtpERROR;
                    g_smtpErrorInfo = atINIT;
                }
            break;

            /*
             * The client normally sends the EHLO command to the
             * server, indicating the client's identity.  In addition to opening the
             * session, use of EHLO indicates that the client is able to process
             * service extensions and requests that the server provide a list of the
             * extensions it supports.
             */
            case smtpHELO:
                retVal = _sendSMTPCommand(smtpSocket, "HELO localhost", NULL, buf);
                ASSERT_ON_ERROR(retVal);

                /* If response has 250, set state to AUTH */
                if(buf[0] == smtp_ok_reply[0] &&
                   buf[1] == smtp_ok_reply[1] &&
                   buf[2] == smtp_ok_reply[2])
                {
                    g_smtpStatus = smtpAUTH;
                }
                /* Else error, set state to ERROR */
                else
                {
                    g_smtpStatus = smtpERROR;
                    g_smtpErrorInfo = atHELO;
                }
            break;

            /* The client sends the AUTH command with SASL mechanism to use with */
            case smtpAUTH:
                /*
                 * SASL PLain - Authentication with server username and password
                 * Refer - http://www.ietf.org/rfc/rfc4616.txt
                 */
                _generateBase64Key((_u8*)g_Email.Username, basekey1);
                _generateBase64Key((_u8*)g_SourcePass.Password, basekey2);

                /* Send request to server for authentication */
                retVal = _sendSMTPCommand(smtpSocket, "AUTH LOGIN", NULL, buf);
                ASSERT_ON_ERROR(retVal);
                /* If response has 334, give username in base64 */
                if(buf[0] == smtp_auth_reply[0] &&
                   buf[1] == smtp_auth_reply[1] &&
                   buf[2] == smtp_auth_reply[2])
                {
                    retVal = _sendSMTPCommand(smtpSocket, basekey1, NULL, buf);
                    ASSERT_ON_ERROR(retVal);
                    /* If response has 334, give password in base64 */
                    if(buf[0] == smtp_auth_reply[0] &&
                       buf[1] == smtp_auth_reply[1] &&
                       buf[2] == smtp_auth_reply[2])
                    {
                        retVal = _sendSMTPCommand(smtpSocket, basekey2, NULL, buf);
                        ASSERT_ON_ERROR(retVal);
                    }
                }

                if(buf[0] == smtp_auth_success[0] &&
                   buf[1] == smtp_auth_success[1] &&
                   buf[2] == smtp_auth_success[2])
                {
                    /* Authentication was successful, set state to FROM */
                    g_smtpStatus = smtpFROM;
                }
                /* Else error, set state to ERROR */
                else
                {
                    g_smtpStatus = smtpERROR;
                    g_smtpErrorInfo = atAUTH;
                }
            break;

            /*
             * The SMTP transaction starts with a MAIL command which includes the
             * sender information
             * MAIL FROM:<reverse-path> [SP <mail-parameters> ] <CRLF>
             */
            case smtpFROM:
                retVal = _sendSMTPCommand(smtpSocket, (_u8 *)smtp_mail_from,
                                 (_u8 *)g_Email.Username, buf);
                ASSERT_ON_ERROR(retVal);

                /* If response has 250, set state to RCPT */
                if(buf[0] == smtp_ok_reply[0] &&
                   buf[1] == smtp_ok_reply[1] &&
                   buf[2] == smtp_ok_reply[2])
                {
                    g_smtpStatus = smtpRCPT;
                }
                else
                {
                    pal_Memset(email_rfc,'\0',MAX_EMAIL_RCF_LEN);
                    pal_Memcpy(email_rfc,"<",1);
                    pal_Memcpy(&email_rfc[1], g_Email.Username,
                           pal_Strlen(g_Email.Username));
                    pal_Memcpy(&email_rfc[1+pal_Strlen(g_Email.Username)],">",1);

                    retVal = _sendSMTPCommand(smtpSocket, (_u8 *)smtp_mail_from,
                                     email_rfc, buf);
                    ASSERT_ON_ERROR(retVal);

                    if(buf[0] == smtp_ok_reply[0] &&
                       buf[1] == smtp_ok_reply[1] &&
                       buf[2] == smtp_ok_reply[2])
                    {
                        g_smtpStatus = smtpRCPT;
                    }
                    else
                    {
                        g_smtpStatus = smtpERROR;
                        g_smtpErrorInfo = atFROM;
                    }
                }
            break;

            /* Send the destination email to the smtp server
             * RCPT TO:<forward-path> [ SP <rcpt-parameters> ] <CRLF>
             */
            case smtpRCPT:
                retVal = _sendSMTPCommand(smtpSocket, (_u8 *)smtp_rcpt,
                                 (_u8 *)g_Destination.Email, buf);
                ASSERT_ON_ERROR(retVal);

                /* If response has 250, set state to DATA */
                if(buf[0] == smtp_ok_reply[0] &&
                   buf[1] == smtp_ok_reply[1] &&
                   buf[2] == smtp_ok_reply[2])
                {
                    g_smtpStatus = smtpDATA;
                }
                else
                {
                    pal_Memset(email_rfc,'\0',MAX_EMAIL_RCF_LEN);
                    pal_Memcpy(email_rfc,"<",1);
                    pal_Memcpy(&email_rfc[1], g_Destination.Email,
                           pal_Strlen(g_Destination.Email));
                    pal_Memcpy(&email_rfc[1+pal_Strlen(g_Destination.Email)],">",1);
                    retVal = _sendSMTPCommand(smtpSocket, (_u8 *)smtp_rcpt,email_rfc, buf);
                    ASSERT_ON_ERROR(retVal);
                    /* If response has 250, set state to DATA */
                    if(buf[0] == smtp_ok_reply[0] &&
                       buf[1] == smtp_ok_reply[1] &&
                       buf[2] == smtp_ok_reply[2])
                    {
                        g_smtpStatus = smtpDATA;
                    }
                    else
                    {
                        g_smtpStatus = smtpERROR;
                        g_smtpErrorInfo = atRCPT;
                    }
                }
            break;

            /*Send the "DATA" message to the server, indicates client is ready
            * to construct the body of the email
            * DATA <CRLF>
            */
            case smtpDATA:
                retVal = _sendSMTPCommand(smtpSocket, (_u8 *)smtp_data, NULL, buf);
                ASSERT_ON_ERROR(retVal);
                /* If Response has 250, set state to MESSAGE */
                if(buf[0] == smtp_intermed_reply[0] &&
                buf[1] == smtp_intermed_reply[1] && buf[2] == smtp_intermed_reply[2])
                {
                    g_smtpStatus = smtpMESSAGE;
                }
                else
                {
                    g_smtpStatus = smtpERROR;
                    g_smtpErrorInfo = atDATA;
                }
            break;

            case smtpMESSAGE:
                /* Send actual Message, preceded by FROM, TO and Subject */

                /* Start with E-Mail's "Subject:" field */
                retVal = _sendSMTPCommand(smtpSocket, (_u8 *)smtp_subject,
                                                (_u8 *)g_Subject.Value, NULL);
                ASSERT_ON_ERROR(retVal);

                /* Add E-mail's "To:" field */
                retVal = _sendSMTPCommand(smtpSocket, (_u8 *)smtp_to,
                                            (_u8 *)g_Destination.Email, NULL);
                ASSERT_ON_ERROR(retVal);

                /* Add E-mail's "From:" field */
                retVal = _sendSMTPCommand(smtpSocket, (_u8 *)smtp_from,
                                                (_u8 *)g_Email.Username, NULL);
                ASSERT_ON_ERROR(retVal);

                /* Send CRLF */
                retVal = sl_Send(smtpSocket,smtp_crlf,pal_Strlen(smtp_crlf),0);
                if(retVal <= 0)
                    ASSERT_ON_ERROR(TCP_SEND_ERROR);

                /* Send body of message */
                retVal = _sendSMTPCommand(smtpSocket, message, NULL, NULL);
                ASSERT_ON_ERROR(retVal);

                /* End Message */
                retVal = _sendSMTPCommand(smtpSocket,(_u8 *)smtp_data_end,NULL,buf);
                ASSERT_ON_ERROR(retVal);

                /* Server will send 250 for successful. Move into QUIT state. */
                if(buf[0] == smtp_ok_reply[0] && buf[1] == smtp_ok_reply[1] &&
                                                        buf[2] == smtp_ok_reply[2])
                {
                    /* Disconnect from server by sending QUIT command */
                    retVal = sl_Send(smtpSocket,smtp_quit,pal_Strlen(smtp_quit),0);
                    if(retVal <= 0)
                        ASSERT_ON_ERROR(TCP_SEND_ERROR);

                    /* Close socket and reset */
                    retVal = sl_Close(smtpSocket);
                    smtpSocket = INVALID_SOCKET_DESC;
                    /* Reset the state machine */
                    g_smtpStatus = smtpINIT;
                    exit = 1;
                }
                else
                {
                    g_smtpStatus = smtpERROR;
                    g_smtpErrorInfo = atMESSAGE;
                }
            break;

            case smtpERROR:
                /* Error Handling for SMTP */
                _smtpHandleERROR(buf);
                /* Close socket and reset */
                retVal = sl_Close(smtpSocket);
                /*Reset the state machine */
                g_smtpStatus = smtpINIT;

                ASSERT_ON_ERROR(SMTP_ERROR);

            default:
                ASSERT_ON_ERROR(SMTP_INVALID_CASE);
        }
    }

    return SUCCESS;
}

/*!
    \brief This function converts the string to Base64 format needed for
           authentication

    \param[in]      input - pointer to string to be converted
    \param[out]     basekey1 - Pointer to string for base64 converted output

    \return         None

    \note

    \warning
*/
static void _generateBase64Key(_u8 *input, _u8 *basekey1)
{
    _u8 *pIn = input;

    /* Convert to base64 format  */
    ConvertToBase64(basekey1, (void *)pIn, pal_Strlen(input));
}

/*!
    \brief Performs Error Handling for SMTP State Machine

    \param[in]      servermessage - server response message

    \return         None

    \note

    \warning
*/
static void _smtpHandleERROR(_u8 *servermessage)
{
    /* Errors are handled using flags set in the smtpStateMachine */
    switch(g_smtpErrorInfo)
    {
        case atINIT:
            /* Server connection could not be established */
            CLI_Write((_u8*)"Server connection error.\r\n");
            CLI_Write((_u8*)servermessage);
        break;

        case atHELO:
            /* Server did not accept the HELO command from server */
            CLI_Write((_u8*)"Server did not accept HELO:\r\n");
            CLI_Write((_u8*)servermessage);
        break;

        case atAUTH:
            /* Server did not accept authorization credentials */
            CLI_Write((_u8*)"Authorization unsuccessful, ");
            CLI_Write((_u8*)"check username/password.\r\n");
            CLI_Write((_u8*)servermessage);
        break;

        case atFROM:
            /* Server did not accept source email. */
            CLI_Write((_u8*)"Email of sender not accepted by server.\r\n");
            CLI_Write((_u8*)servermessage);
        break;

        case atRCPT:
            /* Server did not accept destination email */
            CLI_Write((_u8*)"Email of recipient not accepted by server.\r\n");
            CLI_Write((_u8*)servermessage);
        break;

        case atDATA:
            /* 'DATA' command to server was unsuccessful */
            CLI_Write((_u8*)"smtp 'DATA' command not accepted by server.\r\n");
            CLI_Write((_u8*)servermessage);
        break;

        case atMESSAGE:
            /* Message body could not be sent to server */
            CLI_Write((_u8*)"Email Message was not accepted by the server.\r\n");
            CLI_Write((_u8*)servermessage);
        break;

        case atQUIT:
            /* Message could not be finalized */
            CLI_Write((_u8*)"Connection could not be properly closed.");
            CLI_Write((_u8*)" Message not sent.\r\n");
            CLI_Write((_u8*)servermessage);
        break;
    }
}

/*!
    \brief  Sends the SMTP command and receives the server response.
            If cmd and cmd parameter are NULL, it will only send <CR><LF>

    \param[in]      socket - Socket Descriptor
    \param[in]      cmd -    command to be send to server
    \param[in]      cmdparam - command parameter to be send
    \param[in]      respBuf -    Pointer to buffer for SMTP server response

    \return         None

    \note

    \warning
*/
static _i32 _sendSMTPCommand(_i32 socket, _u8 *cmd, _u8 * cmdparam, _u8 *respBuf)
{
    _i16 sendLen = 0;
    _i32 retVal = -1;

    pal_Memset(g_cmdBuf, 0, sizeof(g_cmdBuf));

    if(cmd != NULL)
    {
        sendLen = pal_Strlen(cmd);
        pal_Memcpy(g_cmdBuf,cmd,pal_Strlen(cmd));
    }

    if(cmdparam != NULL)
    {
        pal_Memcpy(&g_cmdBuf[sendLen], cmdparam, pal_Strlen(cmdparam));
        sendLen += pal_Strlen(cmdparam);
    }

    pal_Memcpy(&g_cmdBuf[sendLen], smtp_crlf, pal_Strlen(smtp_crlf));
    sendLen += pal_Strlen(smtp_crlf);
    retVal = sl_Send(socket, g_cmdBuf,sendLen,0);
    if(retVal <= 0)
        ASSERT_ON_ERROR(TCP_SEND_ERROR);

    if(respBuf != NULL)
    {
        pal_Memset(respBuf,0,SMTP_BUF_LEN);
        retVal = sl_Recv(socket, respBuf,SMTP_BUF_LEN,0);
        if(retVal <= 0)
            ASSERT_ON_ERROR(TCP_RECV_ERROR);
    }

    return SUCCESS;
}
