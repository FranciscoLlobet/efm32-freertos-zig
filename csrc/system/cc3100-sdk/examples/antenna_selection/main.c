/*
 * main.c - Sample application for antenna selection
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
 * Application Name     -   Antenna Selection
 * Application Overview -   This is only a sample application demonstrating how
 *                          'antenna-selection' feature can be implemented on
 *                          Host-MCU. Please note below points when implementing
 *                          this feature on Host-MCU:
 *                          - CC3100, internally, doesn't support this feature
 *                          - In case the application intends to put the Host-MCU
 *                            in Lower Power Mode (LPM) while keeping CC3100
 *                            connected to the access-point, the state of the
 *                            IOs that control the RF-Switch shall be retained
 *
 *                          Not retaining these IOs will break the RF path for CC3100.
 *
 *                          - Few MCUs, like STM32 in STANDBY, don't retain the
 *                            IO states while in LPM. For implementing antenna-selection
 *                            feature on such MCUs, external bus-hold circuitry
 *                            shall be added between IOs and RF Switch to keep
 *                            the RF path intact for CC3100.
 * Application Details  -   http://processors.wiki.ti.com/index.php/CC31xx_Antenna_Selection
 *                          doc\examples\antenna_selection.pdf
 */

#include "simplelink.h"
#include "sl_common.h"

#define APPLICATION_VERSION "1.3.0"

#define SL_STOP_TIMEOUT        0xFF

#define RSSI_THRESHOLD_VALUE    -110
#define SCAN_TABLE_SIZE         20
#define RSSI_BUF_SIZE           5
#define SCAN_INTERVAL           60

/* Application specific status/error codes */
typedef enum{
    AP_NOT_FOUND = -0x7D0,        /* Choosing this number to avoid overlap with host-driver's error codes */
    DEVICE_NOT_IN_STATION_MODE = AP_NOT_FOUND - 1,
    /* ... */
    /* ... */
    STATUS_CODE_MAX = -0xBB8
}e_AppStatusCodes;

/*
 * GLOBAL VARIABLES -- Start
 */
Sl_WlanNetworkEntry_t g_NetEntries[SCAN_TABLE_SIZE] = {0};
_u32  g_Status = 0;
/*
 * GLOBAL VARIABLES -- End
 */


/*
 * CONSTANTS -- Start
 */
const _u8 DIGITS[] = "0123456789";
/*
 * CONSTANTS -- End
 */


/*
 * STATIC FUNCTION DEFINITIONS -- Start
 */
static _u16 itoa(_i16 cNum, _u8 *cString);

static _i8  getSignalStrength();
static _i16 selectAntenna();

static _i32 establishConnectionWithAP();
static _i32 configureSimpleLinkToDefaultState();

static _i32 initializeAppVariables();
static void  displayBanner();
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
               SL_USER_INITIATED_DISCONNECTION */
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
             * Information about the connected AP's IP, gateway, DNS etc.
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
     * This application doesn't work w/ HTTP server - Hence these
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
     * This application doesn't work w/ socket - Hence these
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
     * policies, power policy etc.)
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

    /* Initialize the antenna selection GPIO */
    initAntSelGPIO(); /* Returns void */

    /*
     * Initializing the CC3100 device...
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

    while(1) /* Break on a failure */
    {
        /* Select antenna with better signal strength */
        retVal = selectAntenna();
        if(retVal < 0)
        {
            if (!(AP_NOT_FOUND == retVal))
                break;

            /* Else, continue with Antenna 1 since RSSI with both antenna is below threshold */
        }

        /* Connecting to WLAN AP - Set with static parameters defined at the top
         * After this call we will be connected and have IP address
         */
        retVal = establishConnectionWithAP();
        if(retVal < 0)
        {
            CLI_Write(" Failed to establish connection w/ an AP \n\r");
            LOOP_FOREVER();
        }

        CLI_Write(" Connection established with ");
        CLI_Write((_u8 *)SSID_NAME);
        CLI_Write(" and IP is acquired \n\r");

        /* In case the device gets disconnected from the AP,
         * selectAntenna() followed by establishConnectionWithAP()
         * can be called to reselect the antenna with better signal
         * strength and connect to AP.
         */
        while(IS_CONNECTED(g_Status)) { _SlNonOsMainLoopTask(); }
    }

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
static _u16 itoa(_i16 cNum, _u8 *cString)
{
    _u16 length = 0;
    _u8* ptr = NULL;

    _i16 uTemp = -1;

    /* Value 0 is a special case */
    if (0 == cNum)
    {
        length = 1;
        *cString = '0';

        return length;
    }

    if(cNum < 0)
    {
        length++;
        *cString = '-';
        cNum *= -1;
    }

    uTemp = cNum;

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
        *ptr = DIGITS[uTemp % 10];
        uTemp /= 10;
    }

    return length;
}

