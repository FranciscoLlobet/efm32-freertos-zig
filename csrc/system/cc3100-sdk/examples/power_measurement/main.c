/*
 * main.c - Sample application to demonstrate power measurement use-cases
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
 * Application Name     -   Power Management use cases
 * Application Overview -   This is a sample application demonstrating few
 *                          main use cases simulating the power modes supported
							by CC3100. this application can be used in order to
							create always connected use case, 
							INTERMITTENTLY connected use case,
							Transceiver mode and for measuring constants values.
 * Application Details  -   http://
 */

/**/
#include "simplelink.h"
#include "sl_common.h"

#include <stdio.h>
#include <stdlib.h>


#define APPLICATION_VERSION "1.3.0"

#define SL_STOP_TIMEOUT        		0xFF
#define HIBERNATE_MEASURE 			0
#define SLEEP_MEASURE				1
#define TRANSCEIVER_MODE 			2
#define ALWAYS_CONNECTED_USE_CASE	3
#define INTERMITTENTLY_CONNECTED	4
#define RAW_SOCKET					0
#define UDP_SOCKET					1
#define TCP_SOCKET					2
#define SEC_TCP_SOCKET  			3
#define STATIC_IP					0
#define DHCP						1


/* Application specific status/error codes */
typedef enum{
    LAN_CONNECTION_FAILED = -0x7D0,        /* Choosing this number to avoid overlap with host-driver's error codes */
    INTERNET_CONNECTION_FAILED = LAN_CONNECTION_FAILED - 1,
    DEVICE_NOT_IN_STATION_MODE = INTERNET_CONNECTION_FAILED - 1,
    BSD_UDP_CLIENT_FAILED = DEVICE_NOT_IN_STATION_MODE - 1,
    TCP_RECV_ERROR = BSD_UDP_CLIENT_FAILED  -1,
    TCP_SEND_ERROR = TCP_RECV_ERROR - 1,

    STATUS_CODE_MAX = -0xBB8
} e_AppStatusCodes;


/****************** Test Related Parameters / Defines / Global Variables **********************/
/*
* User scenario defines -- Start
*/
#define IP_ADDR         			SL_IPV4_VAL(192,168,39,200) /* Destenation Ip address */
#define PORT_NUM        			5001              			/* Port number to be used */
#define SSL_PORT					443							/* The port for ssl */
#define MY_IP_ADDRESS   			SL_IPV4_VAL(192,168,39,1)	/* the Device (simplelink) IP address */
#define GW_IP_ADDRESS   			SL_IPV4_VAL(192,168,39,241) /* Gate Way IP address */
#define BUF_SIZE        			1400
#define NO_OF_PACKETS   			1
#define NUM_OF_CYC					30			/* please enter a number < 10,000 */
#define LSI_SLEEP_DURATION_IN_MSEC	(100)  		/* in msec */
#define NOT_ACTIVE_DURATION_IN_MSEC	(10000)  	/* in msec */
#define TAG_TUNED_CHANNEL			1			/* the channel number the transceiver mode will work on */
#define INTERACTIVE					1
/*
* User scenario defines -- End
*/

/*
 * GLOBAL VARIABLES -- Start
 */
_u32  g_Status 			= 0;
_u32  g_GatewayIP 		= 0;
_u8   g_SocketType 		= UDP_SOCKET; 	/* option --> RAW_SOCKET; UDP_SOCKET; TCP_SOCKET; SEC_TCP_SOCKET; */
_u8	  g_IpV4Option		= STATIC_IP;  	/* option --> STATIC_IP; DHCP; */
_i16  g_ActiveUseCase 	= INTERMITTENTLY_CONNECTED; /* options -->  HIBERNATE_MEASURE; SLEEP_MEASURE; TRANSCEIVER_MODE; ALWAYS_CONNECTED_USE_CASE; INTERMITTENTLY_CONNECTED; */
_u8   g_CcaBypass		= 1; 			/* default bypass, if CCA is required use the value 0; */
_i32  g_TimeInterval    = (INTERACTIVE) ? 0 : NOT_ACTIVE_DURATION_IN_MSEC;

_i8   g_SsidName[25]; 			/* hold the AP name in interactive work mode */
_i8   g_PassKey [64]	= ""; 	/* hold the AP passkey in interactive work mode + init value */
_u8   g_SecType;				/* hold the AP secure type in interactive work mode */
char  g_NumOfCyc[4];  			/* internal use: for showing iteration num */
_u32  g_IpAddr = MY_IP_ADDRESS;
_u32  g_GwIpAddr = GW_IP_ADDRESS;


SlSockAddrIn_t  g_Addr;
union
{
    _u8 BsdBuf[BUF_SIZE];
    _u32 demobuf[BUF_SIZE/4];
} uBuf;
/*
 * GLOBAL VARIABLES -- End
 */


/*
 * Tag Profile Definitions -- Start
 */
#define FRAME_TYPE 0x88
#define FRAME_CONTROL 0x00
#define DURATION 0xc0,0x00
#define RECEIVE_ADDR 0x11, 0x22, 0x33, 0x44, 0x55, 0x66
#define TRANSMITTER_ADDR 0x00, 0x06, 0x66, 0x80, 0xE7, 0xAA
#define BSSID_ADDR 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF
#define DESTINATION_ADDR 0x11, 0x22, 0x33, 0x44, 0x55, 0x66
#define FRAME_NUMBER   0x00, 0x00
#define QOS_CTRL 0x00, 0x00
/* user defines */
#define TAG_FRAME_LENGTH                (100)
#define TAG_FRAME_TRANSMIT_RATE         (1)
#define TAG_FRAME_TRANSMIT_POWER        (7)


