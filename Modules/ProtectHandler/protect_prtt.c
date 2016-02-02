#include "ProtectHandler.h"
#include "parser_prtt.h"
#include "activate_prtt.h"
#include "protect_prtt.h"
#include "common.h"

//-----------------------------------------------------------------------------
// Local functions declare:
//-----------------------------------------------------------------------------
static CAGENT_PTHREAD_ENTRY(ProtectActionThreadStart, args);

//-----------------------------------------------------------------------------
// Global functions define:
//-----------------------------------------------------------------------------
void ProtectAction()
{
	handler_context_t *pProtectHandlerContext = (handler_context_t *)&g_HandlerContex;
	if(IsExpired())
	{
#ifndef _WIN32
		//DeleteLicense();
#endif
		SendReplyMsg_Prtt("Expired", prtt_expire, prtt_protect_rep);	
	}
	else
	{
		if(app_os_is_file_exist(InstallPath))
		{
			if(!pProtectHandlerContext->isProtectActionRunning)
			{
				CAGENT_THREAD_HANDLE protectActionThreadHandle;
				pProtectHandlerContext->isProtectActionRunning = TRUE;
				if (app_os_thread_create(&protectActionThreadHandle, ProtectActionThreadStart, pProtectHandlerContext) != 0)
				{
					ProtectLog(g_loghandle, Warning, " %s> Start protect action thread failed!", strPluginName);
					pProtectHandlerContext->isProtectActionRunning = FALSE;
				}
			}
			else
			{
				SendReplyMsg_Prtt("ProtectActionRunning", prtt_busy, prtt_protect_rep);			
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Local functions define:
//-----------------------------------------------------------------------------
static CAGENT_PTHREAD_ENTRY(ProtectActionThreadStart, args)
{
	handler_context_t *pProtectHandlerContext = (handler_context_t *)args;
	char cmdLine[BUFSIZ] = {0};
	char sadminOuputPath[MAX_PATH] = {0};
	char sysDir[MAX_PATH] = {0};
	CAGENT_HANDLE sadminPrcHandle = NULL;

	SendReplyMsg_Prtt("ProtectActionStart", prtt_start, prtt_protect_rep);

#ifdef _WIN32
	app_os_GetSystemDirectory(sysDir, sizeof(sysDir));
	strcat(sadminOuputPath, sysDir);
	strcat(sadminOuputPath, DEF_SADMIN_OUTPUT_FILE_NAME);
	strcat(cmdLine, "cmd.exe /c sadmin.exe so ");
#else
	sprintf(sadminOuputPath, "/tmp%s", DEF_SADMIN_OUTPUT_FILE_NAME);
	sprintf(cmdLine, "sadmin so ");
#endif /* _WIN32 */

	if(strlen(EncodePassWord))
	{
		strcat(cmdLine, " -z ");
		strcat(cmdLine, EncodePassWord);

	}
	strcat(cmdLine, " > ");
	strcat(cmdLine, sadminOuputPath);

	SendReplyMsg_Prtt("ProtectActionSolidifyStart", prtt_soStart, prtt_protect_rep);
	pProtectHandlerContext->sadminOutputCheckEnable = TRUE;
	sadminPrcHandle = app_os_CreateProcessWithCmdLineEx(cmdLine);
	if(!sadminPrcHandle)
	{
		ProtectLog(g_loghandle, Warning, " %s> Create sadimin so process failed!", strPluginName);
		pProtectHandlerContext->sadminOutputCheckEnable = FALSE;
		SendReplyMsg_Prtt("ProtectActionSolidifyFail", prtt_soStartFail, prtt_protect_rep);
	}
	else
	{
		int prcExitCode = 0;
		// wait process and get exit code
		prcExitCode = GetResultFromProcess(sadminPrcHandle);

		app_os_sleep(DEF_SADMIN_OUTPUT_CHECK_INTERVAL_MS);
		pProtectHandlerContext->sadminOutputCheckEnable = FALSE;
		if(app_os_is_file_exist(sadminOuputPath))
		{
			app_os_file_remove(sadminOuputPath);
		}
		app_os_sleep(DEF_SADMIN_OUTPUT_CHECK_INTERVAL_MS);


		if(prcExitCode != 0 )
		{
#ifdef _WIN32			
			{//lockdown CLI
				CAGENT_HANDLE sadminLockdownPrcHandle = NULL;
				memset(cmdLine, 0, sizeof(cmdLine));
				sprintf_s(cmdLine, sizeof(cmdLine), "%s", "cmd.exe /c sadmin.exe lockdown");
				sadminLockdownPrcHandle = app_os_CreateProcessWithCmdLineEx(cmdLine);
				if(sadminLockdownPrcHandle)
				{
					DWORD prcExitCode = 0;
					app_os_WaitForSingleObject(sadminLockdownPrcHandle, INFINITE);
					app_os_GetExitCodeProcess(sadminLockdownPrcHandle, &prcExitCode);
					app_os_CloseHandle(sadminLockdownPrcHandle);
					app_os_sleep(DEF_SADMIN_OUTPUT_CHECK_INTERVAL_MS);
					if(prcExitCode == 0)
					{
					}
				}
			}
			{//recover CLI
				CAGENT_HANDLE sadminRecoverPrcHandle = NULL;
				memset(cmdLine, 0, sizeof(cmdLine));
				sprintf_s(cmdLine, sizeof(cmdLine), "%s", "cmd.exe /c sadmin.exe recover -f");
				sadminRecoverPrcHandle = app_os_CreateProcessWithCmdLineEx(cmdLine);
				if(sadminRecoverPrcHandle)
				{
					DWORD prcExitCode = 0;
					app_os_WaitForSingleObject(sadminRecoverPrcHandle, INFINITE);
					app_os_GetExitCodeProcess(sadminRecoverPrcHandle, &prcExitCode);
					app_os_CloseHandle(sadminRecoverPrcHandle);
				}
			}
#endif /* _WIN32 */
			
			SendReplyMsg_Prtt("ProtectActionSolidifyFail, Sadmin abnormal exit!", prtt_soFail, prtt_protect_rep);
		}
		else
		{
			SendReplyMsg_Prtt("ProtectActionSolidifyEnd", prtt_soEnd, prtt_protect_rep);
			
#ifdef _WIN32			
			{//Report McAfee Solidifier log
				CAGENT_THREAD_HANDLE sysLogHandle = app_os_RegisterEventSource(NULL, "McAfee Solidifier");
				if(sysLogHandle)
				{
					const char * pStr[1];
					pStr[0] = "Mcafee Solidify";
					app_os_ReportEvent(sysLogHandle, 91, pStr);
					app_os_DeregisterEventSource(sysLogHandle);
					sysLogHandle = NULL;
				}
			}

			{//setvalue "SOFTWARE\\McAfee"
				char regName[] = "SOFTWARE\\McAfee";
				char valueName[] = "LastWarningMsg";
				app_os_set_regLocalMachine_value(regName, valueName, "", 0);
			}

			{//sadmin.exe add updaters process
				char mcAfeeAddUpdaterPath[MAX_PATH] = {0};
				char moudlePath[MAX_PATH]={0};
				CAGENT_HANDLE mcafeeAddUpdaterPrcHandle = NULL;
				app_os_get_module_path(moudlePath);
				sprintf_s(mcAfeeAddUpdaterPath, sizeof(mcAfeeAddUpdaterPath), "%s%s", moudlePath, "McAfeeAddUpdater.bat");
				memset(cmdLine, 0, sizeof(cmdLine));
				sprintf_s(cmdLine, sizeof(cmdLine), "cmd.exe /c \"%s\"  ", mcAfeeAddUpdaterPath);
				if(strlen(EncodePassWord))
				{
					strcat(cmdLine, EncodePassWord);
				}
				mcafeeAddUpdaterPrcHandle = app_os_CreateProcessWithCmdLineEx(cmdLine);
				if(!mcafeeAddUpdaterPrcHandle)
				{
					ProtectLog(g_loghandle, Warning, " %s> Create sadimin.exe add updaters process failed!", strPluginName);
					SendReplyMsg_Prtt("ProtectActionAddUpdaterFail", \
						prtt_addUpdtWLFail, prtt_protect_rep);
				}
				else
				{
					SendReplyMsg_Prtt("ProtectActionAddUpdaterSuccess",\
						prtt_addUpdtWLOK, prtt_protect_rep);
					app_os_WaitForSingleObject(mcafeeAddUpdaterPrcHandle, INFINITE);
					app_os_CloseHandle(mcafeeAddUpdaterPrcHandle);
				}
			}
#else
			{//sadmin.exe add updaters process
				char mcAfeeAddUpdaterPath[MAX_PATH] = {0};
				char moudlePath[MAX_PATH]={0};
				int result = -1;
				app_os_get_module_path(moudlePath);
				sprintf_s(mcAfeeAddUpdaterPath, sizeof(mcAfeeAddUpdaterPath), "%s%s", moudlePath, "McAfeeAddUpdater.sh");
				memset(cmdLine, 0, sizeof(cmdLine));
				sprintf_s(cmdLine, sizeof(cmdLine), "%s", mcAfeeAddUpdaterPath);
				result = system(cmdLine);
				if(result < 0)
				{
					ProtectLog(g_loghandle, Warning, " %s> Create sadimin.exe add updaters process failed! %s", strPluginName, strerror(errno));
					SendReplyMsg_Prtt("ProtectActionAddUpdaterFail", prtt_addUpdtWLFail, prtt_protect_rep);
				}
				else 
				{
					if(WIFEXITED(result))
					{
						SendReplyMsg_Prtt("ProtectActionAddUpdaterSuccess", prtt_addUpdtWLOK, prtt_protect_rep);
					}
					else if(WIFSIGNALED(result))
					{
						ProtectLog(g_loghandle, Warning, " %s> Create sadimin.exe add updaters process failed! abnormal termination,signal number =%d", strPluginName, WTERMSIG(result));
						SendReplyMsg_Prtt("ProtectActionAddUpdaterFail", prtt_addUpdtWLFail, prtt_protect_rep);

					}
					else if(WIFSTOPPED(result))
					{
						ProtectLog(g_loghandle, Warning, " %s> Create sadimin.exe add updaters process failed! process stopped, signal number =%d", strPluginName, WSTOPSIG(result));
						SendReplyMsg_Prtt("ProtectActionAddUpdaterFail", prtt_addUpdtWLFail, prtt_protect_rep);
					}
				}
			}
#endif /* _WIN32 */

			{//sadmin.exe enable process
				CAGENT_HANDLE sadminEnablePrcHandle = NULL;
				memset(cmdLine, 0, sizeof(cmdLine));
#ifdef _WIN32
				strcat(cmdLine, "cmd.exe /c sadmin.exe enable ");
#else
				sprintf(cmdLine, "sadmin enable ");
#endif /* _WIN32 */
				if(strlen(EncodePassWord))
				{
					strcat(cmdLine, " -z ");
					strcat(cmdLine, EncodePassWord);
				}
				sadminEnablePrcHandle = app_os_CreateProcessWithCmdLineEx(cmdLine);
				if(!sadminEnablePrcHandle)
				{
					ProtectLog(g_loghandle, Warning, " %s> Create sadimin enable process failed!", strPluginName);
					SendReplyFailMsg_Prtt("ProtectActionEnableFail", prtt_protect_rep);
				} 
				else 
				{	
					// wait process and get exit code
					if (0 == GetResultFromProcess(sadminEnablePrcHandle))
						SendReplySuccessMsg_Prtt("ProtectActionEnableSuccess",prtt_protect_rep);
					else 
						SendReplyFailMsg_Prtt("ProtectActionEnableFail", prtt_protect_rep);
				}
			}
		}
	}
	SendReplyMsg_Prtt("ProtectActionEnd", prtt_end, prtt_protect_rep);
	pProtectHandlerContext->isProtectActionRunning = FALSE;
	GetProtectionCurStatus();
	return 0;
}


