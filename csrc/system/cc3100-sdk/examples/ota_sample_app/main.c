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


/*
 * Application Name     - OTA Sample App
 * Application Overview - This application focuses on showcasing CC3100’s
 *                        ability to receive firmware update and/or any
 *                        related files over the internet enabled Wi-Fi
 *                        interface.  The example uses Dropbox API App
 *                        platform to store and distribute the OTA update
 *                        files.
 * Application Details  - http://processors.wiki.ti.com/index.php/CC31xx_OTA_Sample_Application
 *                        doc\examples\ota_sample_app.pdf
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/* simplelink includes */
#include "simplelink.h"
#include "nonos.h"

#include "flc_api.h"
#include "flc.h"
#include "ota_api.h"
#include "otaconfig.h"
#include "time.h"
#include "net.h"
#include "sl_common.h"

/* OTA sample file name */
#define SL_FILE_NAME       "otaSampleFile.txt"
/* File version string */
#define FILE_VERSION_STR   "FileVersion ="

/*
 * System state Macros
 */
#define SYS_STATE_WAIT          0
#define SYS_STATE_RUN           1
#define SYS_STATE_REBOOT        2
#define SYS_STATE_TEST_REBOOT   3

/*
 * Number of servers in NTP server list
 */
#define NOF_NTP_SERVER         (sizeof(g_acSNTPserver)/30)

/*
 * Application Error code Macros
 */


/*
 * OTA Update task state Macros
 */
#define OTA_UPDATE_STATE_WAIT_START     0
#define OTA_UPDATE_STATE_INIT           2
#define OTA_UPDATE_STATE_RUN            3
#define OTA_UPDATE_STATE_DONE           4

/* OTA complete return Value */
#define OTA_RET_COMPLETE                0xF

#define FALIURE                         -1
#define SUCCESS                         0

#define APP_NAME                        "OTA Update"

#define OTA_VENDOR_MAX_STR              30
#define BUFFER_SIZE                     100

/* Delay in seconds between each update check */
#define DELAY_IN_SEC                    10

/*
 * Global static variables
 */
static OtaOptServerInfo_t g_otaOptServerInfo = {0};
static void *pvOtaApp = 0;
static tDisplayInfo sDisplayInfo = {0};
SlSecParams_t g_SecurityParams = {0};
_u8 g_ucOtaVendorStr[OTA_VENDOR_MAX_STR] = {0};
_u8 g_ucBuff[BUFFER_SIZE] = {0};
/*
 * GLOBAL VARIABLES -- End
 */

/*
 * NTP Server List
 */

/*! ######################### list of SNTP servers ############################
 *! ##
 *! ##         hostname            |        IP       |     location
 *! ## --------------------------------------------------------------------------
 *! ## 2.in.pool.ntp.org           | 113.30.137.34   |
 *! ## dmz0.la-archdiocese.net     | 209.151.225.100 | Los Angeles, CA
 *! ## ntp.inode.at                | 195.58.160.5    | Vienna
 *! ## ntp3.proserve.nl            | 212.204.198.85  | Amsterdam
 *! ## ntp.spadhausen.com          | 109.168.118.249 | Milano - Italy
 *! ## Optimussupreme.64bitVPS.com | 216.128.88.62   | Brooklyn, New York
 *! ## ntp.mazzanet.id.au          | 203.206.205.83  | Regional Victoria, Australia
 *! ## a.ntp.br                    | 200.160.0.8     | Sao Paulo, Brazil
 *! ###########################################################################
 */
const _u8 g_acSNTPserver[][30] =
{
  "dmz0.la-archdiocese.net",
  "2.in.pool.ntp.org",
  "ntp.inode.at",
  "ntp3.proserve.nl",
  "ntp.spadhausen.com",
  "Optimussupreme.64bitVPS.com",
  "ntp.mazzanet.id.au",
  "a.ntp.br"
};