char FrameData[TAG_FRAME_LENGTH] = {0};
char FrameBaseData[] = {
		FRAME_TYPE,                          /* version , type sub type */
		FRAME_CONTROL,                       /* Frame control flag */
		DURATION,                            /* duration */
		RECEIVE_ADDR,                        /* Receiver ADDr */
		TRANSMITTER_ADDR,                    /* Transmitter Address */
		BSSID_ADDR,                   		 /* destination */
		FRAME_NUMBER,                        /* Frame number */
        QOS_CTRL};                   		 /* Transmitter */
/*
 * Tag Profile Definitions -- End
 */		
	
/**********************************************************************************************/


/*
 * STATIC FUNCTION DEFINITIONS -- Start
 */
static _i32 configureSimpleLinkToDefaultState();
static _i32 establishConnectionWithAP();


static _i32 initializeAppVariables();
static void displayBanner();
static void getAPinfo();
static void getIpAddr();
static _i32 GetAndSetUseCase();
static _i32 GetAndSetSocket();
static _i32 getTimeInterval();
static _i32 BsdUdpClient(_u16 Port, _i16 Sid);
static _i32 BsdTcpClient(_u16 Port, _i16 Sid, _u16 DoInit);
static _i32 BsdTcpSecClient(_u16 Port, _i16 Sid, _u16 DoInit);
static void tagConnection();
static _i32 SetTime();
static void hibernateMeasure();
static void sleepMeasure();
static void tagUseCase();
static void hibUseCase();
static void alwaysConnectedUseCase();
static void preConnetion(_u16 Port);
static void nonOsWait(_u32 Dly, _u32 timeStep);

/*
 * STATIC FUNCTION DEFINITIONS -- End
 */

/*
 * Application's entry point
 */
