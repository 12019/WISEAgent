#ifndef _MONITOR_LOG_H_
#define _MONITOR_LOG_H_

#include <Log.h>

#define DEF_LOG_NAME    "MonitorLog.txt"   //default log file name
#define LOG_ENABLE
//#define DEF_LOG_MODE    (LOG_MODE_NULL_OUT)
//#define DEF_LOG_MODE    (LOG_MODE_FILE_OUT)
#define DEF_LOG_MODE    (LOG_MODE_CONSOLE_OUT|LOG_MODE_FILE_OUT)

#ifdef LOG_ENABLE
#define MonitorLog(handle, level, fmt, ...)  do { if (handle != NULL)   \
	WriteLog(handle, DEF_LOG_MODE, level, fmt, ##__VA_ARGS__); } while(0)
#else
#define MonitorLog(level, fmt, ...)
#endif

//#define LINUX_DEBUG_CHU
#ifdef LINUX_DEBUG_CHU
#define TRACE_LOG(enable,format,...) {if (enable) printf("=== METHOD[%s] LINE[%4d] ==="format"\n",__FUNCTION__, __LINE__, ##__VA_ARGS__);}
#else
#define TRACE_LOG(format,...)
#endif


#endif