/*
 * Board_CC310.c
 *
 *  Created on: 11 nov 2022
 *      Author: Francisco
 */

#include "FreeRTOS.h"
#include "board.h"
#include "semphr.h"
#include "simplelink.h"

#define BOARD_CC3100_FD ((int)INT32_C(1))

static SemaphoreHandle_t tx_queue = NULL;
static SemaphoreHandle_t rx_queue = NULL;

#define CC3100_SPI_RX_TIMEOUT_MS (1000)
#define CC3100_SPI_TX_TIMEOUT_MS (1000)

SPIDRV_HandleData_t cc3100_usart;

const SPIDRV_Init_t cc3100_usart_init_data = {
    .port           = WIFI_SERIAL_PORT,
    .portLocation   = _USART_ROUTE_LOCATION_LOC0,
    .bitRate        = WIFI_SPI_BAUDRATE,
    .frameLength    = 8,
    .dummyTxValue   = 0xFF,
    .type           = spidrvMaster,
    .bitOrder       = spidrvBitOrderMsbFirst,
    .clockMode      = spidrvClockMode0,
    .csControl      = spidrvCsControlApplication,
    .slaveStartMode = spidrvSlaveStartImmediate,

};

struct transfer_status_s
{
    Ecode_t transferStatus;  // status
    int itemsTransferred;    // items_transferred
};

static volatile P_EVENT_HANDLER interrupt_handler_callback = NULL;
static volatile void *interrupt_handler_pValue             = NULL;

extern void system_reset(void);

static void cc3100_interrupt_callback(uint8_t intNo)
{
    if (((uint8_t)WIFI_INT_PIN == intNo) && (NULL != interrupt_handler_callback))
    {
        interrupt_handler_callback(interrupt_handler_pValue);
    }
}

static void recieve_callback(struct SPIDRV_HandleData *handle, Ecode_t transferStatus, int itemsTransferred)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    if (handle == &cc3100_usart)
    {
        struct transfer_status_s transfer_status_information = {.transferStatus   = transferStatus,
                                                                .itemsTransferred = itemsTransferred};

        if (NULL != rx_queue)
        {
            (void)xQueueSendFromISR(rx_queue, &transfer_status_information, &xHigherPriorityTaskWoken);
        }
    }

    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

static void send_callback(struct SPIDRV_HandleData *handle, Ecode_t transferStatus, int itemsTransferred)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    if (handle == &cc3100_usart)
    {
        struct transfer_status_s transfer_status_information = {.transferStatus   = transferStatus,
                                                                .itemsTransferred = itemsTransferred};
        if (NULL != tx_queue)
        {
            (void)xQueueSendFromISR(tx_queue, &transfer_status_information, &xHigherPriorityTaskWoken);
        }
    }

    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

static void cc3100_spi_select(void)
{
    GPIO_PinOutClear(WIFI_CSN_PORT, WIFI_CSN_PIN);
    BOARD_usDelay(1);
}

static void cc3100_spi_deselect(void)
{
    BOARD_usDelay(1);
    GPIO_PinOutSet(WIFI_CSN_PORT, WIFI_CSN_PIN);
}

void Board_CC3100_Init(void)
{
    GPIO_PinModeSet(VDD_WIFI_EN_PORT, VDD_WIFI_EN_PIN, VDD_WIFI_EN_MODE, 1);
    GPIO_PinModeSet(WIFI_NRESET_PORT, WIFI_NRESET_PIN, WIFI_NRESET_MODE, 1);
    GPIO_PinModeSet(WIFI_CSN_PORT, WIFI_CSN_PIN, WIFI_CSN_MODE, 1);
    GPIO_PinModeSet(WIFI_INT_PORT, WIFI_INT_PIN, WIFI_INT_MODE, 0);
    GPIO_PinModeSet(WIFI_NHIB_PORT, WIFI_NHIB_PIN, WIFI_NHIB_MODE, 1);
    GPIO_PinModeSet(WIFI_SPI0_MISO_PORT, WIFI_SPI0_MISO_PIN, WIFI_SPI0_MISO_MODE, 0);
    GPIO_PinModeSet(WIFI_SPI0_MOSI_PORT, WIFI_SPI0_MOSI_PIN, WIFI_SPI0_MOSI_MODE, 0);
    GPIO_PinModeSet(WIFI_SPI0_SCK_PORT, WIFI_SPI0_SCK_PIN, WIFI_SPI0_SCK_MODE, 0);

    GPIOINT_CallbackRegister(WIFI_INT_PIN, cc3100_interrupt_callback);

    GPIO_ExtIntConfig(WIFI_INT_PORT, WIFI_INT_PIN, WIFI_INT_PIN, true, false, false);
}

