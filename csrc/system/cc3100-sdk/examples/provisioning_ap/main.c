/*
 * main.c - sample application for Simplelink provisioning
 *
 * Copyright (C) 2016 Texas Instruments Incorporated - http://www.ti.com/
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
 * Application Name     -   Simplelink Provisioning
 * Application Overview -   This is a sample application demonstrating how CC3100's
 *                          provisioning.
 * Application Details  -   http://processors.wiki.ti.com/index.php/CC31xx_Provisioning_AP
 *                          doc\examples\simplelink_provisioning.pdf
 */

#include "simplelink.h"
#include "protocol.h"
#include "sl_common.h"
#include "provisioning_api.h"
#include "provisioning_defs.h"

#include "timer_if.h"


#define APPLICATION_VERSION "1.3.0"

#define SL_STOP_TIMEOUT        0xFF

#define NO_AP        "__No_AP_Available__"

#define SCAN_TABLE_SIZE  20
#define SCAN_INTERVAL    10
#define MAX_KEY_LENGTH   32

#define GET_TOKEN    "__SL_G_U"

#define GET_STATUS   "__SL_G_S.0"

#define SL_PARAM_PRODUCT_VERSION_DATA "R1.0"

#define PROVISIONING_TIMEOUT             300 //Number of seconds to wait for provisioning completion

/* Use bit 32: Lower bits of status variable are used for NWP events
 *
 *      1 in a 'status_variable', profile is added form web page
 *      0 in a 'status_variable', profile is not added form web page
 */
#define STATUS_BIT_PROFILE_ADDED 31
/* Use bit 31
 *      1 in a 'status_variable', successfully connected to AP
 *      0 in a 'status_variable', AP connection was not successful
 */
#define STATUS_BIT_CONNECTED_TO_CONF_AP (STATUS_BIT_PROFILE_ADDED - 1)
/* Use bit 30
 *      1 in a 'status_variable', AP provisioning process completed
 *      0 in a 'status_variable', AP provisioning process yet to complete
 */
#define STATUS_BIT_PROVISIONING_DONE (STATUS_BIT_CONNECTED_TO_CONF_AP - 1)

/* Application specific status/error codes */
typedef enum{
    LAN_CONNECTION_FAILED = -0x7D0,        /* Choosing this number to avoid overlap with host-driver's error codes */
    INTERNET_CONNECTION_FAILED = LAN_CONNECTION_FAILED - 1,
    DEVICE_NOT_IN_STATION_MODE = INTERNET_CONNECTION_FAILED - 1,       /* Choosing this number to avoid overlap w/ host-driver's error codes */
    PROVISIONING_AP_FAILED      = DEVICE_NOT_IN_STATION_MODE -1,

    STATUS_CODE_MAX = -0xBB8
}e_AppStatusCodes;

#define IS_PROFILE_ADDED(status_variable)          GET_STATUS_BIT(status_variable, \
                                                               STATUS_BIT_PROFILE_ADDED)
#define IS_CONNECTED_TO_CONF_AP(status_variable)          GET_STATUS_BIT(status_variable, \
                                                               STATUS_BIT_CONNECTED_TO_CONF_AP)
#define IS_PROVISIONING_DONE(status_variable)          GET_STATUS_BIT(status_variable, \
                                                               STATUS_BIT_PROVISIONING_DONE)

#define IS_PING_DONE(status_variable)           GET_STATUS_BIT(status_variable, \
                                                               STATUS_BIT_PING_DONE)


#define STATUS_BIT_PING_DONE  31

#define HOST_NAME       "www.ti.com"

// #define PROV_SMART_CONFIG
/*
 * Values for below macros shall be modified for setting the 'Ping' properties
 */
#define PING_INTERVAL   1000    /* In msecs */
#define PING_TIMEOUT    3000    /* In msecs */
#define PING_PKT_SIZE   20      /* In bytes */
#define NO_OF_ATTEMPTS  3
/*
 * GLOBAL VARIABLES -- Start
 */