/*
 * Tuesday is the 1st day in 2013 - the relative year
 */
const _u8 g_acDaysOfWeek2013[7][3] = {{"Tue"},
                                    {"Wed"},
                                    {"Thu"},
                                    {"Fri"},
                                    {"Sat"},
                                    {"Sun"},
                                    {"Mon"}};

/*
 * Month string list
 */
const _u8 g_acMonthOfYear[12][3] = {{"Jan"},
                                  {"Feb"},
                                  {"Mar"},
                                  {"Apr"},
                                  {"May"},
                                  {"Jun"},
                                  {"Jul"},
                                  {"Aug"},
                                  {"Sep"},
                                  {"Oct"},
                                  {"Nov"},
                                  {"Dec"}};

/*
 * Days per month
 */
const _u8 g_acNumOfDaysPerMonth[12] = {31, 28, 31, 30, 31, 30,
                                        31, 31, 30, 31, 30, 31};

/*
 * STATIC FUNCTION DEFINITIONS -- Start
 */
static void DisplayBanner();
static _i32 Display();
static void TimeToString(_u32 ulTime, _u8 *ulStrTime);
static _i32 GetSNTPTime(tGetTime *pGetTime);
static _i32 DisplayGetNTPTime();
static _i32 OTAServerInfoSet(void **pvOtaApp, _u8 *vendorStr);
static _i32 OTAUpdateStep();
static void UpdateFwString();
static void OtaVdrStrCreate();
static _i32 UpdateFileString();

/*
 * STATIC FUNCTION DEFINITIONS -- End
 */

/*
 * Application startup display on UART
 *
 * \param    none
 *
 * \return   none
 */

static void DisplayBanner()
{

    CLI_Write("\n\n\n\r");
    CLI_Write("\t\t *************************************************\n\r");
    CLI_Write("\t\t         CC3100 ");
    CLI_Write(APP_NAME);
    CLI_Write("Application       \n\r");
    CLI_Write("\t\t *************************************************\n\r");
    CLI_Write("\n\n\n\r");
}

/*
 * Convert time in seconds to a string of date and time
 *
 * \param[in]  ulTime is time in seconds
 * \param[out] ulStrTime is pointer to an array of characters
 *
 * \return      none
 */
static void TimeToString(_u32 ulTime, _u8 *ulStrTime)
{

    _i16 isGeneralVar = 0;
    _u32 ulGeneralVar = 0;
    _u32 ulGeneralVar1 = 0;
    _u8 iIndex;

    /* seconds are relative to 0h on 1 January 1900 */
    ulTime -= TIME2013;

    /* day, number of days since beginning of 2013 */
    isGeneralVar = ulTime/SEC_IN_DAY;
    memcpy(ulStrTime, g_acDaysOfWeek2013[isGeneralVar%7], 3);
    ulStrTime += 3;
    *ulStrTime++ = '\x20';

    /* month */
    isGeneralVar %= 365;
    for (iIndex = 0; iIndex < 12; iIndex++)
    {
        isGeneralVar -= g_acNumOfDaysPerMonth[iIndex];
        if (isGeneralVar < 0)
                break;
    }

    if(iIndex == 12)
    {
        iIndex = 0;
    }

    memcpy(ulStrTime, g_acMonthOfYear[iIndex], 3);
    ulStrTime += 3;
    *ulStrTime++ = '\x20';

    /* date */
    /* restore the day in current month */
    isGeneralVar += g_acNumOfDaysPerMonth[iIndex];
    ulStrTime += sprintf((char *)ulStrTime,"%02d ",isGeneralVar + 1);

    /* year */
    /* number of days since beginning of 2013 */
    ulGeneralVar = ulTime/SEC_IN_DAY;
    ulGeneralVar /= 365;

    ulStrTime += sprintf((char *)ulStrTime,"%4ld ",YEAR2013 + ulGeneralVar);

    /* time */
    ulGeneralVar = ulTime%SEC_IN_DAY;

    /* number of seconds per hour */
    ulGeneralVar1 = ulGeneralVar%SEC_IN_HOUR;

    /* number of hours */
    ulGeneralVar /= SEC_IN_HOUR;
    ulStrTime += sprintf((char *)ulStrTime,"%02ld:",ulGeneralVar);


    /* number of minutes per hour */
    ulGeneralVar = ulGeneralVar1/SEC_IN_MIN;

    /* number of seconds per minute */
    ulGeneralVar1 %= SEC_IN_MIN;
    ulStrTime += sprintf((char *)ulStrTime,"%02ld:",ulGeneralVar);

    sprintf((char *)ulStrTime,"%02ld",ulGeneralVar1);

}

