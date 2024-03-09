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
	miso_mbedtls_timing_delay_t * data = pvTimerGetTimerID( xTimer );


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


void miso_mbedtls_init_timer(miso_mbedtls_timing_delay_t * data)
{
	data->int_timer = xTimerCreate("inttimer", 1000, false, data, timer_callback );
	data->fin_timer = xTimerCreate("fintimer", 1000, false, data, timer_callback );
}

void miso_mbedtls_deinit_timer(miso_mbedtls_timing_delay_t * data)
{
	xTimerDelete(data->int_timer, portMAX_DELAY);
	xTimerDelete(data->fin_timer, portMAX_DELAY);
}

void miso_mbedtls_timing_set_delay( void *data, uint32_t int_ms, uint32_t fin_ms )
{
	miso_mbedtls_timing_delay_t * timer = (miso_mbedtls_timing_delay_t *)data;

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

int miso_mbedtls_timing_get_delay( void *data )
{
	miso_mbedtls_timing_delay_t * timer = (mbedtls_timing_delay_context *)data;

	return timer->state;
}
