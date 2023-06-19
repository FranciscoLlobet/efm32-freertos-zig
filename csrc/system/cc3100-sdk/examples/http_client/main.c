/*
 * main.c - HTTP Client application
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
 * Application Name     -   HTTP Client
 * Application Overview -   This sample application demonstrates how to use
 *                          HTTP Client (In Minimum mode) API for HTTP based
 *                          application development.
 *                          This application explain user to how to:
 *                          1. Connect to an access point
 *                          2. Connect to a HTTP Server with and without proxy
 *                          3. Do POST, GET, PUT and DELETE
 *                          4. Parse JSON data using “Jasmine JSON Parser”
 *
 * Note: To use HTTP Client in minimum mode, user need to compile library (http_lib)
 * 			with HTTPCli_LIBTYPE_MIN option.
 *
 * 			HTTP Client (minimal) library supports synchronous mode, redirection
 * 			handling, chunked transfer encoding support, proxy support and TLS
 * 			support (for SimpleLink Only. TLS on other platforms are disabled)
 *
 * 			HTTP Client (Full) library supports all the features of the minimal
 * 			library + asynchronous mode and content handling support +
 * 			TLS support and requires RTOS support and not supportd on CC3100.
 *
 * Application Details  -   http://processors.wiki.ti.com/index.php/CC31xx_HTTP_Client
 *                          doc\examples\http_client.pdf
 */

#include "simplelink.h"
#include "sl_common.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


/* HTTP Client lib include */
#include <http/client/httpcli.h>

/* JSON Parser include */
#include "jsmn.h"

#define APPLICATION_VERSION "1.3.0"

#define SL_STOP_TIMEOUT        0xFF

/*
 * HTTP request parameters. These may change as depending upon the server
 */
#define POST_REQUEST_URI       "/post"
#define POST_DATA              "{\n\"name\":\"xyz\",\n\"address\":\n{\n\"plot#\":12,\n\"street\":\"abc\",\n\"city\":\"ijk\"\n},\n\"age\":30\n}"

#define DELETE_REQUEST_URI     "/delete"

#define PUT_REQUEST_URI        "/put"
#define PUT_DATA               "PUT request."

#define GET_REQUEST_URI        "/get"


#define HOST_NAME              "httpbin.org"  
#define HOST_PORT              80

#define PROXY_IP               0xBA5FB660
#define PROXY_PORT             <proxy_port>

#define READ_SIZE       1450
#define MAX_BUFF_SIZE   1460
#define SPACE           32


/* Application specific status/error codes */
typedef enum{
    DEVICE_NOT_IN_STATION_MODE = -0x7D0,        /* Choosing this number to avoid overlap with host-driver's error codes */
    INVALID_HEX_STRING = DEVICE_NOT_IN_STATION_MODE - 1,
    TCP_RECV_ERROR = INVALID_HEX_STRING - 1,
    TCP_SEND_ERROR = TCP_RECV_ERROR - 1,
    FILE_NOT_FOUND_ERROR = TCP_SEND_ERROR - 1,
    INVALID_SERVER_RESPONSE = FILE_NOT_FOUND_ERROR - 1,
    FORMAT_NOT_SUPPORTED = INVALID_SERVER_RESPONSE - 1,
    FILE_WRITE_ERROR = FORMAT_NOT_SUPPORTED - 1,
    INVALID_FILE = FILE_WRITE_ERROR - 1,

    STATUS_CODE_MAX = -0xBB8
}e_AppStatusCodes;

/*
 * GLOBAL VARIABLES -- Start
 */
_u32 g_Status;
_u32 g_DestinationIP;
_u32 g_BytesReceived; /* variable to store the file size */
_u8  g_buff[MAX_BUFF_SIZE+1];

_i32 g_SockID = 0;

/*
 * GLOBAL VARIABLES -- End
 */


/*
 * STATIC FUNCTION DEFINITIONS -- Start
 */
static _i32 establishConnectionWithAP();
static _i32 configureSimpleLinkToDefaultState();
static _i32 initializeAppVariables();
static void  displayBanner();

