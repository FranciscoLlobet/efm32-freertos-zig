/*
 * config.h - email application configuration file
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

/* Modify the following settings as necessary to run the email application */
#define SMTP_BUF_LEN            100
#define GMAIL_HOST_NAME         "smtp.gmail.com"
#define GMAIL_HOST_PORT         465
#define YAHOO_HOST_NAME         "smtp.mail.yahoo.com"
#define YAHOO_HOST_PORT         25

/* Source email credentials */

/* Username should be less than (MAX_USERNAME_LEN) characters */
#define USER                    "<username>"
/* Password should be less than (MAX_PASSWORD_LEN) characters */
#define PASS                    "<password>"

/** Destination Email address should be less than (MAX_DEST_EMAIL_LEN)
  * characters */
#define DESTINATION_EMAIL       "<destination_email>"

/* Email subject should be less than (MAX_SUBJECT_LEN) characters */
#define EMAIL_SUBJECT           "<email_subject>"

/* Email message should be less than (MAX_MESSAGE_LEN) characters */
#define EMAIL_MESSAGE           "<email_body>"

/* Application specific status/error codes */
typedef enum{
    DEVICE_NOT_IN_STATION_MODE = -0x7D0,        /* Choosing this number to avoid overlap w/ host-driver's error codes */
    EMAIL_SET_INVALID_MESSAGE = DEVICE_NOT_IN_STATION_MODE - 1,
    EMAIL_SET_INVALID_CASE =EMAIL_SET_INVALID_MESSAGE - 1,
    EMAIL_CONNECT_INVALID_CONFIURATION = EMAIL_SET_INVALID_CASE - 1,
    TCP_RECV_ERROR = EMAIL_CONNECT_INVALID_CONFIURATION - 1,
    TCP_SEND_ERROR = TCP_RECV_ERROR - 1,
    SMTP_ERROR = TCP_SEND_ERROR,
    SMTP_INVALID_CASE = SMTP_ERROR -1,

    STATUS_CODE_MAX = -0xBB8
}e_AppStatusCodes;


#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* DEMO_CONFIG_H */
