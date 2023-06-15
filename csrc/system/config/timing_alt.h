/*
 * timing_alt.h
 *
 *  Created on: 18 nov 2022
 *      Author: Francisco
 */

#ifndef INCLUDE_CONFIG_TIMING_ALT_H_
#define INCLUDE_CONFIG_TIMING_ALT_H_

#include "uiso.h"

struct uiso_mbedtls_timing_delay_s
{
		int32_t state;
		TimerHandle_t fin_timer;
		TimerHandle_t int_timer;
};

typedef struct uiso_mbedtls_timing_delay_s uiso_mbedtls_timing_delay_t;

typedef struct mbedtls_timing_delay_context
{
	uint32_t val;
} mbedtls_timing_delay_context;

void uiso_mbedtls_init_timer(uiso_mbedtls_timing_delay_t * data);
void uiso_mbedtls_deinit_timer(uiso_mbedtls_timing_delay_t * data);
void uiso_mbedtls_timing_set_delay( void *data, uint32_t int_ms, uint32_t fin_ms );
int uiso_mbedtls_timing_get_delay(void * data);

#endif /* INCLUDE_CONFIG_TIMING_ALT_H_ */
