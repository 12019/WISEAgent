#include "ProtectHandler.h"
#include "parser_prtt.h"
#include "install_update_prtt.h"
#include "activate_prtt.h"
#include "status_prtt.h"


//-----------------------------------------------------------------------------
// Local functions declare:
//-----------------------------------------------------------------------------
static void GetActionMsg(char * actionMsg);
static void GetLastWarningMsg(char * warningMsg);
static BOOL GetLatestVersion(char * latestVer);


//-----------------------------------------------------------------------------
// Global functions define:
//-----------------------------------------------------------------------------
BOOL GetPrttStatus(prtt_status_t * prttStatus)
{
	BOOL bRet = FALSE;
	if(prttStatus == NULL) 
		return bRet;
	{
		prttStatus->isInstalled = app_os_is_file_exist(InstallPath);
		prttStatus->isProtection = IsProtect();
		if(IsActive())
		{
			prttStatus->isActivated = TRUE;
			prttStatus->isExpired = FALSE;
		}
		else
		{
			prttStatus->isActivated = FALSE;
			prttStatus->isExpired = IsExpired();
		}

		{
			BOOL isNewerVer = FALSE;
			//if(prttStatus->isInstalled) IsExistNewerVersion(&isNewerVer);
			prttStatus->isExistNewerVer = isNewerVer;
		}

		{
			char tmpVersion[32] = {0};
			GetLatestVersion(tmpVersion);
			protect_debug_print("tmpVersion:%s", tmpVersion);
			if(strlen(tmpVersion))
				strcpy(prttStatus->version, tmpVersion);
			else
				strcpy(prttStatus->version, "1.0.0");
		}

		GetLastWarningMsg(prttStatus->lastWarningMsg);

		{//action msg
			char actMsg[256] = {0};		
			GetActionMsg(actMsg);
			memset(prttStatus->actionMsg, 0, sizeof(prttStatus->actionMsg));
			if(IsSolidify())
			{				
				strcpy(prttStatus->actionMsg, "Solidifying");
			}

#ifdef _WIN32
			if(IsInstallAction || IsUpdateAction)
			{			
				if(IsUpdateAction)
					strcpy(prttStatus->actionMsg, "Updating");
				else
					strcpy(prttStatus->actionMsg, "Installing");
			}
#endif /* _WIN32 */

			if(strlen(actMsg) > 0) 
				sprintf(prttStatus->actionMsg, "%s--%s", prttStatus->actionMsg, actMsg);

#ifdef _WIN32
			if(IsInstallerRunning())
			{
				if(strlen(actMsg) > 0) 
					strcat(prttStatus->actionMsg, ",");
				else
					strcat(prttStatus->actionMsg, "--");
				strcat(prttStatus->actionMsg, "Installer running");
			}		
#endif /* _WIN32 */

			if(!strlen(prttStatus->actionMsg)) 
				strcpy(prttStatus->actionMsg, "None");
		}		
	}
	return bRet = TRUE;
}

void GetProtectionCurStatus()
{
	//handler_context_t *pProtectHandlerContext = (handler_context_t *)&g_HandlerContex;
	prtt_status_t prttStatus;
	memset(&prttStatus, 0, sizeof(prttStatus));
	GetPrttStatus(&prttStatus);
	{
		char * uploadRepJsonStr = NULL;
		int jsonStrlen = Parser_PackPrttStatusRep(&prttStatus, &uploadRepJsonStr);
		if(jsonStrlen > 0 && uploadRepJsonStr != NULL)
		{
			g_sendcbf(&g_PluginInfo, prtt_status_rep, uploadRepJsonStr, strlen(uploadRepJsonStr)+1, NULL, NULL);
		}
		if (uploadRepJsonStr)	free(uploadRepJsonStr);
	}
}

BOOL IsSolidify()
{
	BOOL bRet = FALSE;
#ifdef _WIN32
	char mcafeeScanCmd[32] = "sadmin.exe so";
#else
	char mcafeeScanCmd[32] = "sadmin so";
#endif /* _WIN32 */
	bRet = app_os_process_check(mcafeeScanCmd);
	app_os_sleep(50);
	bRet = app_os_process_check(mcafeeScanCmd);

	return bRet;
}

BOOL IsProtect()
{
#ifdef _WIN32
	BOOL bRet = FALSE;
	char regName[] = "SYSTEM\\CurrentControlSet\\services\\swin\\Parameters";
	char valueName[] = "RTEMode";
	DWORD valueDW = 0;
	DWORD  valueDataSize = sizeof(valueDW);
	if(app_os_get_regLocalMachine_value(regName, valueName, &valueDW, sizeof(valueDW)))
	{
		bRet = (valueDW == 0 ? FALSE : TRUE);
	}
	return bRet;
#else
	return McAfeeStatus;
#endif /* _WIN32 */
}



//-----------------------------------------------------------------------------
// Local functions define:
//-----------------------------------------------------------------------------
static void GetActionMsg(char * actionMsg)
{
	if(actionMsg == NULL) return;
	{
		if(strlen(ActionMsg))
		{
			app_os_mutex_lock(&g_HandlerContex.actionMsgMutex);
			memcpy(actionMsg, ActionMsg, strlen(ActionMsg)+1);
			memset(ActionMsg, 0, sizeof(ActionMsg));
			app_os_mutex_unlock(&g_HandlerContex.actionMsgMutex);
		}
	}
}

static void GetLastWarningMsg(char * warningMsg)
{
	if(warningMsg == NULL) return;
	{
		if(strlen(LastWarningMsg))
		{
			app_os_mutex_lock(&g_HandlerContex.warningMsgMutex);
			memcpy(warningMsg, LastWarningMsg, strlen(LastWarningMsg)+1);
			memset(LastWarningMsg, 0, sizeof(LastWarningMsg));
			app_os_mutex_unlock(&g_HandlerContex.warningMsgMutex);
		}
		else
		{
			char retMsg[256] = {0};
#ifdef _WIN32
			char valueName[] = "LastWarningMsg";
			char regName[] = "SOFTWARE\\McAfee";
			if(app_os_get_regLocalMachine_value(regName, valueName, retMsg, sizeof(retMsg)))
			{
				app_os_set_regLocalMachine_value(regName, valueName, "", 0);
			}
#endif /* _WIN32 */
			if(strlen(retMsg))
				strcpy(warningMsg, retMsg);
			else
				strcpy(warningMsg, "None");
		}
	}
}

static BOOL GetLatestVersion(char * latestVer)
{
	BOOL bRet = FALSE;
	if(latestVer == NULL) return bRet;
#ifdef _WIN32
	{
		char regName[] = "SOFTWARE\\McAfee";
		char valueName[] = "Version";
		char latestVerTmp[32] = {0};
		if(app_os_get_regLocalMachine_value(regName, valueName, latestVerTmp, sizeof(latestVerTmp)))
		{
			strcpy(latestVer, latestVerTmp);
			bRet = TRUE;
		}
	}
#else
	if (IsInstalled())
	{
		FILE *fd = popen("sadmin version", "r");
		if (fd)
		{
			char buf[1024] = {0};
			char *p = NULL;
			while (fgets(buf, sizeof(buf), fd))
			{
				if (strstr(buf, "Version"))
				{
					strtok(buf, " ");
					p = strtok(NULL, " ");
					if (p)
					{
						strcpy(latestVer, p);
						bRet = TRUE;
						protect_debug_print("McAfee version: %s", latestVer);
					}
					break;
				}
			}
			pclose(fd);
		}
		else
		{
			perror("popen");
		}
	}
#endif /* _WIN32 */
	return bRet;
}
