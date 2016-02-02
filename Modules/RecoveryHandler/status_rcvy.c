#include "RecoveryHandler.h"
#include "parser_rcvy.h"
#include "backup_rcvy.h"
#include "install_update_rcvy.h"
#include "restore_rcvy.h"
#include "activate_rcvy.h"
#include "status_rcvy.h"
#include "asz_rcvy.h"

//-----------------------------------------------------------------------------
// Local Types/Macros/Variables:
//-----------------------------------------------------------------------------
typedef enum{
   None,
   Backuping,
   Restoring,
}acr_action_status;

#ifndef _is_linux
#define DEF_HOTKEY_ACTIVATE_NAME        "HotKeyActivate"
#define DEF_ACRONIS_UNINSTALLER_NAME    "UninstallAcronis.exe"
static BOOL IsAcrCanUpdate = FALSE;
#else
#define ACRONIS_VERSION_FILE_PATH		"/usr/lib/Acronis/BackupAndRecovery_version.txt"
#define DEF_ACRONIS_CMD_NAME            "acrocmd"
#endif /*_is_linux*/

//-----------------------------------------------------------------------------
// Local functions declare:
//-----------------------------------------------------------------------------
static BOOL IsHotKeyActivateRunning();
static BOOL IsHotKeyDeactivateRunning();
static void GetActionMsg(char * actionMsg);
static void GetLastWarningMsg(char * warningMsg);
static BOOL GetLatestBKTime(char * timeStr);
static BOOL GetLatestVersion(char * latestVer);

//-----------------------------------------------------------------------------
// Global functions define:
//-----------------------------------------------------------------------------
void DeactivateARSM()
{
	RunASRM(FALSE);
}

void RunASRM(BOOL isNeedHotkey)
{
#ifndef _is_linux
	char cmdLine[BUFSIZ] = "cmd.exe /c ";

	if(isNeedHotkey)
	{
		if(app_os_is_file_exist(ActivateHotkeyProPath) && !IsHotKeyActivateRunning())
		{
			strcat(cmdLine, ActivateHotkeyProPath);

			if(!app_os_CreateProcess(cmdLine))
			{
				RecoveryLog(g_loghandle, Normal, "%s()[%d]###Create activate hotkey process failed!\n",__FUNCTION__, __LINE__ );
				return;
			}
		}
	}
	else
	{
		if(app_os_is_file_exist(DeactivateHotkeyProPath) && !IsHotKeyDeactivateRunning())
		{
			strcat(cmdLine, DeactivateHotkeyProPath);

			if(!app_os_CreateProcess(cmdLine))
			{
				RecoveryLog(g_loghandle, Normal, "%s()[%d]###Create deactivate hotkey process failed!\n",__FUNCTION__, __LINE__ );
				return;
			}
		}
	}
#else
	char cmdLine[BUFSIZ] = {0};

	if(isNeedHotkey)
	{
		if(app_os_is_file_exist(AcroCmdExePath) && !IsAcrocmdRunning())
		{
			strcpy(cmdLine, "acrocmd activate asrm");
			if(!app_os_CreateProcess(cmdLine))
			{
				RecoveryLog(g_loghandle, Normal, "%s()[%d]###Create activate hotkey process failed!\n",__FUNCTION__, __LINE__ );
				return;
			}
		}
	}
	else
	{
		if(app_os_is_file_exist(AcroCmdExePath) && !IsAcrocmdRunning())
		{
			strcpy(cmdLine, "acrocmd deactivate asrm");
			if(!app_os_CreateProcess(cmdLine))
			{
				RecoveryLog(g_loghandle, Normal, "%s()[%d]###Create deactivate hotkey process failed!\n",__FUNCTION__, __LINE__ );
				return;
			}
		}
	}
#endif /* _is_linux */
}

BOOL IsAcrocmdRunning()
{
	BOOL bRet = app_os_ProcessCheck(DEF_ACRONIS_CMD_NAME);
#ifndef _is_linux
	bRet = app_os_ProcessCheck(DEF_HOTKEY_ACTIVATE_NAME) ? TRUE : bRet;
#endif /* _is_linux */
	return bRet;
}
BOOL GetRcvyStatus(recovery_status_t * rcvyStatus)
{
#ifndef _is_linux
	BOOL bRet = FALSE;
	if(NULL == rcvyStatus) return bRet;
	{
		rcvyStatus->isInstalled = app_os_is_file_exist(AcroCmdExePath);
	
		{
			char aszExitStatus[16] = {0};
			CheckASZStatus();
			GetASZExistRecord(aszExitStatus);
			if(!_stricmp(aszExitStatus, "True"))
				rcvyStatus->isExistASZ = TRUE;
			else
				rcvyStatus->isExistASZ = FALSE;
		}


		if(IsActive()) 
		{
			rcvyStatus->isActivated = TRUE;
			rcvyStatus->isExpired = FALSE;
		}
		else 
		{
			rcvyStatus->isActivated = FALSE;
			rcvyStatus->isExpired = IsExpired();
		}

		{
			BOOL isNewerVer = FALSE;
			//IsExistNewerVersion(&isNewerVer);
			rcvyStatus->isExistNewerVer = isNewerVer;
		}

		{
			app_os_mutex_lock(&Mutex_AcrReady);
			rcvyStatus->isAcrReady = IsAcrReady;
			app_os_mutex_unlock(&Mutex_AcrReady);
		}

		{
			char tmpVersion[32] = {0};
			GetLatestVersion(tmpVersion);
			if(strlen(tmpVersion))
			{
				strcpy(rcvyStatus->version, tmpVersion);
			}
			else
			{
				strcpy(rcvyStatus->version, "1.0.0");
			}
		}

		GetLastWarningMsg(rcvyStatus->lastWarningMsg);
		GetLatestBKTime(rcvyStatus->lastBackupTime);

		//action msg
		memset(rcvyStatus->actionMsg, 0, sizeof(rcvyStatus->actionMsg));
		if(IsBackuping)
		{
			strcpy(rcvyStatus->actionMsg, "Backuping");
			{
				char backupMsg[256] = {0};
				GetActionMsg(backupMsg);
				if(strlen(backupMsg) > 0) 
					sprintf(rcvyStatus->actionMsg, "%s--%s", rcvyStatus->actionMsg, backupMsg);
			}
		}
		else if(IsRestoring)
		{
			strcpy(rcvyStatus->actionMsg, "Restoring");
			{
				char restoreMsg[1024] = {0};
				GetActionMsg(restoreMsg);
				if(strlen(restoreMsg) > 0) 
					sprintf(rcvyStatus->actionMsg, "%s--%s", rcvyStatus->actionMsg, restoreMsg);
			}
		}
		else if(IsInstallAction || IsUpdateAction)
		{
			if(IsUpdateAction)
			{
				strcpy(rcvyStatus->actionMsg, "Updating");
			}
			else
			{
				strcpy(rcvyStatus->actionMsg, "Installing");
			}

			if(IsDownloadAction)
			{
				char downLoadMsg[512] = {0};
				GetActionMsg(downLoadMsg);
				if(strlen(downLoadMsg) > 0) 
					sprintf(rcvyStatus->actionMsg, "%s--%s", rcvyStatus->actionMsg, downLoadMsg);
			}
			else if(IsInstallerRunning())
			{
				char downLoadMsg[512] = {0};
				GetActionMsg(downLoadMsg);
				if(strlen(downLoadMsg) > 0) 
					sprintf(rcvyStatus->actionMsg, "%s--%s,%s", rcvyStatus->actionMsg, downLoadMsg,"Installer running");
				else 
					sprintf(rcvyStatus->actionMsg, "%s--%s", rcvyStatus->actionMsg, "Installer running");
			}
		}
		if(!strlen(rcvyStatus->actionMsg)) 
			strcpy(rcvyStatus->actionMsg, "None");
	}
	return bRet = TRUE;
#else
	if(NULL == rcvyStatus)
		return FALSE;

	rcvyStatus->isInstalled = app_os_is_file_exist(AcroCmdExePath);
	if(rcvyStatus->isInstalled)
	{
		if(IsActive())
		{
			rcvyStatus->isActivated = TRUE;
			rcvyStatus->isExpired = FALSE;
		}
		else
		{
			rcvyStatus->isActivated = FALSE;
			rcvyStatus->isExpired = IsExpired();
		}
		rcvyStatus->isExistASZ = TRUE;
		rcvyStatus->isExistNewerVer = FALSE;
		
		{
			app_os_mutex_lock(&Mutex_AcrReady);
			rcvyStatus->isAcrReady = IsAcrReady;
			app_os_mutex_unlock(&Mutex_AcrReady);
		}

		{
			char tmpVersion[32] = {0};
			if (GetLatestVersion(tmpVersion))
			{
				strcpy(rcvyStatus->version, tmpVersion);
			}
			else
			{
				strcpy(rcvyStatus->version, "1.0.0");
			}
		}

		GetLastWarningMsg(rcvyStatus->lastWarningMsg);
		GetLatestBKTime(rcvyStatus->lastBackupTime);

		if(IsBackuping)
		{
			strcpy(rcvyStatus->actionMsg, "Backuping");
			{
				char backupMsg[256] = {0};
				GetActionMsg(backupMsg);
				if(strlen(backupMsg) > 0) 
					sprintf(rcvyStatus->actionMsg, "%s--%s", rcvyStatus->actionMsg, backupMsg);
			}
		}
		else if(IsRestoring)
		{
			strcpy(rcvyStatus->actionMsg, "Restoring");
			{
				char restoreMsg[1024] = {0};
				GetActionMsg(restoreMsg);
				if(strlen(restoreMsg) > 0) 
					sprintf(rcvyStatus->actionMsg, "%s--%s", rcvyStatus->actionMsg, restoreMsg);
			}
		}
		if(!strlen(rcvyStatus->actionMsg)) 
			strcpy(rcvyStatus->actionMsg, "None");
	}
	else
	{
		rcvyStatus->isExistASZ = FALSE;
		rcvyStatus->isActivated = FALSE;
		rcvyStatus->isExpired = TRUE;
		rcvyStatus->isExistNewerVer = FALSE;
		rcvyStatus->isAcrReady = FALSE;
		strcpy(rcvyStatus->version, "0.0.0");
		strcpy(rcvyStatus->lastBackupTime, "None");
		strcpy(rcvyStatus->actionMsg, "None");
		strcpy(rcvyStatus->lastWarningMsg, "None");
	}
	return TRUE;
#endif /* _is_linux */
}

