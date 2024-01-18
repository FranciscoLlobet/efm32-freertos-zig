/**
 * miso_ntp.h
 *
 * Embedded implementation for sNTP client
 *
 * Implements RFC4330 (2006)
 * Simple Network Time Protocol (SNTP) Version 4 for IPv4, IPv6 and OSI
 * https://www.rfc-editor.org/rfc/rfc4330
 *
 * Also references SNTP v3
 * https://www.rfc-editor.org/rfc/rfc1769
 *
 *  Created on: 13 nov 2022
 *      Author: Francisco
 */

#ifndef UISO_NTP_H_
#define UISO_NTP_H_

#include "miso.h"


#define NTP_MIN_POLL_INTERVAL_MS	UINT32_C(16000) /**< Minimum poll interval */
#define NTP_MAX_POLL_INTERVAL_MS    UINT32_C(86400000) /**< Maximum poll interval 24h */

#define NTP_APPLICATION_START_DELAY_MS  UINT32_C(16000) /* Application start delay */

/**
 * NTP Packet definition
 */
struct ntp_packet_s
{
	uint8_t leap_version_mode; /* Leap-Version-Mode Byte*/
	uint8_t stratum; /* Stratum Byte */
	uint8_t poll_interval; /* Poll interval Byte */
	uint8_t precision; /* Precision Byte */
	uint8_t root_delay[4]; /* Root Delay (32-Bit) */
	uint8_t root_dispersion[4]; /* Root dispersion */
	uint8_t reference_indentifier[4];
	uint8_t reference_timestamp_seconds[4];
	uint8_t reference_timestamp_fraction[4];
	uint8_t originate_timestamp_seconds[4];
	uint8_t originate_timestamp_fraction[4];
	uint8_t recieve_timestamp_seconds[4];
	uint8_t recieve_timestamp_fraction[4];
	uint8_t transmit_timestamp_seconds[4];
	uint8_t transmit_timestamp_fraction[4];
	// uint8_t key_identifier[4]; /* Optional Key Identifier (v4) */
	// uint8_t message_digest[16]; /* Message Digest (v4) */
};



#define SNTP_LI0_BYTE_IDX	6
#define SNTP_LI1_BYTE_IDX   7
#define SNTP_LI_MASK ((1 << SNTP_LI0_BYTE_IDX)|(1<<SNTP_LI1_BYTE_IDX))

#define SNTP_VN0_BYTE_IDX   3
#define SNTP_VN1_BYTE_IDX   4
#define SNTP_VN2_BYTE_IDX   5

#define SNTP_VN_MASK ((1 << SNTP_VN0_BYTE_IDX) | (1 << SNTP_VN1_BYTE_IDX) | (1 << SNTP_VN2_BYTE_IDX))

#define SNTP_MODE0_BYTE_IDX 0
#define SNTP_MODE1_BYTE_IDX 1
#define SNTP_MODE2_BYTE_IDX 2

#define SNTP_MODE_MASK ((1 << SNTP_MODE0_BYTE_IDX) | (1 << SNTP_MODE1_BYTE_IDX) | (1 << SNTP_MODE2_BYTE_IDX))

enum{

	max_timer_interval_s = (UINT32_MAX - 1) / 1000, // Using 32-Bit timer
	max_timer_interval_ms = max_timer_interval_s * 1000,

	min_polling_interval_ms = (1 << 4) * 1024,
	max_polling_interval_ms = (1 << 17) * 1024,

	diff = max_timer_interval_ms - max_polling_interval_ms
};


enum sntp_li_e
{
	ntp_leap_indicator_no_warning = (0 << SNTP_LI0_BYTE_IDX) & SNTP_LI_MASK,
	ntp_leap_indicator_61s = (1 << SNTP_LI0_BYTE_IDX) & SNTP_LI_MASK,
	ntp_leap_indicator_59s = (2 << SNTP_LI0_BYTE_IDX) & SNTP_LI_MASK,
	ntp_leap_indicator_not_sync = (3 << SNTP_LI0_BYTE_IDX) & SNTP_LI_MASK,
};

enum sntp_vn_e
{
	sntp_version_0 = ((0 << SNTP_VN0_BYTE_IDX) & SNTP_VN_MASK),
	sntp_version_1 = ((1 << SNTP_VN0_BYTE_IDX) & SNTP_VN_MASK),
	sntp_version_2 = ((2 << SNTP_VN0_BYTE_IDX) & SNTP_VN_MASK),
	sntp_version_3 = ((3 << SNTP_VN0_BYTE_IDX) & SNTP_VN_MASK),
	sntp_version_4 = ((4 << SNTP_VN0_BYTE_IDX) & SNTP_VN_MASK),
};

