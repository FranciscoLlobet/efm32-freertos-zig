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


#include "simplelink.h"
#include "net.h"
#include "sl_common.h"


#define SL_STOP_TIMEOUT   0xFF

#define FAILURE                 -1
#define SUCCESS                 0


/*
 * GLOBAL VARIABLES -- Start
 */
_u32  g_ulStatus = 0;


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
            SET_STATUS_BIT(g_ulStatus, STATUS_BIT_CONNECTION);

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

            CLR_STATUS_BIT(g_ulStatus, STATUS_BIT_CONNECTION);
            CLR_STATUS_BIT(g_ulStatus, STATUS_BIT_IP_ACQUIRED);

            pEventData = &pWlanEvent->EventData.STAandP2PModeDisconnected;

            /* If the user has initiated 'Disconnect' request, 'reason_code' is SL_USER_INITIATED_DISCONNECTION */
            if(SL_WLAN_DISCONNECT_USER_INITIATED_DISCONNECTION == pEventData->reason_code)
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
    {
        CLI_Write(" [NETAPP EVENT] NULL Pointer Error \n\r");
        return;
    }

    switch(pNetAppEvent->Event)
    {
        case SL_NETAPP_IPV4_IPACQUIRED_EVENT:
        {
            SET_STATUS_BIT(g_ulStatus, STATUS_BIT_IP_ACQUIRED);
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
     * Most of the general errors are not FATAL are to be handled
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
                    CLI_Write((_u8 *)" [SOCK EVENT] Close socket operation failed to transmit all queued packets\n\r");
                break;


                default:
                    CLI_Write((_u8 *)" [SOCK EVENT] Unexpected event \n\r");
                break;
            }
        break;

        default:
            CLI_Write((_u8 *)" [SOCK EVENT] Unexpected event \n\r");
        break;
    }
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
_i32 configureSimpleLinkToDefaultState()
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
            /* If the device is in AP mode, we need to wait for this
             * event before doing anything
             */
            while(!IS_IP_ACQUIRED(g_ulStatus)) { _SlNonOsMainLoopTask(); }
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
            /* We don't want to proceed if the device is not coming up in
             * station-mode
             */
            ASSERT_ON_ERROR(DEVICE_NOT_IN_STATION_MODE);
        }
    }

    /* Get the device's version-information */
    configOpt = SL_DEVICE_GENERAL_VERSION;
    configLen = sizeof(ver);
    retVal = sl_DevGet(SL_DEVICE_GENERAL_CONFIGURATION, &configOpt,
            &configLen, (_u8 *)(&ver));
    ASSERT_ON_ERROR(retVal);

    /* Set connection policy to Auto + SmartConfig
     * (Device's default connection policy)
     */
    retVal = sl_WlanPolicySet(SL_POLICY_CONNECTION,
            SL_CONNECTION_POLICY(1, 0, 0, 0, 1), NULL, 0);
    ASSERT_ON_ERROR(retVal);

    /* Remove all profiles */
    retVal = sl_WlanProfileDel(0xFF);
    ASSERT_ON_ERROR(retVal);

    /*
     * Device in station-mode. Disconnect previous connection if any
     * The function returns 0 if 'Disconnected done', negative number if already
     * disconnected
     * Wait for 'disconnection' event if 0 is returned, Ignore other return-codes
     */
    retVal = sl_WlanDisconnect();
    if(0 == retVal)
    {
        /* Wait */
        while(IS_CONNECTED(g_ulStatus)) { _SlNonOsMainLoopTask(); }
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
    retVal = sl_WlanSet(SL_WLAN_CFG_GENERAL_PARAM_ID,
            WLAN_GENERAL_PARAM_OPT_STA_TX_POWER, 1, (_u8 *)&power);
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

    return retVal; /* Success */
}

