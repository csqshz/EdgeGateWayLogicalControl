/**********************************************************************************************
 * 东软载波边缘计算网关,移植自:
 * Arduino PID Library - Version 1.0.1
 * by Brett Beauregard <br3ttb@gmail.com> brettbeauregard.com
 *
 * This Library is licensed under a GPLv3 License
 **********************************************************************************************/
/*
TODO:NEW: Proportional on Measurement 
http://brettbeauregard.com/blog/2017/06/proportional-on-measurement-the-code/
可以在后续有需要的时候添加测试;
*/

#include "pid.h"

void pid_reInitialize(Pid* self);

/*
函数功能:
获取当前系统时间,单位 毫秒;

输入参数:
无

返回:
当前系统时间,单位 毫秒;
*/
static unsigned long pid_millis(void)
{
    struct timeval tv;
    gettimeofday(&tv,NULL);

    return (unsigned long)(tv.tv_sec*1000 + tv.tv_usec/1000);
}


/*Constructor (...)*********************************************************
 *    The parameters specified here are those for for which we can't set up
 *    reliable defaults, so we need to have the user set them.
 ***************************************************************************/

/*
函数功能:
PID数据结构初始化

输入参数:
self     PID结构体指针
input    输入物理量,比如送风温度测量值,不是设定值!!
output   输出物理量,比如阀门开度,或者变频器频率设定值;
setpoint 相对于输入物理量的设定值,比如送风温度"设定值"
kp       PID算法中的算子P
ki       PID算法中的算子i
kd       PID算法中的算子d
controllerDirection  控制方向 供热选择DIRECT,供冷选择REVERSE

返回:
无
*/
void 
pid_init(Pid* self, double* input, double* output, double* setpoint, double kp,
         double ki, double kd, double deadBand, Pid_Direction controllerDirection) {

  self->myOutput = output;
  self->myInput = input;
  self->mySetpoint = setpoint;
  self->inAuto = false;
  self->deadBand = deadBand;

  /*
  需要在本函数结束后调用输出限制函数进行输出量限制;
  */

  /*
  修改默认PID控制loop时间为15秒;
  */
  self->sampleTime = AIRCOND_PIDSAMPLETIME;        //default Controller Sample Time is 15 seconds

  pid_setControllerDirection(self, controllerDirection);
  pid_setTunings(self, kp, ki, kd);

  // fake last sample time
  /*
  采集上一次的时间,单位毫秒;
  */
  self->lastTime = pid_millis() - self->sampleTime;
}

/* Compute() **********************************************************************
 *  This, as they say, is where the magic happens.  this function should be called
 *  every time "void loop()" executes.  the function will decide for itself whether a new
 *  pid Output needs to be computed.  returns true when the output is computed,
 *  false when nothing has been done.
 **********************************************************************************/
/*
函数功能:
PID运算

输入参数:
self  PID结构体指针

返回:
无

备注:
需要在特定的时间点调用,不能频繁调用,基本按照1个sampletime调用1次;
*/
bool 
pid_compute(Pid* self) {

  if (!self->inAuto)
    return false;

  unsigned long now = pid_millis();
  unsigned long timeChange = (now - self->lastTime);

  if (timeChange >= self->sampleTime) {

    /*Compute all the working error variables*/
    double input = *(self->myInput);
    double error = *(self->mySetpoint) - input;

    if(abs(error) <= self->deadBand){
      error = 0;
    }

    self->iTerm += (self->ki * error);
    if (self->iTerm > self->outMax)
      self->iTerm = self->outMax;
    else if (self->iTerm < self->outMin)
      self->iTerm = self->outMin;
    double dInput = (input - self->lastInput);

    /*Compute PID Output*/
    double output = self->kp * error + self->iTerm - self->kd * dInput;

    if (output > self->outMax)
      output = self->outMax;
    else if (output < self->outMin)
      output = self->outMin;

    *(self->myOutput) = output;

    /*Remember some variables for next time*/
    self->lastInput = input;
    self->lastTime = now;
    return true;
  } else
	return false;
}

/* SetTunings(...)*************************************************************
 * This function allows the controller's dynamic performance to be adjusted.
 * it's called automatically from the constructor, but tunings can also
 * be adjusted on the fly during normal operation
 ******************************************************************************/

