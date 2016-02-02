#ifndef _SERVER_REDUNDANCY_LOG_H_
#define _SERVER_REDUNDANCY_LOG_H_

#include "Log.h"

#define DEF_LOG_NAME    "ServerRedundancyLog.txt"   //default log file name
#define LOG_ENABLE
//#define DEF_LOG_MODE    (LOG_MODE_NULL_OUT)
//#define DEF_LOG_MODE    (LOG_MODE_FILE_OUT)
#define DEF_LOG_MODE    (LOG_MODE_CONSOLE_OUT|LOG_MODE_FILE_OUT)

#ifdef LOG_ENABLE
#define ServerRedundancyLog(handle, level, fmt, ...)  do { if (handle != NULL)   \
	WriteLog(handle, DEF_LOG_MODE, level, fmt, ##__VA_ARGS__); } while(0)
#else
#define TerminalLog(level, fmt, ...)
#endif

#endif