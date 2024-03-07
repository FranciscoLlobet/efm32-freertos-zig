/*
 * board_i2c_sensors.c
 *
 *  Created on: 27 nov 2022
 *      Author: Francisco
 */

#include "board_i2c_sensors.h"

#include "FreeRTOS.h"
#include "em_cmu.h"
#include "queue.h"
#include "semphr.h"
#include "sl_i2cspm.h"
#include "sl_i2cspm_inst_config.h"
#include "task.h"

#define SL_I2CSPM_INSTANCE_HLR           i2cClockHLRAsymetric
#define SL_I2CSPM_INSTANCE_MAX_FREQ      I2C_FREQ_FAST_MAX

#define BOARD_I2C_SENSORS_I2C_TIMEOUT_MS (10000)

I2CSPM_Init_TypeDef init_i2c0 = {.port         = SL_I2CSPM_INSTANCE_PERIPHERAL,
                                 .sclPort      = SL_I2CSPM_INSTANCE_SCL_PORT,
                                 .sclPin       = SL_I2CSPM_INSTANCE_SCL_PIN,
                                 .sdaPort      = SL_I2CSPM_INSTANCE_SDA_PORT,
                                 .sdaPin       = SL_I2CSPM_INSTANCE_SDA_PIN,
                                 .portLocation = SL_I2CSPM_INSTANCE_ROUTE_LOC,
                                 .i2cRefFreq   = 0,
                                 .i2cMaxFreq   = I2C_FREQ_FAST_MAX,
                                 .i2cClhr      = i2cClockHLRAsymetric};

#if 0
I2CSPM_Init_TypeDef init_i2c1 = {
  .port = SL_I2CSPM_INSTANCE_PERIPHERAL,
  .sclPort = SL_I2CSPM_INSTANCE_SCL_PORT,
  .sclPin = SL_I2CSPM_INSTANCE_SCL_PIN,
  .sdaPort = SL_I2CSPM_INSTANCE_SDA_PORT,
  .sdaPin = SL_I2CSPM_INSTANCE_SDA_PIN,
  .portLocation = SL_I2CSPM_INSTANCE_ROUTE_LOC,
  .i2cRefFreq = 0,
  .i2cMaxFreq = I2C_FREQ_FAST_MAX,
  .i2cClhr = i2cClockHLRAsymetric
};
#endif

struct
{
    SemaphoreHandle_t i2c0_semaphore;
    SemaphoreHandle_t i2c0_mutex;
} board_i2c_os_control = {.i2c0_semaphore = NULL, .i2c0_mutex = NULL};

void board_i2c_init(void)
{
    /* Init GPIO settings */
    CMU_ClockEnable(cmuClock_GPIO, true);

    GPIO_PinModeSet(VDD_BMA280_PORT, VDD_BMA280_PIN, VDD_BMA280_MODE, 0);
    GPIO_PinModeSet(VDD_BME280_PORT, VDD_BME280_PIN, VDD_BME280_MODE, 0);
    GPIO_PinModeSet(VDD_BMG160_PORT, VDD_BMG160_PIN, VDD_BMG160_MODE, 0);
    GPIO_PinModeSet(VDD_BMI160_PORT, VDD_BMI160_PIN, VDD_BMI160_MODE, 0);
    GPIO_PinModeSet(VDD_BMM150_PORT, VDD_BMM150_PIN, VDD_BMM150_MODE, 0);
    GPIO_PinModeSet(VDD_MAX44009_PORT, VDD_MAX44009_PIN, VDD_MAX44009_MODE, 0);
    GPIO_PinModeSet(BMA280_INT1_PORT, BMA280_INT1_PIN, BMA280_INT1_MODE, 0);
    GPIO_PinModeSet(BMA280_INT2_PORT, BMA280_INT2_PIN, BMA280_INT2_MODE, 0);
    GPIO_PinModeSet(BMG160_INT1_PORT, BMG160_INT1_PIN, BMG160_INT1_MODE, 0);
    GPIO_PinModeSet(BMG160_INT2_PORT, BMG160_INT2_PIN, BMG160_INT2_MODE, 0);
    GPIO_PinModeSet(BMI160_INT1_PORT, BMI160_INT1_PIN, BMI160_INT1_MODE, 0);
    GPIO_PinModeSet(BMI160_INT2_PORT, BMI160_INT2_PIN, BMI160_INT2_MODE, 0);

    GPIO_PinModeSet(BMM150_DRDY_PORT, BMM150_DRDY_PIN, BMM150_DRDY_MODE, 0);
    GPIO_PinModeSet(BMM150_INT_PORT, BMM150_INT_PIN, BMM150_INT_MODE, 0);
    GPIO_PinModeSet(MAX44009_INTN_PORT, MAX44009_INTN_PIN, MAX44009_INTN_MODE, 0);

    GPIO_PinModeSet(SL_I2CSPM_INSTANCE_SCL_PORT, SL_I2CSPM_INSTANCE_SCL_PIN, gpioModeWiredAnd, 1);
    GPIO_PinModeSet(SL_I2CSPM_INSTANCE_SDA_PORT, SL_I2CSPM_INSTANCE_SDA_PIN, gpioModeWiredAnd, 1);

    NVIC_ClearPendingIRQ(I2C0_IRQn);
    NVIC_SetPriority(I2C0_IRQn, (5));

#if (MISO_APPLICATION)
    if (board_i2c_os_control.i2c0_semaphore == NULL)
    {
        board_i2c_os_control.i2c0_semaphore = xQueueCreate(1, sizeof(I2C_TransferReturn_TypeDef));
    }
    if (board_i2c_os_control.i2c0_mutex == NULL)
    {
        board_i2c_os_control.i2c0_mutex = xSemaphoreCreateMutex();
        (void)xSemaphoreGive(board_i2c_os_control.i2c0_mutex);
    }

    I2CSPM_Init(&init_i2c0);
#endif
}

I2C_TransferReturn_TypeDef board_i2c0_transfer(I2C_TransferSeq_TypeDef *seq)
{
    I2C_TransferReturn_TypeDef ret = i2cTransferSwFault;

    if (pdFALSE == xSemaphoreTake(board_i2c_os_control.i2c0_mutex, pdMS_TO_TICKS(BOARD_I2C_SENSORS_I2C_TIMEOUT_MS)))
    {
        return i2cTransferSwFault;
    }

    NVIC_EnableIRQ(I2C0_IRQn);

    ret = I2C_TransferInit(I2C0, seq);
    if (ret > 0)
    {
        ret = i2cTransferSwFault;
        if (pdFALSE == xQueueReceive(board_i2c_os_control.i2c0_semaphore, &ret, portMAX_DELAY))
        {
            ret = i2cTransferSwFault;
        }
    }
    else if (ret == 0)
    {
        ret = I2C_Transfer(I2C0);
    }

    NVIC_ClearPendingIRQ(I2C0_IRQn);
    NVIC_DisableIRQ(I2C0_IRQn);

    (void)xSemaphoreGive(board_i2c_os_control.i2c0_mutex);

    return ret;
}

void I2C0_IRQHandler(void)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    I2C_TransferReturn_TypeDef ret      = I2C_Transfer(I2C0);

    if (ret != i2cTransferInProgress)
    {
        if (NULL != board_i2c_os_control.i2c0_semaphore)
        {
            xQueueSendFromISR(board_i2c_os_control.i2c0_semaphore, &ret, &xHigherPriorityTaskWoken);

            portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
        }
    }
}