_u32    g_Status = 0;
_u32   g_ConnectTimeoutCnt = 0;
_u32  g_PingPacketsRecv = 0;
_u32  g_GatewayIP = 0;

_i8 g_Wlan_SSID[MAXIMAL_SSID_LENGTH + 1];
_i8 g_Wlan_Security_Key[MAX_KEY_LENGTH];

Sl_WlanNetworkEntry_t g_netEntries[SCAN_TABLE_SIZE];
_u8 g_ssid_list[SCAN_TABLE_SIZE][MAXIMAL_SSID_LENGTH + 1];

SlSecParams_t g_secParams;

_u8 volatile g_TimerBTimedOut;

/*
 * GLOBAL VARIABLES -- End
 */

/*
 * STATIC FUNCTION DEFINITIONS -- Start
 */
static _i32 initializeAppVariables();
static void displayBanner();
_i32 startProvisioning (void);
long Network_IF_InitDriver(unsigned int uiMode);
/*
 * STATIC FUNCTION DEFINITIONS -- End
 */

void generalTimeoutHandler(void)
{
    Timer_IF_InterruptClear(TIMER2);
    g_TimerBTimedOut++;
}

void waitmSec(_i32 timeout)
{
    //Initializes & Starts timer
    Timer_IF_Init(TIMER2,ONE_SHOT);
    Timer_IF_IntSetup(TIMER2, generalTimeoutHandler);

    g_TimerBTimedOut = 0;

    Timer_IF_Start(TIMER2, timeout);

    while(g_TimerBTimedOut != 1)
    {
        // waiting...
#ifndef SL_PLATFORM_MULTI_THREADED
        _SlNonOsMainLoopTask();
#endif
    }

    //Stops timer
    Timer_IF_Stop(TIMER2);
    Timer_IF_DeInit(TIMER2);
}

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
            // CLI_Write(" [WLAN EVENT] Connect event \n\r");
        }
        break;

        case SL_WLAN_DISCONNECT_EVENT:
        {
            slWlanConnectAsyncResponse_t*  pEventData = NULL;

            CLR_STATUS_BIT(g_Status, STATUS_BIT_CONNECTION);
            CLR_STATUS_BIT(g_Status, STATUS_BIT_IP_ACQUIRED);

            pEventData = &pWlanEvent->EventData.STAandP2PModeDisconnected;

            /* If the user has initiated 'Disconnect' request, 'reason_code' is
             * SL_USER_INITIATED_DISCONNECTION */
            if(SL_WLAN_DISCONNECT_USER_INITIATED_DISCONNECTION == pEventData->reason_code)
            {
                /* Device disconnected from the AP on application's request */
            }
            else
            {
                CLI_Write(" Device disconnected from the AP on an ERROR..!! \n\r");
            }
        }
        break;

        case SL_WLAN_STA_CONNECTED_EVENT:
            /*
             * Information about the connected STA (like name, MAC etc) will be
             * available in 'slPeerInfoAsyncResponse_t' - Applications
             * can use it if required
             *
             * slPeerInfoAsyncResponse_t *pEventData = NULL;
             * pEventData = &pWlanEvent->EventData.APModeStaConnected;
             *
             */
        	// CLI_Write(" [WLAN EVENT] STA Conneected event \n\r");
            break;

        case SL_WLAN_STA_DISCONNECTED_EVENT:
            /*
             * Information about the connected STA (device name, MAC) will be
             * available in 'slPeerInfoAsyncResponse_t' - Applications
             * can use it if required
             *
             * slPeerInfoAsyncResponse_t *pEventData = NULL;
             * pEventData = &pWlanEvent->EventData.APModeStaConnected;
             *
             */
        	 CLI_Write(" [WLAN EVENT] STA Dis Conneected event \n\r");
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
 
    if(pNetAppEvent == NULL)
        CLI_Write(" [NETAPP EVENT] NULL Pointer Error \n\r");
 
    switch(pNetAppEvent->Event)
    {
        case SL_NETAPP_IPV4_IPACQUIRED_EVENT:
        {
            SlIpV4AcquiredAsync_t *pEventData = NULL;

            SET_STATUS_BIT(g_Status, STATUS_BIT_IP_ACQUIRED);

            pEventData = &pNetAppEvent->EventData.ipAcquiredV4;
            g_GatewayIP = pEventData->gateway;
        }
        // CLI_Write(" [NETAPP EVENT] IP ACQUIRED \n\r");

        break;

        case SL_NETAPP_IP_LEASED_EVENT:
            /*
             * Information about the connection (like leased IP, lease time etc)
             * will be available in 'SlIpLeasedAsync_t'
             * Applications can use it if required
             *
             * SlIpLeasedAsync_t *pEventData = NULL;
             * pEventData = &pNetAppEvent->EventData.ipLeased;
             *
             */
        	// CLI_Write(" [NETAPP EVENT] IP LEASED \n\r");
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

    \param[in]      pEvent - Contains the relevant event information
    \param[in]      pResponse - Should be filled by the user with the
                    relevant response information

    \return         None

    \note

    \warning
*/
void SimpleLinkHttpServerCallback(SlHttpServerEvent_t *pEvent,
                                  SlHttpServerResponse_t *pResponse)
{
    if(pEvent == NULL || pResponse == NULL)
    {
        CLI_Write(" [HTTP EVENT] NULL Pointer Error \n\r");
        return;
    }

    switch (pEvent->Event)
    {
        case SL_NETAPP_HTTPGETTOKENVALUE_EVENT:
        {
            /* HTTP get token */
            _u8 *ptr;
            _u8 num = 0;

            if (0== pal_Memcmp (pEvent->EventData.httpTokenName.data,
                    GET_STATUS,pEvent->EventData.httpTokenName.len))
            {
                /* Token to get the connection status */
                if(IS_CONNECTED_TO_CONF_AP(g_Status))
                {
                    pal_Strcpy (pResponse->ResponseData.token_value.data,
                            "TRUE");
                    pResponse->ResponseData.token_value.len = pal_Strlen("TRUE");
                }
                else
                {
                    pal_Strcpy (pResponse->ResponseData.token_value.data,
                            "FALSE");
                    pResponse->ResponseData.token_value.len = pal_Strlen("FALSE");
                }
            }
            else
            {
                /* Token to get the AP list */
                ptr = pResponse->ResponseData.token_value.data;
                pResponse->ResponseData.token_value.len = 0;
                if(pal_Memcmp(pEvent->EventData.httpTokenName.data, GET_TOKEN,pal_Strlen(GET_TOKEN)) == 0)
                {
                    num = pEvent->EventData.httpTokenName.data[8] - '0';
                    num *= 10;
                    num += pEvent->EventData.httpTokenName.data[9] - '0';

                    if(g_ssid_list[num][0] == '\0')
                    {
                        pal_Strcpy(ptr, NO_AP);
                    }
                    else
                    {
                        pal_Strcpy(ptr, g_ssid_list[num]);
                    }
                    pResponse->ResponseData.token_value.len = pal_Strlen(ptr);
                }
            }
            // CLI_Write(" [HTTP EVENT] GET TOKEN RECEIVED \n\r");
            break;
        }

        case SL_NETAPP_HTTPPOSTTOKENVALUE_EVENT:
        {
            /* HTTP post token */

            if ((0 == pal_Memcmp(pEvent->EventData.httpPostData.token_name.data, "__SL_P_U.C", pEvent->EventData.httpPostData.token_name.len)) &&
                (0 == pal_Memcmp(pEvent->EventData.httpPostData.token_value.data, "Connect", pEvent->EventData.httpPostData.token_value.len)))
            {
                SET_STATUS_BIT(g_Status, STATUS_BIT_PROFILE_ADDED);
            }
            if (0 == pal_Memcmp (pEvent->EventData.httpPostData.token_name.data, "__SL_P_U.D", pEvent->EventData.httpPostData.token_name.len))
            {
                pal_Memcpy (g_Wlan_SSID,  pEvent->EventData.httpPostData.token_value.data, pEvent->EventData.httpPostData.token_value.len);
                g_Wlan_SSID[pEvent->EventData.httpPostData.token_value.len] = 0;
            }
            if (0 == pal_Memcmp (pEvent->EventData.httpPostData.token_name.data, "__SL_P_U.E", pEvent->EventData.httpPostData.token_name.len))
            {
                if (pEvent->EventData.httpPostData.token_value.data[0] == '0')
                {
                    g_secParams.Type =  SL_SEC_TYPE_OPEN;

                }
                else if (pEvent->EventData.httpPostData.token_value.data[0] == '1')
                {
                    g_secParams.Type =  SL_SEC_TYPE_WEP;

                }
                else if (pEvent->EventData.httpPostData.token_value.data[0] == '2')
                {
                    g_secParams.Type =  SL_SEC_TYPE_WPA;
                }
                else if (pEvent->EventData.httpPostData.token_value.data[0] == '3')
                {
                    g_secParams.Type =  SL_SEC_TYPE_WPA;
                }
            }
            if (0 == pal_Memcmp (pEvent->EventData.httpPostData.token_name.data, "__SL_P_U.F", pEvent->EventData.httpPostData.token_name.len))
            {
                pal_Memcpy (g_Wlan_Security_Key,pEvent->EventData.httpPostData.token_value.data, pEvent->EventData.httpPostData.token_value.len);
                g_Wlan_Security_Key[pEvent->EventData.httpPostData.token_value.len] = 0;
                g_secParams.Key = g_Wlan_Security_Key;
                g_secParams.KeyLen = pEvent->EventData.httpPostData.token_value.len;
            }
            if (0 == pal_Memcmp (pEvent->EventData.httpPostData.token_name.data, "__SL_P_U.0", pEvent->EventData.httpPostData.token_name.len))
            {
                SET_STATUS_BIT(g_Status, STATUS_BIT_PROVISIONING_DONE);
            }
            // CLI_Write(" [HTTP EVENT] POST TOKEN RECEIVED \n\r");
            break;
        }
        default:
            break;
    }
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
     * This application doesn't work w/ socket - Hence not handling these events
     */
    CLI_Write(" [SOCK EVENT] Unexpected event \n\r");
}
/*
 * ASYNCHRONOUS EVENT HANDLERS -- End
 */

static void SimpleLinkPingReport(SlPingReport_t *pPingReport)
{
    SET_STATUS_BIT(g_Status, STATUS_BIT_PING_DONE);

    if(pPingReport == NULL)
    {
        CLI_Write((_u8 *)" [PING REPORT] NULL Pointer Error\r\n");
        return;
    }

    g_PingPacketsRecv = pPingReport->PacketsReceived;
}

static _i32 checkLanConnection()
{
    SlPingStartCommand_t pingParams = {0};
    SlPingReport_t pingReport = {0};

    _i32 retVal = -1;

    CLR_STATUS_BIT(g_Status, STATUS_BIT_PING_DONE);
    g_PingPacketsRecv = 0;

    /* Set the ping parameters */
    pingParams.PingIntervalTime = PING_INTERVAL;
    pingParams.PingSize = PING_PKT_SIZE;
    pingParams.PingRequestTimeout = PING_TIMEOUT;
    pingParams.TotalNumberOfAttempts = NO_OF_ATTEMPTS;
    pingParams.Flags = 0;
    pingParams.Ip = g_GatewayIP;

    /* Check for LAN connection */
    retVal = sl_NetAppPingStart( (SlPingStartCommand_t*)&pingParams, SL_AF_INET,
                                 (SlPingReport_t*)&pingReport, SimpleLinkPingReport);
    ASSERT_ON_ERROR(retVal);

    /* Wait */
    while(!IS_PING_DONE(g_Status)) { _SlNonOsMainLoopTask(); }

    if(0 == g_PingPacketsRecv)
    {
        /* Problem with LAN connection */
        ASSERT_ON_ERROR(LAN_CONNECTION_FAILED);
    }

    /* LAN connection is successful */
    return SUCCESS;
}

/*!
    \brief This function checks the internet connection by pinging
           the external-host (HOST_NAME)

    \param[in]  None

    \return     0 on success, negative error-code on error
*/
static _i32 checkInternetConnection()
{
    SlPingStartCommand_t pingParams = {0};
    SlPingReport_t pingReport = {0};

    _u32 ipAddr = 0;

    _i32 retVal = -1;

    CLR_STATUS_BIT(g_Status, STATUS_BIT_PING_DONE);
    g_PingPacketsRecv = 0;

    /* Set the ping parameters */
    pingParams.PingIntervalTime = PING_INTERVAL;
    pingParams.PingSize = PING_PKT_SIZE;
    pingParams.PingRequestTimeout = PING_TIMEOUT;
    pingParams.TotalNumberOfAttempts = NO_OF_ATTEMPTS;
    pingParams.Flags = 0;
    pingParams.Ip = g_GatewayIP;

    /* Check for Internet connection */
    retVal = sl_NetAppDnsGetHostByName((_i8 *)HOST_NAME, pal_Strlen(HOST_NAME), &ipAddr, SL_AF_INET);
    ASSERT_ON_ERROR(retVal);

    /* Replace the ping address to match HOST_NAME's IP address */
    pingParams.Ip = ipAddr;

    /* Try to ping HOST_NAME */
    retVal = sl_NetAppPingStart( (SlPingStartCommand_t*)&pingParams, SL_AF_INET,
                                 (SlPingReport_t*)&pingReport, SimpleLinkPingReport);
    ASSERT_ON_ERROR(retVal);

    /* Wait */
    while(!IS_PING_DONE(g_Status)) { _SlNonOsMainLoopTask(); }

    if (0 == g_PingPacketsRecv)
    {
        /* Problem with internet connection*/
        ASSERT_ON_ERROR(INTERNET_CONNECTION_FAILED);
    }

    /* Internet connection is successful */
    return SUCCESS;
}


void postProvisioning (void)
{
    _u8 configOpt = SL_SCAN_POLICY(0);

	//UART_PRINT("PostProvisioning\r\n");

	// cc_timer_stop(tTimerHndl);

    // Read the new URN
    // sl_NetAppGet (SL_NET_APP_DEVICE_CONFIG_ID, NETAPP_SET_GET_DEV_CONF_OPT_DEVICE_URN, &applicationData.urnLen, (_u8*)&applicationData.urn);

    /* Since this section runs at first configuration, apply all one-time settings here */

	//Set connection policy
    sl_WlanPolicySet(SL_POLICY_CONNECTION,SL_CONNECTION_POLICY(1,1,0,0,0),NULL,0);

    /* Disabled MDNS services - not applicable when using Long Sleep Interval (they're internally disabled, even when not explictly disabled) */
    //Disable MDNS
    //sl_NetAppStop(SL_NET_APP_MDNS_ID);

    //Enable HTTP server
    sl_NetAppStart(SL_NET_APP_HTTP_SERVER_ID);

    //Disable background scans
    sl_WlanPolicySet(SL_POLICY_SCAN , configOpt, NULL, 0);

}


static _i32 SmartConfigConnect()
{
    _u8 policyVal = 0;
    _i32 retVal = -1;

    /* Clear all profiles */
    /* This is of course not a must, it is used in this example to make sure
    * we will connect to the new profile added by SmartConfig
    */
    retVal = sl_WlanProfileDel(0xFF);
    ASSERT_ON_ERROR(retVal);

    /* set AUTO policy */
    retVal = sl_WlanPolicySet(SL_POLICY_CONNECTION,
                     SL_CONNECTION_POLICY(1, 0, 0, 0, 0),
                     &policyVal,
                     1);    /*PolicyValLen*/
    ASSERT_ON_ERROR(retVal);

    /* Start SmartConfig
     * This example uses the unsecured SmartConfig method
     */
    retVal = sl_WlanSmartConfigStart(0,                          /* groupIdBitmask */
                            SMART_CONFIG_CIPHER_NONE,   /* cipher */
                            0,                          /* publicKeyLen */
                            0,                          /* group1KeyLen */
                            0,                          /* group2KeyLen */
                            (const _u8 *)"",  /* publicKey */
                            (const _u8 *)"",  /* group1Key */
                            (const _u8 *)""); /* group2Key */

    ASSERT_ON_ERROR(retVal);

    /* Wait */
    // while((!IS_CONNECTED(g_Status)) || (!IS_IP_ACQUIRED(g_Status))) { _SlNonOsMainLoopTask(); }

    return retVal;
}

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
     *  Following function configures the device to default state by cleaning
     * the persistent settings stored in NVMEM (viz. connection profiles &
     * policies, power policy etc)
     *
     * Applications may choose to skip this step if the developer is sure
     * that the device is in its default state at start of application
     *
     * Note that all profiles and persistent settings that were done on the
     * device will be lost
     */
     

#ifndef PROV_SMART_CONFIG

	Network_IF_InitDriver(ROLE_STA);

    retVal = startProvisioning();
    if (retVal < 0) {
    	CLI_Write("Provisoning Failed \n\r");
    	 LOOP_FOREVER();
    }
    else
    	CLI_Write("Provisioning Succedded \n\r");

#else
    CLI_Write("Smart config - provisioning \n\r");
    Network_IF_InitDriver(ROLE_STA);
    retVal = SmartConfigConnect();
    Report("Smart config retVal = %u \n",retVal);
#endif

    /* pending on device to connect to AP */
     while ((!IS_CONNECTED(g_Status)) || (!IS_IP_ACQUIRED(g_Status)))
     {
         _SlNonOsMainLoopTask();
     }

#ifndef PROV_SMART_CONFIG
     postProvisioning();
     waitmSec(3000);
#endif

#if 0
     retVal = checkLanConnection();
     if(retVal < 0)
     {
         CLI_Write((_u8 *)" Device couldn't connect to LAN \n\r");
         LOOP_FOREVER();
     }

     CLI_Write((_u8 *)" Device successfully connected to the LAN\r\n");

     retVal = checkInternetConnection();
     if(retVal < 0)
     {
         CLI_Write((_u8 *)" Device couldn't connect to the internet \n\r");
         LOOP_FOREVER();
     }

     CLI_Write((_u8 *)" Device successfully connected to the internet \n\r");
#endif
     return 0;
}


