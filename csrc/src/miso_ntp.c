/*
 * miso_ntp.c
 *
 *  Created on: 14 nov 2022
 *      Author: Francisco
 */

#include "miso.h"

#include "miso_ntp.h"

//#include "simplelink.h"
#include "wifi_service.h"
#include "network.h"

static struct ntp_packet_s ntp_packet;

sntp_server_t sntp_server_list[] =
{
		{ .server_name = "0.de.pool.ntp.org", .next_time_stamp = 0,
				.last_time_stamp = 0, },
		{ .server_name = "1.de.pool.ntp.org", .next_time_stamp = 0,
				.last_time_stamp = 0, },
		{ .server_name = "2.de.pool.ntp.org", .next_time_stamp = 0,
				.last_time_stamp = 0, },
		{ .server_name = "3.de.pool.ntp.org", .next_time_stamp = 0,
				.last_time_stamp = 0, },
		{ .server_name = "time1.google.com", .next_time_stamp = 0,
				.last_time_stamp = 0, },
		{ .server_name = "time2.google.com", .next_time_stamp = 0,
				.last_time_stamp = 0, },
		{ .server_name = "time3.google.com", .next_time_stamp = 0,
				.last_time_stamp = 0, },
		{ .server_name = "time4.google.com", .next_time_stamp = 0,
				.last_time_stamp = 0, },
		{ .server_name = NULL, .next_time_stamp = 0, .last_time_stamp = 0, }, };

uint32_t sntp_sync_state = 0;

uint32_t sntp_is_synced(void)
{
	return sntp_sync_state;
}

sntp_server_t* select_server_from_list(void)
{
	return &sntp_server_list[0];
}

#include "sl_sleeptimer.h"

const char *const ntp_kiss_codes[] =
{ "ACTS", "AUTH", "AUTO", "BCSR", "CRYP", "DENY", "DROP", "RSTR", "INIT",
		"MCST", "NKEY", "RATE", "RMOT", "STEP" };

static uint32_t get_local_ntp_time(void)
{
	uint32_t local_time = (uint32_t) 0;

	if (SL_STATUS_OK
			!= sl_sleeptimer_convert_unix_time_to_ntp(sl_sleeptimer_get_time(),
					&local_time))
	{
		local_time = 0;
	}

	return local_time;
}

