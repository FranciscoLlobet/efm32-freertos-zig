/*
 * base64.c
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

#include "base64.h"

static const _u8 g_ccB64Tbl[64] =
   "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";


void ConvertToBase64(_u8 *pcOutStr, const _u8 *pccInStr, _i16 iLen)
{
    const _u8* pccIn = pccInStr;
    _u8* pcOut;
    _i16 iCount;
    pcOut = pcOutStr;

    //Loop in for Multiple of 24Bits and Convert to Base 64
    for (iCount = 0; iLen - iCount >=3 ; iCount += 3,pccIn += 3)
    {
        *pcOut++ = g_ccB64Tbl[pccIn[0] >> 2];
        *pcOut++ = g_ccB64Tbl[((pccIn[0] & 0x03)<<4) | (pccIn[1] >> 4)];
        *pcOut++ = g_ccB64Tbl[((pccIn[1] & 0x0F)<<2) | (pccIn[2] >> 6)];
        *pcOut++ = g_ccB64Tbl[pccIn[2] & 0x3f];

    }

    //Check if String is not multiple of 3 Bytes
    if (iCount != iLen)
    {
        _u8 ucLastByte;

        *pcOut++ = g_ccB64Tbl[pccIn[0] >> 2];
        ucLastByte = ((pccIn[0] & 0x03)<<4);


        if (iLen - iCount > 1)
        {
            //If there are 2 Extra Bytes
            ucLastByte |= (pccIn[1] >> 4);
            *pcOut++ = g_ccB64Tbl[ucLastByte];
            *pcOut++ = g_ccB64Tbl[((pccIn[1] & 0x0F)<<2)];
        }
        else
        {
             //If there is only 1 Extra Byte
             *pcOut++ = g_ccB64Tbl[ucLastByte];
             *pcOut++ = '=';
        }

         *pcOut++ = '=';
    }

    *pcOut  = '\0';

}