int main(int argc, char** argv)
 {
    _i32 retVal = -1;

    retVal = initializeAppVariables();
    ASSERT_ON_ERROR(retVal);

    /* Stop WDT and initialize the system-clock of the MCU */
    stopWDT();
    initClk();

    /* Configure command line interface */
    CLI_Configure();

    displayBanner();

    if (INTERACTIVE) {
    	retVal = GetAndSetUseCase();
    	ASSERT_ON_ERROR(retVal);
    	getAPinfo();
    	if (g_ActiveUseCase == ALWAYS_CONNECTED_USE_CASE || g_ActiveUseCase == INTERMITTENTLY_CONNECTED) {
    		getIpAddr();
    		retVal = GetAndSetSocket();
    		ASSERT_ON_ERROR(retVal);
    	}
    	if (!(g_ActiveUseCase == HIBERNATE_MEASURE || g_ActiveUseCase == SLEEP_MEASURE)) {
    		g_TimeInterval = getTimeInterval();
    		ASSERT_ON_ERROR(g_TimeInterval);
    	}
    }

    retVal = sprintf(g_NumOfCyc, "%d" ,NUM_OF_CYC);
    ASSERT_ON_ERROR(retVal);

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
            CLI_Write((_u8 *)" Failed to configure the device in its default state \n\r");

        LOOP_FOREVER();
    }
    CLI_Write((_u8 *)" Device is configured in default state \n\r");

    /*
     * Assumption is that the device is configured in station mode already
     * and it is in its default state
     */
    retVal = sl_Start(0, 0, 0);
    if ((retVal < 0) ||
        (ROLE_STA != retVal) )
    {
        CLI_Write((_u8 *)" Failed to start the device \n\r");
        LOOP_FOREVER();
    }
    CLI_Write((_u8 *)" Device started as STATION \n\r");

    if (g_ActiveUseCase != TRANSCEIVER_MODE) {
    	/* Connecting to WLAN AP */
    	retVal = establishConnectionWithAP();
    	if(retVal < 0)
    	{
    		CLI_Write((_u8 *)" Failed to establish connection w/ an AP \n\r");
    		LOOP_FOREVER();
    	}
        CLI_Write((_u8 *)" Connection established w/ AP and IP is acquired \n\r");
    }
    if (!(g_ActiveUseCase == HIBERNATE_MEASURE || g_ActiveUseCase == SLEEP_MEASURE)) {
    	CLI_Write((_u8 *)" The next use-case will iterate ");
    	CLI_Write((_u8 *)g_NumOfCyc);
    	CLI_Write((_u8 *)" Times! \n\r");
    }
	/* Run the current use case according to g_ActiveUseCase */
  	switch (g_ActiveUseCase) {
  		case HIBERNATE_MEASURE :
  			hibernateMeasure();				/* Constant measurement of Hibernate current */
  		case SLEEP_MEASURE :
  			sleepMeasure();					/* Constant measurement of LPDS current */
  		case TRANSCEIVER_MODE :
  			tagUseCase();					/* Sends packet/s using transceiver mode without AP connection */
  			break;
  		case ALWAYS_CONNECTED_USE_CASE :
  			alwaysConnectedUseCase(); 		/* Always connected mode */
  			break;
  		case INTERMITTENTLY_CONNECTED :
  			hibUseCase();					/* intermittently connected mode, hibernate between active cycles */
  			break;
  	}
  	CLI_Write((_u8 *)" The End! \n\r");
    return 0;
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
        CLI_Write((_u8 *)" [WLAN EVENT] NULL Pointer Error \n\r");
    
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
            if(SL_USER_INITIATED_DISCONNECTION == pEventData->reason_code)
            {
                CLI_Write((_u8 *)" Device disconnected from the AP on application's request \n\r");
            }
            else
            {
                CLI_Write((_u8 *)" Device disconnected from the AP on an ERROR..!! \n\r");
            }
        }
        break;

        default:
        {
            CLI_Write((_u8 *)" [WLAN EVENT] Unexpected event \n\r");
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
        CLI_Write((_u8 *)" [NETAPP EVENT] NULL Pointer Error \n\r");
 
    switch(pNetAppEvent->Event)
    {
        case SL_NETAPP_IPV4_IPACQUIRED_EVENT:
        {
            SlIpV4AcquiredAsync_t *pEventData = NULL;

            SET_STATUS_BIT(g_Status, STATUS_BIT_IP_ACQUIRED);

            pEventData = &pNetAppEvent->EventData.ipAcquiredV4;
            g_GatewayIP = pEventData->gateway;
        }
        break;

        default:
        {
            CLI_Write((_u8 *)" [NETAPP EVENT] Unexpected event \n\r");
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
    CLI_Write((_u8 *)" [HTTP EVENT] Unexpected event \n\r");
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
    CLI_Write((_u8 *)" [GENERAL EVENT] \n\r");
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
    CLI_Write((_u8 *)" [SOCK EVENT] Unexpected event \n\r");
}
/*
 * ASYNCHRONOUS EVENT HANDLERS -- End
 */







/*!
    \brief This function configure the SimpleLink device to its default state according
    	   to the selected use case.
    	   It:
           - Sets the mode to STATION
           - Configures connection policy according to use-case:
           	   1. Fast connect and auto for Intermittently connected.
           	   2. None for transceiver mode.
           	   3. Auto and AutoSmartConfig for the rest of the use cases.
           - Deletes all the stored profiles
           - Enables DHCP/Static IP according to user selection
           - Disables Scan policy
           - Sets Tx power to maximum
           - Sets power policy to low at transceiver mode and
           	 to normal at all the other use case.
           - Unregisters & disable mDNS services
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

    SlNetCfgIpV4Args_t ipV4;

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

    if (g_ActiveUseCase == TRANSCEIVER_MODE) {
    	retVal = sl_WlanPolicySet(SL_POLICY_CONNECTION, SL_CONNECTION_POLICY(0, 0, 0, 0, 0), NULL, 0);
    	ASSERT_ON_ERROR(retVal);
    } else {
    	/* Set connection policy to Auto + SmartConfig */
    	retVal = sl_WlanPolicySet(SL_POLICY_CONNECTION, SL_CONNECTION_POLICY(1, 0, 0, 0, 1), NULL, 0);
    	ASSERT_ON_ERROR(retVal);
    }
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
    if (g_IpV4Option == STATIC_IP) {
    	ipV4.ipV4 = g_IpAddr;
    	ipV4.ipV4Mask = (_u32)SL_IPV4_VAL(255,255,255,0);
    	ipV4.ipV4Gateway = g_GwIpAddr;
    	ipV4.ipV4DnsServer = g_GwIpAddr;
    	sl_NetCfgSet(SL_IPV4_STA_P2P_CL_STATIC_ENABLE,IPCONFIG_MODE_ENABLE_IPV4,sizeof(SlNetCfgIpV4Args_t),(_u8 *)&ipV4);
    	CLI_Write((unsigned char *)" Configured to Static IP Address \n\r");
    }
    if (g_IpV4Option == DHCP) {
    	/* Enable DHCP client*/
    	CLI_Write((unsigned char *)" IP Address will be obtained through DHCP \n\r");
    	retVal = sl_NetCfgSet(SL_IPV4_STA_P2P_CL_DHCP_ENABLE,1,1,&val);
    	ASSERT_ON_ERROR(retVal);
    }

    /* Disable scan */
    configOpt = SL_SCAN_POLICY(0);
    retVal = sl_WlanPolicySet(SL_POLICY_SCAN , configOpt, NULL, 0);
    ASSERT_ON_ERROR(retVal);

    /* Set Tx power level for station mode
       Number between 0-15, as dB offset from max power - 0 will set maximum power */
    power = 0;
    retVal = sl_WlanSet(SL_WLAN_CFG_GENERAL_PARAM_ID, WLAN_GENERAL_PARAM_OPT_STA_TX_POWER, 1, (_u8 *)&power);
    ASSERT_ON_ERROR(retVal);
    if (g_ActiveUseCase == TRANSCEIVER_MODE ) {
    	/* Set PM policy to low */
    	retVal = sl_WlanPolicySet(SL_POLICY_PM , SL_LOW_POWER_POLICY, NULL, 0);
    } else {
    	/* Set PM policy to normal */
    	retVal = sl_WlanPolicySet(SL_POLICY_PM , SL_NORMAL_POLICY, NULL, 0);
    }
    ASSERT_ON_ERROR(retVal);

    /* set the sleep interval */
    if (g_ActiveUseCase == ALWAYS_CONNECTED_USE_CASE && LSI_SLEEP_DURATION_IN_MSEC > 100) {
    	_u16 PolicyBuff[4] = {0,0,LSI_SLEEP_DURATION_IN_MSEC,0}; /* PolicyBuff[2] is max sleep time in mSec */
    	sl_WlanPolicySet(SL_POLICY_PM , SL_LONG_SLEEP_INTERVAL_POLICY, (_u8*)PolicyBuff,sizeof(PolicyBuff));
    }
    /* set cc3100 time and date */
    if (g_SocketType == SEC_TCP_SOCKET) {
    	retVal= SetTime();
    	if (retVal < 0) {
    		CLI_Write(" Failed to set the device time \n\r");
    		LOOP_FOREVER();
    	}
    }

    /* Unregister mDNS services */
    retVal = sl_NetAppMDNSUnRegisterService(0, 0);
    ASSERT_ON_ERROR(retVal);
    /* disable mdns */
    sl_NetAppStop(SL_NET_APP_MDNS_ID); /* disable MDNS */
    CLI_Write((unsigned char *)" Disable MDNS \n\r");

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

    if (INTERACTIVE) {
    	secParams.Key = g_PassKey;
    	secParams.KeyLen = pal_Strlen(g_PassKey);
    	secParams.Type = g_SecType;

    	retVal = sl_WlanConnect(g_SsidName, pal_Strlen(g_SsidName), 0, &secParams, 0);
    } else {
    	secParams.Key = (_i8 *)PASSKEY;
    	secParams.KeyLen = pal_Strlen(PASSKEY);
    	secParams.Type = SEC_TYPE;

    	retVal = sl_WlanConnect((_i8 *)SSID_NAME, pal_Strlen(SSID_NAME), 0, &secParams, 0);
    }

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
    g_GatewayIP = 0;

    return SUCCESS;
}

