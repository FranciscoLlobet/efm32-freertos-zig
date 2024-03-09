/*
 * Copyright (c) 2004 Francisco Llobet-Blandino and the "Miso Project".
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
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
void config_set_wifi_ssid(char *ssid);

// Set the WiFi key.
void config_set_wifi_key(char *key);

// Set the LWM2M server URI.
void config_set_lwm2m_uri(char *uri);

// Set the LWM2M server PSK ID.
void config_set_lwm2m_psk_id(char *id);

// Set the LWM2M server PSK key.
void config_set_lwm2m_psk_key(char *key);

// Set the LWM2M endpoint name.
void config_set_lwm2m_endpoint(char *name);

// Set the MQTT device ID.
void config_set_mqtt_device_id(char *id);

// Set the MQTT server URL.
void config_set_mqtt_url(char *url);

// Set the MQTT server PSK ID.
void config_set_mqtt_psk_id(char *id);

// Set the MQTT server PSK key.
void config_set_mqtt_psk_key(char *key);

// Set the HTTP server URI.
void config_set_http_uri(char *uri);

// Set the HTTP signature URI
void config_set_http_sig_uri(char *uri);

// Set the HTTP signature key
void config_set_http_sig_key(char *key);

// Set the config server URI.
void config_set_config_uri(char *uri);

char *const config_get_wifi_ssid(void);

char *const config_get_wifi_key(void);

char *const config_get_lwm2m_uri(void);

char *const config_get_lwm2m_psk_id(void);

char *const config_get_lwm2m_psk_key(void);

char *const config_get_lwm2m_endpoint(void);

char *const config_get_mqtt_device_id(void);

char *const config_get_mqtt_url(void);

char *const config_get_mqtt_psk_id(void);

char *const config_get_mqtt_psk_key(void);

char *const config_get_http_uri(void);

char *const config_get_http_sig_uri(void);

char *const config_get_http_sig_key(void);

char *const config_get_config_uri(void);

void miso_load_config(void);

#endif /* UISO_CONFIG_H_ */
