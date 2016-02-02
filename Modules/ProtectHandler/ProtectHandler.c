/****************************************************************************/
/* Copyright(C) : Advantech Technologies, Inc.		    			        */
/* Create Date  : 2014/11/03 by hailong.dang								*/
/* Modified Date: 2015/04/09 by guolin.huang								*/
/* Abstract     : Handler API                                     			*/
/* Reference    : None														*/
/****************************************************************************/

#include "ProtectHandler.h"
#include "parser_prtt.h"
#include "public_prtt.h"
#include "activate_prtt.h"
#include "protect_prtt.h"
#include "unprotect_prtt.h"
#include "install_update_prtt.h"
#include "status_prtt.h"
#include "capability_prtt.h"

//-----------------------------------------------------------------------------
// Global variables define:
//-----------------------------------------------------------------------------
const char strPluginName[MAX_TOPIC_LEN] = {"protection"};
const int iRequestID = cagent_request_protection;
const int iActionID = cagent_reply_protection;
Handler_info		g_PluginInfo;
handler_context_t	g_HandlerContex;
void*	g_loghandle = NULL;
bool	g_bEnableLog = true;
HandlerSendCbf			g_sendcbf = NULL;			// Client Send information (in JSON format) to Cloud Server	
HandlerSendCustCbf		g_sendcustcbf = NULL;		// Client Send information (in JSON format) to Cloud Server with custom topic	
HandlerAutoReportCbf	g_sendreportcbf = NULL;		// Client Send report (in JSON format) to Cloud Server with AutoReport topic	
HandlerSendCapabilityCbf g_sendcapabilitycbf = NULL;	
HandlerSubscribeCustCbf g_subscribecustcbf = NULL;

char EncodePassWord[DEF_MD5_SIZE] = {0};
char InstallPath[MAX_PATH] = {0};
char LastWarningMsg[256] = {0};
char ActionMsg[256] = {0};

#ifdef _WIN32
char McAfeeInstallerPath[MAX_PATH] = {0};
#else
BOOL McAfeeStatus = FALSE;
#endif /*_WIN32*/


//-----------------------------------------------------------------------------
// Local macros define:
//-----------------------------------------------------------------------------
#define DEF_MCAFEE_FOLDER_NAME                   "McAfee"
#define DEF_ADVANTECH_FOLDER_NAME                "Advantech"
#define DEF_SYS_LOG_NAME                         "Application"
#define DEF_LOG_FORM_APP_NAME                    L"McAfee Solidifier"

#ifdef _WIN32
#define DEF_SADMIN_OUTPUT_TMP_FILE_NAME          "\\McAfeeStatus.txt"
#define DEF_XP_MCAFEE_LOG_DIR                    "C:\\Documents and Settings\\All Users\\Application Data\\McAfee\\Solidcore\\Logs"
#define DEF_XP_MCAFEE_LOG_FILE                   "C:\\Documents and Settings\\All Users\\Application Data\\McAfee\\Solidcore\\Logs\\solidcore.log"
#define DEF_XP_MCAFEE_LOG_FILE1                  "C:\\Documents and Settings\\All Users\\Application Data\\McAfee\\Solidcore\\Logs\\solidcore.log.1"
#define DEF_VISTA_MCAFEE_LOG_DIR                 "C:\\ProgramData\\McAfee\\Solidcore\\Logs"
#define DEF_VISTA_MCAFEE_LOG_FILE                "C:\\ProgramData\\McAfee\\Solidcore\\Logs\\solidcore.log"
#define DEF_VISTA_MCAFEE_LOG_FILE1               "C:\\ProgramData\\McAfee\\Solidcore\\Logs\\solidcore_1.log"
//#define DEF_VISTA_MCAFEE_LOG_FILE1               "C:\\ProgramData\\McAfee\\Solidcore\\Logs\\solidcore.log.1"
#define DEF_MCAFEE_INSTALLER_NAME                "McAfeeInstaller.exe"
#define DEF_MCAFEE_LATESVERSION_NAME             "LatestVersion.txt"
#define DEF_MCAFEE_LOG_TMP                       "\\tmp.txt"

#else

#define DEF_MCAFEE_LOG_TMP                       "/tmp.txt"
#define DEF_SADMIN_OUTPUT_TMP_FILE_NAME          "/McAfeeStatus.txt"
#define DEF_LINUX_MCAFEE_LOG_DIR                 "/var/log/mcafee/solidcore"
#endif /* _WIN32 */

//-----------------------------------------------------------------------------
// Local variables define:
//-----------------------------------------------------------------------------
static char McAfeeLogDir[MAX_PATH] = {0};
static char McAfeeLogFile[MAX_PATH] = {0};
static char McAfeeLogFile1[MAX_PATH] = {0};
static char McAfeeLogFileTmp[MAX_PATH] = {0};
static int  McAfeeLogLastLinelen = 0;

#ifdef _WIN32
//static char McAfeeLatestVersionPath[MAX_PATH] = {0};
//static char AdvantchPath[MAX_PATH] = {0};
static char McAfeePath[MAX_PATH] = {0};
static EINSTALLTYPE CurInstallType = ITT_UNKNOWN;
#endif /* _WIN32 */


//-----------------------------------------------------------------------------
// Local function declare:
//-----------------------------------------------------------------------------
static BOOL InitMcAfeePath();
static void InitLogPath();
static BOOL McAfeeLogCheckStart();
static BOOL McAfeeLogCheckStop();
static BOOL SadminOutputCheckStart();
static BOOL SadminOutputCheckStop();
static CAGENT_PTHREAD_ENTRY(SadminOutputCheckThreadStart, args);
static CAGENT_PTHREAD_ENTRY(McAfeeLogCheckThreadStart, args);
static BOOL McAfeeLogAnalyze(char * logFilePath, int LogLastLen, char * outVaildMsg, BOOL *pSkip);
static void GetVaildMsg(int mcafeeLogLen, int mcafeeLogTmpLen, char * outVaildMsg);
static void ProcessMcAfeeLogChange(int mcafeeLogLen, int mcafeeLogTmpLen);
static void UpdateStatus(char * pValidMsg);
static void ParsingMsg(const char * InMsg, char * dateTimeStr, char * msgType, char * Msg);
static void Reboot();

#ifdef _WIN32
static BOOL SysLogCheckStart();
static BOOL SysLogCheckStop();
static DWORD ReadRecord(HANDLE hEventLog, PBYTE ** pBuffer, DWORD dwRecordNumber, DWORD dwFlags);
static DWORD ProcessNewRecords(HANDLE hEventLog);
static CAGENT_PTHREAD_ENTRY(SysLogCheckThreadStart, args);
static DWORD GetLastRecordNumber(HANDLE hEventLog, DWORD* pRecordNumber);
static DWORD SeekToLastRecord(CAGENT_HANDLE hEventLog);

#else
static int inotify_watch_init(const char *path, int flags, int mask);
static void InitMcAfeeStatus();
#endif /*_WIN32*/


//-----------------------------------------------------------------------------
// Global function define:
//-----------------------------------------------------------------------------
#ifndef _WIN32
char * strcpy_s(char * dest, unsigned int dest_size, const char * src)
{
	return strncpy(dest, src, dest_size);
}
#endif /* _WIN32 */

int GetResultFromProcess(void* prcHandle)
{
#ifdef _WIN32
	DWORD prcExitCode = 0;
	WaitForSingleObject(prcHandle, INFINITE);
	GetExitCodeProcess(prcHandle, &prcExitCode);
	CloseHandle(prcHandle);
	return (int)prcExitCode;
#else
	int prcExitCode = 0;
	waitpid(prcHandle, &prcExitCode, 0);
	return WEXITSTATUS(prcExitCode);
#endif /* _WIN32 */
}