/*
 * Gets the current time from the selected SNTP server
 * \brief  This function obtains the NTP time from the server.
 *
 * \param  pGetTime is pointer to Get time structure
 *
 * \return 0 : success, -ve : failure
 */
static _i32 GetSNTPTime(tGetTime *pGetTime)
{

/*
                            NTP Packet Header:


       0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9  0  1
      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
      |LI | VN  |Mode |    Stratum    |     Poll      |   Precision    |
      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
      |                          Root  Delay                           |
      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
      |                       Root  Dispersion                         |
      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
      |                     Reference Identifier                       |
      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
      |                                                                |
      |                    Reference Timestamp (64)                    |
      |                                                                |
      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
      |                                                                |
      |                    Originate Timestamp (64)                    |
      |                                                                |
      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
      |                                                                |
      |                     Receive Timestamp (64)                     |
      |                                                                |
      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
      |                                                                |
      |                     Transmit Timestamp (64)                    |
      |                                                                |
      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
      |                 Key Identifier (optional) (32)                 |
      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
      |                                                                |
      |                                                                |
      |                 Message Digest (optional) (128)                |
      |                                                                |
      |                                                                |
      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

*/
    _u8 cDataBuf[48] = {0};
    _i32 lRetVal = 0;
    SlSockAddr_t sAddr = {0};
    _u32 ulElapsedSec = 0;

    /* Send a query ? to the NTP server to get the NTP time */
    memset(cDataBuf, 0, sizeof(cDataBuf));
    cDataBuf[0] = '\x1b';

    sAddr.sa_family = AF_INET;

    /* the source port */
    sAddr.sa_data[0] = 0x00;
    sAddr.sa_data[1] = 0x7B;    /* UDP port number for NTP is 123 */
    sAddr.sa_data[2] = (_u8)((pGetTime->ulNtpServerIP >>24)&0xff);
    sAddr.sa_data[3] = (_u8)((pGetTime->ulNtpServerIP >>16)&0xff);
    sAddr.sa_data[4] = (_u8)((pGetTime->ulNtpServerIP >>8)&0xff);
    sAddr.sa_data[5] = (_u8)(pGetTime->ulNtpServerIP&0xff);

    lRetVal = sl_SendTo(pGetTime->iSocket,
                     cDataBuf,
                     sizeof(cDataBuf), 0,
                     &sAddr, sizeof(sAddr));

    if (lRetVal != sizeof(cDataBuf))
    {
      return FALIURE;
    }

    lRetVal = sl_Recv(pGetTime->iSocket,
                       cDataBuf, sizeof(cDataBuf), 0);

    /* Confirm that the MODE is 4 --> server */
    if ((cDataBuf[0] & 0x7) != 4)    /* expect only server response */
    {
         return FALIURE;
    }
    else
    {
        /*
         * Getting the data from the Transmit Timestamp (seconds) field
         * This is the time at which the reply departed the
         * server for the client
         */
        ulElapsedSec = cDataBuf[40];
        ulElapsedSec <<= 8;
        ulElapsedSec += cDataBuf[41];
        ulElapsedSec <<= 8;
        ulElapsedSec += cDataBuf[42];
        ulElapsedSec <<= 8;
        ulElapsedSec += cDataBuf[43];

        /* Compute the UTC time */
        TimeToString(ulElapsedSec, sDisplayInfo.ucUTCTime);

        /* Set the time zone */
        ulElapsedSec += (pGetTime->ucGmtDiffHr * SEC_IN_HOUR);
        ulElapsedSec += (pGetTime->ucGmtDiffMins * SEC_IN_MIN);

        /* Compute the local time */
        TimeToString(ulElapsedSec, sDisplayInfo.ucLocalTime);
    }

    return SUCCESS;
}

