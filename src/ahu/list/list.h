#include <stdbool.h>
#include "data.h"

#ifndef _LIST_H_
#define _LIST_H_

AppAirCondDev_l *InitAirCondListHead();
bool IsExistInAirCondDevList(int);
void DevertAirCondDevList(AppAirCondDev_l *node);
void AirCondDevListinit();

AppAirCondDev_l *NewAirCondDevNode(int deviceID);
int QueryIntValFromAirCondList(char *name, AppAirCondDev_t *Dev);

double QueryDoubleValFromAirCondList(char *name, AppAirCondDev_t *Dev);
double *QueryDoublePtrFromAirCondList(char *name, AppAirCondDev_t *Dev);

int QueryDeviceKeyFromAirCondList(char *name, AppAirCondDev_t *Dev);

int IsPointExist(char *name, AppAirCondDev_t *Dev);
void SetVal2AirCondList(char *name, AppAirCondDev_t *Dev, DataType_u val);

void DelAirCondDevList(AppAirCondDev_l *node);

#endif //_LIST_H_