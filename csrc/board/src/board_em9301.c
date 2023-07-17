#include "board.h"
#include "uartdrv.h"

UARTDRV_HandleData_t em9301_uart;

UARTDRV_Buffer_FifoQueue_t rx_queue;
UARTDRV_Buffer_FifoQueue_t tx_queue;


void BOARD_EM9301_Init(void)
{
    (void)GPIO_PinModeSet(EM9303_IRQ_PORT, EM9303_IRQ_PIN, EM9303_IRQ_MODE, 0);
    (void)GPIO_PinModeSet(EM9303_RST_PORT, EM9303_RST_PIN, EM9303_RST_MODE, 0);
    (void)GPIO_PinModeSet(EM9303_UART0_RX_PORT, EM9303_UART0_RX_PIN, EM9303_UART0_RX_MODE, 0);
    (void)GPIO_PinModeSet(EM9303_UART0_TX_PORT, EM9303_UART0_TX_PIN, EM9303_UART0_TX_MODE, 1);
    (void)GPIO_PinModeSet(EM9303_WAKEUP_PORT, EM9303_WAKEUP_PIN, EM9303_WAKEUP_MODE, 0);
}

void BOARD_EM9301_Enable(void)
{
    UARTDRV_InitUart_t initData;

    initData.port = EM9303_SERIAL_PORT;
    initData.baudRate = EM9303_BAUD_RATE;
    initData.portLocation = 1;
    initData.rxQueue = &rx_queue;
    initData.txQueue = &tx_queue;
    initData.stopBits = usartStopbits1;
    initData.parity = USART_FRAME_PARITY_NONE;
    initData.oversampling = USART_CTRL_OVS_X4;
    initData.fcType = uartdrvFlowControlNone;
    initData.mvdis = true;

    (void)UARTDRV_InitUart(&em9301_uart, &initData);

    BOARD_msDelay(1);
}

void BOARD_EM9301_Disable(void)
{
    UARTDRV_Abort(&em9301_uart, uartdrvAbortAll);

    BOARD_EM9301_Reset();

    UARTDRV_DeInit(&em9301_uart);

    GPIO_PinModeSet(EM9303_IRQ_PORT, EM9303_IRQ_PIN, gpioModeDisabled, 0);
    GPIO_PinModeSet(EM9303_RST_PORT, EM9303_RST_PIN, gpioModeDisabled, 0);
    GPIO_PinModeSet(EM9303_UART0_RX_PORT, EM9303_UART0_RX_PIN, gpioModeDisabled, 0);
    GPIO_PinModeSet(EM9303_UART0_TX_PORT, EM9303_UART0_TX_PIN, gpioModeDisabled, 0);
    GPIO_PinModeSet(EM9303_WAKEUP_PORT, EM9303_WAKEUP_PIN, gpioModeDisabled, 0);

}

void BOARD_EM9301_WakeUp(bool high){
    if(high){
        GPIO_PinOutSet(EM9303_WAKEUP_PORT, EM9303_WAKEUP_PIN);
        BOARD_msDelay(250);
    }
    else{
        GPIO_PinOutClear(EM9303_WAKEUP_PORT, EM9303_WAKEUP_PIN);
    }
}

void BOARD_EM9301_Reset(void)
{
    GPIO_PinOutSet(EM9303_RST_PORT, EM9303_RST_PIN);
    BOARD_msDelay(2); // Pull-up delay
    GPIO_PinOutClear(EM9303_RST_PORT, EM9303_RST_PIN);
    BOARD_msDelay(5);
}