/*
 * This function updated the firmware version buffer
 *
 * \param     none
 *
 * \return    none
 */
static void UpdateFwString()
{
    /* Make the formatted string for firmware version */
    sprintf((char *)sDisplayInfo.ucNwpVersion,
            "%ld.%ld.%ld.%ld.31.%ld.%ld.%ld.%ld.%d.%d.%d.%d",
            sDisplayInfo.sNwpVersion.NwpVersion[0],
            sDisplayInfo.sNwpVersion.NwpVersion[1],
            sDisplayInfo.sNwpVersion.NwpVersion[2],
            sDisplayInfo.sNwpVersion.NwpVersion[3],
            sDisplayInfo.sNwpVersion.ChipFwAndPhyVersion.FwVersion[0],
            sDisplayInfo.sNwpVersion.ChipFwAndPhyVersion.FwVersion[1],
            sDisplayInfo.sNwpVersion.ChipFwAndPhyVersion.FwVersion[2],
            sDisplayInfo.sNwpVersion.ChipFwAndPhyVersion.FwVersion[3],
            sDisplayInfo.sNwpVersion.ChipFwAndPhyVersion.PhyVersion[0],
            sDisplayInfo.sNwpVersion.ChipFwAndPhyVersion.PhyVersion[1],
            sDisplayInfo.sNwpVersion.ChipFwAndPhyVersion.PhyVersion[2],
            sDisplayInfo.sNwpVersion.ChipFwAndPhyVersion.PhyVersion[3]);
}

/*
 * This function read the OTA sample file version and update display parameters
 *
 * \param     none
 *
 * \return    0 for success, -ve otherwise
 */
static _i32 UpdateFileString()
{
    _i32 retVal = 0;
    _i32 fileHandle = -1;
    _u32 Token = 0;
    _u8 *pBuff = 0;

    /* Read the OTA Sample file version no. */

    /* Open a Sample file for reading */
    retVal = sl_FsOpen((_u8 *)SL_FILE_NAME,
                       FS_MODE_OPEN_READ, &Token, &fileHandle);
    if(retVal < 0 && retVal != SL_FS_ERR_FILE_NOT_EXISTS)
        return retVal;

    else if(retVal == SL_FS_ERR_FILE_NOT_EXISTS)
    {
        sDisplayInfo.usFileVersion = 0;
    }
    else
    {
        retVal = sl_FsRead(fileHandle, 0,
                    g_ucBuff, (BUFFER_SIZE - 1));
        if(retVal < 0)
        {
            sl_FsClose(fileHandle,0,0,0);
            return retVal;
        }

        pBuff = (_u8 *)strstr((const char *)g_ucBuff, FILE_VERSION_STR);
        if(pBuff == 0)
            return FILE_MISSING_VERSION;

        /* skip FILE_VERSION_STR */
        pBuff += strlen(FILE_VERSION_STR);
        /* skip while space */
        while(*pBuff == ' ')
            pBuff++;

        sDisplayInfo.usFileVersion = ((*pBuff - '0') * 10) +
                                             (*(pBuff + 1) - '0');

        retVal = sl_FsClose(fileHandle,0,0,0);
        ASSERT_ON_ERROR(retVal);
    }

    /* Update the display buffer */
    sprintf((char *)&sDisplayInfo.ucFileVersion,"%02lu",
            sDisplayInfo.usFileVersion);

    return SUCCESS;
}

