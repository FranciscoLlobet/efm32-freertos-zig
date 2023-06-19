/*
 * main.c - sample messaging application via xmpp server
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

/*
 * Application Name     -   XMPP client
 * Application Overview -   This is a sample application demonstrating how to
 *                          connect and work with XMPP server using CC3100. It
 *                          receives/sends an XMPP messages from/to the server.
 * Application Details  -   http://processors.wiki.ti.com/index.php/CC31xx_XMPP_Reference_Application
 *                          doc\examples\xmpp_client.pdf
 */

#include "xmpp.h"
#include "config.h"
#include "sl_common.h"


#define APPLICATION_VERSION "1.2.0"

#define SL_STOP_TIMEOUT        0xFF

/*
 * GLOBAL VARIABLES -- Start
 */
_u8 g_Status = 0;
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
    SlNetAppXmppOpt_t XmppOption;
    SlNetAppXmppUserName_t UserName;
    SlNetAppXmppPassword_t Password;
    SlNetAppXmppDomain_t Domain;
    SlNetAppXmppResource_t Resource;
    _u8 pRemoteJid[MAX_RCV_BUF_SIZE] = {'\0'};
    _u8 pRecvMessage[MAX_RCV_BUF_SIZE] = {'\0'};
    _i32 Status = -1;

    Status = initializeAppVariables();
    ASSERT_ON_ERROR(Status);

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
    Status = configureSimpleLinkToDefaultState();
    if(Status < 0)
    {
        if (DEVICE_NOT_IN_STATION_MODE == Status)
        {
            /* Failed to configure the device in its default state */
        }

        LOOP_FOREVER();
    }

    /*
     * Assumption is that the device is configured in station mode already
     * and it is in its default state
     */
    /* Initializing the CC3100 device */
    Status = sl_Start(0, 0, 0);
    if ((Status < 0) ||
        (ROLE_STA != Status) )
    {
        CLI_Write(" Failed to start the device \n\r");
        LOOP_FOREVER();
    }

    CLI_Write(" Device started as STATION \n\r");

    /* Connecting to WLAN AP - Set with static parameters defined at the top
       After this call we will be connected and have IP address */
    Status = establishConnectionWithAP();
    if(Status < 0)
    {
        CLI_Write(" Failed to establish connection w/ an AP \n\r");
        LOOP_FOREVER();
    }

    CLI_Write(" Connection established w/ AP and IP is acquired \n\r");

    CLI_Write(" Establishing connection with the XMPP server \n\r");

    /* Configuring parameters required for XMPP connection*/
    XmppOption.Port = XMPP_DST_PORT;
    XmppOption.Family = SL_AF_INET;
    XmppOption.SecurityMethod = SO_SECMETHOD_SSLV3;
    XmppOption.SecurityCypher = SECURE_MASK_SSL_RSA_WITH_RC4_128_SHA;
    XmppOption.Ip = XMPP_IP_ADDR;
    Status = sl_NetAppXmppSet(SL_NET_APP_XMPP_ID, NETAPP_XMPP_ADVANCED_OPT,
                     sizeof(SlNetAppXmppOpt_t), (_u8 *)&XmppOption);
    if(Status < 0)
    {
        CLI_Write(" Error during establishing connection with the XMPP server \n\r");
        LOOP_FOREVER();
    }

    pal_Memcpy(UserName.UserName, XMPP_USER, pal_Strlen(XMPP_USER));
    UserName.Length = pal_Strlen(XMPP_USER);
    Status = sl_NetAppXmppSet(SL_NET_APP_XMPP_ID, NETAPP_XMPP_USER_NAME,
                     UserName.Length, (_u8 *)&UserName);
    if(Status < 0)
    {
        CLI_Write(" Error during establishing connection with the XMPP server \n\r");
        LOOP_FOREVER();
    }

    pal_Memcpy(Password.Password, XMPP_PWD, pal_Strlen(XMPP_PWD));
    Password.Length = pal_Strlen(XMPP_PWD);
    Status = sl_NetAppXmppSet(SL_NET_APP_XMPP_ID, NETAPP_XMPP_PASSWORD,
                     Password.Length, (_u8 *)&Password);
    if(Status < 0)
    {
        CLI_Write(" Error during establishing connection with the XMPP server \n\r");
        LOOP_FOREVER();
    }

    pal_Memcpy(Domain.DomainName, XMPP_DOMAIN, pal_Strlen(XMPP_DOMAIN));
    Domain.Length = pal_Strlen(XMPP_DOMAIN);
    Status = sl_NetAppXmppSet(SL_NET_APP_XMPP_ID, NETAPP_XMPP_DOMAIN,
                     Domain.Length, (_u8 *)&Domain);
    if(Status < 0)
    {
        CLI_Write(" Error during establishing connection with the XMPP server \n\r");
        LOOP_FOREVER();
    }

    pal_Memcpy(Resource.Resource,XMPP_RESOURCE, pal_Strlen(XMPP_RESOURCE));
    Resource.Length = pal_Strlen(XMPP_RESOURCE);
    Status = sl_NetAppXmppSet(SL_NET_APP_XMPP_ID, NETAPP_XMPP_RESOURCE,
                     Resource.Length, (_u8 *)&Resource);
    if(Status < 0)
    {
        CLI_Write(" Error during establishing connection with the XMPP server \n\r");
        LOOP_FOREVER();
    }

    Status = sl_NetAppXmppConnect();
    if(Status < 0)
    {
        CLI_Write(" Error during establishing connection with the XMPP server \n\r");
        LOOP_FOREVER();
    }

    CLI_Write(" Connection established successfully with the XMPP server \n\r");

    CLI_Write(" Waiting for message....\n\r");

    Status = sl_NetAppXmppRecv(pRemoteJid, MAX_RCV_BUF_SIZE, pRecvMessage, MAX_RCV_BUF_SIZE );
    while ( Status < 0)
    {
        Status = sl_NetAppXmppRecv(pRemoteJid, MAX_RCV_BUF_SIZE, pRecvMessage, MAX_RCV_BUF_SIZE );
    }

    /* Send back the received message */
    Status = sl_NetAppXmppSend(pRemoteJid, pal_Strlen(pRemoteJid),
                               pRecvMessage, pal_Strlen(pRecvMessage));
    if(Status < 0)
    {
        CLI_Write(" Error while sending back the message \n\r");
        LOOP_FOREVER();
    }

    CLI_Write(" Message sent back successfully \n\r");

    /* 1 second delay */
    Delay(1000);

    /* Stop the CC3100 device */
    Status = sl_Stop(SL_STOP_TIMEOUT);
    if(Status < 0)
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

    return SUCCESS;
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
    CLI_Write(" XMPP client application - Version ");
    CLI_Write(APPLICATION_VERSION);
    CLI_Write("\n\r*******************************************************************************\n\r");
}