//-----------------------------------------------------------------------------
// Local function define:
//-----------------------------------------------------------------------------
#ifndef _WIN32
static int inotify_watch_init(const char *path, int flags, int mask)
{
	int fd = inotify_init1(flags);
	if (fd < 0)
	{
		perror("inotify_init1");
	}
	else
	{
		int wd = inotify_add_watch(fd, path, mask);
		if (wd < 0)
		{
			perror("inotify_add_watch");
			close(fd);
			fd = -1;
		}
	}
	return fd;
}

static void InitMcAfeeStatus()
{
	BOOL reboot = FALSE;
	if (IsInstalled())
	{
		if (IsSolidify())
		{
			reboot = TRUE;// Make 'sadmin so' abort
		}
		else
		{
			FILE *fd = popen("sadmin status ", "r");
			if (fd)
			{
				char buf[1024] = {0};
				fgets(buf, sizeof(buf)-1, fd);
				if (strstr(buf, "Enabled"))
				{
					McAfeeStatus = TRUE;
					fgets(buf, sizeof(buf)-1, fd);
					if (strstr(buf, "Disabled"))
					{
						reboot = TRUE;
					}
				}
				else if (strstr(buf, "Disabled"))
				{
					McAfeeStatus = FALSE;
					fgets(buf, sizeof(buf)-1, fd);
					if (strstr(buf, "Enabled"))
					{
						reboot = TRUE;
					}
				}
				protect_debug_print("McAfeeStatus:%d", McAfeeStatus);
				pclose(fd);
			}
			else
			{
				ProtectLog(g_loghandle, Warning, "%s","Get McAfee status failed!");
			}
		}
		if (reboot)
			Reboot();
	}
}

#else

static DWORD ReadRecord(HANDLE hEventLog, PBYTE ** pBuffer, DWORD dwRecordNumber, DWORD dwFlags)
{
	DWORD dwStatus = ERROR_SUCCESS;
	DWORD dwBytesToRead = sizeof(EVENTLOGRECORD);
	DWORD dwBytesRead = 0;
	DWORD dwMinimumBytesToRead = 0;
	PBYTE pTemp = NULL;

	*pBuffer= (PBYTE)malloc(sizeof(EVENTLOGRECORD));

	if (!app_os_ReadEventLog(hEventLog, dwFlags, dwRecordNumber, *pBuffer, dwBytesToRead, &dwBytesRead, &dwMinimumBytesToRead))
	{
		dwStatus = app_os_get_last_error();
		if (ERROR_INSUFFICIENT_BUFFER == dwStatus)
		{
			dwStatus = ERROR_SUCCESS;

			pTemp = (PBYTE)realloc(*pBuffer, dwMinimumBytesToRead);
			if (NULL == pTemp)
			{
				ProtectLog(g_loghandle, Warning, " %s> Failed to reallocate memory for the record buffer (%d bytes)", strPluginName, dwMinimumBytesToRead);
				goto done;
			}

			*pBuffer = pTemp;

			dwBytesToRead = dwMinimumBytesToRead;

			if (!app_os_ReadEventLog(hEventLog, dwFlags, dwRecordNumber, *pBuffer, dwBytesToRead, &dwBytesRead, &dwMinimumBytesToRead))
			{
				ProtectLog(g_loghandle, Warning, " %s> Second ReadEventLog failed!", strPluginName);
				goto done;
			}
		}
		else
		{
			if (ERROR_HANDLE_EOF != dwStatus)
			{
				ProtectLog(g_loghandle, Warning, " %s> ReadEventLog failed with %d ", strPluginName, dwStatus);
				goto done;
			}
		}
	}

done:
	return dwStatus;
}

static DWORD ProcessNewRecords(HANDLE hEventLog)
{
	DWORD dwStatus = ERROR_SUCCESS;
	DWORD dwLastRecordNumber = 0;
	LPWSTR pMessage = NULL;
	LPWSTR pFinalMessage = NULL;
	PBYTE pRecord = NULL;

	dwStatus = ReadRecord(hEventLog, (PBYTE**)&pRecord, 0, EVENTLOG_SEQUENTIAL_READ | EVENTLOG_FORWARDS_READ);
	if (ERROR_SUCCESS != dwStatus && ERROR_HANDLE_EOF != dwStatus)
	{
		ProtectLog(g_loghandle, Warning, " %s> ReadRecord (priming read) failed!", strPluginName);
		goto done;
	}

	while (ERROR_HANDLE_EOF != dwStatus)
	{
		if (0 == wcscmp(DEF_LOG_FORM_APP_NAME, (LPWSTR)(pRecord + sizeof(EVENTLOGRECORD))))
		{
			DWORD dwEventID = (((PEVENTLOGRECORD)pRecord)->EventID & 0xFFFF);
			if(dwEventID == 11)
			{
				UpdateStatus("");
			}
		}

		if (pRecord)
		{
			free(pRecord);
			pRecord = NULL;
		}

		dwStatus = ReadRecord(hEventLog, (PBYTE**)&pRecord, 0, EVENTLOG_SEQUENTIAL_READ | EVENTLOG_FORWARDS_READ);
		if (ERROR_SUCCESS != dwStatus && ERROR_HANDLE_EOF != dwStatus)
		{
			ProtectLog(g_loghandle, Warning, " %s> ReadRecord sequential failed!", strPluginName);
			goto done;
		}
	}

	if (ERROR_HANDLE_EOF == dwStatus)
	{
		dwStatus = ERROR_SUCCESS;
	}

done:
	if (pRecord) free(pRecord);
	return dwStatus;
}

static DWORD GetLastRecordNumber(HANDLE hEventLog, DWORD* pRecordNumber)
{
	DWORD dwStatus = ERROR_SUCCESS;
	DWORD OldestRecordNumber = 0;
	DWORD NumberOfRecords = 0;

	if (!app_os_GetOldestEventLogRecord(hEventLog, &OldestRecordNumber))
	{
		ProtectLog(g_loghandle, Warning, " %s> GetOldestEventLogRecord failed with %d!", strPluginName, dwStatus = app_os_get_last_error());
		goto done;
	}

	if (!app_os_GetNumberOfEventLogRecords(hEventLog, &NumberOfRecords))
	{
		ProtectLog(g_loghandle, Warning, " %s> GetNumberOfEventLogRecords failed with %d!", strPluginName, dwStatus = app_os_get_last_error());
		goto done;
	}

	*pRecordNumber = OldestRecordNumber + NumberOfRecords - 1;

done:
	return dwStatus;
}

static DWORD SeekToLastRecord(CAGENT_HANDLE hEventLog)
{
	DWORD dwStatus = ERROR_SUCCESS;
	DWORD dwLastRecordNumber = 0;
	PBYTE pRecord = NULL;

	dwStatus = GetLastRecordNumber(hEventLog, &dwLastRecordNumber);
	if (ERROR_SUCCESS != dwStatus)
	{
		ProtectLog(g_loghandle, Warning, " %s> GetLastRecordNumber failed!", strPluginName);
		goto done;
	}

	dwStatus = ReadRecord(hEventLog, (PBYTE **)&pRecord, dwLastRecordNumber, EVENTLOG_SEEK_READ | EVENTLOG_FORWARDS_READ);
	if (ERROR_SUCCESS != dwStatus)
	{
		ProtectLog(g_loghandle, Warning, " %s> ReadRecord failed seeking to record %d!", strPluginName, dwLastRecordNumber);
		goto done;
	}

done:
	if (pRecord) free(pRecord);
	return dwStatus;
}

