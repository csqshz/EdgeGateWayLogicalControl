#include "ahulist.h"
#include "app.h"
#include "es_print.h"
#include "ahupid.h"

AppAHUDev_l *AirCondList_head = NULL;

/* init list head */
AppAHUDev_l *InitAirCondListHead()
{
    AppAHUDev_l *head = calloc(1, sizeof(AppAHUDev_l));
    pthread_mutex_init(&head->lock, NULL);

    return head;
}

/* 新建一个实例, 新建的依据是参数deviceID */
AppAHUDev_l *NewAHUDevNode(int deviceID) 
{
    AppAHUDev_l *node = calloc(1, sizeof(AppAHUDev_l));

    pthread_mutex_init(&node->lock, NULL);
    pthread_mutex_init(&node->AHUDev.lock, NULL);

    node->AHUDev.deviceID = deviceID;
    
    /* 冬夏季转换标志初始值为CHANGED，这样过渡季的处理直接在温控线程做就行了 */
    node->AHUDev.WSChanged = CHANGED;

    return node;
}

/* 空调实例链表中是否存在deviceID这个实例
 * 存在返回1
 */
bool IsExistInAHUDevList(int deviceID)
{
    AppAHUDev_l *temp = AirCondList_head;

    while(temp->next != NULL){
        if(temp->next->AHUDev.deviceID == deviceID){
            return 1;
        }

        temp = temp->next;
    }

    return 0;
}

void DevertAHUDevList(AppAHUDev_l *node)
{
    assert(node != NULL);

    AppAHUDev_l *temp = AirCondList_head;

    /* 非空，遍历到尾 */
    while(temp->next != NULL){
        temp = temp->next;
    }
    /* insert */
    temp->next = node;

}

void DelAHUDevList(AppAHUDev_l *node)
{
    assert(node != NULL);
    AppAHUDev_l *temp = AirCondList_head;

    while(temp->next != NULL){
        if(temp->next->AHUDev.deviceID == node->AHUDev.deviceID){
            temp->next = node->next;
            free(node);
            break;
        }
        temp = temp->next;
    }
}

/* 根据点位name判断链表中是否存在该点位 */
int IsPointExist(char *name, AppAHUDev_t *Dev)
{
    int i;

    for(i=0; i<Dev->len; i++){
        if(strcmp((Dev->PointProp+i)->name, name) == 0){
            return 1;
        }
    }
    return 0;
}

/* 根据参数name，从实例Dev中查询点位的deviceKey
 * @name: 点位的name
 */
int QueryDeviceKeyFromAirCondList(char *name, AppAHUDev_t *Dev)
{
    int i, ret;

    for(i=0; i<Dev->len; i++){
        if(strcmp((Dev->PointProp+i)->name, name) == 0){
            ret = (Dev->PointProp+i)->deviceKey;
            break;
        }
    }
    return ret;
}

/* 根据参数name，从实例Dev中查询int型点位的val 
 * @name: 点位的name
 */
int QueryIntValFromAirCondList(char *name, AppAHUDev_t *Dev)
{
    int i, ret;

    for(i=0; i<Dev->len; i++){
        if(strcmp((Dev->PointProp+i)->name, name) == 0){
            ret = (Dev->PointProp+i)->Val.valI;
            break;
        }
    }
    return ret;
}

/* 根据参数name，从实例Dev中查询double型点位的val 
 * @name: 点位的name
 */
double QueryDoubleValFromAirCondList(char *name, AppAHUDev_t *Dev)
{
    int i;
    double ret;

    for(i=0; i<Dev->len; i++){
        if(strcmp((Dev->PointProp+i)->name, name) == 0){
            ret = (Dev->PointProp+i)->Val.valD;
            break;
        }
    }
    return ret;
}

/* 根据参数name，从实例Dev中查询double型点位的val 
 * @name: 点位的name
 */
double *QueryDoublePtrFromAirCondList(char *name, AppAHUDev_t *Dev)
{
    int i;
    double *ret = NULL;

    for(i=0; i<Dev->len; i++){
        if(strcmp((Dev->PointProp+i)->name, name) == 0){
            ret = &(Dev->PointProp+i)->Val.valD;
            break;
        }
    }
    return ret;
}

/* 设置点位的val，并写到链表里去
 * 根据参数name从实例Dev中找到点位，将val赋值给对应变量
 * @name: 点位的name
 */
void SetVal2AirCondList(char *name, AppAHUDev_t *Dev, DataType_u val)
{
    int i;

    for(i=0; i<Dev->len; i++){

        if(strcmp((Dev->PointProp+i)->name, name) == 0){

            if((Dev->PointProp+i)->tag == TypeOfVal_INT){
                (Dev->PointProp+i)->Val.valI = val.valI;
//                ES_PRT_INFO("Set val to list(deviceID = %d) > %s: %d \n", Dev->deviceID, name, (Dev->PointProp+i)->Val.valI);

            }else if((Dev->PointProp+i)->tag == TypeOfVal_DOUBLE){
                (Dev->PointProp+i)->Val.valD = val.valD;
//                ES_PRT_INFO("Set val to list(deviceID = %d) > %s: %lf \n", Dev->deviceID, name, (Dev->PointProp+i)->Val.valD);
            }
        }
    }

}