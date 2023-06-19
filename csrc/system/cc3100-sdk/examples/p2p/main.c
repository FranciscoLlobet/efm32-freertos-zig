/*
 * main.c - sample application for using P2P
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
 * Application Name     -   P2P
 * Application Overview -   This is a sample application demonstrating how CC3100 can be
 *                          connected to a P2P device.
 * Application Details  -   http://processors.wiki.ti.com/index.php/CC31xx_P2P_Application
 *                          doc\examples\p2p.pdf
 */

#include "simplelink.h"
#include "sl_common.h"

#define APPLICATION_VERSION "1.3.0"

#define SL_STOP_TIMEOUT        0xFF

#define P2P_REMOTE_DEVICE  "DIRECT-" /* Dummy SSDI to start the p2p connection */
#define P2P_DEVICE_NAME    "cc3100-p2p-device"

#define DEVICE_TYPE        "1-0050F204-1"

#define SECURITY_TYPE      SL_SEC_TYPE_P2P_PBC
#define KEY                ""

#define LISTEN_CHANNEL    11
#define OPRA_CHANNEL      6
#define REGULATORY_CLASS  81

/* Port to be used  by TCP server*/
#define PORT_NUM        5001

#define BUF_SIZE 1400


#define NO_OF_PACKETS 1000

/* Application specific status/error codes */
typedef enum{
    DEVICE_NOT_IN_STATION_MODE = -0x7D0,        /* Choosing this number to avoid overlap w/ host-driver's error codes */
    P2P_CONNECTION_FAILED      = DEVICE_NOT_IN_STATION_MODE -1,
    TCP_RECV_ERROR             = P2P_CONNECTION_FAILED - 1,

    STATUS_CODE_MAX = -0xBB8
}e_AppStatusCodes;

/*
 * GLOBAL VARIABLES -- Start
 */
_u8 g_Status = 0;
_u32 g_DeviceIp = 0;
_i8 g_p2p_dev[MAXIMAL_SSID_LENGTH + 1];

union
{
    _u8 BsdBuf[BUF_SIZE];
    _u32 demobuf[BUF_SIZE/4];
} uBuf;

const _u8 digits[] = "0123456789";
/*
 * GLOBAL VARIABLES -- End
 */

/*
 * STATIC FUNCTION DEFINITIONS -- Start
 */
static _i32 configureSimpleLinkToDefaultState();
static _i32 initializeAppVariables();
static _u8 itoa(_i16 cNum, _u8 *cString);
static _i32 DisplayIP();
static _i32 establishConnectionWithP2Pdevice();
static _i32 BsdTcpServer(_u16 Port);
static void displayBanner();
/*
 * STATIC FUNCTION DEFINITIONS -- End
 */

/*
 * ASYNCHRONOUS EVENT HANDLERS -- Start
 */
/*!
    \brief This function handles WLAN events

    \param[in]      pWlanEvents is the event passed to the handler

    \return         none

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
            /*
             * Information about the connected AP (like name, MAC etc) will be
             * available in 'slWlanConnectAsyncResponse_t' - Applications
             * can use it if required
             *
             * slWlanConnectAsyncResponse_t *pEventData = NULL;
             * pEventData = &pWlanEvent->EventData.STAandP2PModeWlanConnected;
             *
             */
            SET_STATUS_BIT(g_Status, STATUS_BIT_CONNECTION);
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
            SET_STATUS_BIT(g_Status, STATUS_BIT_STA_CONNECTED);
            break;

        case SL_WLAN_DISCONNECT_EVENT:
            /*
             * Information about the disconnected STA and reason code will be
             * available in 'slWlanConnectAsyncResponse_t' - Applications
             * can use it if required
             *
             * slWlanConnectAsyncResponse_t *pEventData = NULL;
             * pEventData = &pWlanEvent->EventData.STAandP2PModeDisconnected;
             *
             */
            CLR_STATUS_BIT(g_Status, STATUS_BIT_CONNECTION);
            CLR_STATUS_BIT(g_Status, STATUS_BIT_IP_ACQUIRED);
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
            CLR_STATUS_BIT(g_Status, STATUS_BIT_STA_CONNECTED);
            CLR_STATUS_BIT(g_Status, STATUS_BIT_IP_LEASED);
            break;

        case SL_WLAN_CONNECTION_FAILED_EVENT:
            /*
             * Status code for connection faliure will be available
             * in 'slWlanConnFailureAsyncResponse_t' - Application
             * can use it if required.
             *
             * slWlanConnFailureAsyncResponse_t *pEventData = NULL;
             * pEventData = &pWlanEvent->EventData.P2PModewlanConnectionFailure
             *
             */
