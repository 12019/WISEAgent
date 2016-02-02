/****************************************************************************/
/* Copyright(C)   : Advantech Technologies, Inc.							*/
/* Create Date    : 2015/01/19 by lige									    */
/* Modefied Date  : 2015/05/20 by guolin.huang								*/
/* Abstract       : Backup&Recovery API interface definition   				*/
/* Reference      : None													*/
/****************************************************************************/

#include "RecoveryHandler.h"
#include "public_rcvy.h"
#include "parser_rcvy.h"
#include "backup_rcvy.h"
#include "restore_rcvy.h"
#include "asz_rcvy.h"
#include "activate_rcvy.h"
#include "status_rcvy.h"
#include "install_update_rcvy.h"
#include "capability_rcvy.h"

//-----------------------------------------------------------------------------
// Global variables define:
//-----------------------------------------------------------------------------
Plugin_info  g_PluginInfo;
void* g_loghandle = NULL;
BOOL g_bEnableLog = true;
char SERVER_IP[512] = {0};
const char MyTopic[MAX_TOPIC_LEN] = {"recovery"};
const int RequestID = cagent_request_recovery;
const int ActionID = cagent_reply_recovery;

HandlerSendCbf 			g_sendcbf = NULL;		// Client Send information (in JSON format) to Cloud Server
HandlerSendCustCbf 		g_sendcustcbf = NULL;	// Client Send information (in JSON format) to Cloud Server with custom topic
HandlerAutoReportCbf	g_sendreportcbf = NULL;	// Client Send report (in JSON format) to Cloud Server with AutoReport topic
recovery_handler_context_t  RecoveryHandlerContext;

char rcvyDBFilePath[MAX_PATH] = {0};
CAGENT_MUTEX_TYPE  CSWMsg;
CAGENT_MUTEX_TYPE  CSAMsg;
CAGENT_MUTEX_TYPE  Mutex_AcrReady;
BOOL IsAcrReady = FALSE;
BOOL IsBackuping = FALSE;
BOOL IsRestoring = FALSE;
char LastWarningMsg[1024] = {0};
char ActionMsg[1024] = {0};
char AcrCmdLineToolPath[MAX_PATH] = {0};
char AcroCmdExePath[MAX_PATH] = {0};
char BackupInfoFilePath[MAX_PATH] = {0};
char BackupInfoCopyFilePath[MAX_PATH] = {0};
char AcrHistoryFilePath[MAX_PATH] = {0};
char BackupBatPath[MAX_PATH] = {0};
char RestoreBatPath[MAX_PATH] = {0};
//char AcrLogPath[MAX_PATH] = {0};
//char AcrTempLogPath[MAX_PATH] = {0};

#ifdef _is_linux
char BackupVolume[512] = {0};
char backupFolder[MAX_PATH] = {0};
char BkpVolumeArgFilePath[MAX_PATH] = {0};
#else
BOOL IsAcrInstall = FALSE;
BOOL IsAcrInstallThenActive = FALSE;
BOOL IsUpdateAction = FALSE;
BOOL IsInstallAction = FALSE;
BOOL IsDownloadAction = FALSE;
EINSTALLTYPE CurInstallType = ITT_UNKNOWN;
char AcrInstallerPath[MAX_PATH] = {0};
char OtherAcronisPath[MAX_PATH] = {0};
char DeactivateHotkeyProPath[MAX_PATH] = {0};
char ActivateHotkeyProPath[MAX_PATH] = {0};
#endif /*_is_linux*/

//-----------------------------------------------------------------------------
// Local macros define:
//-----------------------------------------------------------------------------
#define DEF_ACR_FOLDER_NAME                      "Acronis"

#ifndef _is_linux
#define DEF_ACRONIS_COMMANDLINE_TOOL_NAME         "\\Acronis\\CommandLineTool"
#define DEF_ACRO_CMD_EXE_NAME                     "\\acrocmd.exe"
#define DEF_ACRONIS_HISTORY_FILE_NAME             "\\Acronis\\History.log"
#define DEF_OTHER_ACRONIS_NAME                    "\\Acronis\\TrueImageHome\\TrueImageLauncher.exe"
#define DEF_BACKUP_INFO_FILE_NAME                 "\\Temp\\BackupInfo.txt"
#define DEF_BACKUP_INFO_COPY_FILE_NAME            "\\Temp\\BackupInfo_Copy.txt"
#define DEF_ACR_INSTALLER_NAME					  DEF_ACRONIS_INSTALLER_NAME
#define DEF_ACR_LATESVERSION_NAME                 "LatestVersion.txt"
#define DEF_ADVANTECH_FOLDER_NAME                 "Advantech"
#define DEF_ACRONIS_LOG_NAME                      "\\Advantech\\Acronis\\acrLog.txt"
#define DEF_ACRONIS_TEMP_LOG_NAME                 "\\Advantech\\Acronis\\acrTempLog.txt"
#define DEF_BACKUP_BAT_NAME                       "\\Advantech\\Acronis\\backup.bat"
#define DEF_RESTORE_BAT_NAME                      "\\Advantech\\Acronis\\restore.bat"
#define DEF_ACTIVATE_ASRM_BAT_NAME                "\\Advantech\\Acronis\\ActivateASRM.bat"
#define DEF_DEACTIVATE_ASRM_BAT_NAME              "\\Advantech\\Acronis\\DeactivateASRM.bat"

