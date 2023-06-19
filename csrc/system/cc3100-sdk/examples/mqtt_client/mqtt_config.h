/*
 * mqtt_config.h - MQTT Client configurations
 *
 * Copyright (C) 2014 Texas Instruments Incorporated - http://www.ti.com/
 *
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *
 *    Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 *    Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the
 *    distribution.
 *
 *    Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 *  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 *  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */
 
#ifndef __MQTT_CONFIG_H__
#define __MQTT_CONFIG_H__

//*****************************************************************************
//
// If building with a C++ compiler, make all of the definitions in this header
// have a C binding.
//
//*****************************************************************************
#ifdef __cplusplus
extern "C" {
#endif

/*Operate Lib in MQTT 3.1 mode.*/
#define MQTT_3_1_1              false /*MQTT 3.1.1 */
#define MQTT_3_1                true  /*MQTT 3.1 */

#define WILL_TOPIC              "Client"
#define WILL_MSG                "Client Stopped"
#define WILL_QOS                QOS2
#define WILL_RETAIN             false

/*Defining Broker IP address and port Number*/
#define SERVER_ADDRESS          "m2m.eclipse.org"
#define PORT_NUMBER             1883

/* Client ID must be unique*/
#define CLIENT_ID               "garfield007"
#define MAX_BROKER_CONN         1
#define SERVER_MODE             MQTT_3_1

/*Specifying Receive time out for the Receive task*/
#define RCV_TIMEOUT             30

/* Keep Alive Timer value in seconds.
 * Maximum time interval between messages received from a client.
 */
#define KEEP_ALIVE_TIMER        25

/* Clean session flag. The server must discard any previously
 * maintained information about the client.
 */
#define CLEAN_SESSION           true

/* Retain Flag. Used in publish message.
 * The server should hold on to the publish message after it has
 * been delivered to the current subscribers.
 */
#define RETAIN                  1

/* Defining Publish Topics */
#define PUB_TOPIC_1             "/cc3100/ButtonPressTopic1"
#define PUB_TOPIC_2             "/cc3100/ButtonPressTopic2"

/* Defining number of subscribe topics */
#define SUB_TOPIC_COUNT         2

/* Defining Subscription Topics */
#define SUB_TOPIC1              "/cc3100/ToggleLEDCmdL1"
#define SUB_TOPIC2              "/cc3100/ToggleLEDCmdL2"

/* Defining QOS levels */
#define QOS0                    0
#define QOS1                    1
#define QOS2                    2

/* Task priorities and OSI Stack Size */
#define OSI_STACK_SIZE          (1024)
#define SPAWN_TASK_PRIORITY     7
#define MQTT_APP_TASK_PRIORITY  2
#define TASK_PRIORITY           3

#define MAX_QUEUE_MSG           10

#define LOOPBACK_PORT_NUMBER    1882

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /*__MQTT_CONFIG_H__*/
