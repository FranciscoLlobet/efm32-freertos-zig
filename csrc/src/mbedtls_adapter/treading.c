/*
 * uiso_treading.c
 *
 *  Created on: 19 nov 2022
 *      Author: Francisco
 */
#include "uiso.h"
#include "mbedtls/threading.h"

void uiso_mbedtls_mutex_init( mbedtls_threading_mutex_t * mutex )
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

void uiso_mbedtls_mutex_free( mbedtls_threading_mutex_t * mutex )
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

int uiso_mbedtls_mutex_lock( mbedtls_threading_mutex_t * mutex )
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


int uiso_mbedtls_mutex_unlock( mbedtls_threading_mutex_t * mutex )
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
