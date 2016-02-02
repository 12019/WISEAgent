#include "RecoveryHandler.h"
#include "parser_rcvy.h"
#include "asz_rcvy.h"
#include "restore_rcvy.h"

//-----------------------------------------------------------------------------
// Local functions declare:
//-----------------------------------------------------------------------------
static CAGENT_PTHREAD_ENTRY(AcrRestoreThreadStart, args);

//-----------------------------------------------------------------------------
// Global functions define:
//-----------------------------------------------------------------------------
void DoRestore()
{
	recovery_handler_context_t * pRecoveryHandlerContext = (recovery_handler_context_t *)&RecoveryHandlerContext;
	if(!pRecoveryHandlerContext->isAcrRestoreRunning)
	{
		pRecoveryHandlerContext->isAcrRestoreRunning = TRUE;
		if (app_os_thread_create(&pRecoveryHandlerContext->acrRestoreThreadHandle, AcrRestoreThreadStart, pRecoveryHandlerContext) != 0)
		{
			RecoveryLog(g_loghandle, Normal, "%s()[%d]###Start recovery restore thread failed!\n",__FUNCTION__, __LINE__ );
			pRecoveryHandlerContext->isAcrRestoreRunning = FALSE;
		}
	}
	else
	{
		char * pSendVal = NULL;
		char * str = "AcrRestoreRunning";
		SendReplyMsg_Rcvy(str, rstr_busy, rcvy_restore_rep);
	}
}

BOOL GetRestoreRetMsg(char * restoreRetMsg)
{
#define DEF_DB_RESTORE_RET_QUERY_SQL_STR  "SELECT * FROM Recovery ORDER BY DateTime DESC LIMIT 1" 
#define DEF_DB_INSERT_FORMAT_STR          "INSERT INTO Recovery (Source, Message, EventID, DateTime) values('%s', '%s', %d, datetime('%s', 'localtime'))"
	BOOL bRet = FALSE;
	sqlite3 * rcvyDB = NULL;
	int iRet;
	char * errmsg = NULL;
	iRet = sqlite3_open(rcvyDBFilePath, &rcvyDB);
	if (iRet != SQLITE_OK)
		goto done1;
	else
	{
		char ** retTable = NULL;
		int nRow = 0, nColumn = 0;
		int i = 0;
		iRet = sqlite3_get_table(rcvyDB, DEF_DB_RESTORE_RET_QUERY_SQL_STR, &retTable, &nRow, &nColumn, &errmsg);
		if (iRet != SQLITE_OK)
		{
			sqlite3_free(errmsg);
			goto done2;
		}
		else
		{
			char tmpSourceStr[128] = { 0 };
			char tmpMsgStr[BUFSIZ] = { 0 };
			int tmpEventID = 0;
			char tmpDateTimeStr[32] = { 0 };
			for (i = 0; i < nRow; i++)
			{
				strcpy(tmpSourceStr, retTable[(i + 1) * nColumn]);
				strcpy(tmpMsgStr, retTable[(i + 1) * nColumn + 1]);
				tmpEventID = atoi(retTable[(i + 1) * nColumn + 2]);
				strcpy(tmpDateTimeStr, retTable[(i + 1) * nColumn + 3]);
			}

			if (!strcmp(tmpMsgStr, DEF_BACKUP_START) && tmpEventID == DEF_BACKUP_START_ID)
			{
				char sqlStr[256] = { 0 };
				sprintf_s(sqlStr, sizeof(sqlStr), DEF_DB_INSERT_FORMAT_STR, "Acronis", DEF_RESTORE_SUCCESS, DEF_RESTORE_SUCCESS_ID, "now");
				iRet = sqlite3_exec(rcvyDB, sqlStr, 0, 0, &errmsg);
				if(iRet != SQLITE_OK)
				{
					sqlite3_free(errmsg);
					goto done2;
				}

				strcpy(restoreRetMsg, "Restore success");
				bRet = TRUE;
			}
			else if(!strcmp(tmpMsgStr, DEF_RESTORE_REEOR) && tmpEventID == DEF_RESTORE_ERROR_ID)
			{
				char sqlStr[256] = {0};
				sprintf_s(sqlStr, sizeof(sqlStr), DEF_DB_INSERT_FORMAT_STR, "Acronis", DEF_ACR_CHECK, DEF_ACR_CHECK_ID, "now");
				iRet = sqlite3_exec(rcvyDB, sqlStr, 0, 0, &errmsg);
				if(iRet != SQLITE_OK)
				{
					sqlite3_free(errmsg);
					goto done2;
				}
				strcpy(restoreRetMsg, "Restore error");
				bRet = TRUE;
			}

			if (retTable)
				sqlite3_free_table(retTable);
		}
	}
done2: if (rcvyDB)
		   sqlite3_close(rcvyDB);
done1: return bRet;
}


