/*
 * mqtt_task.h
 *
 *  Created on: 2020. 5. 19.
 *      Author: jihoo
 */

#ifndef MQTT_TASK_H_
#define MQTT_TASK_H_

#include "MQTTClient.h"
#include "MQTTInterface.h"
#include "cmsis_os.h"

#include <string.h>

#define MQTT_BUFSIZE	1024

Network net; //mqtt network
MQTTClient mqttClient; //mqtt client

uint8_t sndBuffer[MQTT_BUFSIZE]; //mqtt send buffer
uint8_t rcvBuffer[MQTT_BUFSIZE]; //mqtt receive buffer
uint8_t msgBuffer[MQTT_BUFSIZE]; //mqtt message buffer

void MqttClientSubTask(void const *argument);
void MqttClientPubTask(void const *argument);
int  MqttConnectBroker(void);
void MqttMessageArrived(MessageData* msg);
void StartT4ADC(void const * argument);
void StartT3Btn(void const * argument);
//TODO probati da prebacis getMPPT u poseban task koji ce da se runuje sam za sebe

#endif /* MQTT_TASK_H_ */
