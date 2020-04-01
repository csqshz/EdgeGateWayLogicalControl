#include "app.h"
#include "mqttlib.h"
#include "es_print.h"
#include "proc.h"
#include "thread_signal.h"
#include "list.h"

ThreadSignal_t TS_WaitPlat;	// 空调启动状态条件变量

extern struct mosquitto *MqttAirCond;
extern bool clean_session;
extern bool unclean_session;

extern AppAirCondDev_l *AirCondList_head;

int main(int argc, const char *argv[])
{
	AirCondList_head = InitAirCondListHead();
	/* 1. mqtt */
	mosquitto_lib_init();
	MqttAirCond = mosquitto_new(NULL, clean_session, NULL);
	if(!MqttAirCond){
        ES_PRT_ERROR("create mqtt client failed.. \n");
		goto mqttclean;
    }

	mosquitto_subscribe_callback_set(MqttAirCond, AirCondSubCb);
	mosquitto_connect_callback_set(MqttAirCond, AirCondConnCb);
	mosquitto_message_callback_set(MqttAirCond, AirCondMessCb);
//	mosquitto_log_callback_set(mosq, AirCondLogCb);

	if(MOSQ_ERR_SUCCESS != mosquitto_connect(MqttAirCond, ES_MQTT_HOST, ES_MQTT_PORT, ES_MQTT_KEEPALIVE)){
		perror("Unable to connect.\n");
		goto mqttdestroy;
		return 1;
	}
	mosquitto_loop_start(MqttAirCond);

	/*2. 条件变量 */
	ThreadSignal_Init(&TS_WaitPlat, 0);

	/* 3订阅 */
	mosquitto_subscribe(MqttAirCond, NULL, MQTT_SUB_GW, 0);

	/* 4. get 实例app */
	AddDevFromLocal();
#if 1
	/* 5. create 控制线程 */
	pthread_t			tid_runstate;		 
	pthread_t			tid_temp;			// 温度pthread id
	pthread_t			tid_hmdt;			// 湿度pthread id
	pthread_t			tid_co2;			// 二氧化碳pthread id
	pthread_t 			tid_rt;				// 运行时间

	//启停控制状态transfer
	pthread_create(&tid_runstate, NULL, AirCondCtrlStart_thread, NULL);
	pthread_detach(tid_runstate);
	//温控
	pthread_create(&tid_temp, NULL, AirCondCtrlTemp_thread, NULL);
	pthread_detach(tid_temp);
	//湿控
	pthread_create(&tid_hmdt, NULL, AirCondCtrlHmdt_thread, NULL);
	pthread_detach(tid_hmdt);
	//Co2控
	pthread_create(&tid_co2, NULL, AirCondCtrlCo2_thread, NULL);
	pthread_detach(tid_co2);
	//运行计时
	pthread_create(&tid_rt, NULL, AirCondCtrlRt_thread, NULL);
	pthread_detach(tid_rt);
#endif
	pause();
	return 0;

mqttdestroy:
	mosquitto_destroy(MqttAirCond);
mqttclean:
	mosquitto_lib_cleanup();

	return 1;
}