void GetRcvyCurStatus()
{
	recovery_status_t rcvyStatus;
	memset(&rcvyStatus, 0, sizeof(rcvyStatus));
	GetRcvyStatus(&rcvyStatus);
	{
		char * pSendVal = NULL;
		char * str = (char *) &rcvyStatus;
		int jsonStrlen = Pack_rcvy_status_rep(str, &pSendVal);
		if (jsonStrlen > 0 && pSendVal != NULL)
		{
			g_sendcbf(&g_PluginInfo, rcvy_status_rep, pSendVal, strlen(pSendVal) + 1, NULL, NULL);
		}
		if (pSendVal) free(pSendVal);
	}
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
			app_os_mutex_lock(&CSAMsg);
			memcpy(actionMsg, ActionMsg, strlen(ActionMsg)+1);
			memset(ActionMsg, 0, sizeof(ActionMsg));
			app_os_mutex_unlock(&CSAMsg);
		}
	}
}

static void GetLastWarningMsg(char * warningMsg)
{
	if(warningMsg == NULL) return;
	{
		if(!IsAcrocmdRunning())
		{
			app_os_mutex_lock(&CSWMsg);
			memset(LastWarningMsg, 0, sizeof(LastWarningMsg));
			GetRestoreRetMsg(LastWarningMsg);
			app_os_mutex_unlock(&CSWMsg);

#ifndef _is_linux
			if(!strlen(LastWarningMsg))
			{
				BOOL bRet = FALSE;
				char actionStr[32] = {0};
				bRet = GetAcrLatestActionFromReg(actionStr);
				if(bRet && !strcmp(actionStr, "Installing"))
				{
					if(app_os_is_file_exist(AcroCmdExePath))
					{
						app_os_mutex_lock(&CSWMsg);
						memset(LastWarningMsg, 0, sizeof(LastWarningMsg));
						strcpy(LastWarningMsg, "Install success");
						app_os_mutex_unlock(&CSWMsg);
					}
					else
					{
						app_os_mutex_lock(&CSWMsg);
						memset(LastWarningMsg, 0, sizeof(LastWarningMsg));
						strcpy(LastWarningMsg, "Install Fail");
						app_os_mutex_unlock(&CSWMsg);
					}
					SetAcrLatestActionToReg("None");
				}
			}
#endif /* _is_linux */
		}

		if(strlen(LastWarningMsg))
		{
			app_os_mutex_lock(&CSWMsg);
			memcpy(warningMsg, LastWarningMsg, strlen(LastWarningMsg)+1);
			memset(LastWarningMsg, 0, sizeof(LastWarningMsg));
			app_os_mutex_unlock(&CSWMsg);
		}
		else
		{
			strcpy(warningMsg, "None");
		}
	}
}

static BOOL GetLatestBKTime(char * timeStr)
{
#define DEF_PER_QUERY_CNT     5
#define DEF_DB_LATEST_BK_TIEM_QUERY_SQL_FORMAT  "SELECT * FROM Recovery ORDER BY DateTime DESC LIMIT %d,%d" 
	BOOL bRet = FALSE;
	BOOL isFind = FALSE;
	int queryCnt = 0;
	char validTimeStr[32] = {0};
	sqlite3 * rcvyDB = NULL;
	int iRet;
	char * errmsg = NULL;
	iRet = sqlite3_open(rcvyDBFilePath, &rcvyDB);
	if(iRet != SQLITE_OK) goto done1;
	else
	{
		BOOL isPrevError = FALSE;
		char tmpSourceStr[128] = {0};
		char tmpMsgStr[BUFSIZ] = {0};
		int tmpEventID = 0;
		char tmpDateTimeStr[32] = {0};
		while(!isFind)
		{
			char ** retTable = NULL;
			int nRow = 0, nColumn = 0;
			int i = 0;
			char sqlStr[128] = {0};
			sprintf_s(sqlStr, sizeof(sqlStr), DEF_DB_LATEST_BK_TIEM_QUERY_SQL_FORMAT, queryCnt*DEF_PER_QUERY_CNT, DEF_PER_QUERY_CNT);
			iRet = sqlite3_get_table(rcvyDB, sqlStr, &retTable, &nRow, &nColumn, &errmsg);
			if(iRet != SQLITE_OK) 
			{
				sqlite3_free(errmsg);
				goto done2;
			}
			else
			{
				for(i = 0; i < nRow; i++)
				{
					memset(validTimeStr, 0, sizeof(validTimeStr));
					memset(tmpSourceStr, 0, sizeof(tmpSourceStr));
					strcpy(tmpSourceStr, retTable[(i+1)*nColumn]);
					memset(tmpMsgStr, 0, sizeof(tmpMsgStr));
					strcpy(tmpMsgStr, retTable[(i+1)*nColumn+1]);
					tmpEventID = 0;
					tmpEventID = atoi(retTable[(i+1)*nColumn+2]);
					memset(tmpDateTimeStr, 0, sizeof(tmpDateTimeStr));
					strcpy(tmpDateTimeStr, retTable[(i+1)*nColumn+3]);
					if(!strcmp(tmpMsgStr, DEF_BACKUP_ERROR) && tmpEventID == DEF_BACKUP_ERROR_ID)
					{
						isPrevError = TRUE;
					}
					else
					{
						if(!isPrevError && \
							!strcmp(tmpMsgStr, DEF_BACKUP_START) && \
							tmpEventID == DEF_BACKUP_START_ID)
						{
							strcpy(validTimeStr, tmpDateTimeStr);
							isFind = TRUE;
							break;
						}
						isPrevError = FALSE;
					}
				}
				if(retTable) sqlite3_free_table(retTable);
				if(nRow < DEF_PER_QUERY_CNT) break;
			}
			queryCnt++;
		}
	}
done2:
	if(rcvyDB) sqlite3_close(rcvyDB);
done1:
	if(strlen(validTimeStr) > 0)
	{
		strcpy(timeStr, validTimeStr);
		bRet = TRUE;
	}
	else
	{
		strcpy(timeStr, "None");
	}
	return bRet;
}

