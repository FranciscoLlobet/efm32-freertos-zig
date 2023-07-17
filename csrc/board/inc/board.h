/*
 * BOARD Interface
 *
 * board.h
 *
 *  Created on: 8 nov 2022
 *      Author: Francisco
 */

#ifndef BOARD_H_
#define BOARD_H_

#ifndef EFM32GG390F1024
#define EFM32GG390F1024  (1)
#endif

#ifndef __PROGRAM_START
#define __PROGRAM_START __main
#endif

#ifndef SL_CATALOG_POWER_MANAGER_PRESENT
#define SL_CATALOG_POWER_MANAGER_PRESENT    (1)
#endif

#include <stdint.h>

#include "em_device.h"
#include "em_chip.h"
#include "em_msc.h"
#include "em_cmu.h"
#include "em_gpio.h"
#include "gpiointerrupt.h"
#include "spidrv.h"
#include "uartdrv.h"
#include "em_usbxpress.h"

/* Adding LEDs and Buttons */
#include "sl_button.h"
#include "sl_led.h"

/* Include us delay */
#include "sl_udelay.h"

/* include usbexpress */
#include "em_usbxpress.h"

#define BOARD_SYSTICK_FREQUENCY    (1000)

/* LEDs */
#define BOARD_LED_ORANGE_PIN	(1)
#define BOARD_LED_ORANGE_PORT   (gpioPortB)
#define BOARD_LED_ORANGE_MODE   (gpioModePushPull)
#define BOARD_LED_ORANGE_INIT   (0)

#define BOARD_LED_RED_PIN       (12)
#define BOARD_LED_RED_PORT      (gpioPortA)
#define BOARD_LED_RED_MODE      (gpioModePushPull)
#define BOARD_LED_RED_INIT      (0)

#define BOARD_LED_YELLOW_PIN    (0)
#define BOARD_LED_YELLOW_PORT	(gpioPortB)
#define BOARD_LED_YELLOW_MODE   (gpioModePushPull)
#define BOARD_LED_YELLOW_INIT   (0)

/* Buttons */
#define BOARD_BUTTON1_PIN       (9)
#define BOARD_BUTTON1_PORT      (gpioPortE)
#define BOARD_BUTTON1_MODE      (gpioModeInputPullFilter)
#define BOARD_BUTTON1_INIT      (1)

#define BOARD_BUTTON2_PIN       (14)
#define BOARD_BUTTON2_PORT      (gpioPortE)
#define BOARD_BUTTON2_MODE      (gpioModeInputPullFilter)
#define BOARD_BUTTON2_INIT		(1)

/* SD CARD */

#define BOARD_SD_CARD_USART      		USART1
#define BOARD_SD_CARD_BITRATE     		UINT32_C(10000000)
#define BOARD_SD_CARD_WAKEUP_BITRATE    UINT32_C(100000)

#define SD_CARD_CS_PIN                  (15)
#define SD_CARD_CS_PORT                 (gpioPortB)
#define SD_CARD_CS_MODE                 (gpioModeWiredAnd)

#define SD_CARD_LS_PIN                  (7)
#define SD_CARD_LS_PORT                 (gpioPortC)
#define SD_CARD_LS_MODE                 (gpioModeWiredAnd)

#define SD_DETECT_PIN                   (12)
#define SD_DETECT_PORT                  (gpioPortF)
#define SD_DETECT_MODE                  (gpioModeInputPullFilter)
#define SD_DETECT_EDGE_RISING           true
#define SD_DETECT_EDGE_FALLING          true

#define SD_CARD_SPI1_MISO_PIN           (1)
#define SD_CARD_SPI1_MISO_PORT          (gpioPortD)
#define SD_CARD_SPI1_MISO_MODE          (gpioModeInput)

#define SD_CARD_SPI1_MOSI_PIN           (0)
#define SD_CARD_SPI1_MOSI_PORT          (gpioPortD)
#define SD_CARD_SPI1_MOSI_MODE          (gpioModePushPull)