enum sntp_mode_e
{
	ntp_mode_reserved = (0 << SNTP_MODE0_BYTE_IDX) & SNTP_MODE_MASK,
	ntp_mode_symmetric_active = (1 << SNTP_MODE0_BYTE_IDX) & SNTP_MODE_MASK,
	ntp_mode_symmetric_passive = (2 << SNTP_MODE0_BYTE_IDX) & SNTP_MODE_MASK,
	ntp_mode_symmetric_client = (3 << SNTP_MODE0_BYTE_IDX) & SNTP_MODE_MASK,
	ntp_mode_symmetric_server = (4 << SNTP_MODE0_BYTE_IDX) & SNTP_MODE_MASK,
	ntp_mode_symmetric_broadcast = (5 << SNTP_MODE0_BYTE_IDX) & SNTP_MODE_MASK,
	ntp_mode_symmetric_res_control = (6 << SNTP_MODE0_BYTE_IDX) & SNTP_MODE_MASK,
	ntp_mode_symmetric_res_private = (7 << SNTP_MODE0_BYTE_IDX) & SNTP_MODE_MASK,
};

enum{
	sntp_client_request_v3 = ntp_leap_indicator_no_warning | sntp_version_3 | ntp_mode_symmetric_client,
	sntp_client_request_v4 = ntp_leap_indicator_no_warning | sntp_version_4 | ntp_mode_symmetric_client,
	sntp_server_response_v3 =  (sntp_version_3 | ntp_mode_symmetric_server),
	sntp_server_response_v4 =  (sntp_version_4 | ntp_mode_symmetric_server)
};

enum sntp_stratum_e
{
	ntp_stratum_kod = 0, /* KISS-of-DEATH */
	ntp_stratum_primary = 1,
	ntp_stratum_secondary_2 = 2,
	ntp_stratum_secondary_3 = 3,
	ntp_stratum_secondary_4 = 4,
	ntp_stratum_secondary_5 = 5,
	ntp_stratum_secondary_6 = 6,
	ntp_stratum_secondary_7 = 7,
	ntp_stratum_secondary_8 = 8,
	ntp_stratum_secondary_9 = 9,
	ntp_stratum_secondary_10 = 10,
	ntp_stratum_secondary_11 = 11,
	ntp_stratum_secondary_12 = 12,
	ntp_stratum_secondary_13 = 12,
	ntp_stratum_secondary_14 = 14,
	ntp_stratum_secondary_15 = 15,
	ntp_stratum_reserved_start = 16,
	ntp_stratum_reserved_end = 255,
};

/**
 * ntp_poll_interval_16384s = 14,
 */
enum sntp_poll_interval_e
{
	ntp_poll_interval_16s = 4, /**< ntp_poll_interval_16s */
	ntp_poll_interval_32s = 5, /**< ntp_poll_interval_32s */
	ntp_poll_interval_64s = 6, /**< ntp_poll_interval_64s */
	ntp_poll_interval_128s = 7, /**< ntp_poll_interval_128s */
	ntp_poll_interval_256s = 8, /**< ntp_poll_interval_256s */
	ntp_poll_interval_512s = 9, /**< ntp_poll_interval_512s */
	ntp_poll_interval_1024s = 10, /**< ntp_poll_interval_1024s */
	ntp_poll_interval_2048s = 11, /**< ntp_poll_interval_2048s */
	ntp_poll_interval_4096s = 12, /**< ntp_poll_interval_4096s */
	ntp_poll_interval_8192s = 13, /**< ntp_poll_interval_8192s */
	ntp_poll_interval_16384s = 14, /**< ntp_poll_interval_16384s */
	ntp_poll_interval_32768s = 15, /**< ntp_poll_interval_32768s */
	ntp_poll_interval_65536s = 16, /**< ntp_poll_interval_65536s */
	ntp_poll_interval_131072s = 17, /**< ntp_poll_interval_131072s */
};

enum sntp_return_codes_e{
	sntp_success = (uint32_t)0,

	sntp_null_pointer,
	sntp_invalid_version,

	sntp_server_no_server,
	sntp_server_ip_dns_error, /* Server ip could not be resolved */
	sntp_server_ip_error, /* Error while sending or receiving packet */
	sntp_server_ip_mismatch, /* Server reply did not come from requested server */

	/* Reply errors */
	sntp_server_reply_header_error, /* Reply header error */
	sntp_server_reply_kod, /* Kiss-of-Death reply*/
	sntp_server_reply_invalid,
	sntp_server_transmit_zero,

	sntp_timestamp_format_error,

	/* Other errors */
	sntp_set_rtc_error,
	sntp_set_simplelink_error,

};

enum sntp_service_state
{
	sntp_not_synced = 0, /**< Not synced */
	sntp_synced = (1 << 0), /**< */
};

enum
{
	sntp_strategy_priority, /**< Priority based SNTP strategy */
	sntp_strategy_balancer, /**< Balanced SNTP query strategy */
};

struct sntp_server_s
{
	const char *server_name;
	uint32_t next_time_stamp;
	uint32_t last_time_stamp;
};

typedef struct sntp_server_s sntp_server_t;

struct ntp_parsed_response_s
{
	enum sntp_vn_e version;
	enum sntp_stratum_e stratum;

	uint32_t poll_interval;
};





enum sntp_return_codes_e miso_sntp_request(sntp_server_t* server, uint32_t * time_to_next_sync);

sntp_server_t* select_server_from_list(void);

uint32_t sntp_is_synced(void);



enum sntp_return_codes_e sntp_packet_create_request(struct ntp_packet_s *packet,
		uint32_t originate, uint32_t originate_frac, enum sntp_vn_e version);

#endif /* UISO_NTP_H_ */
