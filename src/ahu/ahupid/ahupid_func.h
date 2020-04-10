#ifndef __AIRPID_FUNC__
#define __AIRPID_FUNC__

#include "ahudata.h"

void PidInitHmdt(AppAHUDev_l *node);
void PidInitTemp(AppAHUDev_l *node);
void PidInitCO2(AppAHUDev_l *node);
void AirCondHmdtSM(AppAHUDev_l *node);
void AirCondHmdtAM(AppAHUDev_l *node);
void AirCondTempAM(AppAHUDev_l *node);

#endif  //__AIRPID_FUNC__