#else
#define DEF_ACRONIS_COMMANDLINE_TOOL_NAME         "/Acronis/CommandLineTool"
#define DEF_ACRO_CMD_EXE_NAME                     "/acrocmd"
#define DEF_ACRONIS_HISTORY_FILE_NAME             "/Acronis/History.log"
#define DEF_BACKUP_INFO_FILE_NAME                 "/tmp/BackupInfo.txt"
#define DEF_BACKUP_INFO_COPY_FILE_NAME            "/tmp/BackupInfo_Copy.txt"
#define DEF_BACKUP_BAT_NAME                       "/Acronis/backup.sh"
#define DEF_RESTORE_BAT_NAME                      "/Acronis/restore.sh"
#define DEF_ACRONIS_LOG_NAME                      "/Acronis/acrLog.txt"
#define DEF_ACRONIS_TEMP_LOG_NAME                 "/Acronis/acrTempLog.txt"
#define DEF_BACKUPDAT_LOCATION_CONF				  "/Acronis/BackupLocation.conf"
#define DEF_BACKUPDAT_LOCATION_DEFAULT	 	      "/backup"
#define DEF_BACKUP_VOLUME_ARG_LATEST_RECORD		  "/Acronis/BkpVolumeArgLatestRecord.txt"

#define SHELL_FILEHEAD		"#!/bin/sh"
#define CREATE_TABLE		"CREATE TABLE IF NOT EXISTS Recovery(Source nvarchar(100), Message nvarchar(100),EventID nvarchar(100), DateTime datetime);"
#define INSERT_VALUES		"INSERT INTO Recovery(Source, Message, EventID, DateTime) values(\\\"%s\\\", \\\"%s\\\", %d, datetime('now','localtime'));"

#define BACKUPACTION		"acrocmd backup disk --volume=$1 --loc=%s --arc=BK_ASZ2 --backuptype=incremental --silent_mode=on --log=/tmp/BackupInfo.txt > $2"
//#define BACKUPACTION		"acrocmd backup disk --volume=$1 --loc=atis:///asz --arc=BK_ASZ2 --backuptype=incremental --silent_mode=on --log=/tmp/BackupInfo.txt > $2"
#define RESTOREACTION		"acrocmd recover disk --loc=%s --arc=BK_ASZ2 --volume=$1 --target_volume=$2 --reboot"
//#define RESTOREACTION		"acrocmd recover disk --loc=atis:///asz --arc=BK_ASZ2 --volume=$1 --reboot"
#endif /* _is_linux */

#define DEF_RECOVERY_DB_FILE_NAME                "SystemRecovery.db"
#define DEF_RCVY_TABLE_NAME                      "Recovery"
#define DEF_CREATE_RCVY_TABLE_SQL_FORMAT_STR     "CREATE TABLE IF NOT EXISTS %s (Source nvarchar(100), Message nvarchar(100), EventID nvarchar(100), DateTime datetime)"

//-----------------------------------------------------------------------------
// Local variables define:
//-----------------------------------------------------------------------------
static char AcrPath[MAX_PATH] = {0};
static char AcronisPath[MAX_PATH] = {0};
//static char AdvantchPath[MAX_PATH] = {0};
//static char AcrLatestVersionPath[MAX_PATH] = {0};
static BOOL IsRcvyDBCheckThreadRunning = FALSE;
static BOOL IsAcrReadyCheckThreadRunning = FALSE;
static int  AcrInstallASZPersent = 0;
static CAGENT_THREAD_TYPE RcvyDBChangeHandle = INVALID_HANDLE_VALUE;
static CAGENT_THREAD_TYPE RcvyDBCheckThreadHandle = NULL;
static sqlite3 * SysRcvyDB = NULL;

//-----------------------------------------------------------------------------
// Local function declare:
//-----------------------------------------------------------------------------
static void InitRecoveryPath();
static void InitBatPath();
static BOOL InitRcvyDB();
static BOOL UninitRcvyDB();
static void AcronisReadyCheckStart();
static void AcronisReadyCheckStop();

#ifndef _is_linux
static BOOL ProcessRcvyDBChange();
static CAGENT_PTHREAD_ENTRY(RcvyDBCheckThreadStart, args);
static CAGENT_PTHREAD_ENTRY(AcrReadyCheckThreadStart, args);
#else
static void InitBackupScript(void);
static void InitRestoreScript(void);
static void InitShellScript(void);
#endif /*_is_linux*/

//-----------------------------------------------------------------------------
// Local function define:
//-----------------------------------------------------------------------------
#ifndef _is_linux
static BOOL ProcessRcvyDBChange()
{
#define DEF_DB_CHANGE_QUERY_SQL_STR  "SELECT * FROM Recovery ORDER BY DateTime DESC LIMIT 1"
	BOOL bRet = FALSE;
	static char prevMsg[256] = {0};
	if(IsInstallerRunning())
	{
		if(SysRcvyDB)
		{
			int iRet;
			char * errmsg = NULL;
			char ** retTable = NULL;
			int nRow = 0, nColumn = 0;
			int i = 0;
			iRet = sqlite3_get_table(SysRcvyDB, DEF_DB_CHANGE_QUERY_SQL_STR, &retTable, &nRow, &nColumn, &errmsg);
			if(iRet != SQLITE_OK) sqlite3_free(errmsg);
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
				if(!strcmp(tmpSourceStr, "AcronisInstallCommand"))
				{
					if(strcmp(prevMsg, tmpMsgStr))
					{
						char installMsg[256] = {0};
						sprintf(installMsg, "%s,%s,%s","InstallCommand", "InstallingMsg", tmpMsgStr);
						memset(prevMsg, 0, sizeof(prevMsg));
						strcpy(prevMsg, tmpMsgStr);
						AcrInstallMsgSend(CurInstallType, installMsg, istl_updt_istlMsg);
					}
				}
			}
			if(retTable) sqlite3_free_table(retTable);
		}
	}
	return bRet;
}