static _i32 ConnectToHTTPServer(HTTPCli_Handle httpClient);
static _i32 HTTPPostMethod(HTTPCli_Handle httpClient);
static _i32 HTTPDeleteMethod(HTTPCli_Handle httpClient);
static _i32 HTTPPutMethod(HTTPCli_Handle httpClient);
static _i32 HTTPGetMethod(HTTPCli_Handle httpClient);
static _i32 readResponse(HTTPCli_Handle httpClient);
static void FlushHTTPResponse(HTTPCli_Handle httpClient);
static _i32 ParseJSONData(_i8 *ptr);
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


/*
 * Application's entry point
 */
int main(int argc, char** argv)
{
    _i32            retVal = -1;
    HTTPCli_Struct     httpClient;

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
        {
            CLI_Write(" Failed to configure the device in its default state \n\r");
        }

        LOOP_FOREVER();
    }

    CLI_Write(" Device is configured in default state \n\r");

    /*
     * Initializing the CC3100 device
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

    /* Connecting to WLAN AP */
    retVal = establishConnectionWithAP();
    if(retVal < 0)
    {
        CLI_Write(" Failed to establish connection w/ an AP \n\r");
        LOOP_FOREVER();
    }

    CLI_Write(" Connection established w/ AP and IP is acquired \n\r");

    /* Connect to HTTP server */
    retVal = ConnectToHTTPServer(&httpClient);
    if(retVal < 0)
    {
      LOOP_FOREVER();
    }

    CLI_Write("\n\r");
    CLI_Write(" HTTP Post Test Begin:\n\r");
    retVal = HTTPPostMethod(&httpClient);
    if(retVal < 0)
    {
    	CLI_Write(" HTTP Post Test failed.\n\r");
    }
    CLI_Write(" HTTP Post Test Completed Successfully\n\r");

    CLI_Write("\n\r");
    CLI_Write(" HTTP Delete Test Begin:\n\r");
    retVal = HTTPDeleteMethod(&httpClient);

    if(retVal < 0)
    {
    	CLI_Write(" HTTP Delete Test failed.\n\r");
    }
    CLI_Write(" HTTP Delete Test Completed Successfully\n\r");


    CLI_Write("\n\r");
    CLI_Write(" HTTP Put Test Begin:\n\r");
    retVal = HTTPPutMethod(&httpClient);
    if(retVal < 0)
    {
    	CLI_Write(" HTTP Put Test failed.\n\r");
    }
    CLI_Write(" HTTP Put Test Completed Successfully\n\r");

    CLI_Write("\n\r");
    CLI_Write(" HTTP Get Test Begin:\n\r");
    retVal = HTTPGetMethod(&httpClient);
    if(retVal < 0)
    {
    	CLI_Write(" HTTP Get Test failed.\n\r");
    }
    CLI_Write(" HTTP Get Test Completed Successfully\n\r");
    CLI_Write("\n\r");

    /* Stop the CC3100 device */
    retVal = sl_Stop(SL_STOP_TIMEOUT);
    if(retVal < 0)
    {
        LOOP_FOREVER();
    }
    return SUCCESS;
}


/*!
    \brief This function demonstarte the HTTP POST method

    \param[in]      httpClient - HTTP Client object

    \return         0 on success else -ve

    \note

    \warning
*/
static _i32 HTTPPostMethod(HTTPCli_Handle httpClient)
{
    _i32            retVal = 0;
    _i8             tmpBuf[4];
    bool            moreFlags = 1;
    bool            lastFlag = 1;
    const HTTPCli_Field   fields[4] = {
                                    {HTTPCli_FIELD_NAME_HOST, HOST_NAME},
                                    {HTTPCli_FIELD_NAME_ACCEPT, "*/*"},
                                    {HTTPCli_FIELD_NAME_CONTENT_TYPE, "application/json"},
                                    {NULL, NULL}
                                };

    /* Set request header fields to be send for HTTP request. */
    HTTPCli_setRequestFields(httpClient, fields);

    /* Send POST method request. */
    /* Here we are setting moreFlags = 1 as there are some more header fields need to send
       other than setted in previous call HTTPCli_setRequestFields() at later stage.
       Please refer HTTP Library API documentaion @ref HTTPCli_sendRequest for more information.
    */
    moreFlags = 1;
    retVal = HTTPCli_sendRequest(httpClient, HTTPCli_METHOD_POST, POST_REQUEST_URI, moreFlags);
    if(retVal < 0)
    {
        CLI_Write(" Failed to send HTTP POST request header.\n\r");
        return retVal;
    }

    sprintf((char *)tmpBuf, "%d", (sizeof(POST_DATA)-1));

    /* Here we are setting lastFlag = 1 as it is last header field.
       Please refer HTTP Library API documentaion @ref HTTPCli_sendField for more information.
    */
    lastFlag = 1;
    retVal = HTTPCli_sendField(httpClient, HTTPCli_FIELD_NAME_CONTENT_LENGTH, (const char *)tmpBuf, lastFlag);
    if(retVal < 0)
    {
        CLI_Write(" Failed to send HTTP POST request header.\n\r");
        return retVal;
    }


    /* Send POST data/body */
    retVal = HTTPCli_sendRequestBody(httpClient, POST_DATA, (sizeof(POST_DATA)-1));
    if(retVal < 0)
    {
        CLI_Write(" Failed to send HTTP POST request body.\n\r");
        return retVal;
    }


    retVal = readResponse(httpClient);

    return retVal;
}

