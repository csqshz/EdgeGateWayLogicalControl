#include <stdio.h>
#include <stdarg.h>
#include <time.h>

#ifndef _ES_PRINT_H_
#define _ES_PRINT_H_

#define INFO_OUTPUT         3
#define WARNING_OUTPUT      2
#define DEBUG_OUTPUT        1
#define ERROR_OUTPUT        0

#define DEBUG_LEVEL         INFO_OUTPUT

#define __PRINTTIME	\
	{\
		time_t raw; \
		struct tm p; \
		raw=time(NULL); \
		localtime_r(&raw, &p); \
		printf("%d-%d-%d ",(1900+p.tm_year), (p.tm_mon+1), p.tm_mday);	\
		printf("%d:%d:%d ", p.tm_hour, p.tm_min, p.tm_sec); \
	}

#define ES_PRT_INFO(info,...)    \
do{ \
    if(DEBUG_LEVEL>=INFO_OUTPUT){\
		__PRINTTIME	\
        printf("\033[36mINFO %s:%s():%d:\033[0m "info"\n",__FILE__,__FUNCTION__,__LINE__,##__VA_ARGS__);}\
}while(0)

#define ES_PRT_WARN(info,...)    \
do{ \
    if(DEBUG_LEVEL>=WARNING_OUTPUT){\
		__PRINTTIME	\
        printf("\033[33mWARN %s:%s():%d:\033[0m "info"\n",__FILE__,__FUNCTION__,__LINE__,##__VA_ARGS__);}\
}while(0)

#define ES_PRT_DEBUG(info,...)   \
do{ \
    if(DEBUG_LEVEL>=DEBUG_OUTPUT){\
		__PRINTTIME	\
        printf("\033[35mDEBUG %s:%s():%d:\033[0m "info"\n",__FILE__,__FUNCTION__,__LINE__,##__VA_ARGS__);}\
}while(0)

#define ES_PRT_ERROR(info,...)   \
do{ \
    if(DEBUG_LEVEL>=ERROR_OUTPUT){\
		__PRINTTIME	\
        printf("\033[31mERROR %s:%s():%d:\033[0m "info"\n",__FILE__,__FUNCTION__,__LINE__,##__VA_ARGS__);}\
}while(0)

#endif	//_ES_PRINT_H_