static CAGENT_PTHREAD_ENTRY(RcvyDBCheckThreadStart, args)
{
	while(IsRcvyDBCheckThreadRunning)
	{
		if(RcvyDBChangeHandle == INVALID_HANDLE_VALUE)
		{
			RcvyDBChangeHandle = app_os_FindFirstChangeNotification(AcronisPath, FALSE, FILE_NOTIFY_CHANGE_SIZE);
			app_os_sleep(100);
		}
		else
		{
			DWORD dwRet = app_os_WaitForSingleObject(RcvyDBChangeHandle, 3000);
			if(dwRet == WAIT_TIMEOUT || dwRet == WAIT_OBJECT_0)
		 {
			 if(!IsRcvyDBCheckThreadRunning) break;
		 }

			if(dwRet == WAIT_OBJECT_0)
		 {
			 app_os_sleep(100);
			 ProcessRcvyDBChange();
			 app_os_FindNextChangeNotification(RcvyDBChangeHandle);
		 }
		}
		app_os_sleep(10);
	}
	if(RcvyDBChangeHandle != INVALID_HANDLE_VALUE)
	{
		app_os_FindCloseChangeNotification(RcvyDBChangeHandle);
		RcvyDBChangeHandle = INVALID_HANDLE_VALUE;
	}
	return 0;
}

#else

static void InitBackupScript(void)
{
	char buf[BUFSIZ] = {0};

	//Create file: backup.sh
	FILE *fd = fopen(BackupBatPath, "w");
	fprintf(fd, "%s\n\n", SHELL_FILEHEAD);

	//sqlite3 opration--insert backupstart info
	sprintf(buf, INSERT_VALUES, "Acronis", DEF_BACKUP_START, DEF_BACKUP_START_ID);
	fprintf(fd, "sqlite3 %s \"%s %s\"\n\n", rcvyDBFilePath, CREATE_TABLE, buf);

	//Backup action
	sprintf(buf, BACKUPACTION, backupFolder);
	fprintf(fd, "%s\n\n", buf);

	//sqlite3 opration--insert backup result
	fprintf(fd, "if [ $? -ne 0 ]; then \n");
	fprintf(fd, "\techo \"[Backup] an error occurred\" \n\tsleep 1s\n");
	sprintf(buf, INSERT_VALUES, "Acronis", DEF_BACKUP_ERROR, DEF_BACKUP_ERROR_ID);
	fprintf(fd, "\tsqlite3 %s \"%s\"\nelse\n", rcvyDBFilePath, buf);
	fprintf(fd, "\techo \"[Backup] successful\" \n\tsleep 1s\n");
	sprintf(buf, INSERT_VALUES, "Acronis", DEF_BACKUP_SUCCESS, DEF_BACKUP_SUCCESS_ID);
	fprintf(fd, "\tsqlite3 %s \"%s\"\nfi\n", rcvyDBFilePath, buf);
	fprintf(fd, "exit\n");
	fclose(fd);

	//Read & write by owner, execute/search by owner & group
	chmod(BackupBatPath, S_IRUSR | S_IWUSR | S_IXUSR | S_IXGRP);
}

static void InitRestoreScript(void)
{
	char buf[BUFSIZ] = {0};

	//Create file: backup.sh
	FILE *fd = fopen(RestoreBatPath, "w");
	fprintf(fd, "%s\n\n", SHELL_FILEHEAD);

	//sqlite3 opration--insert Restorestart info
	sprintf(buf, INSERT_VALUES, "Acronis", DEF_RESTORE_START, DEF_RESTORE_START_ID);
	fprintf(fd, "sqlite3 %s \"%s %s\"\n\n", rcvyDBFilePath, CREATE_TABLE, buf);

	//Restore action
	sprintf(buf, RESTOREACTION, backupFolder);
	fprintf(fd, "%s\n\n", buf);

	//sqlite3 opration--insert Restore result
	fprintf(fd, "if [ $? -ne 0 ]; then \n");
	fprintf(fd, "\techo \"[Restore] an error occurred\" \n\tsleep 1s\n");
	sprintf(buf, INSERT_VALUES, "Acronis", DEF_RESTORE_REEOR, DEF_RESTORE_ERROR_ID);
	fprintf(fd, "\tsqlite3 %s \"%s\"\nelse\n", rcvyDBFilePath, buf);
	fprintf(fd, "\techo \"[Restore] successful\" \n\tsleep 1s\n");
	sprintf(buf, INSERT_VALUES, "Acronis", DEF_RESTORE_SUCCESS, DEF_RESTORE_SUCCESS_ID);
	fprintf(fd, "\tsqlite3 %s \"%s\"\nfi\n", rcvyDBFilePath, buf);
	fprintf(fd, "exit\n");
	fclose(fd);

	//Read & write by owner, execute/search by owner & group
	chmod(RestoreBatPath, S_IRUSR | S_IWUSR | S_IXUSR | S_IXGRP);
}

static void InitShellScript(void)
{
	if (!app_os_is_file_exist(BackupBatPath))
	{
		InitBackupScript();
	}
	
	if (!app_os_is_file_exist(RestoreBatPath))
	{
		InitRestoreScript();
	}
}
#endif /*_is_linux*/