void CC3100_DeviceEnablePreamble(void)
{
    GPIO_PinOutClear(VDD_WIFI_EN_PORT, VDD_WIFI_EN_PIN);
    GPIO_PinOutSet(WIFI_NHIB_PORT, WIFI_NHIB_PIN);
    BOARD_msDelay(WIFI_SUPPLY_SETTING_DELAY_MS);

    GPIO_PinOutClear(WIFI_NRESET_PORT, WIFI_NRESET_PIN);
    BOARD_msDelay(WIFI_MIN_RESET_DELAY_MS);

    GPIO_PinOutSet(WIFI_NRESET_PORT, WIFI_NRESET_PIN);
    BOARD_msDelay(WIFI_PWRON_HW_WAKEUP_DELAY_MS + WIFI_INIT_DELAY_MS);
}

void CC3100_DeviceEnable(void)
{
    GPIO_PinOutSet(WIFI_NHIB_PORT, WIFI_NHIB_PIN);
    BOARD_msDelay(50);  // Wakeup from hybernate

    //GPIO_PinModeSet(WIFI_INT_PORT, WIFI_INT_PIN, WIFI_INT_MODE, 0);
    //GPIO_ExtIntConfig(WIFI_INT_PORT, WIFI_INT_PIN, WIFI_INT_PIN, true, false, true); // Enable Interrupt active high
}

void CC3100_DeviceDisable(void)
{
    GPIO_ExtIntConfig(WIFI_INT_PORT, WIFI_INT_PIN, WIFI_INT_PIN, true, false, false);
    GPIO_PinModeSet(WIFI_INT_PORT, WIFI_INT_PIN, gpioModeDisabled, 0);
    GPIO_PinOutClear(WIFI_NHIB_PORT, WIFI_NHIB_PIN);  // Set to Hybernate
    BOARD_msDelay(10);                                // Hybernate low pulse
}

int CC3100_IfOpen(const char *pIfName, unsigned long flags)
{
    int ret = -1;
    // Success: FD (positive integer)
    // Failure: -1

    // Plausibility Check for the IfOpen
    if ((strncmp(pIfName, CC3100_DEVICE_NAME, strlen(CC3100_DEVICE_NAME)) == 0) && (flags == 0))
    {
        if (tx_queue == NULL)
        {
            tx_queue = xQueueCreate(1, sizeof(struct transfer_status_s));
        }

        if (rx_queue == NULL)
        {
            rx_queue = xQueueCreate(1, sizeof(struct transfer_status_s));
        }

        if (ECODE_OK == SPIDRV_Init(&cc3100_usart, &cc3100_usart_init_data))
        {
            NVIC_SetPriority(USART0_RX_IRQn, 5);
            NVIC_SetPriority(USART0_TX_IRQn, 6);
            GPIO_PinOutSet(WIFI_NHIB_PORT, WIFI_NHIB_PIN);  // Clear Hybernate
            BOARD_msDelay(50);                              // Wakeup delay
            cc3100_spi_deselect();
            ret = BOARD_CC3100_FD;
        }
    }

    return ret;
}

int CC3100_IfClose(Fd_t Fd)
{
    int ret = -1;

    if (BOARD_CC3100_FD == Fd)
    {
        cc3100_spi_deselect();
        if (ECODE_EMDRV_SPIDRV_OK == SPIDRV_DeInit(&cc3100_usart))
        {
            ret = 0;
        }
        if (NULL != tx_queue)
        {
            vQueueDelete(tx_queue);
            tx_queue = NULL;
        }
        if (NULL != rx_queue)
        {
            vQueueDelete(rx_queue);
            rx_queue = NULL;
        }
    }

    return ret;
}

