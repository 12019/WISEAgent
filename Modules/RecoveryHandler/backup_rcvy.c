#include "RecoveryHandler.h"
#include "parser_rcvy.h"
#include "public_rcvy.h"
#include "asz_rcvy.h"
#include "status_rcvy.h"
#include "backup_rcvy.h"

//-----------------------------------------------------------------------------
// Local functions declare:
//-----------------------------------------------------------------------------
static CAGENT_PTHREAD_ENTRY(AcrBackupThreadStart, args);
static CAGENT_PTHREAD_ENTRY(AcrBackupPercentCheckThreadStart, args);
static BOOL GetBackupRetMsg(char * backupRetMsg);

//-----------------------------------------------------------------------------
// Global functions define:
//-----------------------------------------------------------------------------
void DoBackup()
{
	recovery_handler_context_t * pRecoveryHandlerContext = (recovery_handler_context_t *)&RecoveryHandlerContext;
	char backupMsg[512] = { 0 };
	char aszExitStatus[16] = { 0 };
	BOOL isReady = FALSE;
	IsBackuping = FALSE;

#ifndef _is_linux
	CheckASZStatus();
	GetASZExistRecord(aszExitStatus);
	if(!_stricmp(aszExitStatus, "True"))
		isReady = TRUE;
#else
	LONGLONG backupNeedSpace = GetBackupNeedSpace();
	if(app_os_is_file_exist(backupFolder) && \
		(backupNeedSpace != -1) && \
		(GetFolderAvailSpace(backupFolder) > backupNeedSpace))
		isReady = TRUE;
#endif /* _is_linux */

	if (isReady)
	{
		if (!pRecoveryHandlerContext->isAcrBackupRunning)
		{
			pRecoveryHandlerContext->isAcrBackupRunning = TRUE;
			if (app_os_thread_create(&pRecoveryHandlerContext->acrBackupThreadHandle, AcrBackupThreadStart, pRecoveryHandlerContext) != 0)
			{
				RecoveryLog(g_loghandle, Normal, "%s()[%d]###Start recovery backup thread failed!\n", __FUNCTION__, __LINE__);
				pRecoveryHandlerContext->isAcrBackupRunning = FALSE;
			} 
			else
			{
				IsBackuping = TRUE;
			}
		}
		else
		{			
			char * str = "AcrBackupRunning";
			SendReplyMsg_Rcvy(str, bkp_busy, rcvy_backup_rep);
		}
	}
#ifndef _is_linux
	else
	{
		LONGLONG minSizeMB = 0, maxSizeMB = 0;
		app_os_GetMinAndMaxSpaceMB(&minSizeMB, &maxSizeMB);
		if(maxSizeMB >= minSizeMB)
		{
			sprintf(backupMsg, "ASZ not exist! ASZSize: %lld,%lld", minSizeMB, maxSizeMB);			
			SendReplyMsg_Rcvy(backupMsg, bkp_noASZ, rcvy_backup_rep);
		}
		else
		{
			sprintf(backupMsg, "%s", "ASZSizeNotEnough");			
			SendReplyMsg_Rcvy(backupMsg, bkp_sizeLack, rcvy_backup_rep);
		}
	}
#else
	else
	{
		if (app_os_is_file_exist(backupFolder))
		{
			sprintf(backupMsg, "%s", "BackupFolderSizeNotEnough");				
			SendReplyMsg_Rcvy(backupMsg, bkp_sizeLack, rcvy_backup_rep);
		}
		else
		{
			sprintf(backupMsg, "%s", "BackupFolder not exist!");
			SendReplyMsg_Rcvy(backupMsg, bkp_noASZ, rcvy_backup_rep);
		}
	}
#endif /* _is_linux */
}


