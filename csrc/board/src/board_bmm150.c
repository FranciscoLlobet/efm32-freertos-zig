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

struct bmm150_dev board_bmm150;

uint32_t bmm150_intf_val = 0;

static void _bmm150_delay_us(uint32_t period, void *intf_ptr)
{
    if (intf_ptr == &bmm150_intf_val)
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

static BMM150_INTF_RET_TYPE _bmm150_read(uint8_t reg_addr, uint8_t *reg_data, uint32_t length, void *intf_ptr)
{
    BMM150_INTF_RET_TYPE ret = BMM150_E_COM_FAIL;
    if (intf_ptr == &bmm150_intf_val)
    {
        I2C_TransferSeq_TypeDef transfer;
        transfer.addr        = (BMM150_DEFAULT_I2C_ADDRESS << 1);
        transfer.buf[0].data = &reg_addr;
        transfer.buf[0].len  = 1;
        transfer.buf[1].data = reg_data;
        transfer.buf[1].len  = length;
        transfer.flags       = I2C_FLAG_WRITE_READ;

        if (i2cTransferDone == board_i2c0_transfer(&transfer))
        {
            ret = BMM150_INTF_RET_SUCCESS;
        }
    }

    return ret;
}

static BMM150_INTF_RET_TYPE _bmm150_write(uint8_t reg_addr, const uint8_t *reg_data, uint32_t length, void *intf_ptr)
{
    (void)intf_ptr;
    BMM150_INTF_RET_TYPE ret = BMM150_E_COM_FAIL;

    if (intf_ptr == &bmm150_intf_val)
    {
        I2C_TransferSeq_TypeDef transfer;
        transfer.addr        = (BMM150_DEFAULT_I2C_ADDRESS << 1);
        transfer.buf[0].data = &reg_addr;
        transfer.buf[0].len  = 1;
        transfer.buf[1].data = reg_data;
        transfer.buf[1].len  = length;
        transfer.flags       = I2C_FLAG_WRITE_WRITE;

        if (i2cTransferDone == board_i2c0_transfer(&transfer))
        {
            ret = BMM150_INTF_RET_SUCCESS;
        }
    }
    return ret;
}

void board_bmm150_enable(void)
{
    board_bmm150.intf_ptr = &bmm150_intf_val;
    board_bmm150.intf     = BMM150_I2C_INTF;
    board_bmm150.delay_us = _bmm150_delay_us;
    board_bmm150.read     = _bmm150_read;
    board_bmm150.write    = _bmm150_write;

    GPIO_PinModeSet(VDD_BMM150_PORT, VDD_BMM150_PIN, VDD_BMM150_MODE, 1);
    BOARD_msDelay(10);

    /* Execute this to ensure that the slaves are re-inited correctly */
    I2CSPM_Init(&init_i2c0);
    NVIC_ClearPendingIRQ(I2C0_IRQn);
}
