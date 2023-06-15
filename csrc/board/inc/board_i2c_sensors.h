/*
 * board_i2c_sensors.h
 *
 *  Created on: 27 nov 2022
 *      Author: Francisco
 */

#ifndef BOARD_I2C_SENSORS_H_
#define BOARD_I2C_SENSORS_H_


#include "uiso.h"
#include "board.h"
#include "sl_i2cspm.h"

#include "bma2.h"
#include "bmm150.h"
#include "bme280.h"
#include "bmg160.h"
#include "bmi160.h"

extern I2CSPM_Init_TypeDef init_i2c0;

extern struct bma2_dev board_bma280;
extern struct bme280_dev board_bme280;
extern struct bmg160_t board_bmg160;
extern struct bmi160_dev board_bmi160;
extern struct bmm150_dev board_bmm150;

void board_i2c_init(void);

void board_bma280_enable(void);
void board_bme280_enable(void);
void board_bmg160_enable(void);
void board_bmi160_enable(void);
void board_bmm150_enable(void);

I2C_TransferReturn_TypeDef board_i2c0_transfer(I2C_TransferSeq_TypeDef *seq);

#endif /* BOARD_I2C_SENSORS_H_ */
