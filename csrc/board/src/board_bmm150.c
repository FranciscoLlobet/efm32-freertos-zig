/*
 * board_bmm150.c
 *
 *  Created on: 27 nov 2022
 *      Author: Francisco
 */
#include "board_i2c_sensors.h"

struct bmm150_dev board_bmm150;

uint32_t bmm150_intf_val = 0;

static void _bmm150_delay_us(uint32_t period, void *intf_ptr)
{
	(void) intf_ptr;
	BOARD_usDelay(period);
}

static BMM150_INTF_RET_TYPE _bmm150_read(uint8_t reg_addr, uint8_t *reg_data,
		uint32_t length, void *intf_ptr)
{
	(void) intf_ptr;
	BMM150_INTF_RET_TYPE ret = BMM150_E_COM_FAIL;

	I2C_TransferSeq_TypeDef transfer;
	transfer.addr = (BMM150_DEFAULT_I2C_ADDRESS << 1);
	transfer.buf[0].data = &reg_addr;
	transfer.buf[0].len = 1;
	transfer.buf[1].data = reg_data;
	transfer.buf[1].len = length;
	transfer.flags = I2C_FLAG_WRITE_READ;

	if (i2cTransferDone == board_i2c0_transfer(&transfer))
	{
		ret = BMM150_INTF_RET_SUCCESS;
	}

	return ret;
}

static BMM150_INTF_RET_TYPE _bmm150_write(uint8_t reg_addr,
		const uint8_t *reg_data, uint32_t length, void *intf_ptr)
{
	(void) intf_ptr;
	BMM150_INTF_RET_TYPE ret = BMM150_E_COM_FAIL;

	I2C_TransferSeq_TypeDef transfer;
	transfer.addr = (BMM150_DEFAULT_I2C_ADDRESS << 1);
	transfer.buf[0].data = &reg_addr;
	transfer.buf[0].len = 1;
	transfer.buf[1].data = reg_data;
	transfer.buf[1].len = length;
	transfer.flags = I2C_FLAG_WRITE_WRITE;

	if (i2cTransferDone == board_i2c0_transfer(&transfer))
	{
		ret = BMM150_INTF_RET_SUCCESS;
	}

	return ret;
}

void board_bmm150_enable(void)
{
	board_bmm150.intf_ptr = &bmm150_intf_val;
	board_bmm150.intf = BMM150_I2C_INTF;
	board_bmm150.delay_us = _bmm150_delay_us;
	board_bmm150.read = _bmm150_read;
	board_bmm150.write = _bmm150_write;

	GPIO_PinModeSet(VDD_BMM150_PORT, VDD_BMM150_PIN, VDD_BMM150_MODE, 1);
	BOARD_msDelay(10);

	/* Execute this to ensure that the slaves are re-inited correctly */
	I2CSPM_Init(&init_i2c0);
	NVIC_ClearPendingIRQ(I2C0_IRQn);
}