enum sntp_return_codes_e miso_sntp_request(sntp_server_t *server,
		uint32_t *time_to_next_sync)
{
	sl_sleeptimer_date_t date =
	{ 0 };
	sl_sleeptimer_timestamp_t time_stamp = 0; // Unix timestamp
	sl_status_t sl_status = SL_STATUS_OK;
	_i16 dnsStatus = -1;
	_i16 sd = -1;
	int res = -1;
	enum sntp_return_codes_e sntp_rcode = sntp_success;

	if (NULL == server)
	{
		return sntp_server_no_server;
	}
	else if (NULL == time_to_next_sync)
	{
		return sntp_server_no_server;
	}

	uint32_t local_origin_timestamp = 0;
	uint32_t local_origin_timestamp_fraction = 0;

	res = miso_create_network_connection(miso_get_network_ctx(wifi_service_ntp_socket), server->server_name, strlen(server->server_name), (uint16_t)123, (uint16_t)123, miso_protocol_udp_ip4);

	if (sntp_success == sntp_rcode)
	{
		if (res >= 0)
		{
			local_origin_timestamp = get_local_ntp_time();
			(void) sntp_packet_create_request(&ntp_packet,
					local_origin_timestamp, local_origin_timestamp_fraction,
					sntp_version_4);

			res =  miso_network_send(miso_get_network_ctx(wifi_service_ntp_socket), (uint8_t *)&ntp_packet, sizeof(ntp_packet));
		}

		// select?
		if(res >= 0)
		{
			res = enqueue_select_rx(wifi_service_ntp_socket, 2);
		}

		/* Wait for response */
		if (res >= 0)
		{
			res = miso_network_read(miso_get_network_ctx(wifi_service_ntp_socket), (uint8_t *)&ntp_packet, sizeof(ntp_packet));
		}

		if(res >= 0)
		{
			res = miso_close_network_connection(miso_get_network_ctx(wifi_service_ntp_socket));
		}
		else
		{
			(void) miso_close_network_connection(miso_get_network_ctx(wifi_service_ntp_socket));
		}

		if (res < 0)
		{
			sntp_rcode = sntp_server_ip_error;
		}
	}

	/* Validate Server IP address*/
	if (sntp_success == sntp_rcode)
	{
		//if ((ntp_recv_addr.sin_addr.s_addr != ntp_srv_addr.sin_addr.s_addr))
		//{
		//	sntp_rcode = sntp_server_ip_mismatch;
		//}
	}

	/* Decode and validate received packet */
	if (sntp_success == sntp_rcode)
	{
		if ((uint8_t) sntp_server_response_v4 == ntp_packet.leap_version_mode)
		{
			// Decode packet as v4
		}
		else if ((uint8_t) sntp_server_response_v3
				== ntp_packet.leap_version_mode)
		{
			// Decode packet as v3
		}
		else
		{
			sntp_rcode = sntp_server_reply_header_error;
		}
	}

	if (sntp_success == sntp_rcode)
	{
		if ((uint32_t) ntp_stratum_kod == ntp_packet.stratum)
		{
			sntp_rcode = sntp_server_reply_kod;
		}
	}

	if (sntp_success == sntp_rcode)
	{
		if (((uint32_t) 0 == *(uint32_t*) &ntp_packet.transmit_timestamp_seconds[0]) && ((uint32_t) 0 == *(uint32_t*) &ntp_packet.transmit_timestamp_fraction[0]))
		{
			sntp_rcode = sntp_server_transmit_zero;
		}
	}

	if (sntp_success == sntp_rcode)
	{
		uint32_t server_timestamp = __REV(
				*(uint32_t*) &(ntp_packet.recieve_timestamp_seconds[0]));
		uint32_t server_originate_timestamp = __REV(
				*(uint32_t*) &(ntp_packet.originate_timestamp_seconds[0]));
		uint32_t server_originate_timestamp_fraction = __REV(
				*(uint32_t*) &(ntp_packet.originate_timestamp_fraction[0]));

		uint32_t server_poll_interval = ntp_packet.poll_interval;

		/* Set comparison */
		if (server_originate_timestamp != (uint32_t) local_origin_timestamp)
		{
			sntp_rcode = sntp_server_reply_invalid;
		}
		else if (server_originate_timestamp_fraction
				!= (uint32_t) local_origin_timestamp_fraction)
		{
			sntp_rcode = sntp_server_reply_invalid;
		}

		if (sntp_success == sntp_rcode)
		{
			if (SL_STATUS_OK
					!= sl_sleeptimer_convert_ntp_time_to_unix(server_timestamp,
							&time_stamp))
			{
				sntp_rcode = sntp_timestamp_format_error;
			}
		}

		if (sntp_success == sntp_rcode)
		{
			if (SL_STATUS_OK != sl_sleeptimer_set_time(time_stamp))
			{
				sntp_rcode = sntp_set_rtc_error;
			}
		}

		if (sntp_success == sntp_rcode)
		{
			if (SL_STATUS_OK == sl_sleeptimer_get_datetime(&date))
			{
				SlDateTime_t dateTime =
				{ 0 };

				/* Convert to SimpleLink format */
				dateTime.sl_tm_day = (_u32) date.month_day;
				dateTime.sl_tm_mon = (_u32) (date.month + 1);
				dateTime.sl_tm_year = (_u32) date.year + (_u32) 1900;
				dateTime.sl_tm_hour = (_u32) date.hour;
				dateTime.sl_tm_min = (_u32) date.min;
				dateTime.sl_tm_sec = (_u32) date.sec;

				res = sl_DevSet(SL_DEVICE_GENERAL_CONFIGURATION,
				SL_DEVICE_GENERAL_CONFIGURATION_DATE_TIME, sizeof(SlDateTime_t),
						(_u8*) (&dateTime));
				if (res < 0)
				{
					sntp_rcode = sntp_set_simplelink_error;
				}
			}
			else
			{
				sntp_rcode = sntp_set_rtc_error;
			}
		}

		if (sntp_success == sntp_rcode)
		{
			if (server_poll_interval < (uint32_t) ntp_poll_interval_16s)
			{
				server_poll_interval = (uint32_t) ntp_poll_interval_16s;
			}
			else if (server_poll_interval
					> (uint32_t) ntp_poll_interval_131072s)
			{
				server_poll_interval = (uint32_t) ntp_poll_interval_131072s;
			}

			server->last_time_stamp = time_stamp;
			server->next_time_stamp = time_stamp
					+ (uint32_t) (1 << server_poll_interval);

			*time_to_next_sync = (1 << server_poll_interval) * (uint32_t) 1000;
		}
		else
		{
			server->next_time_stamp = UINT32_MAX;
			*time_to_next_sync = 0;
		}
	}

	if (sntp_success == sntp_rcode)
	{
		sntp_sync_state = (uint32_t)time_stamp;
	}
	else
	{
		sntp_sync_state = 0;
	}

	return sntp_rcode;
}