/*!
    \brief This function Get the signal strength for a SSID

    \param[in]      none

    \return         RSSI value

    \note

    \warning
*/
static _i8 getSignalStrength()
{
    _u32  IntervalVal = SCAN_INTERVAL;
    _u8   policyOpt = 0;

    _i16 retVal = -1;
    _i16 idx = -1;

    policyOpt = SL_CONNECTION_POLICY(0, 0, 0, 0, 0);
    retVal = sl_WlanPolicySet(SL_POLICY_CONNECTION , policyOpt, NULL, 0);
    if(retVal < 0)
    {
        /* Error */
        return (RSSI_THRESHOLD_VALUE-1);
    }

    /* Enable scan */
    policyOpt = SL_SCAN_POLICY(1);

    /* Set scan policy - this starts the scan */
    retVal = sl_WlanPolicySet(SL_POLICY_SCAN , policyOpt,
                            (_u8 *)(&IntervalVal), sizeof(IntervalVal));
    if(retVal < 0)
    {
        /* Error */
        return (RSSI_THRESHOLD_VALUE-1);
    }

    /* Delay 1 second to verify scan is started */
    Delay(1000);

    pal_Memset(g_NetEntries, '\0', (SCAN_TABLE_SIZE * sizeof(Sl_WlanNetworkEntry_t)));

    /* retVal indicates the valid number of entries
     * The scan results are occupied in g_NetEntries[]
     */
    retVal = sl_WlanGetNetworkList(0, SCAN_TABLE_SIZE, &g_NetEntries[0]);

    /* Stop the scan */
    policyOpt = SL_SCAN_POLICY(0);

    /* Set scan policy - this will stop the scan the scan */
    if(sl_WlanPolicySet(SL_POLICY_SCAN , policyOpt, 0, 0) < 0)
        return (RSSI_THRESHOLD_VALUE-1);

    for(idx = 0; idx < retVal; idx++)
    {
        if(pal_Strcmp(g_NetEntries[idx].ssid, SSID_NAME) == 0)
            return (g_NetEntries[idx].rssi);
    }

    return (RSSI_THRESHOLD_VALUE-1);
}

/*!
    \brief This function selects the antenna with better signal strength

    \param[in]      none

    \return         -1 if AP is not available at both antenna, 0 otherwise

    \note

    \warning
*/
static _i16 selectAntenna()
{
    _u8 pBuff[RSSI_BUF_SIZE] = {'\0'};
    _i8 Ant1_RSSI = -1;
    _i8 Ant2_RSSI = -1;

    CLI_Write((_u8 *)" AP configured by application is ");
    CLI_Write((_u8 *)SSID_NAME);
    CLI_Write((_u8 *)"\r\n\r\n");

    CLI_Write((_u8 *)" Selecting antenna 1\r\n");
    /* Switch to Antenna 1 */
    SelAntenna(ANT1);

    /* Get signal strength at Antenna 1 */
    Ant1_RSSI = getSignalStrength();
    pal_Memset(pBuff, '\0', RSSI_BUF_SIZE);
    itoa((_i16)Ant1_RSSI,pBuff);
    CLI_Write((_u8 *)" RSSI value: ");
    CLI_Write(pBuff);
    CLI_Write((_u8 *)"\r\n\r\n");

    CLI_Write((_u8 *)" Selecting antenna 2\r\n");
    /*Switch to Antenna 2 */
    SelAntenna(ANT2);

    /* Get Signal Strength at Antenna 2 */
    Ant2_RSSI = getSignalStrength();
    pal_Memset(pBuff, '\0', RSSI_BUF_SIZE);
    itoa((_i16)Ant2_RSSI,pBuff);
    CLI_Write((_u8 *)" RSSI value: ");
    CLI_Write(pBuff);
    CLI_Write((_u8 *)"\r\n\r\n");

    if(Ant2_RSSI < RSSI_THRESHOLD_VALUE  && Ant1_RSSI < RSSI_THRESHOLD_VALUE)
    {
        CLI_Write((_u8 *)" Unable to find AP: ");
        CLI_Write((_u8 *)SSID_NAME);
        CLI_Write((_u8 *)"\r\n\r\n");
        CLI_Write((_u8 *)" Continuing with antenna 1\r\n");

        /*Select Antenna 1 */
        SelAntenna(ANT1);
        ASSERT_ON_ERROR(AP_NOT_FOUND);
    }

    /* Switch to antenna with better signal strength */
    if(Ant1_RSSI > Ant2_RSSI)
    {
        SelAntenna(ANT1);
        CLI_Write((_u8 *)" Antenna 1 selected since it delivered better signal strength \r\n");
    }
    else
    {
        CLI_Write((_u8 *)" Antenna 2 selected since it delivered better signal strength \r\n");
    }

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
    retVal = sl_WlanSet(SL_WLAN_CFG_GENERAL_PARAM_ID, WLAN_GENERAL_PARAM_OPT_STA_TX_POWER, 1, &power);
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
    secParams.KeyLen = PASSKEY_LEN;
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
    pal_Memset(g_NetEntries, 0, sizeof(g_NetEntries));

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
    CLI_Write(" Antenna selection application - Version ");
    CLI_Write(APPLICATION_VERSION);
    CLI_Write("\n\r*******************************************************************************\n\r");
}