;
            SET_STATUS_BIT(g_Status, STATUS_BIT_CONNECTION_FAILED);
            break;

        case SL_WLAN_P2P_NEG_REQ_RECEIVED_EVENT:
            SET_STATUS_BIT(g_Status, STATUS_BIT_P2P_NEG_REQ_RECEIVED);

            pal_Memset(g_p2p_dev, '\0', MAXIMAL_SSID_LENGTH + 1);
            pal_Memcpy(g_p2p_dev,pWlanEvent->EventData.P2PModeNegReqReceived.go_peer_device_name,
                    pWlanEvent->EventData.P2PModeNegReqReceived.go_peer_device_name_len);
            break;

        case SL_WLAN_P2P_DEV_FOUND_EVENT:
            /*
             * Information about the remote P2P device (device name and MAC)
             * will be available in 'slPeerInfoAsyncResponse_t' - Applications
             * can use it if required
             *
             * slPeerInfoAsyncResponse_t *pEventData = NULL;
             * pEventData = &pWlanEvent->EventData.P2PModeDevFound;
             *
             */
            break;

        default:
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
 
    switch( pNetAppEvent->Event )
    {
        case SL_NETAPP_IPV4_IPACQUIRED_EVENT:
        {
            SlIpV4AcquiredAsync_t *pEventData = NULL;
            pEventData = &pNetAppEvent->EventData.ipAcquiredV4;

            g_DeviceIp = pEventData->ip;
        }
        SET_STATUS_BIT(g_Status, STATUS_BIT_IP_ACQUIRED);
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
            SET_STATUS_BIT(g_Status, STATUS_BIT_IP_LEASED);
            break;

        default:
            CLI_Write(" [NETAPP EVENT] Unexpected event \n\r");
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
    _u8  paramBuff[4];
    _i32 retVal = -1;

    retVal = initializeAppVariables();
    ASSERT_ON_ERROR(retVal);

    /* Stop WDT and initialize the system-clock of the MCU
       These functions needs to be implemented in PAL */
    stopWDT();
    initClk();

    /* Initialize the Application Uart Interface */
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
        {
            CLI_Write(" Failed to configure the device in its default state \n\r");
        }

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

    CLI_Write(" Configuring device in P2P mode\r\n");
    /* Configure P2P mode */
    retVal = sl_WlanSetMode(ROLE_P2P);
    if(retVal < 0)
        LOOP_FOREVER();

    /* Set Auto policy
     * any p2p option (SL_CONNECTION_POLICY(0,0,0,any_p2p,0)) can be used to
     * connect to first available p2p device */
    retVal = sl_WlanPolicySet(SL_POLICY_CONNECTION,SL_CONNECTION_POLICY(1,0,0,0,0),NULL,0);
    if(retVal < 0)
        LOOP_FOREVER();

    /* Set the negotiation role (SL_P2P_ROLE_NEGOTIATE).
     * CC3100 will negotiate with remote device GO/client mode.
     * Other valid options are:
     *             - SL_P2P_ROLE_GROUP_OWNER
     *             - SL_P2P_ROLE_CLIENT                           */
    retVal =  sl_WlanPolicySet(SL_POLICY_P2P, SL_P2P_POLICY(SL_P2P_ROLE_NEGOTIATE,
            SL_P2P_NEG_INITIATOR_ACTIVE),NULL,0);
    if(retVal < 0)
        LOOP_FOREVER();

    /* Set P2P Device name */
    retVal = sl_NetAppSet(SL_NET_APP_DEVICE_CONFIG_ID, NETAPP_SET_GET_DEV_CONF_OPT_DEVICE_URN,
            pal_Strlen(P2P_DEVICE_NAME), (_u8 *)P2P_DEVICE_NAME);
    if(retVal < 0)
        LOOP_FOREVER();

    /* Set P2P device type */
    retVal = sl_WlanSet(SL_WLAN_CFG_P2P_PARAM_ID, WLAN_P2P_OPT_DEV_TYPE,
            pal_Strlen(DEVICE_TYPE), DEVICE_TYPE);
    if(retVal < 0)
        LOOP_FOREVER();

    paramBuff[0] = LISTEN_CHANNEL;
    paramBuff[1] = REGULATORY_CLASS;
    paramBuff[2] = OPRA_CHANNEL;
    paramBuff[3] = REGULATORY_CLASS;

    /* Set P2P Device listen and operation channel valid channels are 1/6/11 */
    retVal = sl_WlanSet(SL_WLAN_CFG_P2P_PARAM_ID, WLAN_P2P_OPT_CHANNEL_N_REGS, 4, paramBuff);
    if(retVal < 0)
        LOOP_FOREVER();

    /* Restart as P2P device */
    retVal = sl_Stop(SL_STOP_TIMEOUT);
    if(retVal < 0)
    {
        LOOP_FOREVER();
    }

    retVal = sl_Start(0, 0, 0);
    if ((retVal < 0) ||
        (ROLE_P2P != retVal) )
    {
        CLI_Write(" Failed to start the device in P2P mode \n\r");
        LOOP_FOREVER();
    }

    CLI_Write(" Device is configured in P2P mode - Device name: ");
    CLI_Write(P2P_DEVICE_NAME);
    CLI_Write("\r\n");

    /* Connect to configure P2P device */
    retVal = establishConnectionWithP2Pdevice();
    if(retVal < 0)
    {
        CLI_Write(" Failed to establish connection w/ remote P2P device \n\r");
        LOOP_FOREVER();
    }

    CLI_Write(" Connection established w/ remote P2P device \r\n");

    retVal = DisplayIP();
    if(retVal < 0)
        LOOP_FOREVER();

    /*After calling this function, you can start sending data to CC3100 IP
    * address on PORT_NUM */
    retVal  = BsdTcpServer(PORT_NUM);
    if(retVal < 0)
           CLI_Write(" Failed to start TCP server \n\r");
    else
           CLI_Write(" TCP client connected successfully \n\r");


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
    \brief Opening a server side socket and receiving data

    This function opens a TCP socket in Listen mode and waits for an incoming
    TCP connection. If a socket connection is established then the function
    will try to read 1000 TCP packets from the connected client.

    \param[in]      port number on which the server will be listening on

    \return         0 on success, -1 on Error.

    \note           This function will wait for an incoming connection till one
                    is established

    \warning
*/
static _i32 BsdTcpServer(_u16 Port)
{
    SlSockAddrIn_t  Addr;
    SlSockAddrIn_t  LocalAddr;

    _u16          idx = 0;
    _u16          AddrSize = 0;
    _i16          SockID = 0;
    _i32          Status = 0;
    _i16          newSockID = 0;
    _u16          LoopCount = 0;
    _u16          recvSize = 0;

    for (idx=0 ; idx<BUF_SIZE ; idx++)
    {
        uBuf.BsdBuf[idx] = (_u8)(idx % 10);
    }

    LocalAddr.sin_family = SL_AF_INET;
    LocalAddr.sin_port = sl_Htons((_u16)Port);
    LocalAddr.sin_addr.s_addr = 0;

    SockID = sl_Socket(SL_AF_INET,SL_SOCK_STREAM, 0);
    if( SockID < 0 )
    {
        CLI_Write(" [TCP Server] Create socket Error \n\r");
        ASSERT_ON_ERROR(SockID);
    }

    AddrSize = sizeof(SlSockAddrIn_t);
    Status = sl_Bind(SockID, (SlSockAddr_t *)&LocalAddr, AddrSize);
    if( Status < 0 )
    {
        sl_Close(SockID);
        CLI_Write(" [TCP Server] Socket address assignment Error \n\r");
        ASSERT_ON_ERROR(Status);
    }

    Status = sl_Listen(SockID, 0);
    if( Status < 0 )
    {
        sl_Close(SockID);
        CLI_Write(" [TCP Server] Listen Error \n\r");
        ASSERT_ON_ERROR(Status);
    }

   newSockID = sl_Accept(SockID, ( struct SlSockAddr_t *)&Addr,
                              (SlSocklen_t*)&AddrSize);
   if( newSockID < 0 )
    {
        sl_Close(SockID);
        CLI_Write(" [TCP Server] Accept connection Error \n\r");
        ASSERT_ON_ERROR(newSockID);
    }

    while (LoopCount < NO_OF_PACKETS)
    {
        recvSize = BUF_SIZE;

        do
        {
            Status = sl_Recv(newSockID, &uBuf.BsdBuf[BUF_SIZE - recvSize], recvSize, 0);
            if( Status <= 0 )
            {
                sl_Close(newSockID);
                sl_Close(SockID);
                CLI_Write(" [TCP Server] Data recv Error \n\r");
                ASSERT_ON_ERROR(TCP_RECV_ERROR);
            }

            recvSize -= Status;

        }while(recvSize > 0);

        LoopCount++;
    }

    Status = sl_Close(newSockID);
    ASSERT_ON_ERROR(Status);

    Status = sl_Close(SockID);
    ASSERT_ON_ERROR(Status);

    return SUCCESS;
}

/*!
    \brief Connecting to a remote p2p device

    This function connects to the required p2p device (P2P_REMOTE_DEVICE).
    This code example connects using key (KEY).

    \param[in]  None

    \return     None

    \note

    \warning
*/
static _i32 establishConnectionWithP2Pdevice()
{
    SlSecParams_t secParams = {0};
    _i32 retVal = 0;

    secParams.Key = KEY;
    secParams.KeyLen = pal_Strlen(KEY);
    secParams.Type = SECURITY_TYPE;

    /* wlan connect call with dummy SSID to start the connection process */
    retVal = sl_WlanConnect(P2P_REMOTE_DEVICE, pal_Strlen(P2P_REMOTE_DEVICE), 0, &secParams, 0);
    ASSERT_ON_ERROR(retVal);

    while(!IS_P2P_NEG_REQ_RECEIVED(g_Status))
    {
        _SlNonOsMainLoopTask();
    }

    /* Connect with the device requesting the connection */
    retVal = sl_WlanConnect(g_p2p_dev, pal_Strlen(g_p2p_dev), 0, &secParams, 0);
    ASSERT_ON_ERROR(retVal);

    while ((!IS_IP_LEASED(g_Status) && !IS_IP_ACQUIRED(g_Status)) ||
            (!IS_CONNECTED(g_Status) && !IS_STA_CONNECTED(g_Status)))
    {
        _SlNonOsMainLoopTask();

        if(IS_CONNECTION_FAILED(g_Status))
        {
            /* Error, connection is failed */
            CLI_Write((_u8 *)" Connection Failed\r\n");
            ASSERT_ON_ERROR(P2P_CONNECTION_FAILED);
        }
    }

    return SUCCESS;
}

/*!
    \brief Display the IP Adderess of device

    \param[in]      none

    \return         none

    \note

    \warning
*/
static _i32 DisplayIP()
{
    _i32 retVal = -1;
    SlNetCfgIpV4Args_t ipV4 = {0};

    _u8  buff[18] = {'\0'};
    _u8  *ccPtr = 0;
    _u8  ccLen = 0;
    _u8  len = sizeof(SlNetCfgIpV4Args_t);
    _u8  dhcpIsOn = 0;

    ccPtr = buff;

    if(IS_IP_LEASED(g_Status))
    {
        /* Device is in GO mode, Get the IP of Device */
        retVal = sl_NetCfgGet(SL_IPV4_AP_P2P_GO_GET_INFO,&dhcpIsOn,&len,(_u8 *)&ipV4);
        ASSERT_ON_ERROR(retVal);
        g_DeviceIp = ipV4.ipV4;
    }

    ccLen = itoa((_u8)SL_IPV4_BYTE(g_DeviceIp,3), ccPtr);
    ccPtr += ccLen;
    *ccPtr++ = '.';
    ccLen = itoa((_u8)SL_IPV4_BYTE(g_DeviceIp,2), ccPtr);
    ccPtr += ccLen;
    *ccPtr++ = '.';
    ccLen = itoa((_u8)SL_IPV4_BYTE(g_DeviceIp,1), ccPtr);
    ccPtr += ccLen;
    *ccPtr++ = '.';
    ccLen = itoa((_u8)SL_IPV4_BYTE(g_DeviceIp,0), ccPtr);
    ccPtr += ccLen;

    CLI_Write((_u8 *)" Device IP: ");
    CLI_Write((_u8 *)buff);
    CLI_Write((_u8 *)"\r\n");

    return SUCCESS;
}

/*!
    \brief Convert integer to ASCII in decimal base

    \param[in]      cNum -  input integer number to convert
    \param[in]      cString -  pointer to output string

    \return         number of ASCII characters

    \note

    \warning
*/
static _u8 itoa(_i16 cNum, _u8 *cString)
{
    _u8* ptr = 0;
    _i16 uTemp = cNum;
    _u8 length = 0;

    /* value 0 is a special case */
    if (cNum == 0)
    {
        length = 1;
        *cString = '0';

        return length;
    }

    /* Find out the length of the number, in decimal base */
    while (uTemp > 0)
    {
        uTemp /= 10;
        length++;
    }

    /* Do the actual formatting, right to left */
    uTemp = cNum;
    ptr = cString + length;
    while (uTemp > 0)
    {
        --ptr;
        *ptr = digits[uTemp % 10];
        uTemp /= 10;
    }

    return length;
}

/*!
    \brief This function initializes the application variables

    \param[in]  None

    \return     0 on success, negative error-code on error
*/
static _i32 initializeAppVariables()
{
    g_Status = 0;
    g_DeviceIp = 0;
    pal_Memset(uBuf.BsdBuf, 0, sizeof(uBuf));
    pal_Memset(g_p2p_dev, 0, MAXIMAL_SSID_LENGTH + 1);

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
    CLI_Write(" P2P Application - Version ");
    CLI_Write(APPLICATION_VERSION);
    CLI_Write("\n\r*******************************************************************************\n\r");
}
