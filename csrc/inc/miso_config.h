#ifndef UISO_CONFIG_H_
#define UISO_CONFIG_H_

#define JSMN_PARENT_LINKS 1

#include <stdio.h>
#include <string.h>

void miso_load_config(void);

char* config_get_wifi_ssid(void);
char* config_get_wifi_key(void);
char* config_get_lwm2m_uri(void);
char* config_get_lwm2m_psk_id(void);
char* config_get_lwm2m_psk_key(void);

char * config_get_lwm2m_endpoint(void);

char * config_get_mqtt_device_id(void);
char * config_get_mqtt_url(void);

#endif /* UISO_CONFIG_H_ */