/*!
    \brief This function displays the application's banner

    \param      None

    \return     None
*/
static void displayBanner()
{
    CLI_Write((_u8 *)"\n\r\n\r");
    CLI_Write((_u8 *)" Power Management application - Version ");
    CLI_Write((_u8 *) APPLICATION_VERSION);
    CLI_Write((_u8 *)"\n\r*******************************************************************************\n\r");
}

/*!
    \brief This function gets the AP information interactively

    \param      None

    \return     None
*/
static void getAPinfo() {
	char buf[64];

	CLI_Write((_u8 *)"			** AP settings **\n\r");
	CLI_Write((_u8 *)" Please Enter SSID_NAME (AP name): ");
	CLI_Read((_u8 *)buf);
	CLI_Write((_u8 *)"\n\r");
	memcpy(g_SsidName,(const void *)buf,strlen(buf) + 1);
	CLI_Write((_u8 *)" Please Enter Security mode type:\n\r");
	CLI_Write((_u8 *)" 1) for Open:\n\r");
	CLI_Write((_u8 *)" 2) for WEP:\n\r");
	CLI_Write((_u8 *)" 3) for WPA:\n\r");
	CLI_Write((_u8 *)"  Enter selection:");
	CLI_Read((_u8 *)buf);
	CLI_Write((_u8 *)"\n\r");
	switch (atoi((const char *)buf)) {
	case 1 :
		g_SecType = SL_SEC_TYPE_OPEN;
		break;
	case 2 :
		g_SecType = SL_SEC_TYPE_WEP;
		break;
	case 3 :
		g_SecType = SL_SEC_TYPE_WPA;
		break;
	default :
		g_SecType = SL_SEC_TYPE_OPEN;
		CLI_Write((_u8 *)" Invalid Selection, security mode set to OPEN (default)\n\r");
		break;

	}
	if (g_SecType != SL_SEC_TYPE_OPEN) { /* passkey is needed */
		CLI_Write((_u8 *)" Please Enter PASSKEY (AP password):");
		CLI_Read((_u8 *)buf);
		CLI_Write((_u8 *)"\n\r");
		memcpy(g_PassKey,buf,strlen(buf) + 1);
	}
	CLI_Write((_u8 *)"\n\r*******************************************************************************\n\r");
}

/*!
    \brief This function gets the IP address information interactively

    \param      None

    \return     None
*/
static void getIpAddr() {
	char buf[16];
	char *s1, *s2, *s3, *s4;

	CLI_Write((_u8 *)"			** IP settings **\n\r");
	CLI_Write((_u8 *)" Please select IP address obtaining method(1 for static IP, any other number for DHCP): ");
	CLI_Read((_u8 *)buf);
	CLI_Write((_u8 *)"\n\r");
	g_IpV4Option = (atoi((const char *)buf) == 1) ? STATIC_IP : DHCP;
	if (g_IpV4Option == STATIC_IP) {
		CLI_Write((_u8 *)" Please Enter Device static IP Address (use commas or dots, NO spaces): ");
		CLI_Read((_u8 *)buf);
		CLI_Write((_u8 *)"\n\r");
		/* parse the input address string */
		s1 = strtok(buf,".,"); /* first pass */
		s2 = strtok(NULL,".,");
		s3 = strtok(NULL,".,");
		s4 = strtok(NULL,".,");
		g_IpAddr = SL_IPV4_VAL((const char *)atoi(s1), (const char *)atoi(s2), (const char *)atoi(s3), (const char *)atoi(s4));
		CLI_Write((_u8 *)" Please Enter the GateWay IP Address (make sure it is in the same domain as the device): ");
		CLI_Read((_u8 *)buf);
		CLI_Write((_u8 *)"\n\r");
		/* parse the input address string */
		s1 = strtok(buf,".,"); /* first pass */
		s2 = strtok(NULL,".,");
		s3 = strtok(NULL,".,");
		s4 = strtok(NULL,".,");
		g_GwIpAddr = SL_IPV4_VAL((const char *)atoi(s1), (const char *)atoi(s2), (const char *)atoi(s3), (const char *)atoi(s4));
	}
}

