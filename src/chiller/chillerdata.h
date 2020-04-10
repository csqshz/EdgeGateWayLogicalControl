#ifndef __CHILLERDATA_H__
#define __CHILLERDATA_H__

#include <stdio.h>
#include <pthread.h>
#include <stdbool.h>
#include <assert.h>
#include "ahupid.h"
#include "data.h"
#include "list.h"

/* 通用设备属性描述符 */
typedef struct _DevProp_t{
    unsigned int virlen;    // 虚点个数
    unsigned int reallen;   // 实点个数
    unsigned int len;       // 虚+实
    PointProp_t *PointProp; // 点位集合

    unsigned int IsVSD;     //是否变频
    double  VsdMax;         //最大频率
    double  VsdMin;         //最小频率

}DevProp_t;

/***************************冷却塔************************/
/* 冷却塔风扇描述符 */
typedef struct _CtfDev_t{
    DevProp_t CtfProp;       // 风扇点位
    struct list_head list;
}CtfDev_t;

/* 冷却塔描述符 */
typedef struct _CtDev_t{
    unsigned int CtfLen;    // 风扇个数

    DevProp_t CtProp;        // 冷却塔点位
    struct list_head list;
}CtDev_t;

/* 冷却塔头 */
typedef struct _CtHead_t{
    unsigned int CtLen;     // 冷却塔个数

    PointProp_t *PointProp; // 塔组的点位集，比如总蝶阀的开关，状态等点位
    struct list_head list;
}CtHead_t;
/*-----------------------冷却塔end-----------------------/

/***************************冷却水泵***********************/
/* 冷却水泵描述符 */
typedef struct _PchwpDev_t{
    DevProp_t CwpProp;
    struct list_head list;
}CwpDev_t;

/* 冷却水泵头 */
typedef struct _CwpHead_t{
    unsigned int CwpLen;    // 冷却水泵个数
    struct list_head list;
}CwpHead_t;

/*-----------------------冷却水泵end-----------------------/

/***********************初级冷冻水泵***************************/
/* 初级冷冻水泵描述符 */
typedef struct _PchwpDev_t{
    DevProp_t PchwpProp;
    struct list_head list;
}PchwpDev_t;

/* 初级冷冻水泵头 */
typedef struct _PchwpHead_t{
    unsigned int PchwpLen;  // 初级冷冻水泵个数
    struct list_head list;
}PchwpHead_t;

/*-----------------------初级冷冻水泵end-----------------------/

/***********************冷机***************************/
/* 冷机描述符 */
typedef struct _ChDev_t{
    DevProp_t ChProp;
    struct list_head list;
}ChDev_t;

/* 冷机头 */
typedef struct _ChHead_t{
    unsigned int ChLen;     // 冷机个数
    struct list_head list;
}ChHead_t;

/*-----------------------冷机end-----------------------/

/* 冷源设备描述符 */
typedef struct _AppChillerDev_t{
    pthread_mutex_t lock;
    unsigned int deviceID;  // 设备唯一ID，将虚点的deviceKey作为当前设备总ID(历史原因)

    CtHead_t CtHead;            // 冷却塔头
    CwpHead_t CwpHead;          // 冷却水泵头
    PchwpHead_t PchwpHead;      // 初级冷冻水泵头
    ChHead_t ChHead;            // 冷却塔头

    PointProp_t *PointProp;     // 系统点位

}AppChillerDev_t;

#endif  //__CHILLERDATA_H__