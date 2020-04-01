#include <pthread.h>
#include <stdio.h>
#include <stdbool.h>
#include <sys/time.h>

//函数涉及的变量
typedef struct _ThreadSignal_t{
    bool relativeTimespan;  //是否采用相对时间

    pthread_cond_t cond;
    pthread_mutex_t mutex;

    pthread_condattr_t cattr;

}ThreadSignal_t;

//初始化
int ThreadSignal_Init(ThreadSignal_t *signal, bool relativeTimespan);
//关闭
void ThreadSignal_Close(ThreadSignal_t *signal);

//等待ms毫秒
int ThreadSignal_Wait(ThreadSignal_t *signal, int ms);
//唤醒线程
void ThreadSignal_Signal(ThreadSignal_t *signal);