static CAGENT_PTHREAD_ENTRY(SysLogCheckThreadStart, args)
{
	handler_context_t *pHandlerContext = (handler_context_t *)args;
	DWORD dwRet = 0;
	DWORD dwStatus = ERROR_SUCCESS;
	while (pHandlerContext->isSysLogCheckThreadRunning)
	{
		dwRet = app_os_WaitForSingleObject(pHandlerContext->logCheckEventHandle, 3000);
		if(dwRet == WAIT_OBJECT_0)
		{
			ResetEvent(pHandlerContext->logCheckEventHandle);
			if(!pHandlerContext->isSysLogCheckThreadRunning) break;
			if (ERROR_SUCCESS != (dwStatus = ProcessNewRecords(pHandlerContext->sysLogHandle)))
			{
				ProtectLog(g_loghandle, Warning, " %s> DumpNewRecords failed!", strPluginName);
			}
		}
		app_os_sleep(10);
	}
	return 0;
}

static BOOL SysLogCheckStart()
{
	BOOL bRet = FALSE;
	DWORD dwStatus = ERROR_SUCCESS;

	g_HandlerContex.logCheckEventHandle = app_os_create_event(TRUE, FALSE, "logCheckEvent");
	if(NULL == g_HandlerContex.logCheckEventHandle)
	{
		ProtectLog(g_loghandle, Warning, " %s> Create log check event failed!", strPluginName);
		goto done;
	}

	g_HandlerContex.sysLogHandle = app_os_open_event_log(DEF_SYS_LOG_NAME);
	if(NULL == g_HandlerContex.sysLogHandle)
	{
		ProtectLog(g_loghandle, Warning, " %s> Open event log %s failed!", strPluginName, DEF_SYS_LOG_NAME);
		goto done;
	}

	dwStatus = SeekToLastRecord(g_HandlerContex.sysLogHandle);
	if (ERROR_SUCCESS != dwStatus)
	{
		ProtectLog(g_loghandle, Warning, " %s> SeekToLastRecord failed with %d", strPluginName, dwStatus);
		goto done;
	}

	if(!NotifyChangeEventLog(g_HandlerContex.sysLogHandle, g_HandlerContex.logCheckEventHandle))
	{
		ProtectLog(g_loghandle, Warning, " %s> NotifyChangeEventLog failed!", strPluginName);
		goto done;
	}

	g_HandlerContex.isSysLogCheckThreadRunning = TRUE;
	if (app_os_thread_create(&g_HandlerContex.sysLogCheckThreadHandle, SysLogCheckThreadStart, &g_HandlerContex) != 0)
	{
		ProtectLog(g_loghandle, Warning, " %s> Start sys log check thread failed!", strPluginName);
		g_HandlerContex.isSysLogCheckThreadRunning = FALSE;
		goto done;
	}

	bRet = TRUE;

done:
	return bRet;
}

static BOOL SysLogCheckStop()
{
	BOOL bRet = TRUE;
	if(g_HandlerContex.isSysLogCheckThreadRunning)
	{
		app_os_set_event(g_HandlerContex.logCheckEventHandle);
		g_HandlerContex.isSysLogCheckThreadRunning = FALSE;
		app_os_thread_join(g_HandlerContex.sysLogCheckThreadHandle);
		g_HandlerContex.sysLogCheckThreadHandle = NULL;
	}

	app_os_CloseHandle(g_HandlerContex.logCheckEventHandle);
	g_HandlerContex.logCheckEventHandle = NULL;

	app_os_CloseEventLog(g_HandlerContex.sysLogHandle);
	g_HandlerContex.sysLogHandle = NULL;

	return bRet;
}
#endif /* _WIN32 */

static BOOL InitMcAfeePath()
{
#ifdef _WIN32
	char tmpProgFilesPath[MAX_PATH] = {0};
	char progFilesPath[MAX_PATH] = {0};
	char * tmpCh = NULL;

	if (!app_os_get_programfiles_path(tmpProgFilesPath)) return FALSE;
	tmpCh = strstr(tmpProgFilesPath, " (x86)");
	if (tmpCh)
	{
		memcpy(progFilesPath, tmpProgFilesPath, tmpCh - tmpProgFilesPath);
	}
	else
	{
		memcpy(progFilesPath, tmpProgFilesPath, strlen(tmpProgFilesPath)+1);
	}
	sprintf(InstallPath, "%s%s", progFilesPath, DEF_MCAFEE_SADMIN_PATH);

	{
		char modulePath[MAX_PATH] = {0};
		char *ptr = NULL;
		if(app_os_get_module_path(modulePath))
		{
			ptr = strstr(modulePath, "Modules");
			if (ptr--)
				*ptr = "\0";// delete sub-string from last slash
			sprintf(McAfeeInstallerPath, "%s\\Downloads\\", modulePath);			
		}
		else 
		{			
			sprintf(McAfeeInstallerPath, "%s\\%s\\%s\\Downloads\\", progFilesPath, DEF_ADVANTECH_FOLDER_NAME, "SUSIAccess 3.1 Agent");
		}
		app_os_create_directory(McAfeeInstallerPath);
		strcat(McAfeeInstallerPath, DEF_MCAFEE_INSTALLER_NAME);
	}
	
	//{
	//	char * winPath = NULL;
	//	winPath = getenv("WINDIR");
	//	if(winPath)
	//		sprintf(McAfeeInstallerPath, "%s\\Temp\\%s\\", winPath, DEF_MCAFEE_FOLDER_NAME);
	//	else
	//		sprintf(McAfeeInstallerPath, "C:\\Windows\\Temp\\%s\\", DEF_MCAFEE_FOLDER_NAME);
	//	app_os_create_directory(McAfeeInstallerPath);
	//	strcat(McAfeeInstallerPath, DEF_MCAFEE_INSTALLER_NAME);
	//}

	//sprintf(AdvantchPath, "%s\\%s", progFilesPath, DEF_ADVANTECH_FOLDER_NAME);
	//app_os_create_directory(AdvantchPath);
	//sprintf(McAfeePath, "%s\\%s\\%s", progFilesPath, DEF_ADVANTECH_FOLDER_NAME, DEF_MCAFEE_FOLDER_NAME);
	//app_os_create_directory(McAfeePath);
	//sprintf(McAfeeInstallerPath, "%s\\%s\\%s\\%s", progFilesPath, DEF_ADVANTECH_FOLDER_NAME,
	//	DEF_MCAFEE_FOLDER_NAME, DEF_MCAFEE_INSTALLER_NAME);
	//sprintf(McAfeeLatestVersionPath, "%s\\%s\\%s\\%s", progFilesPath, DEF_ADVANTECH_FOLDER_NAME,
	//	DEF_MCAFEE_FOLDER_NAME, DEF_MCAFEE_LATESVERSION_NAME);

#else

	sprintf(InstallPath, "%s", DEF_MCAFEE_SADMIN_PATH);
	if (!app_os_is_file_exist(DEF_LINUX_SA_INSTALL_TIME_RECORD) \
		&& app_os_is_file_exist(InstallPath))
	{
		ProtectLog(g_loghandle, Normal, "File '%s' write start!", \
			DEF_LINUX_SA_INSTALL_TIME_RECORD);
		time_t tm_sec = 0;
		char dateInfo[128] = {0}, encodeDatebuf[128] = {0};
		FILE *fd = fopen(DEF_LINUX_SA_INSTALL_TIME_RECORD, "wb");
		if (fd)
		{
			if (-1 != time(&tm_sec))
			{
				int len = 0;
				struct tm now = {0};

				tm_sec += 29 * 24 * 60 * 60;//Add trial days
				localtime_r(&tm_sec, &now);
				sprintf(dateInfo, "%d-%d-%d", now.tm_year+1900, now.tm_mon+1, now.tm_mday);
				protect_debug_print("dateInfo: %s", dateInfo);

				DES_BASE64Encode(dateInfo, encodeDatebuf);
				len = strlen(encodeDatebuf);
				if ((len > 0) && (len == fprintf(fd, "%s", encodeDatebuf)))
				{
					ProtectLog(g_loghandle, Normal, "File '%s' write success!", \
						DEF_LINUX_SA_INSTALL_TIME_RECORD);
				}
			}
			else
			{
				ProtectLog(g_loghandle, Error, "Get current time failed!");
			}
			fclose(fd);
		}
		else
		{
			ProtectLog(g_loghandle, Error, "Create file '%s' failed!", \
				DEF_LINUX_SA_INSTALL_TIME_RECORD);
		}
	}
	else
	{
		char encodeDatebuf[128] = {0}, decodeDatebuf[128] = {0};
		FILE *fd = fopen(DEF_LINUX_SA_INSTALL_TIME_RECORD, "rb");
		if (fd)
		{
			int len = 0;
			fgets(encodeDatebuf, sizeof(encodeDatebuf), fd);
			DES_BASE64Decode(encodeDatebuf, decodeDatebuf);
			len = strlen(decodeDatebuf);
			if ((len > 0))
			{
				protect_debug_print("encodeDatebuf:%s, decodeDatebuf:%s", encodeDatebuf, decodeDatebuf);
			}
			fclose(fd);
		}
		else
		{
			protect_debug_print("Open failed");
		}
	}
#endif /* _WIN32 */
	return TRUE;
}

