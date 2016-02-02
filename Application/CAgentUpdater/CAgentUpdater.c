#include <stdio.h>
#include "Global.h"
#include "Log.h"
#include "md5.h"

#define DEF_MD5_SIZE                            16
#define DEF_UPDATER_LOG_NAME    "CAgentUpdaterLog.txt"   //Updater log file name
#define UPDATER_LOG_ENABLE
//#define DEF_UPDATER_LOG_MODE    (LOG_MODE_NULL_OUT)
//#define DEF_UPDATER_LOG_MODE    (LOG_MODE_FILE_OUT)
#define DEF_UPDATER_LOG_MODE    (LOG_MODE_CONSOLE_OUT|LOG_MODE_FILE_OUT)
LOGHANDLE updaterLogHandle = NULL;
#ifdef UPDATER_LOG_ENABLE
#define UpdaterLog(level, fmt, ...)  do { if (updaterLogHandle != NULL)   \
	WriteIndividualLog(updaterLogHandle, "updater", DEF_UPDATER_LOG_MODE, level, fmt, ##__VA_ARGS__); } while(0)
#else
#define UpdaterLog(level, fmt, ...)
#endif

#define DEF_ADVANTECH_FOLDER_NAME                       "Advantech"
#define DEF_CAGENT_INSTALLER_DOWNLOAD_FILE_NAME         "SA31_CAgent.exe"
#define DEF_CAGENT_UPDATE_CMD                           "/qn"
//#define DEF_CAGENT_UPDATE_CMD                           "/qn  /L*V \"C:\\example.log\""
#define DEF_CAGENT_SERVICE_NAME                         "AgentService_31"
#define DEF_CAGENT_EXE_NAME                             "CAgent.exe"
#define DEF_CAGENT_STDA_EXE_NAME                        "Standalone Agent.exe"
#define DEF_CAGENT_WATCHDOG_SERVICE_NAME                "SAWatchdogService"
#define DEF_CAGENT_WATCHDOG_EXE_NAME                    "SAWatchdog.exe"
static char CagentInstallerPath[MAX_PATH] = {0};
static char McAfeePasswd[BUFSIZ] = {0};

static BOOL GetCAgentInstallerPath(char * installerPath);
static BOOL StartCAgent();
static BOOL UpdateCAgent();
static BOOL StopCAgent();
static BOOL IsProtected();
static void EnterMcAfeeUpdateMode();
static void EndMcAfeeUpdateMode();
static BOOL GetMD5(char * buf, unsigned int bufSize, unsigned char retMD5[DEF_MD5_SIZE]);
static char *GetMcAfeePasswd(char * passwdBuf);

char * GetLastErrorMessage()
{
	LPSTR lpMsgBuf;
	DWORD dw = GetLastError(); 
	char* result = NULL;
	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | 
		FORMAT_MESSAGE_FROM_SYSTEM |
		FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL,
		dw,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR) & lpMsgBuf,
		0, NULL );

	// Display the error message and exit the process
	result = malloc(MAX_PATH);
	memset(result, 0, MAX_PATH);
	sprintf(result,"%s",lpMsgBuf);

	LocalFree(lpMsgBuf);

	return result;
}

static BOOL GetCAgentInstallerPath(char * installerPath)
{
	if(installerPath == NULL) return FALSE;
	{
		char progFilesPath[MAX_PATH] = {0};
		char advantchPath[MAX_PATH] = {0};

		if(GetTempPath (MAX_PATH, progFilesPath) > 0)
		{
			sprintf(advantchPath, "%s%s", progFilesPath, DEF_ADVANTECH_FOLDER_NAME);
			CreateDirectory(advantchPath,NULL);
			sprintf(installerPath, "%s\\%s", advantchPath, DEF_CAGENT_INSTALLER_DOWNLOAD_FILE_NAME);
		}
		else
		{
			char modulePath[MAX_PATH] = {0};     
			GetMoudlePath(modulePath);
			sprintf(installerPath, "%s%s", modulePath, DEF_CAGENT_INSTALLER_DOWNLOAD_FILE_NAME);
		}
	}
	return TRUE;
}

