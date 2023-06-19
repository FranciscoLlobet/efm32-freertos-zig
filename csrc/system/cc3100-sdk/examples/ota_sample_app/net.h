/*
 * Copyright (C) 2014 Texas Instruments Incorporated - http://www.ti.com/
 *
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *
 *    Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 *    Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the
 *    distribution.
 *
 *    Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 *  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 *  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#ifndef __NET_H__
#define __NET_H__

/* Application specific status/error codes */
/* Choosing this number to avoid overlap with host-driver's error codes */
typedef enum{
    DEVICE_NOT_IN_STATION_MODE = -0x7D0,
    FS_FILE_WRITE_ERROR = DEVICE_NOT_IN_STATION_MODE - 1,
    FS_FILE_READ_ERROR = FS_FILE_WRITE_ERROR - 1,
    FILE_MISSING_VERSION = FS_FILE_READ_ERROR - 1,

    STATUS_CODE_MAX = -0xBB8
}e_AppStatusCodes;

extern _i32 configureSimpleLinkToDefaultState(void);
extern _i32 NetWlanConnect(const _u8 *cssid, SlSecParams_t *secParams);
extern _i32 NetGetHostIP( _u8* pcHostName,_u32 * pDestinationIP );
extern _i32 NetMACAddressGet(_u8 *pMACAddress);
extern _i32 NetFwInfoGet(SlVersionFull *sVersion);
extern _u8 NetIsConnectedToAP(void);
extern _i32 NetStop();
extern _i32 NetStart();

#endif /* __NET_H__ */
