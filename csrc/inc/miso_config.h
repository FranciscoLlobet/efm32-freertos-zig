#ifndef UISO_CONFIG_H_
#define UISO_CONFIG_H_

#define JSMN_PARENT_LINKS 1

#include <stdio.h>
#include <string.h>

// Set the default configuration values. These values will be used if no configuration is found in flash.
void config_set_defaults(void);

// Load the configuration from flash.
void config_load(void);

// Write the configuration to flash.
void config_save(void);

// Set the WiFi SSID.
void config_set_wifi_ssid(char * ssid);

// Set the WiFi key.
void config_set_wifi_key(char * key);

// Set the LWM2M server URI.
void config_set_lwm2m_uri(char * uri);

// Set the LWM2M server PSK ID.
void config_set_lwm2m_psk_id(char * id);

// Set the LWM2M server PSK key.
void config_set_lwm2m_psk_key(char * key);

// Set the LWM2M endpoint name.
void config_set_lwm2m_endpoint(char * name);

// Set the MQTT device ID.
void config_set_mqtt_device_id(char * id);

// Set the MQTT server URL.
void config_set_mqtt_url(char * url);

// Set the MQTT server PSK ID.
void config_set_mqtt_psk_id(char * id);

// Set the MQTT server PSK key.
void config_set_mqtt_psk_key(char * key);

// Set the HTTP server URI.
void config_set_http_uri(char * uri);

// Set the config server URI.
void config_set_config_uri(char * uri);

void miso_load_config(void);

char * const config_get_wifi_ssid(void);

char * const config_get_wifi_key(void);

char * const config_get_lwm2m_uri(void);

char * const config_get_lwm2m_psk_id(void);

char * const config_get_lwm2m_psk_key(void);

char * const config_get_lwm2m_endpoint(void);

char * const config_get_mqtt_device_id(void);

char * const config_get_mqtt_url(void);

char * const config_get_mqtt_psk_id(void);

char * const config_get_mqtt_psk_key(void);

char * const config_get_http_uri(void);

char * const config_get_config_uri(void);

#endif /* UISO_CONFIG_H_ */