static BOOL StopCAgent()
{
	BOOL bRet = FALSE;
	if(ProcessCheck(DEF_CAGENT_STDA_EXE_NAME))
	{
		bRet = KillProcessWithName(DEF_CAGENT_STDA_EXE_NAME);
		if(bRet) UpdaterLog(Normal, "Kill process:%s ok", DEF_CAGENT_STDA_EXE_NAME);
		else UpdaterLog(Error, "Kill process:%s failed", DEF_CAGENT_STDA_EXE_NAME);
		Sleep(100);
	}

	bRet = FALSE;
	{
		DWORD dwRet = GetSrvStatus(DEF_CAGENT_WATCHDOG_SERVICE_NAME);
		if(dwRet == SERVICE_RUNNING)
		{
			dwRet = StopSrv(DEF_CAGENT_WATCHDOG_SERVICE_NAME);
			if(dwRet == SERVICE_STOPPED || dwRet == SERVICE_STOP_PENDING)
			{
				UpdaterLog(Normal, "Stop srv:%s ok", DEF_CAGENT_WATCHDOG_SERVICE_NAME);
				bRet = TRUE;
			}
			else
			{
				UpdaterLog(Error, "Stop srv:%s failed", DEF_CAGENT_WATCHDOG_SERVICE_NAME);
			}
		}
		else
		{
			bRet = TRUE;
			if(ProcessCheck(DEF_CAGENT_WATCHDOG_EXE_NAME))
			{
				bRet = KillProcessWithName(DEF_CAGENT_WATCHDOG_EXE_NAME);
				if(bRet) UpdaterLog(Normal, "Kill process:%s ok", DEF_CAGENT_WATCHDOG_EXE_NAME);
				else UpdaterLog(Error, "Kill process:%s failed", DEF_CAGENT_WATCHDOG_EXE_NAME);
				Sleep(200);
			}
		}
	}

	bRet = FALSE;
	{
		DWORD dwRet = GetSrvStatus(DEF_CAGENT_SERVICE_NAME);
		if(dwRet == SERVICE_RUNNING)
		{
			dwRet = StopSrv(DEF_CAGENT_SERVICE_NAME);
			if(dwRet == SERVICE_STOPPED || dwRet == SERVICE_STOP_PENDING)
			{
				UpdaterLog(Normal, "Stop srv:%s ok", DEF_CAGENT_SERVICE_NAME);
				bRet = TRUE;
			}
			else
			{
				UpdaterLog(Error, "Stop srv:%s failed", DEF_CAGENT_SERVICE_NAME);
			}
		}
		else
		{
			bRet = TRUE;
			if(ProcessCheck(DEF_CAGENT_EXE_NAME))
			{
				bRet = KillProcessWithName(DEF_CAGENT_EXE_NAME);
				if(bRet) UpdaterLog(Normal, "Kill process:%s ok", DEF_CAGENT_EXE_NAME);
				else UpdaterLog(Error, "Kill process:%s failed", DEF_CAGENT_EXE_NAME);
				Sleep(200);
			}
		}
	}
	return bRet;
}

static BOOL UpdateCAgent()
{
	BOOL bRet = FALSE;
	{
		KillProcessWithName(DEF_CAGENT_INSTALLER_DOWNLOAD_FILE_NAME);
		{
			char cmdLine[BUFSIZ] = {0};
			STARTUPINFO si;
			PROCESS_INFORMATION pi;

			memset(&si, 0, sizeof(si));
			si.dwFlags = STARTF_USESHOWWINDOW;  
			si.wShowWindow = SW_HIDE;
			si.cb = sizeof(si);
			memset(&pi, 0, sizeof(pi));

			sprintf(cmdLine, "%s \"%s\" %s", "cmd.exe /c ", CagentInstallerPath, DEF_CAGENT_UPDATE_CMD);
			if(CreateProcess(NULL, cmdLine, NULL, NULL, FALSE, CREATE_NO_WINDOW , NULL, NULL, &si, &pi))
			{
				//if(CreateProcess(CagentInstallerPath, DEF_CAGENT_UPDATE_CMD, NULL, NULL, FALSE, CREATE_NO_WINDOW , NULL, NULL, &si, &pi))
				//{
				UpdaterLog(Normal, "Create process:%s ok", CagentInstallerPath);
				WaitForSingleObject(pi.hProcess, INFINITE);
				CloseHandle(pi.hProcess);
				CloseHandle(pi.hThread);
				bRet = TRUE;
			}
			else
			{
				char * error = GetLastErrorMessage();
				UpdaterLog(Error, "Create process:%s failed: %s", CagentInstallerPath, error);
			}
		}
	}
	return bRet;
}

