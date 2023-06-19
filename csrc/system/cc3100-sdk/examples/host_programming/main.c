/*
 * main.c - servicepack flashing example
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
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "simplelink.h"
#include "cli_uart.h"
#include "bl_uart.h"
#include "host_programming_1.0.1.11-2.9.0.0_ucf.h"
#include "host_programming_1.0.1.11-2.9.0.0_ucf-signed.h"

#define VERSION_OFFSET      	(8)
#define UCF_OFFSET      		(20)
#define CHUNK_LEN			(1024)
#define LEN_128KB			(131072)

#define find_min(a,b) (((a) < (b)) ? (a) : (b))

char		  printBuffer[1024];

sflashSize_e	flashSize;

/*
 * ASYNCHRONOUS EVENT HANDLERS -- Start
 */

/*!
    \brief This function handles WLAN events

    \param[in]      pWlanEvents is the event passed to the handler

    \return         None

    \note

    \warning
*/
void SimpleLinkWlanEventHandler(SlWlanEvent_t *pWlanEvents)
{
    /* Unused in this application */
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
    /* Unused in this application */
}

/*!
    \brief This function handles callback for the HTTP server events

    \param[in]      pServerEvent - Contains the relevant event information
    \param[in]      pServerResponse - Should be filled by the user with the
                    relevant response information

    \return         None

    \note

    \warning
*/
void SimpleLinkHttpServerCallback(SlHttpServerEvent_t *pHttpEvent,
                                  SlHttpServerResponse_t *pHttpResponse)
{
    /* Unused in this application */
}

/*!

  	  \brief               This function is used for copying received stream to
                       	   32 bit in little endian format.
  	  \param[in]  			p          pointer to the stream
  	  \param[in]  			offset     offset in the stream

  	  \return               pointer to the new 32 bit

*/

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

void ReadVersion(void)
{
	SlVersionFull 		ver;
    	_u8 				pConfigOpt, pConfigLen;
	_i32         		retVal = 0;
	
	/* read the version and print it on terminal */
	pConfigOpt = SL_DEVICE_GENERAL_VERSION;
	pConfigLen = sizeof(SlVersionFull);
	retVal = sl_DevGet(SL_DEVICE_GENERAL_CONFIGURATION,&pConfigOpt,&pConfigLen,(_u8 *)(&ver));

	if(retVal < 0)
	{
		sprintf(printBuffer, "\r\nReading version failed. Error code: %d\r\n", (int)retVal);
		CLI_Write((_u8 *)printBuffer);

		turnLedOn(LED1);
	}

	if (ver.ChipFwAndPhyVersion.ChipId & 0x10)
		CLI_Write((_u8 *)"\r\nThis is a CC3200");
	else
		CLI_Write((_u8 *)"\r\nThis is a CC3100");

	if (ver.ChipFwAndPhyVersion.ChipId & 0x2)
		CLI_Write((_u8 *)"Z device\r\n");
	else
		CLI_Write((_u8 *)"R device\r\n");

	sprintf(printBuffer, "NWP %d.%d.%d.%d\n\rMAC 31.%d.%d.%d.%d\n\rPHY %d.%d.%d.%d\n\r\n\r", \
			(_u8)ver.NwpVersion[0],(_u8)ver.NwpVersion[1],(_u8)ver.NwpVersion[2],(_u8)ver.NwpVersion[3], \
				 (_u8)ver.ChipFwAndPhyVersion.FwVersion[0],(_u8)ver.ChipFwAndPhyVersion.FwVersion[1], \
				 (_u8)ver.ChipFwAndPhyVersion.FwVersion[2],(_u8)ver.ChipFwAndPhyVersion.FwVersion[3], \
				 ver.ChipFwAndPhyVersion.PhyVersion[0],(_u8)ver.ChipFwAndPhyVersion.PhyVersion[1], \
				 ver.ChipFwAndPhyVersion.PhyVersion[2],(_u8)ver.ChipFwAndPhyVersion.PhyVersion[3]);

	CLI_Write((_u8 *)printBuffer);
}


