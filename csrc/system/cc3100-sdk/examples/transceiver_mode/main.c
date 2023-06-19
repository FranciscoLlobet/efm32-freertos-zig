/*
 * main.c - sample code for using transceiver mode
 *
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
 * Application Name     -   Transceiver mode
 * Application Overview -   This is a sample application demonstrating how to use
 *                          CC31xx's transceiver mode of operation. This is the
 *                          ability of a device to send data directly over the WLAN
 *                          MAC layer without being connected to a WLAN AP.
 * Application Details  -   http://processors.wiki.ti.com/index.php/CC31xx_Transceiver_Mode_Application
 *                          doc\examples\transceiver_mode.pdf
 */

#include "simplelink.h"
#include "sl_common.h"

#define APPLICATION_VERSION "1.3.0"

#define SL_STOP_TIMEOUT        0xFF

/* Power level tone valid range 0-15 */
#define POWER_LEVEL_TONE    1
/* Preamble value 0- short, 1- long */
#define PREAMBLE            1

#define RATE                RATE_11M

/* Channel (1-13) used during the tx and rx*/
#define CHANNEL             11

#define BUF_SIZE 1400
#define NO_OF_PACKETS 100

/* Application specific status/error codes */
typedef enum{
    DEVICE_NOT_IN_STATION_MODE = -0x7D0,        /* Choosing this number to avoid overlap w/ host-driver's error codes */

    STATUS_CODE_MAX = -0xBB8
}e_AppStatusCodes;

/*
 * GLOBAL VARIABLES -- Start
 */
_u8 g_Status = 0;

union
{
  _u8 BsdBuf[BUF_SIZE];
  _u32 demobuf[BUF_SIZE/4];
} uBuf;


const _u8 RawData_Ping[] = {
                   0x88,   /* version , type sub type */
                   0x02,   /* Frame control flag */
                   0x2C, 0x00,
                   0x00, 0x23, 0x75, 0x55,0x55, 0x55,   /* destination */
                   0x00, 0x22, 0x75, 0x55,0x55, 0x55,   /* bssid */
                   0x00, 0x22, 0x75, 0x55,0x55, 0x55,   /* source */
                   0x80, 0x42, 0x00, 0x00,
                   0xAA, 0xAA, 0x03, 0x00, 0x00, 0x00, 0x08, 0x00, /* LLC */
                   /*---- ip header start -----*/
                   0x45, 0x00, 0x00, 0x54, 0x96, 0xA1, 0x00, 0x00, 0x40, 0x01,
                   0x57, 0xFA,                          /* checksum */
                   0xc0, 0xa8, 0x01, 0x64,              /* src ip */
                   0xc0, 0xa8, 0x01, 0x65,              /* dest ip  */
                   /* payload - ping/icmp */
                   0x08, 0x00, 0xA5, 0x51,
                   0x5E, 0x18, 0x00, 0x00, 0x41, 0x08, 0xBB, 0x8D, 0x00, 0x00,
                   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                   0x00, 0x00, 0x00, 0x00};
/*
 * GLOBAL VARIABLES -- End
 */

/*
 * STATIC FUNCTION DEFINITIONS -- Start
 */
static _i32 configureSimpleLinkToDefaultState();
static _i32 initializeAppVariables();
static _i32 RxStatisticsCollect(_i16 channel);
static _i32 TxContinues(_i16 channel);
static void displayBanner();
/*
 * STATIC FUNCTION DEFINITIONS -- End
 */

/*
 * ASYNCHRONOUS EVENT HANDLERS -- Start
 */
