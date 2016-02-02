#ifndef __CAPABILITY_RCVY_H__
#define __CAPABILITY_RCVY_H__

#include "RecoveryHandler.h"
#include "parser_rcvy.h"
#include "status_rcvy.h"

//The linux OS of RecoveryHandler support 
#define RCVY_LINUX_OS_SUPPORT_LIST		"OpenSUSE12.3,CentOS6.5,Ubuntu14.04,Ubuntu12.04"

//Capability: function flags 
#define RCVY_NONE_FUNC_FLAG				0x00          //none
#define RCVY_BACKUP_FUNC_FLAG			0x01          //backup
#define RCVY_RECOVERY_FUNC_FLAG			0x02          //recovery
#define RCVY_REMOTEINSTALL_FUNC_FLAG    0x04          //remote install
#define RCVY_REMOTEUPDATE_FUNC_FLAG     0x08          //remote update
#define RCVY_REMOTEACTIVATE_FUNC_FLAG   0x10          //remote activate

//Function support according to OS
#define RCVY_NONE_FUNC_LIST		"none"

#define RCVY_LINUX_FUNC_CODE	(RCVY_BACKUP_FUNC_FLAG | \
								 RCVY_RECOVERY_FUNC_FLAG) 
#define RCVY_LINUX_FUNC_LIST	 "backup,recovery"

#define RCVY_WIN_FUNC_CODE		(RCVY_LINUX_FUNC_CODE | \
								 RCVY_REMOTEINSTALL_FUNC_FLAG | \
								 RCVY_REMOTEUPDATE_FUNC_FLAG | \
								 RCVY_REMOTEACTIVATE_FUNC_FLAG) 
#define RCVY_WIN_FUNC_LIST		 "backup,recovery,remoteinstall,remoteupdate,remoteactivate"

//Capability decide
#ifdef _WIN32
#define RCVY_FUNC_CODE	RCVY_WIN_FUNC_CODE
#define RCVY_FUNC_LIST	RCVY_WIN_FUNC_LIST

#else

#include <string.h>
#define RCVY_FUNC_TEST  strstr(RCVY_LINUX_OS_SUPPORT_LIST, RCVY_LINUX_OS_VERSION)
#define RCVY_FUNC_CODE	(RCVY_FUNC_TEST ? RCVY_LINUX_FUNC_CODE : RCVY_NONE_FUNC_FLAG)
#define RCVY_FUNC_LIST	(RCVY_FUNC_TEST ? RCVY_LINUX_FUNC_LIST : RCVY_NONE_FUNC_LIST)
#endif /*_WIN32*/

typedef struct{
	const char *pRcvyFuncList;
	int rcvyFuncCode;
	recovery_status_t rcvy_status;
}rcvy_capability_t;

typedef enum{
	rcvy_capability_cmd_way = 0,
	rcvy_capability_api_way,
}rcvy_capability_get_way_t;

extern int GetRcvyCapability(rcvy_capability_get_way_t way, char ** pOutReply);

#endif /*__CAPABILITY_RCVY_H__*/