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


#ifndef __OTA_H__
#define __OTA_H__


#define OTA_SERVER_NAME                 "api.dropbox.com"
#define OTA_SERVER_IP_ADDRESS           0x00000000
#define OTA_SERVER_SECURED              1
#define OTA_SERVER_APP_TOKEN            "<dropbox access token>"
#define OTA_VENDOR_STRING               "Vid01_Pid01_Ver00"



#define OSI_STACK_SIZE          2048
#define GMT_DIFF_TIME_HRS       5
#define GMT_DIFF_TIME_MINS      30

#define OTA_STOPPED             0
#define OTA_INPROGRESS          1
#define OTA_DONE                2
#define OTA_NO_UPDATES          3
#define OTA_ERROR_RETRY         4
#define OTA_ERROR               5


#define TIME2013                3565987200u      /* 113 years + 28 days(leap) */
#define YEAR2013                2013
#define SEC_IN_MIN              60
#define SEC_IN_HOUR             3600
#define SEC_IN_DAY              86400

#define NET_STAT_OFF            0
#define NET_STAT_STARTED        1
#define NET_STAT_CONN           2
#define NET_STAT_CONNED         3

#define APP_VERSION             "1.3.0"

typedef struct
{
  _u8  ucGmtDiffHr;
  _u8  ucGmtDiffMins;
  _u32 ulNtpServerIP;
  _i32 iSocket;

}tGetTime;



typedef struct
{
  _u8 ucAppVersion[30];
  SlVersionFull sNwpVersion;
  _u8 ucNwpVersion[50];
  _u32 usFileVersion;
  _u8 ucFileVersion[10];
  _u8 ucTimeZone[20];
  _u8 ucServerIndex;
  _u8 ucNetStat;
  _u32 ulOTAErrorCount;

  union
  {
    _u32 ulServerIP;
    _u8 ucServerIP[4];
  };

  _u8 ucUTCTime[30];
  _u8 ucLocalTime[30];
  _i32  iOTAStatus;

}tDisplayInfo;


#endif /* __OTA_H__ */