int main(void)
{
    _i32         		fileHandle = -1;

    _u32        		Token = 0;
    _i32         		retVal = 0;
    _u32         		remainingLen, movingOffset, chunkLen;

    /* Stop WDT */
    stopWDT();

    /* Initialize the system clock of MCU */
    initClk();

    /* Initialize the LED */
    initLEDs();

    /* Initialize the Application UART interface */
    CLI_Configure();

#ifdef FORMAT_ENABLE
    /* Initialize the bootloader UART interface */
    bl_uart_Configure();

    /* Configuring CC3100 nHib pin */
    P1SEL &= ~BIT6;
    P1OUT &= ~BIT6;
    P1DIR |= BIT6;

    CLI_Write((_u8 *)"\r\nSetting a break signal and resetting the device");
    /* setting a break signal prior to reset in order to enter boot loader mode */
    bl_uart_SetBreak();
	
    sl_DeviceDisable();

    Delay(100);

    sl_DeviceEnable();

    bl_read_ack();

    /* Initialize the bootloader UART interface - this would flush the RX lines*/
    bl_uart_Configure();

    flashSize = size1MB;
    CLI_Write((_u8 *)"\r\nFormatting the device to 1MB");
    bl_send_format(flashSize);

    bl_read_ack();

    CLI_Write((_u8 *)"\r\nDisconnecting from the device");
    bl_send_disconnect();

    bl_read_ack();
#endif	/* FORMAT_ENABLE */

    /* Initializing the CC3100 device */
    sl_Start(0, 0, 0);

    ReadVersion();

    /* create/open the servicepack file for 128KB with rollback, secured and public write */
	CLI_Write((_u8 *)"\r\nOpenning ServicePack file");
	retVal = sl_FsOpen("/sys/servicepack.ucf",
					   FS_MODE_OPEN_CREATE(LEN_128KB, _FS_FILE_OPEN_FLAG_SECURE|_FS_FILE_OPEN_FLAG_COMMIT|_FS_FILE_PUBLIC_WRITE),
					   &Token, &fileHandle);
	if(retVal < 0)
	{
		sprintf(printBuffer, "\r\nCannot open ServicePack file. Error code: %d", (int)retVal);
		CLI_Write((_u8 *)printBuffer);

		turnLedOn(LED1);
		return -1;
	}

	/* program the servicepack */
	CLI_Write((_u8 *)"\r\nProgramming ServicePack file");

	remainingLen = sizeof(servicePackImage);
	movingOffset = 0;
	chunkLen = (_u32)find_min(CHUNK_LEN, remainingLen);

	/* Flashing must be done in 1024 bytes chunks */
	do
	{
		retVal = sl_FsWrite(fileHandle, movingOffset, (_u8 *)&servicePackImage[movingOffset], chunkLen);
		if (retVal < 0)
		{
			sprintf(printBuffer, "\r\nCannot program ServicePack file. Error code: %d", (int)retVal);
			CLI_Write((_u8 *)printBuffer);

			turnLedOn(LED1);
			return -1;
		}

		remainingLen -= chunkLen;
		movingOffset += chunkLen;
		chunkLen = (_u32)find_min(CHUNK_LEN, remainingLen);
	}while (chunkLen > 0);

	/* close the servicepack file */
	CLI_Write((_u8 *)"\r\nClosing ServicePack file");
	retVal = sl_FsClose(fileHandle, 0, (_u8 *)servicePackImageSig, sizeof(servicePackImageSig));

	if (retVal < 0)
	{
		sprintf(printBuffer, "\r\nCannot close ServicePack file. Error code: %d", (int)retVal);
		CLI_Write((_u8 *)printBuffer);

		turnLedOn(LED1);
		return -1;
	}

	CLI_Write((_u8 *)"\r\nServicePack successfuly programmed\r\n\r\n");

	/* Turn On the Green LED */
	turnLedOn(LED2);

	CLI_Write((_u8 *)"Restarting CC3100... ");
	/* Stop the CC3100 device */
	sl_Stop(0xFF);

	/* Initializing the CC3100 device */
	sl_Start(0, 0, 0);

	ReadVersion();

	return 0;
}