/*!
    \brief This function initializes the application variables

    \param[in]  None

    \return     0 on success, negative error-code on error
*/
static _i32 initializeAppVariables()
{
    g_Status = 0;
    g_ConnectTimeoutCnt = 0;
    pal_Memset(g_Wlan_SSID, 0, (MAXIMAL_SSID_LENGTH + 1));
    pal_Memset(g_Wlan_Security_Key, 0, MAX_KEY_LENGTH);
    pal_Memset(g_netEntries, 0, sizeof(g_netEntries));
    pal_Memset(g_ssid_list, 0, sizeof(g_ssid_list));
    pal_Memset(&g_secParams, 0, sizeof(SlSecParams_t));

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
    CLI_Write(" Simplelink Provisioning application - Version ");
    CLI_Write(APPLICATION_VERSION);
    CLI_Write("\n\r*******************************************************************************\n\r");
}

//*****************************************************************************
//
//! Function for handling provisioning (adding a profile to a new AP).
//!	This function assumes running mostly on first time configurations, so
//! one time settings are handled here.
//!
//! \param  none
//!
//! \return none
//!
//*****************************************************************************
_i32 startProvisioning (void)
{
	SlFsFileInfo_t FsFileInfo;
	_i32 FileHandle = 0;
	_i32	retVal = -1;
	slExtLibProvCfg_t	cfg;

    CLI_Write("Starting Provisioning..\n\r");



	/* check if version token file exists in the device FS.
	 * If not, than create a file and write the required token */
	/* Creating the param_product_version.txt file once */
	if (SL_FS_ERR_FILE_NOT_EXISTS == sl_FsGetInfo(SL_FILE_PARAM_PRODUCT_VERSION, 0 , &FsFileInfo))
	{
		CLI_Write(">>>>> Creating new file: \r\n");

		sl_FsOpen(SL_FILE_PARAM_PRODUCT_VERSION, FS_MODE_OPEN_CREATE(100, _FS_FILE_OPEN_FLAG_COMMIT), NULL, &FileHandle);
		sl_FsWrite(FileHandle, NULL, SL_PARAM_PRODUCT_VERSION_DATA, strlen(SL_PARAM_PRODUCT_VERSION_DATA));
		sl_FsClose(FileHandle, NULL, NULL, NULL);
	}

	/* Creating the config result file once */
	if (SL_FS_ERR_FILE_NOT_EXISTS == sl_FsGetInfo(SL_FILE_PARAM_CFG_RESULT, 0 , &FsFileInfo))
	{
		CLI_Write(">>>>> Creating new file: \r\n");

		sl_FsOpen(SL_FILE_PARAM_CFG_RESULT, FS_MODE_OPEN_CREATE(100, _FS_FILE_OPEN_FLAG_COMMIT), NULL, &FileHandle);
		sl_FsWrite(FileHandle, NULL, GET_CFG_RESULT_TOKEN, strlen(GET_CFG_RESULT_TOKEN));
		sl_FsClose(FileHandle, NULL, NULL, NULL);
	}

	/* Creating the param device name file once */
	if (SL_FS_ERR_FILE_NOT_EXISTS == sl_FsGetInfo(SL_FILE_PARAM_DEVICE_NAME, 0 , &FsFileInfo))
	{
		CLI_Write(">>>>> Creating new file: \r\n");

		sl_FsOpen(SL_FILE_PARAM_DEVICE_NAME, FS_MODE_OPEN_CREATE(100, _FS_FILE_OPEN_FLAG_COMMIT), NULL, &FileHandle);
		sl_FsWrite(FileHandle, NULL, GET_DEVICE_NAME_TOKEN, strlen(GET_DEVICE_NAME_TOKEN));
		sl_FsClose(FileHandle, NULL, NULL, NULL);
	}

	/* Creating the netlist name file once */
	if (SL_FS_ERR_FILE_NOT_EXISTS == sl_FsGetInfo(SL_FILE_NETLIST, 0 , &FsFileInfo))
	{
		CLI_Write(">>>>> Creating new file: \r\n");

		sl_FsOpen(SL_FILE_NETLIST, FS_MODE_OPEN_CREATE(100, _FS_FILE_OPEN_FLAG_COMMIT), NULL, &FileHandle);
		// sl_FsWrite(FileHandle, NULL, SL_SET_NETLIST_TOKENS, strlen(GET_NETWORKS_TOKEN_PREFIX));
		sl_FsWrite(FileHandle, NULL, SL_SET_NETLIST_TOKENS, strlen(SL_SET_NETLIST_TOKENS));
		sl_FsClose(FileHandle, NULL, NULL, NULL);
	}
    cfg.IsBlocking = 1;
    cfg.AutoStartEnabled = 0;
    cfg.Timeout10Secs = PROVISIONING_TIMEOUT/10;
    cfg.ModeAfterFailure = ROLE_STA;
    cfg.ModeAfterTimeout = ROLE_STA;

    CLI_Write("Starting Provisioning..2\n\r");
    retVal = sl_extlib_ProvisioningStart(ROLE_STA, &cfg);

    return retVal;
}

