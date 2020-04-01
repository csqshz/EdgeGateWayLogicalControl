#include <dirent.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include "proc.h"
#include "es_print.h"
#include "mqttlib.h"
#include "list.h"
#include "app.h"
#include "thread_signal.h"
#include "data.h"
#include "ahupid_func.h"

extern ThreadSignal_t TS_WaitPlat;	// 空调启动状态条件变量
extern struct mosquitto *MqttAirCond;

extern AppAirCondDev_l *AirCondList_head;
extern PointProp_t ppinit;

/* 发给下层的json datagram template,用于向设备write/read点位值

{
	"deviceKey": "011",
	"cmd": "write",
	"function": {
		"channel1-value": "1"
	}
}

*/
char *AirApp2Low = "{\"deviceKey\":\"011\",\"cmd\":\"write\",\"function\":{\"realpoint_key\":\"1\"}}";

/* end */

/* 根据template，将prop內部成员的值替换template中对应的成员。 
 * 替换的内容有：deviceKey，cmd，function
 * 
 * @ prop：点位PointProp成员
 * @ oper：操作指令，read/write
 * @ template：payload模板
 */
/* 基于json报文模板，赋值deviceKey、value、function，发给下层 */
struct json_object *CombineJson2Low(PointProp_t *prop, enum CmdOper oper, char *template)
{
    struct json_object *jObj;

    /* convert string to json object */
    jObj = json_tokener_parse(template);

    char deviceKeyVal[30] = {0};
    sprintf(deviceKeyVal, "%d", prop->deviceKey);

    char cmdVal[20] = {0};
    switch (oper){
        case CMD_WRITE:
            strcpy(cmdVal, "write");   break;
        case CMD_READ:
            strcpy(cmdVal, "read");   break;
        default:
            break;
    }

    /* modify deviceKey's value */
    json_object_object_del(jObj, "deviceKey");
    json_object_object_add(jObj, "deviceKey", json_object_new_string(deviceKeyVal));

    /* modify cmd's value */
    json_object_object_del(jObj, "cmd");
    json_object_object_add(jObj, "cmd", json_object_new_string(cmdVal));

    /* modify 虚点的function */
    struct json_object *jfunc;
    jfunc = json_object_object_get(jObj, "function");

    char realpointVal[30] = {0};

    if(prop->tag == TypeOfVal_INT){
        sprintf(realpointVal, "%d", prop->Val.valI);
    }else if(prop->tag == TypeOfVal_DOUBLE){
        sprintf(realpointVal, "%lf", prop->Val.valD);
    }

    json_object_object_del(jfunc, "realpoint_key");
    json_object_object_add(jfunc, prop->func, json_object_new_string(realpointVal));

    return jObj;
    
}

/* 通过mqtt publish向下层发送报文。 
 * 1. 先判断点位是否配置，没有配置的就不发送,判断依据是deviceKey!=INVALID_DEVICEKEY
 * 2. 修改templ指向的json报文成员，作为payload。
 * 
 * @ name：点位名字
 * @ Dev：设备实例
 * @ oper：操作指令，read/write
 * @ template：payload模板
 */
void SendCmd2Low(char *name, AppAirCondDev_t *Dev, enum CmdOper oper, char *template)
{
    /* 目前没有read操作 */
    if(oper == CMD_READ){
        return;
    }

    int i;
    for (i=0; i<Dev->len; i++){
        /* 只有点位存在才发送 */
        if(strcmp((Dev->PointProp+i)->name, name) == 0){
            /* 重新组合payload */
            struct json_object *jObj = CombineJson2Low((Dev->PointProp+i), oper, template);

            /* 组装topic：/local/gatewayID/deviceKey */
            char topic[100];
            sprintf(topic, "%s/%d/command", MQTT_PUBTOPIC_PREFIX, (Dev->PointProp+i)->deviceKey);
            /* convert json object to string */
            const char *payload = json_object_to_json_string_ext(jObj, JSON_C_TO_STRING_PRETTY);
            ES_PRT_DEBUG(" %s \n", payload);

            mosquitto_publish(MqttAirCond, NULL, topic, strlen(payload), payload, 0, 0);

            json_object_put(jObj);
        }
    }


}

/* get 虚点的deviceKey by key
 * @key: json key/value pair 中的key
 */
int GetIntValByKey(struct json_object *jRoot, char *key)
{
    int val;
    struct json_object *jVal;
    jVal = json_object_object_get(jRoot, key);
//    ES_PRT_DEBUG("%s\n",json_object_to_json_string_ext(jdeviceKey, JSON_C_TO_STRING_PRETTY));
    val = atoi(json_object_get_string(jVal));

    return val;
}