/*
 * Function implementing the get time functionality using an NTP server
 *
 * \param     none
 *
 * \return    0 for success, -ve otherwise
 */
static _i32 DisplayGetNTPTime()
{
    _i32 lRetVal = -1;
    struct SlTimeval_t timeVal = {0};
    static tGetTime sGetTime = {0};
    _i8 getTimeDone = 0;


    /* Set the status as stated */
    sDisplayInfo.ucNetStat = NET_STAT_STARTED;

    /* Set the time zone */
    sGetTime.ucGmtDiffHr   = GMT_DIFF_TIME_HRS;
    sGetTime.ucGmtDiffMins = GMT_DIFF_TIME_MINS;

    /* Set connecting status */
    sDisplayInfo.ucNetStat = NET_STAT_CONNED;

    /* Create UDP socket */
    sGetTime.iSocket = sl_Socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

    /* Failed to create a socket */
    while(sGetTime.iSocket < 0)
    {
      return sGetTime.iSocket;
    }

    /* Set socket time option */
    timeVal.tv_sec =  5;
    timeVal.tv_usec = 0;
    lRetVal = sl_SetSockOpt(sGetTime.iSocket,SOL_SOCKET,SL_SO_RCVTIMEO, &timeVal,
                  sizeof(timeVal));
    ASSERT_ON_ERROR(lRetVal);

    /* Initialize Server Index */
    sDisplayInfo.ucServerIndex = 0;

    while(getTimeDone != 1 && sDisplayInfo.ucServerIndex < NOF_NTP_SERVER)
    {
        /* Get the NTP server host IP address using the DNS lookup */
        lRetVal = NetGetHostIP((_u8*)g_acSNTPserver[sDisplayInfo.ucServerIndex],
                         &sGetTime.ulNtpServerIP);
        if(lRetVal < 0)
        {
              strcpy((char *)sDisplayInfo.ucUTCTime,
                      "NTP Server Error. Retrying...");
              sDisplayInfo.ucLocalTime[0]='-';
              sDisplayInfo.ucLocalTime[1]='\0';

              Display();

              sDisplayInfo.ucServerIndex
                = sDisplayInfo.ucServerIndex + 1;
              continue;
        }

        sDisplayInfo.ulServerIP = sGetTime.ulNtpServerIP;

        /* Get the NTP time and display the time */
        lRetVal = GetSNTPTime(&sGetTime);
        if(lRetVal < 0)
        {
            strcpy((char *)sDisplayInfo.ucUTCTime,"NTP Server Error. Retrying...");
            sDisplayInfo.ucLocalTime[0]='-';
            sDisplayInfo.ucLocalTime[1]='\0';

            Display();
            sDisplayInfo.ucServerIndex
              = sDisplayInfo.ucServerIndex + 1;

        }
        else
        {
            getTimeDone = 1;
            Display();
        }
    }

    if(sDisplayInfo.ucServerIndex >= NOF_NTP_SERVER)
        sDisplayInfo.ucServerIndex = 0;

    lRetVal = sl_Close(sGetTime.iSocket);
    ASSERT_ON_ERROR(lRetVal);

    return SUCCESS;
}

static void DisplayServerIP()
{
    _u8 Buff[40] = {0};

    sprintf((char *)Buff, "NTP Server IP\t\t: %d.%d.%d.%d\n\n\r",
            sDisplayInfo.ucServerIP[3],
            sDisplayInfo.ucServerIP[2],
            sDisplayInfo.ucServerIP[1],
            sDisplayInfo.ucServerIP[0]);

    CLI_Write(Buff);
}

/*
 * Display the status on uart terminal
 *
 * \param  none
 *
 * \return 0 : success, -ve : failure
 */
