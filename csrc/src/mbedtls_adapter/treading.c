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
#include "miso.h"
#include "mbedtls/threading.h"

void miso_mbedtls_mutex_init( mbedtls_threading_mutex_t * mutex )
{
	if(NULL != mutex)
	{
	    mutex->mutex = xSemaphoreCreateMutex();

	    if(mutex->mutex == NULL)
	    {
	        mutex->is_valid = pdFALSE;
	    }
	    else
	    {
	        mutex->is_valid = pdTRUE;
	    }
	}
}

void miso_mbedtls_mutex_free( mbedtls_threading_mutex_t * mutex )
{
	if(NULL != mutex)
	{
	    if( mutex->is_valid == pdTRUE )
	    {
	        vSemaphoreDelete( mutex->mutex );
	        mutex->is_valid = pdFALSE;
	    }
	}
}

int miso_mbedtls_mutex_lock( mbedtls_threading_mutex_t * mutex )
{
    int ret = MBEDTLS_ERR_THREADING_BAD_INPUT_DATA;

    if(NULL != mutex)
    {
        if( mutex->is_valid == pdTRUE )
        {
            if(pdPASS == xSemaphoreTake( mutex->mutex, portMAX_DELAY ))
            {
                ret = 0;
            }
            else
            {
                ret = MBEDTLS_ERR_THREADING_MUTEX_ERROR;
            }
        }
    }

    return ret;
}


int miso_mbedtls_mutex_unlock( mbedtls_threading_mutex_t * mutex )
{
    int ret = MBEDTLS_ERR_THREADING_BAD_INPUT_DATA;

    if(NULL != mutex)
    {
        if( mutex->is_valid == pdTRUE )
        {
            if(pdPASS == xSemaphoreGive( mutex->mutex ) )
            {
                ret = 0;
            }
            else
            {
                ret = MBEDTLS_ERR_THREADING_MUTEX_ERROR;
            }
        }
    }

    return ret;
}


void miso_mbedtls_set_treading_alt(void)
{
    mbedtls_threading_set_alt(miso_mbedtls_mutex_init,miso_mbedtls_mutex_free,miso_mbedtls_mutex_lock,miso_mbedtls_mutex_unlock);
}