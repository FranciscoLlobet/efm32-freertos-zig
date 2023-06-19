/*
 * main.c - file operation application
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
 * Application Name     -   File operations
 * Application Overview -   This is a sample application demonstrating the
 *                          file-operation capabilities of CC3100 device.
 *                          Serial-flash is used as the storage medium
 * Application Details  -   http://processors.wiki.ti.com/index.php/CC31xx_File_Operations_Application
 *                          doc\examples\file_operations.pdf
 */

#include "simplelink.h"
#include "sl_common.h"

#define APPLICATION_VERSION "1.3.0"

#define SL_STOP_TIMEOUT        0xFF

#define SL_FILE_NAME    "MacDonalds.txt"
#define BUF_SIZE        2048
#define SIZE_64K        65536

/* Application specific status/error codes */
typedef enum{
    DEVICE_NOT_IN_STATION_MODE = -0x7D0,        /* Choosing this number to avoid overlap with host-driver's error codes */

    STATUS_CODE_MAX = -0xBB8
}e_AppStatusCodes;

/*
 * GLOBAL VARIABLES -- Start
 */
_u32  g_Status = 0;
union{
    _u8 g_Buf[BUF_SIZE];
    _u32 demobuf[BUF_SIZE/sizeof(_u32)];
} uNvmemBuf;
/*
 * GLOBAL VARIABLES -- End
 */

/*
 * CONSTANTS -- Start
 */
const _u8    oldMacDonald[] = "Old MacDonald had a farm,E-I-E-I-O, \
        And on his farm he had a cow, \
        E-I-E-I-O, \
        With a moo-moo here, \
        And a moo-moo there, \
        Here a moo, there a moo, \
        Everywhere a moo-moo. \
        Old MacDonald had a farm, \
        E-I-E-I-O. \
        Old MacDonald had a farm, \
        E-I-E-I-O, \
        And on his farm he had a pig, \
        E-I-E-I-O, \
        With an oink-oink here, \
        And an oink-oink there, \
        Here an oink, there an oink, \
        Everywhere an oink-oink. \
        Old MacDonald had a farm, \
        E-I-E-I-O. \
        Old MacDonald had a farm, \
        E-I-E-I-O, \
        And on his farm he had a duck, \
        E-I-E-I-O, \
        With a quack-quack here, \
        And a quack-quack there, \
        Here a quack, there a quack, \
        Everywhere a quack-quack. \
        Old MacDonald had a farm, \
        E-I-E-I-O. \
        Old MacDonald had a farm, \
        E-I-E-I-O, \
        And on his farm he had a horse, \
        E-I-E-I-O, \
        With a neigh-neigh here, \
        And a neigh-neigh there, \
        Here a neigh, there a neigh, \
        Everywhere a neigh-neigh. \
        Old MacDonald had a farm, \
        E-I-E-I-O. \
        Old MacDonald had a farm, \
        E-I-E-I-O, \
        And on his farm he had a donkey, \
        E-I-E-I-O, \
        With a hee-haw here, \
        And a hee-haw there, \
        Here a hee, there a hee, \
        Everywhere a hee-haw. \
        Old MacDonald had a farm, \
        E-I-E-I-O. \
        Old MacDonald had a farm, \
        E-I-E-I-O, \
        And on his farm he had some chickens, \
        E-I-E-I-O, \
        With a cluck-cluck here, \
        And a cluck-cluck there, \
        Here a cluck, there a cluck, \
        Everywhere a cluck-cluck. \
        Old MacDonald had a farm, \
        E-I-E-I-O.";
/*
 * CONSTANTS -- End
 */