static _i32 Display()
{
    /* Clear screen */
    CLI_Write("\033[2J\033[H\033[?25l");

    /* Display Banner */
    DisplayBanner();

    /* Application version */
    CLI_Write("App Version\t\t: ");
    CLI_Write(sDisplayInfo.ucAppVersion);
    CLI_Write("\n\r");

    /* OTA File Version */
    CLI_Write("OTA File Version\t: ");
    CLI_Write(sDisplayInfo.ucFileVersion);
    CLI_Write("\n\r");

    /* Network F/W version */
    CLI_Write("Nwp Version\t\t: ");
    CLI_Write(sDisplayInfo.ucNwpVersion);
    CLI_Write("\n\n\r");

    CLI_Write("Wifi Status\t\t: ");

    switch(sDisplayInfo.ucNetStat)
    {
    case NET_STAT_OFF:
      CLI_Write("Power Off");
      break;

    case NET_STAT_STARTED:
      CLI_Write("Power On");
      break;

    case NET_STAT_CONN:
      CLI_Write("Connecting...");
      break;

    case NET_STAT_CONNED:
      CLI_Write("Connected to ");
      CLI_Write(SSID_NAME);
    }
    CLI_Write("\n\n\r");

    /* Display Server Info */
    CLI_Write("NTP Server\t\t: ");
    CLI_Write((_u8 *)g_acSNTPserver[sDisplayInfo.ucServerIndex]);
    CLI_Write("\n\r");
    DisplayServerIP();


    /* Display Local Time */
    CLI_Write("GMT Time\t\t: ");
    CLI_Write(sDisplayInfo.ucUTCTime);
    CLI_Write("\n\r");


    /* Display Local Time */
    CLI_Write("Local Time\t\t: ");
    CLI_Write(sDisplayInfo.ucLocalTime);
    CLI_Write("\n\n\r");

    /* Display OTA update status */
    CLI_Write("OTA Update Status\t: ");
    switch(sDisplayInfo.iOTAStatus)
    {
    case OTA_STOPPED :
      CLI_Write("OTA stopped...");
      break;


    case OTA_INPROGRESS :
      CLI_Write("In Progress...");
      break;


    case OTA_DONE :
      CLI_Write("Completed...");
      break;


    case OTA_NO_UPDATES :
      CLI_Write("No Updates found...");
      break;


    case OTA_ERROR_RETRY:
      CLI_Write("Error Retrying...");
      break;

    case OTA_ERROR:
      CLI_Write("Failed to access server after 10 retries.");
      break;
    }

    /* Return progress. */
    return SUCCESS;

}

/*
 * Sets the OTA server info and vendor ID
 *
 * \param pvOtaApp pointer to OtaApp handler
 * \param ucVendorStr vendor string
 * \param pfnOTACallBack is  pointer to callback function
 *
 * \return    0 for success, -ve otherwise
 */
static _i32 OTAServerInfoSet(void **pvOtaApp, _u8 *vendorStr)
{
    _i32 lRetVal = 0;

    /* Set OTA server info */
    g_otaOptServerInfo.ip_address = OTA_SERVER_IP_ADDRESS;
    g_otaOptServerInfo.secured_connection = OTA_SERVER_SECURED;
    strcpy((char *)g_otaOptServerInfo.server_domain, OTA_SERVER_NAME);
    strcpy((char *)g_otaOptServerInfo.rest_hdr_val, OTA_SERVER_APP_TOKEN);
    NetMACAddressGet((_u8 *)g_otaOptServerInfo.log_mac_address);

    /* Set OTA server Info */
    lRetVal = sl_extLib_OtaSet(*pvOtaApp, EXTLIB_OTA_SET_OPT_SERVER_INFO,
                     sizeof(g_otaOptServerInfo), (_u8 *)&g_otaOptServerInfo);
    ASSERT_ON_ERROR(lRetVal);

    /* Set vendor ID. */
    lRetVal = sl_extLib_OtaSet(*pvOtaApp, EXTLIB_OTA_SET_OPT_VENDOR_ID,
            strlen((const char *)vendorStr),
                     (_u8 *)vendorStr);
    ASSERT_ON_ERROR(lRetVal);

    /* Return ok status */
    return RUN_STAT_OK;
}