static void InitLogPath()
{
#ifdef _WIN32
	char osName[DEF_OS_NAME_LEN] = {0};
	if(!app_os_get_os_name(osName)) return;
	if((strcmp(osName, OS_WINDOWS_XP)==0) || (strcmp(osName, OS_WINDOWS_SERVER_2003)==0))
	{
		memcpy(McAfeeLogDir, DEF_XP_MCAFEE_LOG_DIR, sizeof(DEF_XP_MCAFEE_LOG_DIR));
		memcpy(McAfeeLogFile, DEF_XP_MCAFEE_LOG_FILE, sizeof(DEF_XP_MCAFEE_LOG_FILE));
		memcpy(McAfeeLogFile1, DEF_XP_MCAFEE_LOG_FILE1, sizeof(DEF_XP_MCAFEE_LOG_FILE1));
	}
	else if((strcmp(osName, OS_WINDOWS_7)==0) || (strcmp(osName, OS_WINDOWS_VISTA)==0)
		|| (strcmp(osName, OS_WINDOWS_8)==0) || (strcmp(osName, OS_WINDOWS_8_1)==0))
	{
		memcpy(McAfeeLogDir, DEF_VISTA_MCAFEE_LOG_DIR, sizeof(DEF_VISTA_MCAFEE_LOG_DIR));
		memcpy(McAfeeLogFile, DEF_VISTA_MCAFEE_LOG_FILE, sizeof(DEF_VISTA_MCAFEE_LOG_FILE));
		memcpy(McAfeeLogFile1, DEF_VISTA_MCAFEE_LOG_FILE1, sizeof(DEF_VISTA_MCAFEE_LOG_FILE1));
	}
#else
	strncpy(McAfeeLogDir, DEF_LINUX_MCAFEE_LOG_DIR, sizeof(DEF_LINUX_MCAFEE_LOG_DIR));
	strncpy(McAfeeLogFile, DEF_LINUX_MCAFEE_LOG_FILE, sizeof(DEF_LINUX_MCAFEE_LOG_FILE));
	strncpy(McAfeeLogFile1, DEF_LINUX_MCAFEE_LOG_FILE1, sizeof(DEF_LINUX_MCAFEE_LOG_FILE1));
#endif /* _WIN32 */
	sprintf(McAfeeLogFileTmp, "%s%s", McAfeeLogDir, DEF_MCAFEE_LOG_TMP);
	McAfeeLogLastLinelen = app_os_GetFileLineCount(McAfeeLogFile);
	/*	if(!app_os_is_file_exist(McAfeeLogFileTmp))
	{
	app_os_file_copy(McAfeeLogFile, McAfeeLogFileTmp);
	}*/
}

static BOOL McAfeeLogCheckStart()
{
	BOOL bRet = FALSE;
	handler_context_t *pProtectHandlerContext = (handler_context_t *)&g_HandlerContex;
#ifdef _WIN32
	pProtectHandlerContext->mcafeeLogChangeHandle = app_os_FindFirstChangeNotification(McAfeeLogDir, FALSE, FILE_NOTIFY_CHANGE_SIZE);
#endif

	pProtectHandlerContext->isMcAfeeLogCheckThreadRunning = TRUE;
	if (app_os_thread_create(&pProtectHandlerContext->mcafeeLogCheckThreadHandle, McAfeeLogCheckThreadStart, pProtectHandlerContext) != 0)
	{
		ProtectLog(g_loghandle, Warning, " %s> Start McAfee log check thread failed!", strPluginName);
		pProtectHandlerContext->isMcAfeeLogCheckThreadRunning = FALSE;
	}
	else
		bRet = TRUE;

	return bRet;
}

static BOOL McAfeeLogCheckStop()
{
	BOOL bRet = TRUE;
	handler_context_t *pProtectHandlerContext = (handler_context_t *)&g_HandlerContex;
	if(pProtectHandlerContext->isMcAfeeLogCheckThreadRunning)
	{
		app_os_set_event(pProtectHandlerContext->logCheckEventHandle);
		pProtectHandlerContext->isMcAfeeLogCheckThreadRunning = FALSE;
		app_os_thread_join(pProtectHandlerContext->mcafeeLogCheckThreadHandle);
		pProtectHandlerContext->mcafeeLogCheckThreadHandle = NULL;
	}

	if(pProtectHandlerContext->mcafeeLogChangeHandle)
	{
		app_os_FindCloseChangeNotification(pProtectHandlerContext->mcafeeLogChangeHandle);
		pProtectHandlerContext->mcafeeLogChangeHandle = NULL;
	}
	return bRet;
}

static BOOL SadminOutputCheckStart()
{
	BOOL bRet = FALSE;
	g_HandlerContex.sadminOutputCheckIntervalMs = DEF_SADMIN_OUTPUT_CHECK_INTERVAL_MS;
	g_HandlerContex.sadminOutputCheckEnable = FALSE;
	g_HandlerContex.isSadminOutputCheckThreadRunning = TRUE;
	if (app_os_thread_create(&g_HandlerContex.sadminOutputCheckThreadHandle, SadminOutputCheckThreadStart, &g_HandlerContex) != 0)
	{
		ProtectLog(g_loghandle, Warning, " %s> Start samin output check thread failed!", strPluginName);
		g_HandlerContex.isSadminOutputCheckThreadRunning = FALSE;
	}
	else bRet = TRUE;

	return bRet;
}

static BOOL SadminOutputCheckStop()
{
	BOOL bRet = TRUE;
	if(g_HandlerContex.isSadminOutputCheckThreadRunning)
	{
		g_HandlerContex.isSadminOutputCheckThreadRunning = FALSE;
		app_os_thread_join(g_HandlerContex.sadminOutputCheckThreadHandle);
		g_HandlerContex.sadminOutputCheckThreadHandle = NULL;
	}
	return bRet;
}