//-----------------------------------------------------------------------------
// Local functions define:
//-----------------------------------------------------------------------------
static CAGENT_PTHREAD_ENTRY(AcrBackupPercentCheckThreadStart, args)
{
	recovery_handler_context_t * pRecoveryHandlerContext = (recovery_handler_context_t *) args;
	int tryCnt = 10;
	char backupPercentMsg[512] = { 0 };
	FILE *pBackupInfoCopyFile = NULL;
	char lineBuf[1024] = { 0 };
	int lastPercentLineCnt = 0, curPercentLineCnt = 0;

	Recovery_debug_print("isAcrPercentCheckRunning=%d", pRecoveryHandlerContext->isAcrPercentCheckRunning);
	while(pRecoveryHandlerContext->isAcrPercentCheckRunning)
	{
		app_os_sleep(pRecoveryHandlerContext->acrPercentCheckIntervalMs);
		if(pRecoveryHandlerContext->isAcrPercentCheck)
		{
			if(!app_os_is_file_exist(BackupInfoFilePath))
			{
				if(tryCnt > 0)
				{
					tryCnt--;
					app_os_sleep(2000);
					continue;
				}
				else
				{
					Recovery_debug_print("break!");
					break;
				}
			}

			FileCopy(BackupInfoFilePath, BackupInfoCopyFilePath);

			pBackupInfoCopyFile  = fopen (BackupInfoCopyFilePath, "rb");
			if(pBackupInfoCopyFile)
			{
				curPercentLineCnt = 0;
				while (!feof(pBackupInfoCopyFile))
				{
					if(!pRecoveryHandlerContext->isAcrPercentCheckRunning) break;

					memset(lineBuf, 0, sizeof(lineBuf));
					if(fgets(lineBuf, sizeof(lineBuf), pBackupInfoCopyFile))
					{
						curPercentLineCnt ++;
						if(curPercentLineCnt <= lastPercentLineCnt) continue;

						if(strstr(lineBuf, "<progress>"))
						{
							char * word[16] = {NULL};
							char *buf = lineBuf;
							int i = 0, curBackupPercent = 0;
							while((word[i] = strtok( buf, ">")) != NULL)
							{
								i++;
								buf=NULL;
							}
							curBackupPercent = atoi(word[1]);
							if (curBackupPercent <= pRecoveryHandlerContext->acrBackupPercent)
								continue;

							pRecoveryHandlerContext->acrBackupPercent = curBackupPercent;
							lastPercentLineCnt = curPercentLineCnt;

							memset(backupPercentMsg, 0, sizeof(backupPercentMsg));
							sprintf(backupPercentMsg, "%s,%s:%d", "ProgressReport", "BackupPercent", pRecoveryHandlerContext->acrBackupPercent);
							SendReplyMsg_Rcvy(backupPercentMsg, bkp_progress, rcvy_backup_rep);
	
							if(pRecoveryHandlerContext->acrBackupPercent == 100)
							{
								fclose(pBackupInfoCopyFile);
								pRecoveryHandlerContext->isAcrPercentCheckRunning = FALSE;
								return 0;
							}
							break;
						}
					}
				}
				fclose(pBackupInfoCopyFile);
			}
		}
	}
	return 0;
}

