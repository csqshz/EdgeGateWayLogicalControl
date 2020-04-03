#include "app.h"
#include "es_print.h"
#include "mqttlib.h"
#include "list.h"
#include "ahupid_func.h"
#include <signal.h>
#include <math.h>

extern char *AirApp2Low;

extern AppAirCondDev_l *AirCondList_head;

/* 实例初始模板值，刚新创建一个实例时，把AirCondDevInit赋给它 */
PointProp_t ppinit = {
    .deviceKey = INVALID_DEVICEKEY,
};

void AirCondReleaseFrostAlarm(AppAirCondDev_l *node)
{
    ES_PRT_INFO("Enter Release Frost Alarm ... \n");
    /* 1. 风机开 */
    SETVAL_SENDCMD(SF_C, 1, TypeOfVal_INT, &node->AirCondDev, CMD_WRITE, AirApp2Low);
    /* 2 新风阀 */
        /* 1.1 开关型新风阀 */
    SETVAL_SENDCMD(OAD_C, 1, TypeOfVal_INT, &node->AirCondDev, CMD_WRITE, AirApp2Low);
        /* 1.2 调节型新风阀 TODO: 开度大小应该由平台配置下来，有个虚点保存 */
    SETVAL_SENDCMD(OAD_TC, VLV_C_DEF, TypeOfVal_DOUBLE, &node->AirCondDev, CMD_WRITE, AirApp2Low);

    ES_PRT_INFO("Recovery Temp Thread Ctrl... \n");
    node->AirCondDev.TempRun = START;
    node->AirCondDev.runcmd = START;
    node->AirCondDev.ReleaseAlarm = 0;
    node->AirCondDev.GenerateAlarm = 0;
}

/* 防冻报警处理：
 * 1. 暂停温控thread
 * 2. 关风机，关风阀，热水阀开最大
 */
void AirCondFrostAlarm(AppAirCondDev_l *node)
{
    if(node->AirCondDev.SF4thState == 1){
        if(QueryIntValFromAirCondList(SF_S, &node->AirCondDev) == STOP){
            ES_PRT_INFO("SF(deviceID=%d) run state is %d(1:runing, 0:stop) \n", node->AirCondDev.deviceID, \
                        QueryIntValFromAirCondList(SF_S, &node->AirCondDev));
            goto FrostAlarm;
        }else{
            return;
        }
    }
    ES_PRT_WARN("(deviceID=%d)Enter the process of Frost Alarm ... \n", node->AirCondDev.deviceID);
    /* 1. suspend 温控线程 */
    ES_PRT_INFO("(deviceID=%d)Halt Temp Thread Ctrl... \n", node->AirCondDev.deviceID);
    node->AirCondDev.TempRun = STOP;

    /* 2. close 风机 */
    SETVAL_SENDCMD(SF_C, 0, TypeOfVal_INT, &node->AirCondDev, CMD_WRITE, AirApp2Low);
    // 查询风机运行状态， 读取点位前将value设为-1, 下层反馈后，会修改value
    SETVAL_SENDCMD(SF_S, -1, TypeOfVal_INT, &node->AirCondDev, CMD_READ, AirApp2Low);
    node->AirCondDev.SF4thState = 1;

    if(QueryIntValFromAirCondList(SF_S, &node->AirCondDev) == STOP){
        ES_PRT_INFO("SF(deviceID=%d) run state is %d(1:runing, 0:halt) \n", node->AirCondDev.deviceID, \
                    QueryIntValFromAirCondList(SF_S, &node->AirCondDev));
        node->AirCondDev.runcmd = STOP;
    }else{
        return;
    }

FrostAlarm:
    /* 3 close 新风阀 */
        /* 3.1 开关型新风阀 */
    SETVAL_SENDCMD(OAD_C, 0, TypeOfVal_INT, &node->AirCondDev, CMD_WRITE, AirApp2Low);
        /* 3.2 调节型新风阀 */
    SETVAL_SENDCMD(OAD_TC, VLV_C_MIN, TypeOfVal_DOUBLE, &node->AirCondDev, CMD_WRITE, AirApp2Low);

    /* 4 热水阀开至最大 */
    //二通
    SETVAL_SENDCMD(VLV_C, VLV_C_MAX, TypeOfVal_DOUBLE, &node->AirCondDev, CMD_WRITE, AirApp2Low);
    //热水阀
    SETVAL_SENDCMD(HV_C, VLV_C_MAX, TypeOfVal_DOUBLE, &node->AirCondDev, CMD_WRITE, AirApp2Low);

    node->AirCondDev.SF4thState = 0;
    /* 置1可以避免一种情况：
     * 已经处理了报警，然后在报警还没有解除的情况下，防止每次轮询都进入此函数重复关闭操作
     */
    node->AirCondDev.GenerateAlarm = 1;
    ES_PRT_INFO("(deviceID=%d)The process of Frost Alarm is finished \n", node->AirCondDev.deviceID);
}

