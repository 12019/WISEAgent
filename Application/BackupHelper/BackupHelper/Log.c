#include "Log.h"
#include <stdio.h>
#include <stdarg.h>
#include <memory.h>
#include <time.h>

static void WriteLogFile(LOGHANDLE logHandle, LOGMODE logMode, LogLevel logLevel, const char * format, va_list ap);

static void WriteLogFile(LOGHANDLE logHandle, LOGMODE logMode, LogLevel logLevel, const char * format, va_list ap)
{
   char logLine[4096] = {0};
   char logStr[2048] = {0};
   time_t curTime = 0;
   struct tm *pCurTm = NULL;
   char timeStr[32] = {0};
   char levelStr[16] = {0};
   if(logHandle == NULL) return;
   _vsnprintf(logStr, sizeof(logStr), format, ap);
   curTime = time(0);
   pCurTm = localtime(&curTime);
   strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", pCurTm);
   switch(logLevel)
   {
   case Normal:
      {
         memcpy(levelStr, "Normal", sizeof("Normal"));
         break;
      }
   case Warning:
      {
         memcpy(levelStr, "Warning", sizeof("Warning"));
         break;
      }
   case Error:
      {
         memcpy(levelStr, "Error]", sizeof("Error"));
         break;
      }
   default:
      break;
   }

   sprintf_s(logLine, sizeof(logLine), "[%s][%s]: %s\r\n",timeStr, levelStr, logStr);

   if(logMode & LOG_MODE_CONSOLE_OUT)
   {
      printf("%s", logLine);
   }

   if(logMode & LOG_MODE_FILE_OUT)
   {
      fputs(logLine, logHandle);
      fflush(logHandle);
   }
}

LOGHANDLE InitLog(char * logFileName)
{
   LOGHANDLE logHandle = NULL;
   if(logFileName == NULL) return logHandle;
   {
      FILE * logFile = NULL;
      logFile = fopen(logFileName, "ab");
      if(logFile != NULL) logHandle = (LOGHANDLE)logFile;
   }
   return logHandle;
}

void UninitLog(LOGHANDLE logHandle)
{
   fclose(logHandle);
}

void WriteLog(LOGHANDLE logHandle, LOGMODE logMode, LogLevel level, const char * format, ...)
{
   va_list ap;
   va_start(ap, format);
   WriteLogFile(logHandle, logMode, level, format, ap);
   va_end(ap);
}