static void InitRecoveryPath()
{
	char progFilesPath[MAX_PATH] = {0};		
	/*char * cagentPath[MAX_PATH] = {0};*/
	if(!app_os_GetDefProgramFilesPath(progFilesPath)) return;
	Recovery_debug_print("programpath:%s", progFilesPath);

#ifndef _is_linux
	{
		char modulePath[MAX_PATH] = {0};
		char *ptr = NULL;
		if(app_os_get_module_path(modulePath))
		{
			ptr = strstr(modulePath, "Modules");
			if (ptr--)
				*ptr = "\0";// delete sub-string from last slash
			sprintf(AcrInstallerPath, "%s\\Downloads\\", modulePath);			
		}
		else 
		{			
			sprintf(AcrInstallerPath, "%s\\%s\\%s\\Downloads\\", progFilesPath, DEF_ADVANTECH_FOLDER_NAME, "SUSIAccess 3.1 Agent");
		}
		app_os_create_directory(AcrInstallerPath);
		strcat(AcrInstallerPath, DEF_ACR_INSTALLER_NAME);
	}
	{
		char * winPath = NULL;
		winPath = getenv("WINDIR");
		if(winPath)
		{
			sprintf(BackupInfoFilePath, "%s%s", winPath, DEF_BACKUP_INFO_FILE_NAME);
			sprintf(BackupInfoCopyFilePath, "%s%s", winPath, DEF_BACKUP_INFO_COPY_FILE_NAME);
			//sprintf(AcrInstallerPath, "%s\\Temp\\%s\\", winPath, DEF_ACR_FOLDER_NAME);
		}
		else
		{
			sprintf(BackupInfoFilePath, "%s%s", "C:\\Windows", DEF_BACKUP_INFO_FILE_NAME);
			sprintf(BackupInfoCopyFilePath, "%s%s", "C:\\Windows", DEF_BACKUP_INFO_COPY_FILE_NAME);
			//sprintf(AcrInstallerPath, "C:\\Windows\\Temp\\%s\\", DEF_ACR_FOLDER_NAME);
		}
		//app_os_create_directory(AcrInstallerPath);
		//strcat(AcrInstallerPath, DEF_ACR_INSTALLER_NAME);
	}
	sprintf(AcrHistoryFilePath, "%s%s", progFilesPath, DEF_ACRONIS_HISTORY_FILE_NAME);
	sprintf(OtherAcronisPath, "%s%s", progFilesPath, DEF_OTHER_ACRONIS_NAME);
	sprintf(AcrCmdLineToolPath, "%s%s", progFilesPath, DEF_ACRONIS_COMMANDLINE_TOOL_NAME);
	sprintf(AcroCmdExePath, "%s%s", AcrCmdLineToolPath, DEF_ACRO_CMD_EXE_NAME);

	sprintf(AcronisPath, "%s\\%s", progFilesPath, DEF_ACR_FOLDER_NAME);
	app_os_create_directory(AcronisPath);
	sprintf(rcvyDBFilePath, "%s\\%s\\%s", progFilesPath, DEF_ACR_FOLDER_NAME, DEF_RECOVERY_DB_FILE_NAME);


	//sprintf(AcrLatestVersionPath, "%s\\%s\\%s\\%s", progFilesPath, DEF_ADVANTECH_FOLDER_NAME,
	//	DEF_ACR_FOLDER_NAME, DEF_ACR_LATESVERSION_NAME);
	//sprintf(AcrLogPath, "%s%s", progFilesPath, DEF_ACRONIS_LOG_NAME);
	//sprintf(AcrTempLogPath, "%s%s", progFilesPath, DEF_ACRONIS_TEMP_LOG_NAME);
	//sprintf(AdvantchPath, "%s\\%s", progFilesPath, DEF_ADVANTECH_FOLDER_NAME);
	//app_os_create_directory(AdvantchPath);
	//sprintf(AcrPath, "%s\\%s\\%s", progFilesPath, DEF_ADVANTECH_FOLDER_NAME, DEF_ACR_FOLDER_NAME);
	//app_os_create_directory(AcrPath);

#else

	sprintf(BackupInfoFilePath, "%s",  DEF_BACKUP_INFO_FILE_NAME);
	sprintf(BackupInfoCopyFilePath, "%s", DEF_BACKUP_INFO_COPY_FILE_NAME);
	//sprintf(AcrLogPath, "%s%s", "/usr/lib", DEF_ACRONIS_LOG_NAME);
	//sprintf(AcrTempLogPath, "%s%s", "/usr/lib", DEF_ACRONIS_TEMP_LOG_NAME);
	sprintf(AcrHistoryFilePath, "%s%s", "/usr/lib", DEF_ACRONIS_HISTORY_FILE_NAME);
	sprintf(AcrCmdLineToolPath, "%s%s", "/usr/lib", DEF_ACRONIS_COMMANDLINE_TOOL_NAME);
	sprintf(AcroCmdExePath, "%s%s", AcrCmdLineToolPath, DEF_ACRO_CMD_EXE_NAME);
	sprintf(AcrPath, "%s/%s", progFilesPath, DEF_ACR_FOLDER_NAME);
	app_os_create_directory(AcrPath);
	sprintf(AcronisPath, "%s", "/usr/lib/Acronis");
	app_os_create_directory(AcronisPath);
	sprintf(rcvyDBFilePath, "%s/%s", AcronisPath, DEF_RECOVERY_DB_FILE_NAME);
	sprintf(BkpVolumeArgFilePath, "%s%s", progFilesPath, DEF_BACKUP_VOLUME_ARG_LATEST_RECORD);

	{
		char backupFolderPath[BUFSIZ] = {0}, buf[BUFSIZ] = {0};
		sprintf(backupFolderPath, "%s%s", progFilesPath, DEF_BACKUPDAT_LOCATION_CONF);
		if (app_os_is_file_exist(backupFolderPath))
		{
			FILE *fd = fopen(backupFolderPath, "r");
			if (fd)
			{
				while (fgets(buf, sizeof(buf)-1, fd))
				{
					if (strstr(buf, "AcronisBackupLocation:"))
					{
						strncpy(backupFolder, trimwhitespace(strchr(buf, ':') + 1), sizeof(backupFolder));
						break;
					}
				}
				fclose(fd);
			}
		}
		if (!(strlen(backupFolder) > 0))
		{
			strcpy(backupFolder, DEF_BACKUPDAT_LOCATION_DEFAULT);
			FILE *fd = fopen(backupFolderPath, "w");
			if (fd)
			{
				fprintf(fd, "AcronisBackupLocation: %s", backupFolder);
				fclose(fd);
			}
		}
		Recovery_debug_print("backupFolder: %s", backupFolder);
	}
#endif /* _is_linux */
}