/*
 * 停止空调, close 风机、阀门等点位
 */
void AirCondStop(AppAirCondDev_l *node)
{
    /* SF3rdhState == 1 说明上轮已经发送关闭风机指令了，且风机SF-S链表中的值被置-1，
     * 现在只需查看风机运行状态即可 
     */
    if(node->AirCondDev.SF3rdState == 1){
        if(QueryIntValFromAirCondList(SF_S, &node->AirCondDev) == STOP){
            ES_PRT_INFO("SF(deviceID=%d) run state is %d(1:runing, 0:stop) \n", node->AirCondDev.deviceID, \
                        QueryIntValFromAirCondList(SF_S, &node->AirCondDev));
            goto CloseSF;
        }else{
            return;
        }

    }

    /* 1. close 风机 */
    SETVAL_SENDCMD(SF_C, 0, TypeOfVal_INT, &node->AirCondDev, CMD_WRITE, AirApp2Low);
    // 查询风机运行状态， 读取点位前将value设为-1, 下层反馈后，会修改value
    SETVAL_SENDCMD(SF_S, -1, TypeOfVal_INT, &node->AirCondDev, CMD_READ, AirApp2Low);
    node->AirCondDev.SF3rdState = 1;
    if(QueryIntValFromAirCondList(SF_S, &node->AirCondDev) == STOP){
        ES_PRT_INFO("SF(deviceID=%d) run state is %d(1:runing, 0:halt) \n", node->AirCondDev.deviceID, \
                    QueryIntValFromAirCondList(SF_S, &node->AirCondDev));
    }else{
        return;
    }

CloseSF:
    node->AirCondDev.startCompleted = UNCOMPLETED;
    /* 2 新风阀 */
        /* 2.1 开关型新风阀 */
    SETVAL_SENDCMD(OAD_C, 0, TypeOfVal_INT, &node->AirCondDev, CMD_WRITE, AirApp2Low);
        /* 2.2 调节型新风阀 */
    SETVAL_SENDCMD(OAD_TC, VLV_C_MIN, TypeOfVal_DOUBLE, &node->AirCondDev, CMD_WRITE, AirApp2Low);

    /* 3 回风阀 */
        /* 3.1 开关型回风阀 */
    SETVAL_SENDCMD(RAD_C, 0, TypeOfVal_INT, &node->AirCondDev, CMD_WRITE, AirApp2Low);
        /* 3.2 调节型回风阀 */
    SETVAL_SENDCMD(RAD_TC, VLV_C_MIN, TypeOfVal_DOUBLE, &node->AirCondDev, CMD_WRITE, AirApp2Low);

    /* 制冷季过渡季关闭水阀，采暖季水阀开 50% */
        if(QueryIntValFromAirCondList(WS_EX, &node->AirCondDev) == WINTER){
            /* 1 二通 */
            SETVAL_SENDCMD(VLV_C, 50, TypeOfVal_DOUBLE, &node->AirCondDev, CMD_WRITE, AirApp2Low);
            /* 2 热水管 */
            SETVAL_SENDCMD(CV_C, 50, TypeOfVal_DOUBLE, &node->AirCondDev, CMD_WRITE, AirApp2Low);
        }else{
            /* 1 二通 */
            SETVAL_SENDCMD(VLV_C, VLV_C_MIN, TypeOfVal_DOUBLE, &node->AirCondDev, CMD_WRITE, AirApp2Low);
            /* 2 热水管 */
            SETVAL_SENDCMD(CV_C, VLV_C_MIN, TypeOfVal_DOUBLE, &node->AirCondDev, CMD_WRITE, AirApp2Low);
            /* 3 冷水管 */
            SETVAL_SENDCMD(HV_C, VLV_C_MIN, TypeOfVal_DOUBLE, &node->AirCondDev, CMD_WRITE, AirApp2Low);
        }

    node->AirCondDev.SF3rdState = 0;
    ES_PRT_INFO("TSM(%d) has stoped \n", node->AirCondDev.deviceID);
}

