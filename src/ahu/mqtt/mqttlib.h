#ifndef _MQTTLIB_H_
#define _MQTTLIB_H_

#include <json-c/json_util.h>
#include <json-c/json.h>

#include <stdio.h>
#include <stdlib.h>
#include <mosquitto.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include "data.h"

#define ES_MQTT_HOST		"localhost"
//#define ES_MQTT_HOST		"192.168.37.232"
//#define ES_MQTT_HOST		"172.16.100.59"
#define ES_MQTT_PORT		(1883)
#define ES_MQTT_KEEPALIVE	(60)

#define MQTT_SUB_GW				"/local/1000/1000/command"
#define MQTT_PUBTOPIC_PREFIX	"/local/1000"

/* end */

void AirCondPubCb(struct mosquitto *mosq, void *userdata, int mid);

void AirCondSubCb(struct mosquitto *mosq, void *userdata, int mid, 
					int qos_count, const int *granted_qos);

void AirCondConnCb(struct mosquitto *mosq, void *userdata, int rc);

void AirCondMessCb(struct mosquitto *mosq, void *userdata, 
					const struct mosquitto_message *message);

void AirCondLogCb(struct mosquitto *mosq, void *userdata, 
					int level, const char *str);

void SubMqttByDeviceKey(AppAirCondDev_t *Dev);

void PublishWriteBack(struct json_object *jRoot);

#endif //_MQTTLIB_H_