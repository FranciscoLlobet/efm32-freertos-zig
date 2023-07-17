/*
 * timing_alt.h
 *
 *  Created on: 18 nov 2022
 *      Author: Francisco
 */

#ifndef INCLUDE_CONFIG_TIMING_ALT_H_
#define INCLUDE_CONFIG_TIMING_ALT_H_

#include "miso.h"

struct miso_mbedtls_timing_delay_s
{
		int32_t state;
		TimerHandle_t fin_timer;
		TimerHandle_t int_timer;
};

typedef struct miso_mbedtls_timing_delay_s miso_mbedtls_timing_delay_t;

typedef struct mbedtls_timing_delay_context
{
	uint32_t val;
} mbedtls_timing_delay_context;

void miso_mbedtls_init_timer(miso_mbedtls_timing_delay_t * data);
void miso_mbedtls_deinit_timer(miso_mbedtls_timing_delay_t * data);
void miso_mbedtls_timing_set_delay( void *data, uint32_t int_ms, uint32_t fin_ms );
int miso_mbedtls_timing_get_delay(void * data);

#endif /* INCLUDE_CONFIG_TIMING_ALT_H_ */