//*****************************************************************************

// Provisioning Callbacks

//*****************************************************************************


void sl_extlib_ProvWaitHdl(_i32 timeout)
{
    waitmSec(timeout);
}

long Network_IF_InitDriver(unsigned int uiMode)
{
    long lRetVal = -1;
    // Reset CC3100 Network State Machine
    initializeAppVariables();

    //
    // Assumption is that the device is configured in station mode already
    // and it is in its default state
    //

    CLI_Write("Starting SL device... \n\r");
    lRetVal = sl_Start(NULL,NULL,NULL);
    {
    	SlVersionFull   ver = {{0}};
    	unsigned char ucConfigOpt = 0;
        unsigned char ucConfigLen = 0;

    	// Get the device's version-information
        ucConfigOpt = SL_DEVICE_GENERAL_VERSION;
        ucConfigLen = sizeof(ver);
        lRetVal = sl_DevGet(SL_DEVICE_GENERAL_CONFIGURATION, &ucConfigOpt,
                                    &ucConfigLen, (unsigned char *)(&ver));
        ASSERT_ON_ERROR(lRetVal);
    }


    CLI_Write("\r\nWaiting for provisioning to start\n\r");


    // this section will force entering to AP provisioning
    // for each time the app is running
    if (lRetVal == ROLE_STA)
    {
    	sl_WlanDisconnect();
    }

    // Switch to STA role and restart
	lRetVal = sl_WlanSetMode(ROLE_STA);

	CLI_Write("Deleting all profiles.\r\n");
	sl_WlanProfileDel(0xFF);


	lRetVal = sl_Stop(0xFF);

	lRetVal = sl_Start(0, 0, 0);

    if (lRetVal < 0 || lRetVal != ROLE_STA)
    {
        // Switch to STA role and restart
        lRetVal = sl_WlanSetMode(ROLE_STA);

        lRetVal = sl_Stop(0xFF);

        lRetVal = sl_Start(0, 0, 0);

        CLI_Write("Started SimpleLink Device: STA Mode\n\r");
    }

    if(uiMode == ROLE_AP)
    {
    	CLI_Write("Switching to AP mode on application request\n\r");
        // Switch to AP role and restart
        lRetVal = sl_WlanSetMode(uiMode);
        ASSERT_ON_ERROR(lRetVal);

        lRetVal = sl_Stop(0xFF);

        lRetVal = sl_Start(0, 0, 0);
        ASSERT_ON_ERROR(lRetVal);

        // Check if the device is up in AP Mode
        if (ROLE_AP == lRetVal)
        {
            // If the device is in AP mode, we need to wait for this event
            // before doing anything
            while(!IS_IP_ACQUIRED(g_Status))
            {
#ifndef SL_PLATFORM_MULTI_THREADED
              _SlNonOsMainLoopTask();
#else
              osi_Sleep(1);
#endif
            }
        }
        else
        {
            // We don't want to proceed if the device is not coming up in AP-mode
           // ASSERT_ON_ERROR(DEVICE_NOT_IN_AP_MODE);
        	LOOP_FOREVER();
        }

        CLI_Write("Re-started SimpleLink Device: AP Mode\n\r");
    }
    else if(uiMode == ROLE_P2P)
    {
    	CLI_Write("Switching to P2P mode on application request\n\r");
        // Switch to AP role and restart
        lRetVal = sl_WlanSetMode(uiMode);
        ASSERT_ON_ERROR(lRetVal);

        lRetVal = sl_Stop(0xFF);

        lRetVal = sl_Start(0, 0, 0);
        ASSERT_ON_ERROR(lRetVal);

        // Check if the device is in station again
        if (ROLE_P2P != lRetVal)
        {
            // We don't want to proceed if the device is not coming up in P2P-mode
           //  ASSERT_ON_ERROR(DEVICE_NOT_IN_P2P_MODE);
        	LOOP_FOREVER();
        }

        CLI_Write("Re-started SimpleLink Device: P2P Mode\n\r");
    }
    else
    {
        // Device already started in STA-Mode
    }
    return 0;
}

