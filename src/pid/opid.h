#include <stdio.h>
#include <sys/time.h>
#include <stdbool.h>

void Pid_SetControllerDirection(int Direction);
void Pid_Initialize();
void Pid_SetMode(int Mode);
void Pid_SetOutputLimits(double Min, double Max);
void Pid_SetSampleTime(int NewSampleTime);
void Pid_SetTunings(double Kp, double Ki, double Kd, int pOn);
void Pid_Compute();