/*
 * main.c - HTTP server sample example
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
 * Application Name     -   HTTP Server
 * Application Overview -   This is a sample application demonstrating the
 *                          capability of CC3100 device to work as a
 *                          web-server and allowing the end-users to
 *                          communicate w/ it using standard web-browsers.
 * Application Details  -   http://processors.wiki.ti.com/index.php/CC31xx_HTTP_Server
 *                          doc\examples\http_server.pdf
 */

#include "simplelink.h"
#include "protocol.h"
#include "sl_common.h"

#define APPLICATION_VERSION "1.3.0"

#define SL_STOP_TIMEOUT        0xFF

_u8 POST_token[] = "__SL_P_ULD";
_u8 GET_token[]  = "__SL_G_ULD";

/* Application specific status/error codes */
typedef enum{
    DEVICE_NOT_IN_STATION_MODE = -0x7D0,        /* Choosing this number to avoid overlap w/ host-driver's error codes */

    STATUS_CODE_MAX = -0xBB8
}e_AppStatusCodes;

/*
 * GLOBAL VARIABLES -- Start
 */
_u32  g_Status = 0;
_u8 g_auth_name[MAX_AUTH_NAME_LEN+1];
_u8 g_auth_password[MAX_AUTH_PASSWORD_LEN+1];
_u8 g_auth_realm[MAX_AUTH_REALM_LEN+1];

_u8 g_domain_name[MAX_DOMAIN_NAME_LEN];
_u8 g_device_urn[MAX_DEVICE_URN_LEN];
/*
 * GLOBAL VARIABLES -- End
 */


/*
 * STATIC FUNCTION DEFINITIONS -- Start
 */
static _i32 configureSimpleLinkToDefaultState();
static _i32 initializeAppVariables();
static void displayBanner();

static _i32 set_authentication_check (_u8 enable);
static _i32 get_auth_name (_u8 *auth_name);
static _i32 get_auth_password (_u8 *auth_password);
static _i32 get_auth_realm (_u8 *auth_realm);
static _i32 get_device_urn (_u8 *device_urn);
static _i32 get_domain_name (_u8 *domain_name);

