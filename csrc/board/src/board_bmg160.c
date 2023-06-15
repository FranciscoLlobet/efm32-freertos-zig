/*
 * board_bmm160.c
 *
 *  Created on: 27 nov 2022
 *      Author: Francisco
 */
#include "board_i2c_sensors.h"

struct bmg160_t board_bmg160;

static void _bmg160_delay_msec(uint32_t period)
{
	BOARD_msDelay(period);
}

static s8 _bmg160_bus_write(u8 device_addr, u8 register_addr, u8 *register_data,
		u8 wr_len)
{
	s8 ret = -1;

	I2C_TransferSeq_TypeDef transfer;
	transfer.addr = (device_addr << 1);
	transfer.buf[0].data = &register_addr;
	transfer.buf[0].len = 1;
	transfer.buf[1].data = register_data;
	transfer.buf[1].len = wr_len;
	transfer.flags = I2C_FLAG_WRITE_WRITE;

	if (i2cTransferDone == board_i2c0_transfer(&transfer))
	{
		ret = C_BMG160_SUCCESS;
	}

	return ret;
}

static s8 _bmg160_burst_read(u8 device_addr, u8 register_addr,
		u8 *register_data, u16 rd_len)
{
	s8 ret = -1;

	I2C_TransferSeq_TypeDef transfer;
	transfer.addr = (device_addr << 1);
	transfer.buf[0].data = &register_addr;
	transfer.buf[0].len = 1;
	transfer.buf[1].data = register_data;
	transfer.buf[1].len = rd_len;
	transfer.flags = I2C_FLAG_WRITE_READ;

	if (i2cTransferDone == board_i2c0_transfer(&transfer))
	{
		ret = C_BMG160_SUCCESS;
	}

	return ret;
}

static s8 _bmg160_bus_read(u8 device_addr, u8 register_addr, u8 *register_data,
		u8 rd_len)
{
	s8 ret = -1;

	I2C_TransferSeq_TypeDef transfer;
	transfer.addr = (device_addr << 1);
	transfer.buf[0].data = &register_addr;
	transfer.buf[0].len = 1;
	transfer.buf[1].data = register_data;
	transfer.buf[1].len = rd_len;
	transfer.flags = I2C_FLAG_WRITE_READ;

	if (i2cTransferDone == board_i2c0_transfer(&transfer))
	{
		ret = C_BMG160_SUCCESS;
	}

	return ret;
}

void board_bmg160_enable(void)
{

	board_bmg160.dev_addr = BMG160_I2C_ADDR1;
	board_bmg160.delay_msec = _bmg160_delay_msec;
	board_bmg160.bus_write = _bmg160_bus_write;
	board_bmg160.burst_read = _bmg160_burst_read;
	board_bmg160.bus_read = _bmg160_bus_read;

	GPIO_PinModeSet(VDD_BMG160_PORT, VDD_BMG160_PIN, VDD_BMG160_MODE, 1);
	BOARD_msDelay(30);

	I2CSPM_Init(&init_i2c0);
	NVIC_ClearPendingIRQ(I2C0_IRQn);
}