/* 设置空调的虚点PointProp_t，虚点的function等价于name */
void SetVirPointProp(PointProp_t *prop, char *virName, const char *valStr, enum TypeOfVal type, int deviceKey)
{
    strcpy(prop->name, virName);
    strcpy(prop->func, virName);
    prop->tag = type;
    prop->deviceKey = deviceKey;

    if(type == TypeOfVal_INT){
        prop->Val.valI = atoi(valStr);

    }else if(type == TypeOfVal_DOUBLE){
        prop->Val.valD = atof(valStr);
    }

}
/* 设置空调的实点PointProp_t，实点的val字段此处不用 */
void SetRealPointProp(PointProp_t *prop, char *RealName, const char *deviceKey, const char *MapKey, enum TypeOfVal type)
{
    strcpy(prop->name, RealName);
    strcpy(prop->func, MapKey);
    prop->tag = type;
    prop->deviceKey = atoi(deviceKey);
}

/* src赋值给dest
 * dest内存包含了len个src长度
 * 注意：使用此函数要注意len不能超过dest指向的有效空间
 */
void InitPointProp(PointProp_t *dest, PointProp_t *src, int start, int end)
{
    int i;

    for(i=start; i<end; i++){
        memcpy(dest+i, src, sizeof(PointProp_t));
    }
}

/* 遍历jVirFunction中每个虚点，赋值给Device代表的设备
 * jVirFunction格式：
 *  "function": {
 *      "xxx": "xxx",
 *      "xxx": "xxx"
 *  } 
 */
void ExtractVirPoint(struct json_object *jVirPoint, int deviceKey, AppAirCondDev_t *Device)
{
    json_type type;

    /* get 虚点的个数，申请相应内存 */
    Device->lenVir = json_object_object_length(jVirPoint);
    Device->PointProp = calloc(Device->lenVir, sizeof(PointProp_t));

    /* 所有新加的点位初始化，主要将deviceKey赋值Invalid */
    InitPointProp(Device->PointProp, &ppinit, 0, Device->lenVir);

    int i = 0;
    /* 遍历虚点，提取value+deviceKey */
    json_object_object_foreach(jVirPoint,keyVir,valVir){

        type = json_object_get_type(valVir);
        switch(type){
            case json_type_string:
                /* value不为空 */
                if(strcmp(json_object_get_string(valVir), "") != 0){
                    // 风机整机启停
                    if(strcmp(keyVir, SF_ENA) == 0){
                        strcpy((Device->PointProp+i)->name, keyVir);
                        strcpy((Device->PointProp+i)->func, keyVir);
                        (Device->PointProp+i)->deviceKey = deviceKey;
                        (Device->PointProp+i)->tag = TypeOfVal_INT;
                        /* SF_ENA命令在添加设备时默认时false */
                        (Device->PointProp+i)->Val.valI = 0;
                        /*
                        if(strcmp(json_object_get_string(valVir), "false") == 0){
                            (Device->PointProp+i)->Val.valI = 0;
                        }else{
                            (Device->PointProp+i)->Val.valI = 1;
                        }
                        */
                    // 房间温度预设
                    }else if(strcmp(keyVir, RM_TSP) == 0){
                        SetVirPointProp(Device->PointProp+i, keyVir, json_object_get_string(valVir), TypeOfVal_DOUBLE, deviceKey);
                    // 房间湿度预设
                    }else if(strcmp(keyVir, RM_HSP) == 0 ){
                        SetVirPointProp(Device->PointProp+i, keyVir, json_object_get_string(valVir), TypeOfVal_DOUBLE, deviceKey);
                    // 冬夏季转换
                    }else if(strcmp(keyVir, WS_EX) == 0 ){
                        SetVirPointProp(Device->PointProp+i, keyVir, json_object_get_string(valVir), TypeOfVal_INT, deviceKey);
                    // 房间二氧化碳浓度预设
                    }else if(strcmp(keyVir, RM_CO2SP) == 0 ){
                        SetVirPointProp(Device->PointProp+i, keyVir, json_object_get_string(valVir), TypeOfVal_INT, deviceKey);
                    // 新风阀初始开度预设
                    }else if(strcmp(keyVir, OAD_MIN) == 0 ){
                        SetVirPointProp(Device->PointProp+i, keyVir, json_object_get_string(valVir), TypeOfVal_DOUBLE, deviceKey);
                    // 回风阀初始开度预设
                    }else if(strcmp(keyVir, RA_MIN) == 0 ){
                        SetVirPointProp(Device->PointProp+i, keyVir, json_object_get_string(valVir), TypeOfVal_DOUBLE, deviceKey);
                    // 水阀初始开度预设
                    }else if(strcmp(keyVir, VLV_MIN) == 0 ){
                        SetVirPointProp(Device->PointProp+i, keyVir, json_object_get_string(valVir), TypeOfVal_DOUBLE, deviceKey);
                    // 风机运行时间的value不为空，说明是从本地文件加载的，设备已经运行了.
                    }else if(strcmp(keyVir, VSD_RT) == 0 ){
                        Device->RunTime = (unsigned int)atoi(json_object_get_string(valVir));
                    }

                /* VSD-RT(风机运行时间) 的value为空, 说明是新加设备,  */ 
                }else if(strcmp(keyVir, VSD_RT) == 0){
                    
                    SetVirPointProp(Device->PointProp+i, VSD_RT, "0", TypeOfVal_INT, deviceKey);
                    
                }
                break;
            default: break;
        }

        if(i < Device->lenVir){
            i++;
        }
    }

}

