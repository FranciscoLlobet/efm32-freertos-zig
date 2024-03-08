/*
 * board_bma280.c
 *
 *  Created on: 27 nov 2022
 *      Author: Francisco
 */
#include "board_i2c_sensors.h"

struct bma2_dev board_bma280;
static uint32_t dummy_val = 0;

static BMA2_INTF_RET_TYPE _bma280_read(uint8_t reg_addr, uint8_t *reg_data, uint32_t len, void *intf_ptr)
{
    (void)intf_ptr;
    BMA2_INTF_RET_TYPE ret = BMA2_E_COM_FAIL;

    if (intf_ptr == &dummy_val)
    {
        I2C_TransferSeq_TypeDef transfer;
        transfer.addr        = (BMA2_I2C_ADDR1 << 1);
        transfer.buf[0].data = &reg_addr;
        transfer.buf[0].len  = 1;
        transfer.buf[1].data = reg_data;
        transfer.buf[1].len  = len;
        transfer.flags       = I2C_FLAG_WRITE_READ;

        if (i2cTransferDone == board_i2c0_transfer(&transfer))
        {
            ret = BMA2_OK;
        }
    }

    return ret;
}

static BMA2_INTF_RET_TYPE _bma280_write(uint8_t reg_addr, const uint8_t *reg_data, uint32_t len, void *intf_ptr)
{
    BMA2_INTF_RET_TYPE ret = BMA2_E_COM_FAIL;

    if (intf_ptr == &dummy_val)
    {
        I2C_TransferSeq_TypeDef transfer;
        transfer.addr        = (BMA2_I2C_ADDR1 << 1);
        transfer.buf[0].data = &reg_addr;
        transfer.buf[0].len  = 1;
        transfer.buf[1].data = reg_data;
        transfer.buf[1].len  = len;
        transfer.flags       = I2C_FLAG_WRITE_WRITE;

        if (i2cTransferDone == board_i2c0_transfer(&transfer))
        {
            ret = BMA2_OK;
        }
    }
    return ret;
}

static void _bma280_delay_us(uint32_t period, void *intf_ptr)
{
    if (intf_ptr == &dummy_val)
    {
        BOARD_usDelay(period);
    }
    else
    {
        while (1)
        {
            __NOP();
        }
    }
}

void board_bma280_enable(void)
{
    board_bma280.intf_ptr = &dummy_val;
    board_bma280.intf     = BMA2_I2C_INTF;
    board_bma280.read     = _bma280_read;
    board_bma280.write    = _bma280_write;
    board_bma280.delay_us = _bma280_delay_us;

    /* Enable BMA280 */
    GPIO_PinModeSet(VDD_BMA280_PORT, VDD_BMA280_PIN, VDD_BMA280_MODE, 1);
    BOARD_msDelay(3);

    /* Enable BMA280 INT1 and INT2 */
    GPIO_PinOutSet(BMA280_INT1_PORT, BMA280_INT1_PIN);
    GPIO_PinOutSet(BMA280_INT2_PORT, BMA280_INT2_PIN);

    GPIO_ExtIntConfig(BMA280_INT1_PORT, BMA280_INT1_PIN, BMA280_INT1_PIN, true, true, false);

    /* Execute this to ensure that the slaves are re-inited correctly */
    I2CSPM_Init(&init_i2c0);
    NVIC_ClearPendingIRQ(I2C0_IRQn);
}