static CAGENT_PTHREAD_ENTRY(AcrBackupThreadStart, args)
{
	recovery_handler_context_t * pRecoveryHandlerContext = (recovery_handler_context_t *) args;
	char backupMsg[512] = { 0 };
	if (app_os_is_file_exist(BackupInfoFilePath))
		app_os_file_remove(BackupInfoFilePath);
	if (app_os_is_file_exist(BackupInfoCopyFilePath))
		app_os_file_remove(BackupInfoCopyFilePath);
 
	Recovery_debug_print("isAcrPercentCheckRunning=%d", pRecoveryHandlerContext->isAcrPercentCheckRunning);
	if (!pRecoveryHandlerContext->isAcrPercentCheckRunning)
	{
		pRecoveryHandlerContext->acrPercentCheckIntervalMs = DEF_BACKUP_PERCENT_CHECK_INTERVAL_MS;
		pRecoveryHandlerContext->isAcrPercentCheck = FALSE;
		pRecoveryHandlerContext->isAcrPercentCheckRunning = TRUE;
		pRecoveryHandlerContext->acrBackupPercentCheckThreadHandle = NULL;
		if (app_os_thread_create(&pRecoveryHandlerContext->acrBackupPercentCheckThreadHandle,\
			AcrBackupPercentCheckThreadStart, pRecoveryHandlerContext) != 0)
		{
			RecoveryLog(g_loghandle, Normal, "%s()[%d]###Start recovery backup percent check thread failed!\n",	__FUNCTION__, __LINE__);
			pRecoveryHandlerContext->isAcrPercentCheckRunning = FALSE;
			goto done1;
		}
	}
	pRecoveryHandlerContext->acrBackupPercent = 0;

	{
#ifndef _is_linux
		char cmdLine[BUFSIZ] = "cmd.exe /c ";
		char outputPath[MAX_PATH] = "c:\\AcrBackupCmdOutput.txt";
#else
		char cmdLine[BUFSIZ] = { 0 };
		char outputPath[MAX_PATH] = "/tmp/AcrBackupCmdOutput.txt";
#endif /* _is_linux */

		FILE *pOutput = fopen(outputPath, "wb");
		if (NULL == pOutput)
		{
			RecoveryLog(g_loghandle, Normal, "%s()[%d]###Create %s failed!\n", __FUNCTION__, __LINE__, outputPath);
			pRecoveryHandlerContext->isAcrPercentCheckRunning = FALSE;
			goto done1;
		}
		fclose(pOutput);

		//send backup start message
		memset(backupMsg, 0, sizeof(backupMsg));
		sprintf(backupMsg, "%s", "BackupStart");		
		SendReplyMsg_Rcvy(backupMsg, bkp_start, rcvy_backup_rep);	

#ifndef _is_linux
		sprintf(cmdLine, "%s \"%s\" ", cmdLine, BackupBatPath);
		//{
		//	char backupHelperPath[MAX_PATH] = {0};
		//	char moudlePath[MAX_PATH] = {0};
		//	app_os_get_module_path(moudlePath);
		//	sprintf(backupHelperPath, "%s%s", moudlePath, "BackupHelper.exe");
		//	sprintf(cmdLine, "%s \"%s\" ", cmdLine, backupHelperPath);
		//}

		pRecoveryHandlerContext->isAcrPercentCheck = TRUE; // check start
		if(!app_os_CreateProcess(cmdLine))
		{
			RecoveryLog(g_loghandle, Normal, "%s()[%d]###Create acrocmd process failed!\n",__FUNCTION__, __LINE__);
			pRecoveryHandlerContext->isAcrPercentCheckRunning = FALSE;
			goto done1;
		}
#else
		{
			char linuxVolume[512] = {0};
			GetVolume(linuxVolume);
			if (strlen(linuxVolume) > 0) 
			{
				FILE *bkpVolumeFd = NULL;
				sprintf(BackupVolume, "%s", linuxVolume);
				bkpVolumeFd = fopen(BkpVolumeArgFilePath, "wb");
				if (bkpVolumeFd) 
				{
					fprintf(bkpVolumeFd, "volume=%s", linuxVolume);
					fclose(bkpVolumeFd);
				}
			}
			else
			{
				Recovery_debug_print("Get volume failed!");
				RecoveryLog(g_loghandle, Normal,"%s()[%d]###Get volume failed!\n", __FUNCTION__, __LINE__);
			}
		}
		sprintf(cmdLine, "%s %s %s", BackupBatPath, BackupVolume, outputPath);
		pRecoveryHandlerContext->isAcrPercentCheck = TRUE; // check start
		if (!app_os_CreateProcess(cmdLine)) {
			RecoveryLog(g_loghandle, Normal,"%s()[%d]###Create acrocmd process failed!\n", __FUNCTION__, __LINE__);
			pRecoveryHandlerContext->isAcrPercentCheckRunning = FALSE;
			goto done1;
		}
#endif /* _is_linux */
		else
		{
			pRecoveryHandlerContext->isAcrPercentCheckRunning = FALSE;
			pRecoveryHandlerContext->isAcrPercentCheck = FALSE;
			if (0 > app_os_thread_join(pRecoveryHandlerContext->acrBackupPercentCheckThreadHandle))
			{
				Recovery_debug_print("acrBackupPercentCheckThreadHandle thread join error!");
			}

			//send backup end message
			memset(backupMsg, 0, sizeof(backupMsg));
			sprintf(backupMsg, "%s", "BackupEnd");	
			SendReplyMsg_Rcvy(backupMsg, bkp_end, rcvy_backup_rep);
			
			GetRcvyCurStatus();
			//Send backup result message
			{
				char backupRetMsg[128] = { 0 };
				BOOL bRet = GetBackupRetMsg(backupRetMsg);
				memset(backupMsg, 0, sizeof(backupMsg));
				if (bRet) {
					sprintf(backupMsg, "%s", backupRetMsg);
					if (!strcmp(backupMsg,"Backup error"))
						SendReplyFailMsg_Rcvy(backupMsg, rcvy_backup_rep);//Backup error
					else
						SendReplySuccessMsg_Rcvy(backupMsg, rcvy_backup_rep);//Backup success
				} 
				else 
				{
					sprintf(backupMsg, "%s", "Not found backup result message");					
					SendReplyMsg_Rcvy(backupMsg, bkp_noResult, rcvy_backup_rep);	
				}
			}
		}
		FileAppend(outputPath, AcrHistoryFilePath);
		app_os_file_remove(outputPath);
	}

	pRecoveryHandlerContext->isAcrBackupRunning = FALSE;
done1:
	IsBackuping = FALSE;
	return 0;
}

