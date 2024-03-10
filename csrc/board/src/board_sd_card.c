/*
 * Copyright (c) 2022-2024 Francisco Llobet-Blandino and the "Miso Project".
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#include "board_sd_card.h"

#include "board.h"
// clang-format off
#include "ff.h"
#include "diskio.h"
// clang-format on

/* OS resources */
#include <FreeRTOS.h>
#include <semphr.h>

/* SPI DRV Handle */
SPIDRV_HandleData_t sd_card_usart;

/* Detect SD Card */
static void card_detect_callback(uint8_t intNo);

/* TX-RX Semaphores */
static SemaphoreHandle_t tx_semaphore = NULL;
static SemaphoreHandle_t rx_semaphore = NULL;

struct transfer_status_s
{
    Ecode_t transferStatus;  // status
    int itemsTransferred;    // items_transferred
};

uint32_t BOARD_SD_CARD_IsInserted(void)
{
    return (0 == GPIO_PinInGet(SD_DETECT_PORT, SD_DETECT_PIN)) ? (uint32_t)UINT32_C(1) : (uint32_t)UINT32_C(0);
}

static void card_detect_callback(uint8_t intNo)
{
    (void)intNo;
    if (UINT32_C(1) == BOARD_SD_CARD_IsInserted())
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

    GPIO_ExtIntConfig(SD_DETECT_PORT, SD_DETECT_PIN, SD_DETECT_PIN, true, true, false);
}

void BOARD_SD_Card_Enable(void)
{
    if (tx_semaphore == NULL)
    {
        tx_semaphore = xQueueCreate(1, sizeof(Ecode_t));
    }

    if (rx_semaphore == NULL)
    {
        rx_semaphore = xQueueCreate(1, sizeof(Ecode_t));
    }

    /* SPI DRV Init */
    SPIDRV_Init_t sd_card_init_data = {
        .port           = BOARD_SD_CARD_USART,
        .portLocation   = _USART_ROUTE_LOCATION_LOC1,
        .bitRate        = BOARD_SD_CARD_WAKEUP_BITRATE,
        .frameLength    = 8,
        .dummyTxValue   = 0xFF,
        .type           = spidrvMaster,
        .bitOrder       = spidrvBitOrderMsbFirst,
        .clockMode      = spidrvClockMode0,
        .csControl      = spidrvCsControlApplication,
        .slaveStartMode = spidrvSlaveStartImmediate,
    };

    SPIDRV_Init(&sd_card_usart, &sd_card_init_data);

    GPIOINT_CallbackRegister(SD_DETECT_PIN, card_detect_callback);

    GPIO_ExtIntConfig(SD_DETECT_PORT, SD_DETECT_PIN, SD_DETECT_PIN, true, true, true);

    (void)BOARD_SD_CARD_IsInserted();
}

void BOARD_SD_Card_Disable(void)
{
    (void)SPIDRV_DeInit(&sd_card_usart);

    GPIO_PinModeSet(SD_CARD_CS_PORT, SD_CARD_CS_PIN, gpioModeDisabled, 1);
    GPIO_PinModeSet(SD_CARD_LS_PORT, SD_CARD_CS_PIN, gpioModeDisabled, 1);
    GPIO_PinModeSet(SD_DETECT_PORT, SD_DETECT_PIN, SD_DETECT_MODE, 1);
    GPIO_PinModeSet(SD_CARD_SPI1_MISO_PORT, SD_CARD_SPI1_MISO_PIN, gpioModeDisabled, 0);
    GPIO_PinModeSet(SD_CARD_SPI1_MOSI_PORT, SD_CARD_SPI1_MOSI_PIN, gpioModeDisabled, 0);
    GPIO_PinModeSet(SD_CARD_SPI1_SCK_PORT, SD_CARD_SPI1_SCK_PIN, gpioModeDisabled, 0);

    GPIO_ExtIntConfig(SD_DETECT_PORT, SD_DETECT_PIN, SD_DETECT_PIN, false, false, false);
}

void BOARD_SD_CARD_Deselect(void) { GPIO_PinOutSet(SD_CARD_CS_PORT, SD_CARD_CS_PIN); }

void BOARD_SD_CARD_Select(void) { GPIO_PinOutClear(SD_CARD_CS_PORT, SD_CARD_CS_PIN); }

void BOARD_SD_CARD_SetFastBaudrate(void) { SPIDRV_SetBitrate(&sd_card_usart, BOARD_SD_CARD_BITRATE); }

void BOARD_SD_CARD_SetSlowBaudrate(void) { SPIDRV_SetBitrate(&sd_card_usart, BOARD_SD_CARD_WAKEUP_BITRATE); }

/* recieve callback for async reception */
static void recieve_callback(struct SPIDRV_HandleData *handle, Ecode_t transferStatus, int itemsTransferred)
{
    (void)handle;
    (void)itemsTransferred;

    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    if (handle == &sd_card_usart)
    {
        if (NULL != rx_semaphore) xQueueSendFromISR(rx_semaphore, &transferStatus, &xHigherPriorityTaskWoken);
    }

    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

/* tx callback for async tx */
static void send_callback(struct SPIDRV_HandleData *handle, Ecode_t transferStatus, int itemsTransferred)
{
    (void)itemsTransferred;

    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    if (handle == &sd_card_usart)
    {
        if (NULL != tx_semaphore) xQueueSendFromISR(tx_semaphore, &transferStatus, &xHigherPriorityTaskWoken);
    }

    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

/* OS enabled rx callback */
uint32_t BOARD_SD_CARD_Recieve(void *buffer, int count)
{
    Ecode_t ecode = ECODE_EMDRV_SPIDRV_PARAM_ERROR;

    (void)xQueueReset(rx_semaphore);
    ecode = SPIDRV_MReceive(&sd_card_usart, buffer, count, recieve_callback);
    if (ECODE_EMDRV_SPIDRV_OK == ecode)
    {
        (void)xQueueReceive(rx_semaphore, &ecode, portMAX_DELAY);
    }

    return (uint32_t)ecode;
}

/* OS enabled tx callback */
uint32_t BOARD_SD_CARD_Send(const void *buffer, int count)
{
    Ecode_t ecode = ECODE_EMDRV_SPIDRV_PARAM_ERROR;

    (void)xQueueReset(tx_semaphore);
    ecode = SPIDRV_MTransmit(&sd_card_usart, buffer, count, send_callback);
    if (ECODE_EMDRV_SPIDRV_OK == ecode)
    {
        (void)xQueueReceive(tx_semaphore, &ecode, portMAX_DELAY);
    }

    return (uint32_t)ecode;
}

#include "sl_sleeptimer.h"
uint32_t get_fattime(void)
{
    sl_sleeptimer_date_t date;

    (void)sl_sleeptimer_get_datetime(&date);

    return (DWORD)(date.year - 80) << 25 | (DWORD)(date.month + 1) << 21 | (DWORD)date.month_day << 16 |
           (DWORD)date.hour << 11 | (DWORD)date.min << 5 | (DWORD)date.sec >> 1;
}