/*!
    \brief This function handles WLAN events

    \param[in]      pWlanEvent is the event passed to the handler

    \return         None

    \note

    \warning
*/
void SimpleLinkWlanEventHandler(SlWlanEvent_t *pWlanEvent)
{
    if(pWlanEvent == NULL)
    {
        CLI_Write(" [WLAN EVENT] NULL Pointer Error \n\r");
        return;
    }
    
    switch(pWlanEvent->Event)
    {
        case SL_WLAN_CONNECT_EVENT:
        {
            SET_STATUS_BIT(g_Status, STATUS_BIT_CONNECTION);

            /*
             * Information about the connected AP (like name, MAC etc) will be
             * available in 'slWlanConnectAsyncResponse_t' - Applications
             * can use it if required
             *
             * slWlanConnectAsyncResponse_t *pEventData = NULL;
             * pEventData = &pWlanEvent->EventData.STAandP2PModeWlanConnected;
             *
             */
        }
        break;

        case SL_WLAN_DISCONNECT_EVENT:
        {
            slWlanConnectAsyncResponse_t*  pEventData = NULL;

            CLR_STATUS_BIT(g_Status, STATUS_BIT_CONNECTION);

            pEventData = &pWlanEvent->EventData.STAandP2PModeDisconnected;

            /* If the user has initiated 'Disconnect' request, 'reason_code' is
             * SL_USER_INITIATED_DISCONNECTION */
            if(SL_WLAN_DISCONNECT_USER_INITIATED_DISCONNECTION == pEventData->reason_code)
            {
                CLI_Write(" Device disconnected from the AP on application's request \n\r");
            }
            else
            {
                CLI_Write(" Device disconnected from the AP on an ERROR..!! \n\r");
            }
        }
        break;

        default:
        {
            CLI_Write(" [WLAN EVENT] Unexpected event \n\r");
        }
        break;
    }
}

/*!
    \brief This function handles events for IP address acquisition via DHCP
           indication

    \param[in]      pNetAppEvent is the event passed to the handler

    \return         None

    \note

    \warning
*/
void SimpleLinkNetAppEventHandler(SlNetAppEvent_t *pNetAppEvent)
{
    if(pNetAppEvent == NULL)
    {
        CLI_Write(" [NETAPP EVENT] NULL Pointer Error \n\r");
        return;
    }
 
    switch(pNetAppEvent->Event)
    {
        case SL_NETAPP_IPV4_IPACQUIRED_EVENT:
        {
            SET_STATUS_BIT(g_Status, STATUS_BIT_IP_ACQUIRED);

            /*
             * Information about the connection (like IP, gateway address etc)
             * will be available in 'SlIpV4AcquiredAsync_t'
             * Applications can use it if required
             *
             * SlIpV4AcquiredAsync_t *pEventData = NULL;
             * pEventData = &pNetAppEvent->EventData.ipAcquiredV4;
             *
             */
        }
        break;

        default:
        {
            CLI_Write(" [NETAPP EVENT] Unexpected event \n\r");
        }
        break;
    }
}

/*!
    \brief This function handles callback for the HTTP server events

    \param[in]      pHttpEvent - Contains the relevant event information
    \param[in]      pHttpResponse - Should be filled by the user with the
                    relevant response information

    \return         None

    \note

    \warning
*/
void SimpleLinkHttpServerCallback(SlHttpServerEvent_t *pHttpEvent,
                                  SlHttpServerResponse_t *pHttpResponse)
{
    /* Unused in this application */
    CLI_Write(" [HTTP EVENT] Unexpected event \n\r");
}

/*!
    \brief This function handles general error events indication

    \param[in]      pDevEvent is the event passed to the handler

    \return         None
*/
void SimpleLinkGeneralEventHandler(SlDeviceEvent_t *pDevEvent)
{
    /*
     * Most of the general errors are not FATAL are are to be handled
     * appropriately by the application
     */
    CLI_Write(" [GENERAL EVENT] \n\r");
}

/*!
    \brief This function handles socket events indication

    \param[in]      pSock is the event passed to the handler

    \return         None
*/
void SimpleLinkSockEventHandler(SlSockEvent_t *pSock)
{
    if(pSock == NULL)
    {
        CLI_Write(" [SOCK EVENT] NULL Pointer Error \n\r");
        return;
    }
    
    switch( pSock->Event )
    {
        case SL_SOCKET_TX_FAILED_EVENT:
            /*
             * TX Failed
             *
             * Information about the socket descriptor and status will be
             * available in 'SlSockEventData_t' - Applications can use it if
             * required
             *
            * SlSockEventData_u *pEventData = NULL;
            * pEventData = & pSock->socketAsyncEvent;
             */
            switch( pSock->socketAsyncEvent.SockTxFailData.status )
            {
                case SL_ECLOSE:
                    CLI_Write(" [SOCK EVENT] Close socket operation failed to transmit all queued packets\n\r");
                    break;
                default:
                    CLI_Write(" [SOCK EVENT] Unexpected event \n\r");
                    break;
            }
            break;

        default:
            CLI_Write(" [SOCK EVENT] Unexpected event \n\r");
            break;
    }
}
/*
 * ASYNCHRONOUS EVENT HANDLERS -- End
 */