_i32 set_port_number(_u16 num);
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
                CLI_Write((_u8 *)" Device disconnected from the AP on application's request \n\r");
            }
            else
            {
                CLI_Write((_u8 *)" Device disconnected from the AP on an ERROR..!! \n\r");
            }
        }
        break;

        case SL_WLAN_STA_CONNECTED_EVENT:
        {
            SET_STATUS_BIT(g_Status, STATUS_BIT_STA_CONNECTED);
        }
        break;

        case SL_WLAN_STA_DISCONNECTED_EVENT:
        {
            CLR_STATUS_BIT(g_Status, STATUS_BIT_STA_CONNECTED);
            CLR_STATUS_BIT(g_Status, STATUS_BIT_IP_LEASED);
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
            _u8 status = 0;
            _u8 *ptr = 0;

            ptr = pResponse->ResponseData.token_value.data;
            pResponse->ResponseData.token_value.len = 0;
            if(pal_Memcmp(pEvent->EventData.httpTokenName.data, GET_token,
                                         pal_Strlen(GET_token)) == 0)
            {
                status = GetLEDStatus();
                pal_Memcpy(ptr, "LED1_", pal_Strlen("LED1_"));
                ptr += 5;
                pResponse->ResponseData.token_value.len += 5;
                if(status & 0x01)
                {
                    pal_Memcpy(ptr, "ON", pal_Strlen("ON"));
                    ptr += 2;
                    pResponse->ResponseData.token_value.len += 2;
                }
                else
                {
                    pal_Memcpy(ptr, "OFF", pal_Strlen("OFF"));
                    ptr += 3;
                    pResponse->ResponseData.token_value.len += 3;
                }
                pal_Memcpy(ptr,",LED2_", pal_Strlen(",LED2_"));
                ptr += 6;
                pResponse->ResponseData.token_value.len += 6;
                if(status & 0x02)
                {
                    pal_Memcpy(ptr, "ON", pal_Strlen("ON"));
                    ptr += 2;
                    pResponse->ResponseData.token_value.len += 2;
                }
                else
                {
                    pal_Memcpy(ptr, "OFF", pal_Strlen("OFF"));
                    ptr += 3;
                    pResponse->ResponseData.token_value.len += 3;
                }

                *ptr = '\0';
            }

        }
        break;

        case SL_NETAPP_HTTPPOSTTOKENVALUE_EVENT:
        {
            _u8 led = 0;
            _u8 *ptr = pEvent->EventData.httpPostData.token_name.data;

            if(pal_Memcmp(ptr, POST_token, pal_Strlen(POST_token)) == 0)
            {
                ptr = pEvent->EventData.httpPostData.token_value.data;
                if(pal_Memcmp(ptr, "LED", 3) != 0)
                    break;

                ptr += 3;
                led = *ptr;
                ptr += 2;
                if(led == '1')
                {
                    if(pal_Memcmp(ptr, "ON", 2) == 0)
                    {
                        turnLedOn(LED1);
                    }
                    else
                    {
                        turnLedOff(LED1);
                    }
                }
                else if(led == '2')
                {
                    if(pal_Memcmp(ptr, "ON", 2) == 0)
                    {
                        turnLedOn(LED2);
                    }
                    else
                    {
                        turnLedOff(LED2);
                    }
                }
            }
        }
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

        case SL_NETAPP_IP_LEASED_EVENT:
        {
            SET_STATUS_BIT(g_Status, STATUS_BIT_IP_LEASED);
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
    _u8   SecType = 0;
    _i32   retVal = -1;
    _i32   mode = ROLE_STA;

    retVal = initializeAppVariables();
    ASSERT_ON_ERROR(retVal);

    /* Stop WDT and initialize the system-clock of the MCU */
    stopWDT();
    initClk();

    initLEDs();

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
    mode = sl_Start(0, 0, 0);
    if(mode < 0)
    {
        LOOP_FOREVER();
    }
    else
    {
        if (ROLE_AP == mode)
        {
            /* If the device is in AP mode, we need to wait for this
             * event before doing anything */
            while(!IS_IP_ACQUIRED(g_Status)) { _SlNonOsMainLoopTask(); }
        }
        else
        {
            /* Configure CC3100 to start in AP mode */
            retVal = sl_WlanSetMode(ROLE_AP);
            if(retVal < 0)
                LOOP_FOREVER();
        }
    }

    /* Configure AP mode without security */
    retVal = sl_WlanSet(SL_WLAN_CFG_AP_ID, WLAN_AP_OPT_SSID,
               pal_Strlen(SSID_AP_MODE), (_u8 *)SSID_AP_MODE);
    if(retVal < 0)
        LOOP_FOREVER();

    SecType = SEC_TYPE_AP_MODE;
    /* Configure the Security parameter in the AP mode */
    retVal = sl_WlanSet(SL_WLAN_CFG_AP_ID, WLAN_AP_OPT_SECURITY_TYPE, 1,
            (_u8 *)&SecType);
    if(retVal < 0)
        LOOP_FOREVER();

    retVal = sl_WlanSet(SL_WLAN_CFG_AP_ID, WLAN_AP_OPT_PASSWORD, pal_Strlen(PASSWORD_AP_MODE),
            (_u8 *)PASSWORD_AP_MODE);
    if(retVal < 0)
        LOOP_FOREVER();

    /* Restart the CC3100 */
    retVal = sl_Stop(SL_STOP_TIMEOUT);
    if(retVal < 0)
        LOOP_FOREVER();

    g_Status = 0;

    mode = sl_Start(0, 0, 0);
    if (ROLE_AP == mode)
    {
        /* If the device is in AP mode, we need to wait for this event before doing anything */
        while(!IS_IP_ACQUIRED(g_Status)) { _SlNonOsMainLoopTask(); }
    }
    else
    {
        CLI_Write(" Device couldn't come in AP mode \n\r");
        LOOP_FOREVER();
    }

    CLI_Write(" \r\n Device is configured in AP mode \n\r");

    CLI_Write(" Waiting for client to connect\n\r");
    /* wait for client to connect */
    while((!IS_IP_LEASED(g_Status)) || (!IS_STA_CONNECTED(g_Status))) { _SlNonOsMainLoopTask(); }

    CLI_Write(" Client connected\n\r");

    /* Enable the HTTP Authentication */
    retVal = set_authentication_check(TRUE);
    if(retVal < 0)
        LOOP_FOREVER();

    /* Get authentication parameters */
    retVal = get_auth_name(g_auth_name);
    if(retVal < 0)
        LOOP_FOREVER();

    retVal = get_auth_password(g_auth_password);
    if(retVal < 0)
        LOOP_FOREVER();

    retVal = get_auth_realm(g_auth_realm);
    if(retVal < 0)
        LOOP_FOREVER();

    CLI_Write((_u8 *)"\r\n Authentication parameters: ");
    CLI_Write((_u8 *)"\r\n Name = ");
    CLI_Write(g_auth_name);
    CLI_Write((_u8 *)"\r\n Password = ");
    CLI_Write(g_auth_password);
    CLI_Write((_u8 *)"\r\n Realm = ");
    CLI_Write(g_auth_realm);

    /* Get the domain name */
    retVal = get_domain_name(g_domain_name);
    if(retVal < 0)
        LOOP_FOREVER();

    CLI_Write((_u8 *)"\r\n\r\n Domain name = ");
    CLI_Write(g_domain_name);

    /* Get URN */
    retVal = get_device_urn(g_device_urn);
    if(retVal < 0)
        LOOP_FOREVER();

    CLI_Write((_u8 *)"\r\n Device URN = ");
    CLI_Write(g_device_urn);
    CLI_Write((_u8 *)"\r\n");

    /* Process the async events from the NWP */
    while(1)
    {
        _SlNonOsMainLoopTask();
    }
}