#define SD_CARD_SPI1_SCK_PIN            (2)
#define SD_CARD_SPI1_SCK_PORT           (gpioPortD)
#define SD_CARD_SPI1_SCK_MODE           (gpioModePushPull)

#define PWR_2V5_SNOOZE_PIN              (5)
#define PWR_2V5_SNOOZE_PORT             (gpioPortF)
#define PWR_2V5_SNOOZE_MODE             (gpioModeWiredOr)

#define PWR_3V3_EN_PIN                  (11)
#define PWR_3V3_EN_PORT                 (gpioPortC)
#define PWR_3V3_EN_MODE                 (gpioModePushPull)

/* CC3100 */
#define WIFI_SERIAL_PORT                USART0
#define WIFI_SPI_BAUDRATE	            UINT32_C(10000000)

#define WIFI_SUPPLY_SETTING_DELAY_MS                                            UINT32_C(3)
#define WIFI_PWRON_HW_WAKEUP_DELAY_MS                                           UINT32_C(25)
#define WIFI_INIT_DELAY_MS                                                      UINT32_C(1350)
#define WIFI_MIN_HIB_DELAY_MS                                                   UINT32_C(10)
#define WIFI_HIB_WAKEUP_DELAY_MS                                                UINT32_C(50)
#define WIFI_MIN_RESET_DELAY_MS                                                 UINT32_C(5)
#define WIFI_POWER_OFF_DELAY_MS                                                 UINT32_C(20)

#define VDD_WIFI_EN_PIN                                                         (8)
#define VDD_WIFI_EN_PORT                                                        (gpioPortA)
#define VDD_WIFI_EN_MODE                                                        (gpioModePushPull)

#define WIFI_CSN_PIN                                                            (5)
#define WIFI_CSN_PORT                                                           (gpioPortC)
#define WIFI_CSN_MODE                                                           (gpioModePushPull)

#define WIFI_INT_PIN                                                            (10)
#define WIFI_INT_PORT                                                           (gpioPortA)
#define WIFI_INT_MODE                                                           (gpioModeInput)
#define WIFI_INT_EDGE_RISING                                                    true
#define WIFI_INT_EDGE_FALLING                                                   false

#define WIFI_NHIB_PIN                                                           (11)
#define WIFI_NHIB_PORT                                                          (gpioPortA)
#define WIFI_NHIB_MODE                                                          (gpioModePushPull)

#define WIFI_NRESET_PIN                                                         (15)
#define WIFI_NRESET_PORT                                                        (gpioPortA)
#define WIFI_NRESET_MODE                                                        (gpioModeWiredAnd)

#define WIFI_SPI0_MISO_PIN                                                      (11)
#define WIFI_SPI0_MISO_PORT                                                     (gpioPortE)
#define WIFI_SPI0_MISO_MODE                                                     (gpioModeInput)

#define WIFI_SPI0_MOSI_PIN                                                      (10)
#define WIFI_SPI0_MOSI_PORT                                                     (gpioPortE)
#define WIFI_SPI0_MOSI_MODE                                                     (gpioModePushPull)

#define WIFI_SPI0_SCK_PIN                                                       (12)
#define WIFI_SPI0_SCK_PORT                                                      (gpioPortE)
#define WIFI_SPI0_SCK_MODE                                                      (gpioModePushPull)


/* Sensors */
#define VDD_BMA280_PIN                                                          (7)
#define VDD_BMA280_PORT                                                         (gpioPortE)
#define VDD_BMA280_MODE                                                         (gpioModePushPull)

#define VDD_BME280_PIN                                                          (8)
#define VDD_BME280_PORT                                                         (gpioPortE)
#define VDD_BME280_MODE                                                         (gpioModePushPullDrive)

#define VDD_BMG160_PIN                                                          (13)
#define VDD_BMG160_PORT                                                         (gpioPortD)
#define VDD_BMG160_MODE                                                         (gpioModePushPullDrive)

#define VDD_BMI160_PIN                                                          (3)
#define VDD_BMI160_PORT                                                         (gpioPortE)
#define VDD_BMI160_MODE                                                         (gpioModePushPullDrive)

