#ifndef __AIRPID_FUNC__
#define __AIRPID_FUNC__

#include "data.h"

void PidInitHmdt(AppAirCondDev_l *node);
void PidInitTemp(AppAirCondDev_l *node);
void PidInitCO2(AppAirCondDev_l *node);
void AirCondHmdtSM(AppAirCondDev_l *node);
void AirCondHmdtAM(AppAirCondDev_l *node);
void AirCondTempAM(AppAirCondDev_l *node);

#endif  //__AIRPID_FUNC__