/*!
    \brief Set the HTTP port

    This function can be used to change the default port (80) for HTTP request

    \param[in]      num- contains the port number to be set

    \return         None

    \note

    \warning
*/
_i32 set_port_number(_u16 num)
{
    _NetAppHttpServerGetSet_port_num_t port_num;
    _i32 status = -1;

    port_num.port_number = num;

    /*Need to restart the server in order for the new port number configuration
     *to take place */
    status = sl_NetAppStop(SL_NET_APP_HTTP_SERVER_ID);
    ASSERT_ON_ERROR(status);

    status  = sl_NetAppSet (SL_NET_APP_HTTP_SERVER_ID, NETAPP_SET_GET_HTTP_OPT_PORT_NUMBER,
                  sizeof(_NetAppHttpServerGetSet_port_num_t), (_u8 *)&port_num);
    ASSERT_ON_ERROR(status);

    status = sl_NetAppStart(SL_NET_APP_HTTP_SERVER_ID);
    ASSERT_ON_ERROR(status);

    return SUCCESS;
}

/*!
    \brief Enable/Disable the authentication check for http server,
           By default authentication is disabled.

    \param[in]      enable - false to disable and true to enable the authentication

    \return         None

    \note

    \warning
*/
static _i32 set_authentication_check (_u8 enable)
{
    _NetAppHttpServerGetSet_auth_enable_t auth_enable;
    _i32 status = -1;

    auth_enable.auth_enable = enable;
    status = sl_NetAppSet(SL_NET_APP_HTTP_SERVER_ID, NETAPP_SET_GET_HTTP_OPT_AUTH_CHECK,
                 sizeof(_NetAppHttpServerGetSet_auth_enable_t), (_u8 *)&auth_enable);
    ASSERT_ON_ERROR(status);

    return SUCCESS;
}

