#ifndef _PROTECT_LOG_H_
#define _PROTECT_LOG_H_

#include <Log.h>

#define LOG_ENABLE
//#define DEF_LOG_MODE    (LOG_MODE_CONSOLE_OUT|LOG_MODE_FILE_OUT)
#define DEF_LOG_MODE    (LOG_MODE_CONSOLE_OUT)

#ifdef  LOG_ENABLE
#define ProtectLog(handle, level, fmt, ...) 	do { if (handle != NULL)   \
		WriteLog(handle, DEF_LOG_MODE, level, fmt, ##__VA_ARGS__); } while(0)
#else
#define ProtectLog(level, fmt, ...)
#endif /* LOG_ENABLE */


/*==============================debug print define===============================*/
/* macro on-off */
#define	Protect_DEBUG
#undef	Protect_DEBUG

#ifdef Protect_DEBUG
#include <stdio.h>
#define	protect_debug_print(fmt, ...)	\
		printf("%s-L%d:"#fmt"\n", __FUNCTION__, __LINE__, ##__VA_ARGS__)
#else
#define	protect_debug_print(fmt, ...)
#endif /* Recovery_DEBUG */
/*================================================================================*/

#endif
