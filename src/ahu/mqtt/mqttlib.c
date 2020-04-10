#include <string.h>
#include "mqttlib.h"
#include "es_print.h"
#include "proc.h"

struct mosquitto *MqttAirCond;
bool clean_session = true;
bool unclean_session = false;

void  PublishWriteBack(struct json_object *jRoot)
{
	//add errno to json payload
	char topic[100];

	int deviceKey;
    deviceKey = GetIntValByKey(jRoot, "deviceKey");

	sprintf(topic, "%s/%d", MQTT_PUBTOPIC_PREFIX , deviceKey);

	const char *str = json_object_to_json_string_ext(jRoot, JSON_C_TO_STRING_PRETTY);
	mosquitto_publish(MqttAirCond, NULL, topic, strlen(str), str, 0, 0);
}

void SubMqttByDeviceKey(AppAHUDev_t *Dev)
{
	int i;
	char topic_ack[100];
	char topic_cmd[100];

	/* 订阅虚点相关："local/{gateway}/{虚点deviceKey}/command" */
	sprintf(topic_cmd, "%s/%d/command", MQTT_PUBTOPIC_PREFIX, Dev->deviceID);
	mosquitto_subscribe(MqttAirCond, NULL, topic_cmd, 0);

	/* 订阅实点相关，实点都是设备返回的值 "local/{gateway}/{实点deviceKey}" */
	for(i=Dev->lenVir; i<Dev->len; i++){
		sprintf(topic_ack, "%s/%u", MQTT_PUBTOPIC_PREFIX, (Dev->PointProp+i)->deviceKey);
//		ES_PRT_DEBUG("name = %s, deviceKey = %u \n", (Dev->PointProp+i)->name, (Dev->PointProp+i)->deviceKey);
		ES_PRT_DEBUG("topic ack = %s \n", topic_ack);
		mosquitto_subscribe(MqttAirCond, NULL, topic_ack, 0);
	}
}

/* mqtt publish 回调 */
void AirCondPubCb(struct mosquitto *mosq, void *userdata, int mid)
{
	ES_PRT_INFO("AirCond publish callback \n");	
}

/* mqtt subscribe 回调 */
void AirCondSubCb(struct mosquitto *mosq, void *userdata, int mid, int qos_count, const int *granted_qos)
{
//	printf("Subscribe callback, mid = %d \n", mid);
}

/* mqtt connect 回调 */
void AirCondConnCb(struct mosquitto *mosq, void *userdata, int rc)
{	//rc=0, 连接成功
	//mosquitto_connack_string()获取rc取值的含义
	if(!rc){
		ES_PRT_INFO("connect callback \n");
    }else{
        ES_PRT_INFO("Connect failed\n");
    }
}

void AirCondLogCb(struct mosquitto *mosq, void *userdata, int level, const char *str)
{
	ES_PRT_INFO("%s \n", str);
}

/* mqtt message回调 */
void AirCondMessCb(struct mosquitto *mosq, void *userdata, const struct mosquitto_message *message)
{
//	ES_PRT_DEBUG("message->topic = %s \n", (char *)message->topic);
	ES_PRT_DEBUG("message->payload = %s \n", (char *)message->payload);

	if(strstr((char *)message->topic, "command") != NULL){
    	MqttCmdMessProc((char *)message->payload, (char *)message->topic);
	}else{
		MqttMessProc((char *)message->payload);
	}

}