static CAGENT_PTHREAD_ENTRY(McAfeeLogCheckThreadStart, args)
{
	handler_context_t *pProtectHandlerContext = (handler_context_t *)args;
	int mcafeeLogLen = 0;
#ifdef _WIN32
	while(pProtectHandlerContext->isMcAfeeLogCheckThreadRunning)
	{
		if(McAfeeIsInstalling)
		{
			if(pProtectHandlerContext->mcafeeLogChangeHandle)
			{
				app_os_CloseHandle(pProtectHandlerContext->mcafeeLogChangeHandle);
				pProtectHandlerContext->mcafeeLogChangeHandle = INVALID_HANDLE_VALUE;
			}
			app_os_sleep(1000);
			continue;
		}
		if(pProtectHandlerContext->mcafeeLogChangeHandle == INVALID_HANDLE_VALUE)
		{
			pProtectHandlerContext->mcafeeLogChangeHandle = app_os_FindFirstChangeNotification(McAfeeLogDir, FALSE, FILE_NOTIFY_CHANGE_SIZE);
			app_os_sleep(100);
		}
		else
		{
			DWORD dwRet = app_os_WaitForSingleObject(pProtectHandlerContext->mcafeeLogChangeHandle, 2000);
			if(dwRet == WAIT_TIMEOUT || dwRet == WAIT_OBJECT_0)
			{
				if(!pProtectHandlerContext->isMcAfeeLogCheckThreadRunning) break;
			}

			if(dwRet == WAIT_OBJECT_0)
			{
				mcafeeLogLen = app_os_GetFileLineCount(McAfeeLogFile);
				if (mcafeeLogLen != McAfeeLogLastLinelen)
				{
					ProcessMcAfeeLogChange(mcafeeLogLen, McAfeeLogLastLinelen);
					McAfeeLogLastLinelen = mcafeeLogLen;
				}
				app_os_FindNextChangeNotification(pProtectHandlerContext->mcafeeLogChangeHandle);
			}
		}
		app_os_sleep(10);
	}

#else

	enum {EVENT_SIZE = sizeof(struct inotify_event)};
	enum {BUF_SIZE = (EVENT_SIZE + NAME_MAX + 1) << 10};
	int fd = -1;
	void *buf;
	struct inotify_event *pEvent;

	if (app_os_is_file_exist(McAfeeLogDir))
	{
		fd = inotify_watch_init(McAfeeLogDir, IN_NONBLOCK, IN_MODIFY);
	}
	buf = malloc(BUF_SIZE);
	while(pProtectHandlerContext->isMcAfeeLogCheckThreadRunning)
	{
		//protect_debug_print("McAfeeLogCheck thread: fd=%d", fd);
		if(fd < 0)
		{
			if (app_os_is_file_exist(McAfeeLogDir))
			{
				fd = inotify_watch_init(McAfeeLogDir, IN_NONBLOCK, IN_MODIFY);
			}
			app_os_sleep(5000);
			continue;
		}
		else
		{
			int i, len;
			if((len = read(fd, buf, BUF_SIZE)) > 0)
			{
				protect_debug_print("inotify_evnt on");
				for (i = 0; i < len; i += EVENT_SIZE+pEvent->len)
				{
					pEvent = (struct inotify_event *)(buf + i);
					if (pEvent->len && \
						(!strcmp(pEvent->name, strrchr(DEF_LINUX_MCAFEE_LOG_FILE, '/')+1) || \
						!strcmp(pEvent->name, strrchr(DEF_LINUX_MCAFEE_LOG_FILE1, '/')+1)))
					{
						protect_debug_print("inotify_evnt match");
						mcafeeLogLen = app_os_GetFileLineCount(McAfeeLogFile);
						protect_debug_print("mcafeeLogLen = %d, McAfeeLogLastLinelen = %d", mcafeeLogLen, McAfeeLogLastLinelen);
						if(mcafeeLogLen != McAfeeLogLastLinelen)
						{
							protect_debug_print("Check log");
							ProcessMcAfeeLogChange(mcafeeLogLen, McAfeeLogLastLinelen);
							McAfeeLogLastLinelen = mcafeeLogLen;
						}
					}
				}
			}
		}
		app_os_sleep(1000);
	}
	if (buf) free(buf);
	if (fd > 0) close(fd);
#endif /* _WIN32 */
	return 0;
}

static CAGENT_PTHREAD_ENTRY(SadminOutputCheckThreadStart, args)
{
	handler_context_t *pProtectHandlerContext = (handler_context_t *)args;
	time_t nowTime = 0, oldTime = 0;
	char sysDir[MAX_PATH] = {0};
	char sadminOuputPath[MAX_PATH] = {0};
	char sadminOutputTmpPath[MAX_PATH] = {0};
	FILE * fd = NULL;
	int lastLineCnt = 0, curLineCnt = 0;
#ifdef _WIN32
	app_os_GetSystemDirectory(sysDir, MAX_PATH);
#else
	sprintf(sysDir, "/tmp");
#endif
	strcat(sadminOuputPath, sysDir);
	strcat(sadminOutputTmpPath, sysDir);
	strcat(sadminOuputPath, DEF_SADMIN_OUTPUT_FILE_NAME);
	strcat(sadminOutputTmpPath, DEF_SADMIN_OUTPUT_TMP_FILE_NAME);

	while (pProtectHandlerContext->isSadminOutputCheckThreadRunning)
	{
		if(pProtectHandlerContext->sadminOutputCheckEnable)
		{
			if(app_os_is_file_exist(sadminOuputPath))
			{
				if(app_os_file_copy(sadminOuputPath, sadminOutputTmpPath))
				{
					//remove(sadminOuputPath);

					fd = fopen(sadminOutputTmpPath, "rb");
					if(fd)
					{
						char lineBuf[1024] = {0};
						int lineLen = 0;
						curLineCnt = 0;
						while(fgets(lineBuf, sizeof(lineBuf), fd))
						{
							curLineCnt ++;
							if(curLineCnt <= lastLineCnt) continue;
							lineLen = strlen(lineBuf) + 1;
							nowTime = time(NULL);
							if(nowTime - oldTime >= 1)
							{
								oldTime = nowTime;
								SendReplyMsg_Prtt(lineBuf, prtt_soInfo, prtt_solidify_rep);
							}
							app_os_sleep(100);
						}
						lastLineCnt = curLineCnt;
						fclose(fd);
						remove(sadminOutputTmpPath);
					}
				}
				else
				{
					ProtectLog(g_loghandle, Warning, " %s> File copy %s to %s failed!", strPluginName, DEF_SADMIN_OUTPUT_FILE_NAME, DEF_SADMIN_OUTPUT_TMP_FILE_NAME);
				}
			}
		}
		app_os_sleep(pProtectHandlerContext->sadminOutputCheckIntervalMs);
	}
	return 0;
}


static void Reboot()
{
#ifdef Protect_DEBUG
	protect_debug_print("============== OS will reboot! =============");
	app_os_sleep(5000);//5 sec
#endif
	if (-1 == app_os_PowerRestart())
	{
		ProtectLog(g_loghandle, Warning, " %s> Create shutdown process failed!", strPluginName);
	}
}


