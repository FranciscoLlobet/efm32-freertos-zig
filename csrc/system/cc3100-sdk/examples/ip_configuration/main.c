/*
 * main.c - DHCP/IP configuration sample example
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
 * Application Name     -   IP Configuration
 * Application Overview -   This sample application demonstrates how to
 *                          configure a static IP address for CC3100. The
 *                          device connects to the AP w/ the configured static
 *                          IP. The IP address will be stored in the non-volatile
 *                          memory.
 * Application Details  -   http://processors.wiki.ti.com/index.php/CC31xx_IP_Configuration_Application
 *                          doc\examples\ip_configuration.pdf
 */

#include "simplelink.h"
#include "sl_common.h"

#define APPLICATION_VERSION "1.3.0"

#define SL_STOP_TIMEOUT        0xFF

/* Define 'USE_DHCP' to put the device in DHCP mode */
/*#define USE_DHCP */

#ifndef USE_DHCP
#define CONFIG_IP       SL_IPV4_VAL(192,168,1,45)       /* Static IP to be configured */
#define AP_MASK         SL_IPV4_VAL(255,255,255,0)      /* Subnet Mask for the station */
#define AP_GATEWAY      SL_IPV4_VAL(192,168,1,1)        /* Default Gateway address */
#define AP_DNS          SL_IPV4_VAL(0,0,0,0)            /* DNS Server Address */
#endif /* USE_DHCP */

/* Application specific status/error codes */
typedef enum{
    LAN_CONNECTION_FAILED = -0x7D0,        /* Choosing this number to avoid overlap w/ host-driver's error codes */
    INTERNET_CONNECTION_FAILED = LAN_CONNECTION_FAILED - 1,
    DEVICE_NOT_IN_STATION_MODE = INTERNET_CONNECTION_FAILED - 1,

    STATUS_CODE_MAX = -0xBB8
}e_AppStatusCodes;

/*
 * GLOBAL VARIABLES -- Start
 */
_u32  g_Status = 0;
/*
 * GLOBAL VARIABLES -- End
 */


/*
 * STATIC FUNCTION DEFINITIONS -- Start
 */
static _i32 configureSimpleLinkToDefaultState();
static _i32 establishConnectionWithAP();
static _i32 initializeAppVariables();
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
             CLR_STATUS_BIT(g_Status, STATUS_BIT_IP_ACQUIRED);

             pEventData = &pWlanEvent->EventData.STAandP2PModeDisconnected;

             /* If the user has initiated 'Disconnect' request, 'reason_code' is SL_USER_INITIATED_DISCONNECTION */
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
             * Information about the connected AP's IP, gateway, DNS etc
             * will be available in 'SlIpV4AcquiredAsync_t' - Applications
             * can use it if required
             *
             * SlIpV4AcquiredAsync_t *pEventData = NULL;
             * pEventData = &pNetAppEvent->EventData.ipAcquiredV4;
             * <gateway_ip> = pEventData->gateway;
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
    /*
     * This application doesn't work with HTTP server - Hence these
     * events are not handled here
     */
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
    /*
     * This application doesn't work with socket - Hence these
     * events are not handled here
     */
    CLI_Write(" [SOCK EVENT] Unexpected event \n\r");
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

#ifdef USE_DHCP
    _u8 val = 1;
#else
    SlNetCfgIpV4Args_t ipV4;
#endif
    retVal = initializeAppVariables();
    ASSERT_ON_ERROR(retVal);

    /* Stop WDT and initialize the system-clock of the MCU */
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
        if (DEVICE_NOT_IN_STATION_MODE == retVal)
            CLI_Write(" Failed to configure the device in its default state \n\r");

        LOOP_FOREVER();
    }

    CLI_Write(" Device is configured in default state \n\r");

    /*
     * Assumption is that the device is configured in station mode already
     * and it is in its default state
     */
    retVal = sl_Start(0, 0, 0);
    if ((retVal < 0) ||
        (ROLE_STA != retVal) )
    {
        CLI_Write(" Failed to start the device \n\r");
        LOOP_FOREVER();
    }

    CLI_Write(" Device started as STATION \n\r");

#ifdef USE_DHCP
    CLI_Write(" Configuring device in DHCP mode \n\r");

    /* After calling this API device will be configured for DHCP mode */
    retVal = sl_NetCfgSet(SL_IPV4_STA_P2P_CL_DHCP_ENABLE,1,1,&val);
    if(retVal < 0)
        LOOP_FOREVER();

    /* Restart the device */
    retVal = sl_Stop(SL_STOP_TIMEOUT);
    if(retVal < 0)
        LOOP_FOREVER();

    retVal = sl_Start(0, 0, 0);
    if ((retVal < 0) ||
        (ROLE_STA != retVal) )
    {
        CLI_Write(" Failed to start the device \n\r");
        LOOP_FOREVER();
    }
#else /* USE_DHCP */
    ipV4.ipV4 = CONFIG_IP;
    ipV4.ipV4Mask = AP_MASK;
    ipV4.ipV4Gateway = AP_GATEWAY;
    ipV4.ipV4DnsServer = AP_DNS;

    CLI_Write(" Configuring device to connect using static IP \n\r");

    /* After calling this API device will be configure for static IP address.*/
    retVal = sl_NetCfgSet(SL_IPV4_STA_P2P_CL_STATIC_ENABLE,1,
                    sizeof(SlNetCfgIpV4Args_t), (_u8 *)&ipV4);
    if(retVal < 0)
        LOOP_FOREVER();

#endif /* USE_DHCP */

    /* Connecting to WLAN AP */
    retVal = establishConnectionWithAP();
    if(retVal < 0)
    {
        CLI_Write(" Failed to establish connection w/ an AP \n\r");
        LOOP_FOREVER();
    }

    CLI_Write(" Connection established w/ AP and IP is configured \n\r");
    return SUCCESS;
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
       Number between 0-15, as dB offset from maximum power - 0 will set maximum power */
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
    \brief Connecting to a WLAN Access point

    This function connects to the required AP (SSID_NAME).
    The function will return once we are connected and have acquired IP address

    \param[in]  None

    \return     0 on success, negative error-code on error

    \note

    \warning    If the WLAN connection fails or we don't acquire an IP address,
                We will be stuck in this function forever.
*/
static _i32 establishConnectionWithAP()
{
    SlSecParams_t secParams = {0};
    _i32 retVal = 0;

    secParams.Key = PASSKEY;
    secParams.KeyLen = pal_Strlen(PASSKEY);
    secParams.Type = SEC_TYPE;

    retVal = sl_WlanConnect(SSID_NAME, pal_Strlen(SSID_NAME), 0, &secParams, 0);
    ASSERT_ON_ERROR(retVal);

    /* Wait */
    while((!IS_CONNECTED(g_Status)) || (!IS_IP_ACQUIRED(g_Status))) { _SlNonOsMainLoopTask(); }

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
    CLI_Write(" IP configuration application - Version ");
    CLI_Write(APPLICATION_VERSION);
    CLI_Write("\n\r*******************************************************************************\n\r");
}