_i32 NetStop()
{
    _i32 lRetVal = 0;

    /* Disconnect from the AP */
    lRetVal = sl_WlanDisconnect();
    if(0 == lRetVal)
    {
        /* Wait */
        while(IS_CONNECTED(g_ulStatus))
        {
              _SlNonOsMainLoopTask();
        }
    }

    /* Stop the simplelink host */
    lRetVal = sl_Stop(SL_STOP_TIMEOUT);

    /* Reset all variables */
    g_ulStatus = 0;

    return lRetVal;
}

_i32 NetStart()
{
    _i32 lRetVal = 0;

    lRetVal = sl_Start(0,0,0);
    if(lRetVal < 0 || lRetVal != ROLE_STA)
    {
        /* Failed to start device in STation mode */
        ASSERT_ON_ERROR(DEVICE_NOT_IN_STATION_MODE);
    }

    return SUCCESS;
}

/*
 * \brief Connecting to a WLAN Accesspoint
 *
 * This function connects to the required AP (SSID_NAME) with Security
 * parameters specified in te form of macros at the top of this file
 *
 * \param[in]   cssid     - SSID of the AP
 *              secParams - security parameters for the AP
 *
 * \return  0 for success, -ve otherwise
 *
 * \warning    If the WLAN connection fails or we don't aquire an IP
 *            address, It will be stuck in this function forever.
 */
_i32 NetWlanConnect(const _u8 *cssid, SlSecParams_t *secParams)
{
    _i32 lRetVal = 0;

    lRetVal = sl_WlanConnect((_i8 *)cssid, strlen((const char *)cssid), 0,
            secParams, 0);
    if( lRetVal != 0)
    {
      return lRetVal;
    }

    while((!IS_CONNECTED(g_ulStatus)) || (!IS_IP_ACQUIRED(g_ulStatus)))
    {
        /* Wait for WLAN Event */
        _SlNonOsMainLoopTask();

    }

    return SUCCESS;

}

/*
 * Network_IF_GetHostIP
 *
 * \brief  This function obtains the server IP address using a DNS lookup
 *
 * \param[in]  pcHostName        The server hostname
 * \param[out] pDestinationIP    This parameter is filled with host IP address.
 *
 * \return On success, +ve value is returned. On error, -ve value is returned
 */
_i32 NetGetHostIP( _u8* pcHostName,_u32 * pDestinationIP )
{
    _i32 lStatus = 0;

    lStatus = sl_NetAppDnsGetHostByName((_i8 *) pcHostName,
                                        strlen((const char *)pcHostName),
                        pDestinationIP, SL_AF_INET);

    return lStatus;

}

/*
 * Get the device mac address
 *
 * \brief This function gets the MAC address of the device
 *
 * \param[out]  pMACAddress is pointer to variable where MAC address will be stored
 *
 * \return      0 for success, -ve otherwise
 */
_i32 NetMACAddressGet(_u8 *pMACAddress)
{
    _i32 lRetVal = 0;
    _u8 macAddressLen = SL_MAC_ADDR_LEN;

    /* Get the MAC address */
    lRetVal = sl_NetCfgGet(SL_MAC_ADDRESS_GET, NULL, &macAddressLen, pMACAddress);
    return lRetVal;

}

/*
 * \brief The Function gets the network firmware info
 *
 * \param[out]  sVersion - Pointer to SlVersionFull Info structure
 *
 * \return      0 for success, -ve otherwise
 */

_i32 NetFwInfoGet(SlVersionFull *sVersion)
{
  _i32 lRetVal = 0;
  _u8 ucConfigOpt = 0;
  _u8 ucConfigLen = 0;

  /* Get the device's version-information */
  ucConfigOpt = SL_DEVICE_GENERAL_VERSION;
  ucConfigLen = sizeof(SlVersionFull);
  lRetVal = sl_DevGet(SL_DEVICE_GENERAL_CONFIGURATION, &ucConfigOpt,
                              &ucConfigLen, (_u8 *)(sVersion));

  return lRetVal;
}

/*
 * \brief The Function check if wifi is connected to AP
 *
 * \return true if connected, false otherwise.
 */
_u8 NetIsConnectedToAP()
{
  return IS_CONNECTED(g_ulStatus);
}
