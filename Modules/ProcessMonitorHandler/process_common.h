#ifndef _PROCESS_COMMON_H_
#define _PROCESS_COMMON_H_
#include "Parser.h"
#include "susiaccess_handler_api.h"

Handler_info  g_PluginInfo;
sw_mon_handler_context_t  SWMonHandlerContext;
BOOL SetSWMThrThreadRunning;
CAGENT_THREAD_HANDLE SetSWMThrThreadHandle;// = NULL;

HandlerSendCbf  g_sendcbf;						// Client Send information (in JSON format) to Cloud Server	
HandlerSendCustCbf  g_sendcustcbf;			    // Client Send information (in JSON format) to Cloud Server with custom topic	
HandlerAutoReportCbf g_sendreportcbf;				// Client Send report (in JSON format) to Cloud Server with AutoReport topic	
HandlerSendCapabilityCbf g_sendcapabilitycbf;	
HandlerSubscribeCustCbf g_subscribecustcbf;

#endif