/* 根据虚点的个数，申请相应的内存，将jRealPoint报文中的点位信息赋值给设备节点Dev */
void ExtractRealPoint(struct json_object *jRealPoint, AppAirCondDev_t *Dev)
{
    /* 实点操作 */
    struct json_object *jdeviceKey, *jMap, *jMapKey;

    int i;
    i = Dev->lenVir;
    /* 遍历实点 */
    json_object_object_foreach(jRealPoint,keyReal,valReal){

        /* 跳过"virDevType"对象 */
        if(strcmp(keyReal, "virDevType") == 0){
            continue;
        }
        /* 实点deviceKey */
        jdeviceKey = json_object_object_get(valReal, "deviceKey");
        jMap = json_object_object_get(valReal, "jsonMap");
        jMapKey = json_object_object_get(jMap, "key");

        /* 没有选中的点位，deviceKey和MapKey的value="" */
        if(strcmp(json_object_get_string(jdeviceKey), "") != 0
            && strcmp(json_object_get_string(jMapKey), "") != 0){

            /* 累计实点个数 */ 
            Dev->lenReal++;
            Dev->len = Dev->lenReal + Dev->lenVir;
            /* 扩展点位，在虚点后面加入实点 */
            Dev->PointProp = realloc(Dev->PointProp, sizeof(PointProp_t) * Dev->len);
            memcpy(Dev->PointProp + Dev->len - 1, &ppinit, sizeof(PointProp_t));

            //回风温度
            if(strcmp(keyReal, RA_T) == 0){
                SetRealPointProp(Dev->PointProp+i, keyReal, json_object_get_string(jdeviceKey), json_object_get_string(jMapKey), TypeOfVal_DOUBLE);
            //回风湿度 
            }else if(strcmp(keyReal, RA_H) == 0){
                SetRealPointProp(Dev->PointProp+i, keyReal, json_object_get_string(jdeviceKey), json_object_get_string(jMapKey), TypeOfVal_DOUBLE);
            //送风温度
            }else if(strcmp(keyReal, SA_T) == 0){
                SetRealPointProp(Dev->PointProp+i, keyReal, json_object_get_string(jdeviceKey), json_object_get_string(jMapKey), TypeOfVal_DOUBLE);
            //送风湿度
            }else if(strcmp(keyReal, SA_H) == 0){
                SetRealPointProp(Dev->PointProp+i, keyReal, json_object_get_string(jdeviceKey), json_object_get_string(jMapKey), TypeOfVal_DOUBLE);

            //室内二氧化碳含量
            }else if(strcmp(keyReal, RM_CO2) == 0){
                SetRealPointProp(Dev->PointProp+i, keyReal, json_object_get_string(jdeviceKey), json_object_get_string(jMapKey), TypeOfVal_INT);

            //二通调节水阀反馈
            }else if(strcmp(keyReal, VLV_FB) == 0){
                SetRealPointProp(Dev->PointProp+i, keyReal, json_object_get_string(jdeviceKey), json_object_get_string(jMapKey), TypeOfVal_DOUBLE);

            //调节型回风阀反馈
            }else if(strcmp(keyReal, RAD_FB) == 0){
                SetRealPointProp(Dev->PointProp+i, keyReal, json_object_get_string(jdeviceKey), json_object_get_string(jMapKey), TypeOfVal_DOUBLE);

            //调节型室外新风阀反馈
            }else if(strcmp(keyReal, OAD_FB) == 0){
                SetRealPointProp(Dev->PointProp+i, keyReal, json_object_get_string(jdeviceKey), json_object_get_string(jMapKey), TypeOfVal_DOUBLE);

            //二通调节水阀
            }else if(strcmp(keyReal, VLV_C) == 0){
                SetRealPointProp(Dev->PointProp+i, keyReal, json_object_get_string(jdeviceKey), json_object_get_string(jMapKey), TypeOfVal_DOUBLE);

            //调节型加湿阀
            }else if(strcmp(keyReal, HUM_TC) == 0){
                SetRealPointProp(Dev->PointProp+i, keyReal, json_object_get_string(jdeviceKey), json_object_get_string(jMapKey), TypeOfVal_DOUBLE);

            //冷水调节阀
            }else if(strcmp(keyReal, CV_C) == 0){
                SetRealPointProp(Dev->PointProp+i, keyReal, json_object_get_string(jdeviceKey), json_object_get_string(jMapKey), TypeOfVal_DOUBLE);

            //热水调节阀
            }else if(strcmp(keyReal, HV_C) == 0){
                SetRealPointProp(Dev->PointProp+i, keyReal, json_object_get_string(jdeviceKey), json_object_get_string(jMapKey), TypeOfVal_DOUBLE);

            //回风调节阀
            }else if(strcmp(keyReal, RAD_TC) == 0){
                SetRealPointProp(Dev->PointProp+i, keyReal, json_object_get_string(jdeviceKey), json_object_get_string(jMapKey), TypeOfVal_DOUBLE);

            //室外新风调节阀
            }else if(strcmp(keyReal, OAD_TC) == 0){
                SetRealPointProp(Dev->PointProp+i, keyReal, json_object_get_string(jdeviceKey), json_object_get_string(jMapKey), TypeOfVal_DOUBLE);

            //送风机故障
            }else if(strcmp(keyReal, SF_F) == 0){
                SetRealPointProp(Dev->PointProp+i, keyReal, json_object_get_string(jdeviceKey), json_object_get_string(jMapKey), TypeOfVal_INT);

            //送风机手/自动状态
            }else if(strcmp(keyReal, SF_AM) == 0){
                SetRealPointProp(Dev->PointProp+i, keyReal, json_object_get_string(jdeviceKey), json_object_get_string(jMapKey), TypeOfVal_INT);

            //送风机压差状态
            }else if(strcmp(keyReal, SF_DP) == 0){
                SetRealPointProp(Dev->PointProp+i, keyReal, json_object_get_string(jdeviceKey), json_object_get_string(jMapKey), TypeOfVal_INT);

            //送风机状态
            }else if(strcmp(keyReal, SF_S) == 0){
                SetRealPointProp(Dev->PointProp+i, keyReal, json_object_get_string(jdeviceKey), json_object_get_string(jMapKey), TypeOfVal_INT);
                /* 初始将风机运行状态设为-1，下层会通过mqtt定时发出点位的信息，到时候会修改本程序内对应点位的value */
                (Dev->PointProp+i)->Val.valI = -1;
            //防冻报警
            }else if(strcmp(keyReal, FR_PR) == 0){
                SetRealPointProp(Dev->PointProp+i, keyReal, json_object_get_string(jdeviceKey), json_object_get_string(jMapKey), TypeOfVal_INT);

            //滤网压差状态
            }else if(strcmp(keyReal, FILT_S) == 0){
                SetRealPointProp(Dev->PointProp+i, keyReal, json_object_get_string(jdeviceKey), json_object_get_string(jMapKey), TypeOfVal_INT);

            //新风滤网压差报警
            }else if(strcmp(keyReal, FFILT_S) == 0){
                SetRealPointProp(Dev->PointProp+i, keyReal, json_object_get_string(jdeviceKey), json_object_get_string(jMapKey), TypeOfVal_INT);

            //开关型新风阀状态
            }else if(strcmp(keyReal, OAD_S) == 0){
                SetRealPointProp(Dev->PointProp+i, keyReal, json_object_get_string(jdeviceKey), json_object_get_string(jMapKey), TypeOfVal_INT);

            //开关型新风阀状态
            }else if(strcmp(keyReal, RAD_S) == 0){
                SetRealPointProp(Dev->PointProp+i, keyReal, json_object_get_string(jdeviceKey), json_object_get_string(jMapKey), TypeOfVal_INT);

            //开关型加湿阀
            }else if(strcmp(keyReal, HUM_C) == 0){
                SetRealPointProp(Dev->PointProp+i, keyReal, json_object_get_string(jdeviceKey), json_object_get_string(jMapKey), TypeOfVal_INT);

            //送风机命令
            }else if(strcmp(keyReal, SF_C) == 0){
                SetRealPointProp(Dev->PointProp+i, keyReal, json_object_get_string(jdeviceKey), json_object_get_string(jMapKey), TypeOfVal_INT);

            //回风阀开关
            }else if(strcmp(keyReal, RAD_C) == 0){
                SetRealPointProp(Dev->PointProp+i, keyReal, json_object_get_string(jdeviceKey), json_object_get_string(jMapKey), TypeOfVal_INT);

            //室外新风阀开关
            }else if(strcmp(keyReal, OAD_C) == 0){
                SetRealPointProp(Dev->PointProp+i, keyReal, json_object_get_string(jdeviceKey), json_object_get_string(jMapKey), TypeOfVal_INT);

            }
        i++;
        }   
    }
}

