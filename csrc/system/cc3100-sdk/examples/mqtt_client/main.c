/*
 * mqtt_client.c - Sample application to connect to a MQTT broker and 
 * exercise functionalities like subscribe, publish etc.
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
 *
 * Application Name     -   MQTT Client
 * Application Overview -   This application acts as a MQTT client and connects
 *							to the IBM MQTT broker, simultaneously we can
 *							connect a web client from a web browser. Both
 *							clients can inter-communicate using appropriate
 *						    topic names.
 *
 * Application Details  -   http://processors.wiki.ti.com/index.php/CC31xx_MQTT_Client
 *                          docs\examples\mqtt_client.pdf
 *
 */

/*
 *
 *! \addtogroup mqtt_client
 *! @{
 *
 */

/* Standard includes */
#include <stdlib.h>
#include "simplelink.h"
#include "sl_common.h"

/* Free-RTOS includes */
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "portmacro.h"
#include "osi.h"

#include <stdio.h>
#include <stdlib.h>
#include "cli_uart.h"

#include "sl_mqtt_client.h"
#include "mqtt_config.h"

#define APPLICATION_VERSION "1.3.0"
#define SL_STOP_TIMEOUT     0xFF

#undef  LOOP_FOREVER
#define LOOP_FOREVER() \
            {\
                while(1) \
                { \
                    osi_Sleep(10); \
                } \
            }

#define PRINT_BUF_LEN    64
_i8 print_buf[PRINT_BUF_LEN];

/* Application specific status/error codes */
typedef enum
{
    DEVICE_NOT_IN_STATION_MODE = -0x7D0, /* Choosing this number to avoid overlap with host-driver's error codes */
    STATUS_CODE_MAX = -0xBB8
} e_AppStatusCodes;

/* Application specific data types */
typedef struct connection_config
{
    SlMqttClientCtxCfg_t broker_config;
    void *clt_ctx;
    _u8 *client_id;
    _u8 *usr_name;
    _u8 *usr_pwd;
    bool is_clean;
    _u32 keep_alive_time;
    SlMqttClientCbs_t CallBAcks;
    _i32 num_topics;
    char *topic[SUB_TOPIC_COUNT];
    _u8 qos[SUB_TOPIC_COUNT];
    SlMqttWill_t will_params;
    bool is_connected;
} connect_config;

typedef enum events
{
	NO_ACTION = -1,
	PUSH_BUTTON_PRESSED = 0,
	BROKER_DISCONNECTION = 2
} osi_messages;

/*
 * GLOBAL VARIABLES -- Start
 */
_u32  g_Status;
_u32  g_GatewayIP;
_u32  g_publishCount;
OsiMsgQ_t g_PBQueue; /*Message Queue*/
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
static void Mqtt_Recv(void *application_hndl, _const char  *topstr, _i32 top_len,
          _const void *payload, _i32 pay_len, bool dup,_u8 qos, bool retain);
static void sl_MqttEvt(void *application_hndl,_i32 evt, _const void *buf,
		  _u32 len);
static void sl_MqttDisconnect(void *application_hndl);
static _i32 Dummy(_const char *inBuff, ...);
static void MqttClient(void *pvParameters);
/*
 * STATIC FUNCTION DEFINITIONS -- End
 */

/* 
 * APPLICATION SPECIFIC DATA -- Start
 */
/* Connection configuration */
connect_config usr_connect_config[] =
{
    {
        {
            {
            	SL_MQTT_NETCONN_URL,
                SERVER_ADDRESS,
                PORT_NUMBER,
                0,
                0,
                0,
                NULL
            },
            SERVER_MODE,
            true,
        },
        NULL,
        CLIENT_ID, /* Must be unique */
        NULL,
        NULL,
        true,
        KEEP_ALIVE_TIMER,
        {Mqtt_Recv, sl_MqttEvt, sl_MqttDisconnect},
        SUB_TOPIC_COUNT,
        {SUB_TOPIC1, SUB_TOPIC2},
        {QOS2, QOS2},
        {WILL_TOPIC, WILL_MSG, WILL_QOS, WILL_RETAIN},
        false
    }
};

/* library configuration */
SlMqttClientLibCfg_t Mqtt_Client =
{
    LOOPBACK_PORT_NUMBER,
    TASK_PRIORITY,
    RCV_TIMEOUT,
    true,
    (_i32(*)(_const char *, ...))Dummy
};