static BOOL GetBackupRetMsg(char * backupRetMsg)
{
#define DEF_DB_RESTORE_RET_QUERY_SQL_STR  "SELECT * FROM Recovery ORDER BY DateTime DESC LIMIT 1" 
#define DEF_DB_INSERT_FORMAT_STR          "INSERT INTO Recovery (Source, Message, EventID, DateTime) values('%s', '%s', %d, datetime('%s', 'localtime'))"
	BOOL bRet = FALSE;
	sqlite3 * rcvyDB = NULL;
	int iRet;
	char * errmsg = NULL;
	iRet = sqlite3_open(rcvyDBFilePath, &rcvyDB);
	if(iRet != SQLITE_OK) goto done1;
	else
	{
		char ** retTable = NULL;
		int nRow = 0, nColumn = 0;
		int i = 0;
		iRet = sqlite3_get_table(rcvyDB, DEF_DB_RESTORE_RET_QUERY_SQL_STR, &retTable, &nRow, &nColumn, &errmsg);
		if(iRet != SQLITE_OK) 
		{
			sqlite3_free(errmsg);
			goto done2;
		}
		else
		{
			char tmpSourceStr[128] = {0};
			char tmpMsgStr[BUFSIZ] = {0};
			int tmpEventID = 0;
			char tmpDateTimeStr[32] = {0};
			for(i = 0; i < nRow; i++)
			{
				strcpy(tmpSourceStr, retTable[(i+1)*nColumn]);
				strcpy(tmpMsgStr, retTable[(i+1)*nColumn+1]);
				tmpEventID = atoi(retTable[(i+1)*nColumn+2]);
				strcpy(tmpDateTimeStr, retTable[(i+1)*nColumn+3]);
			}

			if(!strcmp(tmpMsgStr, DEF_BACKUP_SUCCESS) && tmpEventID == DEF_BACKUP_SUCCESS_ID)
			{
				char sqlStr[256] = {0};
				sprintf_s(sqlStr, sizeof(sqlStr), DEF_DB_INSERT_FORMAT_STR, "Acronis", DEF_ACR_CHECK, DEF_ACR_CHECK_ID, "now");
				iRet = sqlite3_exec(rcvyDB, sqlStr, 0, 0, &errmsg);
				if(iRet != SQLITE_OK)
				{
					sqlite3_free(errmsg);
					goto done2;
				}

				strcpy(backupRetMsg, "Backup success");
				bRet = TRUE;
			}
			else if(!strcmp(tmpMsgStr, DEF_BACKUP_ERROR) && tmpEventID == DEF_BACKUP_ERROR_ID)
			{
				char sqlStr[256] = {0};
				sprintf_s(sqlStr, sizeof(sqlStr), DEF_DB_INSERT_FORMAT_STR, "Acronis", DEF_ACR_CHECK, DEF_ACR_CHECK_ID, "now");
				iRet = sqlite3_exec(rcvyDB, sqlStr, 0, 0, &errmsg);
				if(iRet != SQLITE_OK)
				{
					sqlite3_free(errmsg);
					goto done2;
				}
				strcpy(backupRetMsg, "Backup error");
				bRet = TRUE;
			}

			if(retTable) sqlite3_free_table(retTable);
		}
	}
done2:
	if(rcvyDB) sqlite3_close(rcvyDB);
done1:
	return bRet;
}