/*!
    \brief This function demonstarte the HTTP DLETE method

    \param[in]      httpClient - HTTP Client object

    \return         0 on success else -ve

    \note

    \warning
*/
static _i32 HTTPDeleteMethod(HTTPCli_Handle httpClient)
{
    _i32            retVal = 0;
    bool            moreFlags;
    const HTTPCli_Field   fields[3] = {
                                    {HTTPCli_FIELD_NAME_HOST, HOST_NAME},
                                    {HTTPCli_FIELD_NAME_ACCEPT, "*/*"},
                                    {NULL, NULL}
                                };


    /* Set request header fields to be send for HTTP request. */
    HTTPCli_setRequestFields(httpClient, fields);

    /* Send DELETE method request. */
    /* Here we are setting moreFlags = 0 as there are no more header fields needs to be send
       at later stage. Please refer HTTP Library API documentaion @ref HTTPCli_sendRequest
       for more information.
    */
    moreFlags = 0;
    retVal = HTTPCli_sendRequest(httpClient, HTTPCli_METHOD_DELETE, DELETE_REQUEST_URI, moreFlags);
    if(retVal < 0)
    {
        CLI_Write(" Failed to send HTTP DELETE request header.\n\r");
        return retVal;
    }

    retVal = readResponse(httpClient);

    return retVal;
}

/*!
    \brief This function demonstarte the HTTP PUT method

    \param[in]      httpClient - HTTP Client object

    \return         0 on success else -ve

    \note

    \warning
*/
static _i32 HTTPPutMethod(HTTPCli_Handle httpClient)
{
    _i32            retVal = 0;
    _i8             tmpBuf[4];
    bool            moreFlags = 1;
    bool            lastFlag = 1;
    const HTTPCli_Field   fields[4] = {
                                    {HTTPCli_FIELD_NAME_HOST, HOST_NAME},
                                    {HTTPCli_FIELD_NAME_ACCEPT, "*/*"},
                                    {NULL, NULL}
                                };


    /* Set request header fields to be send for HTTP request. */
    HTTPCli_setRequestFields(httpClient, fields);

    /* Send PUT method request. */
    /* Here we are setting moreFlags = 1 as there are some more header fields needs to be send
       other than setted in previous call HTTPCli_setRequestFields() at later stage.
       Please refer HTTP Library API documentaion @ref HTTPCli_sendRequest for more information.
    */
    moreFlags = 1;
    retVal = HTTPCli_sendRequest(httpClient, HTTPCli_METHOD_PUT, PUT_REQUEST_URI, moreFlags);
    if(retVal < 0)
    {
        CLI_Write(" Failed to send HTTP PUT request header.\n\r");
        return retVal;
    }

    sprintf((char *)tmpBuf, "%d", (sizeof(PUT_DATA)-1));

    /* Here we are setting lastFlag = 1 as it is last header field.
       Please refer HTTP Library API documentaion @ref HTTPCli_sendField for more information.
    */
    lastFlag = 1;
    retVal = HTTPCli_sendField(httpClient, HTTPCli_FIELD_NAME_CONTENT_LENGTH, (char *)tmpBuf, lastFlag);
    if(retVal < 0)
    {
        CLI_Write(" Failed to send HTTP PUT request header.\n\r");
        return retVal;
    }

    /* Send PUT data/body */
    retVal = HTTPCli_sendRequestBody(httpClient, PUT_DATA, (sizeof(PUT_DATA)-1));
    if(retVal < 0)
    {
        CLI_Write(" Failed to send HTTP PUT request body.\n\r");
        return retVal;
    }

    retVal = readResponse(httpClient);

    return retVal;
}

