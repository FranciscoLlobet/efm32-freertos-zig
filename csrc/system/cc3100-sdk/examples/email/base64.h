/*
 * base64.h
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

#ifndef __BASE64_H__
#define __BASE64_H__

#ifdef  __cplusplus
extern "C" {
#endif  /* __cplusplus */

/*!
    \brief Convert the raw bytes in quasi-big-endian order to Base64 string

    \param[in]      pcOutStr -  Pointer to string for base64 converted output
    \param[in]      pccInStr - pointer to string to be converted
    \param[out]     iLen - length of string to be converted

    \return         None

    \note

    \warning
*/
void ConvertToBase64(_u8 *pcOutStr, const _u8 *pccInStr, _i16 iLen);

#ifdef  __cplusplus
}
#endif /* __cplusplus */
#endif  /* __BASE64_H__ */