/* 解析虚实点，赋值给设备实例 */
void ExtractPoints(AppAirCondDev_t *Dev, struct json_object *jRoot)
{
    pthread_mutex_lock(&Dev->lock);

    struct json_object *jVirPoint;

    /* 1 get 虚点deviceKey */
    int deviceKey = GetIntValByKey(jRoot, "deviceKey");
    /* get 虚点的function集合 */
    jVirPoint = json_object_object_get(jRoot, "function");

    ExtractVirPoint(jVirPoint, deviceKey, Dev);

    /* 2. parse 实点 */
    struct json_object *jRealPoint;

    jRealPoint = json_object_object_get(jRoot, "configuration");
    ExtractRealPoint(jRealPoint, Dev);

    /* 3. 根据deviceKey，订阅topic */
    SubMqttByDeviceKey(Dev);
#if 0
int hh;
ES_PRT_DEBUG("len = %d \n", Dev->len); 
for(hh=Dev->lenVir; hh<Dev->len; hh++){
    if((Dev->PointProp+hh)->tag == TypeOfVal_DOUBLE){
        ES_PRT_DEBUG("name = %s, deviceKey = %u, func = %s, tag = %d, val = %lf\n", \
        (Dev->PointProp+hh)->name, (Dev->PointProp+hh)->deviceKey, \
        (Dev->PointProp+hh)->func, \
        (Dev->PointProp+hh)->tag, (Dev->PointProp+hh)->Val.valD);

    }else if((Dev->PointProp+hh)->tag == TypeOfVal_INT){
        ES_PRT_DEBUG("name = %s, deviceKey = %u, func = %s, tag = %d, val = %d\n", \
        (Dev->PointProp+hh)->name, (Dev->PointProp+hh)->deviceKey, \
        (Dev->PointProp+hh)->func, \
        (Dev->PointProp+hh)->tag, (Dev->PointProp+hh)->Val.valI);
    }
}
#endif

    pthread_mutex_unlock(&Dev->lock);
}