/*!
    \brief This function demonstarte the HTTP GET method

    \param[in]      httpClient - HTTP Client object

    \return         0 on success else -ve

    \note

    \warning
*/
static _i32 HTTPGetMethod(HTTPCli_Handle httpClient)
{
    _i32             retVal = 0;
    bool             moreFlags;
    const HTTPCli_Field    fields[4] = {
                                    {HTTPCli_FIELD_NAME_HOST, HOST_NAME},
                                    {HTTPCli_FIELD_NAME_ACCEPT, "*/*"},
                                    {HTTPCli_FIELD_NAME_CONTENT_LENGTH, "0"},
                                    {NULL, NULL}
                                };


    /* Set request header fields to be send for HTTP request. */
    HTTPCli_setRequestFields(httpClient, fields);

    /* Send GET method request. */
    /* Here we are setting moreFlags = 0 as there are no more header fields need to send
       at later stage. Please refer HTTP Library API documentaion @ HTTPCli_sendRequest
       for more information.
    */
    moreFlags = 0;
    retVal = HTTPCli_sendRequest(httpClient, HTTPCli_METHOD_GET, GET_REQUEST_URI, moreFlags);
    if(retVal < 0)
    {
        CLI_Write(" Failed to send HTTP GET request.\n\r");
        return retVal;
    }


    retVal = readResponse(httpClient);

    return retVal;
}


/*!
    \brief This function read respose from server and dump on console

    \param[in]      httpClient - HTTP Client object

    \return         0 on success else -ve

    \note

    \warning
*/
static _i32 readResponse(HTTPCli_Handle httpClient)
{
	_i32            retVal = 0;
	_i32            bytesRead = 0;
	_i32            id = 0;
	_u32            len = 0;
	_i32            json = 0;
	_i8             *dataBuffer=NULL;
	bool            moreFlags = 1;
	const _i8       *ids[4] = {
                                HTTPCli_FIELD_NAME_CONTENT_LENGTH,
			                    HTTPCli_FIELD_NAME_CONNECTION,
			                    HTTPCli_FIELD_NAME_CONTENT_TYPE,
			                    NULL
	                          };

	/* Read HTTP POST request status code */
	retVal = HTTPCli_getResponseStatus(httpClient);
	if(retVal > 0)
	{
		switch(retVal)
		{
		case 200:
		{
			CLI_Write(" HTTP Status 200\n\r");
			/*
                 Set response header fields to filter response headers. All
                  other than set by this call we be skipped by library.
			 */
			HTTPCli_setResponseFields(httpClient, (const char **)ids);

			/* Read filter response header and take appropriate action. */
			/* Note:
                    1. id will be same as index of fileds in filter array setted
                    in previous HTTPCli_setResponseFields() call.

                    2. moreFlags will be set to 1 by HTTPCli_getResponseField(), if  field
                    value could not be completely read. A subsequent call to
                    HTTPCli_getResponseField() will read remaining field value and will
                    return HTTPCli_FIELD_ID_DUMMY. Please refer HTTP Client Libary API
                    documenation @ref HTTPCli_getResponseField for more information.
			 */
			while((id = HTTPCli_getResponseField(httpClient, (char *)g_buff, sizeof(g_buff), &moreFlags))
					!= HTTPCli_FIELD_ID_END)
			{

				switch(id)
				{
				case 0: /* HTTPCli_FIELD_NAME_CONTENT_LENGTH */
				{
					len = strtoul((char *)g_buff, NULL, 0);
				}
				break;
				case 1: /* HTTPCli_FIELD_NAME_CONNECTION */
				{
				}
				break;
				case 2: /* HTTPCli_FIELD_NAME_CONTENT_TYPE */
				{
					if(!strncmp((const char *)g_buff, "application/json",
							sizeof("application/json")))
					{
						json = 1;
					}
					else
					{
						/* Note:
                                Developers are advised to use appropriate
                                content handler. In this example all content
                                type other than json are treated as plain text.
						 */
						json = 0;
					}
					CLI_Write(" ");
					CLI_Write(HTTPCli_FIELD_NAME_CONTENT_TYPE);
					CLI_Write(" : ");
					CLI_Write("application/json\n\r");
				}
				break;
				default:
				{
					CLI_Write(" Wrong filter id\n\r");
					retVal = -1;
					goto end;
				}
				}
			}
			bytesRead = 0;
			if(len > sizeof(g_buff))
			{
				dataBuffer = (_i8 *) malloc(len);
				if(dataBuffer)
				{
					CLI_Write(" Failed to allocate memory\n\r");
					retVal = -1;
					goto end;
				}
			}
			else
			{
				dataBuffer = (_i8 *)g_buff;
			}

			/* Read response data/body */
			/* Note:
                    moreFlag will be set to 1 by HTTPCli_readResponseBody() call, if more
		            data is available Or in other words content length > length of buffer.
		            The remaining data will be read in subsequent call to HTTPCli_readResponseBody().
		            Please refer HTTP Client Libary API documenation @ref HTTPCli_readResponseBody
		            for more information
			 	 	
			 */
			bytesRead = HTTPCli_readResponseBody(httpClient, (char *)dataBuffer, len, &moreFlags);
			if(bytesRead < 0)
			{
				CLI_Write(" Failed to received response body\n\r");
				retVal = bytesRead;
				goto end;
			}
			else if( bytesRead < len || moreFlags)
			{
				CLI_Write(" Mismatch in content length and received data length\n\r");
				goto end;
			}
			dataBuffer[bytesRead] = '\0';

			if(json)
			{
				/* Parse JSON data */
				retVal = ParseJSONData(dataBuffer);
				if(retVal < 0)
				{
					goto end;
				}
			}
			else
			{
				/* treating data as a plain text */
			}

		}
		break;

		case 404:
			CLI_Write(" File not found. \r\n");
			/* Handle response body as per requirement.
                  Note:
                    Developers are advised to take appopriate action for HTTP
                    return status code else flush the response body.
                    In this example we are flushing response body in default
                    case for all other than 200 HTTP Status code.
			 */
		default:
			/* Note:
              Need to flush received buffer explicitly as library will not do
              for next request.Apllication is responsible for reading all the
              data.
			 */
			FlushHTTPResponse(httpClient);
			break;
		}
	}
	else
	{
		CLI_Write(" Failed to receive data from server.\r\n");
		goto end;
	}

	retVal = 0;

end:
    if(len > sizeof(g_buff) && (dataBuffer != NULL))
	{
	    free(dataBuffer);
    }
    return retVal;
}


