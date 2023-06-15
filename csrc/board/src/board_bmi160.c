/*
 * board_bmi160.c
 *
 *  Created on: 27 nov 2022
 *      Author: Francisco
 */

#include "board_i2c_sensors.h"

struct bmi160_dev board_bmi160;

static int8_t _bmi160_read(uint8_t dev_addr, uint8_t reg_addr,
		uint8_t *read_data, uint16_t len)
{
	BMA2_INTF_RET_TYPE ret = BMA2_E_COM_FAIL;

	I2C_TransferSeq_TypeDef transfer;
	transfer.addr = (BMI160_I2C_ADDR << 1);
	transfer.buf[0].data = &reg_addr;
	transfer.buf[0].len = 1;
	transfer.buf[1].data = read_data;
	transfer.buf[1].len = len;
	transfer.flags = I2C_FLAG_WRITE_READ;

	if (i2cTransferDone == board_i2c0_transfer(&transfer))
	{
		ret = BMA2_OK;
	}

	return ret;
}
static int8_t _bmi160_write(uint8_t dev_addr, uint8_t reg_addr,
		uint8_t *write_data, uint16_t len)
{
	int8_t ret = BMA2_E_COM_FAIL;

	I2C_TransferSeq_TypeDef transfer;
	transfer.addr = (BMI160_I2C_ADDR << 1);
	transfer.buf[0].data = &reg_addr;
	transfer.buf[0].len = 1;
	transfer.buf[1].data = write_data;
	transfer.buf[1].len = len;
	transfer.flags = I2C_FLAG_WRITE_WRITE;

	if (i2cTransferDone == board_i2c0_transfer(&transfer))
	{
		ret = BMA2_OK;
	}

	return ret;
}

static void _bmi160_delay_ms(uint32_t period)
{
	BOARD_msDelay(period);
}

void board_bmi160_enable(void)
{
	board_bmi160.intf = BMI160_I2C_INTF;
	board_bmi160.read = _bmi160_read;
	board_bmi160.write = _bmi160_write;
	board_bmi160.delay_ms = _bmi160_delay_ms;

	/* Enable BMI160 */
	GPIO_PinModeSet(VDD_BMI160_PORT, VDD_BMI160_PIN, VDD_BMI160_MODE, 1);
	BOARD_msDelay(10);

	/* Enable BMA280 INT1 and INT2 */
	GPIO_PinOutSet(BMI160_INT1_PORT, BMI160_INT1_PIN);
	GPIO_PinOutSet(BMI160_INT2_PORT, BMI160_INT2_PIN);

	GPIO_ExtIntConfig(BMI160_INT1_PORT,
	BMI160_INT1_PIN,
	BMI160_INT1_PIN,
	true,
	true,
	false);

	/* Execute this to ensure that the slaves are re-inited correctly */
	I2CSPM_Init(&init_i2c0);
	NVIC_ClearPendingIRQ(I2C0_IRQn);
}