/*
 * Application's entry point
 */
int main(int argc, char** argv)
{
    _i32 retVal = -1;

    retVal = initializeAppVariables();
    ASSERT_ON_ERROR(retVal);

    /* Stop WDT and initialize the system-clock of the MCU
       These functions needs to be implemented in PAL */
    stopWDT();
    initClk();

    /* Configure command line interface */
    CLI_Configure();

    displayBanner();

    /*
     * Following function configures the device to default state by cleaning
     * the persistent settings stored in NVMEM (viz. connection profiles &
     * policies, power policy etc)
     *
     * Applications may choose to skip this step if the developer is sure
     * that the device is in its default state at start of application
     *
     * Note that all profiles and persistent settings that were done on the
     * device will be lost
     */
    retVal = configureSimpleLinkToDefaultState();
    if(retVal < 0)
    {
        CLI_Write(" Failed to configure the device in its default state \n\r");

        LOOP_FOREVER();
    }

    CLI_Write(" Device is configured in default state \n\r");

    /*
     * Assumption is that the device is configured in station mode already
     * and it is in its default state
     */
    /* Initializing the CC3100 device */
    retVal = sl_Start(0, 0, 0);
    if ((retVal < 0) ||
        (ROLE_STA != retVal) )
    {
        CLI_Write(" Failed to start the device \n\r");
        LOOP_FOREVER();
    }

    CLI_Write(" Device started as STATION \n\r");

    CLI_Write(" Sending raw data over wlan PHY \n\r");

    /* Transmits raw packets over the configured channel. There should be
     * minimum 50 ms delay between each packet */
    retVal = TxContinues(CHANNEL);
    if(retVal < 0)
        CLI_Write(" Error in sending raw data over wlan PHY \n\r");
    else
        CLI_Write(" Raw data sent over wlan PHY successfully\n\r");

    /* Wait 500 msec */
    Delay(500);

    CLI_Write(" Collecting Rx statistics \n\r");

    retVal = RxStatisticsCollect(CHANNEL);
    if(retVal < 0)
        CLI_Write(" Error while collecting Rx statistics \n\r");
    else
        CLI_Write(" Rx statistics collected successfully \n\r");

    /* Stop the CC3100 device */
    retVal = sl_Stop(SL_STOP_TIMEOUT);
    if(retVal < 0)
        LOOP_FOREVER();

    return 0;
}