/*
 * STATIC FUNCTION DEFINITIONS -- Start
 */
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
    if(pSock == NULL)
    {
        CLI_Write(" [SOCK EVENT] NULL Pointer Error \n\r");
        return;
    }
    
    switch( pSock->Event )
    {
        case SL_SOCKET_TX_FAILED_EVENT:
        {
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


/*!

 \brief The aim of this example code is to demonstrate NVMEM capabilities of
        the device.
        The procedure includes the following steps:
            1) open/Create a user file for writing
            2) write "Old MacDonalds" child song (SIZE_64K/sizeof(oldMacDonald))
               times to get just below a 63KB file
            3) close the user file
            4) open the user file for reading
            5) read the data and compare with the stored buffer
            6) close the user file
            7) Delete the file

 \param             None

 \return            -1 or -2 in case of error, 0 otherwise
                    Also, LED2 is turned solid in case of success
                    LED1 is turned solid in case of failure

 \note

 \warning

*/
/*
 * Application's entry point
 */

int main(int argc, char** argv)
{
    _i32         fileHandle = -1;
    _i32         retVal = -1;
    _u32        Token = 0;
    _u16        loop = 0;


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

    /* Open a user file for writing */
    retVal = sl_FsOpen((_u8 *)SL_FILE_NAME,
                       FS_MODE_OPEN_WRITE, &Token, &fileHandle);
    if(retVal < 0)
    {
        /* File Doesn't exit create a new of 63 KB file */
        retVal = sl_FsOpen((_u8 *)SL_FILE_NAME,
                           FS_MODE_OPEN_CREATE(SIZE_64K,_FS_FILE_OPEN_FLAG_COMMIT|_FS_FILE_PUBLIC_WRITE),
                           &Token, &fileHandle);
        if(retVal < 0)
        {
            CLI_Write(" Error in creating the file \n\r");
            LOOP_FOREVER();
        }
    }

    /* Write "Old MacDonalds" child song (SIZE_64K/sizeof(oldMacDonald)) times
     * to get just below a 63KB size*/
    for (loop = 0; loop < (SIZE_64K/sizeof(oldMacDonald)); loop++)
    {
        retVal = sl_FsWrite(fileHandle, (_u32)(loop * sizeof(oldMacDonald)),
                                 (_u8 *)oldMacDonald, sizeof(oldMacDonald));
        if (retVal < 0)
        {
            CLI_Write(" Error in writing the file \n\r");

            retVal = sl_FsClose(fileHandle, 0, 0, 0);
            retVal = sl_FsDel((_u8 *)SL_FILE_NAME, Token);

            LOOP_FOREVER();
        }
    }

    /* Close the user file */
    retVal = sl_FsClose(fileHandle, 0, 0, 0);
    if (retVal < 0)
    {
        CLI_Write(" Error in closing the file \n\r");
        retVal = sl_FsDel((_u8 *)SL_FILE_NAME, Token);

        LOOP_FOREVER();
    }

    /* Open a user file for reading */
    retVal = sl_FsOpen((_u8 *)SL_FILE_NAME,
                       FS_MODE_OPEN_READ, &Token, &fileHandle);
    if (retVal < 0)
    {
        CLI_Write(" Error in opening the file for reading \n\r");
        retVal = sl_FsDel((_u8 *)SL_FILE_NAME, Token);

        LOOP_FOREVER();
    }

    /* Read the data and compare with the stored buffer */
    pal_Memset(&uNvmemBuf.g_Buf[0], '\0', BUF_SIZE);
    for (loop = 0; loop < (SIZE_64K/sizeof(oldMacDonald)); loop++)
    {
        retVal = sl_FsRead(fileHandle, (_u32)(loop * sizeof(oldMacDonald)),
                           uNvmemBuf.g_Buf, sizeof(oldMacDonald));
        if ( (retVal < 0) ||
             (retVal != sizeof(oldMacDonald)) )
        {
            CLI_Write(" Error in reading the file \n\r");
            retVal = sl_FsClose(fileHandle, 0, 0, 0);
            retVal = sl_FsDel((_u8 *)SL_FILE_NAME, Token);

            LOOP_FOREVER();
        }

        retVal = pal_Memcmp(oldMacDonald, uNvmemBuf.g_Buf, sizeof(oldMacDonald));
        if (retVal != 0)
        {
            CLI_Write(" Contents that were written and read back didn't match \n\r");
            LOOP_FOREVER();
        }
    }

    /* Close the user file */
    retVal = sl_FsClose(fileHandle, 0, 0, 0);
    if (retVal < 0)
    {
        CLI_Write(" Error in closing the file \n\r");
        retVal = sl_FsDel((_u8 *)SL_FILE_NAME, Token);

        LOOP_FOREVER();
    }

    /* Delete the user file */
    retVal = sl_FsDel((_u8 *)SL_FILE_NAME, Token);
    if(retVal < 0)
    {
        CLI_Write(" Error in deleting the file \n\r");
        LOOP_FOREVER();
    }

    CLI_Write(" Device successfully wrote/verified the data to/on serial-flash \n\r");

    /* Stop the CC3100 device */
    retVal = sl_Stop(SL_STOP_TIMEOUT);
    if(retVal < 0)
    {
        CLI_Write(" Error in stopping the device \n\r");
        LOOP_FOREVER();
    }

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
    \brief This function initializes the application variables

    \param[in]  None

    \return     0 on success, negative error-code on error
*/
static _i32 initializeAppVariables()
{
    g_Status = 0;
    pal_Memset(&uNvmemBuf, 0, sizeof(uNvmemBuf));

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
    CLI_Write(" File operations application - Version ");
    CLI_Write(APPLICATION_VERSION);
    CLI_Write("\n\r*******************************************************************************\n\r");
}
