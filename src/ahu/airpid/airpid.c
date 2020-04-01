#include "ahupid.h"
#include "list.h"
#include "es_print.h"
#include "proc.h"
#include <math.h>

extern char *AirApp2Low;

/* 调节模式温度控制：PID */
void AirCondTempAM(AppAirCondDev_l *node)
{
    long *tv_sec = &node->AirCondDev.PidSet.PidT.tv_sec;

    double *in = node->AirCondDev.PidSet.PidT.in;
    double *out = node->AirCondDev.PidSet.PidT.out;
    double *set = node->AirCondDev.PidSet.PidT.set;

    time_t t;
    time(&t);

    /* 每15s 计算一次pid */
    if((t - *tv_sec) > 15){
        /* (设定温度 - 回风温度)的绝对值 > 1℃ 才进行控制 */
        // 死区在pid内部实现，此处在pid外部通过偏差实现，其实意义不大
        if( fabs(*set - *in) > 1.0 ){
            pid_compute(&node->AirCondDev.PidSet.PidT.pid);
            
            ES_PRT_INFO("Temp(deviceID=%d): pid out = %f in = %f set %f \n", node->AirCondDev.deviceID, *out, *in, *set);
            /* SendCmd2Low 函数内部会判断点位是否选中，选中的才发送 */
            SendCmd2Low(VLV_C, &node->AirCondDev, CMD_WRITE, AirApp2Low);
            SendCmd2Low(CV_C, &node->AirCondDev, CMD_WRITE, AirApp2Low);
            SendCmd2Low(HV_C, &node->AirCondDev, CMD_WRITE, AirApp2Low);
        }
        /* 刷新计时 */
        *tv_sec = t;
    }


}

/* 开关模式湿度控制：根据预设和回风湿度判定是否打开加湿阀 */
void AirCondHmdtSM(AppAirCondDev_l *node)
{
    /* (目标湿度 - 回风湿度) > 1.0 : 就打开加湿阀 */
    if(QueryDoubleValFromAirCondList(RM_HSP, &node->AirCondDev)
       - QueryDoubleValFromAirCondList(RA_H, &node->AirCondDev)
       > 1.0){

        /* open开关型加湿阀(没开才开) */
        if(QueryIntValFromAirCondList(HUM_C, &node->AirCondDev) == 0){
            ES_PRT_INFO("Enter Humidity-SwitchMode ctrl (open), deviceID=%d \n", node->AirCondDev.deviceID);
            SETVAL_SENDCMD(HUM_C, 1, TypeOfVal_INT, &node->AirCondDev, CMD_WRITE, AirApp2Low);
        }

    }else{
        /* close开关型加湿阀(没关才关) */
        if(QueryIntValFromAirCondList(HUM_C, &node->AirCondDev) == 1){
            ES_PRT_INFO("Enter Humidity-SwitchMode ctrl (close), deviceID=%d \n", node->AirCondDev.deviceID);
            SETVAL_SENDCMD(HUM_C, 0, TypeOfVal_INT, &node->AirCondDev, CMD_WRITE, AirApp2Low);
        }
    }
}

/* 调节模式湿度控制：PID */
void AirCondHmdtAM(AppAirCondDev_l *node)
{
    long *tv_sec = &node->AirCondDev.PidSet.PidH.tv_sec;

    double *in = node->AirCondDev.PidSet.PidH.in;
    double *out = node->AirCondDev.PidSet.PidH.out;
    double *set = node->AirCondDev.PidSet.PidH.set;

    time_t t;
    time(&t);

    /* 每15s 计算一次pid */
    if((t - *tv_sec) > 15){
        /* (设定湿度 - 回风湿度) 绝对值 > 1% 才进行控制 */
        // 死区在pid内部实现，此处在pid外部通过偏差实现，其实意义不大
        if( fabs(*set - *in) > 1.0 ){
            pid_compute(&node->AirCondDev.PidSet.PidH.pid);

            ES_PRT_INFO("Hmdt(deviceID=%d): pid out = %f in = %f set %f \n", node->AirCondDev.deviceID, *out, *in, *set);
            SendCmd2Low(HUM_TC, &node->AirCondDev, CMD_WRITE, AirApp2Low);
        }
        /* 刷新计时 */
        *tv_sec = t;

    }
    
}