static void ParsingMsg(const char * InMsg, char * dateTimeStr, char * msgType, char * Msg)
{
	char *token = NULL;
	char inMsgTmp[1024] = {0};
	char * word[16] = {NULL};
	int i = 0, wordCnt = 0;
	char *buf = NULL;
	if(NULL == InMsg || NULL == dateTimeStr || NULL == msgType || NULL == Msg) return;
	strcpy(inMsgTmp, InMsg);
	buf = inMsgTmp;
	while((word[i] = strtok( buf, ":" )) != NULL)
	{
		i++;
		buf=NULL;
	}
	wordCnt = 0;
	while(word[wordCnt] != NULL)
	{
		TrimStr(word[wordCnt]);
		wordCnt++;
	}
	if(wordCnt) wordCnt--;

	strcpy(msgType, word[5]);

	for (i = 8; i <= wordCnt; i++)
	{
		strcat(Msg, word[i]);
		if(i < wordCnt)
		{
			strcat(Msg, ":");
		}
	}

	{
		char *dateStr[4] = {NULL};
		char *dateBuf = word[1];
		int j = 0, month = 0;
		i = 0;
		while((dateStr[i] = strtok(dateBuf, " ")) != NULL)
		{
			i++;
			dateBuf=NULL;
		}
		for(j = 0; j < 3; j++)
		{
			TrimStr(dateStr[j]);
		}
		if(dateStr[0] != NULL)
		{

			if(!strcmp("Jan", dateStr[0]))
			{
				month = 1;
			}
			else if(!strcmp("Feb", dateStr[0]))
			{
				month = 2;
			}
			else if(!strcmp("Mar", dateStr[0]))
			{
				month = 3;
			}
			else if(!strcmp("Apr", dateStr[0]))
			{
				month = 4;
			}
			else if(!strcmp("May", dateStr[0]))
			{
				month = 5;
			}
			else if(!strcmp("Jun", dateStr[0]))
			{
				month = 6;
			}
			else if(!strcmp("Jul", dateStr[0]))
			{
				month = 7;
			}
			else if(!strcmp("Aug", dateStr[0]))
			{
				month = 8;
			}
			else if(!strcmp("Sep", dateStr[0]))
			{
				month = 9;
			}
			else if(!strcmp("Oct", dateStr[0]))
			{
				month = 10;
			}
			else if(!strcmp("Nov", dateStr[0]))
			{
				month = 11;
			}
			else if(!strcmp("Dec", dateStr[0]))
			{
				month = 12;
			}
		}
		if(month >0 && month < 13)
		{
			sprintf(dateTimeStr, "%d/%s/%s %s:%s:%s", month, dateStr[1], dateStr[2], word[2], word[3], word[4]);
		}
		else
		{
			time_t nowTime = time(NULL);
			struct tm *pNowTm = gmtime(&nowTime);
			sprintf(dateTimeStr, "%d/%d/%d %d:%d:%d", pNowTm->tm_mon, pNowTm->tm_year, pNowTm->tm_mday, pNowTm->tm_hour
				, pNowTm->tm_min, pNowTm->tm_sec);
		}
	}
}

static BOOL McAfeeLogAnalyze(char * logFilePath, int LogLastLen, char * outVaildMsg, BOOL *pSkip)
{
	BOOL bRet = FALSE;
	BOOL isUnicode = app_os_FileIsUnicode(logFilePath);
	FILE *plog = fopen(logFilePath, "rb");
	if(plog)
	{
		char lineBuf[1024] = {0};
		wchar_t lineUBuf[1024] = {0};
		void * pLine = NULL;
		int currentLineNum = 0;
		while (!feof(plog))
		{
			memset(lineBuf, 0, sizeof(lineBuf));
			if(isUnicode)
			{
				memset(lineUBuf, 0, sizeof(lineUBuf));
				pLine = fgetws(lineUBuf, sizeof(lineUBuf), plog);
			}
			else
			{
				pLine = fgets(lineBuf, sizeof(lineBuf), plog);
			}

			if(pLine)
			{
				currentLineNum++;
				if(currentLineNum > LogLastLen)
				{
					if(isUnicode)
					{
						wcstombs(lineBuf, (wchar_t *)lineUBuf, sizeof(lineBuf));
					}

					if(strstr(lineBuf, "inv_volume.c") || strlen(lineBuf) == 0)
					{
						if(!(*pSkip)) sprintf(outVaildMsg, "%s", "NoSend");
					}
					else if(strstr(lineBuf, "evt.c") || strstr(lineBuf, "cmdi_process.c"))
					{
						if(strstr(lineBuf, "sadmin so\' returned 0") || \
							strstr(lineBuf, "sadmin.exe so\' returned 0") || \
							strstr(lineBuf, "sadmin version") || \
							strstr(lineBuf, "sadmin solidify\' returned 0"))
						{
							if(!(*pSkip)) sprintf(outVaildMsg, "%s", "NoSend");
						}
						else
						{
							sprintf(outVaildMsg, "%s", lineBuf);
							(*pSkip) = TRUE;
						}
					}
					else
					{
						if(!(*pSkip)) sprintf(outVaildMsg, "%s", "NoSend");
					}
				}
			}
		}
		fclose(plog);
		bRet = TRUE;
	}

	return bRet;
}

static void GetVaildMsg(int mcafeeLogCurLen, int mcafeeLogLastLen, char * outVaildMsg)
{
	BOOL skip = FALSE;
	if(mcafeeLogLastLen < mcafeeLogCurLen) 
	{// just analyze solidcore.log
		if (!McAfeeLogAnalyze(McAfeeLogFile, mcafeeLogLastLen, outVaildMsg, &skip))
			ProtectLog(g_loghandle, Warning, " %s> McAfee analyze log \'%s\' failed!", strPluginName, McAfeeLogFile);
	}
	else if(mcafeeLogLastLen > mcafeeLogCurLen)
	{// solidcore.log and solidcore.log.1 need analyze

		// analyze .log.1 first
		if (!McAfeeLogAnalyze(McAfeeLogFile1, mcafeeLogLastLen, outVaildMsg, &skip))
			ProtectLog(g_loghandle, Warning, " %s> McAfee analyze log \'%s\' failed!", strPluginName, McAfeeLogFile1);

		// analyze .log second		
		if (!McAfeeLogAnalyze(McAfeeLogFile, 0, outVaildMsg, &skip))
			ProtectLog(g_loghandle, Warning, " %s> McAfee analyze log \'%s\' failed!", strPluginName, McAfeeLogFile);
	}
}

static void ProcessMcAfeeLogChange(int logCurLineLen, int logLastLinelen)
{
	char vaildMsg[1024] = {0};

	GetVaildMsg(logCurLineLen, logLastLinelen, vaildMsg);
	protect_debug_print("vaildMsg:%s", vaildMsg);

	if (!strlen(vaildMsg) || !strcmp(vaildMsg, "NoSend"))
	{
		// skip;
	}
	else if (strstr(vaildMsg, "sadmin enable\' returned 0") ||
		strstr(vaildMsg, "sadmin enable -z \' returned 0") ||
		strstr(vaildMsg, "sadmin.exe enable\' returned 0") ||
		strstr(vaildMsg, "sadmin.exe enable -z \' returned 0") ||
		strstr(vaildMsg, "sadmin.exe\" enable\' returned 0") ||
		strstr(vaildMsg, "sadmin.exe\" enable -z \' returned 0") ||
		strstr(vaildMsg, "sadmin disable\' returned 0") ||
		strstr(vaildMsg, "sadmin disable -z \' returned 0") ||
		strstr(vaildMsg, "sadmin.exe disable\' returned 0") ||
		strstr(vaildMsg, "sadmin.exe disable -z \' returned 0") ||
		strstr(vaildMsg, "sadmin.exe\" disable\' returned 0") ||
		strstr(vaildMsg, "sadmin.exe\" disable -z \' returned 0")
		)
	{
		Reboot();
	}
	else if(!strstr(vaildMsg, "McAfee Solidifier is currently disabled"))
	{
		//parsing message, error or warning.
		UpdateStatus(vaildMsg);
	}
}

