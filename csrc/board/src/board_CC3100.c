/*
 * Board_CC310.c
 *
 *  Created on: 11 nov 2022
 *      Author: Francisco
 */

#include "board.h"
#include "simplelink.h"

#include "FreeRTOS.h"
#include "semphr.h"

#define BOARD_CC3100_FD		((int)INT32_C(1))

static SemaphoreHandle_t tx_semaphore = NULL;
static SemaphoreHandle_t rx_semaphore = NULL;

SPIDRV_HandleData_t cc3100_usart;

const SPIDRV_Init_t cc3100_usart_init_data =
{ .port = WIFI_SERIAL_PORT, .portLocation = _USART_ROUTE_LOCATION_LOC0,
		.bitRate = WIFI_SPI_BAUDRATE, .frameLength = 8, .dummyTxValue = 0xFF, .type = spidrvMaster,
		.bitOrder = spidrvBitOrderMsbFirst, .clockMode = spidrvClockMode0, .csControl =
				spidrvCsControlApplication, .slaveStartMode = spidrvSlaveStartImmediate,

};

struct transfer_status_s
{
	Ecode_t transferStatus; // status
	int itemsTransferred; // items_transferred
};

static volatile P_EVENT_HANDLER interrupt_handler_callback = NULL;
static void *interrupt_handler_pValue = NULL;

static void cc3100_interrupt_callback(uint8_t intNo)
{
	(void) intNo; // Validate interrupt ?

	if (NULL != interrupt_handler_callback)
	{
		interrupt_handler_callback(interrupt_handler_pValue);
	}
}