/*
设置PID算子
*/
void 
pid_setTunings(Pid* self, double kp, double ki, double kd) {

  if (kp < 0 || ki < 0 || kd < 0)
    return;

  self->dispKp = kp;
  self->dispKi = ki;
  self->dispKd = kd;

  double SampleTimeInSec = ((double) self->sampleTime) / 1000;
  self->kp = kp;
  self->ki = ki * SampleTimeInSec;
  self->kd = kd / SampleTimeInSec;

  if (self->controllerDirection == PID_REVERSE) {
    self->kp = (0 - self->kp);
    self->ki = (0 - self->ki);
    self->kd = (0 - self->kd);
  }
}

/* SetSampleTime(...) *********************************************************
 * sets the period, in Milliseconds, at which the calculation is performed
 ******************************************************************************/
/*
函数功能:
设置PID loop时间

输入参数:
self           PID结构体指针
newSampleTime  新采样时间,单位毫秒;
返回值:
无
*/
void 
pid_setSampleTime(Pid* self, unsigned long newSampleTime) {

  if (newSampleTime > 0) {
    double ratio = (double) newSampleTime / (double) self->sampleTime;
    self->ki *= ratio;
    self->kd /= ratio;
    self->sampleTime = (unsigned long) newSampleTime;
  }
}

/* setOutputLimits(...)****************************************************
 *     This function will be used far more often than SetInputLimits.  while
 *  the input to the controller will generally be in the 0-1023 range (which is
 *  the default already,)  the output will be a little different.  maybe they'll
 *  be doing a time window and will need 0-8000 or something.  or maybe they'll
 *  want to clamp it from 0-125.  who knows.  at any rate, that can all be done
 *  here.
 **************************************************************************/
/*
设置输出限制,对于阀门控制输出,设置0%~100%,对于变频风机,如何获得相关数值??

self PID结构体指针
newMin  最小输出限制数值
newMax  最大输出限制数值
*/
void 
pid_setOutputLimits(Pid* self, double newMin, double newMax) {

  if (newMin >= newMax)
    return;
  self->outMin = newMin;
  self->outMax = newMax;

  if (self->inAuto) {
    if (*(self->myOutput) > self->outMax)
      *(self->myOutput) = self->outMax;
    else if (*(self->myOutput) < self->outMin)
      *(self->myOutput) = self->outMin;

    if (self->iTerm > self->outMax)
      self->iTerm = self->outMax;
    else if (self->iTerm < self->outMin)
      self->iTerm = self->outMin;
  }
}

/* setMode(...)****************************************************************
 * Allows the controller Mode to be set to PID_MANUAL or PID_AUTOMATIC
 * when the transition from PID_MANUAL to auto occurs, the controller is
 * automatically initialized
 ******************************************************************************/
void 
pid_setMode(Pid* self, Pid_OperationMode newMode) {

  if (newMode == !(self->inAuto)) { /*we just went from PID_MANUAL to auto*/
    pid_reInitialize(self);
  }
  self->inAuto = newMode;
}

/* pid_reInitialize()**********************************************************
 *  does all the things that need to happen to ensure a bumpless transfer
 *  from PID_MANUAL to PID_AUTOMATIC mode.
 ******************************************************************************/
void 
pid_reInitialize(Pid* self) {

  self->iTerm = *(self->myOutput);
  self->lastInput = *(self->myInput);
  if (self->iTerm > self->outMax)
    self->iTerm = self->outMax;
  else if (self->iTerm < self->outMin)
    self->iTerm = self->outMin;
}

/* SetControllerDirection(...)*************************************************
 * The PID will either be connected to a DIRECT acting process (+Output leads
 * to +Input) or a reverse acting process(+Output leads to -Input.)  we need to
 * know which one, because otherwise we may increase the output when we should
 * be decreasing.  This is called from the constructor.
 ******************************************************************************/
/*
设置PID控制方向
*/
void 
pid_setControllerDirection(Pid* self, Pid_Direction direction) {

  if (self->inAuto && direction != self->controllerDirection) {
    self->kp = (0 - self->kp);
    self->ki = (0 - self->ki);
    self->kd = (0 - self->kd);
  }
  self->controllerDirection = direction;
}

/* Status Functions*************************************************************
 * Just because you set the Kp=-1 doesn't mean it actually happened.  these
 * functions query the internal state of the PID.  they're here for display
 * purposes.  this are the functions the PID Front-end uses for example
 ******************************************************************************/
double 
pid_getKp(Pid* self) {
  return self->dispKp;
}

double 
pid_getKi(Pid* self) {
  return self->dispKi;
}

double 
pid_getKd(Pid* self) {
  return self->dispKd;
}

int 
pid_getMode(Pid* self) {
  return self->inAuto ? PID_AUTOMATIC : PID_MANUAL;
}

Pid_Direction 
pid_getDirection(Pid* self) {
  return self->controllerDirection;
}
