/*
 * uiso_entropy.c
 *
 *  Created on: 19 nov 2022
 *      Author: Francisco
 */
#include "uiso.h"

#include "mbedtls/ctr_drbg.h"
#include "mbedtls/entropy.h"

static int uiso_mbedtls_entropy_callback( void * ssl_context, unsigned char * output_buffer, size_t output_buffer_len )
{
    int ret = MBEDTLS_ERR_ENTROPY_SOURCE_FAILED;

    if(NULL == ssl_context)
    {
    	ret = MBEDTLS_ERR_ENTROPY_SOURCE_FAILED;
    }
    else if(NULL == output_buffer)
    {
    	ret = MBEDTLS_ERR_ENTROPY_SOURCE_FAILED;
    }

    // get entropy


    return ret;
}