/* 解析并添加平台下发的新实例 */
int AddDevFromJson(struct json_object *jRoot)
{
    struct json_object *jData, *jConf, *jvirDevType;

    /* get data object */
    if( !json_object_object_get_ex(jRoot, "data", &jData) ){
        return -1;
    }
    /* get data/configuration object */
    if( !json_object_object_get_ex(jData, "configuration", &jConf) ){
        return -1;
    }
    /* get data/configuration/virDevType object */
    if( !json_object_object_get_ex(jConf, "virDevType", &jvirDevType) ){
        return -1;
    }

    if(strcmp(APPNAME, json_object_get_string(jvirDevType)) != 0){
        ES_PRT_WARN(" Unknown virDevType: %s \n", json_object_get_string(jvirDevType));
        return -1;
    }

    int VirDeviceKey, deviceID;
    /* 获取虚点的deviceKey，并作为当前设备实例的deviceID */
    VirDeviceKey = GetIntValByKey(jData, "deviceKey");
    deviceID = VirDeviceKey;

    AppAirCondDev_l *node;

    pthread_mutex_lock(&AirCondList_head->lock);

    /* 根据deviceID判断链表中此设备实例是否存在，0：不存在，就添加新实例 */
    if(IsExistInAirCondDevList(deviceID) == 0){
        node = NewAirCondDevNode(deviceID);
        DevertAirCondDevList(node);

        /* 解析虚/实点，assign to 设备实例 */
        ExtractPoints(&node->AirCondDev, jData);

        ES_PRT_INFO("deviceID <%d> has been added to instance list \n", deviceID);
        /* pid init */
        PidInitHmdt(node);
        PidInitTemp(node);
        PidInitCO2(node);
    }else{
        ES_PRT_WARN("repeat deviceID: %d when add points from plat, please remove device from list and resend \n", deviceID);
        pthread_mutex_unlock(&AirCondList_head->lock);
        return -1;
    }

    pthread_mutex_unlock(&AirCondList_head->lock);
    return 0;
}

void _AddDevFromLocal(char *dir, char *file)
{
    ES_PRT_INFO("Parsing local json file: %s \n", file);

    char BasePath[100];
    struct json_object *jRoot;

    sprintf(BasePath, "%s/%s", dir, file);

    jRoot = json_object_from_file(BasePath);

    AddDevFromJson(jRoot);
    /* 将报文再发给/local/app/bacnet/command */
    ES_PRT_INFO("Sending new device to bacnet/104... \n");
    const char *message = json_object_to_json_string_ext(jRoot, JSON_C_TO_STRING_PRETTY);
    mosquitto_publish(MqttAirCond, NULL, "/local/app/bacnet/command", strlen(message), message, 0, 0);
}

/* 
 *  get设备实例from本地文件：平台下发的点位配置清单都会保存下来，
 *  所以程序如果是重启的话要解析本地文件的。
 */