/* 启停状态transfer */
void *AirCondCtrlStart_thread(void *arg)
{
    AppAirCondDev_l *node;

    ES_PRT_INFO("Enter Start-Stop thread ctrl ... \n");

    while(AirCondList_head->next == NULL){
        ES_PRT_INFO("AirCondDev list is empty ... \n");
        sleep(1);
    }
#if 1
    while(1){

        for(node = AirCondList_head->next; node != NULL; node = node->next){
            pthread_mutex_lock(&node->AirCondDev.lock);

            /* 启动时，先读取风机运行状态，确定现在状态，更新标志位 */
            /* 
               第一次查询风机运行状态标志位：为了防止下层程序没有及时返回风机运行状态而阻塞影响其他设备的轮询处理。
               (风机运行状态点位SF-S在初始化为-1)当第一次轮询到设备时，从链表中读取风机运行状态
               (SF-S的值发生翻转时由下层主动发过来，相应的代码会修改链表中对应的值，所以只需要从链表中查询即可)，
               当返回值为0或1才将此标志位置1，这样下次轮询到此设备就不需要再查询风机状态了。
            */

            if(node->AirCondDev.SF1stState == 0){
                /* 只需从链表中查询即可，因为下层会定时反馈点位的状态，本程序相应地会修改链表中的value */
                if(QueryIntValFromAirCondList(SF_S, &node->AirCondDev) == 1){
                    ES_PRT_INFO("(deviceID=%d)Reboot app: SF has run \n", node->AirCondDev.deviceID);
                    node->AirCondDev.runcmd = START;
                    SETVAL2LIST(SF_C, 1, TypeOfVal_INT, &node->AirCondDev);
                    node->AirCondDev.startCompleted = COMPLETED;
                    /* 查询完毕，置1,以后就不用再查了，除非重启 */
                    node->AirCondDev.SF1stState = 1;

                }else if(QueryIntValFromAirCondList(SF_S, &node->AirCondDev) == 0){
                    ES_PRT_INFO("(deviceID=%d)Reboot app: SF run state is halt, wait for start cmd \n", node->AirCondDev.deviceID);
                    /* 查询完毕，置1 */
                    node->AirCondDev.SF1stState = 1;
                }

            }

            /* 第一次风机状态查询完毕，才能执行后面代码 */
            if(node->AirCondDev.SF1stState == 0){
                pthread_mutex_unlock(&node->AirCondDev.lock);
                continue;
            }
            /* 1 如果runcmd==START, 并且SF_C点位的val==0，
             *   说明收到上层发来启动命令，且现在设备还没有启动
             * 
             * 2 如果ReleaseAlarm == 0，说明当前报警状态不是由报警->解除报警，而是初始化就处在无报警状态
		     * 
             * 将SF_C点位在链表中的val置1
		     */
            if(node->AirCondDev.runcmd == START 
                && QueryIntValFromAirCondList(FR_PR, &node->AirCondDev) == NORMAL
                && node->AirCondDev.ReleaseAlarm == 0
                && QueryIntValFromAirCondList(SF_C, &node->AirCondDev) == 0 ){

                SETVAL2LIST(SF_C, 1, TypeOfVal_INT, &node->AirCondDev);
                ES_PRT_INFO("TSM(%d) set SF_C val in list: %d(0:stop, 1:start) \n", node->AirCondDev.deviceID, QueryIntValFromAirCondList(SF_C, &node->AirCondDev));

            }else if(QueryIntValFromAirCondList(SF_C, &node->AirCondDev) == 1
                    && QueryIntValFromAirCondList(FR_PR, &node->AirCondDev) == NORMAL
                    && node->AirCondDev.ReleaseAlarm == 0
                    && node->AirCondDev.startCompleted == UNCOMPLETED){

                /* SF2ndState == 1表示SF-S已经被设为-1查询了, 说明新风阀和风机启动命令已经发下去了
                 * 所以直接判断风机运行状态，看是否需要打开回风阀 */
                if(node->AirCondDev.SF2ndState == 1){
                    if(QueryIntValFromAirCondList(SF_S, &node->AirCondDev) == 1){
                        ES_PRT_INFO("(deviceID=%d)SF run state is %d(1:runing, 0:halt) \n", \
                                    node->AirCondDev.deviceID, QueryIntValFromAirCondList(SF_S, &node->AirCondDev));
                        /* 3 回风阀 */
                        /* 3.1 开关型回风阀 */
                        SETVAL_SENDCMD(RAD_C, 1, TypeOfVal_INT, &node->AirCondDev, CMD_WRITE, AirApp2Low);
                        /* 3.2 调节型回风阀 TODO: 开度大小根据实际情况定 */
                        SETVAL_SENDCMD(RAD_TC, VLV_C_DEF, TypeOfVal_DOUBLE, &node->AirCondDev, CMD_WRITE, AirApp2Low);

                        node->AirCondDev.startCompleted = COMPLETED;

                        ES_PRT_INFO("TSM(%d) wind machine and wind valve has worked \n", node->AirCondDev.deviceID);
                        /* reset */
                        node->AirCondDev.SF2ndState = 0;
                        pthread_mutex_unlock(&node->AirCondDev.lock);
                        continue;
                    }else{
                        pthread_mutex_unlock(&node->AirCondDev.lock);
                        continue;
                    }
                }

                ES_PRT_INFO("TSM(%d) Enter startup wind machine and wind valve... \n", node->AirCondDev.deviceID);
                //风机、风阀联锁控制
                /* 1 新风阀 */
                    /* 1.1 开关型新风阀 */
                SETVAL_SENDCMD(OAD_C, 1, TypeOfVal_INT, &node->AirCondDev, CMD_WRITE, AirApp2Low);
                    /* 1.2 调节型新风阀 TODO: 开度大小应该由平台配置下来，有个虚点保存 */
                SETVAL_SENDCMD(OAD_TC, VLV_C_DEF, TypeOfVal_DOUBLE, &node->AirCondDev, CMD_WRITE, AirApp2Low);

                /* 2 打开风机 */
                SETVAL_SENDCMD(SF_C, 1, TypeOfVal_INT, &node->AirCondDev, CMD_WRITE, AirApp2Low);

                /* 查询风机开启状态, 读取点位前将value设为-1, 下层反馈后，会修改value */
                SETVAL_SENDCMD(SF_S, -1, TypeOfVal_INT, &node->AirCondDev, CMD_READ, AirApp2Low);
                node->AirCondDev.SF2ndState = 1;

                if(QueryIntValFromAirCondList(SF_S, &node->AirCondDev) == 1){
                    ES_PRT_INFO("(deviceID=%d)SF run state is %d(1:runing, 0:halt) \n", \
                                    node->AirCondDev.deviceID, QueryIntValFromAirCondList(SF_S, &node->AirCondDev));
                    /* 3 回风阀 */
			        /* 3.1 开关型回风阀 */
                    SETVAL_SENDCMD(RAD_C, 1, TypeOfVal_INT, &node->AirCondDev, CMD_WRITE, AirApp2Low);
                    /* 3.2 调节型回风阀 TODO: 开度大小根据实际情况定 */
                    SETVAL_SENDCMD(RAD_TC, VLV_C_DEF, TypeOfVal_DOUBLE, &node->AirCondDev, CMD_WRITE, AirApp2Low);

                    node->AirCondDev.startCompleted = COMPLETED;
            
                    ES_PRT_INFO("TSM(%d) wind machine and wind valve has worked \n", node->AirCondDev.deviceID);
                    /* reset */
                    node->AirCondDev.SF2ndState = 0;
                }
                

            /* runcmd==0 说明收到关闭命令或者本身就在关闭状态
             * 再判断 startCompleted == COMPLETED 说明系统处于运行状态，说明runcmd是从0->1的
             * 那么执行关闭操作
             */
            //TODO: 如果正处在防冻报警还没有解除，收到停机命令怎么处理
            }else if(node->AirCondDev.runcmd == STOP 
                    && node->AirCondDev.startCompleted == COMPLETED){

                AirCondStop(node);

            /* 初始状态全为0 或全为1, 无防冻报警 */
            }else if(
                        (
                        node->AirCondDev.runcmd == STOP
                        && QueryIntValFromAirCondList(SF_C, &node->AirCondDev) == 0
                        && node->AirCondDev.startCompleted == UNCOMPLETED
                        && QueryIntValFromAirCondList(FR_PR, &node->AirCondDev) == NORMAL
                        && node->AirCondDev.ReleaseAlarm == 0
                        )
                    ||
                        (
                        node->AirCondDev.runcmd == START
                        && QueryIntValFromAirCondList(SF_C, &node->AirCondDev) == 1
                        && node->AirCondDev.startCompleted == COMPLETED
                        && QueryIntValFromAirCondList(FR_PR, &node->AirCondDev) == NORMAL
                        && node->AirCondDev.ReleaseAlarm == 0
                        )
                    )
            {
                /* do nothing */

            /* 防冻报警处理 */
            /* GenerateAlarm == 1 表示已经针对防冻报警做了处理，避免在报警还没有解除的情况下重复处理
             * 此标志会在AirCondReleaseFrostAlarm()函数中reset
             */
            }else if(QueryIntValFromAirCondList(FR_PR, &node->AirCondDev) == ALARM
                    && node->AirCondDev.GenerateAlarm == 0){

                AirCondFrostAlarm(node);
            
            /* ReleaseAlarm == 1 表示防冻报警状态由 报警->解除报警 */
            }else if(QueryIntValFromAirCondList(FR_PR, &node->AirCondDev) == NORMAL
                    && node->AirCondDev.ReleaseAlarm == 1){
                AirCondReleaseFrostAlarm(node);

            }

            pthread_mutex_unlock(&node->AirCondDev.lock);
            usleep(1000*100);
        }
    }
#endif
    return NULL;
}