/*
 * Function to create the OTA vendor string
 *
 * \param     none
 *
 * \return    none
 */
static void OtaVdrStrCreate()
{
    _u32 ulVendorStrLen = 0;

    strcpy((char *)g_ucOtaVendorStr,OTA_VENDOR_STRING);
    ulVendorStrLen = strlen(OTA_VENDOR_STRING);

    /* Append the NWP version in the string */
    sprintf((char *)&g_ucOtaVendorStr[ulVendorStrLen],"_%02lu.%02lu.%02lu.%02lu",
            sDisplayInfo.sNwpVersion.NwpVersion[0],
			sDisplayInfo.sNwpVersion.NwpVersion[1],
			sDisplayInfo.sNwpVersion.NwpVersion[2],
			sDisplayInfo.sNwpVersion.NwpVersion[3]);
}

/*
 * Function implementing the OTA update functionality
 *
 * \param     none
 *
 * \return    +ve for success, -ve otherwise
 */
static _i32 OTAUpdateStep()
{
    _i32 iRet = 0;
    _i32 retVal = SUCCESS;
    _i32 SetCommitInt = 1;

    static _u8 cTaskOwnState = OTA_UPDATE_STATE_WAIT_START;

    switch(cTaskOwnState)
    {

    case OTA_UPDATE_STATE_WAIT_START:

        if(NetIsConnectedToAP())
        {
            cTaskOwnState = OTA_UPDATE_STATE_INIT;
        }

        break;

    case OTA_UPDATE_STATE_INIT:

        /* Create vendor string from html file version and vendor prefix */
        OtaVdrStrCreate();

        /* Initialize OTA service */
        OTAServerInfoSet(&pvOtaApp, g_ucOtaVendorStr);

        /* Set the OTA status and system state to RUN */
        sDisplayInfo.iOTAStatus = OTA_INPROGRESS;
        cTaskOwnState           = OTA_UPDATE_STATE_RUN;

        Display();

        break;

    case OTA_UPDATE_STATE_RUN:

        iRet = sl_extLib_OtaRun(pvOtaApp);

        if ( iRet < 0 )
        {

            if( RUN_STAT_ERROR_CONTINUOUS_ACCESS_FAILURES == iRet )
            {
              /* Reset the device */
              sDisplayInfo.iOTAStatus = OTA_ERROR;
              NetStop();
              NetStart();
              /* Get the firmware version Info */
              NetFwInfoGet(&sDisplayInfo.sNwpVersion);
              UpdateFwString();

              /* Update the OTA sample file version string */
              UpdateFileString();

              retVal = iRet;
            }
            else
            {
              sDisplayInfo.iOTAStatus = OTA_ERROR_RETRY;
            }
            Display();
        }
        else if( iRet == RUN_STAT_NO_UPDATES )
        {
            sDisplayInfo.iOTAStatus = OTA_NO_UPDATES;
            cTaskOwnState = OTA_UPDATE_STATE_INIT;
            retVal =OTA_RET_COMPLETE;
            Display();
        }
        else if ((iRet & RUN_STAT_DOWNLOAD_DONE))
        {
            /* Set OTA File for testing */
            iRet = sl_extLib_OtaSet(pvOtaApp, EXTLIB_OTA_SET_OPT_IMAGE_TEST,
                                    sizeof(_i32), (_u8 *)&SetCommitInt);

            sDisplayInfo.iOTAStatus = OTA_DONE;

            if (iRet & (OTA_ACTION_RESET_NWP) )
            {
                NetStop();
                NetStart();
                /* Get the firmware version Info */
                NetFwInfoGet(&sDisplayInfo.sNwpVersion);
                UpdateFwString();
            }
            cTaskOwnState = OTA_UPDATE_STATE_DONE;
            retVal = OTA_RET_COMPLETE;

            /* Update the OTA sample file version string */
            UpdateFileString();

            Display();
        }
        else if(sDisplayInfo.iOTAStatus == OTA_ERROR_RETRY)
        {
          sDisplayInfo.iOTAStatus = OTA_INPROGRESS;
          Display();
        }
        break;

    case OTA_UPDATE_STATE_DONE:
        cTaskOwnState = OTA_UPDATE_STATE_WAIT_START;


    default:
      return FALIURE;

    }

    return retVal;
}

