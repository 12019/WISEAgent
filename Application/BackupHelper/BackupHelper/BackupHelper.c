#include <stdio.h>
#include <windows.h>
#include "Log.h"

#define DEF_BACKUP_HELPER_LOG_NAME    "CAgentBackupHelperLog.txt"   //Backup helper log file name
#define BACKUP_HELPER_LOG_ENABLE
//#define DEF_BACKUP_HELPER_LOG_MODE    (LOG_MODE_NULL_OUT)
//#define DEF_BACKUP_HELPER_LOG_MODE    (LOG_MODE_FILE_OUT)
#define DEF_BACKUP_HELPER_LOG_MODE    (LOG_MODE_CONSOLE_OUT|LOG_MODE_FILE_OUT)
LOGHANDLE BackupHelperLogHandle = NULL;
#ifdef BACKUP_HELPER_LOG_ENABLE
#define BackupHelperLog(level, fmt, ...)  do { if (BackupHelperLogHandle != NULL)   \
	WriteLog(BackupHelperLogHandle, DEF_BACKUP_HELPER_LOG_MODE, level, fmt, ##__VA_ARGS__); } while(0)
#else
#define BackupHelperLog(level, fmt, ...)
#endif

char SQLiteInterfacePath[MAX_PATH] = {0};
char AcrocmdPath[MAX_PATH] = {0};
char BackupLogPath[MAX_PATH] = {0};
#define SQLITE_INTERFACE_PATH   "\\Advantech\\Acronis\\SQLiteInterface.exe"
#define ACROCMD_PATH            "\\Acronis\\CommandLineTool\\acrocmd.exe"
#define BACKUP_LOG_PATH         "\\Temp\\BackupInfo.txt"

BOOL GetDefProgramFilesPath(char *pProgramFilesPath)
{
	BOOL bRet = FALSE;
	HKEY hk;
	char regName[] = "SOFTWARE\\Microsoft\\Windows\\CurrentVersion";
	if(NULL == pProgramFilesPath) return bRet;

	if(ERROR_SUCCESS != RegOpenKeyEx(HKEY_LOCAL_MACHINE, regName, 0, KEY_ALL_ACCESS, &hk)) return bRet;
	else
	{
		char valueName[] = "ProgramFilesDir";
		char valueData[MAX_PATH] = {0};
		int  valueDataSize = sizeof(valueData);
		if(ERROR_SUCCESS != RegQueryValueEx(hk, valueName, 0, NULL, valueData, &valueDataSize)) return bRet;
		else
		{
			strcpy(pProgramFilesPath, valueData);
			bRet = TRUE;
		}
		RegCloseKey(hk);
	}
	return bRet;
}

BOOL GetMoudlePath(char * moudlePath)
{
	BOOL bRet = FALSE;
	char * lastSlash = NULL;
	char tempPath[MAX_PATH] = {0};
	if(NULL == moudlePath) return bRet;
	if(ERROR_SUCCESS != GetModuleFileName(NULL, tempPath, sizeof(tempPath)))
	{
		lastSlash = strrchr(tempPath, '\\');
		if(NULL != lastSlash)
		{
			strncpy(moudlePath, tempPath, lastSlash - tempPath + 1);
			bRet = TRUE;
		}
	}
	return bRet;
}

int main(int argc, char * argv[])
{
#ifdef BACKUP_HELPER_LOG_ENABLE
	if(BackupHelperLogHandle == NULL)
	{
		char mdPath[MAX_PATH] = {0};
		char BackupHelperLogPath[MAX_PATH] = {0};
		GetMoudlePath(mdPath);
		sprintf_s(BackupHelperLogPath, sizeof(BackupHelperLogPath), "%s%s", mdPath, DEF_BACKUP_HELPER_LOG_NAME);
		BackupHelperLogHandle = InitLog(BackupHelperLogPath);
	}
	BackupHelperLog(Normal, "%s", "Cagentbackuphelper start!");
#endif
	{
		char tmpProgFilesPath[MAX_PATH] = {0}; 
		char * tmpCh = NULL;
		if(!GetDefProgramFilesPath(tmpProgFilesPath)) return FALSE;
		sprintf(AcrocmdPath, "%s%s", tmpProgFilesPath, ACROCMD_PATH);
		sprintf(SQLiteInterfacePath, "%s%s", tmpProgFilesPath, SQLITE_INTERFACE_PATH);
	}
	{
		char * winPath = NULL;
		winPath = getenv("WINDIR");
		if(winPath)
		{
			sprintf(BackupLogPath, "%s%s", winPath, BACKUP_LOG_PATH);
		}
		else
		{
			sprintf(BackupLogPath, "%s%s", "C:\\Windows", BACKUP_LOG_PATH);
		}
	}
	{
		char cmdLine[MAX_PATH] = {0};
		STARTUPINFO si;
		PROCESS_INFORMATION pi;
		DWORD prcExitCode = -1;
		memset(&si, 0, sizeof(si));
		si.dwFlags = STARTF_USESHOWWINDOW;  
		si.wShowWindow = SW_HIDE;
		si.cb = sizeof(si);
		memset(&pi, 0, sizeof(pi));

		sprintf(cmdLine, "%s", " \"Acronis\" \"BackupStart\" \"10\"");
		//printf("SQLiteInterfacePath: %s\n", SQLiteInterfacePath);
		//printf("AcrocmdPath: %s\n", AcrocmdPath);
		//printf("BackupLogPath: %s\n", BackupLogPath);
		if(CreateProcess(SQLiteInterfacePath, cmdLine, NULL, NULL, FALSE, CREATE_NO_WINDOW , NULL, NULL, &si, &pi))
		{
			WaitForSingleObject(pi.hProcess, INFINITE);
			GetExitCodeProcess(pi.hProcess, &prcExitCode);
			if(prcExitCode == 0)
			{
				prcExitCode = -1;
				memset(cmdLine, 0, sizeof(cmdLine));
				memset(&si, 0, sizeof(si));
				si.dwFlags = STARTF_USESHOWWINDOW;  
				si.wShowWindow = SW_HIDE;
				si.cb = sizeof(si);
				memset(&pi, 0, sizeof(pi));

				sprintf(cmdLine, " backup disk --volume=C --loc=atis:///asz --arc=BK_ASZ2 --backuptype=incremental --silent_mode=on --log=\"%s\"", BackupLogPath);

				if(CreateProcess(AcrocmdPath, cmdLine, NULL, NULL, FALSE, CREATE_NO_WINDOW , NULL, NULL, &si, &pi))
				{
					WaitForSingleObject(pi.hProcess, INFINITE);
					GetExitCodeProcess(pi.hProcess, &prcExitCode);
					memset(cmdLine, 0, sizeof(cmdLine));
					memset(&si, 0, sizeof(si));
					si.dwFlags = STARTF_USESHOWWINDOW;  
					si.wShowWindow = SW_HIDE;
					si.cb = sizeof(si);
					memset(&pi, 0, sizeof(pi));
					if(prcExitCode == 0)
					{
						prcExitCode = -1;
						sprintf(cmdLine, "%s", " \"Acronis\" \"BackupSuccess\" \"4\"");
						if(CreateProcess(SQLiteInterfacePath, cmdLine, NULL, NULL, FALSE, CREATE_NO_WINDOW , NULL, NULL, &si, &pi))
						{
							WaitForSingleObject(pi.hProcess, INFINITE);
							GetExitCodeProcess(pi.hProcess, &prcExitCode);
							if(prcExitCode != 0)
							{
								BackupHelperLog(Error, "%sExitCode:%d", "SQLite write backup success info process exit code!=0 !", prcExitCode);
							}
						}
						else
						{
							BackupHelperLog(Error, "%sGetlastError:%d", "Create SQLite write backup success info process failed!", GetLastError());
						}
					}
					else
					{
						prcExitCode = -1;
						sprintf(cmdLine, "%s", " \"Acronis\" \"BackupError\" \"3\"");
						if(CreateProcess(SQLiteInterfacePath, cmdLine, NULL, NULL, FALSE, CREATE_NO_WINDOW , NULL, NULL, &si, &pi))
						{
							WaitForSingleObject(pi.hProcess, INFINITE);
							GetExitCodeProcess(pi.hProcess, &prcExitCode);
							if(prcExitCode != 0)
							{
								BackupHelperLog(Error, "%sExitCode:%d", "SQLite write backup error info process exit code!=0 !", prcExitCode);
							}
						}
						else
						{
							BackupHelperLog(Error, "%sGetlastError:%d", "Create SQLite write backup error info process failed!", GetLastError());
						}
					}
				}
				else
				{
					BackupHelperLog(Error, "%sGetlastError:%d", "Create backup process failed!", GetLastError());
				}
			}
			else
			{
				BackupHelperLog(Error, "%sExitCode:%d", "SQLite write backup start info process exit code!=0 !", prcExitCode);
			}
		}
		else
		{
			BackupHelperLog(Error, "%sGetlastError:%d", "Create SQLite write backup start info process failed!", GetLastError());
		}
	}
	BackupHelperLog(Normal, "%s", "Cagentbackuphelper quit!");
#ifdef BACKUP_HELPER_LOG_ENABLE
	if(BackupHelperLogHandle != NULL) 
	{
		UninitLog(BackupHelperLogHandle);
		BackupHelperLogHandle = NULL;
	}
#endif
	return 0;
}