#ifndef _is_linux
static BOOL IsHotKeyActivateRunning()
{
	return app_os_ProcessCheck("HotKeyActivate");
}

static BOOL IsHotKeyDeactivateRunning()
{
	return app_os_ProcessCheck("HotKeyDeactivate");
}
#endif /*_is_linux*/


static BOOL GetLatestVersion(char * latestVer)
{
#ifdef _WIN32
	BOOL bRet = FALSE;
	if(latestVer == NULL) return bRet;
	{
		HKEY hk;
		char regName[] = "SOFTWARE\\AdvantechAcronis";
		if(ERROR_SUCCESS == app_os_RegOpenKeyExA(HKEY_LOCAL_MACHINE, regName, 0, KEY_READ, &hk))
		{
			char valueName[] = "Version";
			char latestVerTmp[32] = {0};
			int  valueDataSize = sizeof(latestVerTmp);
			if(ERROR_SUCCESS == app_os_RegQueryValueExA(hk, valueName, 0, NULL, latestVerTmp, &valueDataSize))
			{
				strcpy(latestVer, latestVerTmp);
				bRet = TRUE;
			}
			app_os_RegCloseKey(hk);
		}
	}
	return bRet;

#else
	
	BOOL bRet = FALSE;
	char lineBuf[258] = {0};
	char major_Ver[8] = {0};
	char minor_Ver[8] = {0};
	char build_number[8] = {0};
	FILE *fd = fopen(ACRONIS_VERSION_FILE_PATH, "rb");
	if (fd)
	{
		while (fgets(lineBuf, sizeof(lineBuf)-1, fd))
		{
			if (strstr(lineBuf, "MAJOR_VERSION"))
			{
				strncpy(major_Ver, strstr(lineBuf, "=")+1, sizeof(major_Ver)-1);
			}
			else if (strstr(lineBuf, "MINOR_VERSION"))
			{
				strncpy(minor_Ver, strstr(lineBuf, "=")+1, sizeof(minor_Ver)-1);
			}
			else if (strstr(lineBuf, "BUILD_NUMBER"))
			{
				strncpy(build_number, strstr(lineBuf, "=")+1, sizeof(build_number)-1);
			}
		}
		sprintf(latestVer, "%s.%s-%s", trimwhitespace(major_Ver), \
			trimwhitespace(minor_Ver), trimwhitespace(build_number));
		if (strlen(latestVer) > 0)
		{
			bRet = TRUE;
			Recovery_debug_print("latestVer: %s", latestVer);
		}
		fclose(fd);
	}
	return bRet;
#endif /*_WIN32*/
}

//BOOL CheckAcronisService()
//{
//#ifndef _is_linux
//   DWORD dwRet = app_os_GetSrvStatus(DEF_ACRONIS_SERVICE_NAME);
//   if(dwRet != SERVICE_STOPPED && dwRet != SERVICE_STOP_PENDING)
//   {
//      return TRUE;
//   }
//   return FALSE;
//#else
//   return app_os_ProcessCheck(DEF_ACRONIS_SERVICE_NAME);
//#endif /* _is_linux */
//}
//this may not use for SA3.0+
//
//static BOOL ReadInstallerVersion()
//{
//   return FALSE;
//}
//
//static BOOL IsUnInstallerRunning()
//{
//   return app_os_ProcessCheck(DEF_ACRONIS_UNINSTALLER_NAME);
//}
//
//static BOOL IsCloseAcrocmdRunning()
//{
//   return app_os_ProcessCheck(DEF_CLOSE_ACROCMD_NAME);
//}
//
//static BOOL IsUninstallOldAcronis()
//{
//	return app_os_ProcessCheck(DEF_UNINSTALL_OLD_ACRONIS_NAME);
//}

