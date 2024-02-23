/*
 * sd_card.c
 *
 *  Created on: 9 nov 2022
 *      Author: Francisco
 */
#include "board.h"
#include "board_sd_card.h"

#include "ff.h"
#include "diskio.h"

/* OS resources */
#include "FreeRTOS.h"
#include "semphr.h"

/* SPI DRV Handle */
static SPIDRV_HandleData_t sd_card_usart;

/* Detect SD Card */
static void card_detect_callback(uint8_t intNo);

/* TX-RX Semaphores */
static SemaphoreHandle_t tx_semaphore = NULL;
static SemaphoreHandle_t rx_semaphore = NULL;


uint32_t BOARD_SD_CARD_IsInserted(void)
{
	return (0 == GPIO_PinInGet(SD_DETECT_PORT,SD_DETECT_PIN)) ? (uint32_t)UINT32_C(1) : (uint32_t)UINT32_C(0);
}

static void card_detect_callback(uint8_t intNo)
{
	(void)intNo;
	if(UINT32_C(1) == BOARD_SD_CARD_IsInserted())
	{
		// send event: SD card removed
	}
	else
	{
		// send event: SD card inserted
	}
}

void BOARD_SD_Card_Init(void)
{
    GPIO_PinModeSet(SD_CARD_CS_PORT, SD_CARD_CS_PIN, SD_CARD_CS_MODE, 1);
    GPIO_PinModeSet(SD_CARD_LS_PORT, SD_CARD_LS_PIN, SD_CARD_LS_MODE, 1);
    GPIO_PinModeSet(SD_DETECT_PORT, SD_DETECT_PIN, SD_DETECT_MODE, 1);
    GPIO_PinModeSet(SD_CARD_SPI1_MISO_PORT, SD_CARD_SPI1_MISO_PIN, SD_CARD_SPI1_MISO_MODE, 0);
    GPIO_PinModeSet(SD_CARD_SPI1_MOSI_PORT, SD_CARD_SPI1_MOSI_PIN, SD_CARD_SPI1_MOSI_MODE, 0);
    GPIO_PinModeSet(SD_CARD_SPI1_SCK_PORT, SD_CARD_SPI1_SCK_PIN, SD_CARD_SPI1_SCK_MODE, 0);

    GPIOINT_CallbackRegister(SD_DETECT_PIN, card_detect_callback);

    GPIO_ExtIntConfig(SD_DETECT_PORT,
    				  SD_DETECT_PIN,
					  SD_DETECT_PIN,
                      true,
                      true,
                      false);
}

void BOARD_SD_Card_Enable(void)
{
	if(tx_semaphore == NULL)
	{
		tx_semaphore = xQueueCreate( 1, sizeof(Ecode_t) );
	}

	if(rx_semaphore == NULL)
	{
		rx_semaphore = xQueueCreate( 1, sizeof(Ecode_t) );
	}

    GPIOINT_CallbackRegister(SD_DETECT_PIN, card_detect_callback);

    GPIO_ExtIntConfig(SD_DETECT_PORT,
    				  SD_DETECT_PIN,
					  SD_DETECT_PIN,
                      true,
                      true,
                      true);

    (void)BOARD_SD_CARD_IsInserted();

		/* SPI DRV Init */
	SPIDRV_Init_t sd_card_init_data = {
			  .port = BOARD_SD_CARD_USART,
			  .portLocation = _USART_ROUTE_LOCATION_LOC1,
			  .bitRate = BOARD_SD_CARD_WAKEUP_BITRATE,
			  .frameLength = 8,
			  .dummyTxValue = 0xFF,
			  .type = spidrvMaster,
			  .bitOrder = spidrvBitOrderMsbFirst,
			  .clockMode = spidrvClockMode0,
			  .csControl = spidrvCsControlApplication,
			  .slaveStartMode = spidrvSlaveStartImmediate,
	};

	(void)SPIDRV_Init(&sd_card_usart, &sd_card_init_data);
}

