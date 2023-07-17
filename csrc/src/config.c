/*
 * config.c
 *
 *  Created on: 15 dic 2022
 *      Author: Francisco
 */

#include "miso_config.h"
#include "FreeRTOS.h"
#include "task.h"
#include "ff.h"

#define JSMN_STATIC    1
#define JSMN_PARENT_LINKS    1
#include "jsmn.h"

char config_wifi_ssid[32];
char config_wifi_key[32];

char config_lwm2m_uri[128];
char config_lwm2m_psk_id[128];
char config_lwm2m_psk_key[64];

char config_ntp_url[64];
char config_sha256[32] = {0};

char* config_get_wifi_ssid(void)
{
	return (char*) &config_wifi_ssid[0];
}

char* config_get_wifi_key(void)
{
	return (char*) &config_wifi_key[0];
}

char* config_get_lwm2m_uri(void)
{
	return (char*) &config_lwm2m_uri[0];
}

char* config_get_lwm2m_psk_id(void)
{
	return (char*) &config_lwm2m_psk_id[0];
}

char* config_get_lwm2m_psk_key(void)
{
	return (char*) &config_lwm2m_psk_key[0];
}

FATFS fs;

//extern int calculate_sha256_of_file(const char *filename, uint8_t *hash);
//extern int verify_signature_of_file(const char *file, const char *sig_file, const char * pub_key_file);