/*!
    \brief This function configure the SimpleLink device in its default state. It:
           - Sets the mode to STATION
           - Configures connection policy to Auto and AutoSmartConfig
           - Deletes all the stored profiles
           - Enables DHCP
           - Disables Scan policy
           - Sets Tx power to maximum
           - Sets power policy to normal
           - Unregisters mDNS services
           - Remove all filters

    \param[in]      none

    \return         On success, zero is returned. On error, negative is returned
*/
static _i32 configureSimpleLinkToDefaultState()
{
    SlVersionFull   ver = {0};
    _WlanRxFilterOperationCommandBuff_t  RxFilterIdMask = {0};

    _u8           val = 1;
    _u8           configOpt = 0;
    _u8           configLen = 0;
    _u8           power = 0;

    _i32          retVal = -1;
    _i32          mode = -1;

    mode = sl_Start(0, 0, 0);
    ASSERT_ON_ERROR(mode);

    /* If the device is not in station-mode, try configuring it in station-mode */
    if (ROLE_STA != mode)
    {
        if (ROLE_AP == mode)
        {
            /* If the device is in AP mode, we need to wait for this event before doing anything */
            while(!IS_IP_ACQUIRED(g_Status)) { _SlNonOsMainLoopTask(); }
        }

        /* Switch to STA role and restart */
        retVal = sl_WlanSetMode(ROLE_STA);
        ASSERT_ON_ERROR(retVal);

        retVal = sl_Stop(SL_STOP_TIMEOUT);
        ASSERT_ON_ERROR(retVal);

        retVal = sl_Start(0, 0, 0);
        ASSERT_ON_ERROR(retVal);

        /* Check if the device is in station again */
        if (ROLE_STA != retVal)
        {
            /* We don't want to proceed if the device is not coming up in station-mode */
            ASSERT_ON_ERROR(DEVICE_NOT_IN_STATION_MODE);
        }
    }

    /* Get the device's version-information */
    configOpt = SL_DEVICE_GENERAL_VERSION;
    configLen = sizeof(ver);
    retVal = sl_DevGet(SL_DEVICE_GENERAL_CONFIGURATION, &configOpt, &configLen, (_u8 *)(&ver));
    ASSERT_ON_ERROR(retVal);

    /* Set connection policy to Auto + SmartConfig (Device's default connection policy) */
    retVal = sl_WlanPolicySet(SL_POLICY_CONNECTION, SL_CONNECTION_POLICY(1, 0, 0, 0, 1), NULL, 0);
    ASSERT_ON_ERROR(retVal);

    /* Remove all profiles */
    retVal = sl_WlanProfileDel(0xFF);
    ASSERT_ON_ERROR(retVal);

    /*
     * Device in station-mode. Disconnect previous connection if any
     * The function returns 0 if 'Disconnected done', negative number if already disconnected
     * Wait for 'disconnection' event if 0 is returned, Ignore other return-codes
     */
    retVal = sl_WlanDisconnect();
    if(0 == retVal)
    {
        /* Wait */
        while(IS_CONNECTED(g_Status)) { _SlNonOsMainLoopTask(); }
    }

    /* Enable DHCP client*/
    retVal = sl_NetCfgSet(SL_IPV4_STA_P2P_CL_DHCP_ENABLE,1,1,&val);
    ASSERT_ON_ERROR(retVal);

    /* Disable scan */
    configOpt = SL_SCAN_POLICY(0);
    retVal = sl_WlanPolicySet(SL_POLICY_SCAN , configOpt, NULL, 0);
    ASSERT_ON_ERROR(retVal);

    /* Set Tx power level for station mode
       Number between 0-15, as dB offset from max power - 0 will set maximum power */
    power = 0;
    retVal = sl_WlanSet(SL_WLAN_CFG_GENERAL_PARAM_ID, WLAN_GENERAL_PARAM_OPT_STA_TX_POWER, 1, (_u8 *)&power);
    ASSERT_ON_ERROR(retVal);

    /* Set PM policy to normal */
    retVal = sl_WlanPolicySet(SL_POLICY_PM , SL_NORMAL_POLICY, NULL, 0);
    ASSERT_ON_ERROR(retVal);

    /* Unregister mDNS services */
    retVal = sl_NetAppMDNSUnRegisterService(0, 0);
    ASSERT_ON_ERROR(retVal);

    /* Remove  all 64 filters (8*8) */
    pal_Memset(RxFilterIdMask.FilterIdMask, 0xFF, 8);
    retVal = sl_WlanRxFilterSet(SL_REMOVE_RX_FILTER, (_u8 *)&RxFilterIdMask,
                       sizeof(_WlanRxFilterOperationCommandBuff_t));
    ASSERT_ON_ERROR(retVal);

    retVal = sl_Stop(SL_STOP_TIMEOUT);
    ASSERT_ON_ERROR(retVal);

    retVal = initializeAppVariables();
    ASSERT_ON_ERROR(retVal);

    return retVal; /* Success */
}

/*!
    \brief Entering raw Transmitter\Receiver mode in order to send raw data
           over the WLAN PHY

    This function shows how to send raw data, in this case ping packets over
    the air in transmitter mode.

    \param[in]      Channel number on which the data will be sent

    \return         0 on success, Negative on Error.

    \note

    \warning        We must be disconnected from WLAN AP in order to succeed
                    changing to transmitter mode
*/
static _i32 TxContinues(_i16 channel)
{
    _i16 SockID = 0;
    _i32 Status = 0;
    _u16 Len = 0;
    _i16 count = 0;

    /*
     * Disconnect previous connection if any
     * The function returns 0 if 'Disconnected done', negative number if already disconnected
     * Wait for 'disconnection' event if 0 is returned, Ignore other return-codes
     */
    Status = sl_WlanDisconnect();
    if(0 == Status)
    {
        /* Wait */
        while(IS_CONNECTED(g_Status)) { _SlNonOsMainLoopTask(); }
    }

    /* make sure device is disconnected & auto mode is off */
    SockID = sl_Socket(SL_AF_RF,SL_SOCK_RAW,channel);
    ASSERT_ON_ERROR(SockID);

    Len = sizeof(RawData_Ping);

    while (count < NO_OF_PACKETS)
    {
        Status = sl_Send(SockID, RawData_Ping, Len,
               SL_RAW_RF_TX_PARAMS(channel, RATE, POWER_LEVEL_TONE, PREAMBLE));
        if(Status < 0)
        {
               sl_Close(SockID);
               ASSERT_ON_ERROR(Status);
           }

           count++;
        /* 100 ms Delay between each packet*/
        Delay(100);
    }

    Status = sl_Close(SockID);
    ASSERT_ON_ERROR(Status);

    return SUCCESS;
}


