#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <json-c/json_util.h>
#include <json-c/json.h>
#include "data.h"
#include "proc.h"
#include "data.h"

#ifndef _APP_H_
#define _APP_H_

void *AirCondCtrlStart_thread(void *arg);
void *AirCondCtrlTemp_thread(void *arg);
void *AirCondCtrlHmdt_thread(void *arg);
void *AirCondCtrlCo2_thread(void *arg);
void *AirCondCtrlRt_thread(void *arg);

#endif //_APP_H_