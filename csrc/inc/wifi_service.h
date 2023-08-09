/*
 * wifi_service.h
 *
 *  Created on: 12 nov 2022
 *      Author: Francisco
 */

#ifndef WIFI_SERVICE_H_
#define WIFI_SERVICE_H_

#include "miso.h"

#include "network.h"

#include "FreeRTOS.h"
#include "task.h"
#include "timer.h"
#include "semphr.h"



int enqueue_select_rx(enum wifi_socket_id_e id, uint32_t timeout_s);
int enqueue_select_tx(enum wifi_socket_id_e id, uint32_t timeout_s);



void create_wifi_service_task(void);


#define MONITOR_MAX_RESPONSE_S    2

extern TaskHandle_t wifi_task_handle;
extern TaskHandle_t network_monitor_task_handle;
int create_network_mediator(void);

extern TaskHandle_t get_lwm2m_task_handle(void);
extern TaskHandle_t get_mqtt_task_handle(void);
extern TaskHandle_t get_http_task_handle(void);

#endif /* WIFI_SERVICE_H_ */