static void InitBatPath()
{
	char progFilesPath[MAX_PATH] = {0};		
	if(!app_os_GetDefProgramFilesPath(progFilesPath)) return;

	sprintf(BackupBatPath, "%s%s", progFilesPath, DEF_BACKUP_BAT_NAME);
	sprintf(RestoreBatPath, "%s%s", progFilesPath, DEF_RESTORE_BAT_NAME);
#ifndef _is_linux
	sprintf(ActivateHotkeyProPath, "%s%s", progFilesPath, DEF_ACTIVATE_ASRM_BAT_NAME);
	sprintf(DeactivateHotkeyProPath, "%s%s", progFilesPath, DEF_DEACTIVATE_ASRM_BAT_NAME);
#endif /*_is_linux*/
}

static BOOL InitRcvyDB()
{
	BOOL bRet = FALSE;
	if(SysRcvyDB == NULL)
	{
		int iRet;
		char * errmsg = NULL;
		char createTableStr[256] = {0};
		iRet = sqlite3_open(rcvyDBFilePath, &SysRcvyDB);
		if(iRet != SQLITE_OK) goto done1;

		sprintf(createTableStr, DEF_CREATE_RCVY_TABLE_SQL_FORMAT_STR, DEF_RCVY_TABLE_NAME);
		iRet = sqlite3_exec(SysRcvyDB, createTableStr, NULL, NULL, &errmsg);
		if(iRet != SQLITE_OK)
		{
			sqlite3_free(errmsg);
			goto done2;
		}

#ifndef _is_linux
		IsRcvyDBCheckThreadRunning = TRUE;
		if (app_os_thread_create(&RcvyDBCheckThreadHandle, RcvyDBCheckThreadStart, NULL) != 0)
		{
			RecoveryLog(g_loghandle, Normal, "%s()[%d]###Start recovery database check thread failed!\n",__FUNCTION__, __LINE__ );
			IsRcvyDBCheckThreadRunning = FALSE;
			goto done2;
		}
#endif /* _is_linux */

		bRet = TRUE;
done2:
		if(!bRet)
		{
			sqlite3_close(SysRcvyDB);
			SysRcvyDB = NULL;
		}
	}
done1:
	return bRet;
}

static BOOL UninitRcvyDB()
{
	BOOL bRet = TRUE;

	if(IsRcvyDBCheckThreadRunning)
	{
		IsRcvyDBCheckThreadRunning = FALSE;
		app_os_thread_join(RcvyDBCheckThreadHandle);
		RcvyDBCheckThreadHandle = NULL;
	}

	if(RcvyDBChangeHandle)
	{
		app_os_FindCloseChangeNotification(RcvyDBChangeHandle);
		RcvyDBChangeHandle = INVALID_HANDLE_VALUE;
	}

	if(SysRcvyDB)
	{
		sqlite3_close(SysRcvyDB);
		SysRcvyDB = NULL;
	}

	return bRet;
}

static int GetResultFromProcess(void* prcHandle)
{
#ifndef _is_linux
	DWORD prcExitCode = 0;
	WaitForSingleObject(prcHandle, INFINITE);
	GetExitCodeProcess(prcHandle, &prcExitCode);
	CloseHandle(prcHandle);
	return (int)prcExitCode;
#else
	int prcExitCode = 0;
	waitpid(prcHandle, &prcExitCode, 0);
	return WEXITSTATUS(prcExitCode);
#endif /* _is_linux */
}

static CAGENT_PTHREAD_ENTRY(AcrReadyCheckThreadStart, args)
{
	CAGENT_HANDLE prcHandle = NULL;
#ifndef _is_linux
	char cmdLine[] = "cmd.exe /c acrocmd list disks";
#else
	char cmdLine[] = "acrocmd list disks";
#endif /* _is_linux */

	RecoveryLog(g_loghandle, Normal, "Acronis service ready check Start!");

	while (IsAcrReadyCheckThreadRunning) {
		//check
		prcHandle = app_os_CreateProcessWithCmdLineEx(cmdLine);
		if (0 == GetResultFromProcess(prcHandle)) // is ready !! 
			break;

		//sleep 1s
		app_os_sleep(1000);
	}	
	app_os_mutex_lock(&Mutex_AcrReady);
	IsAcrReady = TRUE;
	app_os_mutex_unlock(&Mutex_AcrReady);
	RecoveryLog(g_loghandle, Normal, "Acronis service ready check Finish!");
	// update status
	GetRcvyCurStatus();
	return;
}

static void AcronisReadyCheckStart()
{
	CAGENT_THREAD_TYPE AcrReadyCheckThreadHandle;
	
	// exit when Acronis is not installed
	if (!app_os_is_file_exist(AcroCmdExePath))
		return;

	IsAcrReadyCheckThreadRunning = TRUE;
	if (app_os_thread_create(&AcrReadyCheckThreadHandle, AcrReadyCheckThreadStart, NULL) != 0)
	{
		RecoveryLog(g_loghandle, Normal, "%s()[%d]###Start Acronis service ready check thread failed!\n",__FUNCTION__, __LINE__ );
		IsAcrReadyCheckThreadRunning = FALSE;
	}
	else
	{
		/* thread detach */
#ifndef _is_linux
		app_os_CloseHandle(AcrReadyCheckThreadHandle);
		//CloseHandle(&AcrReadyCheckThreadHandle);
#else
		pthread_detach(AcrReadyCheckThreadHandle);
#endif /* _is_linux */
	}
}