static void recieve_callback(struct SPIDRV_HandleData *handle, Ecode_t transferStatus,
		int itemsTransferred)
{
	(void) handle; // Validate handle ?
	struct transfer_status_s transfer_status_information =
	{ .transferStatus = transferStatus, .itemsTransferred = itemsTransferred };

	BaseType_t xHigherPriorityTaskWoken = pdFALSE;

	if (NULL != rx_semaphore)
	{
		(void)xQueueSendFromISR(rx_semaphore, &transfer_status_information, &xHigherPriorityTaskWoken);
	}

	portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

static void send_callback(struct SPIDRV_HandleData *handle, Ecode_t transferStatus,
		int itemsTransferred)
{
	(void) handle; // Validate handle ?
	struct transfer_status_s transfer_status_information =
	{ .transferStatus = transferStatus, .itemsTransferred = itemsTransferred };

	BaseType_t xHigherPriorityTaskWoken = pdFALSE;

	if (NULL != tx_semaphore)
	{
		(void)xQueueSendFromISR(tx_semaphore, &transfer_status_information, &xHigherPriorityTaskWoken);
	}

	portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

static void cc3100_spi_select(void)
{
	GPIO_PinOutClear(WIFI_CSN_PORT, WIFI_CSN_PIN);
}

static void cc3100_spi_deselect(void)
{
	GPIO_PinOutSet(WIFI_CSN_PORT, WIFI_CSN_PIN);
}

void Board_CC3100_Init(void)
{
	GPIO_PinModeSet(VDD_WIFI_EN_PORT, VDD_WIFI_EN_PIN, VDD_WIFI_EN_MODE, 1);
	GPIO_PinModeSet(WIFI_NRESET_PORT, WIFI_NRESET_PIN, WIFI_NRESET_MODE, 1);
	GPIO_PinModeSet(WIFI_CSN_PORT, WIFI_CSN_PIN, WIFI_CSN_MODE, 1);
	GPIO_PinModeSet(WIFI_INT_PORT, WIFI_INT_PIN, WIFI_INT_MODE, 0);
	GPIO_PinModeSet(WIFI_NHIB_PORT, WIFI_NHIB_PIN, WIFI_NHIB_MODE, 1);
	GPIO_PinModeSet(WIFI_SPI0_MISO_PORT, WIFI_SPI0_MISO_PIN,
	WIFI_SPI0_MISO_MODE, 0);
	GPIO_PinModeSet(WIFI_SPI0_MOSI_PORT, WIFI_SPI0_MOSI_PIN,
	WIFI_SPI0_MOSI_MODE, 0);
	GPIO_PinModeSet(WIFI_SPI0_SCK_PORT, WIFI_SPI0_SCK_PIN, WIFI_SPI0_SCK_MODE, 0);

	GPIOINT_CallbackRegister(WIFI_INT_PIN, cc3100_interrupt_callback);

	GPIO_ExtIntConfig(WIFI_INT_PORT,
	WIFI_INT_PIN,
	WIFI_INT_PIN,
	true,
	false,
	false);
}

void CC3100_DeviceEnablePreamble(void)
{
	GPIO_PinOutClear(VDD_WIFI_EN_PORT, VDD_WIFI_EN_PIN);
	GPIO_PinOutSet(WIFI_NHIB_PORT, WIFI_NHIB_PIN);
	BOARD_msDelay(WIFI_SUPPLY_SETTING_DELAY_MS);

	GPIO_PinOutClear(WIFI_NRESET_PORT, WIFI_NRESET_PIN);
	BOARD_msDelay(WIFI_MIN_RESET_DELAY_MS);
	GPIO_PinOutSet(WIFI_NRESET_PORT, WIFI_NRESET_PIN);
	BOARD_msDelay(WIFI_PWRON_HW_WAKEUP_DELAY_MS);
	BOARD_msDelay(WIFI_INIT_DELAY_MS);
}

void CC3100_DeviceEnable(void)
{
	if (tx_semaphore == NULL)
	{
		tx_semaphore = xQueueCreate(1, sizeof(struct transfer_status_s));
	}

	if (rx_semaphore == NULL)
	{
		rx_semaphore = xQueueCreate(1, sizeof(struct transfer_status_s));
	}

	GPIO_PinOutSet(WIFI_NHIB_PORT, WIFI_NHIB_PIN);
	GPIO_PinModeSet(WIFI_INT_PORT, WIFI_INT_PIN, WIFI_INT_MODE, 0);
	GPIO_ExtIntConfig(WIFI_INT_PORT,
	WIFI_INT_PIN,
	WIFI_INT_PIN,
	true,
	false,
	true);
}

void CC3100_DeviceDisable(void)
{
	GPIO_PinOutClear(WIFI_NHIB_PORT, WIFI_NHIB_PIN);
	GPIO_ExtIntConfig(WIFI_INT_PORT,
	WIFI_INT_PIN,
	WIFI_INT_PIN,
	true,
	false,
	false);
	BOARD_msDelay(1); // Very important (!)
//	GPIO_PinModeSet(VDD_WIFI_EN_PORT, VDD_WIFI_EN_PIN, gpioModeDisabled, 0);
//	GPIO_PinModeSet(WIFI_CSN_PORT, WIFI_CSN_PIN, gpioModeDisabled, 0);
	GPIO_PinModeSet(WIFI_INT_PORT, WIFI_INT_PIN, gpioModeDisabled, 0);
//	GPIO_PinModeSet(WIFI_NHIB_PORT, WIFI_NHIB_PIN, gpioModeDisabled, 0);
//	GPIO_PinModeSet(WIFI_NRESET_PORT, WIFI_NRESET_PIN, gpioModeDisabled, 0);
//	GPIO_PinModeSet(WIFI_SPI0_MISO_PORT, WIFI_SPI0_MISO_PIN, gpioModeDisabled, 0);
//	GPIO_PinModeSet(WIFI_SPI0_MOSI_PORT, WIFI_SPI0_MOSI_PIN, gpioModeDisabled, 0);
//	GPIO_PinModeSet(WIFI_SPI0_SCK_PORT, WIFI_SPI0_SCK_PIN, gpioModeDisabled, 0);

	if (NULL != tx_semaphore)
	{
		vQueueDelete(tx_semaphore);
		tx_semaphore = NULL;
	}
	if (NULL != rx_semaphore)
	{
		vQueueDelete(rx_semaphore);
		rx_semaphore = NULL;
	}
}

int CC3100_IfOpen(char *pIfName, unsigned long flags)
{
	(void) pIfName;
	(void) flags;
	int ret = -1;
	// Success: FD (positive integer)
	// Failure: -1
	if (strncmp(pIfName, CC3100_DEVICE_NAME, strlen(CC3100_DEVICE_NAME)) == 0)
	{
		if (ECODE_OK == SPIDRV_Init(&cc3100_usart, &cc3100_usart_init_data))
		{
			GPIO_PinOutSet(WIFI_NHIB_PORT, WIFI_NHIB_PIN); // Clear Hybernate
			BOARD_msDelay(50);
			cc3100_spi_deselect();
			ret = BOARD_CC3100_FD;
		}
	}

	return ret;
}

int CC3100_IfClose(Fd_t Fd)
{
	int ret = 0;

	if(BOARD_CC3100_FD != Fd)
	{
		ret = -1;
	}

	cc3100_spi_deselect();

	if (ECODE_EMDRV_SPIDRV_OK != SPIDRV_DeInit(&cc3100_usart))
	{
		ret = -1;
	}

	return ret;
}

int CC3100_IfRead(Fd_t Fd, uint8_t *pBuff, int Len)
{
	if((NULL == pBuff) || (Len <= 0) || (BOARD_CC3100_FD != Fd))
	{
		return -1;
	}

	int retVal = -1;
	struct transfer_status_s transfer_status =
	{ ECODE_EMDRV_SPIDRV_PARAM_ERROR, 0 };
	Ecode_t ecode = ECODE_EMDRV_SPIDRV_PARAM_ERROR;

	cc3100_spi_select();

	if (rx_semaphore != NULL) // Change to check if scheduler is active
	{
		ecode = SPIDRV_MReceive(&cc3100_usart, pBuff, Len, recieve_callback);
		if (ECODE_EMDRV_SPIDRV_OK == ecode)
		{
			if(pdTRUE == xQueueReceive(rx_semaphore, &transfer_status, 2*1000)) // Added rx timeout
			{
				if (ECODE_EMDRV_SPIDRV_OK == transfer_status.transferStatus)
				{
					retVal = transfer_status.itemsTransferred;
				}
			}
			else
			{
				retVal = -1;
			}
		}
	} else
	{
		ecode = SPIDRV_MReceiveB(&cc3100_usart, pBuff, Len);
		if (ECODE_EMDRV_SPIDRV_OK == ecode)
		{
			retVal = Len;
		}
	}

	cc3100_spi_deselect();
	return retVal;

}

int CC3100_IfWrite(Fd_t Fd, uint8_t *pBuff, int Len)
{
	if((NULL == pBuff) || (Len <= 0) || (BOARD_CC3100_FD != Fd))
	{
		return -1;
	}

	int retVal = -1;
	struct transfer_status_s transfer_status =
	{ ECODE_EMDRV_SPIDRV_PARAM_ERROR, 0 };
	Ecode_t ecode = ECODE_EMDRV_SPIDRV_PARAM_ERROR;

	cc3100_spi_select();

	if (rx_semaphore != NULL) // Change to check if scheduler is active
	{
		ecode = SPIDRV_MTransmit(&cc3100_usart, pBuff, Len, send_callback);
		if (ECODE_EMDRV_SPIDRV_OK == ecode)
		{
			(void) xQueueReceive(tx_semaphore, &transfer_status, portMAX_DELAY);
			if (ECODE_EMDRV_SPIDRV_OK == transfer_status.transferStatus)
			{
				retVal = transfer_status.itemsTransferred;
			} else
			{
				retVal = -1;
			}
		}
	} else
	{
		ecode = SPIDRV_MTransmitB(&cc3100_usart, pBuff, Len);
		if (ECODE_EMDRV_SPIDRV_OK == ecode)
		{
			retVal = Len;
		}
	}

	cc3100_spi_deselect();
	return retVal;
}

void CC3100_IfRegIntHdlr(P_EVENT_HANDLER interruptHdl, void *pValue)
{
	interrupt_handler_callback = interruptHdl;
	interrupt_handler_pValue = pValue;
}

void CC3100_MaskIntHdlr(void)
{
	;
}

void CC3100_UnmaskIntHdlr(void)
{
	;
}

extern void system_reset(void);

/* General Event Handler */
void CC3100_GeneralEvtHdlr(SlDeviceEvent_t *slGeneralEvent)
{
	if(NULL == slGeneralEvent)
	{
		return;
	}

	switch (slGeneralEvent->Event)
	{
		case SL_DEVICE_GENERAL_ERROR_EVENT:
			// General error event
			 printf("General Error: Status=%d, Sender=%d\n",
			               slGeneralEvent->EventData.deviceEvent.status,
			               slGeneralEvent->EventData.deviceEvent.sender);
			system_reset();
			break;
		case SL_DEVICE_ABORT_ERROR_EVENT:
	        printf("Abort Error: AbortType=%u, AbortData=%u\n",
	               slGeneralEvent->EventData.deviceReport.AbortType,
	               slGeneralEvent->EventData.deviceReport.AbortData);
			system_reset();
	        break;
		case SL_DEVICE_DRIVER_ASSERT_ERROR_EVENT:
			 printf("Driver Assert Error Occurred.\n");
            break;
		case SL_DEVICE_DRIVER_TIMEOUT_CMD_COMPLETE:
			printf("Driver Timeout: Command did not complete.\n");
			break;
		case SL_DEVICE_DRIVER_TIMEOUT_SYNC_PATTERN:
			printf("Driver Timeout: Sync Pattern not received.\n");
			break;
		case SL_DEVICE_DRIVER_TIMEOUT_ASYNC_EVENT:
			printf("Driver Timeout: Async event not received.\n");
			system_reset();
			break;
		default:
			break;
	}
}

void CC3100_HttpServerCallback(SlHttpServerEvent_t *pSlHttpServerEvent,
		SlHttpServerResponse_t *pSlHttpServerResponse)
{
	(void)pSlHttpServerEvent;
	(void)pSlHttpServerResponse;
}
void CC3100_SockEvtHdlr(SlSockEvent_t *pSlSockEvent)
{
	if(pSlSockEvent == NULL)
	{
		return;
	}

	switch (pSlSockEvent->Event)
	{
		case SL_SOCKET_TX_FAILED_EVENT:
			printf("tx failed\r\n");
			break;
		case SL_SOCKET_ASYNC_EVENT:
			printf("socket async event\r\n");
			break;
		default:
			break;
	}
}