//
//
//void SendAgentStartMessage(BOOL isCallHotkey)
//{	
//#ifndef _is_linux
//   recovery_handler_context_t * pRecoveryHandlerContext = (recovery_handler_context_t *)&RecoveryHandlerContext;
//   acr_action_status acrActionStatus = None;
//   char acrMsg[BUFSIZ] = {0};
//
//   Recovery_debug_print("----------------------------------------------");
//#pragma region  if (IsUninstallOldAcronis())
//   if (IsUninstallOldAcronis())
//   {
//      memset(acrMsg, 0, sizeof(acrMsg));
//      sprintf(acrMsg, "%s,%s,%d", "InstallCommand", "UnInstalling Old Acronis",100); //different 2.1, Download progress
//	  {
//		  char * pSendVal = NULL;
//		  char * str = acrMsg;
//		  int jsonStrlen = Pack_string(str, &pSendVal);
//		  if(jsonStrlen > 0 && pSendVal != NULL)
//		  {
//			  g_sendcbf(&g_PluginInfo, rcvy_log_rep, pSendVal, strlen(pSendVal)+1, NULL, NULL);
//		  }
//		  if(pSendVal)free(pSendVal);
//	  }
//   }
//#pragma endregion
//#pragma region  else if(IsInstallerRunning() && !IsUnInstallerRunning())
//   else if(IsInstallerRunning() && !IsUnInstallerRunning())
//   {
//      //add other code
//   }
//#pragma endregion
//#pragma region  else if(IsInstallerRunning() && IsUnInstallerRunning())
//   else if(IsInstallerRunning() && IsUnInstallerRunning())
//   {
//      if (IsActive())
//      {
//         memset(acrMsg, 0, sizeof(acrMsg));
//         sprintf(acrMsg, "%s,%s", "ActivateInfo", "OK");
//		 {
//			 char * pSendVal = NULL;
//			 char * str = acrMsg;
//			 int jsonStrlen = Pack_string(str, &pSendVal);
//			 if(jsonStrlen > 0 && pSendVal != NULL)
//			 {
//				 g_sendcbf(&g_PluginInfo, rcvy_log_rep, pSendVal, strlen(pSendVal)+1, NULL, NULL);
//			 }
//			 if(pSendVal)free(pSendVal);
//		 }
//
//         if (isCallHotkey) RunASRM(TRUE);
//      }
//      else
//      {
//         memset(acrMsg, 0, sizeof(acrMsg));
//         sprintf(acrMsg, "%s,%s", "ActivateInfo", "Fail");
//		 {
//			 char * pSendVal = NULL;
//			 char * str = acrMsg;
//			 int jsonStrlen = Pack_string(str, &pSendVal);
//			 if(jsonStrlen > 0 && pSendVal != NULL)
//			 {
//				 g_sendcbf(&g_PluginInfo, rcvy_log_rep, pSendVal, strlen(pSendVal)+1, NULL, NULL);
//			 }
//			 if(pSendVal)free(pSendVal);
//		 }
//
//         if (IsExpired())
//         {
//            memset(acrMsg, 0, sizeof(acrMsg));
//            sprintf(acrMsg, "%s,%s", "ActivateInfo", "Expired");
//			{
//				char * pSendVal = NULL;
//				char * str = acrMsg;
//				int jsonStrlen = Pack_string(str, &pSendVal);
//				if(jsonStrlen > 0 && pSendVal != NULL)
//				{
//					g_sendcbf(&g_PluginInfo, rcvy_log_rep, pSendVal, strlen(pSendVal)+1, NULL, NULL);
//				}
//				if(pSendVal)free(pSendVal);
//			}
//
//            if (isCallHotkey) DeactivateARSM();
//         }
//      }
//
//      IsAcrInstall = TRUE;
//      memset(acrMsg, 0, sizeof(acrMsg));
//      sprintf(acrMsg, "%s,%s,%d", "UpdateInfo", "Updating", 100);  //different 2.1, Download progress
//	  {
//		  char * pSendVal = NULL;
//		  char * str = acrMsg;
//		  int jsonStrlen = Pack_string(str, &pSendVal);
//		  if(jsonStrlen > 0 && pSendVal != NULL)
//		  {
//			  g_sendcbf(&g_PluginInfo, rcvy_log_rep, pSendVal, strlen(pSendVal)+1, NULL, NULL);
//		  }
//		  if(pSendVal)free(pSendVal);
//	  }
//
//   }
//#pragma endregion
//#pragma region  else
//   else
//   {
//#pragma region       if(app_os_is_file_exist(AcroCmdExePath))
//      if(app_os_is_file_exist(AcroCmdExePath))
//      {
//#pragma region       if (CheckAcronisService())
//         if (CheckAcronisService())
//         {
//            char aszStatus[32] = {0};
//            if (GetASZExistRecord(aszStatus))
//            {
//               memset(acrMsg, 0, sizeof(acrMsg));
//               sprintf(acrMsg, "%s,%s", "ASZExistStatus", aszStatus);
//			   {
//				   char * pSendVal = NULL;
//				   char * str = acrMsg;
//				   int jsonStrlen = Pack_string(str, &pSendVal);
//				   if(jsonStrlen > 0 && pSendVal != NULL)
//				   {
//					   g_sendcbf(&g_PluginInfo, rcvy_log_rep, pSendVal, strlen(pSendVal)+1, NULL, NULL);
//				   }
//				   if(pSendVal)free(pSendVal);
//			   }
//            }
//
//            if(IsActive())
//            {
//               memset(acrMsg, 0, sizeof(acrMsg));
//               sprintf(acrMsg, "%s,%s", "ActivateInfo", "OK");
//			   {
//				   char * pSendVal = NULL;
//				   char * str = acrMsg;
//				   int jsonStrlen = Pack_string(str, &pSendVal);
//				   if(jsonStrlen > 0 && pSendVal != NULL)
//				   {
//					   g_sendcbf(&g_PluginInfo, rcvy_log_rep, pSendVal, strlen(pSendVal)+1, NULL, NULL);
//				   }
//				   if(pSendVal)free(pSendVal);
//			   }
//
//               if(isCallHotkey) RunASRM(TRUE);
//            }
//            else
//            {
//               memset(acrMsg, 0, sizeof(acrMsg));
//               sprintf(acrMsg, "%s,%s", "ActivateInfo", "Fail");
//			   {
//				   char * pSendVal = NULL;
//				   char * str = acrMsg;
//				   int jsonStrlen = Pack_string(str, &pSendVal);
//				   if(jsonStrlen > 0 && pSendVal != NULL)
//				   {
//					   g_sendcbf(&g_PluginInfo, rcvy_log_rep, pSendVal, strlen(pSendVal)+1, NULL, NULL);
//				   }
//				   if(pSendVal)free(pSendVal);
//			   }
//
//               if(IsExpired())
//               {
//                  memset(acrMsg, 0, sizeof(acrMsg));
//                  sprintf(acrMsg, "%s,%s", "ActivateInfo", "Expired");
//				  {
//					  char * pSendVal = NULL;
//					  char * str = acrMsg;
//					  int jsonStrlen = Pack_string(str, &pSendVal);
//					  if(jsonStrlen > 0 && pSendVal != NULL)
//					  {
//						  g_sendcbf(&g_PluginInfo, rcvy_log_rep, pSendVal, strlen(pSendVal)+1, NULL, NULL);
//					  }
//					  if(pSendVal)free(pSendVal);
//				  }
//
//                  if(isCallHotkey) DeactivateARSM();
//               }
//            }
//
//#pragma region  if (IsCloseAcrocmdRunning())
//            if (IsCloseAcrocmdRunning())
//            {
//               memset(acrMsg, 0, sizeof(acrMsg));
//               sprintf(acrMsg, "%s,%s", "ASZInfo", "Creating ASZ...");
//			   {
//				   char * pSendVal = NULL;
//				   char * str = acrMsg;
//				   int jsonStrlen = Pack_string(str, &pSendVal);
//				   if(jsonStrlen > 0 && pSendVal != NULL)
//				   {
//					   g_sendcbf(&g_PluginInfo, rcvy_log_rep, pSendVal, strlen(pSendVal)+1, NULL, NULL);
//				   }
//				   if(pSendVal)free(pSendVal);
//			   }
//
//            }
//#pragma endregion
//#pragma region  else if(!_stricmp(aszStatus, "TRUE"))
//            else if(!_stricmp(aszStatus, "TRUE"))
//            {
//               if(app_os_is_file_exist(AcrLogPath) && (GetFileLineCount(AcrLogPath) > 0))
//               {
//                  if(FileCopy(AcrLogPath, AcrTempLogPath)) app_os_file_remove(AcrLogPath);
//               }
//
//               if(app_os_is_file_exist(AcrTempLogPath) && (GetFileLineCount(AcrTempLogPath)))
//               {
//#pragma region  if(IsAcrocmdRunning())
//                  if(IsAcrocmdRunning())
//                  {
//                     FILE *pAcrTempLogFile = NULL;
//                     char lineBuf[1024] = {0};
//                     pAcrTempLogFile = fopen(AcrTempLogPath, "rb");
//                     if(pAcrTempLogFile)
//                     {
//                        char tempSourceStr[32] = {0};
//                        int tempEventID = 0;
//                        while (!feof(pAcrTempLogFile))
//                        {
//                           memset(lineBuf, 0, sizeof(lineBuf));
//                           if(fgets(lineBuf, sizeof(lineBuf), pAcrTempLogFile))
//                           {
//                              char * word[6] = {NULL};
//                              char * pTempBuf  = lineBuf;
//                              int i = 0, wordIndex = 0;
//                              while((word[i] = strtok( pTempBuf, "," )) != NULL)
//                              {
//                                 i++;
//                                 pTempBuf = NULL;
//                              }
//                              while(word[wordIndex] != NULL)
//                              {
//                                 TrimStr(word[wordIndex]);
//                                 wordIndex++;
//                              }
//
//                              memset(tempSourceStr, 0, sizeof(tempSourceStr));
//                              tempEventID = 0;
//                              if(NULL != word[0]) strcpy(tempSourceStr, word[0]);
//                              if(NULL != word[2]) tempEventID = atoi(word[2]);
//                              if(!strcmp(tempSourceStr, "Acronis"))
//                              {
//                                 switch(tempEventID)
//                                 {
//                                 case DEF_RESTORING_EVENT_ID:
//                                    {
//                                       acrActionStatus = Restoring;
//                                       break;
//                                    }
//                                 case DEF_BACKUPING_EVENT_ID:
//                                    {
//                                       acrActionStatus = Backuping;
//                                       break;
//                                    }
//                                 default:
//                                    {
//                                       acrActionStatus = None;
//                                       break;
//                                    }
//                                 }
//                                 break;
//                              }
//                           }
//                        }
//                        fclose(pAcrTempLogFile);
//                     }
//                  }
//#pragma endregion
//                  {
//                     int lineIndex = 0;
//                     char curSource[32] = {0};
//                     int curEventID = 0;
//                     char curMsg[BUFSIZ] = {0};
//                     char curDateTimeStr[32] = {0};
//
//                     int firstStartBackup = -1;
//                     int jumpOneCase = 1;
//
//                     FILE *pAcrTempLogFile = NULL;
//                     char lineBuf[1024] = {0};
//                     pAcrTempLogFile = fopen(AcrTempLogPath, "rb");
//                     if(pAcrTempLogFile)
//                     {
//                        char tempSourceStr[32] = {0};
//                        int tempEventID = 0;
//                        while (!feof(pAcrTempLogFile))
//                        {
//                           memset(lineBuf, 0, sizeof(lineBuf));
//                           if(fgets(lineBuf, sizeof(lineBuf), pAcrTempLogFile))
//                           {
//                              char * word[6] = {NULL};
//                              char * pTempBuf  = lineBuf;
//                              int i = 0, wordIndex = 0;
//                              lineIndex ++;
//                              while((word[i] = strtok( pTempBuf, "," )) != NULL)
//                              {
//                                 i++;
//                                 pTempBuf = NULL;
//                              }
//                              while(word[wordIndex] != NULL)
//                              {
//                                 TrimStr(word[wordIndex]);
//                                 wordIndex++;
//                              }
//                              memset(tempSourceStr, 0, sizeof(tempSourceStr));
//                              tempEventID = 0;
//                              if(NULL != word[0]) strcpy(tempSourceStr, word[0]);
//                              if(NULL != word[2]) tempEventID = atoi(word[2]);
//
//                              if(!strcmp(tempSourceStr, "Acronis") && tempEventID == DEFBACKUP_SUCCESS_EVENT_ID) // ????????
//                              {
//                                 break;
//                              }
//                              if(!strcmp(tempSourceStr, "Acronis") && tempEventID == DEF_BACKUPING_EVENT_ID)
//                              {
//                                 if(acrActionStatus == Backuping)
//                                 {
//                                    if(jumpOneCase == 1)   //???????backup start
//                                    {
//                                       strcpy(curSource, tempSourceStr);
//                                       if(NULL != word[1]) strcpy(curMsg, word[1]);
//                                       curEventID = tempEventID;
//                                       if(NULL != word[3]) strcpy(curDateTimeStr, word[3]);
//                                       firstStartBackup = lineIndex;
//                                       break;
//                                    }
//                                    jumpOneCase = jumpOneCase + 1;
//                                 }
//                                 else
//                                 {
//                                    strcpy(curSource, tempSourceStr);
//                                    if(NULL != word[1]) strcpy(curMsg, word[1]);
//                                    curEventID = tempEventID;
//                                    if(NULL != word[3]) strcpy(curDateTimeStr, word[3]);
//                                    firstStartBackup = lineIndex;
//                                    break;
//                                 }
//                              }
//                           }
//                        }
//                        app_os_CloseHandle(pAcrTempLogFile);
//                     }
//#pragma region  if (firstStartBackup != -1)
//                     if (firstStartBackup != -1)
//                     {
//                        BOOL isEmptyEventLog = TRUE;
//                        FILE *pAcrTempLogFile = NULL;
//                        char lineBuf[1024] = {0};
//                        pAcrTempLogFile = fopen(AcrTempLogPath, "rb");
//                        if(pAcrTempLogFile)
//                        {
//                           char tempSourceStr[32] = {0};
//                           char tempMsgStr[BUFSIZ] = {0};
//                           int tempEventID = 0;
//                           char tempDateTimeStr[32] = {0};
//                           int lineIndex = 0;
//                           while (!feof(pAcrTempLogFile))
//                           {
//                              memset(lineBuf, 0, sizeof(lineBuf));
//                              if(fgets(lineBuf, sizeof(lineBuf), pAcrTempLogFile))
//                              {
//                                 char * word[6] = {NULL};
//                                 char * pTempBuf  = lineBuf;
//                                 int i = 0, wordIndex = 0;
//                                 lineIndex ++;
//                                 if(lineIndex < firstStartBackup) continue;
//
//                                 while((word[i] = strtok( pTempBuf, "," )) != NULL)
//                                 {
//                                    i++;
//                                    pTempBuf = NULL;
//                                 }
//                                 while(word[wordIndex] != NULL)
//                                 {
//                                    TrimStr(word[wordIndex]);
//                                    wordIndex++;
//                                 }
//                                 memset(tempSourceStr, 0, sizeof(tempSourceStr));
//                                 tempEventID = 0;
//                                 if(NULL != word[0]) strcpy(tempSourceStr, word[0]);
//                                 if(NULL != word[1]) strcpy(tempMsgStr, word[1]);
//                                 if(NULL != word[2]) tempEventID = atoi(word[2]);
//                                 if(NULL != word[3]) strcpy(tempDateTimeStr, word[3]);
//
//                                 if(!strcmp(tempSourceStr, "Acronis"))
//                                 {
//                                    if(3 == tempEventID || 4 == tempEventID)
//                                    {
//                                       isEmptyEventLog = FALSE;
//                                       memset(acrMsg, 0 , sizeof(acrMsg));
//                                       switch(acrActionStatus)
//                                       {
//                                       case Backuping:
//                                          {
//                                             sprintf(acrMsg, "%s,%s,%s,%s", "Eventlog", tempDateTimeStr, tempMsgStr, "BackingUp");
//                                             break;
//                                          }
//                                       case Restoring:
//                                          {
//                                             sprintf(acrMsg, "%s,%s,%s,%s", "Eventlog", tempDateTimeStr, tempMsgStr, "Restoring");
//                                             break;
//                                          }
//                                       case None:
//                                          {
//                                             sprintf(acrMsg, "%s,%s,%s,%s", "Eventlog", tempDateTimeStr, tempMsgStr, "FirstAgentRun");
//                                             break;
//                                          }
//                                       default: break;
//                                       }
//                                       if(strlen(acrMsg))
//                                       {
//										  {
//											  char * pSendVal = NULL;
//											  char * str = acrMsg;
//											  int jsonStrlen = Pack_string(str, &pSendVal);
//											  if(jsonStrlen > 0 && pSendVal != NULL)
//											  {
//												  g_sendcbf(&g_PluginInfo, rcvy_log_rep, pSendVal, strlen(pSendVal)+1, NULL, NULL);
//											  }
//											  if(pSendVal)free(pSendVal);
//										  }
//                                       }
//                                       break;
//                                    }
//                                 }
//                                 else if(acrActionStatus == Backuping)
//                                 {
//                                    memset(acrMsg, 0 , sizeof(acrMsg));
//                                    sprintf(acrMsg, "%s,%s,%s,%s", "Eventlog", tempDateTimeStr, tempMsgStr, "Restoring");
//									{
//										char * pSendVal = NULL;
//										char * str = acrMsg;
//										int jsonStrlen = Pack_string(str, &pSendVal);
//										if(jsonStrlen > 0 && pSendVal != NULL)
//										{
//											g_sendcbf(&g_PluginInfo, rcvy_log_rep, pSendVal, strlen(pSendVal)+1, NULL, NULL);
//										}
//										if(pSendVal)free(pSendVal);
//									}
//
//                                    {
//                                       if(!pRecoveryHandlerContext->isAcrPercentCheckRunning)
//                                       {
//                                          pRecoveryHandlerContext->acrPercentCheckIntervalMs = DEF_BACKUP_PERCENT_CHECK_INTERVAL_MS;
//                                          pRecoveryHandlerContext->isAcrPercentCheck = FALSE;
//                                          pRecoveryHandlerContext->isAcrPercentCheckRunning = TRUE;
//                                          pRecoveryHandlerContext->acrBackupPercentCheckThreadHandle = NULL;
//                                          if (app_os_thread_create(&pRecoveryHandlerContext->acrBackupPercentCheckThreadHandle, AcrBackupPercentCheckThreadStart, pRecoveryHandlerContext) != 0)
//                                          {
//															RecoveryLog(g_loghandle, Normal, "%s()[%d]###Start recovery backup percent check thread failed!\n",__FUNCTION__, __LINE__ );
//                                             pRecoveryHandlerContext->isAcrPercentCheckRunning = FALSE;
//                                          }
//                                       }
//                                       pRecoveryHandlerContext->acrBackupPercent = 0;
//                                    }
//
//                                    break;
//                                 }
//                                 else if(acrActionStatus == Restoring)
//                                 {
//                                    memset(acrMsg, 0 , sizeof(acrMsg));
//                                    sprintf(acrMsg, "%s,%s,%s,%s", "Eventlog", tempDateTimeStr, tempMsgStr, "Restoring");
//									{
//										char * pSendVal = NULL;
//										char * str = acrMsg;
//										int jsonStrlen = Pack_string(str, &pSendVal);
//										if(jsonStrlen > 0 && pSendVal != NULL)
//										{
//											g_sendcbf(&g_PluginInfo, rcvy_log_rep, pSendVal, strlen(pSendVal)+1, NULL, NULL);
//										}
//										if(pSendVal)free(pSendVal);
//									}
//                                 }
//                              }
//                           }
//                           app_os_CloseHandle(pAcrTempLogFile);
//                        }
//
//                        if(isEmptyEventLog)
//                        {
//                           memset(acrMsg, 0 , sizeof(acrMsg));
//                           switch(acrActionStatus)
//                           {
//                           case Backuping:
//                              {
//                                 sprintf(acrMsg, "%s,%s,%s,%s", "Eventlog", curDateTimeStr, curMsg, "BackingUp");
//                                 break;
//                              }
//                           case Restoring:
//                              {
//                                 sprintf(acrMsg, "%s,%s,%s,%s", "Eventlog", curDateTimeStr, curMsg, "Restoring");
//                                 break;
//                              }
//                           case None:
//                              {
//                                 sprintf(acrMsg, "%s,%s,%s,%s", "Eventlog", curDateTimeStr, "BackupStart", "FirstAgentRun");
//                                 break;
//                              }
//                           default: break;
//                           }
//                           if(strlen(acrMsg))
//                           {
//							  {
//								  char * pSendVal = NULL;
//								  char * str = acrMsg;
//								  int jsonStrlen = Pack_string(str, &pSendVal);
//								  if(jsonStrlen > 0 && pSendVal != NULL)
//								  {
//									  g_sendcbf(&g_PluginInfo, rcvy_log_rep, pSendVal, strlen(pSendVal)+1, NULL, NULL);
//								  }
//								  if(pSendVal)free(pSendVal);
//							  }
//                           }
//                        }
//                     }
//#pragma endregion
//#pragma region  else
//                     else
//                     {
//                        memset(acrMsg, 0 , sizeof(acrMsg));
//                        switch(acrActionStatus)
//                        {
//                        case Backuping:
//                           {
//                              sprintf(acrMsg, "%s,%s,%s,%s", "Eventlog", "PastNoBackuped", "", "BackingUp");
//                              break;
//                           }
//                        case Restoring:
//                           {
//                              sprintf(acrMsg, "%s,%s,%s,%s", "Eventlog", "PastNoBackuped", "", "Restoring");
//                              break;
//                           }
//                        case None:
//                           {
//                              sprintf(acrMsg, "%s,%s,%s,%s", "Eventlog", "PastNoBackuped", "", "FirstAgentRun");
//                              break;
//                           }
//                        default: break;
//                        }
//                        if(strlen(acrMsg))
//                        {
//						   {
//							   char * pSendVal = NULL;
//							   char * str = acrMsg;
//							   int jsonStrlen = Pack_string(str, &pSendVal);
//							   if(jsonStrlen > 0 && pSendVal != NULL)
//							   {
//								   g_sendcbf(&g_PluginInfo, rcvy_log_rep, pSendVal, strlen(pSendVal)+1, NULL, NULL);
//							   }
//							   if(pSendVal)free(pSendVal);
//						   }
//                        }
//                     }
//#pragma endregion
//                  }
//						app_os_file_remove(AcrTempLogPath);
//               }
//            }
//#pragma endregion
//         }
//#pragma endregion
//#pragma region  else
//         else
//         {
//            memset(acrMsg, 0, sizeof(acrMsg));
//            sprintf(acrMsg, "%s,%s,%d", "InstallCommand", "InstallFail", 100);   //different 2.1, Downlad progree
//			{
//				char * pSendVal = NULL;
//				char * str = acrMsg;
//				int jsonStrlen = Pack_string(str, &pSendVal);
//				if(jsonStrlen > 0 && pSendVal != NULL)
//				{
//					g_sendcbf(&g_PluginInfo, rcvy_log_rep, pSendVal, strlen(pSendVal)+1, NULL, NULL);
//				}
//				if(pSendVal)free(pSendVal);
//			}
//         }
//#pragma endregion
//#pragma region       if (ReadInstallerVersion())
//         if (ReadInstallerVersion())
//         {
//            if (IsAcrCanUpdate)
//            {
//               memset(acrMsg, 0, sizeof(acrMsg));
//               sprintf(acrMsg, "%s,%s", "UpdateInfo", "Yes");
//			   {
//				   char * pSendVal = NULL;
//				   char * str = acrMsg;
//				   int jsonStrlen = Pack_string(str, &pSendVal);
//				   if(jsonStrlen > 0 && pSendVal != NULL)
//				   {
//					   g_sendcbf(&g_PluginInfo, rcvy_log_rep, pSendVal, strlen(pSendVal)+1, NULL, NULL);
//				   }
//				   if(pSendVal)free(pSendVal);
//			   }
//            }
//            else
//            {
//               memset(acrMsg, 0, sizeof(acrMsg));
//               sprintf(acrMsg, "%s,%s", "UpdateInfo", "No");
//			   {
//				   char * pSendVal = NULL;
//				   char * str = acrMsg;
//				   int jsonStrlen = Pack_string(str, &pSendVal);
//				   if(jsonStrlen > 0 && pSendVal != NULL)
//				   {
//					   g_sendcbf(&g_PluginInfo, rcvy_log_rep, pSendVal, strlen(pSendVal)+1, NULL, NULL);
//				   }
//				   if(pSendVal)free(pSendVal);
//			   }
//            }
//         }
//         else
//         {
//            memset(acrMsg, 0, sizeof(acrMsg));
//            sprintf(acrMsg, "%s,%s", "UpdateInfo", "No");
//			{
//				char * pSendVal = NULL;
//				char * str = acrMsg;
//				int jsonStrlen = Pack_string(str, &pSendVal);
//				if(jsonStrlen > 0 && pSendVal != NULL)
//				{
//					g_sendcbf(&g_PluginInfo, rcvy_log_rep, pSendVal, strlen(pSendVal)+1, NULL, NULL);
//				}
//				if(pSendVal)free(pSendVal);
//			}
//         }
//#pragma endregion
//      }
//#pragma endregion
//#pragma region  else
//      else
//      {
//         IsAcrInstall = TRUE;
//         if (CheckOtherAcronis())
//         {
//            memset(acrMsg, 0, sizeof(acrMsg));
//            sprintf(acrMsg, "%s,%s,%d", "InstallCommand", "InstalledOtherAcronis", 100);   //different 2.1, Downlad progree
//			{
//				char * pSendVal = NULL;
//				char * str = acrMsg;
//				int jsonStrlen = Pack_string(str, &pSendVal);
//				if(jsonStrlen > 0 && pSendVal != NULL)
//				{
//					g_sendcbf(&g_PluginInfo, rcvy_log_rep, pSendVal, strlen(pSendVal)+1, NULL, NULL);
//				}
//				if(pSendVal)free(pSendVal);
//			}
//         }
//         else
//         {
//            memset(acrMsg, 0, sizeof(acrMsg));
//            sprintf(acrMsg, "%s,%s,%s,%s", "Eventlog", "NotInstall", "", "FirstAgentRun");   //different 2.1, Downlad progree
//			{
//				char * pSendVal = NULL;
//				char * str = acrMsg;
//				int jsonStrlen = Pack_string(str, &pSendVal);
//				if(jsonStrlen > 0 && pSendVal != NULL)
//				{
//					g_sendcbf(&g_PluginInfo, rcvy_log_rep, pSendVal, strlen(pSendVal)+1, NULL, NULL);
//				}
//				if(pSendVal)free(pSendVal);
//			}
//         }
//      }
//#pragma endregion
//   }
//#pragma endregion
//
//#else
//   recovery_handler_context_t * pRecoveryHandlerContext = (recovery_handler_context_t *)&RecoveryHandlerContext;
//   acr_action_status acrActionStatus = None;
//   char acrMsg[BUFSIZ] = {0};
//
//   if(app_os_is_file_exist(AcroCmdExePath))
//   {
//		if (CheckAcronisService())
//		{
//			char aszStatus[32] = { 0 };
//			//if (GetASZExistRecord(aszStatus))
//			strncpy(aszStatus, "True", sizeof(aszStatus));
//			{
//				memset(acrMsg, 0, sizeof(acrMsg));
//				sprintf(acrMsg, "%s,%s", "ASZExistStatus", aszStatus);
//				{
//					char * pSendVal = NULL;
//					char * str = acrMsg;
//					int jsonStrlen = Pack_string(str, &pSendVal);
//					if (jsonStrlen > 0 && pSendVal != NULL)
//					{
//						g_sendcbf(&g_PluginInfo, rcvy_log_rep, pSendVal, strlen(pSendVal) + 1, NULL, NULL);
//					}
//					if (pSendVal)
//						free(pSendVal);
//				}
//			}
//
//			if (IsActive())
//			{
//				memset(acrMsg, 0, sizeof(acrMsg));
//				sprintf(acrMsg, "%s,%s", "ActivateInfo", "OK");
//				{
//					char * pSendVal = NULL;
//					char * str = acrMsg;
//					int jsonStrlen = Pack_string(str, &pSendVal);
//					if (jsonStrlen > 0 && pSendVal != NULL)
//					{
//						g_sendcbf(&g_PluginInfo, rcvy_log_rep, pSendVal, strlen(pSendVal) + 1, NULL, NULL);
//					}
//					if (pSendVal)
//						free(pSendVal);
//				}
//
//				if (isCallHotkey)
//					RunASRM(TRUE);
//			}
//			else
//			{
//				memset(acrMsg, 0, sizeof(acrMsg));
//				sprintf(acrMsg, "%s,%s,%s", "ActivateInfo", "Fail", "Not support the remote activation!");
//				{
//					char * pSendVal = NULL;
//					char * str = acrMsg;
//					int jsonStrlen = Pack_string(str, &pSendVal);
//					if (jsonStrlen > 0 && pSendVal != NULL)
//					{
//						g_sendcbf(&g_PluginInfo, rcvy_log_rep, pSendVal, strlen(pSendVal) + 1, NULL, NULL);
//					}
//					if (pSendVal)
//						free(pSendVal);
//				}
//
//				if (IsExpired())
//				{
//					memset(acrMsg, 0, sizeof(acrMsg));
//					sprintf(acrMsg, "%s,%s", "ActivateInfo", "Expired");
//					{
//						char * pSendVal = NULL;
//						char * str = acrMsg;
//						int jsonStrlen = Pack_string(str, &pSendVal);
//						if (jsonStrlen > 0 && pSendVal != NULL)
//						{
//							g_sendcbf(&g_PluginInfo, rcvy_log_rep, pSendVal, strlen(pSendVal) + 1, NULL, NULL);
//						}
//						if (pSendVal)
//							free(pSendVal);
//					}
//
//					if (isCallHotkey)
//						DeactivateARSM();
//				}
//			}
//
//			if (!_stricmp(aszStatus, "TRUE"))
//			{
//				if (app_os_is_file_exist(AcrLogPath) && (GetFileLineCount(AcrLogPath) > 0))
//				{
//					if (FileCopy(AcrLogPath, AcrTempLogPath))
//						app_os_file_remove(AcrLogPath);
//				}
//
//				if (app_os_is_file_exist(AcrTempLogPath) && GetFileLineCount(AcrTempLogPath))
//				{
//					if (IsAcrocmdRunning())
//					{
//						FILE *pAcrTempLogFile = NULL;
//						char lineBuf[1024] = { 0 };
//						pAcrTempLogFile = fopen(AcrTempLogPath, "rb");
//						if (pAcrTempLogFile)
//						{
//							char tempSourceStr[32] = { 0 };
//							int tempEventID = 0;
//							while (!feof(pAcrTempLogFile))
//							{
//								memset(lineBuf, 0, sizeof(lineBuf));
//								if (fgets(lineBuf, sizeof(lineBuf),	pAcrTempLogFile))
//								{
//									char * word[6] = { NULL };
//									char * pTempBuf = lineBuf;
//									int i = 0, wordIndex = 0;
//									while ((word[i] = strtok(pTempBuf, ",")) != NULL)
//									{
//										i++;
//										pTempBuf = NULL;
//									}
//									while (word[wordIndex] != NULL)
//									{
//										TrimStr(word[wordIndex]);
//										wordIndex++;
//									}
//
//									memset(tempSourceStr, 0, sizeof(tempSourceStr));
//									tempEventID = 0;
//									if (NULL != word[0])
//										strcpy(tempSourceStr, word[0]);
//									if (NULL != word[2])
//										tempEventID = atoi(word[2]);
//									if (!strcmp(tempSourceStr, "Acronis"))
//									{
//										switch (tempEventID)
//										{
//										case DEF_RESTORING_EVENT_ID:
//											acrActionStatus = Restoring;
//											break;
//										case DEF_BACKUPING_EVENT_ID:
//											acrActionStatus = Backuping;
//											break;
//										default:
//											acrActionStatus = None;
//											break;
//										}
//										break;
//									}
//								}
//							}
//							fclose(pAcrTempLogFile);
//						}
//					}
//					{
//						int lineIndex = 0;
//						char curSource[32] = { 0 };
//						int curEventID = 0;
//						char curMsg[BUFSIZ] = { 0 };
//						char curDateTimeStr[32] = { 0 };
//
//						int firstStartBackup = -1;
//						int jumpOneCase = 1;
//
//						FILE *pAcrTempLogFile = NULL;
//						char lineBuf[1024] = { 0 };
//						pAcrTempLogFile = fopen(AcrTempLogPath, "rb");
//						if (pAcrTempLogFile)
//						{
//							char tempSourceStr[32] = { 0 };
//							int tempEventID = 0;
//							while (!feof(pAcrTempLogFile))
//							{
//								memset(lineBuf, 0, sizeof(lineBuf));
//								if (fgets(lineBuf, sizeof(lineBuf), pAcrTempLogFile))
//								{
//									char * word[6] = { NULL };
//									char * pTempBuf = lineBuf;
//									int i = 0, wordIndex = 0;
//									lineIndex++;
//									while ((word[i] = strtok(pTempBuf, ",")) != NULL)
//									{
//										i++;
//										pTempBuf = NULL;
//									}
//									while (word[wordIndex] != NULL)
//									{
//										TrimStr(word[wordIndex]);
//										wordIndex++;
//									}
//									memset(tempSourceStr, 0, sizeof(tempSourceStr));
//									tempEventID = 0;
//									if (NULL != word[0])
//										strcpy(tempSourceStr, word[0]);
//									if (NULL != word[2])
//										tempEventID = atoi(word[2]);
//
//									if (!strcmp(tempSourceStr, "Acronis") && tempEventID == DEFBACKUP_SUCCESS_EVENT_ID)
//										break;
//									if (!strcmp(tempSourceStr, "Acronis") && tempEventID == DEF_BACKUPING_EVENT_ID)
//									{
//										if (acrActionStatus == Backuping)
//										{
//											if (jumpOneCase == 1)
//											{
//												strcpy(curSource, tempSourceStr);
//												if (NULL != word[1])
//													strcpy(curMsg, word[1]);
//												curEventID = tempEventID;
//												if (NULL != word[3])
//													strcpy(curDateTimeStr, word[3]);
//												firstStartBackup = lineIndex;
//												break;
//											}
//											jumpOneCase = jumpOneCase + 1;
//										}
//										else
//										{
//											strcpy(curSource, tempSourceStr);
//											if (NULL != word[1])
//												strcpy(curMsg, word[1]);
//											curEventID = tempEventID;
//											if (NULL != word[3])
//												strcpy(curDateTimeStr, word[3]);
//											firstStartBackup = lineIndex;
//											break;
//										}
//									}
//								}
//							}
//							app_os_CloseHandle(pAcrTempLogFile);
//						}
//
//						if (firstStartBackup != -1)
//						{
//							BOOL isEmptyEventLog = TRUE;
//							FILE *pAcrTempLogFile = NULL;
//							char lineBuf[1024] = { 0 };
//							pAcrTempLogFile = fopen(AcrTempLogPath, "rb");
//							if (pAcrTempLogFile) {
//								char tempSourceStr[32] = { 0 };
//								char tempMsgStr[BUFSIZ] = { 0 };
//								int tempEventID = 0;
//								char tempDateTimeStr[32] = { 0 };
//								int lineIndex = 0;
//								while (!feof(pAcrTempLogFile))
//								{
//									memset(lineBuf, 0, sizeof(lineBuf));
//									if (fgets(lineBuf, sizeof(lineBuf),
//											pAcrTempLogFile)) {
//										char * word[6] = { NULL };
//										char * pTempBuf = lineBuf;
//										int i = 0, wordIndex = 0;
//										lineIndex++;
//										if (lineIndex < firstStartBackup)
//											continue;
//
//										while ((word[i] = strtok(pTempBuf, ",")) != NULL)
//										{
//											i++;
//											pTempBuf = NULL;
//										}
//										while (word[wordIndex] != NULL)
//										{
//											TrimStr(word[wordIndex]);
//											wordIndex++;
//										}
//										memset(tempSourceStr, 0, sizeof(tempSourceStr));
//										tempEventID = 0;
//										if (NULL != word[0])
//											strcpy(tempSourceStr, word[0]);
//										if (NULL != word[1])
//											strcpy(tempMsgStr, word[1]);
//										if (NULL != word[2])
//											tempEventID = atoi(word[2]);
//										if (NULL != word[3])
//											strcpy(tempDateTimeStr, word[3]);
//
//										if (!strcmp(tempSourceStr, "Acronis"))
//										{
//											if (3 == tempEventID || 4 == tempEventID)
//											{
//												isEmptyEventLog = FALSE;
//												memset(acrMsg, 0, sizeof(acrMsg));
//												switch (acrActionStatus)
//												{
//												case Backuping:
//												{
//													sprintf(acrMsg,
//															"%s,%s,%s,%s",
//															"Eventlog",
//															tempDateTimeStr,
//															tempMsgStr,
//															"BackingUp");
//													break;
//												}
//												case Restoring:
//												{
//													sprintf(acrMsg,
//															"%s,%s,%s,%s",
//															"Eventlog",
//															tempDateTimeStr,
//															tempMsgStr,
//															"Restoring");
//													break;
//												}
//												case None:
//												{
//													sprintf(acrMsg,
//															"%s,%s,%s,%s",
//															"Eventlog",
//															tempDateTimeStr,
//															tempMsgStr,
//															"FirstAgentRun");
//													break;
//												}
//												default:
//													break;
//												}
//												if (strlen(acrMsg))
//												{
//													char * pSendVal = NULL;
//													char * str = acrMsg;
//													int jsonStrlen = Pack_string(str, &pSendVal);
//													if (jsonStrlen > 0&& pSendVal != NULL)
//													{
//														g_sendcbf(
//																&g_PluginInfo,
//																rcvy_log_rep,
//																pSendVal,
//																strlen(pSendVal) + 1,
//																NULL, NULL);
//													}
//													if (pSendVal)
//														free(pSendVal);
//												}
//												break;
//											}
//										}
//										else if (acrActionStatus == Backuping)
//										{
//											memset(acrMsg, 0, sizeof(acrMsg));
//											sprintf(acrMsg, "%s,%s,%s,%s",
//													"Eventlog", tempDateTimeStr,
//													tempMsgStr, "Restoring");
//											{
//												char * pSendVal = NULL;
//												char * str = acrMsg;
//												int jsonStrlen = Pack_string(str, &pSendVal);
//												if (jsonStrlen > 0&& pSendVal != NULL)
//												{
//													g_sendcbf(&g_PluginInfo,
//															rcvy_log_rep,
//															pSendVal,
//															strlen(pSendVal) + 1,
//															NULL, NULL);
//												}
//												if (pSendVal)
//													free(pSendVal);
//											}
//
//											{
//												if (!pRecoveryHandlerContext->isAcrPercentCheckRunning)
//												{
//													pRecoveryHandlerContext->acrPercentCheckIntervalMs = DEF_BACKUP_PERCENT_CHECK_INTERVAL_MS;
//													pRecoveryHandlerContext->isAcrPercentCheck = FALSE;
//													pRecoveryHandlerContext->isAcrPercentCheckRunning = TRUE;
//													pRecoveryHandlerContext->acrBackupPercentCheckThreadHandle = NULL;
//													if (app_os_thread_create(
//															&pRecoveryHandlerContext->acrBackupPercentCheckThreadHandle,
//															AcrBackupPercentCheckThreadStart,
//															pRecoveryHandlerContext)
//															!= 0)
//													{
//														RecoveryLog(g_loghandle,
//																Normal,
//																"%s()[%d]###Start recovery backup percent check thread failed!\n",
//																__FUNCTION__,
//																__LINE__);
//														pRecoveryHandlerContext->isAcrPercentCheckRunning = FALSE;
//													}
//												}
//												pRecoveryHandlerContext->acrBackupPercent = 0;
//											}
//											break;
//										}
//										else if (acrActionStatus == Restoring)
//										{
//											memset(acrMsg, 0, sizeof(acrMsg));
//											sprintf(acrMsg, "%s,%s,%s,%s",
//													"Eventlog", tempDateTimeStr,
//													tempMsgStr, "Restoring");
//											{
//												char * pSendVal = NULL;
//												char * str = acrMsg;
//												int jsonStrlen = Pack_string(str, &pSendVal);
//												if (jsonStrlen > 0&& pSendVal != NULL)
//												{
//													g_sendcbf(&g_PluginInfo,
//															rcvy_log_rep,
//															pSendVal,
//															strlen(pSendVal)
//																	+ 1, NULL,
//															NULL);
//												}
//												if (pSendVal)
//													free(pSendVal);
//											}
//										}
//									}
//								}
//								app_os_CloseHandle(pAcrTempLogFile);
//							}
//
//							if (isEmptyEventLog)
//							{
//								memset(acrMsg, 0, sizeof(acrMsg));
//								switch (acrActionStatus)
//								{
//								case Backuping: {
//									sprintf(acrMsg, "%s,%s,%s,%s", "Eventlog",
//											curDateTimeStr, curMsg,
//											"BackingUp");
//									break;
//								}
//								case Restoring: {
//									sprintf(acrMsg, "%s,%s,%s,%s", "Eventlog",
//											curDateTimeStr, curMsg,
//											"Restoring");
//									break;
//								}
//								case None: {
//									sprintf(acrMsg, "%s,%s,%s,%s", "Eventlog",
//											curDateTimeStr, "BackupStart",
//											"FirstAgentRun");
//									break;
//								}
//								default:
//									break;
//								}
//								if (strlen(acrMsg))
//								{
//									{
//										char * pSendVal = NULL;
//										char * str = acrMsg;
//										int jsonStrlen = Pack_string(str, &pSendVal);
//										if (jsonStrlen > 0 && pSendVal != NULL)
//										{
//											g_sendcbf(&g_PluginInfo,
//													rcvy_log_rep, pSendVal,
//													strlen(pSendVal) + 1, NULL,
//													NULL);
//										}
//										if (pSendVal)
//											free(pSendVal);
//									}
//								}
//							}
//						}
//						else
//						{
//							memset(acrMsg, 0, sizeof(acrMsg));
//							switch (acrActionStatus)
//							{
//							case Backuping: {
//								sprintf(acrMsg, "%s,%s,%s,%s", "Eventlog",
//										"PastNoBackuped", "", "BackingUp");
//								break;
//							}
//							case Restoring: {
//								sprintf(acrMsg, "%s,%s,%s,%s", "Eventlog",
//										"PastNoBackuped", "", "Restoring");
//								break;
//							}
//							case None: {
//								sprintf(acrMsg, "%s,%s,%s,%s", "Eventlog",
//										"PastNoBackuped", "", "FirstAgentRun");
//								break;
//							}
//							default:
//								break;
//							}
//							if (strlen(acrMsg))
//							{
//								char * pSendVal = NULL;
//								char * str = acrMsg;
//								int jsonStrlen = Pack_string(str, &pSendVal);
//								if (jsonStrlen > 0 && pSendVal != NULL)
//								{
//									g_sendcbf(&g_PluginInfo, rcvy_log_rep, pSendVal, strlen(pSendVal) + 1, NULL, NULL);
//								}
//								if (pSendVal)
//									free(pSendVal);
//							}
//						}
//					}
//					app_os_file_remove(AcrTempLogPath);
//				}
//			}
//		}
//		else
//		{
//			memset(acrMsg, 0, sizeof(acrMsg));
//			sprintf(acrMsg, "%s,%s,%d", "InstallCommand", "InstallFail", 100); //different 2.1, Downlad progree
//			{
//				char * pSendVal = NULL;
//				char * str = acrMsg;
//				int jsonStrlen = Pack_string(str, &pSendVal);
//				if (jsonStrlen > 0 && pSendVal != NULL)
//				{
//					g_sendcbf(&g_PluginInfo, rcvy_log_rep, pSendVal, strlen(pSendVal) + 1, NULL, NULL);
//				}
//				if (pSendVal)
//					free(pSendVal);
//			}
//		}
//	}
//#endif /* _is_linux */
//}