void PidInitHmdt(AppAirCondDev_l *node)
{   
    AppAirCondDev_t *Dev = &node->AirCondDev;

    Pid_t *PidH = &Dev->PidSet.PidH;
    /* 如果没有配置RA_H, HUM_TC, RM_HSP等点位，将返回NULL */
    PidH->in = QueryDoublePtrFromAirCondList(RA_H, Dev);
    PidH->out = QueryDoublePtrFromAirCondList(HUM_TC, Dev);
    PidH->set = QueryDoublePtrFromAirCondList(RM_HSP, Dev);

    if(PidH->in && PidH->out && PidH->set){
        ES_PRT_INFO("Points about computer Humidity-pid are choosed, deviceID(%d) \n", 
                    Dev->deviceID);

        /* 现在只有加湿功能，所以direct方向固定的，只init一次就好 */
        pid_init(&PidH->pid, PidH->in, PidH->out, PidH->set, 
                HMDT_PID_P, HMDT_PID_I, HMDT_PID_D, 1.0, PID_DIRECT);

        /* 自动模式 */
	    pid_setMode(&PidH->pid, PID_AUTOMATIC);

        /* 限制输出值的范围 */
	    pid_setOutputLimits(&PidH->pid, VLV_C_MIN, VLV_C_MAX);
    }else{
        ES_PRT_INFO("Points about computer Humidity-pid aren't choosed, deviceID(%d) \n",
                    Dev->deviceID);
    }

}

void PidInitTemp(AppAirCondDev_l *node)
{
    AppAirCondDev_t *Dev = &node->AirCondDev;

    if(QueryIntValFromAirCondList(WS_EX, &node->AirCondDev) == TRANSITION){
        ES_PRT_INFO(" Transitional season: no need to start Temperature-pid, deviceID(%d) \n",
                    Dev->deviceID);
        return;
    }

    Pid_t *PidT = &Dev->PidSet.PidT;
    /* 如果没有配置RA_T, VLV_C, RM_TSP等点位，将返回NULL */
    // 回风温度
    PidT->in = QueryDoublePtrFromAirCondList(RA_T, Dev);
    /* VLV_C/CV_C/HV_C 只能3选1, 平台决定选择哪个 */
    // 二通水阀，冷水阀，热水阀
    if((PidT->out = QueryDoublePtrFromAirCondList(VLV_C, Dev))){
        ES_PRT_INFO("Temperature pid-out uses <VLV_C>, deviceID(%d) \n", 
                    Dev->deviceID);
    }else if((PidT->out = QueryDoublePtrFromAirCondList(CV_C, Dev))){
        ES_PRT_INFO("Temperature pid-out uses <CV_C>, deviceID(%d) \n", 
                    Dev->deviceID);
    }else if((PidT->out = QueryDoublePtrFromAirCondList(HV_C, Dev))){
        ES_PRT_INFO("Temperature pid-out uses <HV_C>, deviceID(%d) \n", 
                    Dev->deviceID);
    }
    // 房间预设温度
    PidT->set = QueryDoublePtrFromAirCondList(RM_TSP, Dev);
    
    if(!PidT->in && !PidT->out && !PidT->set){
            ES_PRT_INFO("Points about Temperature-pid(in/out/set) aren't choosed, deviceID(%d) \n",
                        Dev->deviceID);
        return;
    }

    Pid_Direction dirt;

    if(QueryIntValFromAirCondList(WS_EX, &node->AirCondDev) == WINTER){
        dirt = PID_DIRECT;
    }else if(QueryIntValFromAirCondList(WS_EX, &node->AirCondDev) == SUMMER){
        dirt = PID_REVERSE;
    }

    pid_init(&PidT->pid, PidT->in, PidT->out, PidT->set, 
            TEMP_PID_P, TEMP_PID_I, TEMP_PID_D, 1.0, dirt);

    /* 自动模式 */
    pid_setMode(&PidT->pid, PID_AUTOMATIC);

    /* 限制输出值的范围 */
    pid_setOutputLimits(&PidT->pid, VLV_C_MIN, VLV_C_MAX);

    Dev->TempRun = START;
}


void PidInitCO2(AppAirCondDev_l *node)
{
    
}