/* 过渡季送风机运行时，新风阀全开，回风阀关闭，按最大新风比运行 */
void TransitSeaonProc(AppAirCondDev_l *node)
{
    /* 如果是转到过渡季的，或者平台初次下发点位配置也是CHANGED */
    if(node->AirCondDev.WSChanged == CHANGED){
        ES_PRT_INFO("Change to Transition season... \n");
        
        /* 1 新风阀开到最大 */
            /* 1.1 开关型新风阀 */
        SETVAL_SENDCMD(OAD_C, 1, TypeOfVal_INT, &node->AirCondDev, CMD_WRITE, AirApp2Low);
            /* 1.2 调节型新风阀 TODO: 开度大小应该由平台配置下来，有个虚点保存 */
        SETVAL_SENDCMD(OAD_TC, VLV_C_MAX, TypeOfVal_DOUBLE, &node->AirCondDev, CMD_WRITE, AirApp2Low);

        /* 2 回风阀关闭 */
            /* 2.1 开关型回风阀 */
        SETVAL_SENDCMD(RAD_C, 0, TypeOfVal_INT, &node->AirCondDev, CMD_WRITE, AirApp2Low);
            /* 2.2 调节型回风阀 TODO: 开度大小根据实际情况定 */
        SETVAL_SENDCMD(RAD_TC, VLV_C_MIN, TypeOfVal_DOUBLE, &node->AirCondDev, CMD_WRITE, AirApp2Low);

        //reset
        node->AirCondDev.WSChanged = !CHANGED;
    }
}


