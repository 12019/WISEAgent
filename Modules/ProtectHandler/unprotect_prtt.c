#include "ProtectHandler.h"
#include "parser_prtt.h"
#include "unprotect_prtt.h"


//-----------------------------------------------------------------------------
// Local functions declare:
//-----------------------------------------------------------------------------
static CAGENT_PTHREAD_ENTRY(UnProtectActionThreadStart, args);


//-----------------------------------------------------------------------------
// Global functions define:
//-----------------------------------------------------------------------------
void UnProtectAction()
{
	handler_context_t *pProtectHandlerContext = (handler_context_t *)&g_HandlerContex;
	CAGENT_THREAD_HANDLE unProtectActionThreadHandle;
	if (app_os_thread_create(&unProtectActionThreadHandle, UnProtectActionThreadStart, pProtectHandlerContext) != 0)
	{
		ProtectLog(g_loghandle, Warning, " %s> Start unprotect action thread failed!", strPluginName);
	}
}


//-----------------------------------------------------------------------------
// Local functions define:
//-----------------------------------------------------------------------------
static CAGENT_PTHREAD_ENTRY(UnProtectActionThreadStart, args)
{
	CAGENT_HANDLE sadminDisablePrcHandle = NULL;
	char cmdLine[BUFSIZ] = {0};

#ifdef _WIN32
	{
		char regName[] = "SOFTWARE\\McAfee";
		char valueName[] = "LastWarningMsg";
		app_os_set_regLocalMachine_value(regName, valueName, "", 0);
	}
	strcat(cmdLine, "cmd.exe /c sadmin.exe disable");
#else		
	sprintf(cmdLine, "sadmin disable");
#endif /* _WIN32 */
	if(strlen(EncodePassWord))
	{
		strcat(cmdLine, " -z ");
		strcat(cmdLine, EncodePassWord);
	}

	sadminDisablePrcHandle = app_os_CreateProcessWithCmdLineEx(cmdLine);
	if(!sadminDisablePrcHandle)
	{
		ProtectLog(g_loghandle, Warning, " %s> Create sadimin.exe disable process failed!", strPluginName);
		SendReplyFailMsg_Prtt("ProtectActionDisableFail", prtt_unprotect_rep);
	}
	else
	{
		// wait process and get exit code
		if (0 == GetResultFromProcess(sadminDisablePrcHandle))
			SendReplySuccessMsg_Prtt("ProtectActionDisableSuccess",prtt_unprotect_rep);
		else 
			SendReplyFailMsg_Prtt("ProtectActionDisableFail", prtt_unprotect_rep);
	}
	GetProtectionCurStatus();
	return 0;
}

