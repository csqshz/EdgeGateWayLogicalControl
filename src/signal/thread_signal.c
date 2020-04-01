#include "thread_signal.h"

/* 参数：
	bool relativeTimespan: 
		1:使用相对时间用来判断是否超时:
			从调用wait时开始计时
		0: 不使用相对时间
*/
int ThreadSignal_Init(ThreadSignal_t *signal, bool relativeTimespan)
{
	int ret;
    //relativeTimespan 是不是采用相对时间等待。参见函数 ThreadSignal_Wait
    signal->relativeTimespan = relativeTimespan;

    pthread_mutex_init(&signal->mutex, NULL);

    if (relativeTimespan){
        //如果采用相对时间等待，需要额外的处理。
        //采用相对时间等待。可以避免：因系统调整时间，导致等待时间出现错误。
        ret = pthread_condattr_init(&signal->cattr);
        ret = pthread_condattr_setclock(&signal->cattr, CLOCK_MONOTONIC);

        ret = pthread_cond_init(&signal->cond, &signal->cattr);
    }else{
        pthread_cond_init(&signal->cond, NULL);
    }
	
	return ret;
}

/* 销毁条件变量 */
void ThreadSignal_Close(ThreadSignal_t *signal)
{
    if (signal->relativeTimespan){
        pthread_condattr_destroy(&(signal->cattr));
    }

    pthread_mutex_destroy(&signal->mutex);
    pthread_cond_destroy(&signal->cond);
}

/* 调用pthread_cond_timedwait阻塞等待信号 
 * 参数：
 *		ms：超时等待时间
			>0 使用pthread_cond_timedwait
			<0 使用pthread_cond_wait
 */
int ThreadSignal_Wait(ThreadSignal_t *signal, int ms)
{
	int ret;

    pthread_mutex_lock(&signal->mutex);
	// 使用pthread_cond_timedwait
	if(ms >= 0){
		if (signal->relativeTimespan){
			//获取时间
			struct timespec outtime;
			clock_gettime(CLOCK_MONOTONIC, &outtime);
			//ms为毫秒，换算成秒
			outtime.tv_sec += ms/1000;
			
			//在outtime的基础上，增加ms毫秒
			//outtime.tv_nsec为纳秒，1微秒=1000纳秒
			//tv_nsec此值再加上剩余的毫秒数 ms%1000，有可能超过1秒。需要特殊处理
			unsigned int us = outtime.tv_nsec/1000 + 1000 * (ms % 1000); //微秒
			//us的值有可能超过1秒，
			outtime.tv_sec += us / 1000000; 

			us = us % 1000000;
			outtime.tv_nsec = us * 1000;//换算成纳秒

			ret = pthread_cond_timedwait(&signal->cond, &signal->mutex, &outtime);
		}else{
			struct timeval now;
			gettimeofday(&now, NULL);

			//在now基础上，增加ms毫秒
			struct timespec outtime;
			outtime.tv_sec = now.tv_sec + ms / 1000;

			//us的值有可能超过1秒
			unsigned int us = now.tv_usec + 1000 * (ms % 1000); 
			outtime.tv_sec += us / 1000000; 

			us = us % 1000000;
			outtime.tv_nsec = us * 1000;

			ret = pthread_cond_timedwait(&signal->cond, &signal->mutex, &outtime);
		}
	}else{
		ret = pthread_cond_wait(&signal->cond, &signal->mutex);
	}
    pthread_mutex_unlock(&signal->mutex);
	
	return ret;
}

/* 向pthread_cond_timedwait发送信号 */
void ThreadSignal_Signal(ThreadSignal_t *signal)
{
    pthread_mutex_lock(&signal->mutex);
    pthread_cond_signal(&signal->cond);
    pthread_mutex_unlock(&signal->mutex);
}