/*!
    \brief This function gets the use-case selection interactively

    \param      None

    \return     inputs string length
*/
static _i32 GetAndSetUseCase() {
	_u8 sel[2];
	_i32 len = 0;

	CLI_Write((_u8 *)"			** Please select use-case: **\n\r");
	CLI_Write((_u8 *)" 1) for HIBERNATE_MEASURE\n\r");
	CLI_Write((_u8 *)" 2) for SLEEP_MEASURE\n\r");
	CLI_Write((_u8 *)" 3) for TRANSCEIVER_MODE\n\r");
	CLI_Write((_u8 *)" 4) for ALWAYS_CONNECTED_USE_CASE\n\r");
	CLI_Write((_u8 *)" 5) for INTERMITTENTLY_CONNECTED\n\r");
	CLI_Write((_u8 *)"any other key) for hard coded settings\n\r");
	CLI_Write((_u8 *)" Enter your selection: ");
	len =  CLI_Read((_u8 *)sel);
	CLI_Write((_u8 *)"\n\r");

	switch (atoi((const char *)sel)) {
	case 1 :
		g_ActiveUseCase = HIBERNATE_MEASURE;
		break;
	case 2 :
		g_ActiveUseCase = SLEEP_MEASURE;
		break;
	case 3 :
		g_ActiveUseCase = TRANSCEIVER_MODE;
		break;
	case 4 :
		g_ActiveUseCase = ALWAYS_CONNECTED_USE_CASE;
		break;
	case 5 :
		g_ActiveUseCase = INTERMITTENTLY_CONNECTED;
		break;
	default :
		CLI_Write((_u8 *)" Applying the Hardcoded settings\n\r");
		break;
	}
	CLI_Write((_u8 *)"\n\r*******************************************************************************\n\r");
	return len;
}

/*!
    \brief This function gets the socket type selection interactively (only for dynamic use-cases,excluding tranciver mode)

    \param      None

    \return     inputs string length
*/
static _i32 GetAndSetSocket() {
	_u8 sel[2];
	_i32 len = 0;
	if (g_ActiveUseCase == HIBERNATE_MEASURE || g_ActiveUseCase == SLEEP_MEASURE || g_ActiveUseCase == TRANSCEIVER_MODE) {
		return 0;
	} else {
		CLI_Write((_u8 *)"			** Please select socket type: **\n\r");
		CLI_Write((_u8 *)" 1) for UDP Socket\n\r");
		CLI_Write((_u8 *)" 2) for TCP Socket\n\r");
		CLI_Write((_u8 *)" 3) for Secure TCP Socket (TLS/SSL)\n\r");
		CLI_Write((_u8 *)"any other key) for hard coded settings\n\r");
		CLI_Write((_u8 *)" Enter your selection: ");
		len =  CLI_Read((_u8 *)sel);
		CLI_Write((_u8 *)"\n\r");
		switch (atoi((const char *)sel)) {
		case 1 :
			g_SocketType = UDP_SOCKET;
			break;
		case 2 :
			g_SocketType = TCP_SOCKET;
			break;
		case 3 :
			g_SocketType = SEC_TCP_SOCKET;
			break;
		default :
			CLI_Write((_u8 *)" Applying the Hardcoded settings\n\r");
			break;
		}
		CLI_Write((_u8 *)"\n\r*******************************************************************************\n\r");
		return len;
	}
}
/*!
    \brief This function gets the Time interval between 2 active stages  interactively (only for dynamic use-cases)

    \param      None

    \return     time interval value
*/
static _i32 getTimeInterval() {
	_u8 num[32];

	CLI_Write((_u8 *)" Please select the Desired time interval (in msec): ");
	CLI_Read((_u8 *)num);
	CLI_Write((_u8 *)"\n\r");

	return (strtoll((const char*)num,NULL,10));
}