/*Publishing topics and messages*/
_const char *pub_topic_1 = PUB_TOPIC_1;
_const char *pub_topic_2 = PUB_TOPIC_2;
_u8 *data_1={"Push button s1 is pressed: Data 1"};
_u8 *data_2={"Push button s1 is pressed: Data 2"};
void *g_application_hndl = (void*)usr_connect_config;
/* 
 * APPLICATION SPECIFIC DATA -- End
 */

/*Dummy print message*/
static _i32 Dummy(_const char *inBuff, ...)
{
    return 0;
}

/*!
    \brief Handles the button press on the MCU 
           and updates the queue with relevant action.
 	
    \param none
    
    \return none
 */
static void 
buttonHandler(void)
{
    osi_messages var;

    g_publishCount++;

    var = PUSH_BUTTON_PRESSED;

    osi_MsgQWrite(&g_PBQueue, &var, OSI_NO_WAIT);
}

/*!
    \brief Defines Mqtt_Pub_Message_Receive event handler.
           Client App needs to register this event handler with sl_ExtLib_mqtt_Init 
           API. Background receive task invokes this handler whenever MQTT Client 
           receives a Publish Message from the broker.
 	
    \param[out]     topstr => pointer to topic of the message
    \param[out]     top_len => topic length
    \param[out]     payload => pointer to payload
    \param[out]     pay_len => payload length
    \param[out]     retain => Tells whether its a Retained message or not
    \param[out]     dup => Tells whether its a duplicate message or not
    \param[out]     qos => Tells the Qos level
 
    \return none
 */
static void
Mqtt_Recv(void *application_hndl, _const char *topstr, _i32 top_len, _const void *payload,
                       _i32 pay_len, bool dup,_u8 qos, bool retain)
{
    _i8 *output_str=(_i8*)malloc(top_len+1);
    pal_Memset(output_str,'\0',top_len+1);
    pal_Strncpy(output_str, topstr, top_len);
    output_str[top_len]='\0';

    if(pal_Strncmp(output_str, SUB_TOPIC1, top_len) == 0)
    {
        toggleLed(LED1);
    }
    else if(pal_Strncmp(output_str, SUB_TOPIC2, top_len) == 0)
    {
        toggleLed(LED2);
    }

    CLI_Write((_u8 *)"\n\r Publish Message Received");
    
    pal_Memset(print_buf, 0x00, PRINT_BUF_LEN);
    sprintf((char*) print_buf, "\n\r Topic: %s", output_str);
    CLI_Write((_u8 *) print_buf);

    free(output_str);

    pal_Memset(print_buf, 0x00, PRINT_BUF_LEN);
    sprintf((char*) print_buf, " [Qos: %d] ", qos);
    CLI_Write((_u8 *) print_buf);

    if(retain)
      CLI_Write((_u8 *)" [Retained]");
    if(dup)
      CLI_Write((_u8 *)" [Duplicate]");
    
    output_str=(_i8*)malloc(pay_len+1);
    pal_Memset(output_str,'\0',pay_len+1);
    pal_Strncpy(output_str, payload, pay_len);
    output_str[pay_len]='\0';

    pal_Memset(print_buf, 0x00, PRINT_BUF_LEN);
    sprintf((char*) print_buf, " \n\r Data is: %s\n\r", (char*)output_str);
    CLI_Write((_u8 *) print_buf);

    free(output_str);
    
    return;
}

/*!
    \brief Defines sl_MqttEvt event handler.
           Client App needs to register this event handler with sl_ExtLib_mqtt_Init 
           API. Background receive task invokes this handler whenever MQTT Client 
           receives an ack(whenever user is in non-blocking mode) or encounters an error.
 	
    \param[out]     evt => Event that invokes the handler. Event can be of the
                           following types:
                           MQTT_ACK - Ack Received 
                           MQTT_ERROR - unknown error
                         
   
    \param[out]     buf => points to buffer
    \param[out]     len => buffer length
        
    \return none
 */
