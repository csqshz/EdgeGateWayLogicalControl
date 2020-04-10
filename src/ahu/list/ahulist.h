#include <stdbool.h>
#include "ahudata.h"

#ifndef _LIST_H_
#define _LIST_H_

AppAHUDev_l *InitAirCondListHead();
bool IsExistInAHUDevList(int);
void DevertAHUDevList(AppAHUDev_l *node);
void AHUDevListinit();

AppAHUDev_l *NewAHUDevNode(int deviceID);
int QueryIntValFromAirCondList(char *name, AppAHUDev_t *Dev);

double QueryDoubleValFromAirCondList(char *name, AppAHUDev_t *Dev);
double *QueryDoublePtrFromAirCondList(char *name, AppAHUDev_t *Dev);

int QueryDeviceKeyFromAirCondList(char *name, AppAHUDev_t *Dev);

int IsPointExist(char *name, AppAHUDev_t *Dev);
void SetVal2AirCondList(char *name, AppAHUDev_t *Dev, DataType_u val);

void DelAHUDevList(AppAHUDev_l *node);

#endif //_LIST_H_