void miso_load_config(void)
{
	FSIZE_t fSize = 0;
	FRESULT fRes = FR_OK;

	UINT fRead = 0;
	jsmn_parser parser;
	jsmntok_t *json_tokens = NULL;
	uint8_t *read_buffer = NULL;
	FIL file;
	jsmn_init(&parser);

	fRes = f_mount(&fs, "SD", 1);
	if (FR_OK == fRes)
	{
		fRes = f_open(&file, "SD:/CONFIG.TXT", FA_READ);
	}

	if (FR_OK == fRes)
	{
		fSize = f_size(&file);
		if (fSize > 0)
		{
			read_buffer = pvPortMalloc(fSize);
		}
	}

	if (FR_OK == fRes)
	{
		fRes = f_read(&file, read_buffer, f_size(&file), &fRead);
	}

	if (FR_OK == fRes)
	{

		int parse_result = 0;
		parse_result = jsmn_parse(&parser, (const char *)read_buffer, (size_t) fRead,
		NULL, 0);
		if (parse_result > 0)
		{
			json_tokens = pvPortMalloc(parse_result * sizeof(jsmntok_t));
		}

		jsmn_init(&parser);

		parse_result = jsmn_parse(&parser, (const char *)read_buffer, (size_t) fRead,
				json_tokens, parse_result);

		/* Find Wlan Key */
		int wifi_key = INT8_MIN;
		int lwm2m_key = INT8_MIN;
		int ntp_key = INT8_MIN;
		int lwm2m_psk_key = INT8_MIN;
		int ntp_url_key = INT8_MIN;
		int ntp_url_array = INT8_MIN;

		for (int i = 0; i < parse_result; i++)
		{
			char *json_key = (char*) read_buffer
					+ (size_t) json_tokens[i].start;
			if ((0 == json_tokens[i].parent)
					&& (json_tokens[i].type == JSMN_STRING))
			{
				if (0 == strncmp(json_key, "wifi", strlen("wifi")))
				{
					wifi_key = i;
				}
				else if (0 == strncmp(json_key, "lwm2m", strlen("lwm2m")))
				{
					lwm2m_key = i;
				}
				else if (0 == strncmp(json_key, "ntp", strlen("ntp")))
				{
					ntp_key = i;
				}
			}
			/* Access the wifi configuration */
			else if (((wifi_key + 1) == json_tokens[i].parent)
					&& (json_tokens[i].type == JSMN_STRING))
			{
				char *dest_ptr = NULL;
				size_t dest_len = 0;

				if (0 == strncmp(json_key, "ssid", strlen("ssid")))
				{
					dest_ptr = &config_wifi_ssid[0];
					dest_len = sizeof(config_wifi_key);
				}
				else if (0 == strncmp(json_key, "key", strlen("key")))
				{
					dest_ptr = &config_wifi_key[0];
					dest_len = sizeof(config_wifi_key);
				}

				if ((JSMN_STRING == json_tokens[i + 1].type)
						&& (dest_ptr != NULL))
				{
					char *src_ptr = (char*) read_buffer
							+ json_tokens[i + 1].start;
					size_t src_len = json_tokens[i + 1].end
							- json_tokens[i + 1].start;
					if (src_len > dest_len)
						src_len = dest_len;

					strncpy(dest_ptr, src_ptr, src_len);
				}
			}
			else if (((lwm2m_key + 1) == json_tokens[i].parent)
					&& (json_tokens[i].type == JSMN_STRING))
			{
				char *dest_ptr = NULL;
				size_t dest_len = 0;

				if (0 == strncmp(json_key, "uri", strlen("uri")))
				{
					dest_ptr = &config_lwm2m_uri[0];
					dest_len = sizeof(config_lwm2m_uri);
				}
				else if (0 == strncmp(json_key, "psk", strlen("psk")))
				{
					lwm2m_psk_key = i;
				}
				else if (0
						== strncmp(json_key, "bootstrap", strlen("bootstrap")))
				{
					// asm volatile ("bkpt 0");
				}

				if ((JSMN_STRING == json_tokens[i + 1].type)
						&& (dest_ptr != NULL))
				{
					char *src_ptr = (char*) read_buffer
							+ json_tokens[i + 1].start;
					size_t src_len = json_tokens[i + 1].end
							- json_tokens[i + 1].start;
					if (src_len > dest_len)
						src_len = dest_len;

					strncpy(dest_ptr, src_ptr, src_len);
				}
			}
			else if (((ntp_key + 1) == json_tokens[i].parent)
					&& (json_tokens[i].type == JSMN_STRING))
			{
				if(0 == strncmp(json_key, "url", strlen("url")))
				{
					ntp_url_key = i;
				}
			}
			else if(((ntp_url_key) == json_tokens[i].parent) && (json_tokens[i].type == JSMN_ARRAY))
			{
				ntp_url_array = i;


			}
			else if((ntp_url_array == json_tokens[i].parent) && (json_tokens[i].type == JSMN_STRING))
			{
				// Push NTP url
				size_t src_len = json_tokens[i].end - json_tokens[i].start;
				strncpy(&config_ntp_url[0], (char*)read_buffer + json_tokens[i].start, src_len );
			}
			else if (((lwm2m_psk_key + 1) == json_tokens[i].parent)
					&& (json_tokens[i].type == JSMN_STRING))
			{
				char *dest_ptr = NULL;
				size_t dest_len = 0;

				if (0 == strncmp(json_key, "id", strlen("id")))
				{
					dest_ptr = &config_lwm2m_psk_id[0];
					dest_len = sizeof(config_lwm2m_psk_id);
				}
				else if (0 == strncmp(json_key, "key", strlen("key")))
				{
					dest_ptr = &config_lwm2m_psk_key[0];
					dest_len = sizeof(config_lwm2m_psk_key);
				}


				if ((JSMN_STRING == json_tokens[i + 1].type)
						&& (dest_ptr != NULL))
				{
					char *src_ptr = (char*) read_buffer
							+ json_tokens[i + 1].start;
					size_t src_len = json_tokens[i + 1].end
							- json_tokens[i + 1].start;
					if (src_len > dest_len)
						src_len = dest_len;

					strncpy(dest_ptr, src_ptr, src_len);
				}
			}
		}
	}

	if (FR_OK == fRes)
	{
		fRes = f_close(&file);
	}
	else
	{
		(void) f_close(&file);
	}

	if(FR_OK == fRes)
	{
		//(void)verify_signature_of_file("SD:/CONFIG.TXT","SD:/CONFIG.b64","SD:/CONFIG.PUB");
	}

	if (FR_OK == fRes)
	{
		fRes = f_unmount("SD");
	}
	else
	{
		(void) f_unmount("SD");
	}

	if (NULL != json_tokens)
	{
		vPortFree(json_tokens);
	}

	if (NULL != read_buffer)
	{
		vPortFree(read_buffer);
	}
}