void main()
{

    _i32 Status = 0;

    g_SecurityParams.Key = (_i8 *)PASSKEY;
    g_SecurityParams.KeyLen = PASSKEY_LEN;
    g_SecurityParams.Type = SEC_TYPE;

    /* Stop WDT and initialize the system-clock of the MCU */
    stopWDT();
    initClk();

    /* Configure command line interface */
    CLI_Configure();

    Status = configureSimpleLinkToDefaultState();
    if(Status < 0)
    {
        CLI_Write((_u8 *)" Failed to configure the device in its default state \n\r");
        LOOP_FOREVER();
    }

    Status = NetStart();
    if(Status < 0)
        LOOP_FOREVER();

    /* Initialize OTA */
    pvOtaApp = sl_extLib_OtaInit(RUN_MODE_NONE_OS | RUN_MODE_BLOCKING,0);

    /* Initialize Display Info */
    memset(&sDisplayInfo,0,sizeof(tDisplayInfo));

    /* Get the Firmware Version Info */
    Status = NetFwInfoGet(&sDisplayInfo.sNwpVersion);
    if(Status < 0)
        LOOP_FOREVER();

    UpdateFwString();

    /* Update the file version info */
    Status = UpdateFileString();
    if(Status < 0)
        LOOP_FOREVER();

    sDisplayInfo.ucLocalTime[0] = '-';
    sDisplayInfo.ucUTCTime[0]   = '-';

    sprintf((char *)sDisplayInfo.ucTimeZone,"%+03d:%02d",
          (_u8)GMT_DIFF_TIME_HRS,
          (_u8)GMT_DIFF_TIME_MINS);


    sprintf((char *)sDisplayInfo.ucAppVersion, APP_VERSION);

    /* Connect to the Access point */
    Status = NetWlanConnect(SSID_NAME, &g_SecurityParams);
    if(Status < 0)
        LOOP_FOREVER();

    DisplayGetNTPTime();

    while(1)
    {
        Status = OTAUpdateStep();
        /* Check if to sleep or to continue serve the processes */
        if(Status == OTA_RET_COMPLETE)
        {
            CLI_Write("OTA Update completed...");
            CLI_Write("Sleep for 10 Sec\r\n");
            Delay(DELAY_IN_SEC*1000);
        }

        if(Status == RUN_STAT_ERROR_CONTINUOUS_ACCESS_FAILURES)
        {
            CLI_Write("OTA Server Access Error...");
            CLI_Write("Sleep for 10 Sec\r\n");
            Delay(10*1000);
        }

        if (!NetIsConnectedToAP())
        {
            /* Disconnected from the AP Reconnect */
            CLI_Write("Device disconnected, reconnecting....\r\n");
            if(NetWlanConnect(SSID_NAME,&g_SecurityParams) < 0)
            {
                CLI_Write("Error connecting with AP\r\n");
                LOOP_FOREVER();
            }
        }

        if(Status == RUN_STAT_ERROR_CONTINUOUS_ACCESS_FAILURES
                || Status == OTA_RET_COMPLETE)
            DisplayGetNTPTime();
    }
}
