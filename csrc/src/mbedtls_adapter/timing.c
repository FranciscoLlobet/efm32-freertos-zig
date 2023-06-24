/*
 * uiso_timing.c
 *
 *  Created on: 18 nov 2022
 *      Author: Francisco
 */

#include "mbedtls/timing.h"
#include "timing_alt.h"


enum{
	timer_cancelled = -1,
	timer_normal = 0,
	timer_intermediate_passed = 1,
	timer_final_delay_passed = 2
};


static void timer_callback( TimerHandle_t xTimer )
{
	uiso_mbedtls_timing_delay_t * data = pvTimerGetTimerID( xTimer );


	if( xTimer == data->int_timer)
	{
		data->state = (int32_t)timer_intermediate_passed;
	}
	else if(xTimer == data->fin_timer)
	{
		data->state = (int32_t)timer_final_delay_passed;
	}
	else
	{
		data->state = (int32_t)timer_cancelled;
	}

}


void uiso_mbedtls_init_timer(uiso_mbedtls_timing_delay_t * data)
{
	data->int_timer = xTimerCreate("inttimer", 1000, false, data, timer_callback );
	data->fin_timer = xTimerCreate("fintimer", 1000, false, data, timer_callback );
}

void uiso_mbedtls_deinit_timer(uiso_mbedtls_timing_delay_t * data)
{
	xTimerDelete(data->int_timer, portMAX_DELAY);
	xTimerDelete(data->fin_timer, portMAX_DELAY);
}

void uiso_mbedtls_timing_set_delay( void *data, uint32_t int_ms, uint32_t fin_ms )
{
	uiso_mbedtls_timing_delay_t * timer = (uiso_mbedtls_timing_delay_t *)data;

	if(pdTRUE == xTimerIsTimerActive(timer->int_timer))
	{
		xTimerStop(timer->int_timer, portMAX_DELAY);
	}
	if(pdTRUE == xTimerIsTimerActive(timer->fin_timer))
	{
		xTimerStop(timer->fin_timer, portMAX_DELAY);
	}

	if(int_ms > 0)
	{
		xTimerChangePeriod(timer->int_timer, int_ms, portMAX_DELAY);
	}

	if(fin_ms > 0)
	{
		xTimerChangePeriod(timer->fin_timer, int_ms, portMAX_DELAY);
	}

	if(fin_ms == 0)
	{
		timer->state = (int32_t)timer_cancelled;
	}
	else
	{
		timer->state = timer_normal;
	}
}

int uiso_mbedtls_timing_get_delay( void *data )
{
	uiso_mbedtls_timing_delay_t * timer = (mbedtls_timing_delay_context *)data;

	return timer->state;
}