/*!
    \brief This function establish a HTTP connection

    \param[in]      httpClient - HTTP Client object

    \return         0 on success else -ve

    \note

    \warning
*/
static _i32 ConnectToHTTPServer(HTTPCli_Handle httpClient)
{
    _i32                retVal = -1;
    struct sockaddr_in     addr;

#ifdef USE_PROXY
    struct sockaddr_in     paddr;

    paddr.sin_family = AF_INET;
    paddr.sin_port = htons(PROXY_PORT);
    paddr.sin_addr.s_addr = sl_Htonl(PROXY_IP);
    HTTPCli_setProxy((struct sockaddr *)&paddr);
#endif

     /* Resolve HOST NAME/IP */
    retVal = sl_NetAppDnsGetHostByName(HOST_NAME, pal_Strlen(HOST_NAME),
                                       &g_DestinationIP, SL_AF_INET);
    if(retVal < 0)
    {
        CLI_Write(" Device couldn't get the IP for the host-name\r\n");
        ASSERT_ON_ERROR(retVal);
    }

    /* Set up the input parameters for HTTP Connection */
    addr.sin_family = AF_INET;
    addr.sin_port = htons(HOST_PORT);
    addr.sin_addr.s_addr = sl_Htonl(g_DestinationIP);

    /* HTTPCli open call: handle, address params only */
    HTTPCli_construct(httpClient);
    retVal = HTTPCli_connect(httpClient, (struct sockaddr *)&addr, 0, NULL);
    if (retVal < 0)
    {
        CLI_Write("Connection to server failed\n\r");
        ASSERT_ON_ERROR(retVal);
    }

    CLI_Write(" Successfully connected to the server \r\n");
    return SUCCESS;
}

