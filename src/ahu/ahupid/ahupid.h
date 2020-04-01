#ifndef __AIRPID_H__
#define __AIRPID_H__

#include "pid.h"


//TODO: 是否要改成平台下发
#define TEMP_PID_P	(2.0)	// 1.67-5
#define TEMP_PID_I	(300)	// 180-600
#define TEMP_PID_D	(30)	// 18-60

#define HMDT_PID_P	(2.0)	// 1.67-5
#define HMDT_PID_I	(300)	// 180-600
#define HMDT_PID_D	(30)	// 18-60


typedef struct _Pid_t{
		Pid pid;
		double *in;
		double *out;
		double *set;
		long tv_sec;	// 两次pid计算间的累计时间(us)，
		int initflag;	//1: 已经被init过了，除非pid的方向有变或者其他因素需要重新init，那么此标志置0
}Pid_t;

/* 温度、湿度、CO2的pid */
typedef struct _PidSet_t{
	// 温度
	Pid_t PidT;
	// 湿度
	Pid_t PidH;
	// CO2
	Pid_t PidCO2;
}PidSet_t;


#endif //__AIRPID_H__