static void AcronisReadyCheckStop()
{
	if (IsAcrReadyCheckThreadRunning)
		IsAcrReadyCheckThreadRunning = FALSE;
}

/* **************************************************************************************
 *  Function Name: Handler_Initialize
 *  Description: Init any objects or variables of this handler
 *  Input :  PLUGIN_INFO *pluginfo
 *  Output: None
 *  Return:  handler_success  : Success Init Handler
 *           handler_fail : Fail Init Handler
 * ***************************************************************************************/
int HANDLER_API Handler_Initialize( PLUGIN_INFO *pluginfo )
{
	handler_result hRet = handler_fail;
	Recovery_debug_print("Recovery> Initialize");
	if(pluginfo == NULL)
	{
		return hRet;
	}

	if(g_bEnableLog)
	{
		g_loghandle = pluginfo->loghandle;
	}

	// 1. Topic of this handler
	snprintf( pluginfo->Name, sizeof(pluginfo->Name), "%s", MyTopic );
	pluginfo->RequestID = RequestID;
	pluginfo->ActionID = ActionID;
	printf(" >Name: %s\n", MyTopic);
	// 2. Copy agent info 
	memcpy(&g_PluginInfo, pluginfo, sizeof(PLUGIN_INFO));
	g_PluginInfo.agentInfo = pluginfo->agentInfo;
	g_sendcbf = g_PluginInfo.sendcbf = pluginfo->sendcbf;
	g_sendcustcbf = g_PluginInfo.sendcustcbf = pluginfo->sendcustcbf;
	g_sendreportcbf = g_PluginInfo.sendreportcbf = pluginfo->sendreportcbf;
	strcpy(SERVER_IP, g_PluginInfo.ServerIP);

	InitRecoveryPath();
	InitBatPath();
#ifdef _is_linux
	InitShellScript();
#endif /*_is_linux*/

	app_os_mutex_setup(&CSWMsg);
	app_os_mutex_setup(&CSAMsg);
	app_os_mutex_setup(&Mutex_AcrReady);
	memset(&RecoveryHandlerContext, 0, sizeof(recovery_handler_context_t));
	RecoveryHandlerContext.susiHandlerContext.cagentHandle = pluginfo;
	InitRcvyDB();
	hRet = handler_success;

	Recovery_debug_print("Recovery> Initialize");
	return hRet;
}

/* **************************************************************************************
 *  Function Name: Handler_Get_Status
 *  Description: Get Handler Threads Status. CAgent will restart current Handler or restart CAgent self if busy.
 *  Input :  None
 *  Output: char * : pOutStatus       // cagent handler status
 *  Return:  handler_success  : Success Init Handler
 *			 handler_fail : Fail Init Handler
 * **************************************************************************************/
int HANDLER_API Handler_Get_Status( HANDLER_THREAD_STATUS * pOutStatus )
{
	return handler_success;
}

/* **************************************************************************************
 *  Function Name: Handler_OnStatusChange
 *  Description: CAgent status change notify.
 *  Input :  PLUGIN_INFO *pluginfo
 *  Output: None
 *  Return:  None
 * ***************************************************************************************/
void HANDLER_API Handler_OnStatusChange( HANDLER_INFO *pluginfo )
{
	printf(" %s> Update Status", MyTopic);
	if(pluginfo)
	{
		memcpy(&g_PluginInfo, pluginfo, sizeof(HANDLER_INFO));
		/*if(g_PluginInfo.agentInfo->status == AGENT_STATUS_ONLINE)
		{
			PowerGetAMTParams();
		}*/
	}
	else
	{
		memset(&g_PluginInfo, 0, sizeof(HANDLER_INFO));
		snprintf( g_PluginInfo.Name, sizeof( g_PluginInfo.Name), "%s", MyTopic );
		g_PluginInfo.RequestID = RequestID;
		g_PluginInfo.ActionID = ActionID;
	}
	if(pluginfo->agentInfo->status == AGENT_STATUS_ONLINE)
	{
		GetRcvyCurStatus();
	}
}

/* **************************************************************************************
 *  Function Name: Handler_Start
 *  Description: Start Running
 *  Input :  None
 *  Output: None
 *  Return:  handler_success : Success to Start Handler
 *           handler_fail : Fail to Start Handler
 * ***************************************************************************************/
int HANDLER_API Handler_Start( void )
{
	Recovery_debug_print("Recovery> Start");
	AcronisReadyCheckStart();
	if(g_PluginInfo.agentInfo->status == AGENT_STATUS_ONLINE)
	{
		GetRcvyCurStatus();
	}
	return handler_success;
}

/* **************************************************************************************
 *  Function Name: Handler_Stop
 *  Description: Stop the handler
 *  Input :  None
 *  Output: None
 *  Return:  handler_success : Success to Stop
 *           handler_fail: Fail to Stop handler
 * ***************************************************************************************/
int HANDLER_API Handler_Stop( void )
{
	recovery_handler_context_t *pRecoveryHandlerContext =  (recovery_handler_context_t *)&RecoveryHandlerContext;
	if(pRecoveryHandlerContext->acrBackupPercentCheckThreadHandle != NULL && pRecoveryHandlerContext->isAcrPercentCheckRunning)
	{
		pRecoveryHandlerContext->isAcrPercentCheckRunning = FALSE;
		app_os_thread_join(pRecoveryHandlerContext->acrBackupPercentCheckThreadHandle);
		pRecoveryHandlerContext->acrBackupPercentCheckThreadHandle = NULL;
	}
	app_os_mutex_cleanup(&CSWMsg);
	app_os_mutex_cleanup(&CSAMsg);
	app_os_mutex_cleanup(&Mutex_AcrReady);
	AcronisReadyCheckStop();
	UninitRcvyDB();

	Recovery_debug_print("Recovery> Stop");
	return handler_success;
}

