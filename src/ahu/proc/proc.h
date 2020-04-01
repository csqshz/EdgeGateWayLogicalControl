#ifndef __PROC_H__
#define __PROC_H__

#include <json-c/json_util.h>
#include <json-c/json.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include "data.h"

#define SUCCESS 0
#define JFILESIZE_10K		(10*1024)

#ifdef false
#undef false
#endif
#define false (0)

#ifdef true
#undef true
#endif
#define true (1)

#define LOCAL_CONF_DIR   "./localfile"

/* @brief: 将目录LOCAL_CONF_DIR和文件名file组合出完整路径，用FileFullName保存 */
#define COMBINEFULLFILENAME(file, FileFullName) \
    char FileFullName[100]; \
    sprintf(FileFullName, "%s/%s", LOCAL_CONF_DIR, file);

/* @brief：将Dev链表节点中的name点位的值改为val */
#define SETVAL2LIST(name, val, type, Dev) \
{   \
    DataType_u a;   \
    if(type == TypeOfVal_INT){ \
        a.valI = (int)val; \
    }else if(type == TypeOfVal_DOUBLE){  \
        a.valD = (double)val; \
    }   \
    SetVal2AirCondList(name, Dev, a);    \
}

/*
 * @brief：设置点位的值，并发送给下层
 * 
 * @name: 要控制的点位名字
 * @val: 要往这个点位写的值
 * @type: val的数据类型
 * @Dev: 点位属于哪一个实例
 * @cmd: read/write，目前read操作只执行SetVal2AirCondList，不执行SendCmd2Low
 * @templ: 数据报文模板 
 */
#define SETVAL_SENDCMD(name, val, type, Dev, cmd, templ) \
{   \
    DataType_u a;   \
    if(type == TypeOfVal_INT){ \
        a.valI = (int)val; \
    }else{  \
        a.valD = (double)val; \
    }   \
    SetVal2AirCondList(name, Dev, a);    \
    SendCmd2Low(name, Dev, cmd, templ);  \
}

void AddDevFromLocal();
void ExtractPoints(AppAirCondDev_t *, struct json_object *);
void MqttMessProc(char *message);
void MqttCmdMessProc(char *message);
void SendCmd2Low(char *name, AppAirCondDev_t *Dev, enum CmdOper oper, char *template);
int deviceKey2deviceID(int deviceKey);
int GetIntValByKey(struct json_object *jRoot, char *key);
int SaveTime2Local(unsigned int deviceID, unsigned int RunTime);

#endif //__PROC_H__
