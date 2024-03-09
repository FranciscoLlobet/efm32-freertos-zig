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