void AddDevFromLocal()
{
    /* 1. 如果文件存在，get point from file 
     * 文件名字格式：AppName_GatewayDeviceKey_VirPointDeviceID.json，
     */

    DIR *dirp;
    struct dirent *ent;

    if(access(LOCAL_CONF_DIR, F_OK) == 0){

        ES_PRT_INFO("Foreach Json conf dir: %s \n", LOCAL_CONF_DIR);

        dirp = opendir(LOCAL_CONF_DIR);

        while( (ent = readdir(dirp)) != NULL ){
            if ( (strcmp(".", ent->d_name) == 0 ) || (strcmp("..", ent->d_name) == 0) ){
                continue;
            }
            _AddDevFromLocal(LOCAL_CONF_DIR, ent->d_name);
        }

        closedir(dirp);

    }else{
        ES_PRT_WARN("Local Json conf dir: %s is unexist \n", LOCAL_CONF_DIR);
    }
}

/* @brief: 将RunTime转化成json对象，写进文件file中 */
void _SaveTime2Local(unsigned int RunTime, char *file)
{
    COMBINEFULLFILENAME(file, FileFullName);

    char RTStr[40];
    sprintf(RTStr, "%d", RunTime);

    json_object *jRunTime = json_object_new_string(RTStr);

    json_object *jRoot = json_object_from_file(FileFullName);
    json_object *jData = json_object_object_get(jRoot, "data");
    json_object *jFunc = json_object_object_get(jData, "function");

    json_object_object_del(jFunc, "VSD-RT");
    json_object_object_add(jFunc, "VSD-RT", jRunTime);

    json_object_to_file_ext(FileFullName, jRoot, JSON_C_TO_STRING_PRETTY);

    json_object_put(jRoot);
}

/* @brief: 将风机累计运行时间保存到本地json文件，替换"VSD-RT"对象 
 * @deviceID: 设备id，也是虚点的ID
 * @return: sucess-0
 */
int SaveTime2Local(unsigned int deviceID, unsigned int RunTime)
{
    char dIDStr[20];
    sprintf(dIDStr, "%d", deviceID);

    DIR *dirp;
    struct dirent *ent;

    dirp = opendir(LOCAL_CONF_DIR);

    while( (ent = readdir(dirp)) != NULL ){
        if ( (strcmp(".", ent->d_name) == 0 ) || (strcmp("..", ent->d_name) == 0) ){
            continue;
        }
        if( strstr(ent->d_name, dIDStr) ){
            _SaveTime2Local(RunTime, ent->d_name);
            break;
        }
    }

    if(ent == NULL){
        ES_PRT_ERROR("Can't find related local file with deviceID: %d, when saving time to local \n", deviceID);
        closedir(dirp);
        return -1;
    }

    closedir(dirp);
    return 0;
}

/* @brief：将jNewVirPoint中包含的点位和实例Dev中的虚点的deviceKey和function一一对比
 * 只要比对的上，说明平台要更新这些虚点的value
 */
void _UpdatePoints(struct json_object *jPoints, AppAirCondDev_t *Dev, int deviceKey)
{
    int i;
    json_type type;

    json_object_object_foreach(jPoints,key,jVal){

        type = json_object_get_type(jVal);
        switch(type){
            case json_type_string:

                for(i=0; i<Dev->len; i++){

                    if((Dev->PointProp+i)->deviceKey != deviceKey){
                        continue;
                    }

                    if(strcmp(key, (Dev->PointProp+i)->func) == 0){
                        ES_PRT_INFO("Updating points: name = %s, function = %s, val = %s \n", \
                                    (Dev->PointProp+i)->name, key, \
                                    json_object_to_json_string_ext(jVal, JSON_C_TO_STRING_PRETTY));

                        /* 风机使能，此点的value不是数字型单独算 */
                        if(strcmp(key, SF_ENA) == 0){
                            if(strcmp(json_object_get_string(jVal), "false") == 0){
                                Dev->runcmd = STOP;
                            }else{
                                Dev->runcmd = START;
                            }
                        /* 冬夏季需要额外更新标志位，所以也要单独处理 */
                        }else if(strcmp(key, WS_EX) == 0){
                            if((Dev->PointProp+i)->Val.valI != atoi(json_object_get_string(jVal))){
                                Dev->WSChanged = CHANGED;
                            }
                            (Dev->PointProp+i)->Val.valI = atoi(json_object_get_string(jVal));

                        /* 报警解除 */
                        }else if(strcmp((Dev->PointProp+i)->name, FR_PR) == 0){
                            /* 由报警->解除报警 */
                            if((Dev->PointProp+i)->Val.valI == ALARM && atoi(json_object_get_string(jVal)) == NORMAL){
                                Dev->ReleaseAlarm = 1;
                            }
                            (Dev->PointProp+i)->Val.valI = atoi(json_object_get_string(jVal));
                        
                        /* 其他点位 */
                        }else if((Dev->PointProp+i)->tag == TypeOfVal_INT){
                            (Dev->PointProp+i)->Val.valI = atoi(json_object_get_string(jVal));
                        
                        }else if((Dev->PointProp+i)->tag == TypeOfVal_DOUBLE){
                            (Dev->PointProp+i)->Val.valD = atof(json_object_get_string(jVal));
                        }

                    }
                }
                break;
            default: break;
        }
    }

}

