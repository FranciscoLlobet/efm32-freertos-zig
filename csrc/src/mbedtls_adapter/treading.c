/*
 * miso_treading.c
 *
 *  Created on: 19 nov 2022
 *      Author: Francisco
 */
#include "miso.h"
#include "mbedtls/threading.h"

void miso_mbedtls_mutex_init( mbedtls_threading_mutex_t * mutex )
{
	if(NULL == mutex)
	{
		// Error
	}
	else
	{
	    mutex->mutex = xSemaphoreCreateMutex();

	    if( mutex->mutex != NULL )
	    {
	        mutex->is_valid = 1;
	    }
	    else
	    {
	        mutex->is_valid = 0;
	    }
	}
}

void miso_mbedtls_mutex_free( mbedtls_threading_mutex_t * mutex )
{
	if(NULL != mutex)
	{
	    if( mutex->is_valid == 1 )
	    {
	        vSemaphoreDelete( mutex->mutex );
	        mutex->is_valid = 0;
	    }
	}
}

int miso_mbedtls_mutex_lock( mbedtls_threading_mutex_t * mutex )
{
    int ret = MBEDTLS_ERR_THREADING_BAD_INPUT_DATA;

    if(NULL != mutex)
    {
        if( mutex->is_valid == 1 )
        {
            if( xSemaphoreTake( mutex->mutex, portMAX_DELAY ) )
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
        if( mutex->is_valid == 1 )
        {
            if( xSemaphoreGive( mutex->mutex ) )
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