static BOOL StartCAgent()
{
	BOOL bRet = FALSE;
	if(!IsSrvExist(DEF_CAGENT_SERVICE_NAME))
	{
		if(!ProcessCheck(DEF_CAGENT_EXE_NAME))
		{
			char modulePath[MAX_PATH] = {0};
			char cmdLine[BUFSIZ] = "cmd.exe /c ";
			STARTUPINFO si;
			PROCESS_INFORMATION pi;

			memset(&si, 0, sizeof(si));
			si.dwFlags = STARTF_USESHOWWINDOW;  
			si.wShowWindow = SW_HIDE;
			si.cb = sizeof(si);
			memset(&pi, 0, sizeof(pi));        
			GetMoudlePath(modulePath);
			sprintf(cmdLine, "%s \"%s\\%s\"  %s", cmdLine, modulePath, DEF_CAGENT_EXE_NAME, "-n");
			if(CreateProcess(NULL, cmdLine, NULL, NULL, FALSE, CREATE_NO_WINDOW , NULL, NULL, &si, &pi))
			{
				bRet = TRUE;
			}
		}
	}
	else
	{
		bRet = StartSrv(DEF_CAGENT_SERVICE_NAME);
	}

	return bRet;
}

static void PowerOff()
{
	char cmdLine[BUFSIZ] = {0};
	STARTUPINFO si;
	PROCESS_INFORMATION pi;

	memset(&si, 0, sizeof(si));
	si.dwFlags = STARTF_USESHOWWINDOW;  
	si.wShowWindow = SW_HIDE;
	si.cb = sizeof(si);
	memset(&pi, 0, sizeof(pi));

	memset(cmdLine, 0, sizeof(cmdLine));
	sprintf(cmdLine, "%s", "cmd.exe /c shutdown /r /t 10");
	if(!CreateProcess(NULL, cmdLine, NULL, NULL, FALSE, CREATE_NO_WINDOW , NULL, NULL, &si, &pi))
	{
		UpdaterLog(Error, "Create shutdown process failed!cmd:%s", cmdLine);
	}
	else
	{
		UpdaterLog(Normal, "Create shutdown process ok!cmd:%s", cmdLine);
	}
}

static BOOL IsProtected()
{
	BOOL bRet = FALSE;
	char regName[] = "SYSTEM\\CurrentControlSet\\services\\swin\\Parameters";
	char valueName[] = "RTEMode";
	HKEY hk;
	DWORD valueDW = 0;
	DWORD  valueDataSize = sizeof(valueDW);
	if(ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, regName, 0, KEY_ALL_ACCESS, &hk)) 
	{
		if(ERROR_SUCCESS == RegQueryValueEx(hk, valueName, 0, NULL, (LPBYTE)&valueDW, &valueDataSize))
		{			
			bRet = (valueDW == 0 ? FALSE : TRUE);			
			UpdaterLog(Normal, "Get McAfee status successfully!");
		}
		RegCloseKey(hk);
	}
	return bRet;
}

static BOOL GetMD5(char * buf, unsigned int bufSize, unsigned char retMD5[DEF_MD5_SIZE])
{
	MD5_CTX context;
	if(NULL == buf || NULL == retMD5) return FALSE;
	memset(&context, 0, sizeof(MD5_CTX));
	MD5Init(&context);
	MD5Update(&context, buf, bufSize);
	MD5Final(retMD5, &context);
	return TRUE;
}

static char *GetMcAfeePasswd(char * passwdBuf)
{
	char *pRet = NULL;
	char PassWord[9] = {0};
	char regName[] = "SOFTWARE\\McAfee";
	char valueName[] = "Passwd";
	HKEY hk;
	char valueData[BUFSIZ] = {0};
	DWORD  valueDataSize = sizeof(valueData);
	if (passwdBuf)
	{
		//Read data from reg
		if(ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, regName, 0, KEY_ALL_ACCESS, &hk)) 
		{
			if(ERROR_SUCCESS == RegQueryValueEx(hk, valueName, 0, NULL, (LPBYTE)valueData, &valueDataSize))
			{	
				if(strlen(valueData) && strcmp(valueData, "None"))
				{
					strcpy(PassWord, valueData);
				}
			}
			RegCloseKey(hk);
		}	
		//Get the password
		if (strlen(PassWord) > 0)
		{
			char srcBuf[32] = {0};
			unsigned char retMD5[DEF_MD5_SIZE] = {0};
			char md5str0x[DEF_MD5_SIZE*2+1] = {0};
			strcat(srcBuf, PassWord);
			strcat(srcBuf, "AdvSUSIAccess");
			GetMD5(srcBuf, strlen(srcBuf), retMD5);
			{
				int i = 0;
				for(i = 0; i<DEF_MD5_SIZE; i++)
				{
					sprintf(&md5str0x[i*2], "%.2x", retMD5[i]);
				}
			}
			memcpy(passwdBuf, md5str0x, 8);
			pRet = passwdBuf;
			UpdaterLog(Normal, "Get McAfee Password successfully!");
		}
	}
	return pRet;
}