/* **************************************************************************************
 *  Function Name: Handler_Recv
 *  Description: Receive Packet from MQTT Server
 *  Input : char * const topic, 
 *			void* const data, 
 *			const size_t datalen
 *  Output: void *pRev1, 
 *			void* pRev2
 *  Return: None
 * ***************************************************************************************/
void HANDLER_API Handler_Recv( char * const topic, void* const data, const size_t datalen, void *pRev1, void* pRev2  )
{
	char rcvyMsg[512] = {0};
	int commCmd = unknown_cmd;
	susi_comm_data_t *pSusiCommData = NULL;
	char errorStr[128] = {0};
	if(!data || datalen <= 0) return;

	if(!ParseReceivedData(data, datalen, &commCmd))
	{
		Recovery_debug_print("ParseReceivedData error!");
		return;
	}

	Recovery_debug_print("=================commCmd=%d===================", commCmd);
	switch(commCmd)
	{
	case rcvy_backup_req:
		{
			if(IsActive())
			{
				DoBackup();
			}
			else
			{
				if(IsExpired())
				{
					memset(rcvyMsg, 0, sizeof(rcvyMsg));
					sprintf(rcvyMsg, "%s,%s", "ActivateInfo", "Expired");
					SendReplyMsg_Rcvy(rcvyMsg, bkp_rstr_expire, rcvy_backup_rep);
					DeactivateARSM();
				}
				else
				{
					DoBackup();
				}
			}
		}
		break;

	case rcvy_restore_req:
		{
			if(IsActive())
			{
				DoRestore();
			}
			else
			{
				if(IsExpired())
				{
					memset(rcvyMsg, 0, sizeof(rcvyMsg));
					sprintf(rcvyMsg, "%s,%s", "ActivateInfo", "Expired");
					SendReplyMsg_Rcvy(rcvyMsg, bkp_rstr_expire, rcvy_restore_rep);
					DeactivateARSM();
				}
				else
				{
					DoRestore();
				}
			}
		}
		break;

	case rcvy_active_req:
		{
#ifndef _is_linux		
			Activate();
			if(IsActive())
			{
				memset(rcvyMsg, 0, sizeof(rcvyMsg));
				sprintf(rcvyMsg, "%s,%s", "ActivateInfo", "OK");
				SendReplySuccessMsg_Rcvy(rcvyMsg, rcvy_active_rep);
				RunASRM(TRUE);
			}
			else
			{
				memset(rcvyMsg, 0, sizeof(rcvyMsg));
				sprintf(rcvyMsg, "%s,%s", "ActivateInfo", "Fail");
				SendReplyFailMsg_Rcvy(rcvyMsg, rcvy_active_rep);
			}
#else
			memset(rcvyMsg, 0, sizeof(rcvyMsg));
			sprintf(rcvyMsg, "%s,%s,%s", "ActivateInfo", "Fail", "Not support the remote activation!");
			SendReplyFailMsg_Rcvy(rcvyMsg, rcvy_active_rep);
#endif /* _is_linux */
		}
		break;

	case rcvy_install_req:
		{
#ifndef _is_linux
		
			LONGLONG  minS = 0, maxS = 0;
			app_os_GetMinAndMaxSpaceMB(&minS, &maxS);
			if(maxS <= minS + 900)
			{
				memset(rcvyMsg, 0, sizeof(rcvyMsg));
				sprintf(rcvyMsg, "%s", "ASZ size not enough!");
				SendReplyMsg_Rcvy(rcvyMsg, bkp_sizeLack, rcvy_install_rep);
			}
			else
			{
				recovery_install_params recoveryVal;
				int dataLen = 0;
				IsAcrInstallThenActive = FALSE;
				dataLen = sizeof(susi_comm_data_t) + sizeof(recovery_install_params);
				pSusiCommData = (susi_comm_data_t *)malloc(dataLen);
				memset(pSusiCommData, 0, dataLen);
				if(parse_rcvy_install_req((char *)data, &recoveryVal))
				{
					recovery_install_params * pInstallParams = &recoveryVal;
					if(pInstallParams->ASZPersent > 100)
					{
						memset(errorStr, 0, sizeof(errorStr));
						sprintf(errorStr, "Command(%d) params error!", rcvy_install_req);
						SendReplyMsg_Rcvy(errorStr, istl_argErr, rcvy_error_rep);
					}
					else
					{
						WriteDefaultReg();
						if(pInstallParams->ASZPersent >= 0)
						{
							WriteASZPercentRegistry(pInstallParams->ASZPersent);
						}
						CurInstallType = ITT_UNKNOWN;
						pInstallParams->installType = ITT_INSTALL;
						IsAcrInstallThenActive = pInstallParams->isThenActive;
						InstallAction(pInstallParams);
					}
				}
				else
				{
					memset(errorStr, 0, sizeof(errorStr));
					sprintf(errorStr, "Command(%d) parse error!", rcvy_install_req);
					SendReplyMsg_Rcvy(errorStr, istl_parseErr, rcvy_error_rep);
				}
				if(pSusiCommData) free(pSusiCommData);
			}
#else
			memset(rcvyMsg, 0, sizeof(rcvyMsg));
			sprintf(rcvyMsg, "%s,%s,%s", "InstallInfo", "Fail", "Not support the remote installation!");
			SendReplyFailMsg_Rcvy(rcvyMsg, rcvy_install_rep);
#endif /* _is_linux */
		}
		break;

	case rcvy_update_req:
		{		
#ifndef _is_linux
			recovery_install_params recoveryVal;
			//UpdateAction(pInstallParams);
			int dataLen = 0;
			IsAcrInstallThenActive = FALSE;
			dataLen = sizeof(susi_comm_data_t) + sizeof(recovery_install_params);
			pSusiCommData = (susi_comm_data_t *)malloc(dataLen);
			memset(pSusiCommData, 0, dataLen);
			if(parse_rcvy_update_req((char *)data, &recoveryVal))
			{
				recovery_install_params * pInstallParams = &recoveryVal;
				if(pInstallParams->ASZPersent > 100)
				{
					memset(errorStr, 0, sizeof(errorStr));
					sprintf(errorStr, "Command(%d) params error!", rcvy_update_req);
					SendReplyMsg_Rcvy(errorStr, updt_argErr, rcvy_error_rep);
				}
				else
				{
					IsAcrInstallThenActive = pInstallParams->isThenActive;
					pInstallParams->installType = ITT_UPDATE;
					UpdateAction(pInstallParams);
				}
			}
			else
			{
				memset(errorStr, 0, sizeof(errorStr));
				sprintf(errorStr, "Command(%d) parse error!", rcvy_update_req);
				SendReplyMsg_Rcvy(errorStr, updt_parseErr, rcvy_error_rep);
			}
			if(pSusiCommData) free(pSusiCommData);
#endif /* _is_linux */
		}
		break;

	case rcvy_cancel_install_req:
		break;

	case rcvy_reinstall_req:
		break;

	case rcvy_create_asz_req:
		{
#ifndef _is_linux
			char tempVal[BUFSIZ] = {0};
			char ASZParams[BUFSIZ] = {0};
			BOOL isCreate = FALSE;
			int dataLen = sizeof(susi_comm_data_t) + sizeof(ASZParams);
			susi_comm_data_t * pSusiCommData = (susi_comm_data_t *)malloc(dataLen);
			memset(pSusiCommData, 0, dataLen);
			if(parse_rcvy_create_asz_req((char *)data, tempVal))
			{
				strcpy(ASZParams, tempVal);
				if(ASZCreateParamCheck(ASZParams))
				{
					isCreate = TRUE;
				}
				else
				{
					memset(errorStr, 0, sizeof(errorStr));
					sprintf(errorStr, "Command(%d) params error!", rcvy_create_asz_req);
					SendReplyMsg_Rcvy(errorStr, asz_argErr, rcvy_error_rep);
				}	
			}
			else
			{
				memset(errorStr, 0, sizeof(errorStr));
				sprintf(errorStr, "Command(%d) parse error!", rcvy_create_asz_req);
				SendReplyMsg_Rcvy(errorStr, asz_parseErr, rcvy_error_rep);
			}
			if(pSusiCommData) free(pSusiCommData);
			if(isCreate) CreateASZ(ASZParams);		
#endif /* _is_linux */
		}
		break;

	case rcvy_status_req:
		{
			GetRcvyCurStatus();
		}
		break;

	case rcvy_capability_req:
		{
			char * pSendVal = NULL;
			int jsonStrlen = GetRcvyCapability(rcvy_capability_cmd_way, &pSendVal);
			if(jsonStrlen > 0 && pSendVal != NULL)
			{
				g_sendcbf(&g_PluginInfo, rcvy_capability_rep, pSendVal, jsonStrlen, NULL, NULL);
			}
		}
		break;
	//case rcvy_return_status_req:
	//	{
	//		SendAgentStartMessage(TRUE);
	//	}
	//	break;

	//case rcvy_return_status_without_call_hotkey_req:
	//	{
	//		SendAgentStartMessage(FALSE);
	//	}		
	//	break;

	default:
		{
			char * errorStr = "Unknown cmd!";
			SendReplyMsg_Rcvy(errorStr, oprt_unkown, rcvy_error_rep);
		}
		break;
	}

	return;
}