static _i32 BsdUdpClient(_u16 Port, _i16 Sid)
{
    _u16            AddrSize = 0;
    _i16            SockID = 0;
    _i16            Status = 0;
    _u16            LoopCount = 0;

    AddrSize = sizeof(SlSockAddrIn_t);
    if (Sid < 0) { /* Need to open socket  */
    	SockID = sl_Socket(SL_AF_INET,SL_SOCK_DGRAM, 0);
    	if( SockID < 0 ) {ASSERT_ON_ERROR(SockID);}
    } else { /* someone already opened a socket */
    	SockID = Sid;
    }

    if (g_ActiveUseCase == INTERMITTENTLY_CONNECTED) {
    	Status = sl_SendTo(SockID, uBuf.BsdBuf, BUF_SIZE, 0,
    	                               (SlSockAddr_t *)&g_Addr, AddrSize);
    	if( Status <= 0 )
    	{
    		Status = sl_Close(SockID);
    		ASSERT_ON_ERROR(BSD_UDP_CLIENT_FAILED);
    	}
    } else {
    	while (LoopCount < NO_OF_PACKETS)
    	{
    		Status = sl_SendTo(SockID, uBuf.BsdBuf, BUF_SIZE, 0,
                               (SlSockAddr_t *)&g_Addr, AddrSize);
    		if( Status <= 0 )
    		{
    			Status = sl_Close(SockID);
    			ASSERT_ON_ERROR(BSD_UDP_CLIENT_FAILED);
    		}

    		LoopCount++;
    	}
    }
    if (Sid < 0) { /* this function opened the socket hence it must close it. */
    	Status = sl_Close(SockID);
    	ASSERT_ON_ERROR(Status);
    }
    return SUCCESS;
}



/*!
    \brief Opening a TCP client side socket and trnasmitt data

    This function transmitt on UDP socket.

    \param[in]      port number on which the server will be listening on
    				socket id if it the socket already opened
    				do init (sl_connect)

    \return         0 on success, Negative value on Error.

    \note

    \warning
*/
static _i32 BsdTcpClient(_u16 Port, _i16 Sid, _u16 DoInit)
{

    _u16          AddrSize = 0;
    _i16          SockID = 0;
    _i16          Status = 0;
    _u16          LoopCount = 0;

    AddrSize = sizeof(SlSockAddrIn_t);

    if (Sid < 0) { /* Need to open socket  */
    	SockID = sl_Socket(SL_AF_INET,SL_SOCK_STREAM, 0);
    	if( SockID < 0 ) {
    		CLI_Write(" [TCP Client] Create socket Error \n\r");
    		ASSERT_ON_ERROR(SockID);
    	}
    } else { /* someone already opened a socket */
    	SockID = Sid;
    }

    if (DoInit) {
    	Status = sl_Connect(SockID, ( SlSockAddr_t *)&g_Addr, AddrSize);
    	if( Status < 0 ) {
    		sl_Close(SockID);
    		CLI_Write(" [TCP Client]  TCP connection Error \n\r");
    		ASSERT_ON_ERROR(Status);
    	}
    }

    while (LoopCount < NO_OF_PACKETS)
    {
        Status = sl_Send(SockID, uBuf.BsdBuf, BUF_SIZE, 0 );
        if( Status <= 0 )
        {
            CLI_Write(" [TCP Client] Data send Error \n\r");
            Status = sl_Close(SockID);
            ASSERT_ON_ERROR(TCP_SEND_ERROR);
        }

        LoopCount++;
    }
    if (Sid < 0) { /* this function opened the socket hence it must close it */
    	Status = sl_Close(SockID);
    	ASSERT_ON_ERROR(Status);
    }
    return SUCCESS;
}

/*!
    \brief Opening a TCP client side socket and trnasmitt data

    This function transmitt on TCP socket.

    \param[in]      port number on which the server will be listening on

    \return         0 on success, Negative value on Error.

    \note

    \warning
*/
static _i32 BsdTcpSecClient(_u16 Port, _i16 Sid, _u16 DoInit) {

    _u32  cipher = SL_SEC_MASK_SSL_RSA_WITH_RC4_128_SHA;
    _u8   method = SL_SO_SEC_METHOD_SSLV3;
    _i16  SockID = 0;
    _u16  AddrSize;
    _u16  LoopCount = 0;
    _i16  Status = 0;

    AddrSize = sizeof(SlSockAddrIn_t);

    if (Sid < 0) {
    	SockID = sl_Socket(SL_AF_INET,SL_SOCK_STREAM, SL_SEC_SOCKET);
    	if( SockID < 0 ) {
    		CLI_Write(" Failed to open socket \n\r");
    		LOOP_FOREVER();
    	}
    } else {
    	SockID = Sid;
    }

    if (DoInit) {
    	Status = sl_SetSockOpt(SockID, SL_SOL_SOCKET, SL_SO_SECMETHOD,
                               &method, sizeof(method));
    	if( Status < 0 ) {
    		CLI_Write(" Failed to configure the socket \n\r");
    		LOOP_FOREVER();
    	}
    	Status = sl_SetSockOpt(SockID, SL_SOL_SOCKET, SL_SO_SECURE_MASK,
                               &cipher, sizeof(cipher));
    	if( Status < 0 ) {
    		CLI_Write(" Failed to configure the socket \n\r");
    		LOOP_FOREVER();
    	}

    	/* If the user flashed the server certificate the lines below can be uncomment  */
    	/*
    	retVal = sl_SetSockOpt(g_SockID, SL_SOL_SOCKET, SL_SO_SECURE_FILES_CA_FILE_NAME,
                SL_SSL_CA_CERT, pal_Strlen(SL_SSL_CA_CERT));
    	if( retVal < 0 ) {
    		CLI_Write(" Failed to configure the socket \n\r");
    		LOOP_FOREVER();
    	}
    	 */
    	/* connect to the peer server */
    	Status = sl_Connect(SockID, ( SlSockAddr_t *)&g_Addr, AddrSize);
    	if ((Status < 0) && (Status != SL_ESECSNOVERIFY)) { /* ignore authentication error */
    		CLI_Write(" Failed to connect w/ server \n\r");
    		LOOP_FOREVER();
    	}
    	CLI_Write(" Connection w/ server established successfully \n\r");
	}
    while (LoopCount < NO_OF_PACKETS) {
    	Status = sl_Send(SockID, uBuf.BsdBuf, BUF_SIZE, 0 );
    	if( Status <= 0 ) {
    		CLI_Write(" [TCP Client] Data send Error \n\r");
    		Status = sl_Close(SockID);
    		ASSERT_ON_ERROR(TCP_SEND_ERROR);
        }
    	LoopCount++;
    }
    if (Sid < 0) { /* this function opened the socket hence it must close it */
    	Status = sl_Close(SockID);
    	ASSERT_ON_ERROR(Status);
    }
    return SUCCESS;
}

