#ifndef _LOG_H_
#define _LOG_H_

typedef void *  LOGHANDLE;

typedef int LOGMODE;

#define LOG_MODE_NULL_OUT         0x00
#define LOG_MODE_CONSOLE_OUT      0x01
#define LOG_MODE_FILE_OUT         0x02

typedef enum LogLevel { 
   Debug = 0,
   Normal, 
   Warning, 
   Error,
   Alarm,
   Fatal
}LogLevel;

#ifdef __cplusplus
extern "C" {
#endif

LOGHANDLE InitLog(char * logFileName);

void UninitLog(LOGHANDLE logHandle);

void WriteLog(LOGHANDLE logHandle, LOGMODE logMode, LogLevel level, const char * format, ...);

int GetLogID(LOGHANDLE logHandle, char * logname);

void WriteIndividualLog(LOGHANDLE logHandle, char* group, LOGMODE logMode, LogLevel level, const char * format, ...);

#ifdef __cplusplus
}
#endif
#endif