/* **************************************************************************************
 *  Function Name: Handler_Get_Capability
 *  Description: Get Handler Information specification.
 *  Input :  None
 *  Output: char ** : pOutReply       // JSON Format
 *  Return:  int  : Length of the status information in JSON format
 *                :  0 : no data need to trans
 * **************************************************************************************/
int HANDLER_API Handler_Get_Capability( char ** pOutReply )// JSON Format
{
	return GetRcvyCapability(rcvy_capability_api_way, pOutReply);
}

/* **************************************************************************************
 *  Function Name: Handler_AutoReportStart
 *  Description: Start Auto Report
 *  Input : char *pInQuery
 *  Output: None
 *  Return: None
 * ***************************************************************************************/
void HANDLER_API Handler_AutoReportStart(char *pInQuery)
{
	return;
}

/* **************************************************************************************
 *  Function Name: Handler_AutoReportStop
 *  Description: Stop Auto Report
 *  Input : char *pInQuery, if *pInQuery = NULL, then stop all upload message.
 *  Output: None
 *  Return: None
 * ***************************************************************************************/
void HANDLER_API Handler_AutoReportStop(char *pInQuery)
{
	return;
}

/* **************************************************************************************
 *  Function Name: Handler_MemoryFree
 *  Description: free the mamory allocated for Handler_Get_Capability
 *  Input : char *pInData.
 *  Output: None
 *  Return: None
 * ***************************************************************************************/
void HANDLER_API Handler_MemoryFree(char *pInData)
{
	if(pInData)
	{
		free(pInData);
		pInData = NULL;
	}
}
