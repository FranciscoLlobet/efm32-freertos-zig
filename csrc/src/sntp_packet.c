/*
 * ntp_packet.c
 *
 *  Created on: 1 feb 2023
 *      Author: Francisco
 */

#include "miso_ntp.h"

enum sntp_return_codes_e sntp_packet_create_request(struct ntp_packet_s *packet,
		uint32_t originate, uint32_t originate_frac, enum sntp_vn_e version)
{
	enum sntp_return_codes_e rc = sntp_success;

	if (NULL == packet)
	{
		return sntp_null_pointer;
	}
	else
	{
		(void) memset(packet, (int) 0, sizeof(struct ntp_packet_s));
	}

	if (sntp_version_4 == version)
	{
		packet->leap_version_mode = (uint8_t) sntp_client_request_v4;

		/* Set the origin timestamp */
		*((uint32_t*) &(packet->transmit_timestamp_seconds[0])) = __REV(
				originate);

		/* Set the origin fractional timestamp */
		*((uint32_t*) &(packet->transmit_timestamp_fraction[0])) = __REV(
				originate_frac);

	}
	else
	{
		rc = sntp_invalid_version;
	}

	return rc;
}