/*!
    \brief This function implements the transceiver use case

    \param      None

    \return     None
*/
static void tagConnection() {
	_i16 socketId, idx;
	_i16 Status = 0;

	if (g_CcaBypass) {
		socketId = sl_Socket(SL_AF_RF,SL_SOCK_RAW,0);
	} else {
		socketId = sl_Socket(SL_AF_RF,SL_SOCK_DGRAM,0);
	}
	for(idx = 0;idx < NO_OF_PACKETS;idx++) {
		Status = sl_Send(socketId,FrameData,TAG_FRAME_LENGTH, SL_RAW_RF_TX_PARAMS(TAG_TUNED_CHANNEL,TAG_FRAME_TRANSMIT_RATE,TAG_FRAME_TRANSMIT_POWER,1));
		Delay(10);
		if (Status <= 0) {
			CLI_Write(" [tagConnection] Data send Error \n\r");
			LOOP_FOREVER();
		}
	}
	Status = sl_Close(socketId);
}

static _i32 SetTime()
{
    _i32 retVal = -1;
    SlDateTime_t dateTime= {0};

    dateTime.sl_tm_day = DATE;
    dateTime.sl_tm_mon = MONTH;
    dateTime.sl_tm_year = YEAR;
    dateTime.sl_tm_hour = HOUR;
    dateTime.sl_tm_min = MINUTE;
    dateTime.sl_tm_sec = SECOND;

    retVal = sl_DevSet(SL_DEVICE_GENERAL_CONFIGURATION,SL_DEVICE_GENERAL_CONFIGURATION_DATE_TIME,
            sizeof(SlDateTime_t),(_u8 *)(&dateTime));
    ASSERT_ON_ERROR(retVal);

    return SUCCESS;
}
/*!
    \brief 		This function is for hibernate power consumption measurement, start measure after the message

    \param      None

    \return     NONE
*/
static void hibernateMeasure() {

	CLI_Write((_u8 *)" Entering hibernate state start measure \n\r");
	if (0 < sl_Stop(10)) {
		CLI_Write((_u8 *)" failed to stop the device for Hibernate current measuring! \n\r");
	}
	LOOP_FOREVER();
}

/*!
    \brief 		This function is Idle/beacon profile power consumption measurement, start measure after the message

    \param      None

    \return     None
*/
static void sleepMeasure() {
    _u8  configOpt = 0;
    _u8  configLen = 0;
    _i32 retVal = -1;

    SlVersionFull   ver = {0};

	sl_Stop(10);
	Delay(100);
	sl_Start(0,0,0);
	/* the Lines below are for the NWP to pull irq by sending a command */
	configOpt = SL_DEVICE_GENERAL_VERSION;
	configLen = sizeof(ver);
	retVal = sl_DevGet(SL_DEVICE_GENERAL_CONFIGURATION, &configOpt, &configLen, (_u8 *)(&ver));
	if (retVal < 0) {
		CLI_Write((_u8 *)" unable to get device version if \n\r");
	};
	CLI_Write((_u8 *)" entering deep sleep state start measure \n\r");
	LOOP_FOREVER();
}
/* !
     \brief		This function is for tag use case power consumption model.
  	  	  	  	it has number iteration of hib active  cycles, in each cycle configurable nuber of packet are sent.
  	  	  	  	 the packet are sent on raw sockets.

 	\param      None

    \return     None
*/
static void tagUseCase() {
	_u16 iter;
	_i16 count;
	_i32 retVal = -1;

	memcpy (&FrameData,&FrameBaseData,sizeof(FrameBaseData));
	for (count=0; count<TAG_FRAME_LENGTH-sizeof(FrameBaseData); count++)
	FrameData[count+sizeof(FrameBaseData)] = count % 256;

	CLI_Write((_u8 *)" Starting TRANSCEIVER_MODE  \n\r");

	for(iter = 0;iter <= NUM_OF_CYC;iter++) {
		retVal = sl_Stop(10);
		if (retVal < 0) {
			CLI_Write((_u8 *)" failed to stop the device \n\r");
		};
		Delay(g_TimeInterval);
		retVal = sl_Start(0,0,0);
		if (retVal < 0) {
			CLI_Write((_u8 *)" failed to start the device \n\r");
		};
		tagConnection();
	}
}

