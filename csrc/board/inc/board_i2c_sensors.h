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

/*
 * board_i2c_sensors.h
 *
 *  Created on: 27 nov 2022
 *      Author: Francisco
 */

#ifndef BOARD_I2C_SENSORS_H_
#define BOARD_I2C_SENSORS_H_

#include "bma2.h"
#include "bme280.h"
#include "bmg160.h"
#include "bmi160.h"
#include "bmm150.h"
#include "board.h"
#include "sl_i2cspm.h"

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