//-----------------------------------------------------------------------------
// Local functions define:
//-----------------------------------------------------------------------------
static CAGENT_PTHREAD_ENTRY(AcrRestoreThreadStart, args)
{
	recovery_handler_context_t * pRecoveryHandlerContext = (recovery_handler_context_t *)args;
#ifndef _is_linux
	char cmdLine[BUFSIZ] = "cmd.exe /c ";
	sprintf(cmdLine, "%s \"%s\" ", cmdLine, RestoreBatPath);
#else
	char cmdLine[BUFSIZ] = {0};
	char targetVolume[512] = {0};
	if (strlen(BackupVolume) == 0)
	{
		char buf[BUFSIZ] = {0};
		FILE *bkpVolumeFd = fopen(BkpVolumeArgFilePath, "rb");
		if (bkpVolumeFd)
		{
			fgets(buf, sizeof(buf)-1, bkpVolumeFd);
			strncpy(BackupVolume, trimwhitespace(strchr(buf, '=') + 1), sizeof(BackupVolume)-1);
			fclose(bkpVolumeFd);
			RecoveryLog(g_loghandle, Normal,"%s()[%d]###BackupVolume: %s\n", \
				__FUNCTION__, __LINE__, BackupVolume);
		}
		else
		{			
			Recovery_debug_print("Open file \'%s\' failed!", BkpVolumeArgFilePath);
			RecoveryLog(g_loghandle, Normal,"%s()[%d]###Open file \'%s\' failed!\n", \
				__FUNCTION__, __LINE__, BkpVolumeArgFilePath);
		}
	}

	{
		char linuxVolume[512] = {0};
		GetVolume(linuxVolume);
		if (strlen(linuxVolume) > 0)
		{
			sprintf(targetVolume, "%s", linuxVolume);
		}
		else
		{
			Recovery_debug_print("Get target volume failed!");
			RecoveryLog(g_loghandle, Normal,"%s()[%d]###Get target volume failed!\n", __FUNCTION__, __LINE__);
		}
	}
	Recovery_debug_print("BackupVolume:%s", BackupVolume);
	sprintf(cmdLine, "%s %s %s", RestoreBatPath, BackupVolume, targetVolume);
#endif /* _is_linux */

	//Send restore start message
	{
		char * str = "RestoreStart";
		SendReplyMsg_Rcvy(str, rstr_start, rcvy_restore_rep);
	}

	IsRestoring = TRUE;
	if(!app_os_CreateProcess(cmdLine))
	{
		RecoveryLog(g_loghandle, Normal, "%s()[%d]###Create restore bat process failed!\n",__FUNCTION__, __LINE__ );
	}
	else
	{
		//Send restore end message
		{
			char * str = "RestoreEnd";
			SendReplyMsg_Rcvy(str, rstr_end, rcvy_restore_rep);
		}

		//Send restore result message
		{
			char restoreRetMsg[128] = {0};
			GetRestoreRetMsg(restoreRetMsg);
			if(strlen(restoreRetMsg))
			{
				if (!strcmp("Restore error", restoreRetMsg))
					SendReplyFailMsg_Rcvy(restoreRetMsg, rcvy_restore_rep);					
				else
					SendReplySuccessMsg_Rcvy(restoreRetMsg, rcvy_backup_rep);
			}
		}
	}
	IsRestoring = FALSE;
	pRecoveryHandlerContext->isAcrRestoreRunning = FALSE;
	app_os_CloseHandle(pRecoveryHandlerContext->acrRestoreThreadHandle);
	pRecoveryHandlerContext->acrRestoreThreadHandle = NULL;

	GetRcvyCurStatus();

	return 0;
}

