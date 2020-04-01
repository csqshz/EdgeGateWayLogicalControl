/**********************************************************************************************
 * 移植到eastsoft边缘计算网关,来自:
 * Arduino PID Library - Version 1.0.1
 * by Brett Beauregard <br3ttb@gmail.com> brettbeauregard.com
 *
 * This Library is licensed under a GPLv3 License
 **********************************************************************************************/

#ifndef PID_INCLUDE_PID_PID_H_
#define PID_INCLUDE_PID_PID_H_

#include <stdbool.h>
#include <stdio.h>
#include <sys/time.h>
#include <stdlib.h>

#define AIRCOND_PIDSAMPLETIME (15000)	// 空调pid采样时间 15s

/*
PID自动调节为1,正常采用的模式;
PID手动写入为0,调试时使用;
*/
typedef enum {
	PID_AUTOMATIC = 1, PID_MANUAL = 0
} Pid_OperationMode;

/*
PID方向选择:
正向选择 DIRECT,典型业务场景为制热,即输出变大,输入也会变大;
反向选择 REVERSE,典型业务场景为制冷,即输入(冷水阀)变大,输入会变小;
*/
typedef enum {
  PID_DIRECT = 3, PID_REVERSE = 4
} Pid_Direction;


typedef struct {

// private 'member' data
/*
用于数据获取的 P I D算子
*/
	double dispKp;        // * we'll hold on to the tuning parameters in user-entered
	double dispKi;        //   format for display purposes
	double dispKd;        //

	double kp;            // * (P)roportional Tuning Parameter
	double ki;            // * (I)ntegral Tuning Parameter
	double kd;            // * (D)erivative Tuning Parameter

/*
控制方向
*/
	Pid_Direction controllerDirection;

/*
输入,输出,设置点的指针
*/
	double *myInput;		// * Pointers to the Input, Output, and Setpoint variables
	double *myOutput;	//   This creates a hard link between the variables and the
	double *mySetpoint;	//   PID, freeing the user from having to constantly tell us
						//   what these values are.  with pointers we'll just know.

/*
上次运行时间
*/
	unsigned long lastTime;
	double iTerm, lastInput;

/*
PID loop运行时间,建议时间为15秒;
*/
	unsigned long sampleTime;
/*
输出最大限制,最小限制
*/
	double outMin, outMax;
	Pid_OperationMode inAuto;

	double deadBand;

} Pid;

// * constructor.  links the PID to the Input, Output, and
//   Setpoint.  Initial tuning parameters are also set here
extern void pid_init(Pid* self, double* input, double* output, double* setpoint, double kp,
        double ki, double kd, double deadBand, Pid_Direction controllerDirection);

extern void pid_setMode(Pid* self, Pid_OperationMode newMode); // * sets PID to either Manual (0) or Auto (non-0)

// performs the PID calculation.  it should be
// called every time loop() cycles. ON/OFF and
// calculation frequency can be set using SetMode
// SetSampleTime respectively
extern bool pid_compute(Pid* self);

//clamps the output to a specific range. 0-255 by default, but
//it's likely the user will want to change this depending on
//the application
/*
设置输出限制,对于阀门控制输出,设置0%~100%,对于变频风机,如何获得相关数值??

self PID结构体指针
newMin  最小输出限制数值
newMax  最大输出限制数值
*/
extern void pid_setOutputLimits(Pid* self, double, double);

//available but not commonly used functions ********************************************************

// * While most users will set the tunings once in the
//   constructor, this function gives the user the option
//   of changing tunings during runtime for Adaptive control
/*
设置PID算子
*/
extern void pid_setTunings(Pid* self, double, double, double);

// * Sets the Direction, or "Action" of the controller. DIRECT
//   means the output will increase when error is positive. REVERSE
//   means the opposite.  it's very unlikely that this will be needed
//   once it is set in the constructor.
/*
设置PID控制方向
*/
extern void pid_setControllerDirection(Pid* self, Pid_Direction);

// * sets the frequency, in Milliseconds, with which
//   the PID calculation is performed.  default is 100
extern void pid_setSampleTime(Pid* self, unsigned long);

//Display functions ****************************************************************
extern double pid_getKp(Pid* self);   // These functions query the pid for interal values.
extern double pid_getKi(Pid* self);    //  they were created mainly for the pid front-end,
extern double pid_getKd(Pid* self);       // where it's important to know what is actually
extern int pid_getMode(Pid* self);              //  inside the PID.
extern Pid_Direction pid_getDirection(Pid* self);           //

#endif /* PID_INCLUDE_PID_PID_H_ */