int CC3100_IfRead(Fd_t Fd, uint8_t *pBuff, int Len)
{
    if ((NULL == pBuff) || (Len <= 0) || (BOARD_CC3100_FD != Fd))
    {
        return -1;
    }

    int retVal = -1;
    cc3100_spi_select();

    if (rx_queue != NULL)  // Change to check if scheduler is active
    {
        struct transfer_status_s transfer_status = {ECODE_EMDRV_SPIDRV_PARAM_ERROR, -1};

        (void)xQueueReset(rx_queue);

        Ecode_t ecode = SPIDRV_MReceive(&cc3100_usart, pBuff, Len, recieve_callback);
        if (ECODE_EMDRV_SPIDRV_OK == ecode)
        {
            if (pdTRUE == xQueueReceive(rx_queue, &transfer_status, CC3100_SPI_RX_TIMEOUT_MS))  // Added rx timeout
            {
                ecode = transfer_status.transferStatus;
            }
            else
            {
                ecode = ECODE_EMDRV_SPIDRV_TIMEOUT;
            }
        }

        if (ECODE_EMDRV_SPIDRV_OK == ecode)
        {
            retVal = transfer_status.itemsTransferred;
        }
    }

    cc3100_spi_deselect();
    return retVal;
}

int CC3100_IfWrite(Fd_t Fd, const uint8_t *pBuff, int Len)
{
    if ((NULL == pBuff) || (Len <= 0) || (BOARD_CC3100_FD != Fd))
    {
        return -1;
    }

    int retVal = -1;

    cc3100_spi_select();

    if (tx_queue != NULL)  // Change to check if scheduler is active
    {
        struct transfer_status_s transfer_status = {ECODE_EMDRV_SPIDRV_PARAM_ERROR, -1};

        xQueueReset(tx_queue);
        Ecode_t ecode = SPIDRV_MTransmit(&cc3100_usart, pBuff, Len, send_callback);
        if (ECODE_EMDRV_SPIDRV_OK == ecode)
        {
            if (pdTRUE == xQueueReceive(tx_queue, &transfer_status, CC3100_SPI_TX_TIMEOUT_MS))
            {
                ecode = transfer_status.transferStatus;
            }
            else
            {
                ecode = ECODE_EMDRV_SPIDRV_TIMEOUT;
            }
        }

        if (ECODE_EMDRV_SPIDRV_OK == ecode)
        {
            retVal = transfer_status.itemsTransferred;
        }
    }

    cc3100_spi_deselect();
    return retVal;
}

void CC3100_IfRegIntHdlr(P_EVENT_HANDLER interruptHdl, void *pValue)
{
    interrupt_handler_callback = interruptHdl;
    interrupt_handler_pValue   = pValue;

    if(interruptHdl == NULL)
    {
        GPIO_ExtIntConfig(WIFI_INT_PORT, WIFI_INT_PIN, WIFI_INT_PIN, true, false, false);
        GPIO_PinModeSet(WIFI_INT_PORT, WIFI_INT_PIN, gpioModeDisabled, 0);
    }
    else
    {
        GPIO_PinModeSet(WIFI_INT_PORT, WIFI_INT_PIN, gpioModeInput, 0);
        GPIO_ExtIntConfig(WIFI_INT_PORT, WIFI_INT_PIN, WIFI_INT_PIN, true, false, true);
    }
}

void CC3100_MaskIntHdlr(void) { ; }

void CC3100_UnmaskIntHdlr(void) { ; }

/* General Event Handler */
void CC3100_GeneralEvtHdlr(SlDeviceEvent_t *slGeneralEvent)
{
    if (NULL == slGeneralEvent)
    {
        return;
    }

    switch (slGeneralEvent->Event)
    {
        case SL_DEVICE_GENERAL_ERROR_EVENT:
            // General error event
            printf("General Error: Status=%d, Sender=%d\n", slGeneralEvent->EventData.deviceEvent.status,
                   slGeneralEvent->EventData.deviceEvent.sender);
            system_reset();
            break;
        case SL_DEVICE_ABORT_ERROR_EVENT:
            printf("Abort Error: AbortType=%l, AbortData=%l\n", slGeneralEvent->EventData.deviceReport.AbortType,
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

void CC3100_HttpServerCallback(SlHttpServerEvent_t *pSlHttpServerEvent, SlHttpServerResponse_t *pSlHttpServerResponse)
{
    (void)pSlHttpServerEvent;
    (void)pSlHttpServerResponse;
}
void CC3100_SockEvtHdlr(SlSockEvent_t *pSlSockEvent)
{
    if (pSlSockEvent == NULL)
    {
        return;
    }

    switch (pSlSockEvent->Event)
    {
        case SL_SOCKET_TX_FAILED_EVENT:
            // status, sd
            printf("tx failed\r\n");
            break;
        case SL_SOCKET_ASYNC_EVENT:
            // sd, type
            printf("socket async event\r\n");
            break;
        default:
            break;
    }
}