static void
sl_MqttEvt(void *application_hndl,_i32 evt, _const void *buf,_u32 len)
{
    _i32 i;
    
    switch(evt)
    {
        case SL_MQTT_CL_EVT_PUBACK:
        {
            CLI_Write((_u8 *)" PubAck:\n\r");
        }
        break;
    
        case SL_MQTT_CL_EVT_SUBACK:
        {
            CLI_Write((_u8 *)"\n\r Granted QoS Levels\n\r");
        
            for(i=0;i<len;i++)
            {
                pal_Memset(print_buf, 0x00, PRINT_BUF_LEN);
                sprintf((char*) print_buf, " QoS %d\n\r",((_u8*)buf)[i]);
                CLI_Write((_u8 *) print_buf);
            }
        }
        break;

        case SL_MQTT_CL_EVT_UNSUBACK:
        {
            CLI_Write((_u8 *)" UnSub Ack:\n\r");
        }
        break;
    
        default:
        {
            CLI_Write((_u8 *)" [MQTT EVENT] Unexpected event \n\r");
        }
        break;
    }
}

/*!

    \brief callback event in case of MQTT disconnection
    
    \param application_hndl is the handle for the disconnected connection
    
    \return none
    
*/
static void
sl_MqttDisconnect(void *application_hndl)
{
    connect_config *local_con_conf;
    osi_messages var = BROKER_DISCONNECTION;
    local_con_conf = application_hndl;
    sl_ExtLib_MqttClientUnsub(local_con_conf->clt_ctx, local_con_conf->topic,
                              SUB_TOPIC_COUNT);

    pal_Memset(print_buf, 0x00, PRINT_BUF_LEN);
    sprintf((char*) print_buf, " Disconnect from broker %s\r\n",
           (local_con_conf->broker_config).server_info.server_addr);
    CLI_Write((_u8 *) print_buf);

    local_con_conf->is_connected = false;
    sl_ExtLib_MqttClientCtxDelete(local_con_conf->clt_ctx);

    /*
     * Write message indicating publish message
     */
    osi_MsgQWrite(&g_PBQueue, &var, OSI_NO_WAIT);
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
        CLI_Write((_u8 *)" [WLAN EVENT] NULL Pointer Error \n\r");
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
        CLI_Write((_u8 *)" [NETAPP EVENT] NULL Pointer Error \n\r");
        return;
    }

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


/*
 * FreeRTOS User Hook Functions enabled in FreeRTOSConfig.h
 */

/*
    \brief Application defined hook (or callback) function - assert
    
    \param[in]  pcFile - Pointer to the File Name
    \param[in]  ulLine - Line Number
    
    \return none
*/
void vAssertCalled(_const _i8 *pcFile, _u32 ulLine)
{
    /* Handle Assert here */
    while(1)
    {
    }
}

/*
 
    \brief Application defined idle task hook
 
    \param  none
 
    \return none
 
 */
void vApplicationIdleHook(void)
{
    /* Handle Idle Hook for Profiling, Power Management etc */
}

/*
 
    \brief Application defined malloc failed hook
 
    \param  none
 
    \return none
 
 */
void vApplicationMallocFailedHook(void)
{
    /* Handle Memory Allocation Errors */
    while(1)
    {
    }
}

/*
 
  \brief Application defined stack overflow hook
 
  \param  none
 
  \return none
 
 */
void vApplicationStackOverflowHook( OsiTaskHandle *pxTask,
                                    _i8 *pcTaskName )
{
    /* Handle FreeRTOS Stack Overflow */
    while(1)
    {
    }
}

/*!
    \brief Publishes the message on a topic.
 	
    \param[in] clt_ctx - Client context
    \param[in] publish_topic - Topic on which the message will be published
    \param[in] publish_data - The message that will be published
    
    \return none
 */
static void 
publishData(void *clt_ctx, _const char *publish_topic, _u8 *publish_data)
{
    _i32 retVal = -1;

    /*
     * Send the publish message
     */
    retVal = sl_ExtLib_MqttClientSend((void*)clt_ctx,
            publish_topic ,publish_data, pal_Strlen(publish_data), QOS2, RETAIN);
    if(retVal < 0)
    {
        CLI_Write((_u8 *)"\n\r CC3100 failed to publish the message\n\r");
        return;
    }

    CLI_Write((_u8 *)"\n\r CC3100 Publishes the following message \n\r");

    pal_Memset(print_buf, 0x00, PRINT_BUF_LEN);
    sprintf((char*) print_buf, " Topic: %s\n\r", publish_topic);
    CLI_Write((_u8 *) print_buf);

    pal_Memset(print_buf, 0x00, PRINT_BUF_LEN);
    sprintf((char*) print_buf, " Data: %s\n\r", publish_data);
    CLI_Write((_u8 *) print_buf);
}