/* 温控 */ 
void *AirCondCtrlTemp_thread(void *arg)
{
    AppAirCondDev_l *node;

    ES_PRT_INFO("Enter Temp thread ctrl ... \n");
#if 1
    while(1){
        /* 遍历每一个设备 */
        for(node = AirCondList_head->next; node != NULL; node = node->next){
            pthread_mutex_lock(&node->AirCondDev.lock);

            /* 确保各标志位都处于开启状态, 且不是过渡季 */
            if(node->AirCondDev.runcmd == START 
               && QueryIntValFromAirCondList(SF_C, &node->AirCondDev) == 1
               && node->AirCondDev.startCompleted == COMPLETED
               && IsPointExist(RM_TSP, &node->AirCondDev)
               && node->AirCondDev.TempRun == START
               && QueryIntValFromAirCondList(WS_EX, &node->AirCondDev) != TRANSITION
               )
            {
                /* 如果冬夏季节更替，就重新init */
                if(node->AirCondDev.WSChanged == CHANGED){
                    ES_PRT_INFO("Change to (%d, 1:winter, 2:summer) season... \n", QueryIntValFromAirCondList(WS_EX, &node->AirCondDev));
                    PidInitTemp(node);
                    node->AirCondDev.WSChanged = !CHANGED;
                }
                AirCondTempAM(node);

            /* 过渡季处理 */
            }else if(QueryIntValFromAirCondList(WS_EX, &node->AirCondDev) == TRANSITION){
                TransitSeaonProc(node);
            }
            

            pthread_mutex_unlock(&node->AirCondDev.lock);
        }

        usleep(1000*100);
    }
#endif
    return NULL;
}