void BOARD_SD_Card_Disable(void)
{
	(void)SPIDRV_DeInit(&sd_card_usart);

	GPIO_PinModeSet(SD_CARD_CS_PORT, SD_CARD_CS_PIN, gpioModeDisabled, 1);
	GPIO_PinModeSet(SD_CARD_LS_PORT, SD_CARD_CS_PIN, gpioModeDisabled, 1);
	GPIO_PinModeSet(SD_DETECT_PORT, SD_DETECT_PIN, SD_DETECT_MODE, 1);
	GPIO_PinModeSet(SD_CARD_SPI1_MISO_PORT, SD_CARD_SPI1_MISO_PIN, gpioModeDisabled, 0);
	GPIO_PinModeSet(SD_CARD_SPI1_MOSI_PORT, SD_CARD_SPI1_MOSI_PIN, gpioModeDisabled, 0);
	GPIO_PinModeSet(SD_CARD_SPI1_SCK_PORT, SD_CARD_SPI1_SCK_PIN, gpioModeDisabled,0);

    GPIO_ExtIntConfig(SD_DETECT_PORT,
    				  SD_DETECT_PIN,
					  SD_DETECT_PIN,
                      false,
                      false,
                      false);
}



void BOARD_SD_CARD_Deselect(void)
{
	GPIO_PinOutSet(SD_CARD_CS_PORT, SD_CARD_CS_PIN);
}

void BOARD_SD_CARD_Select(void)
{
	GPIO_PinOutClear(SD_CARD_CS_PORT, SD_CARD_CS_PIN);
}


void BOARD_SD_CARD_SetFastBaudrate(void)
{
	SPIDRV_SetBitrate(&sd_card_usart, BOARD_SD_CARD_BITRATE);
}

void BOARD_SD_CARD_SetSlowBaudrate(void)
{
	SPIDRV_SetBitrate(&sd_card_usart, BOARD_SD_CARD_WAKEUP_BITRATE);
}


/* recieve callback for async reception */
static void recieve_callback(struct SPIDRV_HandleData *handle,
                                  Ecode_t transferStatus,
                                  int itemsTransferred)
{
	(void)handle;
	(void)itemsTransferred;

	BaseType_t xHigherPriorityTaskWoken = pdFALSE;

	if(NULL != rx_semaphore) xQueueSendFromISR( rx_semaphore, &transferStatus, &xHigherPriorityTaskWoken );

	portYIELD_FROM_ISR( xHigherPriorityTaskWoken );
}

/* tx callback for async tx */
static void send_callback(struct SPIDRV_HandleData *handle,
                                  Ecode_t transferStatus,
                                  int itemsTransferred)
{
	(void)handle;
	(void)itemsTransferred;

	BaseType_t xHigherPriorityTaskWoken = pdFALSE;

	if(NULL != tx_semaphore) xQueueSendFromISR( tx_semaphore, &transferStatus, &xHigherPriorityTaskWoken );

	portYIELD_FROM_ISR( xHigherPriorityTaskWoken );
}

/* OS enabled rx callback */
uint32_t BOARD_SD_CARD_Recieve(void * buffer, int count)
{
	Ecode_t ecode = ECODE_EMDRV_SPIDRV_PARAM_ERROR;
	if(rx_semaphore != NULL) // Change to check if scheduler is active
	{
		ecode = SPIDRV_MReceive(&sd_card_usart, buffer, count, recieve_callback);
		if(ECODE_EMDRV_SPIDRV_OK == ecode)
		{
			(void)xQueueReceive( rx_semaphore, &ecode, portMAX_DELAY);
		}
	}
	else
	{
		// Perform blocking reception
		ecode = SPIDRV_MReceiveB(&sd_card_usart, buffer, count);
	}

	return (uint32_t)ecode;
}

/* OS enabled tx callback */
uint32_t BOARD_SD_CARD_Send(const void * buffer, int count)
{
	Ecode_t ecode = ECODE_EMDRV_SPIDRV_PARAM_ERROR;
	if(tx_semaphore != NULL) // Change to check if scheduler is active
	{
		ecode = SPIDRV_MTransmit(&sd_card_usart, buffer, count, send_callback);
		if(ECODE_EMDRV_SPIDRV_OK == ecode)
		{
			(void)xQueueReceive( tx_semaphore, &ecode, portMAX_DELAY);
		}
	}
	else
	{
		// Perform blocking transmit
		ecode = SPIDRV_MTransmitB(&sd_card_usart, buffer, count);
	}

	return (uint32_t)ecode;
}


#include "sl_sleeptimer.h"
uint32_t get_fattime (void)
{
    sl_sleeptimer_date_t date;

    (void)sl_sleeptimer_get_datetime(&date);

    return (DWORD)(date.year - 80) << 25 |
           (DWORD)(date.month + 1) << 21 |
           (DWORD)date.month_day << 16 |
           (DWORD)date.hour << 11 |
           (DWORD)date.min << 5 |
           (DWORD)date.sec >> 1;
}
