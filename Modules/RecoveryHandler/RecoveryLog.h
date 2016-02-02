#ifndef _RECOVETY_LOG_H_
#define _RECOVETY_LOG_H_

#include <Log.h>

#define DEF_LOG_NAME    "RecoveryLog.txt"   //default log file name
#define LOG_ENABLE
//#define DEF_LOG_MODE    (LOG_MODE_NULL_OUT)
//#define DEF_LOG_MODE    (LOG_MODE_FILE_OUT)
#define DEF_LOG_MODE    (LOG_MODE_CONSOLE_OUT|LOG_MODE_FILE_OUT)

#ifdef LOG_ENABLE
#define RecoveryLog(handle, level, fmt, ...)  do { if (handle != NULL)   \
	WriteLog(handle, DEF_LOG_MODE, level, fmt, ##__VA_ARGS__); } while(0)
#else
#define RecoveryLog(level, fmt, ...)
#endif

/*==============================debug print define===============================*/
/* macro on-off */
#define	Recovery_DEBUG
#undef	Recovery_DEBUG

#ifdef Recovery_DEBUG
#include <stdio.h>
#define	Recovery_debug_print(fmt, ...)	\
		printf("%s-L%d:"#fmt"\n", __FUNCTION__, __LINE__, ##__VA_ARGS__)
#else
#define	Recovery_debug_print(fmt, ...)
#endif /* Recovery_DEBUG */
/*================================================================================*/

#endif /* _RECOVETY_LOG_H_ */