static void UpdateStatus(char * pValidMsg)
{
	handler_context_t *pProtectHandlerContext = (handler_context_t *)&g_HandlerContex;
	BOOL sendBack = FALSE;
	char dateTimeStr[64] = {0};
	char msgType[32] = {0};
	char msg[512] = {0};
	static char oldRetMsg[1024] = {0};
	static int repeatCnt = 0;
	char retMsg[1024] = {0};
	BOOL isProtect = FALSE;
	if(NULL == pValidMsg) return;
	isProtect = IsProtect();
	if(strlen(pValidMsg))
	{
		ParsingMsg(pValidMsg, dateTimeStr, msgType, msg);
		if(strstr(oldRetMsg, msg))
		{
			repeatCnt++;
		}
		else
		{
			repeatCnt = 0;
		}
		if(repeatCnt == 0)
		{
			if(!strcmp(msgType, "ERROR"))
			{
				sprintf(retMsg, "McAfeeError %s! %s", dateTimeStr, msg);
				{
#ifdef _WIN32
					char regName[] = "SOFTWARE\\McAfee";
					char valueName[] = "LastWarningMsg";
					if(app_os_set_regLocalMachine_value(regName, valueName, retMsg, strlen(retMsg)+1))
#endif
					{
						sendBack = TRUE;
					}
				}
			}
			else if(!strcmp(msgType, "WARNING"))
			{
				sprintf(retMsg, "McAfeeWarning %s! %s", dateTimeStr, msg);
				{
#ifdef _WIN32
					char regName[] = "SOFTWARE\\McAfee";
					char valueName[] = "LastWarningMsg";
					if(app_os_set_regLocalMachine_value(regName, valueName, retMsg, strlen(retMsg)+1))
#endif
					{
						sendBack = TRUE;
					}
				}
			}
			else if(!strcmp(msgType, "SYSTEM"))
			{
				char dateTimeTmp[64] = {0};
				time_t nowTime = time(NULL);
				struct tm *pNowTm = gmtime(&nowTime);
				if(!isProtect)
				{
					//             char msgTmp[] = "McAfee Solidifier is currently disabled.";
					//             strftime(dateTimeTmp, sizeof(dateTimeTmp), "%Y-%m-%d %H:%M:%S", pNowTm);
					//             sprintf(retMsg, "McAfeeWarning %s! %s", dateTimeTmp, msgTmp);
				}
				else
				{
#ifdef _WIN32
					char regName[] = "SOFTWARE\\McAfee";
					char valueName[] = "LastWarningMsg";
					if(app_os_get_regLocalMachine_value(regName, valueName, retMsg, sizeof(retMsg)))
#endif
					{
						if(strlen(retMsg) || !strcmp(retMsg, "None"))
						{
							strftime(dateTimeTmp, sizeof(dateTimeTmp), "%Y-%m-%d %H:%M:%S", pNowTm);
							sprintf(retMsg, "%s", dateTimeTmp);
						}
					}
				}
			}
		}
		if(repeatCnt >= 6)
		{
			memset(oldRetMsg, 0, sizeof(oldRetMsg));
			repeatCnt = 0;
		}
	}

	if(sendBack && strlen(retMsg) && strcmp(retMsg, oldRetMsg))
	{
		memset(oldRetMsg, 0, sizeof(oldRetMsg));
		memcpy(oldRetMsg, retMsg, strlen(retMsg)+1);
		if (!strcmp(msgType, "ERROR"))
		{
			SendReplyMsg_Prtt(retMsg, log_err, prtt_msg_rep);
		}
		else if (!strcmp(msgType, "WARNING"))
		{
			SendReplyMsg_Prtt(retMsg, log_warning, prtt_msg_rep);
		}
	}
}



/* **************************************************************************************
*  Function Name: Handler_Initialize
*  Description: Init any objects or variables of this handler
*  Input :  PLUGIN_INFO *pluginfo
*  Output: None
*  Return:  0  : Success Init Handler
*              -1 : Fail Init Handler
* ***************************************************************************************/
int HANDLER_API Handler_Initialize( HANDLER_INFO *pluginfo )
{
	if(pluginfo == NULL)
	{
		return handler_fail;
	}
	if(g_bEnableLog)
	{
		/*char MonitorLogPath[MAX_PATH] = {0};
		path_combine(MonitorLogPath, pluginfo->WorkDir, DEF_LOG_NAME);
		printf(" %s> Log Path: %s", MyTopic, MonitorLogPath);
		g_loghandle = InitLog(MonitorLogPath);*/
		g_loghandle = pluginfo->loghandle;
	}
	// 1. Topic of this handler
	snprintf( pluginfo->Name, sizeof(pluginfo->Name), "%s", strPluginName );
	pluginfo->RequestID = iRequestID;
	pluginfo->ActionID = iActionID;
	printf(" >Name: %s\n", strPluginName);
	// 2. Copy agent info 
	memcpy(&g_PluginInfo, pluginfo, sizeof(HANDLER_INFO));
	g_PluginInfo.agentInfo = pluginfo->agentInfo;

	memset(&g_HandlerContex, 0, sizeof(handler_context_t));
	InitMcAfeePath();
	InitLogPath();
#ifndef _WIN32
	InitMcAfeeStatus();
	//InitLicense();
#endif /* _WIN32 */
	if(app_os_mutex_setup(&g_HandlerContex.warningMsgMutex)!=0)
	{
		ProtectLog(g_loghandle, Normal, " %s> Create WarningMsg mutex failed!", strPluginName);
		return handler_fail;
	}
	if(app_os_mutex_setup(&g_HandlerContex.actionMsgMutex)!=0)
	{
		ProtectLog(g_loghandle, Normal, " %s> Create ActionMsg mutex failed!", strPluginName);
		return handler_fail;
	}

	// 3. Callback function -> Send JSON Data by this callback function
	g_sendcbf = g_PluginInfo.sendcbf = pluginfo->sendcbf;
	g_sendcustcbf = g_PluginInfo.sendcustcbf = pluginfo->sendcustcbf;
	g_subscribecustcbf = g_PluginInfo.subscribecustcbf = pluginfo->subscribecustcbf;
	g_sendreportcbf = g_PluginInfo.sendreportcbf = pluginfo->sendreportcbf;
	g_sendcapabilitycbf =g_PluginInfo.sendcapabilitycbf = pluginfo->sendcapabilitycbf;

	return handler_success;
}

/* **************************************************************************************
*  Function Name: Handler_Get_Status
*  Description: Get Handler Status 
*  Input :
*  Output: char * pOutReply ( JSON )
*  Return:  int  : Length of the status information in JSON format
*                       :  0 : no data need to trans
* **************************************************************************************/
int HANDLER_API Handler_Get_Status( HANDLER_THREAD_STATUS * pOutStatus ) // JSON Format
{
	int len = 0; // Data length of the pOutReply 
	return len;
}