static void EnterMcAfeeUpdateMode()
{
	char cmdLine[BUFSIZ] = {0};
	STARTUPINFO si;
	PROCESS_INFORMATION pi;

	memset(&si, 0, sizeof(si));
	si.dwFlags = STARTF_USESHOWWINDOW;  
	si.wShowWindow = SW_HIDE;
	si.cb = sizeof(si);
	memset(&pi, 0, sizeof(pi));

	memset(cmdLine, 0, sizeof(cmdLine));
	sprintf(cmdLine, "%s", "cmd.exe /c sadmin.exe begin-update");
	if (GetMcAfeePasswd(McAfeePasswd))
	{
		strcat(cmdLine, " -z ");
		strcat(cmdLine, McAfeePasswd);
	}
	if(!CreateProcess(NULL, cmdLine, NULL, NULL, FALSE, CREATE_NO_WINDOW , NULL, NULL, &si, &pi))
	{
		UpdaterLog(Error, "Create begin-update process failed!");
	}
	else
	{
		UpdaterLog(Normal, "Create begin-update process ok!");
	}
}

static void EndMcAfeeUpdateMode()
{
	char cmdLine[BUFSIZ] = {0};
	STARTUPINFO si;
	PROCESS_INFORMATION pi;

	memset(&si, 0, sizeof(si));
	si.dwFlags = STARTF_USESHOWWINDOW;  
	si.wShowWindow = SW_HIDE;
	si.cb = sizeof(si);
	memset(&pi, 0, sizeof(pi));

	memset(cmdLine, 0, sizeof(cmdLine));
	sprintf(cmdLine, "%s", "cmd.exe /c sadmin.exe end-update");
	if (strlen(McAfeePasswd) > 0)
	{
		strcat(cmdLine, " -z ");
		strcat(cmdLine, McAfeePasswd);
	}
	if(!CreateProcess(NULL, cmdLine, NULL, NULL, FALSE, CREATE_NO_WINDOW , NULL, NULL, &si, &pi))
	{
		UpdaterLog(Error, "Create end-update process failed!");
	}
	else
	{
		UpdaterLog(Normal, "Create end-update process ok!");
	}
}

int main(int argc, char * argv[])
{ 
	BOOL bMcAfeeStatus = FALSE;
#ifdef UPDATER_LOG_ENABLE
	if(updaterLogHandle == NULL)
	{
		char mdPath[MAX_PATH] = {0};
		//char updaterLogPath[MAX_PATH] = {0};
		GetMoudlePath(mdPath);
		//sprintf_s(updaterLogPath, sizeof(updaterLogPath), "%s%s", mdPath, DEF_UPDATER_LOG_NAME);
		updaterLogHandle = InitLog(mdPath);
	}
#endif
	GetCAgentInstallerPath(CagentInstallerPath);
	UpdaterLog(Normal, "CagentInstallerPath:%s", CagentInstallerPath);
	if(!IsFileExist(CagentInstallerPath))
	{
		UpdaterLog(Error, "%s not exist!", CagentInstallerPath);
	}
	else
	{
		StopCAgent();
		bMcAfeeStatus = IsProtected();
		if (bMcAfeeStatus)
		{
			UpdaterLog(Normal, "McAfee enable!");
			EnterMcAfeeUpdateMode();
		}
		else
		{
			UpdaterLog(Normal, "McAfee disable!");
		}
		UpdaterLog(Normal, "%s", "Run cagent updater...");
		if(UpdateCAgent())
		{
			Sleep(5000);
			while(TRUE)
			{
				if(ProcessCheck(DEF_CAGENT_INSTALLER_DOWNLOAD_FILE_NAME))
				{
					Sleep(500);
				}
				else
				{
					break;
				}
			}
			UpdaterLog(Normal, "%s", "Run cagent updater ok!");
			//PowerOff();

			//          printf("Start cagent...\n");
			//          if(StartCAgent())
			//          {
			//             printf("Start cagent ok!\n");
			//          }
			//          else
			//          {
			//             printf("Start cagent failed!\n");
			//          }
		}
		else
		{
			UpdaterLog(Error, "%s", "Run cagent updater failed!");
		}
		if (bMcAfeeStatus)
		{
			EndMcAfeeUpdateMode();
		}
	}
	//    getchar();
#ifdef UPDATER_LOG_ENABLE
	if(updaterLogHandle != NULL) 
	{
		UninitLog(updaterLogHandle);
		updaterLogHandle = NULL;
	}
#endif
	UpdaterLog(Normal, "%s", "Cagentupdater quit!");
	return 0;
}