/* !
     \brief		hibernate use case (sensor like) connection establishment and TX/RX between long hibernate

 	\param      None

    \return     None
*/
static void hibUseCase() {
	_u16 iter;
	_i32 retVal;


	CLI_Write((_u8 *)" Starting  INTERMITTENTLY_CONNECTED use case \n\r");
	/* Set connection policy to Auto + fast connect */
	retVal = sl_WlanPolicySet(SL_POLICY_CONNECTION, SL_CONNECTION_POLICY(1, 1, 0, 0, 0), NULL, 0);
	/* prepare the destination address & bulid the packet */
	preConnetion(((g_SocketType == SEC_TCP_SOCKET) ? SSL_PORT : PORT_NUM));
	for(iter = 0;iter <= NUM_OF_CYC;iter++) {
		sl_Stop(10);
		Delay(g_TimeInterval);
		g_Status = 0;
		sl_Start(0,0,0);
		while((!IS_CONNECTED(g_Status)) || (!IS_IP_ACQUIRED(g_Status))) { _SlNonOsMainLoopTask(); }
		if (g_SocketType == SEC_TCP_SOCKET) {
				retVal = BsdTcpSecClient(SSL_PORT,-1,1);
				if (retVal < 0) {
					CLI_Write((_u8 *)" failed to operate on secure tcp socket \n\r");
				}
		} else if (g_SocketType == TCP_SOCKET) {
			retVal = BsdTcpClient(PORT_NUM,-1,1);
			if (retVal < 0) {
				CLI_Write((_u8 *)" failed to operate on tcp socket \n\r");
			}
		} else {
			retVal = BsdUdpClient(PORT_NUM,-1);
			if (retVal < 0) {
				CLI_Write((_u8 *)" failed to operate on udp socket \n\r");
			}
		}
	}
	sl_Stop(10);
}

/* !
     \brief		This Function is for Always connected , idle/beacon profile with peridic pass to active

 	\param      None

    \return     None
*/
static void alwaysConnectedUseCase() {
	_i32 retVal = 0;
	_u16 iter;
	_i16 SockID = -1;
	_u16 DoInit = 1;

	CLI_Write((_u8 *)" Starting  ALWAYS_CONNECTED_USE_CASE \n\r");
	/* prepare the destination address & bulid the packet */
	preConnetion(((g_SocketType == SEC_TCP_SOCKET) ? SSL_PORT : PORT_NUM));
	/* open relevant socket */
	switch (g_SocketType) {
	case UDP_SOCKET :
		SockID = sl_Socket(SL_AF_INET,SL_SOCK_DGRAM, 0);
		if( SockID < 0 ) {
			CLI_Write((_u8 *)" [alwaysConnectedUseCase] failed to open UDP socket \n\r");
			LOOP_FOREVER();
		}
		CLI_Write((_u8 *)" UDP \n\r");
		break;
	case TCP_SOCKET :
		SockID = sl_Socket(SL_AF_INET,SL_SOCK_STREAM, 0);
		if( SockID < 0 ) {
			CLI_Write(" [alwaysConnectedUseCase] failed to open TCP socket \n\r");
			LOOP_FOREVER();
		}
		CLI_Write((_u8 *)" TCP \n\r");
		break;
	case SEC_TCP_SOCKET :
		SockID = sl_Socket(SL_AF_INET,SL_SOCK_STREAM, SL_SEC_SOCKET);
		if( SockID < 0 ) {
			CLI_Write(" Failed to open socket \n\r");
			LOOP_FOREVER();
		}
		CLI_Write((_u8 *)" SSL/TLS  \n\r");
		break;
	}

	for(iter = 0;iter <= NUM_OF_CYC;iter++) {

		if (g_SocketType == UDP_SOCKET) {
			retVal = BsdUdpClient(PORT_NUM,SockID);
		}
		if (g_SocketType == TCP_SOCKET) {
			retVal = BsdTcpClient(PORT_NUM,SockID,DoInit);
		}
		if (g_SocketType == SEC_TCP_SOCKET) {
			retVal = BsdTcpSecClient(SSL_PORT,SockID,DoInit);
		}

		if (retVal < 0) {
			CLI_Write((_u8 *)" alwaysConnectedUseCase function failed to operate  \n\r");
		}
		DoInit = 0;
		nonOsWait(g_TimeInterval, 100);
	}

	/* close socket */
	retVal = sl_Close(SockID);
}

/* !
     \brief		This Function is does preliminary settings before the use case is applied.
     	 	 	1) build the packet that going to be sent.
     	 	 	2) build the destention IP Addr

 	\param      None

    \return     None
*/
static void preConnetion(_u16 Port) {
	_i16 idx;
	for (idx=0 ; idx<BUF_SIZE ; idx++)
	{
		uBuf.BsdBuf[idx] = (_u8)(idx % 10);
	}

	g_Addr.sin_family = SL_AF_INET;
	g_Addr.sin_port = sl_Htons((_u16)Port);
	g_Addr.sin_addr.s_addr = sl_Htonl((_u32)IP_ADDR);
}

/* !
     \brief		This Function implement wait delay with periodically nonOs main loop task calls

 	\param[In] 	Dly - the desired delay
 	\param[In] 	timeStep - the period of the _SlNonOsMainLoopTask() calls

    \return     None
*/
static void nonOsWait(_u32 Dly, _u32 timeStep) {
	_u32 tmr = 0;
	while (tmr < Dly) {
		Delay(timeStep);
		tmr = tmr + timeStep;
		_SlNonOsMainLoopTask();
	}
}
