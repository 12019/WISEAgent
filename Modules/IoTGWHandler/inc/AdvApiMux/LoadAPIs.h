#ifndef  __LOAD_ADV_API_H__
#define __LOAD_ADV_API_H__

#include "../SensorNetwork_Manager_API.h"

#define MAX_FN_NAME 64
#define MAX_FNS			7
#define MAX_PAS            5

#define MAX_RECV_BUF  4096
#define MAX_SEND_BUF 4096

#define Fn0						SN_Manager_Initialize_API
#define Fn1						SN_Manager_Uninitialize_API
#define Fn2						SN_Manager_GetVersion_API
#define Fn3						SN_Manager_ActionProc_API
#define Fn4						SN_Manager_GetCapability_API
#define Fn5						SN_Manager_GetData_API
#define Fn6						SN_Manager_SetData_API

#define Fn2P1                  char*
#define Fn2P2                  int

#define Fn3P1                  int
#define Fn3P2                  ReportSNManagerDataCbf
#define Fn3P3                  void*
#define Fn3P4                  void*
#define Fn3P5                  void*

#define Fn5Ps                    2
#define Fn5P1                  char*
#define Fn5P2                  ACTION_MODE
	
#define Fn6Ps					3
#define Fn6P1					char*
#define Fn6P2					char*
#define Fn6P3					void*






static const char *g_szFnName[ ] ={		"SN_Manager_Initialize", 
																	"SN_Manager_Uninitialize",
																	"SN_Manager_GetVersion",
																	"SN_Manager_ActionProc",
																	"SN_Manager_GetCapability",	
																	"SN_Manager_GetData",	
																	"SN_Manager_SetData",	
																};







#endif __LOAD_ADV_API_H__
