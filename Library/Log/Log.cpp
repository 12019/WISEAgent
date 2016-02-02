#include "log4z.h"
#include "Log.h"
#include "util_path.h"
#include <stdarg.h>

using namespace zsummer::log4z;
static ILog4zManager* g_logger = NULL;

static void _WriteLog(LOGHANDLE logHandle, LoggerId id, ENUM_LOG_LEVEL level, char* log)
{
	if(!logHandle)
		return;
	g_logger = (ILog4zManager*)logHandle;
	if (g_logger->prePushLog(id,level))
	{
		char logBuf[LOG4Z_LOG_BUF_SIZE]={0};
		zsummer::log4z::Log4zStream ss(logBuf, LOG4Z_LOG_BUF_SIZE);
		ss << log;
		g_logger->pushLog(id, level, logBuf, NULL, NULL);
	}
}

static void WriteLogFile(LOGHANDLE logHandle, int id, LOGMODE logMode, LogLevel logLevel, const char * format, va_list ap)
{
    char logStr[2048] = {0};
    if(logHandle == NULL) return;
    vsnprintf(logStr, sizeof(logStr), format, ap);
	switch(logLevel)
    {
	case Debug:
	  {
		_WriteLog(logHandle, (LoggerId)id, LOG_LEVEL_DEBUG, logStr);
		break;
	  }
    case Normal:
	  {
		_WriteLog(logHandle, (LoggerId)id, LOG_LEVEL_INFO, logStr);
		break;
	  }
	case Warning:
      {
         _WriteLog(logHandle, (LoggerId)id, LOG_LEVEL_WARN, logStr);
         break;
      }
   case Error:
      {
         _WriteLog(logHandle, (LoggerId)id, LOG_LEVEL_ERROR, logStr);
         break;
      }
   case Alarm:
      {
         _WriteLog(logHandle, (LoggerId)id, LOG_LEVEL_ALARM, logStr);
         break;
      }
   case Fatal:
      {
         _WriteLog(logHandle, (LoggerId)id, LOG_LEVEL_FATAL, logStr);
         break;
      }
   default:
      break;
   }
}

LOGHANDLE InitLog(char * logFileName)
{
	LOGHANDLE logHandle = NULL;
	if(!g_logger)
	{
		//char path [MAX_PATH] = {0};
		//char filename[MAX_PATH] = {0};
		char configpath[MAX_PATH] = {0};
		//split_path_file(logFileName, path, filename);
		util_path_combine(configpath, logFileName, "logger.ini");
		ILog4zManager::getRef().config(configpath);
		logHandle = g_logger=ILog4zManager::getPtr();
		//logHandle = (LOGHANDLE)g_logger->findLogger("agent");
		//start log4z
		g_logger->start();
		//hot update configure
		g_logger->setAutoUpdate(10);
	}
   return logHandle;
}

int GetLogID(LOGHANDLE logHandle,char * logname)
{
	LoggerId id = 0;
	
	if(logHandle)
	{
		g_logger = (ILog4zManager*)logHandle;
		id = g_logger->findLogger(logname);
		if(id<0)
			id = g_logger->findLogger("agent");
	}
	if(id<0)
		id = 0;
	return id;
}

void UninitLog(LOGHANDLE logHandle)
{
	if(logHandle)
		((ILog4zManager*)logHandle)->stop();
}

void WriteLog(LOGHANDLE logHandle, LOGMODE logMode, LogLevel level, const char * format, ...)
{
	int id = -1;
	va_list ap;
    va_start(ap, format);
	id = GetLogID(logHandle, "agent");
    WriteLogFile(logHandle, id, logMode, level, format, ap);
    va_end(ap);
}

void WriteIndividualLog(LOGHANDLE logHandle, char* group, LOGMODE logMode, LogLevel level, const char * format, ...)
{
	int id = -1;
	va_list ap;
    va_start(ap, format);
	id = GetLogID(logHandle, group);
    WriteLogFile(logHandle, id, logMode, level, format, ap);
    va_end(ap);
}
