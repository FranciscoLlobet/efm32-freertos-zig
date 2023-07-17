/*
 * miso_gmttime.c
 *
 *  Created on: 19 nov 2022
 *      Author: Francisco
 */


#include "miso.h"
#include <time.h>
#include "mbedtls/timing.h"
#include "mbedtls/threading.h"
#include "mbedtls/platform_time.h"

#include "sl_sleeptimer.h"

struct tm *mbedtls_platform_gmtime_r( const mbedtls_time_t *tt,
                                      struct tm *tm_buf )
{
	sl_sleeptimer_date_t date;
	if(NULL == tt)
	{
		return (struct tm*)NULL;
	}

    sl_sleeptimer_timestamp_t time_stamp = 0;

    (void)sl_sleeptimer_convert_time_to_date((sl_sleeptimer_timestamp_t)*tt, (sl_sleeptimer_time_zone_offset_t)0,&date);

	tm_buf->tm_wday = date.day_of_week;
    tm_buf->tm_yday = date.day_of_year;
    tm_buf->tm_mon = date.month;
	tm_buf->tm_hour = date.hour;
	tm_buf->tm_min = date.min;
	tm_buf->tm_mday = date.month_day;
	tm_buf->tm_sec = date.sec;
	tm_buf->tm_year = date.year;

    return tm_buf;
}
