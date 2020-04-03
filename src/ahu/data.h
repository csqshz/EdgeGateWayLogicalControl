#ifndef _DATA_H_
#define _DATA_H_

#include <stdio.h>
#include <pthread.h>
#include <stdbool.h>
#include <assert.h>
#include "ahupid.h"

/* BO */
#define HUM_C 	"HUM-C"		// 开关型加湿阀
#define RAD_C	"RAD-C"		// 回风阀开关
#define SF_C	"SF-C"		// 风机启停命令
#define OAD_C	"OAD-C"		// 室外新风阀开关
/* BI */
#define SF_F	"SF-F"		// 送风机故障
#define SF_S	"SF-S"		// 送风机状态
#define SF_AM	"SF-AM"		// 送风机手自动状态
#define FILT_S	"FILT-S"	// 滤网压差状态
#define FR_PR	"FR-PR"		// 防冻开关
#define SF_DP	"SF-DP"		// 送风机压差状态
#define FFILT_S	"FFILT-S"	// 新风滤网压差报警
#define OAD_S	"OAD-S"		// 开关型新风阀状态
#define RAD_S	"RAD-S"		// 开关型新风阀状态
/* AO */
#define VLV_C	"VLV-C"		// 二通调节水阀
#define HUM_TC	"HUM-TC"	// 调节型加湿阀
#define CV_C	"CV-C"		// 冷水调节阀
#define HV_C	"HV-C"		// 热水调节阀
#define RAD_TC	"RAD-TC"	// 回风调节阀
#define OAD_TC	"OAD-TC"	// 室外新风调节阀
/* AI */
#define SA_T	"SA-T"		// 送风温度
#define SA_H	"SA-H"		// 送风湿度
#define RA_T	"RA-T"		// 回风温度
#define RA_H	"RA-H"		// 回风湿度
#define RAD_FB	"RAD-FB"	// 回风调节阀反馈
#define OAD_FB	"OAD-FB"	// 室外新风阀反馈
#define VLV_FB	"VLV-FB"	// 二通调节水阀反馈
#define RM_CO2	"RM-CO2"	// 室内二氧化碳含量
#define RA_CO2	"RA-CO2"	// 回风二氧化碳含量
#define SA_CO2	"SA-CO2"	// 送风二氧化碳含量
/* 软点位 */
#define SF_ENA	"SF-ENA"	// 风机整机启动
#define RM_TSP	"RM-TSP"	// 送风温度设定点
#define RM_HSP	"RM-HSP"	// 送风湿度设定点
#define WS_EX	"WS-EX"		// 冬夏季节转换
#define RM_CO2SP	"RM-CO2SP" // 房间CO2含量设定点
#define OAD_MIN	"OAD-MIN"	// 新风阀初始开度设定
#define RA_MIN	"RA-MIN"	// 回风阀初始开度设定
#define VLV_MIN	"VLV-MIN"	// 水阀初始开度设定
#define VSD_RT	"VSD-RT"	// 风机运行时间

#define START	(1)
#define STOP	(0)
#define COMPLETED	(1)
#define UNCOMPLETED	(0)

#define NORMAL	(false)
#define ALARM	(true)

#define VLV_C_MIN	(0)		// 阀最小开度
#define VLV_C_MAX	(100)	// 阀最大开度
#define VLV_C_DEF	(10)	// 初始默认开度

#define TEMP_PID_P	(2.0)	// 1.67-5
#define TEMP_PID_I	(300)	// 180-600
#define TEMP_PID_D	(30)	// 18-60

#define INVALID_DEVICEKEY	(0xffffffff)

#define CHANGED	(1)

#define APPNAME	"AHU"

enum Season{
	WINTER = 1,
	SUMMER,
	TRANSITION
};

enum CmdOper{
	CMD_WRITE,
	CMD_READ,
};

enum TypeOfVal{
	TypeOfVal_BOOL,
	TypeOfVal_INT,
	TypeOfVal_CHAR,
	TypeOfVal_DOUBLE
};

typedef union _DataType_u{
	bool		valB;
	int 		valI;
	char		valC;
	double		valD;
}DataType_u;

typedef struct _PointProp_t{
	char			name[20];

	// 决定Val的类型：enum TypeOfVal, 暂时没用到
	enum TypeOfVal	tag;
	DataType_u		Val;

	unsigned int 	deviceKey;
	char			func[30];

}PointProp_t;

/* App Device */
typedef struct _AppAirCondDev_t{

	pthread_mutex_t lock;

	unsigned int deviceID;		//设备唯一ID，将虚点的deviceKey作为当前设备总ID(历史原因)

	unsigned int RunTime;		//整机运行时间 单位s
	int lenVir;			//虚点总数
	int lenReal;		//实点总数
	int len;			//虚+实总数

	PointProp_t *PointProp;	// 点位的属性

	PidSet_t PidSet;	//每个设备的PID集合

	/* 点位相关的标志位，系统相关的标志位 */
		//TODO: 要不要做不停机条件下更换点位配置清单

		int startCompleted;	// 1：启动结束
							// 0：没有启动

		int runcmd;			// 1: 收到上层发来的启动命令
							// 0: 初始值 或者 关闭命令

		int WSChanged;		// 冬夏季(制冷/制热)转换标志: CHANGED: 值发生改变
	
		int TempRun;		// 温控线程运行状态
		int SF1stState;		// 风机的第一次查询状态，程序启动后要先向下层查询设备运行状态，然后此标志置1
							// 此标志是为了防止每次轮询都查询
		int SF2ndState;		// 风机的第二次查询状态
		int SF3rdState;
		int SF4thState;

		int ReleaseAlarm;	// 报警解除标志, 1: 解除, 0:初始状态
		int GenerateAlarm;	// 报警生成标志, 1: 生成, 0:初始状态，
	/* 标志位end */

}AppAirCondDev_t;

typedef struct _AppAirCondDev_l{
	pthread_mutex_t lock;		// 此lock控制链表节点的增删改查，
								// 而AppAirCondDev_t内的lock控制各自实例内部点位的查改
	AppAirCondDev_t AirCondDev;
	struct _AppAirCondDev_l *next;
}AppAirCondDev_l;

#endif //_DATA_H_