#define VDD_BMM150_PIN                                                          (5)
#define VDD_BMM150_PORT                                                         (gpioPortE)
#define VDD_BMM150_MODE                                                         (gpioModePushPull)

#define VDD_MAX44009_PIN                                                        (6)
#define VDD_MAX44009_PORT                                                       (gpioPortE)
#define VDD_MAX44009_MODE                                                       (gpioModePushPullDrive)

#define BMA280_INT1_PIN                                                         (8)
#define BMA280_INT1_PORT                                                        (gpioPortF)
#define BMA280_INT1_MODE                                                        (gpioModeInputPullFilter)
#define BMA280_INT1_EDGE_RISING                                                 false
#define BMA280_INT1_EDGE_FALLING                                                true

#define BMA280_INT2_PIN                                                         (9)
#define BMA280_INT2_PORT                                                        (gpioPortF)
#define BMA280_INT2_MODE                                                        (gpioModeInputPullFilter)
#define BMA280_INT2_EDGE_RISING                                                 false
#define BMA280_INT2_EDGE_FALLING                                                true

#define BMG160_INT1_PIN                                                         (6)
#define BMG160_INT1_PORT                                                        (gpioPortC)
#define BMG160_INT1_MODE                                                        (gpioModeInputPullFilter)
#define BMG160_INT1_EDGE_RISING                                                 false
#define BMG160_INT1_EDGE_FALLING                                                true

#define BMG160_INT2_PIN                                                         (7)
#define BMG160_INT2_PORT                                                        (gpioPortA)
#define BMG160_INT2_MODE                                                        (gpioModeInputPullFilter)
#define BMG160_INT2_EDGE_RISING                                                 false
#define BMG160_INT2_EDGE_FALLING                                                true

#define BMI160_INT1_PIN                                                         (13)
#define BMI160_INT1_PORT                                                        (gpioPortA)
#define BMI160_INT1_MODE                                                        (gpioModeInputPullFilter)
#define BMI160_INT1_EDGE_RISING                                                 false
#define BMI160_INT1_EDGE_FALLING                                                true

#define BMI160_INT2_PIN                                                         (14)
#define BMI160_INT2_PORT                                                        (gpioPortA)
#define BMI160_INT2_MODE                                                        (gpioModeInputPullFilter)
#define BMI160_INT2_EDGE_RISING                                                 false
#define BMI160_INT2_EDGE_FALLING                                                true

#define BMM150_DRDY_PIN                                                         (15)
#define BMM150_DRDY_PORT                                                        (gpioPortE)
#define BMM150_DRDY_MODE                                                        (gpioModeInputPullFilter)
#define BMM150_DRDY_EDGE_RISING                                                 true
#define BMM150_DRDY_EDGE_FALLING                                                false

#define BMM150_INT_PIN                                                          (12)
#define BMM150_INT_PORT                                                         (gpioPortD)
#define BMM150_INT_MODE                                                         (gpioModeInputPullFilter)
#define BMM150_INT_EDGE_RISING                                                  false
#define BMM150_INT_EDGE_FALLING                                                 true

#define MAX44009_INTN_PIN                                                       (4)
#define MAX44009_INTN_PORT                                                      (gpioPortE)
#define MAX44009_INTN_MODE                                                      (gpioModeInputPullFilter)
#define MAX44009_INTN_EDGE_RISING                                               false
#define MAX44009_INTN_EDGE_FALLING                                              true

/* EM9303 */
#define EM9303_BAUD_RATE                                                        (115200)
#define EM9303_ROUTE_LOCATION
#define EM9303_SERIAL_PORT                                                      UART0

#define EM9303_IRQ_PIN                                                          (11)
#define EM9303_IRQ_PORT                                                         (gpioPortD)
#define EM9303_IRQ_MODE                                                         (gpioModeDisabled)

#define EM9303_RST_PIN                                                          (9)
#define EM9303_RST_PORT                                                         (gpioPortA)
#define EM9303_RST_MODE                                                         (gpioModeWiredOr)