/* @brief: 根据deviceID找到设备，再根据jRoot报文的function找到虚点
 * 并将function中虚点的value赋值给设备
 * 支持function中多个虚点
 * 返回deviceKey
 */
unsigned int UpdatePoints(struct json_object *jRoot)
{
    AppAirCondDev_l *node;

    unsigned int deviceKey;
    deviceKey = GetIntValByKey(jRoot, "deviceKey");
    ES_PRT_INFO("Updating points: DeviceKey = %d \n", deviceKey);

    struct json_object *jFunc;
    if( !json_object_object_get_ex(jRoot, "function", &jFunc) ){
         return -1;
    }

    pthread_mutex_lock(&AirCondList_head->lock);

    for(node = AirCondList_head->next; node != NULL; node = node->next){
        pthread_mutex_lock(&node->AirCondDev.lock);
        _UpdatePoints(jFunc, &node->AirCondDev, deviceKey);
        pthread_mutex_unlock(&node->AirCondDev.lock);
    }

    pthread_mutex_unlock(&AirCondList_head->lock);

    return deviceKey;
}

/* 将平台下发的新设备点位保存到本地 */
void SaveDev2Local(struct json_object *jRoot)
{
    /* 网关的deviceKey */
    int GwDeviceKey;
    GwDeviceKey = GetIntValByKey(jRoot, "deviceKey");

    struct json_object *jData;

    /* get data object */
    jData = json_object_object_get(jRoot, "data");

    int VirDeviceKey, deviceID;
    /* 获取虚点的deviceKey，并作为当前实例的deviceID */
    VirDeviceKey = GetIntValByKey(jData, "deviceKey");
    deviceID = VirDeviceKey;

    char FileName[100];
    sprintf(FileName, "%s_%d_%d.json", APPNAME, GwDeviceKey, deviceID);

    COMBINEFULLFILENAME(FileName, FileFullName);

    DIR *dirp;
    struct dirent *ent;

    if(access(LOCAL_CONF_DIR, F_OK) == 0){

        ES_PRT_INFO("Saving new device to local: %s \n", LOCAL_CONF_DIR);

        dirp = opendir(LOCAL_CONF_DIR);

        while( (ent = readdir(dirp)) != NULL ){
            if ( (strcmp(".", ent->d_name) == 0 ) || (strcmp("..", ent->d_name) == 0) ){
                continue;
            }
            if(strcmp(ent->d_name, FileName) == 0){
                ES_PRT_ERROR("File: %s has existed, when save points to local. will remove it \n", FileName);
                rmdir(FileName);
            }
        }

        ES_PRT_INFO("Creating new local file: %s, writing json to it. \n", FileFullName);
        json_object_to_file_ext(FileFullName, jRoot, JSON_C_TO_STRING_PRETTY);

        closedir(dirp);

    }else{
        ES_PRT_ERROR("Local Json conf dir: %s is unexist, exit!! \n", LOCAL_CONF_DIR);
        exit(-1);
    }
}

void _UpdatePoints2Local(char *file, struct json_object *jNew)
{
    COMBINEFULLFILENAME(file, FileFullName);

    json_object *jRoot = json_object_from_file(FileFullName);
    json_object *jData = json_object_object_get(jRoot, "data");
    json_object *jFunc = json_object_object_get(jData, "function");

    json_object *jNewFunc = json_object_object_get(jNew, "function");

    json_object_object_foreach(jNewFunc, NewKey, NewValue){
        /* 根据NewKey删掉文件中对应的object，然后把新的Key-Value写回文件 */
        json_object_object_del(jFunc, NewKey);
        json_object_object_add(jFunc, NewKey, NewValue);
    }

    json_object_to_file_ext(FileFullName, jRoot, JSON_C_TO_STRING_PRETTY);
    /* ！！！！此处不能释放，不然有段错误！！！！ */
//    json_object_put(jRoot);
}

/* 将虚点的新value更新到本地 */
void UpdatePoints2Local(struct json_object *jRoot)
{
    int deviceKey;
    deviceKey = GetIntValByKey(jRoot, "deviceKey");

    char deviceIDStr[20];
    sprintf(deviceIDStr, "%u", deviceKey);

    DIR *dirp;
    struct dirent *ent;

    dirp = opendir(LOCAL_CONF_DIR);

    while( (ent = readdir(dirp)) != NULL ){
        if ( (strcmp(".", ent->d_name) == 0 ) || (strcmp("..", ent->d_name) == 0) ){
                continue;
        }
        /* 找到虚点所在的文件 */
        if(strstr(ent->d_name, deviceIDStr) != NULL){
            _UpdatePoints2Local(ent->d_name, jRoot);
        }
    }

    closedir(dirp);

}