/*!
    \brief This function flush HTTP response

    \param[in]      httpClient - HTTP Client object

    \return         None

    \note

    \warning
*/
static void FlushHTTPResponse(HTTPCli_Handle httpClient)
{
    const _i8       *ids[2] = {
                               HTTPCli_FIELD_NAME_CONNECTION,
                               NULL
                              };
    _i8             buf[128];
    _i32            id;
    _i32            len = 1;
    bool            moreFlag = 0;
    _i8             **prevRespFilelds = NULL;

    /* Store previosly store array if any */
    prevRespFilelds = (_i8 **)HTTPCli_setResponseFields(httpClient, (const char **)ids);

    /* Read response headers */
    while ((id = HTTPCli_getResponseField(httpClient, (char *)buf, sizeof(buf), &moreFlag))
            != HTTPCli_FIELD_ID_END)
    {

        if(id == 0)
        {
            if(!strncmp((const char *)buf, "close", sizeof("close")))
            {
                CLI_Write(" Connection terminated by server\n\r");
            }
        }
    }

    /* Restore previosuly store array if any */
    HTTPCli_setResponseFields(httpClient, (const char **)prevRespFilelds);

    while(1)
    {
        /* Read response data/body */
        /* Note:
                moreFlag will be set to 1 by HTTPCli_readResponseBody() call, if more
                data is available Or in other words content length > length of buffer.
                The remaining data will be read in subsequent call to HTTPCli_readResponseBody().
                Please refer HTTP Client Libary API documenation @ref HTTPCli_readResponseBody
                for more information.
        */
        HTTPCli_readResponseBody(httpClient, (char *)buf, sizeof(buf) - 1, &moreFlag);
        CLI_Write((_u8 *)buf);
        CLI_Write("\r\n");

        if ((len - 2) >= 0 && buf[len - 2] == '\r' && buf [len - 1] == '\n')
        {
        }

        if(!moreFlag)
        {
            /* There no more data. break the loop. */
            break;
        }
    }
}


/*!
    \brief This function parse JSON data

    \param[in]      ptr - Pointer to JSON data

    \return         0 on success else -ve

    \note

    \warning
*/
static _i32 ParseJSONData(_i8 *ptr)
{
	_i32			retVal = 0;
    _i32            noOfToken;
    jsmn_parser     parser;
    jsmntok_t       *tokenList;
    _i8             printBuffer[4];

    /* Initialize JSON PArser */
    jsmn_init(&parser);

    /* Get number of JSON token in stream as we we dont know how many tokens need to pass */
    noOfToken = jsmn_parse(&parser, (const char *)ptr, strlen((const char *)ptr), NULL, 10);
    if(noOfToken <= 0)
    {
    	CLI_Write(" Failed to initialize JSON parser\n\r");
    	return -1;

    }

    /* Allocate memory to store token */
    tokenList = (jsmntok_t *) malloc(noOfToken*sizeof(jsmntok_t));
    if(tokenList == NULL)
    {
        CLI_Write(" Failed to allocate memory\n\r");
        return -1;
    }

    /* Initialize JSON Parser again */
    jsmn_init(&parser);
    noOfToken = jsmn_parse(&parser, (const char *)ptr, strlen((const char *)ptr), tokenList, noOfToken);
    if(noOfToken < 0)
    {
    	CLI_Write(" Failed to parse JSON tokens\n\r");
    	retVal = noOfToken;
    }
    else
    {
    	CLI_Write(" Successfully parsed ");
    	sprintf((char *)printBuffer, "%ld", noOfToken);
    	CLI_Write((_u8 *)printBuffer);
    	CLI_Write(" JSON tokens\n\r");
    }

    free(tokenList);

    return retVal;
}


/*!
    \brief Obtain the file from the server

    This function requests the file from the server and save it on serial flash.
    To request a different file for different user needs to modify the
    PREFIX_BUFFER and POST_BUFFER macros.

    \param[in]      None

    \return         0 for success and negative for error

*/
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
    g_SockID = 0;
    g_DestinationIP = 0;
    g_BytesReceived = 0;
    pal_Memset(g_buff, 0, sizeof(g_buff));

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
    CLI_Write(" HTTP Client - Version ");
    CLI_Write(APPLICATION_VERSION);
    CLI_Write("\n\r*******************************************************************************\n\r");
}