/*!
    \brief Entering raw Transmitter\Receiver mode in order to perform Rx
           statistic over a specific WLAN channel


    \param[in]      Channel number on which the statistics will be calculated

    \return         0 on success, Negative on Error.

    \note

    \warning        We must be disconnected from WLAN AP in order to succeed
                    changing to Receiver mode
*/
static _i32 RxStatisticsCollect(_i16 channel)
{
    SlGetRxStatResponse_t rxStatResp = {0};
    SlTimeval_t     timeVal = {0};

    _i16 SockID = -1;
    _i32 Status = -1;

    /*
     * Disconnect previous connection if any
     * The function returns 0 if 'Disconnected done', negative number if already disconnected
     * Wait for 'disconnection' event if 0 is returned, Ignore other return-codes
     */
    Status = sl_WlanDisconnect();
    if(0 == Status)
    {
        /* Wait */
        while(IS_CONNECTED(g_Status)) { _SlNonOsMainLoopTask(); }
    }

    SockID = sl_Socket(SL_AF_RF,SL_SOCK_RAW,channel);
    ASSERT_ON_ERROR(SockID);

    timeVal.tv_sec = 1;
    timeVal.tv_usec = 0;

    Status = sl_SetSockOpt(SockID, SL_SOL_SOCKET, SL_SO_RCVTIMEO,
                                &timeVal, sizeof(timeVal));
    if( Status < 0 )
    {
        sl_Close(SockID);
        ASSERT_ON_ERROR(Status);
    }

    /*We need to call sl_Recv() at least once before starting Rx statistics
    * engine */
    Status = sl_Recv(SockID, uBuf.BsdBuf, BUF_SIZE, 0);
    if (Status < 0)
    {
        sl_Close(SockID);
        ASSERT_ON_ERROR(Status);
    }

    /* starting to collect statistics */
    Status = sl_WlanRxStatStart();
    if (Status < 0)
    {
        sl_Close(SockID);
        ASSERT_ON_ERROR(Status);
    }

    /* Delay of 2 second to give CC3100 time to collect statistics */
    Delay(2000);

    /* Get the statistics */
    Status = sl_WlanRxStatGet(&rxStatResp, 0);
    if (Status < 0)
    {
        sl_Close(SockID);
        ASSERT_ON_ERROR(Status);
    }

    /* ********** Rx Statistics*********/
    /* Received Packets = rxStatResp.ReceivedValidPacketsNumber */
    /* Received FCS = rxStatResp.ReceivedFcsErrorPacketsNumber */
    /* Received Address Mismatch Error = 
	 *                    rxStatResp.ReceivedAddressMismatchPacketsNumber */
    /* Average Rssi for management = rxStatResp.AvarageMgMntRssi */
    /* Average Rssi for other packets = rxStatResp.AvarageDataCtrlRssi */
    /* Rssi Histogram cell = rxStatResp.RssiHistogram[i],
                                            i:{0,SIZE_OF_RSSI_HISTOGRAM} */
    /* Rate Histogram cell = rxStatResp.RateHistogram[i],
                                            i:{0,NUM_OF_RATE_INDEXES} */
    /* Sample Time = rxStatResp.GetTimeStamp - rxStatResp.StartTimeStamp */

    Status = sl_WlanRxStatStop();
    ASSERT_ON_ERROR(Status);

    Status = sl_Close(SockID);
    ASSERT_ON_ERROR(Status);

    return SUCCESS;
}

/*!
    \brief This function initializes the application variables

    \param[in]  None

    \return     0 on success, negative error-code on error
*/
static _i32 initializeAppVariables()
{
    g_Status = 0;
    pal_Memset(uBuf.BsdBuf, 0, sizeof(uBuf));

    return SUCCESS;
}

/*!
    \brief This function displays the application's banner

    \param      None

    \return     None
*/
static void displayBanner()
{
    CLI_Write("\n\r\n\r");
    CLI_Write(" Transceiver mode application - Version ");
    CLI_Write(APPLICATION_VERSION);
    CLI_Write("\n\r*******************************************************************************\n\r");
}