void DelDevFromList(unsigned int deviceID)
{
    pthread_mutex_lock(&AirCondList_head->lock);

    AppAirCondDev_l *node;

    for(node=AirCondList_head->next; node!=NULL; node=node->next){

        if(node->AirCondDev.deviceID == deviceID){
            /* free 点位 */
            pthread_mutex_lock(&node->AirCondDev.lock);
            free(node->AirCondDev.PointProp);
            pthread_mutex_unlock(&node->AirCondDev.lock);

            /* free 设备 */
            DelAirCondDevList(node);
            ES_PRT_INFO("Delete device from list(deviceID = %d) \n", node->AirCondDev.deviceID);
        }

    }

    pthread_mutex_unlock(&AirCondList_head->lock);
}

void DelDevFromLocal(unsigned int deviceID)
{
    char FileFullName[300];
    char deviceIDStr[20];
    sprintf(deviceIDStr, "%u", deviceID);

    DIR *dirp;
    struct dirent *ent;

    dirp = opendir(LOCAL_CONF_DIR);

    while( (ent = readdir(dirp)) != NULL ){
        if ( (strcmp(".", ent->d_name) == 0 ) || (strcmp("..", ent->d_name) == 0) ){
                continue;
        }
        /* 找到虚点所在的文件 */
        if(strstr(ent->d_name, deviceIDStr) != NULL){
            sprintf(FileFullName, "%s/%s", LOCAL_CONF_DIR, ent->d_name);
            unlink(FileFullName);
            ES_PRT_INFO("Delete device from local (%s) \n", FileFullName);
        }
    }

    closedir(dirp);
}

void DelDevFromJson(struct json_object *jData)
{
    struct json_object *jdeviceKey;
    jdeviceKey = json_object_object_get(jData, "deviceKey");

    unsigned int deviceID = (unsigned int)atoi(json_object_get_string(jdeviceKey));
    
    DelDevFromList(deviceID);
    DelDevFromLocal(deviceID);
}

/* 收到mqtt的message回调后，根据cmd字段分类处理 */
void MqttCmdMessProc(char *message)
{
    const char *cmd;
    struct json_object *jRoot, *jCmdVal;

    /* 1 string -> json */
    jRoot = json_tokener_parse(message);

    /* 2 提取cmd字段 */
    /* 不存在cmd字段的报文返回 */
    if( !json_object_object_get_ex(jRoot, "cmd", &jCmdVal) ){
        return;
    }

    cmd = json_object_get_string(jCmdVal);

    int deviceKey;
    deviceKey = GetIntValByKey(jRoot, "deviceKey");
    ES_PRT_INFO("Rcv json message, DeviceKey = %d, cmd = %s \n", deviceKey, cmd);

    /* 上层下发配置点位 */
    if(strcmp(cmd, "addDevice") == 0){
        //TODO: 判断"virDevType":"AHU"，否则返回
        struct json_object *jData, *jvirDevType;
        jData = json_object_object_get(jRoot, "data");
        jvirDevType = json_object_object_get(jData, "virDevType");
        /* 不是AHU的报文不处理 */
        if(strcmp("AHU", json_object_get_string(jvirDevType)) != 0){
            return;
        }

        /* 添加空调实例, 成功返回0 */
        if(AddDevFromJson(jRoot) != -1){
            /* 新设备保存到本地 */
            SaveDev2Local(jRoot);
        }

        /* 将报文再发给/local/app/bacnet/command */
        //TODO: add errno to json payload
        ES_PRT_INFO("Send new device to /local/app/bacnet/command ... \n");
        mosquitto_publish(MqttAirCond, NULL, "/local/app/bacnet/command", strlen(message), message, 0, 0);

    /* 2. 更新虚点的值 */
    }else if(strcmp(cmd, "write") == 0){
        if( UpdatePoints(jRoot) != -1 ){
            UpdatePoints2Local(jRoot);

            // local/{gatewaydeviceKey}/{hvacdeviceKey}
            PublishWriteBack(jRoot);
        }
    }else if(strcmp(cmd, "delDevice") == 0){
        struct json_object *jData, *jvirDevType;
        jData = json_object_object_get(jRoot, "data");
        jvirDevType = json_object_object_get(jData, "virDevType");
        /* 不是AHU的报文不处理 */
        if(strcmp("AHU", json_object_get_string(jvirDevType)) != 0){
            return;
        }

        DelDevFromJson(jData);
        //TODO: ack, add errno

    }

    json_object_put(jRoot);

}

/* @brief: 收到mqtt的message回调后，无cmd字段的分类处理 */
void MqttMessProc(char *message)
{
    struct json_object *jRoot;

    /* 1 string -> json */
    jRoot = json_tokener_parse(message);

    /* 2 提取cmd字段 */
    /* 存在cmd字段的报文不做处理 */
    struct json_object *jCmdVal;
    if( json_object_object_get_ex(jRoot, "cmd", &jCmdVal) ){
        return;
    }

    UpdatePoints(jRoot);
    json_object_put(jRoot);
}