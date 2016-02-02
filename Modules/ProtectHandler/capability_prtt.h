#ifndef __CAPABILITY_PRTT_H__
#define __CAPABILITY_PRTT_H__

#include "ProtectHandler.h"
#include "parser_prtt.h"
#include "status_prtt.h"

//The linux OS of RecoveryHandler support 
#define PRTT_LINUX_OS_SUPPORT_LIST		"CentOS6.5"

//Capability: function flags 
#define PRTT_NONE_FUNC_FLAG				0x00          //none
#define PRTT_PROTECT_FUNC_FLAG			0x01          //protect 
#define PRTT_UNPROTECT_FUNC_FLAG		0x02          //unprotect 
#define PRTT_REMOTEINSTALL_FUNC_FLAG    0x04          //remote install
#define PRTT_REMOTEUPDATE_FUNC_FLAG     0x08          //remote update
#define PRTT_REMOTEACTIVATE_FUNC_FLAG   0x10          //remote activate

//Function support according to OS
#define PRTT_NONE_FUNC_LIST		"none"

#define PRTT_LINUX_FUNC_CODE	(PRTT_PROTECT_FUNC_FLAG | \
								 PRTT_UNPROTECT_FUNC_FLAG) 
#define PRTT_LINUX_FUNC_LIST	 "protect,unprotect"

#define PRTT_WIN_FUNC_CODE		(PRTT_LINUX_FUNC_CODE | \
								 PRTT_REMOTEINSTALL_FUNC_FLAG | \
								 PRTT_REMOTEUPDATE_FUNC_FLAG | \
								 PRTT_REMOTEACTIVATE_FUNC_FLAG) 
#define PRTT_WIN_FUNC_LIST		 "protect,unprotect,remoteinstall,remoteupdate,remoteactivate"

//Capability decide
#ifdef _WIN32
#define PRTT_FUNC_CODE	PRTT_WIN_FUNC_CODE
#define PRTT_FUNC_LIST	PRTT_WIN_FUNC_LIST

#else

#include <string.h>
//The macro 'PRTT_LINUX_OS_VERSION' is define in Makefile of ProtectHandler project
#define PRTT_FUNC_TEST  strstr(PRTT_LINUX_OS_SUPPORT_LIST, PRTT_LINUX_OS_VERSION)
#define PRTT_FUNC_CODE	(PRTT_FUNC_TEST ? PRTT_LINUX_FUNC_CODE : PRTT_NONE_FUNC_FLAG)
#define PRTT_FUNC_LIST	(PRTT_FUNC_TEST ? PRTT_LINUX_FUNC_LIST : PRTT_NONE_FUNC_LIST)
#endif /*_WIN32*/

typedef struct{
	const char *pPrttFuncList;
	int prttFuncCode;
	prtt_status_t prtt_status;
}prtt_capability_t;

typedef enum{
	prtt_capability_cmd_way = 0,
	prtt_capability_api_way,
}prtt_capability_get_way_t;

extern int GetPrttCapability(prtt_capability_get_way_t way, char ** pOutReply);

#endif /*__CAPABILITY_PRTT_H__*/