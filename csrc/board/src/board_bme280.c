/*
 * board_bme280.c
 *
 *  Created on: 27 nov 2022
 *      Author: Francisco
 */
#include "board_i2c_sensors.h"

struct bme280_dev board_bme280;
static uint32_t dummy_val = 0;

static void _bme280_delay_us(uint32_t period, void *intf_ptr)
{
	(void) intf_ptr;
	BOARD_usDelay(period);
}

static BME280_INTF_RET_TYPE _bme280_read(uint8_t reg_addr, uint8_t *reg_data,
		uint32_t len, void *intf_ptr)
{
	(void) intf_ptr;
	BME280_INTF_RET_TYPE ret = BME280_E_COMM_FAIL;

	I2C_TransferSeq_TypeDef transfer;
	transfer.addr = (BME280_I2C_ADDR_PRIM << 1);
	transfer.buf[0].data = &reg_addr;
	transfer.buf[0].len = 1;
	transfer.buf[1].data = reg_data;
	transfer.buf[1].len = len;
	transfer.flags = I2C_FLAG_WRITE_READ;

	if (i2cTransferDone == board_i2c0_transfer(&transfer))
	{
		ret = BME280_INTF_RET_SUCCESS;
	}

	return ret;
}

static BME280_INTF_RET_TYPE _bme280_write(uint8_t reg_addr,
		const uint8_t *reg_data, uint32_t len, void *intf_ptr)
{
	(void) intf_ptr;
	BME280_INTF_RET_TYPE ret = BME280_E_COMM_FAIL;

	I2C_TransferSeq_TypeDef transfer;
	transfer.addr = (BME280_I2C_ADDR_PRIM << 1);
	transfer.buf[0].data = &reg_addr;
	transfer.buf[0].len = 1;
	transfer.buf[1].data = reg_data;
	transfer.buf[1].len = len;
	transfer.flags = I2C_FLAG_WRITE_WRITE;

	if (i2cTransferDone == board_i2c0_transfer(&transfer))
	{
		ret = BME280_INTF_RET_SUCCESS;
	}

	return ret;
}

void board_bme280_enable(void)
{
	board_bme280.intf_ptr = &dummy_val;
	board_bme280.intf = BME280_I2C_INTF;
	board_bme280.delay_us = _bme280_delay_us;
	board_bme280.read = _bme280_read;
	board_bme280.write = _bme280_write;

	GPIO_PinModeSet(VDD_BME280_PORT, VDD_BME280_PIN, VDD_BME280_MODE, 1);
	BOARD_msDelay(10);

	I2CSPM_Init(&init_i2c0);
	NVIC_ClearPendingIRQ(I2C0_IRQn);
}