/* **************************************************************************************
*  Function Name: Handler_OnStatusChange
*  Description: Agent can notify handler the status is changed.
*  Input :  PLUGIN_INFO *pluginfo
*  Output: None
*  Return:  None
* ***************************************************************************************/
void HANDLER_API Handler_OnStatusChange( HANDLER_INFO *pluginfo )
{
	printf(" %s> Update Status", strPluginName);
	if(pluginfo)
		memcpy(&g_PluginInfo, pluginfo, sizeof(HANDLER_INFO));
	else
	{
		memset(&g_PluginInfo, 0, sizeof(HANDLER_INFO));
		snprintf( g_PluginInfo.Name, sizeof( g_PluginInfo.Name), "%s", strPluginName );
		g_PluginInfo.RequestID = iRequestID;
		g_PluginInfo.ActionID = iActionID;
	}

	if(pluginfo->agentInfo->status == AGENT_STATUS_ONLINE)
	{
		GetProtectionCurStatus();
	}
}

/* **************************************************************************************
*  Function Name: Handler_Start
*  Description: Start Running
*  Input :  None
*  Output: None
*  Return:  0  : Success Init Handler
*              -1 : Fail Init Handler
* ***************************************************************************************/
int HANDLER_API Handler_Start( void )
{
	int iRet = handler_fail;

#ifdef _WIN32
	if (!SysLogCheckStart())
	{
		ProtectLog(g_loghandle, Warning, "%s","Sys log check start failed!");
		goto done;
	}
#endif /* _WIN32 */
	if (!SadminOutputCheckStart())
	{
		ProtectLog(g_loghandle, Warning, "%s","Sadmin output check start failed!");
		goto done;
	}
	if (!McAfeeLogCheckStart())
	{
		ProtectLog(g_loghandle, Warning, "%s","McAfee log check start failed!");
		goto done;
	}
	iRet = handler_success;

	if(g_PluginInfo.agentInfo->status == AGENT_STATUS_ONLINE)
	{
		GetProtectionCurStatus();
	}

done:
	if(iRet == handler_fail)
	{
		Handler_Stop();
	}
	return iRet;
}

/* **************************************************************************************
*  Function Name: Handler_Stop
*  Description: Stop the handler
*  Input :  None
*  Output: None
*  Return:  0  : Success Init Handler
*              -1 : Fail Init Handler
* ***************************************************************************************/
int HANDLER_API Handler_Stop( void )
{
	SadminOutputCheckStop();
	McAfeeLogCheckStop();
#ifdef _WIN32
	SysLogCheckStop();
	McAfeeInstallThreadStop();
#endif
	app_os_mutex_cleanup(&g_HandlerContex.warningMsgMutex);
	app_os_mutex_cleanup(&g_HandlerContex.actionMsgMutex);
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
void HANDLER_API Handler_Recv(char * const topic, void* const data, const size_t datalen, void *pRev1, void* pRev2  )
{
	int cmdID = 0;
	char errorStr[128] = {0};
	EINSTALLTYPE curInstallType = ITT_UNKNOWN;
	ProtectLog(g_loghandle, Normal, " %s>Recv Topic [%s] Data %s", strPluginName, topic, (char*) data );
	printf(" >Recv Topic [%s] Data %s\n", topic, (char*) data );

	if(!ParseReceivedData(data, datalen, &cmdID))
		return;

	protect_debug_print("====================== cmdId=%d =======================", cmdID);
	switch(cmdID)
	{
	case prtt_protect_req:
		{
#ifdef _WIN32
			IniPasswd();
#endif /* _WIN32 */
			ProtectAction();
			break;
		}
	case prtt_unprotect_req:
		{
#ifdef _WIN32
			IniPasswd();
#endif /* _WIN32 */
			UnProtectAction();
			break;
		}
	case prtt_return_status_req:
		{
#ifdef _WIN32
			IniPasswd();
#endif /* _WIN32 */
			if (app_os_is_file_exist(InstallPath))
			{
				int mcafeeLogLen = app_os_GetFileLineCount(McAfeeLogFile);
				if(mcafeeLogLen != McAfeeLogLastLinelen)
				{
					ProcessMcAfeeLogChange(mcafeeLogLen, McAfeeLogLastLinelen);
					McAfeeLogLastLinelen = mcafeeLogLen;
				}
				else
				{
					UpdateStatus("");
				}
			}
			else
			{
				UpdateStatus("");
			}
			break;
		}
	case prtt_update_req:
#ifdef _WIN32
		curInstallType = ITT_UPDATE;
#endif /*_WIN32*/
	case prtt_install_req:
#ifdef _WIN32
		{
			BOOL isInstall = FALSE;
			prtt_installer_dl_params_t dlParams;
			memset(&dlParams, 0, sizeof(prtt_installer_dl_params_t));
			IsInstallThenActive = FALSE;
			if(Parser_ParseInstallerDlParams(data, datalen, &dlParams))
			{
				if(curInstallType == ITT_UNKNOWN) curInstallType = ITT_INSTALL;
				isInstall = TRUE;
			}
			else
			{
				memset(errorStr, 0, sizeof(errorStr));
				if (curInstallType == ITT_UPDATE)
				{
					sprintf(errorStr, "Command(%d) parse error!", prtt_update_rep);
					SendReplyMsg_Prtt(errorStr, updt_parseErr, prtt_update_rep);
				}
				else
				{
					sprintf(errorStr, "Command(%d) parse error!", prtt_install_rep);
					SendReplyMsg_Prtt(errorStr, istl_parseErr, prtt_install_rep);	
				}
			}
			dlParams.installType = curInstallType;
			if(isInstall) 
			{
				CurInstallType = ITT_UNKNOWN;
				IsInstallThenActive = dlParams.isInstallThenActive;
				InstallAction(&dlParams);
			}
			break;
		}
#else
		{
			char msgBuf[1024] = {0};
			sprintf(msgBuf, "%s,%s,%s", "InstallInfo", "Fail", "Not support the remote installation!");
			SendReplyMsg_Prtt(msgBuf, istl_notSupport, prtt_install_rep);	
			break;
		}
#endif /* _WIN32 */
	case prtt_active_req:
		{
			Activate();
			//UpdateStatus("");
			break;
		}
	case prtt_status_req:
		{
			GetProtectionCurStatus();
			break;
		}
	case prtt_capability_req:
		{
			char * pSendVal = NULL;
			int jsonStrlen = GetPrttCapability(prtt_capability_cmd_way, &pSendVal);
			if(jsonStrlen > 0 && pSendVal != NULL)
			{
				g_sendcbf(&g_PluginInfo, prtt_capability_rep, pSendVal, jsonStrlen, NULL, NULL);
			}
			break;
		}
	default: 
		{
			SendReplyMsg_Prtt("Unknown cmd!", oprt_unkown, prtt_error_rep);			
			break;
		}
	}
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
	/*{"susiCommData":{"catalogID":4,"autoUploadIntervalSec":30,"requestID":1001,"requestItems":["all"],"commCmd":2053,"type":"WSN"}}*/
}

/* **************************************************************************************
*  Function Name: Handler_AutoReportStop
*  Description: Stop Auto Report
*  Input : char *pInQuery
*  Output: None
*  Return: None
* ***************************************************************************************/
void HANDLER_API Handler_AutoReportStop(char *pInQuery)
{

}

/* **************************************************************************************
*  Function Name: Handler_Get_Capability
*  Description: Get Handler Information specification. 
*  Input :  None
*  Output: char * : pOutReply       // JSON Format
*  Return:  int  : Length of the status information in JSON format
*                :  0 : no data need to trans
* **************************************************************************************/
int HANDLER_API Handler_Get_Capability( char ** pOutReply ) // JSON Format
{
	int len = 0; // Data length of the pOutReply 
	len = GetPrttCapability(prtt_capability_api_way, pOutReply);
	return len;
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