/* 湿控 */
void *AirCondCtrlHmdt_thread(void *arg)
{
    AppAirCondDev_l *node;

    ES_PRT_INFO("Enter Hmdt thread ctrl ... \n");
#if 1
    while(1){
        /* 遍历每一个设备实例 */
        for(node = AirCondList_head->next; node != NULL; node = node->next){
            pthread_mutex_lock(&node->AirCondDev.lock);

            /* 确保各标志位都处于开启状态 */
            if(node->AirCondDev.runcmd == START 
               && QueryIntValFromAirCondList(SF_C, &node->AirCondDev) == 1
               && node->AirCondDev.startCompleted == COMPLETED
               && IsPointExist(RM_HSP, &node->AirCondDev)
               )
            {
                /* 1 开关型加湿阀 */
                if(IsPointExist(HUM_C, &node->AirCondDev)){
                    AirCondHmdtSM(node);

                /* 2 调节型加湿器 */
                }else if(IsPointExist(HUM_TC, &node->AirCondDev)){
                    AirCondHmdtAM(node);
                }
            }

            pthread_mutex_unlock(&node->AirCondDev.lock);
        }

        usleep(1000*100);
    }
#endif
    return NULL;
}

/* Co2控
 * 制冷季：根据室内外焓值和CO2浓度控制新风回风阀
 * 制暖季：根据CO2浓度控制新风回风阀
 * （refer to 万达文档）
 */
void *AirCondCtrlCo2_thread(void *arg)
{
    AppAirCondDev_l *node;

    ES_PRT_INFO("Enter CO2 thread ctrl ... \n");

    while(1){
        /* 遍历每一个设备 */
        for(node = AirCondList_head->next; node != NULL; node = node->next){
            pthread_mutex_lock(&node->AirCondDev.lock);
            pthread_mutex_unlock(&node->AirCondDev.lock);
        }

        usleep(1000*100);
    }
    return NULL;
}

/* 运行时间 */
void *AirCondCtrlRt_thread(void *arg)
{
    AppAirCondDev_l *node;
#if 1
    while(1){
        /* 遍历每一个设备 */
        for(node = AirCondList_head->next; node != NULL; node = node->next){
            pthread_mutex_lock(&node->AirCondDev.lock);

            if(QueryIntValFromAirCondList(SF_S, &node->AirCondDev) == 0){
                node->AirCondDev.RunTime = 0;
            }else if(QueryIntValFromAirCondList(SF_S, &node->AirCondDev) != 0
                    && QueryIntValFromAirCondList(SF_C, &node->AirCondDev) == 1){
                node->AirCondDev.RunTime += 1;
            }
            //TODO:send to other app//
//            SETVAL2LIST(VSD_RT, node->AirCondDev.RunTime, TypeOfVal_INT, &node->AirCondDev);
            UpdateVal2Local(node->AirCondDev.deviceID, "VSD-RT");
            pthread_mutex_unlock(&node->AirCondDev.lock);
        }
        /* 每秒1s一次，不考虑cpu高负载造成的延时 */
        sleep(1);
    }
#endif
    return NULL;
}