/*!

    \brief Task implementing MQTT client communication to other web client through
 	       a broker
 
    \param  none
 
    This function
        1. Initializes network driver and connects to the default AP
        2. Initializes the mqtt library and set up MQTT connection configurations
        3. set up the button events and their callbacks(for publishing)
 	    4. handles the callback signals
 
    \return none
 
 */
static void MqttClient(void *pvParameters)
{
    _i32 retVal = -1;
    _i32 iCount = 0;
    _i32 iNumBroker = 0;
    _i32 iConnBroker = 0;
    osi_messages RecvQue;
    
    connect_config *local_con_conf = (connect_config *)g_application_hndl;

    /* Configure LED */
    initLEDs();

    registerButtonIrqHandler(buttonHandler, NULL);

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
        if(DEVICE_NOT_IN_STATION_MODE == retVal)
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

    /* Connecting to WLAN AP */
    retVal = establishConnectionWithAP();
    if(retVal < 0)
    {
        CLI_Write((_u8 *)" Failed to establish connection w/ an AP \n\r");
        LOOP_FOREVER();
    }

    CLI_Write((_u8 *)" Connection established with AP\n\r");
    
    /* Initialize MQTT client lib */
    retVal = sl_ExtLib_MqttClientInit(&Mqtt_Client);
    if(retVal != 0)
    {
        /* lib initialization failed */
    	CLI_Write((_u8 *)"MQTT Client lib initialization failed\n\r");
    	LOOP_FOREVER();
    }
    
    /*
     * Connection to the broker
     */
    iNumBroker = sizeof(usr_connect_config)/sizeof(connect_config);
    if(iNumBroker > MAX_BROKER_CONN)
    {
        CLI_Write((_u8 *)"Num of brokers are more then max num of brokers\n\r");
        LOOP_FOREVER();
    }

    while(iCount < iNumBroker)
    {
        /* 
         * create client context
         */
        local_con_conf[iCount].clt_ctx =
		sl_ExtLib_MqttClientCtxCreate(&local_con_conf[iCount].broker_config,
									  &local_con_conf[iCount].CallBAcks,
									  &(local_con_conf[iCount]));

        /*
         * Set Client ID
         */
        sl_ExtLib_MqttClientSet((void*)local_con_conf[iCount].clt_ctx,
                                SL_MQTT_PARAM_CLIENT_ID,
                                local_con_conf[iCount].client_id,
                                pal_Strlen((local_con_conf[iCount].client_id)));

        /*
         * Set will Params
         */
        if(local_con_conf[iCount].will_params.will_topic != NULL)
        {
            sl_ExtLib_MqttClientSet((void*)local_con_conf[iCount].clt_ctx,
									SL_MQTT_PARAM_WILL_PARAM,
									&(local_con_conf[iCount].will_params),
									sizeof(SlMqttWill_t));
        }

        /*
         * Setting user name and password
         */
        if(local_con_conf[iCount].usr_name != NULL)
        {
            sl_ExtLib_MqttClientSet((void*)local_con_conf[iCount].clt_ctx,
								    SL_MQTT_PARAM_USER_NAME,
								    local_con_conf[iCount].usr_name,
								    pal_Strlen(local_con_conf[iCount].usr_name));

            if(local_con_conf[iCount].usr_pwd != NULL)
            {
                sl_ExtLib_MqttClientSet((void*)local_con_conf[iCount].clt_ctx,
								        SL_MQTT_PARAM_PASS_WORD,
								        local_con_conf[iCount].usr_pwd,
								        pal_Strlen(local_con_conf[iCount].usr_pwd));
            }
        }

        /*
         * Connecting to the broker
         */
        if((sl_ExtLib_MqttClientConnect((void*)local_con_conf[iCount].clt_ctx,
							            local_con_conf[iCount].is_clean,
                                        local_con_conf[iCount].keep_alive_time) & 0xFF) != 0)
        {
            pal_Memset(print_buf, 0x00, PRINT_BUF_LEN);
            sprintf((char*) print_buf, "\n\r Broker connect failed for conn no. %d \n\r", (_i16)iCount+1);
            CLI_Write((_u8 *) print_buf);
            
            /* 
             * Delete the context for this connection
             */
            sl_ExtLib_MqttClientCtxDelete(local_con_conf[iCount].clt_ctx);
            
            break;
        }
        else
        {
            pal_Memset(print_buf, 0x00, PRINT_BUF_LEN);
            sprintf((char*) print_buf, "\n\r Success: conn to Broker no. %d \n\r ", (_i16)iCount+1);
            CLI_Write((_u8 *) print_buf);

            local_con_conf[iCount].is_connected = true;
            iConnBroker++;
        }

        /*
         * Subscribe to topics
         */
        if(sl_ExtLib_MqttClientSub((void*)local_con_conf[iCount].clt_ctx,
								   local_con_conf[iCount].topic,
								   local_con_conf[iCount].qos, SUB_TOPIC_COUNT) < 0)
        {
            pal_Memset(print_buf, 0x00, PRINT_BUF_LEN);
            sprintf((char*) print_buf, "\n\r Subscription Error for conn no. %d \n\r", (_i16)iCount+1);
            CLI_Write((_u8 *) print_buf);
            
            CLI_Write((_u8 *)"Disconnecting from the broker\r\n");
            
            sl_ExtLib_MqttClientDisconnect(local_con_conf[iCount].clt_ctx);
			local_con_conf[iCount].is_connected = false;
            
            /* 
             * Delete the context for this connection
             */
            sl_ExtLib_MqttClientCtxDelete(local_con_conf[iCount].clt_ctx);
            iConnBroker--;
            break;
        }
        else
        {
            _i32 iSub;
            
            CLI_Write((_u8 *)"Client subscribed on following topics:\n\r");
            for(iSub = 0; iSub < local_con_conf[iCount].num_topics; iSub++)
            {
                pal_Memset(print_buf, 0x00, PRINT_BUF_LEN);
                sprintf((char*) print_buf, " %s\n\r", local_con_conf[iCount].topic[iSub]);
                CLI_Write((_u8 *) print_buf);
            }
        }
        iCount++;
    }

    if(iConnBroker < 1)
    {
        /*
         * No successful connection to broker
         */
        goto end;
    }

    iCount = 0;

    for(;;)
    {
        osi_MsgQRead( &g_PBQueue, &RecvQue, OSI_WAIT_FOREVER);
        
        if(PUSH_BUTTON_PRESSED == RecvQue)
        {
            if(g_publishCount % 2 == 0)
            {
                publishData((void*)local_con_conf[iCount].clt_ctx, pub_topic_1, data_1);
            }
            else
            {
                publishData((void*)local_con_conf[iCount].clt_ctx, pub_topic_2, data_2);
            }

            enableButtonIrq();
        }
        else if(BROKER_DISCONNECTION == RecvQue)
        {
            iConnBroker--;
            if(iConnBroker < 1)
            {
                /*
                 * Device not connected to any broker
                 */
                 goto end;
            }
        }
    }

end:
    /*
     * De-initializing the client library
     */
    sl_ExtLib_MqttClientExit();
    
    CLI_Write((_u8 *)"\n\r Exiting the Application\n\r");
    
    LOOP_FOREVER();
}


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

    /* Display Application Banner */
    displayBanner();

    /*
     * Start the SimpleLink Host
     */
    retVal = VStartSimpleLinkSpawnTask(SPAWN_TASK_PRIORITY);
    if(retVal < 0)
    {
        LOOP_FOREVER();
    }

    /*
     * Start the MQTT Client task
     */
    osi_MsgQCreate(&g_PBQueue,"PBQueue",sizeof(osi_messages),MAX_QUEUE_MSG);
    retVal = osi_TaskCreate(MqttClient,
                    		(_const _i8 *)"Mqtt Client App",
                    		OSI_STACK_SIZE, NULL, MQTT_APP_TASK_PRIORITY, NULL );

    if(retVal < 0)
    {
        LOOP_FOREVER();
    }

    /*
     * Start the task scheduler
     */
    osi_start();

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
            while(!IS_IP_ACQUIRED(g_Status));
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
        while(IS_CONNECTED(g_Status));
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

    secParams.Key = (_i8 *)PASSKEY;
    secParams.KeyLen = pal_Strlen(PASSKEY);
    secParams.Type = SEC_TYPE;

    retVal = sl_WlanConnect((_i8 *)SSID_NAME, pal_Strlen(SSID_NAME), 0, &secParams, 0);
    ASSERT_ON_ERROR(retVal);

    /* Wait */
    while((!IS_CONNECTED(g_Status)) || (!IS_IP_ACQUIRED(g_Status)));

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
    g_publishCount = 0;
    
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
    CLI_Write((_u8 *)" MQTT Client Application - Version ");
    CLI_Write((_u8 *) APPLICATION_VERSION);
    CLI_Write((_u8 *)"\n\r*******************************************************************************\n\r");
}