#define EM9303_UART0_RX_PIN                                                     (1)
#define EM9303_UART0_RX_PORT                                                    (gpioPortE)
#define EM9303_UART0_RX_MODE                                                    (gpioModeInput)
#define EM9303_UART0_RX_DMA_CHANNEL                                             (3)

#define EM9303_UART0_TX_PIN                                                     (0)
#define EM9303_UART0_TX_PORT                                                    (gpioPortE)
#define EM9303_UART0_TX_MODE                                                    (gpioModePushPull)
#define EM9303_UART0_TX_DMA_CHAN                                                (2)

#define EM9303_WAKEUP_PIN                                                       (10)
#define EM9303_WAKEUP_PORT                                                      (gpioPortD)
#define EM9303_WAKEUP_MODE                                                      (gpioModeWiredOr)

/* MASKS */
#define BOARD_2V5_MCU_MASK		  UINT32_C(1 << 0)
#define BOARD_2V5_EM9301_MASK     UINT32_C(1 << 1)
#define BOARD_2V5_CC31MOD_MASK	  UINT32_C(1 << 2)
#define BOARD_2V5_SD_CARD_MASK	  UINT32_C(1 << 3)
#define BOARD_2V5_LED_RED_MASK    UINT32_C(1 << 4)
#define BOARD_2V5_LED_ORANGE_MASK UINT32_C(1 << 5)
#define BOARD_2V5_LED_YELLOW_MASK UINT32_C(1 << 6)
#define BOARD_2V5_BMA280_MASK     UINT32_C(1 << 7)
#define BOARD_2V5_BMG160_MASK     UINT32_C(1 << 8)
#define BOARD_2V5_BME280_MASK     UINT32_C(1 << 9)
#define BOARD_2V5_BMI150_MASK     UINT32_C(1 << 10)
#define BOARD_2V5_MAX44009_MASK   UINT32_C(1 << 11)
#define BOARD_2V5_BMI160_MASK     UINT32_C(1 << 12)
#define BOARD_2V5_AKU340_MASK     UINT32_C(1 << 13)

void BOARD_Init(void);

/* Board SysTick Enable */
void BOARD_SysTick_Enable(void);
void BOARD_SysTick_Disable(void);

/* Delay function */
void BOARD_usDelay(uint32_t delay_in_us);
void BOARD_msDelay(uint32_t delay_in_ms);

/* Perform the MCU Reset */
void BOARD_MCU_Reset(void);

void Board_CC3100_Init(void);
void board_i2c_init(void);

/* SD CARD Functionality */
void BOARD_SD_Card_Init(void);
void BOARD_SD_Card_Enable(void);
void BOARD_SD_Card_Disable(void);

void BOARD_SD_CARD_Select(void);
void BOARD_SD_CARD_Deselect(void);
void BOARD_SD_CARD_SetFastBaudrate(void);
void BOARD_SD_CARD_SetSlowBaudrate(void);
uint32_t BOARD_SD_CARD_Send(const void *buffer, int count);
uint32_t BOARD_SD_CARD_Recieve(void *buffer, int count);
uint32_t BOARD_SD_CARD_IsInserted(void);

/* Watchdog */
void BOARD_Watchdog_Init(void);
void BOARD_Watchdog_Feed(void);
void BOARD_Watchdog_Enable(void);


void BOARD_EM9301_Init(void);
void BOARD_EM9301_Enable(void);
void BOARD_EM9301_Reset(void);
void BOARD_EM9301_Disable(void);
void BOARD_EM9301_Wakeup(bool wakeup);

void BOARD_USB_Init(void);

/* Button group */
extern sl_button_t button1;
extern sl_button_t button2;

/* LED group */
extern sl_led_t led_red;
extern sl_led_t led_orange;
extern sl_led_t led_yellow;

/* SPI DRV Handles */
extern SPIDRV_HandleData_t sd_card_usart;
extern SPIDRV_HandleData_t cc3100_usart;

extern UARTDRV_HandleData_t em9301_uart;

extern uint8_t usb_rx_buf[64];
extern uint8_t usb_tx_buf[64];

#endif /* BOARD_H_ */