/*!
    \brief Get the authentication user name

    \param[in]      auth_name - Pointer to the string to store authentication
                    name

    \return         None

    \note

    \warning
*/
static _i32 get_auth_name (_u8 *auth_name)
{
    _u8 len = MAX_AUTH_NAME_LEN;
    _i32 status = -1;

    status = sl_NetAppGet(SL_NET_APP_HTTP_SERVER_ID, NETAPP_SET_GET_HTTP_OPT_AUTH_NAME,
                 &len, (_u8 *) auth_name);
    ASSERT_ON_ERROR(status);

    auth_name[len] = '\0';

    return SUCCESS;
}

/*!
    \brief Get the authentication password

    \param[in]      auth_password - Pointer to the string to store
                    authentication password

    \return         None

    \note

    \warning
*/
static _i32 get_auth_password (_u8 *auth_password)
{
    _u8 len = MAX_AUTH_PASSWORD_LEN;
    _i32 status = -1;

    status = sl_NetAppGet(SL_NET_APP_HTTP_SERVER_ID, NETAPP_SET_GET_HTTP_OPT_AUTH_PASSWORD,
                                                &len, (_u8 *) auth_password);
    ASSERT_ON_ERROR(status);

    auth_password[len] = '\0';

    return SUCCESS;
}

/*!
    \brief Get the authentication realm

    \param[in]      auth_realm - Pointer to the string to store authentication
                    realm

    \return         None

    \note

    \warning
*/
static _i32 get_auth_realm (_u8 *auth_realm)
{
    _u8 len = MAX_AUTH_REALM_LEN;
    _i32 status = -1;

    status = sl_NetAppGet(SL_NET_APP_HTTP_SERVER_ID, NETAPP_SET_GET_HTTP_OPT_AUTH_REALM,
                 &len, (_u8 *) auth_realm);
    ASSERT_ON_ERROR(status);

    auth_realm[len] = '\0';

    return SUCCESS;
}

/*!
    \brief Get the device URN

    \param[in]      device_urn - Pointer to the string to store device urn

    \return         None

    \note

    \warning
*/
static _i32 get_device_urn (_u8 *device_urn)
{
    _u8 len = MAX_DEVICE_URN_LEN;
    _i32 status = -1;

    status = sl_NetAppGet(SL_NET_APP_DEVICE_CONFIG_ID, NETAPP_SET_GET_DEV_CONF_OPT_DEVICE_URN,
                 &len, (_u8 *) device_urn);
    ASSERT_ON_ERROR(status);

    device_urn[len] = '\0';

    return SUCCESS;
}

/*!
    \brief Get the domain Name

    \param[in]      domain_name - Pointer to the string to store domain name

    \return         None

    \note

    \warning        Domain name is used only in AP mode.
*/
static _i32 get_domain_name (_u8 *domain_name)
{
    _u8 len = MAX_DOMAIN_NAME_LEN;
    _i32 status = -1;

    status = sl_NetAppGet(SL_NET_APP_DEVICE_CONFIG_ID, NETAPP_SET_GET_DEV_CONF_OPT_DOMAIN_NAME,
                 &len, (_u8 *)domain_name);
    ASSERT_ON_ERROR(status);

    domain_name[len] = '\0';

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
    \brief This function initializes the application variables

    \param[in]  None

    \return     0 on success, negative error-code on error
*/
static _i32 initializeAppVariables()
{
    g_Status = 0;
    pal_Memset(g_auth_name, 0, sizeof(g_auth_name));
    pal_Memset(g_auth_password, 0, sizeof(g_auth_name));
    pal_Memset(g_auth_realm, 0, sizeof(g_auth_name));
    pal_Memset(g_domain_name, 0, sizeof(g_auth_name));
    pal_Memset(g_device_urn, 0, sizeof(g_auth_name));

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
    CLI_Write(" HTTP Server application - Version ");
    CLI_Write(APPLICATION_VERSION);
    CLI_Write("\n\r*******************************************************************************\n\r");
}
