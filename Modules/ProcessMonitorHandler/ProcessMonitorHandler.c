/****************************************************************************/
/* Copyright(C) : Advantech Technologies, Inc.								*/
/* Create Date  : 								*/
/* Abstract     : ProcessMonitor API interface definition   					*/
/* Reference    : None														*/
/****************************************************************************/
#include "process_common.h"
#include "process_threshold.h"
#include "process_config.h"
#include "ProcessMonitorHandler.h"
#include "ProcessMonitorLog.h"
//#include <configuration.h>

#ifdef WIN32
#include <psapi.h>
#pragma comment(lib,"Psapi.lib")
#endif

#ifdef IS_SA_WATCH
#include "SUSIAgentWatch.h"
#endif

static void* g_loghandle = NULL;
static BOOL g_bEnableLog = true;

const char strPluginName[MAX_TOPIC_LEN] = {"ProcessMonitor"};
const int iRequestID = cagent_request_software_monitoring;
const int iActionID = cagent_reply_software_monitoring;

//Limit sending process spec info.
#define   MAX_PROC_CHANGED_CNT 10
#define   SPEC_INFO_TIME_OUT_MS 20000
clock_t m_startTime = 0;
int m_cntProcChanged = 0;
static BOOL m_bWillShutdown = false;
//BOOL m_bNormalStatusChange = true;
static char agentConfigFilePath[MAX_PATH] = {0};
//end
#ifdef WIN32
char CmdHelperFile[MAX_PATH] = {0};
#endif

static BOOL UploadSysMonInfo(int sendLevel,int repCommandID);
static BOOL UpdateLogonUserPrcList(prc_mon_info_node_t * head);
BOOL KillProcessWithID(DWORD prcID);
DWORD RestartProcessWithID(DWORD prcID);
static prc_mon_info_node_t * FindPrcMonInfoNode(prc_mon_info_node_t * head, DWORD prcID);

/******************function**************************/
static void SWMKillPrc(sw_mon_handler_context_t * pSWMonHandlerContext, char * killPrcInfo)
{
	if(killPrcInfo == NULL || pSWMonHandlerContext == NULL) return;
	{
		unsigned int temp = 0;
		char repMsg[2*1024] = {0};
		char errorStr[128] = {0};

		if(Parse_swm_kill_prc_req(killPrcInfo, &temp))
		{
			unsigned int * pPrcID = &temp;
			if(pPrcID)
			{
				prc_mon_info_node_t * findPrcMonInfoNode = NULL;
				app_os_EnterCriticalSection(&pSWMonHandlerContext->swPrcMonCS);
				findPrcMonInfoNode = FindPrcMonInfoNode(pSWMonHandlerContext->prcMonInfoList, *pPrcID);
				if(findPrcMonInfoNode)
				{
					BOOL bRet = FALSE;
					bRet = KillProcessWithID(*pPrcID);
					if(!bRet)
					{
						sprintf_s(repMsg, sizeof(repMsg), "Process(PrcID-%d) #tk#kill failed#tk#!", *pPrcID);
					}
					else
					{
						sprintf_s(repMsg, sizeof(repMsg), "Process(PrcID-%d) #tk#kill success#tk#!", *pPrcID);
					}
				}
				// Don't send 'is not exist' message
				/*else
				{
					sprintf_s(repMsg, sizeof(repMsg), "Process(PrcID-%d) #tk#is not exist#tk#!", *pPrcID);
				}*/
				app_os_LeaveCriticalSection(&pSWMonHandlerContext->swPrcMonCS);
			}
			else
			{
				memset(errorStr, 0, sizeof(errorStr));
				sprintf(errorStr, "Command(%d) params error!", swm_kill_prc_req);
				{
					char * uploadRepJsonStr = NULL;
					char * str = errorStr;
					int jsonStrlen = Parser_string(str, &uploadRepJsonStr,swm_error_rep);
					if(jsonStrlen > 0 && uploadRepJsonStr != NULL)
					{
						g_sendcbf(&g_PluginInfo, swm_error_rep, uploadRepJsonStr, strlen(uploadRepJsonStr)+1, NULL, NULL);
					}
					if(uploadRepJsonStr)free(uploadRepJsonStr);	
				}
			}
		}
		else
		{
			memset(errorStr, 0, sizeof(errorStr));
			sprintf(errorStr, "Command(%d) parse error!", swm_kill_prc_req);
			{
				char * uploadRepJsonStr = NULL;
				char * str = errorStr;
				int jsonStrlen = Parser_string(str, &uploadRepJsonStr,swm_error_rep);
				if(jsonStrlen > 0 && uploadRepJsonStr != NULL)
				{
					g_sendcbf(&g_PluginInfo, swm_error_rep, uploadRepJsonStr, strlen(uploadRepJsonStr)+1, NULL, NULL);
				}
				if(uploadRepJsonStr)free(uploadRepJsonStr);	
			}

		}

		if(strlen(repMsg) == 0)
		{
			char * uploadRepJsonStr = NULL;
			char * str = errorStr;
			int jsonStrlen = Parser_string(str, &uploadRepJsonStr,swm_kill_prc_rep);
			if(jsonStrlen > 0 && uploadRepJsonStr != NULL)
			{
				g_sendcbf(&g_PluginInfo, swm_kill_prc_rep, uploadRepJsonStr, strlen(uploadRepJsonStr)+1, NULL, NULL);
			}
			if(uploadRepJsonStr)free(uploadRepJsonStr);	
		}

	}
}

static void SWMRestartPrc(sw_mon_handler_context_t * pSWMonHandlerContext, char * restartPrcInfo)
{
	if(restartPrcInfo == NULL || pSWMonHandlerContext == NULL) return;
	{
		unsigned int temp = 0;
		char repMsg[2*1024] = {0};
		char errorStr[128] = {0};

		if(Parse_swm_restart_prc_req(restartPrcInfo, &temp))
		{
			unsigned int * pPrcID = &temp;
			if(pPrcID)
			{
				prc_mon_info_node_t * findPrcMonInfoNode = NULL;
				app_os_EnterCriticalSection(&pSWMonHandlerContext->swPrcMonCS);
				findPrcMonInfoNode = FindPrcMonInfoNode(pSWMonHandlerContext->prcMonInfoList, *pPrcID);
				if(findPrcMonInfoNode)
				{
					DWORD newPrcID = 0;
					newPrcID = RestartProcessWithID(*pPrcID);
					if(newPrcID > 0)
					{
						UpdateLogonUserPrcList(pSWMonHandlerContext->prcMonInfoList);
						sprintf_s(repMsg, sizeof(repMsg), "Process(PrcID-%d) #tk#restart success#tk#!", *pPrcID);
					}
					else
					{
						sprintf_s(repMsg, sizeof(repMsg), "Process(PrcID-%d) #tk#restart failed#tk#!", *pPrcID);
					}
				}
				// Don't send 'is not exist' message
				/*else
				{
					sprintf_s(repMsg, sizeof(repMsg), "Process(PrcID-%d) #tk#is not exist#tk#!", *pPrcID);
				}*/
				app_os_LeaveCriticalSection(&pSWMonHandlerContext->swPrcMonCS);
			}
			else
			{
				memset(errorStr, 0, sizeof(errorStr));
				sprintf(errorStr, "Command(%d) params error!", swm_restart_prc_req);
				{
					char * uploadRepJsonStr = NULL;
					char * str = errorStr;
					int jsonStrlen = Parser_string(str, &uploadRepJsonStr,swm_error_rep);
					if(jsonStrlen > 0 && uploadRepJsonStr != NULL)
					{
						g_sendcbf(&g_PluginInfo, swm_error_rep, uploadRepJsonStr, strlen(uploadRepJsonStr)+1, NULL, NULL);
					}
					if(uploadRepJsonStr)free(uploadRepJsonStr);	
				}
			}
		}
		else
		{
			memset(errorStr, 0, sizeof(errorStr));
			sprintf(errorStr, "Command(%d) parse error!", swm_restart_prc_req);
			{
				char * uploadRepJsonStr = NULL;
				char * str = errorStr;
				int jsonStrlen = Parser_string(str, &uploadRepJsonStr,swm_error_rep);
				if(jsonStrlen > 0 && uploadRepJsonStr != NULL)
				{
					g_sendcbf(&g_PluginInfo, swm_error_rep, uploadRepJsonStr, strlen(uploadRepJsonStr)+1, NULL, NULL);
				}
				if(uploadRepJsonStr)free(uploadRepJsonStr);	
			}

		}
		if(strlen(repMsg) > 0)
		{
			{
				char * uploadRepJsonStr = NULL;
				char * str = repMsg;
				int jsonStrlen = Parser_string(str, &uploadRepJsonStr,swm_restart_prc_rep);
				if(jsonStrlen > 0 && uploadRepJsonStr != NULL)
				{
					g_sendcbf(&g_PluginInfo, swm_restart_prc_rep, uploadRepJsonStr, strlen(uploadRepJsonStr)+1, NULL, NULL);
				}
				if(uploadRepJsonStr)free(uploadRepJsonStr);	
			}
		}
	}
}

static int DeleteAllPrcMonInfoNode(prc_mon_info_node_t * head)
{
	int iRet = -1;
	prc_mon_info_node_t * delNode = NULL;
	if(head == NULL) return iRet;

	delNode = head->next;
	while(delNode)
	{
		head->next = delNode->next;
		if(delNode->prcMonInfo.prcName)
		{
			free(delNode->prcMonInfo.prcName);
			delNode->prcMonInfo.prcName = NULL;
		}
		if(delNode->prcMonInfo.ownerName)
		{
			free(delNode->prcMonInfo.ownerName);
			delNode->prcMonInfo.ownerName = NULL;
		}
		free(delNode);
		delNode = head->next;
	}

	iRet = 0;
	return iRet;
}

static void DestroyPrcMonInfoList(prc_mon_info_node_t * head)
{
	if(NULL == head) return;
	DeleteAllPrcMonInfoNode(head);
	free(head); 
	head = NULL;
}

#ifdef WIN32
BOOL AdjustPrivileges() 
{
	HANDLE hToken;
	TOKEN_PRIVILEGES tp;
	TOKEN_PRIVILEGES oldtp;
	DWORD dwSize=sizeof(TOKEN_PRIVILEGES);
	LUID luid;

	if (!app_os_OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken)) 
	{
		if (app_os_GetLastError()==ERROR_CALL_NOT_IMPLEMENTED) return TRUE;
		else return FALSE;
	}

	if (!app_os_LookupPrivilegeValueA(NULL, SE_DEBUG_NAME, &luid)) 
	{
		app_os_CloseHandle(hToken);
		return FALSE;
	}

	memset(&tp, 0, sizeof(tp));
	tp.PrivilegeCount=1;
	tp.Privileges[0].Luid=luid;
	tp.Privileges[0].Attributes=SE_PRIVILEGE_ENABLED;

	if (!app_os_AdjustTokenPrivileges(hToken, FALSE, &tp, sizeof(TOKEN_PRIVILEGES), &oldtp, &dwSize)) 
	{
		app_os_CloseHandle(hToken);
		return FALSE;
	}

	app_os_CloseHandle(hToken);
	return TRUE;
}

HANDLE GetProcessHandleWithID(DWORD prcID)
{
	HANDLE hPrc = NULL;
	hPrc = app_os_OpenProcess(PROCESS_ALL_ACCESS, FALSE, prcID);
	if(hPrc == NULL) 
	{
		DWORD dwRet = app_os_GetLastError();          
		if(dwRet == 5)
		{
			if(AdjustPrivileges())
			{
				hPrc = app_os_OpenProcess(PROCESS_ALL_ACCESS, FALSE, prcID);
			}
		}
	}
	return hPrc;
}
#else
HANDLE GetProcessHandleWithID(DWORD prcID)
{
	return prcID;
}
#endif

static int GetProcessCPUUsageWithID(DWORD prcID, prc_cpu_usage_time_t * pPrcCpuUsageLastTimes)
{
	HANDLE hPrc = NULL;
	int prcCPUUsage = 0;
	BOOL bRet = FALSE;
	__int64 nowKernelTime = 0, nowUserTime = 0, nowTime = 0, divTime = 0;
	__int64 creationTime = 0, exitTime = 0;
	SYSTEM_INFO sysInfo;
	int processorCnt = 1;
	if(pPrcCpuUsageLastTimes == NULL) return prcCPUUsage;
	app_os_GetSystemInfo(&sysInfo);
	if(sysInfo.dwNumberOfProcessors) processorCnt = sysInfo.dwNumberOfProcessors;
	hPrc =GetProcessHandleWithID(prcID);
	if(hPrc == NULL) return prcCPUUsage;
	GetSystemTimeAsFileTime((FILETIME*)&nowTime);
	bRet = app_os_GetProcessTimes(hPrc, (FILETIME*)&creationTime, (FILETIME*)&exitTime, 
		(FILETIME*)&nowKernelTime, (FILETIME*)&nowUserTime);
	if(!bRet)
	{
		app_os_CloseHandle(hPrc);
		return prcCPUUsage;
	}
	if(pPrcCpuUsageLastTimes->lastKernelTime == 0 && pPrcCpuUsageLastTimes->lastUserTime == 0)
	{
		pPrcCpuUsageLastTimes->lastKernelTime = nowKernelTime;
		pPrcCpuUsageLastTimes->lastUserTime = nowUserTime;
		pPrcCpuUsageLastTimes->lastTime = nowTime;
		app_os_CloseHandle(hPrc);
		return prcCPUUsage;
	}
	divTime = nowTime - pPrcCpuUsageLastTimes->lastTime;

	prcCPUUsage =(int) ((((nowKernelTime - pPrcCpuUsageLastTimes->lastKernelTime) + (nowUserTime - pPrcCpuUsageLastTimes->lastUserTime))*100/divTime)/processorCnt);

	pPrcCpuUsageLastTimes->lastKernelTime = nowKernelTime;
	pPrcCpuUsageLastTimes->lastUserTime = nowUserTime;
	pPrcCpuUsageLastTimes->lastTime = nowTime;
	app_os_CloseHandle(hPrc);
	return prcCPUUsage;
}

#ifdef WIN32
BOOL CALLBACK SAEnumProc(HWND hWnd,LPARAM lParam)
{
	DWORD dwProcessId;
	LPWNDINFO pInfo = NULL;
	app_os_GetWindowThreadProcessId(hWnd, &dwProcessId);
	pInfo = (LPWNDINFO)lParam;

	if(dwProcessId == pInfo->dwProcessId)
	{
		BOOL isWindowVisible = app_os_IsWindowVisible(hWnd);
		if(isWindowVisible == TRUE)
		{
			pInfo->hWnd = hWnd;
			return FALSE;
		}
	}
	return TRUE;
}


HWND GetProcessMainWnd(DWORD dwProcessId)
{
	WNDINFO wi;
	wi.dwProcessId = dwProcessId;
	wi.hWnd = NULL;
	EnumWindows(SAEnumProc,(LPARAM)&wi);
	return wi.hWnd;
}
/*
//useless
BOOL IsProcessActiveWithIDEx(DWORD prcID)
{
	BOOL bRet = FALSE;

	if(prcID > 0)
	{
		HWND hWnd = NULL;
		bRet = TRUE;
		hWnd = GetProcessMainWnd(prcID);
		if(hWnd && IsWindow(hWnd))
		{
			if(IsHungAppWindow(hWnd))
			{
				bRet = FALSE;
			}
		}
	}

	return bRet;
}
*/
BOOL GetProcessMemoryUsageKBWithID(DWORD prcID, long * memUsageKB)
{
   BOOL bRet = FALSE;
   HANDLE hPrc = NULL;
   PROCESS_MEMORY_COUNTERS pmc;
   if(NULL == memUsageKB) return FALSE;
   hPrc =GetProcessHandleWithID(prcID);

   memset(&pmc, 0, sizeof(pmc));

   if (GetProcessMemoryInfo(hPrc, &pmc, sizeof(pmc)))
   {
      *memUsageKB = (long)(pmc.WorkingSetSize/DIV);
      bRet = TRUE;
   }

   app_os_CloseHandle(hPrc);

   return bRet;
}
#endif

#ifdef WIN32
BOOL RunProcessInfoHelperAsUser(char * cmdLine, char *prcUserName)
{
	BOOL bRet = FALSE;
	if(NULL == cmdLine || NULL == prcUserName) return bRet;
	{
		int errorNo  = 0;
		BOOL isRun = TRUE;
		HANDLE hToken = NULL;
		HANDLE hPrc = NULL;
		PROCESSENTRY32 pe;
		HANDLE hSnapshot=NULL;

		hSnapshot=CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS,0);
		pe.dwSize=sizeof(PROCESSENTRY32);
		if(Process32First(hSnapshot,&pe))
		{
			while(isRun)
			{
				pe.dwSize=sizeof(PROCESSENTRY32);
				isRun = Process32Next(hSnapshot,&pe);
				if(isRun)
				{
					if(_stricmp(pe.szExeFile, "EXPLORER.EXE")==0)
					{	
						hPrc = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, pe.th32ProcessID);
						bRet = OpenProcessToken(hPrc,TOKEN_ALL_ACCESS, &hToken);
						if(bRet)
						{
							//bRet = ScreenshotAdjustPrivileges(hToken);
							//if(bRet)
							char tokUserName[128] = {0};
							app_os_GetProcessUsername(hPrc, tokUserName, 128);
							if(!strcmp(prcUserName,tokUserName))
							{
								STARTUPINFO si;
								PROCESS_INFORMATION pi;
								DWORD dwCreateFlag = CREATE_NO_WINDOW;
								memset(&si, 0, sizeof(si));
								si.dwFlags = STARTF_USESHOWWINDOW; 
								si.wShowWindow = SW_HIDE;
								si.cb = sizeof(si);
								memset(&pi, 0, sizeof(pi));
								bRet = FALSE;
								if(CreateProcessAsUser(hToken, NULL,cmdLine,  NULL ,NULL,
									FALSE, dwCreateFlag, NULL, NULL, &si, &pi))
								{
									errorNo = GetLastError();
									bRet = TRUE;
									//isRun = FALSE;
									WaitForSingleObject(pi.hProcess, INFINITE);
									CloseHandle(pi.hProcess);
									CloseHandle(pi.hThread);
								}
/*#ifndef DEBUG
								else
								{
									ProcessMonitorLog(g_loghandle, Normal, "%s()[%d]###Create user process failed!\n",__FUNCTION__, __LINE__);
								}
#endif*/
							}
							CloseHandle(hToken);
						}
						else
						{
							ProcessMonitorLog(g_loghandle, Normal, "%s()[%d]###Get user token failed!\n",__FUNCTION__, __LINE__);
						}
						CloseHandle(hPrc);
					}
				}
			}
		}
		if(hSnapshot) CloseHandle(hSnapshot);
	}
	return bRet;
}

BOOL IsProcessActiveWithIDEx(DWORD prcID, BOOL *isActive, BOOL *isShutdown)
{
	BOOL bRet = TRUE;
	if(prcID > 0){
		HANDLE hMutex      = NULL;
		HANDLE hFileMapping   = NULL;
		LPVOID lpShareMemory  = NULL;
		HANDLE hServerWriteOver = NULL;
		HANDLE hClientReadOver = NULL;
		msg_context_t *pShareBuf = NULL;
		SECURITY_ATTRIBUTES securityAttr;
		SECURITY_DESCRIPTOR secutityDes;

		HANDLE hPrc = NULL;
		char prcUserName[128] = {0};
		//SUSIAgentLog(ERROR, "---SoftwareMonitor S---");
		//FILE *fp = NULL;
		//if(fp = fopen("server_log.txt","a+"))
		//  fputs("Open ok!\n",fp);
		//create share memory 
		 //LPSECURITY_ATTRIBUTES lpAttributes;
		InitializeSecurityDescriptor(&secutityDes, SECURITY_DESCRIPTOR_REVISION);
		SetSecurityDescriptorDacl(&secutityDes, TRUE, NULL, FALSE);

		securityAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
		securityAttr.bInheritHandle = FALSE;
		securityAttr.lpSecurityDescriptor = &secutityDes;
		hFileMapping = CreateFileMapping(INVALID_HANDLE_VALUE,
										&securityAttr,
										PAGE_READWRITE,
										0,
										sizeof(msg_context_t),
										"Global\\Share-Mem-For-RMM-3-ID-0x0001");

		if (NULL == hFileMapping) 
		{ 
			ProcessMonitorLog(g_loghandle, Normal, "%s()[%d]###CreateFileMapping failed!\n",__FUNCTION__, __LINE__);
			goto SERVER_SHARE_MEMORY_END; 
		}
		lpShareMemory = MapViewOfFile(hFileMapping, 
									FILE_MAP_ALL_ACCESS, 
									0, 
									0,   //memory start address 
									0);   //all memory space 
		if (NULL == lpShareMemory) 
		{ 
			ProcessMonitorLog(g_loghandle, Normal, "%s()[%d]###MapViewOfFile failed!\n",__FUNCTION__, __LINE__);
			goto SERVER_SHARE_MEMORY_END; 
		}
		pShareBuf = (msg_context_t *)lpShareMemory;
		memset(pShareBuf,0,sizeof(msg_context_t));
		pShareBuf->procID = prcID;  //firefox for test.
		pShareBuf->isActive = TRUE;
		pShareBuf->isShutdown = FALSE;
		hPrc =  GetProcessHandleWithID(prcID);
		if(hPrc)
		{
			app_os_GetProcessUsername(hPrc, prcUserName, 128);
			RunProcessInfoHelperAsUser(CmdHelperFile, prcUserName);
		}

		bRet = *isActive = pShareBuf->isActive;
		*isShutdown = pShareBuf->isShutdown;
SERVER_SHARE_MEMORY_END:
		//if(fp) fclose(fp);
		if (NULL != lpShareMemory)   UnmapViewOfFile(lpShareMemory); 
		if (NULL != hFileMapping)    CloseHandle(hFileMapping);
	}
	return bRet;
}
#endif

static void GetPrcMonInfo(prc_mon_info_node_t * head)
{
	prc_mon_info_node_t * curNode;
	long prcMemUsage = -1;
	if(NULL == head) return;
	curNode = head->next;
	while(curNode && !m_bWillShutdown)
	{
		app_os_EnterCriticalSection(&SWMonHandlerContext.swPrcMonCS);
		if(curNode->prcMonInfo.prcName)
		{
			BOOL isActive = TRUE;
			BOOL isShutdown = FALSE;
			curNode->prcMonInfo.cpuUsage = GetProcessCPUUsageWithID(curNode->prcMonInfo.prcID, &curNode->prcMonInfo.prcCpuUsageLastTimes);
			//curNode->prcMonInfo.isActive = IsProcessActiveWithIDEx(curNode->prcMonInfo.prcID); 
#ifdef WIN32
			IsProcessActiveWithIDEx(curNode->prcMonInfo.prcID,&isActive, &isShutdown);   
			curNode->prcMonInfo.isActive = isActive;
			m_bWillShutdown = isShutdown;
#else
			curNode->prcMonInfo.isActive = IsProcessActiveWithIDEx(curNode->prcMonInfo.prcID); 
#endif
			if(GetProcessMemoryUsageKBWithID(curNode->prcMonInfo.prcID, &prcMemUsage))
			{
				curNode->prcMonInfo.memUsage = prcMemUsage;
			}
			else
			{
				curNode->prcMonInfo.memUsage = 0;
			}
		}
		curNode = curNode->next;
		app_os_LeaveCriticalSection(&SWMonHandlerContext.swPrcMonCS);
		app_os_sleep(10);
	}
}

static BOOL UpdatePrcList(prc_mon_info_node_t * head)
{
	BOOL bRet = FALSE;
	if(head == NULL) return bRet;
	{
		prc_mon_info_node_t * frontNode = head;
		prc_mon_info_node_t * delNode = NULL;
		prc_mon_info_node_t * curNode = frontNode->next;
		while(curNode)
		{
			if(curNode->prcMonInfo.isValid == 0)
			{
				frontNode->next = curNode->next;
				delNode = curNode;
				if(delNode->prcMonInfo.prcName)
				{
					free(delNode->prcMonInfo.prcName);
					delNode->prcMonInfo.prcName = NULL;
				}
				if(delNode->prcMonInfo.ownerName)
				{
					free(delNode->prcMonInfo.ownerName);
					delNode->prcMonInfo.ownerName = NULL;
				}
				free(delNode);
				delNode = NULL;
				bRet = TRUE;   //change flag
				m_cntProcChanged++;
			}
			else
			{
				frontNode = curNode;
			}
			curNode = frontNode->next;
		}
	}
	return bRet;
}

static BOOL ResetPrcList(prc_mon_info_node_t * head)
{
	BOOL bRet = FALSE;
	if(head == NULL) return bRet;
	{
		prc_mon_info_node_t * curNode = head->next;
		while(curNode)
		{
			curNode->prcMonInfo.isValid = 0;
			curNode = curNode->next;
		}
	}
	return bRet = TRUE;
}

#ifdef WIN32
BOOL IsWorkStationLocked()
{
	// note: we can't call OpenInputDesktop directly because it's not
	// available on win 9x
	typedef HDESK (WINAPI *PFNOPENDESKTOP)(LPSTR lpszDesktop, DWORD dwFlags, BOOL fInherit, ACCESS_MASK dwDesiredAccess);
	typedef BOOL (WINAPI *PFNCLOSEDESKTOP)(HDESK hDesk);
	typedef BOOL (WINAPI *PFNSWITCHDESKTOP)(HDESK hDesk);

	// load user32.dll once only
	HMODULE hUser32 = LoadLibrary("user32.dll");

	if (hUser32)
	{
		PFNOPENDESKTOP fnOpenDesktop = (PFNOPENDESKTOP)GetProcAddress(hUser32, "OpenDesktopA");
		PFNCLOSEDESKTOP fnCloseDesktop = (PFNCLOSEDESKTOP)GetProcAddress(hUser32, "CloseDesktop");
		PFNSWITCHDESKTOP fnSwitchDesktop = (PFNSWITCHDESKTOP)GetProcAddress(hUser32, "SwitchDesktop");

		if (fnOpenDesktop && fnCloseDesktop && fnSwitchDesktop)
		{
			HDESK hDesk = fnOpenDesktop("Default", 0, FALSE, DESKTOP_SWITCHDESKTOP);

			if (hDesk)
			{
				BOOL bLocked = !fnSwitchDesktop(hDesk);

				// cleanup
				fnCloseDesktop(hDesk);

				return bLocked;
			}
		}
	}

	// must be win9x
	return FALSE;
}
#endif

static prc_mon_info_node_t * FindPrcMonInfoNode(prc_mon_info_node_t * head, DWORD prcID)
{
	prc_mon_info_node_t * findNode = NULL;
	if(head == NULL) return findNode;
	findNode = head->next;
	while(findNode)
	{
		if(findNode->prcMonInfo.prcID == prcID) break;
		else
		{
			findNode = findNode->next;
		}
	}

	return findNode;
}

static int InsertPrcMonInfoNode(prc_mon_info_node_t * head, prc_mon_info_t * prcMonInfo)
{
	int iRet = -1;
	prc_mon_info_node_t * newNode = NULL, * findNode = NULL;
	if(prcMonInfo == NULL || head == NULL) return iRet;
	findNode = FindPrcMonInfoNode(head, prcMonInfo->prcID);
	if(findNode == NULL && prcMonInfo->prcName)
	{
		newNode = (prc_mon_info_node_t *)malloc(sizeof(prc_mon_info_node_t));
		if(!newNode) return 1;
		memset(newNode, 0, sizeof(prc_mon_info_node_t));
		if(prcMonInfo->prcName)
		{
			int prcNameLen = strlen(prcMonInfo->prcName);
			newNode->prcMonInfo.prcName = (char *)malloc(prcNameLen + 1);
			if(newNode->prcMonInfo.prcName)
			{
				memset(newNode->prcMonInfo.prcName, 0, prcNameLen + 1);
				memcpy(newNode->prcMonInfo.prcName, prcMonInfo->prcName, prcNameLen);
			}
		}
		if(prcMonInfo->ownerName)
		{
			int ownerNameLen = strlen(prcMonInfo->ownerName);
			newNode->prcMonInfo.ownerName = (char *)malloc(ownerNameLen + 1);
			if(newNode->prcMonInfo.ownerName)
			{
				memset(newNode->prcMonInfo.ownerName, 0, ownerNameLen + 1);
				memcpy(newNode->prcMonInfo.ownerName, prcMonInfo->ownerName, ownerNameLen);
			}
		}
		newNode->prcMonInfo.prcID = prcMonInfo->prcID;
		newNode->prcMonInfo.isValid = prcMonInfo->isValid;

		newNode->prcMonInfo.cpuUsage = 0;
		newNode->prcMonInfo.memUsage = 0;
		newNode->prcMonInfo.isActive = 0;
		newNode->prcMonInfo.prcCpuUsageLastTimes.lastKernelTime = 0;
		newNode->prcMonInfo.prcCpuUsageLastTimes.lastUserTime = 0;
		newNode->prcMonInfo.prcCpuUsageLastTimes.lastTime = 0;

		newNode->next = head->next;
		head->next = newNode;
		iRet = 0;
	}
	else
	{
		iRet = 1;
	}
	return iRet;
}

BOOL ProcessCheck(char * processName)
{
	BOOL bRet = FALSE;
	PROCESSENTRY32 pe;
	//DWORD id=0;
	HANDLE hSnapshot=NULL;
	if(NULL == processName) return bRet;
	hSnapshot=app_os_CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS,0);
	pe.dwSize=sizeof(PROCESSENTRY32);
	if(!app_os_Process32First(hSnapshot,&pe))
	{
		app_os_CloseHandle(hSnapshot);
		return bRet;
	}
	while(TRUE)
	{
		pe.dwSize=sizeof(PROCESSENTRY32);
		if(app_os_Process32Next(hSnapshot,&pe)==FALSE)
			break;
		if(stricmp(pe.szExeFile,processName)==0)
		{
			//id=pe.th32ProcessID;
			bRet = TRUE;
			break;
		}
	}
	app_os_CloseSnapShot32Handle(hSnapshot);//app_os_CloseHandle(hSnapshot);
	return bRet;
}

BOOL CheckPrcUserFromList(char *logonUserList, char *procUserName, int logonUserCnt,int maxLogonUserNameLen)
{
	BOOL bRet = FALSE;
	int i = 0;
	if(logonUserList == NULL || procUserName == NULL || logonUserCnt == 0) return bRet;
	for(i = 0; i<logonUserCnt; i++)
	{
		if(!strcmp(&(logonUserList[i*maxLogonUserNameLen]), procUserName))
		{
			bRet = TRUE;
			break;
		}
	}
	return bRet;
}

void UpdatePrcList_CheckAllUsers(prc_mon_info_node_t * head, sw_mon_handler_context_t * pSoftwareMonHandlerContext,BOOL *bChangeFlag)
{
	if(NULL == head || NULL == pSoftwareMonHandlerContext) return;
	{
	char logonUserName[MAX_LOGON_USER_NAME_LEN] = {0};
	char procUserName[MAX_LOGON_USER_NAME_LEN] = {0};
	char logonUserList[MAX_LOGON_USER_CNT][MAX_LOGON_USER_NAME_LEN] = {0};
	int logonUserCnt = 0;
	//app_os_GetSysLogonUserName(logonUserName, sizeof(logonUserName));
	app_os_GetSysLogonUserList(logonUserList, &logonUserCnt, MAX_LOGON_USER_CNT, MAX_LOGON_USER_NAME_LEN);
	//if(strlen(logonUserName))
	if( logonUserCnt > 0)
	{
		PROCESSENTRY32 pe;
		HANDLE hSnapshot = NULL;
		pSoftwareMonHandlerContext->isUserLogon = TRUE;
		hSnapshot=app_os_CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS,0);
		pe.dwSize=sizeof(PROCESSENTRY32);
		if(app_os_Process32First(hSnapshot,&pe))
		{
			while(TRUE)
			{
				HANDLE procHandle = NULL;
				memset(procUserName, 0, sizeof(procUserName));
				procHandle = GetProcessHandleWithID(pe.th32ProcessID);
				if(procHandle)
				{
					app_os_GetProcessUsername(procHandle, procUserName, sizeof(procUserName));
				}
				//if(strlen(procUserName) && !strcmp(logonUserName, procUserName))
				if(strlen(procUserName) && CheckPrcUserFromList(logonUserList,procUserName,logonUserCnt, MAX_LOGON_USER_NAME_LEN))
				{
					prc_mon_info_node_t * findNode = NULL;
					findNode = FindPrcMonInfoNode(head, pe.th32ProcessID);
					if(findNode)
					{
						findNode->prcMonInfo.isValid = 1;
					}
					else
					{
						prc_mon_info_t prcMonInfo;
						int tmpLen = 0;
						memset(&prcMonInfo, 0, sizeof(prc_mon_info_t));
						prcMonInfo.isValid = 1;
						prcMonInfo.prcID = pe.th32ProcessID;
						tmpLen = strlen(pe.szExeFile);
						prcMonInfo.prcName = (char*)malloc(tmpLen + 1);
						if(prcMonInfo.prcName)
						{
							memset(prcMonInfo.prcName, 0, tmpLen + 1);
							memcpy(prcMonInfo.prcName, pe.szExeFile, tmpLen);
						}
						tmpLen = strlen(procUserName);
						if(tmpLen)
						{
							prcMonInfo.ownerName = (char*)malloc(tmpLen + 1);
							if(prcMonInfo.ownerName)
							{
								memset(prcMonInfo.ownerName, 0, tmpLen + 1);
								memcpy(prcMonInfo.ownerName, procUserName, tmpLen);
							}
						}
						app_os_EnterCriticalSection(&pSoftwareMonHandlerContext->swPrcMonCS);
						InsertPrcMonInfoNode(head, &prcMonInfo);
						app_os_LeaveCriticalSection(&pSoftwareMonHandlerContext->swPrcMonCS);
						if(prcMonInfo.prcName)
						{
							free(prcMonInfo.prcName);
							prcMonInfo.prcName = NULL;
						}
						if(prcMonInfo.ownerName)
						{
							free(prcMonInfo.ownerName);
							prcMonInfo.ownerName = NULL;
						}
						*bChangeFlag = TRUE;
						m_cntProcChanged++;
					}
				}
				if(procHandle) app_os_CloseHandle(procHandle);
				app_os_sleep(10);
				pe.dwSize=sizeof(PROCESSENTRY32);
				if(app_os_Process32Next(hSnapshot,&pe)==FALSE)
					break;
			}
		}
		if(hSnapshot) app_os_CloseSnapShot32Handle(hSnapshot);//app_os_CloseHandle(hSnapshot);//Wei.Gang modified
	}
	else
		pSoftwareMonHandlerContext->isUserLogon = FALSE;
	}
}

void UpdatePrcList_CheckUser(prc_mon_info_node_t * head, sw_mon_handler_context_t * pSoftwareMonHandlerContext, char * chkUserName, BOOL *bChangeFlag)
{
	if(NULL == head || NULL == pSoftwareMonHandlerContext || NULL == chkUserName) return;
	if(!strlen(chkUserName)) return;
	{
	//char logonUserName[MAX_LOGON_USER_NAME_LEN] = {0};
	char procUserName[MAX_LOGON_USER_NAME_LEN] = {0};
	char logonUserList[MAX_LOGON_USER_CNT][MAX_LOGON_USER_NAME_LEN] = {0};
	int logonUserCnt = 0;
	BOOL isUserExist = FALSE;
	//app_os_GetSysLogonUserName(logonUserName, sizeof(logonUserName));
	app_os_GetSysLogonUserList(logonUserList, &logonUserCnt, MAX_LOGON_USER_CNT, MAX_LOGON_USER_NAME_LEN);
	//if(strlen(logonUserName))
	if( logonUserCnt > 0)
	{
		int i = 0;
		for(i = 0; i < logonUserCnt; i++)
		if(!strcmp(logonUserList[i], chkUserName))
		{
			isUserExist=TRUE;
			break;
		}
	}
	if(isUserExist)
	{
		PROCESSENTRY32 pe;
		HANDLE hSnapshot = NULL;
		pSoftwareMonHandlerContext->isUserLogon = TRUE;
		hSnapshot=app_os_CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS,0);
		pe.dwSize=sizeof(PROCESSENTRY32);
		if(app_os_Process32First(hSnapshot,&pe))
		{
			while(TRUE)
			{
				HANDLE procHandle = NULL;
				memset(procUserName, 0, sizeof(procUserName));
				procHandle = GetProcessHandleWithID(pe.th32ProcessID);
				if(procHandle)
				{
					app_os_GetProcessUsername(procHandle, procUserName, sizeof(procUserName));
				}
				if(strlen(procUserName) && !strcmp(chkUserName, procUserName))
				{
					prc_mon_info_node_t * findNode = NULL;
					findNode = FindPrcMonInfoNode(head, pe.th32ProcessID);
					if(findNode)
					{
						findNode->prcMonInfo.isValid = 1;
					}
					else
					{
						prc_mon_info_t prcMonInfo;
						int tmpLen = 0;
						memset(&prcMonInfo, 0, sizeof(prc_mon_info_t));
						prcMonInfo.isValid = 1;
						prcMonInfo.prcID = pe.th32ProcessID;
						tmpLen = strlen(pe.szExeFile);
						prcMonInfo.prcName = (char*)malloc(tmpLen + 1);
						if(prcMonInfo.prcName)
						{
							memset(prcMonInfo.prcName, 0, tmpLen + 1);
							memcpy(prcMonInfo.prcName, pe.szExeFile, tmpLen);
						}
						tmpLen = strlen(procUserName);
						if(tmpLen)
						{
							prcMonInfo.ownerName = (char*)malloc(tmpLen + 1);
							if(prcMonInfo.ownerName)
							{
								memset(prcMonInfo.ownerName, 0, tmpLen + 1);
								memcpy(prcMonInfo.ownerName, procUserName, tmpLen);
							}
						}
						app_os_EnterCriticalSection(&pSoftwareMonHandlerContext->swPrcMonCS);
						InsertPrcMonInfoNode(head, &prcMonInfo);
						app_os_LeaveCriticalSection(&pSoftwareMonHandlerContext->swPrcMonCS);
						if(prcMonInfo.prcName)
						{
							free(prcMonInfo.prcName);
							prcMonInfo.prcName = NULL;
						}
						if(prcMonInfo.ownerName)
						{
							free(prcMonInfo.ownerName);
							prcMonInfo.ownerName = NULL;
						}
						*bChangeFlag = TRUE;
						m_cntProcChanged++;
					}
				}
				if(procHandle) app_os_CloseHandle(procHandle);
				app_os_sleep(10);
				pe.dwSize=sizeof(PROCESSENTRY32);
				if(app_os_Process32Next(hSnapshot,&pe)==FALSE)
					break;
			}
		}
		if(hSnapshot) app_os_CloseSnapShot32Handle(hSnapshot);//app_os_CloseHandle(hSnapshot);//Wei.Gang modified
	}
	else
		pSoftwareMonHandlerContext->isUserLogon = FALSE;
	}
}

void UpdatePrcList_All(prc_mon_info_node_t * head, sw_mon_handler_context_t * pSoftwareMonHandlerContext, BOOL *bChangeFlag)
{
	if(NULL ==head || NULL == pSoftwareMonHandlerContext) return;
	{
	//char logonUserName[MAX_LOGON_USER_NAME_LEN] = {0};
	char procUserName[MAX_LOGON_USER_NAME_LEN] = {0};
	//char logonUserList[MAX_LOGON_USER_CNT][MAX_LOGON_USER_NAME_LEN] = {0};
	//int logonUserCnt = 0;
	//app_os_GetSysLogonUserName(logonUserName, sizeof(logonUserName));
	//app_os_GetSysLogonUserList(logonUserList, &logonUserCnt, MAX_LOGON_USER_CNT, MAX_LOGON_USER_NAME_LEN);
	//if(strlen(logonUserName))
	//if( logonUserCnt > 0)
	{
		PROCESSENTRY32 pe;
		HANDLE hSnapshot = NULL;
		pSoftwareMonHandlerContext->isUserLogon = TRUE;   //always
		hSnapshot=app_os_CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS,0);
		pe.dwSize=sizeof(PROCESSENTRY32);
		if(app_os_Process32First(hSnapshot,&pe))
		{
			while(TRUE)
			{
				HANDLE procHandle = NULL;
				memset(procUserName, 0, sizeof(procUserName));
				procHandle = GetProcessHandleWithID(pe.th32ProcessID);
				if(procHandle)
				{
					app_os_GetProcessUsername(procHandle, procUserName, sizeof(procUserName));
				}
				//if(strlen(procUserName) && !strcmp(logonUserName, procUserName))
				//if(strlen(procUserName) && CheckPrcUserFromList(logonUserList,procUserName,logonUserCnt, MAX_LOGON_USER_NAME_LEN))
				{
					prc_mon_info_node_t * findNode = NULL;
					findNode = FindPrcMonInfoNode(head, pe.th32ProcessID);
					if(findNode)
					{
						findNode->prcMonInfo.isValid = 1;
					}
					else
					{
						prc_mon_info_t prcMonInfo;
						int tmpLen = 0;
						memset(&prcMonInfo, 0, sizeof(prc_mon_info_t));
						prcMonInfo.isValid = 1;
						prcMonInfo.prcID = pe.th32ProcessID;
						tmpLen = strlen(pe.szExeFile);
						prcMonInfo.prcName = (char*)malloc(tmpLen + 1);
						if(prcMonInfo.prcName)
						{
							memset(prcMonInfo.prcName, 0, tmpLen + 1);
							memcpy(prcMonInfo.prcName, pe.szExeFile, tmpLen);
						}
						tmpLen = strlen(procUserName);
						if(tmpLen)
						{
							prcMonInfo.ownerName = (char*)malloc(tmpLen + 1);
							if(prcMonInfo.ownerName)
							{
								memset(prcMonInfo.ownerName, 0, tmpLen + 1);
								memcpy(prcMonInfo.ownerName, procUserName, tmpLen);
							}
						}
						app_os_EnterCriticalSection(&pSoftwareMonHandlerContext->swPrcMonCS);
						InsertPrcMonInfoNode(head, &prcMonInfo);
						app_os_LeaveCriticalSection(&pSoftwareMonHandlerContext->swPrcMonCS);
						if(prcMonInfo.prcName)
						{
							free(prcMonInfo.prcName);
							prcMonInfo.prcName = NULL;
						}
						if(prcMonInfo.ownerName)
						{
							free(prcMonInfo.ownerName);
							prcMonInfo.ownerName = NULL;
						}
						*bChangeFlag = TRUE;
						m_cntProcChanged++;
					}
				}
				//if(procHandle) app_os_CloseHandle(procHandle);
				app_os_sleep(10);
				pe.dwSize=sizeof(PROCESSENTRY32);
				if(app_os_Process32Next(hSnapshot,&pe)==FALSE)
					break;
			}
		}
		if(hSnapshot) app_os_CloseSnapShot32Handle(hSnapshot);//app_os_CloseHandle(hSnapshot);//Wei.Gang modified
	}
	//else
	//	pSoftwareMonHandlerContext->isUserLogon = FALSE;
	}
}

static BOOL UpdateLogonUserPrcList(prc_mon_info_node_t * head)
{
	sw_mon_handler_context_t *pSoftwareMonHandlerContext = (sw_mon_handler_context_t *)&SWMonHandlerContext;
	BOOL bRet = FALSE;
	BOOL bChangeFlag = FALSE;
	if(NULL == head) return bRet;
	{
		app_os_EnterCriticalSection(&pSoftwareMonHandlerContext->swPrcMonCS);
		ResetPrcList(head);
		app_os_LeaveCriticalSection(&pSoftwareMonHandlerContext->swPrcMonCS);
//#ifdef WIN32
//		if(!IsWorkStationLocked() && !ProcessCheck("LogonUI.exe"))
//#else
//		if(!IsWorkStationLocked())
//#endif
		{
		switch(pSoftwareMonHandlerContext->gatherLevel)
		{
			case CFG_FLAG_SEND_PRCINFO_ALL:
				UpdatePrcList_All(head, pSoftwareMonHandlerContext,&bChangeFlag);
				break;
			case CFG_FLAG_SEND_PRCINFO_BY_USER:
				UpdatePrcList_CheckUser(head, pSoftwareMonHandlerContext, pSoftwareMonHandlerContext->sysUserName,&bChangeFlag);
				break;
			default://CFG_FLAG_SEND_PRCINFO_BY_ALLUSERS:
				UpdatePrcList_CheckAllUsers(head, pSoftwareMonHandlerContext,&bChangeFlag);
				break;
		}
		}
//#ifdef WIN32
		//if(!IsWorkStationLocked() && !ProcessCheck("LogonUI.exe"))
		//else
		//	pSoftwareMonHandlerContext->isUserLogon = FALSE;
//#endif
		app_os_EnterCriticalSection(&pSoftwareMonHandlerContext->swPrcMonCS);
		if(UpdatePrcList(head))
			bChangeFlag = TRUE;
		app_os_LeaveCriticalSection(&pSoftwareMonHandlerContext->swPrcMonCS);
		bRet = TRUE;
		if(bChangeFlag)
		{
			clock_t tmpTimeNow = clock();
			if(tmpTimeNow - m_startTime > SPEC_INFO_TIME_OUT_MS || m_cntProcChanged > MAX_PROC_CHANGED_CNT)
			{
				UploadSysMonInfo(swm_send_infospec_cbf,unknown_cmd);
				m_startTime = clock();
				m_cntProcChanged = 0;
			}
		}
	}
	return bRet;
}


#ifdef WIN32
BOOL GetTokenByName(HANDLE * hToken, char * prcName)
{
	BOOL bRet = FALSE;
	HANDLE hPrc = NULL;
	PROCESSENTRY32 pe;
	HANDLE hSnapshot=NULL;
	if(NULL == prcName || NULL == hToken) return bRet;
	hSnapshot=app_os_CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS,0);
	pe.dwSize=sizeof(PROCESSENTRY32);
	if(!app_os_Process32First(hSnapshot,&pe))
		return bRet;
	while(TRUE)
	{
		pe.dwSize=sizeof(PROCESSENTRY32);
		if(app_os_Process32Next(hSnapshot,&pe)==FALSE)
			break;
		if(stricmp(pe.szExeFile, prcName)==0)
		{	
			hPrc = app_os_OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, pe.th32ProcessID);
			bRet = app_os_OpenProcessToken(hPrc,TOKEN_ALL_ACCESS,hToken);
			app_os_CloseHandle(hPrc);
			break;
		}
	}
	if(hSnapshot) app_os_CloseHandle(hSnapshot);
	return bRet;
}

BOOL RunProcessAsUser(char * cmdLine, BOOL isAppNameRun, BOOL isShowWindow, DWORD * newPrcID)
{
	BOOL bRet = FALSE;
	if(NULL == cmdLine) return bRet;
	{
		HANDLE hToken;
		if(!GetTokenByName(&hToken,"EXPLORER.EXE"))
		{
			return bRet;
		}
		else
		{
			STARTUPINFO si;
			PROCESS_INFORMATION pi;
			DWORD dwCreateFlag = CREATE_NO_WINDOW;
			memset(&si, 0, sizeof(si));
			si.dwFlags = STARTF_USESHOWWINDOW; 
			si.wShowWindow = SW_HIDE;
			if(isShowWindow)
			{
				si.wShowWindow = SW_SHOW;
				dwCreateFlag = CREATE_NEW_CONSOLE;
			}
			si.cb = sizeof(si);
			memset(&pi, 0, sizeof(pi));
			if(isAppNameRun)
			{
				bRet = app_os_CreateProcessAsUserA(hToken, cmdLine, NULL,  NULL ,NULL,
					FALSE, dwCreateFlag, NULL, NULL, &si, &pi);
			}
			else
			{
				bRet = app_os_CreateProcessAsUserA(hToken, NULL, cmdLine, NULL ,NULL,
					FALSE, dwCreateFlag, NULL, NULL, &si, &pi);
			}

			if(!bRet)
			{
				printf("error code: %s  %d\n", cmdLine, app_os_GetLastError());
			}
			else
			{
				if(newPrcID != NULL) *newPrcID = pi.dwProcessId;
			}
			app_os_CloseHandle(hToken);
		}
	}
	return bRet;
}
#else
BOOL RunProcessAsUser(char * cmdLine, BOOL isAppNameRun, BOOL isShowWindow, DWORD * newPrcID)
{
	if(!cmdLine) return FALSE;

	FILE *fp = NULL;
	char cmdBuf[256];
	char logonUserName[32] = {0};
	if(app_os_GetSysLogonUserName(logonUserName, sizeof(logonUserName)))
	{
		//sprintf(cmdBuf,"su - %s -c %s &",logonUserName,cmdLine);
		//sprintf(cmdBuf,"DISPLAY=:0 su -c %s %s &",cmdLine,logonUserName);
		sprintf(cmdBuf,"DISPLAY=:0 su -c 'xterm -e /bin/bash -c '%s'' %s &",cmdLine,logonUserName);
		if((fp=popen(cmdBuf,"r"))==NULL)
		{
			//printf("restart process failed,%s",cmdBuf);
			pclose(fp);
			return FALSE;	
		}
		pclose(fp);
	}
	if(newPrcID != NULL)
		*newPrcID = getPIDByName(cmdLine);
	return TRUE;
}
#endif

#ifdef WIN32
BOOL ConverNativePathToWin32(char * nativePath, char * win32Path)
{
	BOOL bRet = FALSE;
	if(NULL == nativePath || NULL == win32Path) return bRet;
	{
		char drv = 'A';
		char devName[3] = {drv, ':', '\0'};
		char tmpDosPath[MAX_PATH] = {0};
		while( drv <= 'Z')
		{
			devName[0] = drv;
			memset(tmpDosPath, 0, sizeof(tmpDosPath));
			if(app_os_QueryDosDeviceA(devName, tmpDosPath, sizeof(tmpDosPath) - 1)!=0)
			{
				if(strstr(nativePath, tmpDosPath))
				{
					strcat(win32Path, devName);
					strcat(win32Path, nativePath + strlen(tmpDosPath));
					bRet = TRUE;
					break;
				}
			}
			drv++;
		}
	}
	return bRet;
}

DWORD RestartProcessWithID(DWORD prcID)
{
	DWORD dwPrcID = 0;
	HANDLE hPrc = NULL;
	PROCESSENTRY32 pe;
	HANDLE hSnapshot=NULL;
	hSnapshot=app_os_CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS,0);
	pe.dwSize=sizeof(PROCESSENTRY32);
	if(!app_os_Process32First(hSnapshot,&pe))
	{
		app_os_CloseHandle(hSnapshot);
		return dwPrcID;
	}
	while(TRUE)
	{
		pe.dwSize=sizeof(PROCESSENTRY32);
		if(app_os_Process32Next(hSnapshot,&pe)==FALSE)
			break;
		if(pe.th32ProcessID == prcID)
		{
			hPrc = app_os_OpenProcess(PROCESS_ALL_ACCESS, FALSE, pe.th32ProcessID);

			if(hPrc == NULL) 
			{
				DWORD dwRet = app_os_GetLastError();          
				if(dwRet == 5)
				{
					if(AdjustPrivileges())
					{
						hPrc = app_os_OpenProcess(PROCESS_ALL_ACCESS, FALSE, pe.th32ProcessID);
					}
				}
			}
			if(hPrc)
			{
				char nativePath[MAX_PATH] = {0};
				char win32Path[MAX_PATH] = {0};
				if(app_os_GetProcessImageFileNameA(hPrc, nativePath, sizeof(nativePath)))
				{
					if(ConverNativePathToWin32(nativePath, win32Path))
					{              
						app_os_TerminateProcess(hPrc, 0);    
						{
							char cmdLine[BUFSIZ] = {0};
							DWORD tmpPrcID = 0;
							sprintf(cmdLine, "%s", win32Path);
							//sprintf(cmdLine, "%s \"%s\"", "cmd.exe /c ", path);
							if(RunProcessAsUser(cmdLine, TRUE, TRUE, &tmpPrcID))
								//if(CreateProcess(cmdLine, NULL, NULL, NULL, FALSE, CREATE_NO_WINDOW , NULL, NULL, &si, &pi))
								//if(CreateProcess(NULL, cmdLine, NULL, NULL, FALSE, CREATE_NO_WINDOW , NULL, NULL, &si, &pi))
							{
								dwPrcID = tmpPrcID;
							}
						}
					}
				}
				app_os_CloseHandle(hPrc);            
			}
			break;
		}
	}
	if(hSnapshot) app_os_CloseHandle(hSnapshot);
	return dwPrcID;
}


BOOL KillProcessWithID(DWORD prcID)
{
	BOOL bRet = FALSE;
	PROCESSENTRY32 pe;
	HANDLE hSnapshot=NULL;
	hSnapshot=app_os_CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS,0);
	pe.dwSize=sizeof(PROCESSENTRY32);
	if(!app_os_Process32First(hSnapshot,&pe))
	{
		app_os_CloseHandle(hSnapshot);
		return bRet;
	}
	while(TRUE)
	{
		pe.dwSize=sizeof(PROCESSENTRY32);
		if(app_os_Process32Next(hSnapshot,&pe)==FALSE)
			break;
		if(pe.th32ProcessID == prcID)
		{
			HANDLE hPrc = app_os_OpenProcess(PROCESS_ALL_ACCESS, FALSE, pe.th32ProcessID);
			if(hPrc == NULL) 
			{
				DWORD dwRet = app_os_GetLastError();          
				if(dwRet == 5)
				{
					if(AdjustPrivileges())
					{
						hPrc = app_os_OpenProcess(PROCESS_ALL_ACCESS, FALSE, pe.th32ProcessID);
					}
				}
			}

			if(hPrc)
			{
				app_os_TerminateProcess(hPrc, 0);    //asynchronous
				bRet = TRUE;
				app_os_CloseHandle(hPrc);
			}

			break;
		}
	}
	app_os_CloseHandle(hSnapshot);
	return bRet;
}
#else
DWORD RestartProcessWithID(DWORD prcID)
{
	if(!prcID) return 0;
	DWORD dwPrcID = 0;
    	FILE* fp = NULL;
    	char cmdLine[256];
	char cmdBuf[300];
    	char file[128] = {0};
    	char buf[BUFF_LEN] = {0};
	//unsigned int pid = 0;
    	sprintf(file,"/proc/%d/cmdline",prcID);
    	if (!(fp = fopen(file,"r")))
    	{
        	printf("read %s file fail!\n",file);
        	return dwPrcID;
    	}
    	//if (read_line(fp,line_buff,BUFF_LEN,1))
	if(fgets(buf,sizeof(buf),fp))
    	{
        	sscanf(buf,"%s",cmdLine);
			fclose(fp);
			printf("cmd line is %s\n",cmdLine);
    	}
	else 
	{
		fclose(fp);
		return dwPrcID;
	}

	if(kill(prcID,SIGKILL)!=0) 
		return dwPrcID;
	//waitpid();
	char logonUserName[32] = {0};
	if(app_os_GetSysLogonUserName(logonUserName, sizeof(logonUserName)))
	{
		app_os_sleep(10);
		fp = NULL;
		//sprintf(cmdBuf,"su - %s -c %s &",logonUserName,cmdLine);
		//sprintf(cmdBuf,"DISPLAY=:0 su -c %s %s &",cmdLine,logonUserName);
		sprintf(cmdBuf,"DISPLAY=:0 su -c 'xterm -e /bin/bash -c '%s'' %s &",cmdLine,logonUserName);
		if((fp=popen(cmdBuf,"r"))==NULL)
		{
			printf("restart process failed,%s",cmdBuf);
			pclose(fp);
			return dwPrcID;	
		}
		pclose(fp);
	}
	else
		printf("restart process failed,%s",cmdBuf);
	dwPrcID = getPIDByName(cmdLine);
	return dwPrcID;
}

BOOL KillProcessWithID(DWORD prcID)
{
	if(!prcID) return FALSE;
	if(kill(prcID,SIGKILL)!=0) 
		return FALSE;
	return TRUE;
}
#endif
static BOOL PrcMonOnEvent(mon_obj_info_t * curMonObjInfo, prc_thr_type_t prcThrType)
{
	BOOL bRet = FALSE;
	if(curMonObjInfo == NULL) return bRet;
	{
		prc_action_t prcAct = prc_act_unknown;
		char prcActCmd[128] = {0};

		switch(prcThrType)
		{
		case prc_thr_type_active:
			/*{
				if(curMonObjInfo->act != prc_act_unknown)
				{
					prcAct = curMonObjInfo->act;
					if(curMonObjInfo->actCmd)
					{
						strcpy(prcActCmd, curMonObjInfo->actCmd);
					}
				}
				else
				{
					prcAct = curMonObjInfo->memAct;
					if(curMonObjInfo->memActCmd)
					{
						strcpy(prcActCmd, curMonObjInfo->memActCmd);
					}
				}
				break;
			}
		case prc_thr_type_cpu:
			{
				prcAct = curMonObjInfo->cpuAct;
				if(curMonObjInfo->cpuActCmd)
				{
					strcpy(prcActCmd, curMonObjInfo->cpuActCmd);
				}
				break;
			}
		case prc_thr_type_mem:*/
			{
				if(curMonObjInfo->act != prc_act_unknown){
					prcAct = curMonObjInfo->act;
					if(curMonObjInfo->actCmd)
					{
						strcpy(prcActCmd, curMonObjInfo->actCmd);
					}
				}
				break;
			}
		//Wei.Gang add for delete warning
		case prc_thr_type_unknown:
			break;
		default:
			break;
		//Wei.Gang add end
		}

		switch(prcAct)
		{
		case prc_act_stop:
			{
				bRet = KillProcessWithID(curMonObjInfo->prcID);
				break;
			}
		case prc_act_restart:
			{
				DWORD dwPrcID = 0;
				dwPrcID = RestartProcessWithID(curMonObjInfo->prcID);
				if(dwPrcID) 
				{
					curMonObjInfo->prcID = dwPrcID;
					//ClearMonObjThr(curMonObjInfo);  //don't clear, will Delete in next circle.
					bRet = TRUE;
				}
				break;
			}
		case prc_act_with_cmd:
			{
				if(strlen(prcActCmd) > 0 && strcmp(prcActCmd, "None"))
				{
#ifdef WIN32
					char realCmdLine[512] = {0};
					sprintf_s(realCmdLine, sizeof(realCmdLine), "%s \"%s\"", "cmd.exe /c ", prcActCmd);
					if(RunProcessAsUser(realCmdLine, FALSE, FALSE, NULL))
#else
					if(RunProcessAsUser(prcActCmd, FALSE, FALSE, NULL))
#endif
					{
						bRet = TRUE;
					}
				}
				break;
			}
		//Wei.Gang add to void warning
		case prc_act_unknown:
			break;
		//Wei.Gang add end
		}
	}
	return bRet;
}

static BOOL GetPrcMonEventMsg(sw_thr_check_params_t * pCurThrCheckParams, prc_thr_type_t prcThrType, BOOL checkRet, char * eventMsg, int eventMsgLen)
{
	BOOL bRet = FALSE;
	char eventStr[256] = {0};
	prc_mon_info_t * curPrcMonInfo = NULL;
	mon_obj_info_t * curMonObjInfo = NULL;
	prc_action_t prcAct = prc_act_unknown;
	char prcActCmd[128] = {0};
	//sw_mon_handler_context_t *pSoftwareMonHandlerContext = (sw_mon_handler_context_t *)&SWMonHandlerContext;
	if(pCurThrCheckParams == NULL || eventStr == NULL) return bRet;
	curPrcMonInfo = pCurThrCheckParams->pPrcMonInfo; 
	curMonObjInfo = pCurThrCheckParams->pMonObjInfo;
	if(curPrcMonInfo == NULL || curMonObjInfo == NULL || curMonObjInfo->prcID != curPrcMonInfo->prcID) return bRet;
/*	sprintf_s(eventStr, sizeof(eventStr), "Process(PrcName:%s, PrcID:%d) CpuUsage:%d, MemUsage:%ld", curPrcMonInfo->prcName, curMonObjInfo->prcID, 
		curPrcMonInfo->cpuUsage, curPrcMonInfo->memUsage);
	if(curPrcMonInfo->isActive)
	{
		sprintf_s(eventStr, sizeof(eventStr), "%s, PrcStatus:Running", eventStr);
	}
	else
	{
		sprintf_s(eventStr, sizeof(eventStr), "%s, PrcStatus:NoResponse", eventStr);
	}
*/
	/*
	if(strlen(pCurThrCheckParams->checkMsg))
	{
		//sprintf_s(eventStr, sizeof(eventStr), "%s (%s)", eventStr, pCurThrCheckParams->checkMsg);
		sprintf_s(eventStr, sizeof(eventStr), "%s %s", curMonObjInfo->path, pCurThrCheckParams->checkMsg);
	}
	*/
	if(checkRet)
	{
		switch(prcThrType)
		{
		case prc_thr_type_active:
			{
				if(curMonObjInfo->act != prc_act_unknown)
				{
					prcAct = curMonObjInfo->act;
					if(curMonObjInfo->actCmd)
					{
						strcpy(prcActCmd, curMonObjInfo->actCmd);
					}
				}
				break;
			}
		case prc_thr_type_unknown:
			{
				//break;
			}
		}

		switch(prcAct)
		{
		case prc_act_stop:
			{
				sprintf_s(eventStr, sizeof(eventStr), "%s, #tk#will stop#tk#",curMonObjInfo->path);
				break;
			}
		case prc_act_restart:
			{
				sprintf_s(eventStr, sizeof(eventStr), "%s, #tk#will restart#tk#",curMonObjInfo->path);
				break;
			}
		case prc_act_with_cmd:
			{
				sprintf_s(eventStr, sizeof(eventStr), "%s, #tk#will execute cmd#tk#: %s",curMonObjInfo->path,prcActCmd);
				break;
			}
		case prc_act_unknown:
			{
				//break;
			}
		}
	}

	if(strlen(eventStr))
	{
		//strcpy(eventMsg, eventStr);
		if(eventMsg && strlen(eventMsg))
			sprintf_s(eventMsg,eventMsgLen,"%s, %s",eventMsg,eventStr);
		else
			sprintf_s(eventMsg,eventMsgLen,"%s",eventStr);
		return bRet = TRUE;
	}
	return bRet = FALSE;
}

int DynamicStrCat(char *pBuf, int *bufTotalLen, char *pSeparator ,char *pSrcStr)
{
	int bufStrLen = 0;
	int srcStrLen = 0;
	int bufNewLen = 0;
	int sepLen = 0;
	char * pDestNewBuf = NULL;
	if (pSrcStr)
	{
		srcStrLen = strlen(pSrcStr) + 1;
		if(pBuf)
			bufStrLen = strlen(pBuf) + 1;
		if(pSeparator)
			sepLen = strlen(pSeparator) + 1;
		bufNewLen = srcStrLen + bufStrLen + sepLen + sizeof(LONG);//sizeof(LONG)*3;
		if(bufNewLen >= *bufTotalLen) 
		{
			*bufTotalLen = *bufTotalLen + bufNewLen + sizeof(LONG)*512;
			pDestNewBuf = (char *)malloc(*bufTotalLen);
			if(pDestNewBuf)
			{
				memset(pDestNewBuf,0,sizeof(char)*(*bufTotalLen));
				if(pBuf)
				{
					sprintf_s(pDestNewBuf, sizeof(char)*(*bufTotalLen), "%s%s%s", pBuf, pSeparator, pSrcStr); //Will show "Buffer too small" occasionally. Have no reasion. 
					//sprintf((char *)*ppDestStr,"%s%s%s", pTmpBuf, pSeparator, pSrcStr);
					free(pBuf);
				}
				else
					sprintf_s(pDestNewBuf, sizeof(char)*(*bufTotalLen), "%s", pSrcStr); 
			}
		}
		else
		{
			sprintf_s(pBuf, sizeof(char)*(*bufTotalLen), "%s%s%s", pBuf, pSeparator, pSrcStr); 
			pDestNewBuf = pBuf;
		}
	}
	return pDestNewBuf;
}

static BOOL PrcMonInfoAnalyze()
{
	BOOL bRet = FALSE;
	sw_mon_handler_context_t *pSoftwareMonHandlerContext = (sw_mon_handler_context_t *)&SWMonHandlerContext;
	if(pSoftwareMonHandlerContext == NULL || pSoftwareMonHandlerContext->prcMonInfoList == NULL || 
		pSoftwareMonHandlerContext->monObjInfoList == NULL) return bRet;
	{
		int repEventMsgLen = 0;
		//char repEventMsg[2*1024] = {0};
		char *pRepEventMsg = NULL;
		int repEventBufLen = 0;
		char tmpEventMsg[512] = {0};
		int repActMsgLen = 0;
		//char repActMsg[2*1024] = {0};
		char *pRepActMsg = NULL;
		int repActBufLen = 0;
		char tmpActMsg[512] = {0};
		//int testCnt = 0;	//Wei.Gang add for test
		mon_obj_info_node_t * curMonObjInfoNode = pSoftwareMonHandlerContext->monObjInfoList->next;
		prc_mon_info_node_t * curPrcMonInfoNode = NULL;
		BOOL isPrcMonInfoFind = FALSE;
		app_os_EnterCriticalSection(&pSoftwareMonHandlerContext->swMonObjCS);
		while(curMonObjInfoNode)
		{
			//++testCnt;   //Wei.gang add for test
			memset(tmpEventMsg, 0, sizeof(tmpEventMsg));
			memset(tmpActMsg, 0, sizeof(tmpActMsg));
			if(!pSoftwareMonHandlerContext->isSoftwareMonThreadRunning) break;
			if(curMonObjInfoNode->monObjInfo.thrItem.isEnable) //|| curMonObjInfoNode->monObjInfo.memThrItem.isEnable)
			{
				if(curMonObjInfoNode->monObjInfo.isValid == 1)
				{
					if(curMonObjInfoNode->monObjInfo.prcID == THR_SYSINFO_FLAG 
					&& !strcmp(curMonObjInfoNode->monObjInfo.prcName, SWM_SYS_MON_INFO))
					{// system info threshold node
						prc_mon_info_t sysMonInfo;
						sw_thr_check_params_t swThrCheckParams;
						BOOL checkRet = FALSE;
						prc_thr_type_t prcThrType = prc_thr_type_unknown;
						memset(&swThrCheckParams, 0, sizeof(sw_thr_check_params_t));
						{
							int tmpLen = strlen(SWM_SYS_MON_INFO) + 1;
							sysMonInfo.prcID = THR_SYSINFO_FLAG;
							sysMonInfo.prcName = (char *)malloc(tmpLen);
							if(sysMonInfo.prcName)
							{
								memset(sysMonInfo.prcName, 0, tmpLen);
								strcpy(sysMonInfo.prcName, SWM_SYS_MON_INFO);
							}
							sysMonInfo.cpuUsage = pSoftwareMonHandlerContext->sysMonInfo.cpuUsage;
							sysMonInfo.memUsage = pSoftwareMonHandlerContext->sysMonInfo.totalPhysMemoryKB
												  - pSoftwareMonHandlerContext->sysMonInfo.availPhysMemoryKB;
							sysMonInfo.isActive = 1;
							sysMonInfo.isValid = 1;
							//sysMonInfo.prcCpuUsageLastTimes = pSoftwareMonHandlerContext->sysMonInfo.sysCpuUsageLastTimes;
						}
						swThrCheckParams.pPrcMonInfo = &sysMonInfo; //&curPrcMonInfoNode->prcMonInfo;
						swThrCheckParams.pMonObjInfo = &curMonObjInfoNode->monObjInfo; 
						prcThrType = MonObjThrCheck(&swThrCheckParams, &checkRet);
						if(prcThrType != prc_thr_type_unknown)
						{
							BOOL bRet = FALSE;
							if(strlen((&swThrCheckParams)->checkMsg))
							{
								char eventStr[256] = {0};
								sprintf_s(eventStr, sizeof(eventStr), "%s %s", (&swThrCheckParams)->pMonObjInfo->path, (&swThrCheckParams)->checkMsg);
								if(strlen(eventStr))
								{
									strcpy(tmpEventMsg, eventStr);
								}
							}
							bRet = GetPrcMonEventMsg(&swThrCheckParams, prcThrType, checkRet, tmpActMsg, sizeof(tmpActMsg));
							if(bRet)
							{
								if(checkRet)
								{
									//isSWMThrNormal = FALSE;
									bRet = PrcMonOnEvent(&curMonObjInfoNode->monObjInfo, prcThrType);
									if(!bRet)
									{
										if((prcThrType == prc_thr_type_active && (curMonObjInfoNode->monObjInfo.act > prc_act_unknown && curMonObjInfoNode->monObjInfo.act<=prc_act_with_cmd))) 
										{
											sprintf_s(tmpActMsg, sizeof(tmpActMsg), "%s,#tk#Action#tk#: False", tmpActMsg);
										}
									}
									else
									{
										if((prcThrType == prc_thr_type_active && (curMonObjInfoNode->monObjInfo.act > prc_act_unknown && curMonObjInfoNode->monObjInfo.act<=prc_act_with_cmd)))
										{
											sprintf_s(tmpActMsg, sizeof(tmpActMsg), "%s,#tk#Action#tk#: True", tmpActMsg);
										}
									}
								}
							}
							//else
							//{
							//	sprintf_s(repMsg, sizeof(repMsg), "%s #tk#threshold is reached, but report event msg failed#tk#!", curMonObjInfoNode->monObjInfo.path);
							//}
						}
						if(sysMonInfo.prcName)
								free(sysMonInfo.prcName);
					}
				    else
					{  //process info threshold node
						if(curMonObjInfoNode->monObjInfo.prcID == THR_PRC_SAME_NAME_ALL_FLAG)
						{
							curMonObjInfoNode = curMonObjInfoNode->next;
							continue;
						}
						isPrcMonInfoFind = FALSE;
						curPrcMonInfoNode = pSoftwareMonHandlerContext->prcMonInfoList->next;
						while(curPrcMonInfoNode)
						{
							app_os_EnterCriticalSection(&pSoftwareMonHandlerContext->swPrcMonCS);
							if(!strcmp(curPrcMonInfoNode->prcMonInfo.prcName, curMonObjInfoNode->monObjInfo.prcName) &&
								curPrcMonInfoNode->prcMonInfo.prcID == curMonObjInfoNode->monObjInfo.prcID)
							{
								isPrcMonInfoFind = TRUE;
								if(curPrcMonInfoNode)
								{
									sw_thr_check_params_t swThrCheckParams;
									BOOL checkRet = FALSE;
									prc_thr_type_t prcThrType = prc_thr_type_unknown;
									memset(&swThrCheckParams, 0, sizeof(sw_thr_check_params_t));
									swThrCheckParams.pPrcMonInfo = &curPrcMonInfoNode->prcMonInfo;
									swThrCheckParams.pMonObjInfo = &curMonObjInfoNode->monObjInfo; 
									prcThrType = MonObjThrCheck(&swThrCheckParams, &checkRet);
									if(prcThrType != prc_thr_type_unknown)
									{
										BOOL bRet = FALSE;
										if(strlen((&swThrCheckParams)->checkMsg))
										{
											char eventStr[256] = {0};
											sprintf_s(eventStr, sizeof(eventStr), "%s %s", (&swThrCheckParams)->pMonObjInfo->path, (&swThrCheckParams)->checkMsg);
											if(strlen(eventStr))
											{
												strcpy(tmpEventMsg, eventStr);
											}
										}
										bRet = GetPrcMonEventMsg(&swThrCheckParams, prcThrType, checkRet, tmpActMsg, sizeof(tmpActMsg));
										if(bRet)
										{
											if(checkRet)
											{
												//isSWMThrNormal = FALSE;
												bRet = PrcMonOnEvent(&curMonObjInfoNode->monObjInfo, prcThrType);
												if(!bRet)
												{
													if((prcThrType == prc_thr_type_active && (curMonObjInfoNode->monObjInfo.act > prc_act_unknown && curMonObjInfoNode->monObjInfo.act<=prc_act_with_cmd))) 
													{
														sprintf_s(tmpActMsg, sizeof(tmpActMsg), "%s,#tk#Action#tk#: False", tmpActMsg);
													}
												}
												else
												{
													if((prcThrType == prc_thr_type_active && (curMonObjInfoNode->monObjInfo.act > prc_act_unknown && curMonObjInfoNode->monObjInfo.act<=prc_act_with_cmd)))
													{
														sprintf_s(tmpActMsg, sizeof(tmpActMsg), "%s,#tk#Action#tk#: True", tmpActMsg);
													}
												}
											}
										}
										//else
										//{
										//	sprintf_s(repMsg, sizeof(repMsg), "%s #tk#threshold is reached, but report event msg failed#tk#!", curMonObjInfoNode->monObjInfo.path);
										//}
									}
								}
							}
							curPrcMonInfoNode = curPrcMonInfoNode->next;
							app_os_LeaveCriticalSection(&pSoftwareMonHandlerContext->swPrcMonCS);
							app_os_sleep(10);
						}

						if(!isPrcMonInfoFind)
						{
							/*if(curMonObjInfoNode->monObjInfo.isValid == 1)
							{
								curMonObjInfoNode->monObjInfo.isValid = 0;
								//sprintf_s(repMsg, sizeof(repMsg), "Process(PrcName-%s) is not found, the monobj invalid!", curMonObjInfoNode->monObjInfo.prcName);
							}
							{//delete invalid mon obj(but must keep one<same prcName> exist)
								mon_obj_info_node_t * tmpMonObjInfoNode = pSoftwareMonHandlerContext->monObjInfoList->next;
								while(tmpMonObjInfoNode)
								{
									if(!strcmp(tmpMonObjInfoNode->monObjInfo.prcName, curMonObjInfoNode->monObjInfo.prcName) &&
										tmpMonObjInfoNode->monObjInfo.prcID == curMonObjInfoNode->monObjInfo.prcID && 
										!strcmp(tmpMonObjInfoNode->monObjInfo.thrItem.tagName, curMonObjInfoNode->monObjInfo.thrItem.tagName)) break;
									tmpMonObjInfoNode = tmpMonObjInfoNode->next;
								}
								if(tmpMonObjInfoNode)
								{*/
									int delPrcId = curMonObjInfoNode->monObjInfo.prcID;
									mon_obj_info_node_t * tmpMonObjInfoNode = curMonObjInfoNode;
									curMonObjInfoNode = curMonObjInfoNode->next;
									// Don't send 'is not exist' message
									/*if(strlen(repMsg))
										sprintf_s(repMsg, sizeof(repMsg), "%s; Process(PrcName:%s, PrcID:%d) #tk# is not exist#tk#", repMsg,tmpMonObjInfoNode->monObjInfo.prcName, 
											tmpMonObjInfoNode->monObjInfo.prcID);//, tmpMonObjInfoNode->monObjInfo.thrItem.tagName);
									else
										sprintf_s(repMsg, sizeof(repMsg), "Process(PrcName:%s, PrcID:%d) #tk# is not exist#tk#", tmpMonObjInfoNode->monObjInfo.prcName, 
											tmpMonObjInfoNode->monObjInfo.prcID); //, tmpMonObjInfoNode->monObjInfo.thrItem.tagName);
									*/
									DeleteMonObjInfoNodeWithID(pSoftwareMonHandlerContext->monObjInfoList, delPrcId,
									                       tmpMonObjInfoNode->monObjInfo.thrItem.tagName);
									continue;
								//}
							//}
						}	
					}//else end
				}//
			}
			if(strlen(tmpEventMsg))
			{
				pRepEventMsg = (char *)DynamicStrCat(pRepEventMsg, &repEventBufLen, ";",tmpEventMsg);
				//int tmpEventMsgLen = strlen(tmpEventMsg) + 1;
				/*if(pRepEventMsg)
					repEventMsgLen = strlen(pRepEventMsg) + 1;
				if(repEventMsgLen > 1)
				{
					DynamicStrCat(&pRepEventMsg,";",tmpEventMsg);
					//int newEventMsgLen = tmpEventMsgLen + repEventMsgLen;
					//int tmpBufLen = repEventMsgLen;
					//char * pTmpBuf = (char *)malloc(tmpBufLen);
					//memset(pTmpBuf,0,sizeof(char)*tmpBufLen);
					//sprintf_s(pTmpBuf, sizeof(char)*tmpBufLen, "%s", pRepEventMsg);

					//if(pRepEventMsg)free(pRepEventMsg);
					//pRepEventMsg = (char *)malloc(newEventMsgLen);
					//memset(pRepEventMsg,0,sizeof(char)*newEventMsgLen);
					//sprintf_s(pRepEventMsg, sizeof(char)*newEventMsgLen, "%s;%s", pTmpBuf, tmpEventMsg);
					//if(pTmpBuf) free(pTmpBuf);
				}
				else
				{
					int tmpEventMsgLen = strlen(tmpEventMsg) + 1;
					pRepEventMsg = (char *)malloc(tmpEventMsgLen);
					if(pRepEventMsg)
					{
						memset(pRepEventMsg, 0, sizeof(char)*tmpEventMsgLen);
						sprintf_s(pRepEventMsg, sizeof(char)*tmpEventMsgLen, "%s", tmpEventMsg);
					}
				}*/
			}

			if(strlen(tmpActMsg))
			{
				pRepActMsg = (char *)DynamicStrCat(pRepActMsg, &repActBufLen, ";",tmpActMsg);
				//int tmpActMsgLen = strlen(tmpActMsg) + 1;
				/*if(pRepActMsg)
					repActMsgLen = strlen(pRepActMsg) + 1;
				if(repActMsgLen > 1)
				{
					DynamicStrCat(&pRepActMsg, ";", tmpActMsg);
					//int newRepActMesgLen = tmpActMsgLen + repActMsgLen;
					//int tmpBufLen = repActMsgLen;
					//char *pTmpBuf = (char*)malloc(tmpBufLen);
					//memset(pTmpBuf,0,sizeof(char)*tmpBufLen);
					//sprintf_s(pTmpBuf, sizeof(char)*tmpBufLen, "%s", pRepActMsg);

					//if(pRepActMsg) free(pRepActMsg);
					//pRepActMsg = (char *)malloc(newRepActMesgLen);
					//memset(pRepActMsg,0,sizeof(char)*newRepActMesgLen);
					//sprintf_s(pRepActMsg, sizeof(char)*newRepActMesgLen, "%s;%s", pTmpBuf, tmpActMsg);
					//if(pTmpBuf) free(pTmpBuf);
				}
				else
				{
					int tmpActMsgLen = strlen(tmpActMsg) + 1;
					pRepActMsg = (char *)malloc(tmpActMsgLen);
					if(pRepActMsg)
					{
						memset(pRepActMsg, 0, sizeof(char)*tmpActMsgLen);
						sprintf_s(pRepActMsg, sizeof(char)*tmpActMsgLen, "%s",tmpActMsg);
					}
				}*/
			}
			app_os_sleep(10);
			curMonObjInfoNode = curMonObjInfoNode->next;
		}
		app_os_LeaveCriticalSection(&pSoftwareMonHandlerContext->swMonObjCS);
		
		if(pRepEventMsg)
			repEventMsgLen = strlen(pRepEventMsg)+1;
		//if(strlen(repMsg))
		if(repEventMsgLen > 1)
		{
			BOOL isSWMNormal = TRUE;
			//int repActMsgLen = 0;
			if(IsSWMThrNormal(&isSWMNormal))
			{
				swm_thr_rep_info_t thrRepInfo;
				thrRepInfo.repInfo = NULL;
				thrRepInfo.isTotalNormal = isSWMNormal;
				//if(!thrRepInfo.repInfo && repMsgLen > 1)
				{
					thrRepInfo.repInfo = (char *)malloc(repEventMsgLen);
					if(thrRepInfo.repInfo)
					{
						memset(thrRepInfo.repInfo, 0, sizeof(char)*repEventMsgLen);
						sprintf_s(thrRepInfo.repInfo, sizeof(char)*repEventMsgLen, "%s", pRepEventMsg);
					}
					if(pRepEventMsg) free(pRepEventMsg);
				}
				//memset(thrRepInfo.repInfo, 0, sizeof(thrRepInfo.repInfo));
				//strcpy(thrRepInfo.repInfo, repMsg);
				{
					char * uploadRepJsonStr = NULL;
					char * str = (char *)&thrRepInfo;
					int jsonStrlen = Parser_swm_mon_prc_event_rep(str, &uploadRepJsonStr);
					//int jsonStrlen = Parser_swm_mon_prc_action_rep(str, &uploadRepJsonStr);
					if(jsonStrlen > 0 && uploadRepJsonStr != NULL)
					{
						g_sendcbf(&g_PluginInfo, swm_mon_prc_event_rep, uploadRepJsonStr, strlen(uploadRepJsonStr)+1, NULL, NULL);
					}
					if(uploadRepJsonStr)free(uploadRepJsonStr);	
				}
				if(pRepActMsg)
					repActMsgLen = strlen(pRepActMsg)+1;
				if(repActMsgLen > 1)
				{
					{
						if(thrRepInfo.repInfo) free(thrRepInfo.repInfo);
						thrRepInfo.repInfo = (char *)malloc(repActMsgLen);
						if(thrRepInfo.repInfo)
						{
							memset(thrRepInfo.repInfo, 0, sizeof(char)*repActMsgLen);
							sprintf_s(thrRepInfo.repInfo, sizeof(char)*repActMsgLen, "%s", pRepActMsg);
						}
						if(pRepActMsg) free(pRepActMsg);
					}
					{
						char * uploadRepJsonStr = NULL;
						char * str = (char *)&thrRepInfo;
						int jsonStrlen = Parser_swm_mon_prc_action_rep(str, &uploadRepJsonStr);
						if(jsonStrlen > 0 && uploadRepJsonStr != NULL)
						{
							g_sendcbf(&g_PluginInfo, swm_mon_prc_event_rep, uploadRepJsonStr, strlen(uploadRepJsonStr)+1, NULL, NULL);
						}
						if(uploadRepJsonStr)free(uploadRepJsonStr);	
					}
				}
				if(thrRepInfo.repInfo) free(thrRepInfo.repInfo);

				//SendNormalMsg(isSWMNormal, NULL);
				//m_bNormalStatusChange = isSWMNormal;
			}
		}
		/*else
		{
			BOOL isSWMNormal = TRUE;
			if(IsSWMThrNormal(&isSWMNormal))
				if(m_bNormalStatusChange != isSWMNormal)
				{
					m_bNormalStatusChange = isSWMNormal;
					if(isSWMNormal)
						SendNormalMsg(isSWMNormal, NULL);
				}
		}*/
	}
	return bRet = TRUE;
}

//---------------------------------process monitor info list function-----------------------------------------
static prc_mon_info_node_t * CreatePrcMonInfoList()
{
	prc_mon_info_node_t * head = NULL;
	head = (prc_mon_info_node_t *)malloc(sizeof(prc_mon_info_node_t));
	if(head) 
	{
		head->next = NULL;
		head->prcMonInfo.isValid = 1;
		head->prcMonInfo.prcName = NULL;
		head->prcMonInfo.ownerName = NULL;
		head->prcMonInfo.prcID = 0;
		head->prcMonInfo.memUsage = 0;
		head->prcMonInfo.cpuUsage = 0;
		head->prcMonInfo.isActive = 0;
		head->prcMonInfo.prcCpuUsageLastTimes.lastKernelTime = 0;
		head->prcMonInfo.prcCpuUsageLastTimes.lastUserTime = 0;
		head->prcMonInfo.prcCpuUsageLastTimes.lastTime = 0;
	}
	return head;
}

BOOL GetSysMemoryUsageKB(long * totalPhysMemKB, long * availPhysMemKB) 
{ 
	MEMORYSTATUSEX memStatex;
	if(NULL == totalPhysMemKB || NULL == availPhysMemKB) return FALSE;
	memStatex.dwLength = sizeof (memStatex);
	app_os_GlobalMemoryStatusEx (&memStatex);
#ifdef WIN32
	*totalPhysMemKB = (long)(memStatex.ullTotalPhys/DIV);
	*availPhysMemKB = (long)(memStatex.ullAvailPhys/DIV);
#else
	*totalPhysMemKB = (long)(memStatex.ullTotalPhys);
	*availPhysMemKB = (long)(memStatex.ullAvailPhys);
#endif
	return TRUE;
}


static int GetSysCPUUsage(sys_cpu_usage_time_t * pSysCpuUsageLastTimes)
{
	__int64 nowIdleTime = 0, nowKernelTime = 0, nowUserTime = 0;
	__int64 sysTime = 0, idleTime = 0;
	int cpuUsage = 0;
	if(pSysCpuUsageLastTimes == NULL) return cpuUsage;
	app_os_GetSystemTimes((FILETIME *)&nowIdleTime, (FILETIME *)&nowKernelTime, (FILETIME *)&nowUserTime);
	if(pSysCpuUsageLastTimes->lastUserTime == 0 && pSysCpuUsageLastTimes->lastKernelTime == 0 && pSysCpuUsageLastTimes->lastIdleTime == 0)
	{
		pSysCpuUsageLastTimes->lastIdleTime = nowIdleTime;
		pSysCpuUsageLastTimes->lastKernelTime = nowKernelTime;
		pSysCpuUsageLastTimes->lastUserTime = nowUserTime;
		return cpuUsage;
	}

	sysTime = (nowKernelTime - pSysCpuUsageLastTimes->lastKernelTime) + (nowUserTime - pSysCpuUsageLastTimes->lastUserTime);
	idleTime = nowIdleTime - pSysCpuUsageLastTimes->lastIdleTime;

	if(sysTime) 
	{
#ifdef WIN32
		cpuUsage = (int)((sysTime - idleTime)*100/sysTime);
#else
		cpuUsage = (int)((sysTime)*100/(sysTime + idleTime));
#endif
	}

	pSysCpuUsageLastTimes->lastKernelTime = nowKernelTime;
	pSysCpuUsageLastTimes->lastUserTime = nowUserTime;
	pSysCpuUsageLastTimes->lastIdleTime = nowIdleTime;

	return cpuUsage;
}

static CAGENT_PTHREAD_ENTRY(SoftwareMonThreadStart, args)
{
	sw_mon_handler_context_t *pSoftwareMonHandlerContext = (sw_mon_handler_context_t *)args;
	long sysTotalPhysMemKB = 0, sysAvailPhysMemKB = 0;
	UpdateLogonUserPrcList(pSoftwareMonHandlerContext->prcMonInfoList);
	while (pSoftwareMonHandlerContext->isSoftwareMonThreadRunning)
	{
		app_os_EnterCriticalSection(&pSoftwareMonHandlerContext->swSysMonCS);
		pSoftwareMonHandlerContext->sysMonInfo.cpuUsage = GetSysCPUUsage(&pSoftwareMonHandlerContext->sysMonInfo.sysCpuUsageLastTimes);
		if(GetSysMemoryUsageKB(&sysTotalPhysMemKB, &sysAvailPhysMemKB))
		{
			pSoftwareMonHandlerContext->sysMonInfo.totalPhysMemoryKB = sysTotalPhysMemKB;
			pSoftwareMonHandlerContext->sysMonInfo.availPhysMemoryKB = sysAvailPhysMemKB;
		}
		app_os_LeaveCriticalSection(&pSoftwareMonHandlerContext->swSysMonCS);

		UpdateLogonUserPrcList(pSoftwareMonHandlerContext->prcMonInfoList);
		if(pSoftwareMonHandlerContext->isUserLogon)
		{
			GetPrcMonInfo(pSoftwareMonHandlerContext->prcMonInfoList);
			if(!pSoftwareMonHandlerContext->isSoftwareMonThreadRunning) break;
			app_os_EnterCriticalSection(&pSoftwareMonHandlerContext->swMonObjCS);
			MonObjUpdate();  //update mon objs
			app_os_LeaveCriticalSection(&pSoftwareMonHandlerContext->swMonObjCS);
			PrcMonInfoAnalyze(); 
		}
		if(!pSoftwareMonHandlerContext->isSoftwareMonThreadRunning) break;
		{//app_os_sleep(1000);
			int i = 0;
			for(i = 0; pSoftwareMonHandlerContext->isSoftwareMonThreadRunning && i<10; i++)
			{
				app_os_sleep(100);
			}
		}

#ifdef IS_SA_WATCH
		SetWatchThreadFlag(SWM_THREAD1_FLAG);
#endif
	}
	return 0;
}

static void SendErrMsg(char * errorStr,int sendLevel)
{
	{
		char * uploadRepJsonStr = NULL;
		char * str = errorStr;
		int jsonStrlen = Parser_string(str, &uploadRepJsonStr,swm_error_rep);
		if(jsonStrlen > 0 && uploadRepJsonStr != NULL)
		{
			switch(sendLevel)
			{
			case swm_send_cbf:
				{
					g_sendcbf(&g_PluginInfo, swm_error_rep, uploadRepJsonStr, strlen(uploadRepJsonStr)+1, NULL, NULL);
					break;
				}
			case swm_send_report_cbf:
				{
					g_sendreportcbf(&g_PluginInfo, uploadRepJsonStr, strlen(uploadRepJsonStr)+1, NULL, NULL);
					break;
				}
			//case swm_send_infospec_cbf:
			//case swm_cust_cbf:
			//case swm_subscribe_cust_cbf:
			default:
				{
					g_sendcbf(&g_PluginInfo, swm_error_rep, uploadRepJsonStr, strlen(uploadRepJsonStr)+1, NULL, NULL);
					break;
				}
			}
		}
		if(uploadRepJsonStr)free(uploadRepJsonStr);	
	}
}

static sensor_info_list CreateSensorInfoList()
{
	sensor_info_node_t * head = NULL;
	head = (sensor_info_node_t *)malloc(sizeof(sensor_info_node_t));
	if(head)
	{
		memset(head, 0, sizeof(sensor_info_node_t));
		head->next = NULL;
	}
	return head;
}

static int DeleteAllSensorInfoNode(sensor_info_list sensorInfoList)
{
	int iRet = -1;
	sensor_info_node_t * delNode = NULL, *head = NULL;
	int i = 0;
	if(sensorInfoList == NULL) return iRet;
	head = sensorInfoList;
	delNode = head->next;
	while(delNode)
	{
		head->next = delNode->next;
		if(delNode->sensorInfo.jsonStr != NULL)
		{
			free(delNode->sensorInfo.jsonStr);
			delNode->sensorInfo.jsonStr = NULL;
		}
		if(delNode->sensorInfo.pathStr != NULL)
		{
			free(delNode->sensorInfo.pathStr);
			delNode->sensorInfo.pathStr = NULL;
		}
		free(delNode);
		delNode = head->next;
	}

	iRet = 0;
	return iRet;
}

static void DestroySensorInfoList(sensor_info_list sensorInfoList)
{
	if(NULL == sensorInfoList) return;
	DeleteAllSensorInfoNode(sensorInfoList);
	free(sensorInfoList); 
	sensorInfoList = NULL;
}

static BOOL IsSensorInfoListEmpty(sensor_info_list sensorInfoList)
{
	BOOL bRet = TRUE;
	sensor_info_node_t * curNode = NULL, *head = NULL;
	if(sensorInfoList == NULL) return bRet;
	head = sensorInfoList;
	curNode = head->next;
	if(curNode != NULL) bRet = FALSE;
	return bRet;
}


static BOOL UploadSysMonInfo(int sendLevel,int repCommandID)
{//if sendLeven  =  swm_send_cbf, repCommandID is reply command ID. repCommandID useless if sendLevel = swm_send_report_cbf.
	sw_mon_handler_context_t * pSWMonHandlerContext = (sw_mon_handler_context_t *)&SWMonHandlerContext;
	BOOL bRet = FALSE;
	char *repJsonStr = NULL;
	int len = 0;
	cagent_status_t status;

	if(pSWMonHandlerContext == NULL) //return bRet;
	{
		char errorStr[128] = {0};
		sprintf(errorStr, "Command(%d), Get capability error!", repCommandID);
		SendErrMsg(errorStr, sendLevel);
		return bRet; //len;
	}
	/*else if(pSWMonHandlerContext->gatherLevel > CFG_FLAG_SEND_PRCINFO_ALL && !(pSWMonHandlerContext->isUserLogon))
	{
		char errorStr[128] = {0};
		sprintf(errorStr, "Not logged on!");
		SendErrMsg(errorStr, swm_send_cbf);
		//return bRet; //len;
	}*/

	app_os_EnterCriticalSection(&pSWMonHandlerContext->swSysMonCS);
	app_os_EnterCriticalSection(&pSWMonHandlerContext->swPrcMonCS);
	{
		char * strSys = (char*)&pSWMonHandlerContext->sysMonInfo;
		char * strProc = (char*)&pSWMonHandlerContext->prcMonInfoList;
		if( swm_send_infospec_cbf == sendLevel || swm_get_capability_rep == repCommandID 
			|| (swm_send_report_cbf == sendLevel && pSWMonHandlerContext->isAutoUpdateAll))
		{
			len = Parser_get_spec_info_rep(strSys,strProc,pSWMonHandlerContext->gatherLevel,pSWMonHandlerContext->isUserLogon,&repJsonStr);
			if(len == 0 || repJsonStr == NULL)
			{
				char errorStr[128] = {0};
				sprintf(errorStr, "Command(%d), Get capability error!", repCommandID);
				SendErrMsg(errorStr, sendLevel);
			}
		}
		else if( swm_send_report_cbf == sendLevel) 
		{
			app_os_EnterCriticalSection(&pSWMonHandlerContext->autoUploadItemsListCS);
			if( pSWMonHandlerContext->autoUploadItemsList && !IsSensorInfoListEmpty(pSWMonHandlerContext->autoUploadItemsList))
			{
				len = Parser_get_request_items_rep(strSys,strProc,pSWMonHandlerContext->gatherLevel,pSWMonHandlerContext->isUserLogon,pSWMonHandlerContext->autoUploadItemsList,&repJsonStr);
				if(len == 0 || repJsonStr == NULL)
				{
					char errorStr[128] = {0};
					sprintf(errorStr, "Command(%d), Get data error!", repCommandID);
					SendErrMsg(errorStr, sendLevel);
				}
			}
			app_os_LeaveCriticalSection(&pSWMonHandlerContext->autoUploadItemsListCS);
		}
		else if(swm_send_cbf == sendLevel && repCommandID == swm_span_auto_report_rep)
		{
			app_os_EnterCriticalSection(&pSWMonHandlerContext->autoReportItemsListCS);
			if( pSWMonHandlerContext->autoReportItemsList && !IsSensorInfoListEmpty(pSWMonHandlerContext->autoReportItemsList))
			{
				len = Parser_get_request_items_rep(strSys,strProc,pSWMonHandlerContext->gatherLevel,pSWMonHandlerContext->isUserLogon,pSWMonHandlerContext->autoReportItemsList,&repJsonStr);
				if(len == 0 || repJsonStr == NULL)
				{
					char errorStr[128] = {0};
					sprintf(errorStr, "Command(%d), Get data error!", repCommandID);
					SendErrMsg(errorStr, sendLevel);
				}
			}
			app_os_LeaveCriticalSection(&pSWMonHandlerContext->autoReportItemsListCS);
		}
		/***********************************/
		//Add for default
		/*else
		{
			len = Parser_get_spec_info_rep(strSys,strProc,&repJsonStr);
		}*/
		/*************************************/
	}
	app_os_LeaveCriticalSection(&pSWMonHandlerContext->swPrcMonCS);
	app_os_LeaveCriticalSection(&pSWMonHandlerContext->swSysMonCS);

	if(len > 0 && repJsonStr != NULL)
	{
			switch(sendLevel)
			{
			case swm_send_cbf:
				{
					status = g_sendcbf(&g_PluginInfo, repCommandID, repJsonStr, strlen(repJsonStr)+1, NULL, NULL);
					break;
				}
			case swm_send_report_cbf:
				{
					status = g_sendreportcbf(&g_PluginInfo, repJsonStr, strlen(repJsonStr)+1, NULL, NULL);
					break;
				}
			case swm_send_infospec_cbf:
				{
					status = g_sendcapabilitycbf(&g_PluginInfo, repJsonStr, strlen(repJsonStr)+1, NULL, NULL);
					break;
				}
			//case swm_cust_cbf:
			//case swm_subscribe_cust_cbf:
			default:
				{
					status = g_sendcbf(&g_PluginInfo, swm_get_capability_rep, repJsonStr, strlen(repJsonStr)+1, NULL, NULL);
					break;
				}
			}
	}

	if(repJsonStr)free(repJsonStr);

	if(status == cagent_success)
	{
		bRet = TRUE;
	}
	return bRet;
}

static void UploadSysMonData(sensor_info_list sensorInfoList, char * pSessionID)
{
	if(NULL == sensorInfoList || NULL == pSessionID) return;
	{
		sw_mon_handler_context_t * pSWMonHandlerContext = (sw_mon_handler_context_t *)&SWMonHandlerContext;
		if(!IsSensorInfoListEmpty(sensorInfoList))
		{
			sensor_info_node_t * curNode = NULL;
			curNode = sensorInfoList->next;
			while(curNode)
			{
				app_os_EnterCriticalSection(&pSWMonHandlerContext->swSysMonCS);
				app_os_EnterCriticalSection(&pSWMonHandlerContext->swPrcMonCS);
				{
					char * strSys = (char*)&pSWMonHandlerContext->sysMonInfo;
					char * strProc = (char*)&pSWMonHandlerContext->prcMonInfoList;
					Parser_GetSensorJsonStr(strSys,strProc,&curNode->sensorInfo);
				}
				app_os_LeaveCriticalSection(&pSWMonHandlerContext->swPrcMonCS);
				app_os_LeaveCriticalSection(&pSWMonHandlerContext->swSysMonCS);

				curNode = curNode->next;
			}
			{
				char * repJsonStr = NULL;
				int jsonStrlen = Parser_PackGetSensorDataRep(sensorInfoList, pSessionID, &repJsonStr);
				if(jsonStrlen > 0 && repJsonStr != NULL)
				{
					g_sendcbf(&g_PluginInfo, swm_get_sensors_data_rep, repJsonStr, strlen(repJsonStr)+1, NULL, NULL);
				}
				if(repJsonStr)free(repJsonStr);
			}
		}
		else
		{
			char * errorRepJsonStr = NULL;
			char errorStr[128];
			int jsonStrlen = 0;
			sprintf(errorStr, "Command(%d), Get sensors data empty!", swm_get_sensors_data_req);
			jsonStrlen = Parser_PackGetSensorDataError(errorStr, pSessionID, &errorRepJsonStr);
			if(jsonStrlen > 0 && errorRepJsonStr != NULL)
			{
				g_sendcbf(&g_PluginInfo, swm_get_sensors_data_error_rep, errorRepJsonStr, strlen(errorRepJsonStr)+1, NULL, NULL);
			}
			if(errorRepJsonStr)free(errorRepJsonStr);
		}
	}
}
//For auto report command
static CAGENT_PTHREAD_ENTRY(AutoReportThreadStart, args)
{
	sw_mon_handler_context_t *pSoftwareMonHandlerContext = (sw_mon_handler_context_t *)args;
	clock_t timeoutNow = 0;
	BOOL isAuto = FALSE;
	int timeoutMS = 0;
	int intervalMS = 0;
	while (pSoftwareMonHandlerContext->isAutoReportThreadRunning)
	{
		app_os_EnterCriticalSection(&pSoftwareMonHandlerContext->autoReportCS);
		isAuto = pSoftwareMonHandlerContext->isAutoReport;
		timeoutMS = pSoftwareMonHandlerContext->autoReportTimeoutMs;
		intervalMS = pSoftwareMonHandlerContext->autoReportIntervalMs;
		app_os_LeaveCriticalSection(&pSoftwareMonHandlerContext->autoReportCS);
		if(!isAuto)
		{
			app_os_cond_wait(&pSoftwareMonHandlerContext->autoReportSyncCond, &pSoftwareMonHandlerContext->autoReportSyncMutex);
			if(!pSoftwareMonHandlerContext->isAutoReportThreadRunning)
			{
				break;
			}
			UploadSysMonInfo(swm_send_cbf, swm_span_auto_report_rep);
		}
		else
		{
			//Timeout is useless.
			//if(timeoutMS)
			{
				timeoutNow = clock();
				if(pSoftwareMonHandlerContext->autoReportTimeoutStart == 0)
				{
					pSoftwareMonHandlerContext->autoReportTimeoutStart = timeoutNow;
				}

				if(timeoutNow - pSoftwareMonHandlerContext->autoReportTimeoutStart >= timeoutMS)
				{
					app_os_EnterCriticalSection(&pSoftwareMonHandlerContext->autoReportCS);
					pSoftwareMonHandlerContext->isAutoReport = FALSE;
					app_os_LeaveCriticalSection(&pSoftwareMonHandlerContext->autoReportCS);
					pSoftwareMonHandlerContext->autoReportTimeoutStart = 0;
				}
				else
				{
					UploadSysMonInfo(swm_send_cbf,swm_span_auto_report_rep);
					app_os_sleep(intervalMS);
				}
			}
		}
		app_os_sleep(100);
	}
	return 0;
}


static CAGENT_PTHREAD_ENTRY(AutoUploadThreadStart, args)
{
	sw_mon_handler_context_t *pSoftwareMonHandlerContext = (sw_mon_handler_context_t *)args;
	BOOL isAuto = FALSE;
	int intervalMS = 0;
	while (pSoftwareMonHandlerContext->isAutoUploadThreadRunning)
	{
		app_os_EnterCriticalSection(&pSoftwareMonHandlerContext->autoUploadCS);
		isAuto = pSoftwareMonHandlerContext->isAutoUpload;
		//timeoutMS = pSoftwareMonHandlerContext->sysAutoUploadTimeoutMs;
		intervalMS = pSoftwareMonHandlerContext->autoUploadIntervalMs;
		app_os_LeaveCriticalSection(&pSoftwareMonHandlerContext->autoUploadCS);
		if(!isAuto)
		{
			app_os_cond_wait(&pSoftwareMonHandlerContext->autoUploadSyncCond, &pSoftwareMonHandlerContext->autoUploadSyncMutex);
			if(!pSoftwareMonHandlerContext->isAutoUploadThreadRunning)
			{
				break;
			}
			UploadSysMonInfo(swm_send_report_cbf,unknown_cmd);
		}
		else
		{
				app_os_sleep(intervalMS);
				UploadSysMonInfo(swm_send_report_cbf,unknown_cmd);
		}    
		app_os_sleep(100);
	}
	return 0;
}

/* **************************************************************************************
 *  Function Name: Handler_Initialize
 *  Description: Init any objects or variables of this handler
 *  Input :  PLUGIN_INFO *pluginfo
 *  Output: None
 *  Return:  handler_success  : Success Init Handler
 *           handler_fail : Fail Init Handler
 * ***************************************************************************************/
int HANDLER_API Handler_Initialize( HANDLER_INFO *pluginfo )
{
	BOOL bRet = FALSE;
	char tmpCfg_gatherLev[4] = {0};

	if(pluginfo == NULL)
	{
		return handler_fail;
	}

	if(g_bEnableLog)
	{
		g_loghandle = pluginfo->loghandle;
	}

	SetSWMThrThreadRunning = FALSE;
	SetSWMThrThreadHandle = 0;
	// 1. Topic of this handler
	snprintf( pluginfo->Name, sizeof(pluginfo->Name), "%s", strPluginName );
	pluginfo->RequestID = iRequestID;
	pluginfo->ActionID = iActionID;
	printf(" >Name: %s\n", strPluginName);
	// 2. Copy agent info 
	memcpy(&g_PluginInfo, pluginfo, sizeof(HANDLER_INFO));
	g_PluginInfo.agentInfo = pluginfo->agentInfo;
	{
	char mdPath[MAX_PATH] = {0};
	app_os_get_module_path(mdPath);
	path_combine(agentConfigFilePath, mdPath, DEF_CONFIG_FILE_NAME);
	}
	// 3. Callback function -> Send JSON Data by this callback function
	g_sendcbf = g_PluginInfo.sendcbf = pluginfo->sendcbf;
	g_sendcustcbf = g_PluginInfo.sendcustcbf = pluginfo->sendcustcbf;
	g_subscribecustcbf = g_PluginInfo.subscribecustcbf = pluginfo->subscribecustcbf;
	g_sendreportcbf = g_PluginInfo.sendreportcbf = pluginfo->sendreportcbf;
	g_sendcapabilitycbf =g_PluginInfo.sendcapabilitycbf = pluginfo->sendcapabilitycbf;
#if 1	
	memset(&SWMonHandlerContext, 0, sizeof(sw_mon_handler_context_t));
	memset(MonObjConfigPath, 0,sizeof(MonObjConfigPath));
	memset(MonObjTmpConfigPath, 0, sizeof(MonObjTmpConfigPath));

	SWMonHandlerContext.prcMonInfoList = CreatePrcMonInfoList();
	SWMonHandlerContext.monObjInfoList = CreateMonObjInfoList();

	{
		char modulePath[MAX_PATH] = {0};     
		if(!app_os_get_module_path(modulePath)) goto done;
		sprintf(MonObjConfigPath, "%s%s", modulePath, DEF_PRC_MON_CONFIG_NAME);
		sprintf(MonObjTmpConfigPath, "%s%sTmp", modulePath, DEF_PRC_MON_CONFIG_NAME);
		//InitMonObjInfoListFromConfig(SWMonHandlerContext.monObjInfoList, MonObjConfigPath);
		//DeleteMonObjInfoNodeWithID(SWMonHandlerContext.monObjInfoList, 0);
#ifdef WIN32
		sprintf(CmdHelperFile, "%s\\%s", modulePath, CMD_HELPER_FILE_NAME);
#endif
	}
	app_os_InitializeCriticalSection(&SWMonHandlerContext.swPrcMonCS);
	app_os_InitializeCriticalSection(&SWMonHandlerContext.swMonObjCS);
	app_os_InitializeCriticalSection(&SWMonHandlerContext.swSysMonCS);
	app_os_InitializeCriticalSection(&SWMonHandlerContext.autoUploadCS);
	app_os_InitializeCriticalSection(&SWMonHandlerContext.autoUploadItemsListCS);
	app_os_InitializeCriticalSection(&SWMonHandlerContext.autoReportCS);
	app_os_InitializeCriticalSection(&SWMonHandlerContext.autoReportItemsListCS);

	if(strlen(tmpCfg_gatherLev) == 0)
	{
		if(proc_config_get(agentConfigFilePath, CFG_PROCESS_GATHER_LEVEL, tmpCfg_gatherLev, sizeof(tmpCfg_gatherLev)) <= 0)
		{
			SWMonHandlerContext.gatherLevel = CFG_FLAG_SEND_PRCINFO_BY_ALLUSERS; //1 indicate gather logon user processes, 0 gather all processes.
		}
		else
		{
			SWMonHandlerContext.gatherLevel = atoi(tmpCfg_gatherLev);
			if(SWMonHandlerContext.gatherLevel > CFG_FLAG_SEND_PRCINFO_BY_ALLUSERS)
			{
				memset(SWMonHandlerContext.sysUserName, 0 , sizeof(SWMonHandlerContext.sysUserName));
				proc_config_get(agentConfigFilePath, CFG_SYSTEM_LOGON_USER, SWMonHandlerContext.sysUserName, sizeof(SWMonHandlerContext.sysUserName));
			}
		}
	}

	SWMonHandlerContext.isSoftwareMonThreadRunning = TRUE;
	if (app_os_thread_create(&SWMonHandlerContext.softwareMonThreadHandle, SoftwareMonThreadStart, &SWMonHandlerContext) != 0)
	{
		ProcessMonitorLog(g_loghandle, Normal, "%s()[%d]###Start software monitor thread failed!\n",__FUNCTION__, __LINE__);
		SWMonHandlerContext.isSoftwareMonThreadRunning = FALSE;
		goto done;
	}

	SWMonHandlerContext.isUserLogon = FALSE;
	SWMonHandlerContext.isAutoUploadThreadRunning = FALSE;
	SWMonHandlerContext.isAutoUpload = FALSE;
	SWMonHandlerContext.autoUploadIntervalMs = DEF_PRC_MON_INFO_UPLOAD_INTERVAL_MS;
	if(app_os_cond_setup(&SWMonHandlerContext.autoUploadSyncCond) != 0)
	{
		ProcessMonitorLog(g_loghandle, Normal, "%s()[%d]###Create sysMonInfo upload sync cond failed!\n",__FUNCTION__, __LINE__);

		goto done;
	}

	if(app_os_mutex_setup(&SWMonHandlerContext.autoUploadSyncMutex) != 0)
	{
		ProcessMonitorLog(g_loghandle, Normal, "%s()[%d]###Create sysMonInfo upload sync mutex failed!\n",__FUNCTION__, __LINE__);
		goto done;
	}

	SWMonHandlerContext.isAutoUploadThreadRunning = TRUE;
	if (app_os_thread_create(&SWMonHandlerContext.autoUploadThreadHandle, AutoUploadThreadStart, &SWMonHandlerContext) != 0)
	{
		ProcessMonitorLog(g_loghandle, Normal,"%s()[%d]###Start auto upload thread failed!\n",__FUNCTION__, __LINE__);

		SWMonHandlerContext.isAutoUploadThreadRunning = FALSE;
		goto done;
	}
	//bRet = TRUE;
	//auto report thread initiallize.
	SWMonHandlerContext.isAutoReportThreadRunning = FALSE;
	SWMonHandlerContext.isAutoReport = FALSE;
	SWMonHandlerContext.autoReportTimeoutStart = 0;
	SWMonHandlerContext.autoReportIntervalMs = DEF_PRC_MON_INFO_UPLOAD_INTERVAL_MS;
	SWMonHandlerContext.autoReportTimeoutMs = DEF_PRC_MON_INFO_UPLOAD_TIMEOUT_MS;
	if(app_os_cond_setup(&SWMonHandlerContext.autoReportSyncCond) != 0)
	{
		ProcessMonitorLog(g_loghandle, Normal, "%s()[%d]###Create sysMonInfo report sync cond failed!\n",__FUNCTION__, __LINE__);
		goto done;
	}

	if(app_os_mutex_setup(&SWMonHandlerContext.autoReportSyncMutex) != 0)
	{
		ProcessMonitorLog(g_loghandle, Normal, "%s()[%d]###Create sysMonInfo report sync mutex failed!\n",__FUNCTION__, __LINE__);
		goto done;
	}

	SWMonHandlerContext.isAutoReportThreadRunning = TRUE;
	if (app_os_thread_create(&SWMonHandlerContext.autoReportThreadHandle, AutoReportThreadStart, &SWMonHandlerContext) != 0)
	{
		ProcessMonitorLog(g_loghandle, Normal,"%s()[%d]###Start auto report thread failed!\n",__FUNCTION__, __LINE__);

		SWMonHandlerContext.isAutoReportThreadRunning = FALSE;
		goto done;
	}
	bRet = TRUE;

done:
	if(bRet == FALSE)
	{
		Handler_Stop();
	}
	return bRet;
#endif

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
	if(SetSWMThrThreadRunning)
	{
		SetSWMThrThreadRunning = FALSE;
		app_os_thread_join(SetSWMThrThreadHandle);
	}

	if(SWMonHandlerContext.isSoftwareMonThreadRunning)
	{
		SWMonHandlerContext.isSoftwareMonThreadRunning = FALSE;
		app_os_thread_join(SWMonHandlerContext.softwareMonThreadHandle);
	}

	if(SWMonHandlerContext.isAutoUploadThreadRunning)
	{
		SWMonHandlerContext.isAutoUploadThreadRunning = FALSE;
		app_os_cond_signal(&SWMonHandlerContext.autoUploadSyncCond);
		app_os_thread_join(SWMonHandlerContext.autoUploadThreadHandle);
		DestroySensorInfoList(SWMonHandlerContext.autoUploadItemsList);
	}
	app_os_cond_cleanup(&SWMonHandlerContext.autoUploadSyncCond);
	app_os_mutex_cleanup(&SWMonHandlerContext.autoUploadSyncMutex);

	if(SWMonHandlerContext.isAutoReportThreadRunning)
	{
		SWMonHandlerContext.isAutoReportThreadRunning = FALSE;
		app_os_cond_signal(&SWMonHandlerContext.autoReportSyncCond);
		app_os_thread_join(SWMonHandlerContext.autoReportThreadHandle);
		DestroySensorInfoList(SWMonHandlerContext.autoReportItemsList);
	}
	app_os_cond_cleanup(&SWMonHandlerContext.autoReportSyncCond);
	app_os_mutex_cleanup(&SWMonHandlerContext.autoReportSyncMutex);

	if(SWMonHandlerContext.monObjInfoList)
	{
		SetThresholdConfigFile(SWMonHandlerContext.monObjInfoList, MonObjConfigPath);
		DestroyMonObjInfoList(SWMonHandlerContext.monObjInfoList);
		SWMonHandlerContext.monObjInfoList = NULL;
	}
	if(SWMonHandlerContext.prcMonInfoList)
	{
		DestroyPrcMonInfoList(SWMonHandlerContext.prcMonInfoList);
		SWMonHandlerContext.prcMonInfoList = NULL;
	}

	app_os_DeleteCriticalSection(&SWMonHandlerContext.autoReportItemsListCS);
	app_os_DeleteCriticalSection(&SWMonHandlerContext.autoReportCS);
	app_os_DeleteCriticalSection(&SWMonHandlerContext.autoUploadItemsListCS);
	app_os_DeleteCriticalSection(&SWMonHandlerContext.swPrcMonCS);
	app_os_DeleteCriticalSection(&SWMonHandlerContext.swMonObjCS);
	app_os_DeleteCriticalSection(&SWMonHandlerContext.swSysMonCS);
	app_os_DeleteCriticalSection(&SWMonHandlerContext.autoUploadCS);

	return TRUE;
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
	sw_mon_handler_context_t *pSWMonHandlerContext =  &SWMonHandlerContext;
	int commCmd = unknown_cmd;
	char errorStr[128] = {0};
	if(!data || datalen <= 0) return;// status;

	if(!ParseReceivedData(data, datalen, &commCmd))
		return;
	switch(commCmd)
	{
	case swm_get_capability_req:
		{//{"susiCommData":{"commCmd":521,"catalogID":4,"handlerName":"ProcessMonitor"}}
			//GetCapability();
			UploadSysMonInfo(swm_send_cbf,swm_get_capability_rep);
			break;
		}
	case swm_get_sensors_data_req:
		{
			//{"susiCommData":{"commCmd":523,"catalogID":4,"handlerName":"ProcessMonitor","sessionID":"1234","sensorIDList":{"e":[{"n":"ProcessMonitor/System Monitor Info/CPU Usage"}]}}}
			//UploadSysMonInfo(swm_send_cbf,swm_get_sensors_data_rep);
			char curSessionID[256] = {0};
			sensor_info_list sensorInfoList = CreateSensorInfoList();
			if(Parser_ParseGetSensorDataReqEx(data, sensorInfoList, curSessionID))
			{
				if(strlen(curSessionID))
				{
					UploadSysMonData(sensorInfoList, curSessionID);
				}
			}
			DestroySensorInfoList(sensorInfoList);
			break;
		}
	//Deal by default.
	//case swm_set_sensors_data_req:
	//	{

	//		break;
	//	}
		//replaced by Handler_AutoUploadStart
	case swm_span_auto_report_req:
		{//{"susiCommData": {"commCmd": 533,"catalogID": 4,"handlerName": "ProcessMonitor",
		 //"requestItems": {"e":[{ "n": "ProcessMonitor/System Monitor Info"},{"n": "ProcessMonitor/Process Monitor Info/17364-cmd.exe}]},
	     //"autoUploadIntervalMs": 1000,"autoUploadTimeoutMs": 10000}}
			unsigned int intervalMs = 0; //Ms
			unsigned int timeoutMs = 0; //Ms
			bool bRet = FALSE;
			if(pSWMonHandlerContext->isAutoReport)
			{
				app_os_EnterCriticalSection(&pSWMonHandlerContext->autoReportCS);
				pSWMonHandlerContext->isAutoReport = FALSE;
				app_os_LeaveCriticalSection(&pSWMonHandlerContext->autoReportCS);
				pSWMonHandlerContext->autoReportTimeoutStart = 0;
			}

			app_os_EnterCriticalSection(&pSWMonHandlerContext->autoReportItemsListCS);
			if(pSWMonHandlerContext->autoReportItemsList)
			{
				DestroySensorInfoList(pSWMonHandlerContext->autoReportItemsList);
				pSWMonHandlerContext->autoReportItemsList = NULL;
			}
			pSWMonHandlerContext->autoReportItemsList = CreateSensorInfoList();
			pSWMonHandlerContext->isAutoReportAll = FALSE;
			bRet = Parser_ParseAutoReportReq(data, pSWMonHandlerContext->autoReportItemsList, &(pSWMonHandlerContext->isAutoReportAll),&intervalMs,&timeoutMs);
			app_os_LeaveCriticalSection(&pSWMonHandlerContext->autoReportItemsListCS);
			if(bRet && intervalMs> 0 && timeoutMs > 0 && !(timeoutMs < intervalMs)){
				app_os_EnterCriticalSection(&pSWMonHandlerContext->autoReportCS);
				pSWMonHandlerContext->autoReportIntervalMs = intervalMs;
				pSWMonHandlerContext->autoReportTimeoutMs = timeoutMs;
				if(!pSWMonHandlerContext->isAutoReport)
				{
					pSWMonHandlerContext->isAutoReport = TRUE; 
					app_os_cond_signal(&pSWMonHandlerContext->autoReportSyncCond);
				}
				app_os_LeaveCriticalSection(&pSWMonHandlerContext->autoReportCS);
			}
			else
			{
				memset(errorStr, 0, sizeof(errorStr));
				sprintf(errorStr, "Command(%d) format error!", swm_span_auto_report_rep);
				{
					char * uploadRepJsonStr = NULL;
					char * str = errorStr;
					int jsonStrlen = Parser_string(str, &uploadRepJsonStr,swm_error_rep);
					if(jsonStrlen > 0 && uploadRepJsonStr != NULL)
					{
						g_sendcbf(&g_PluginInfo, swm_error_rep, uploadRepJsonStr, strlen(uploadRepJsonStr)+1, NULL, NULL);
					}
					if(uploadRepJsonStr)free(uploadRepJsonStr);	
				}
			}
			break;
		}
	case swm_set_mon_objs_req:     //set threshold
		{//{"susiCommData":{"commCmd":157,"catalogID":4,"handlerName":"ProcessMonitor","swmThr":{"cpu":[{"prc":"cmd.exe","prcID":0,"max":100,"min":0,"lastingTimeS":5,"intervalTimeS":10,"type":3,"enable":"true","actType":0,"actCmd":"None"}],"mem":[{"prc":"cmd.exe","prcID":0,"max":8000,"min":12,"lastingTimeS":5,"intervalTimeS":10,"type":3,"enable":"true","actType":0,"actCmd":"None"}]}}}
			SWMSetMonObjs(pSWMonHandlerContext, (char*)data);
			break;
		}
	case swm_del_all_mon_objs_req:  //delete threshold
		{//{"susiCommData":{"commCmd":159,"catalogID":4,"handlerName":"ProcessMonitor"}}
			SWMDelAllMonObjs();
			break;
		}
	case swm_restart_prc_req:
		{//{"susiCommData":{"commCmd":167,"catalogID":4,"requestID":15,"prcID":31281}}
			SWMRestartPrc(pSWMonHandlerContext, (char*)data);
			break;
		}
	case swm_kill_prc_req:
		{//{"susiCommData":{"commCmd":169,"catalogID":4,"requestID":15,"prcID":31281}}
		 //{"susiCommData":{"commCmd":169,"catalogID":4,"handlerName":"ProcessMonitor","prcID":21420}}
			SWMKillPrc(pSWMonHandlerContext, (char*)data);
			break;
		}
	default:
		{
			{
				char * uploadRepJsonStr = NULL;
				char * str = "Unknown cmd!";
				int jsonStrlen = Parser_string(str, &uploadRepJsonStr,swm_error_rep);
				if(jsonStrlen > 0 && uploadRepJsonStr != NULL)
				{
					g_sendcbf(&g_PluginInfo, swm_error_rep, uploadRepJsonStr, strlen(uploadRepJsonStr)+1, NULL, NULL);
				}
				if(uploadRepJsonStr)free(uploadRepJsonStr);	
			}

			break;
		}
	}
	return ;//status;
}

/* **************************************************************************************
 *  Function Name: Handler_Get_Capability
 *  Description: Get Handler Information specification. 
 *  Input :  None
 *  Output: char ** : pOutReply       // JSON Format
 *  Return:  int  : Length of the status information in JSON format
 *                :  0 : no data need to trans
 * **************************************************************************************/
int HANDLER_API Handler_Get_Capability( char ** pOutReply )
{
	//{"susiCommData":{"catalogID":4,"autoUploadIntervalSec":30,"requestID":1001,"requestItems":["HWM"],"commCmd":2051,"type":"WSN"}}
	int len = 0; // Data length of the pOutReply 
	char * repJsonStr = NULL;

	sw_mon_handler_context_t * pSWMonHandlerContext = (sw_mon_handler_context_t *)&SWMonHandlerContext;

	if(pSWMonHandlerContext == NULL) return len;
	app_os_EnterCriticalSection(&pSWMonHandlerContext->swSysMonCS);
	app_os_EnterCriticalSection(&pSWMonHandlerContext->swPrcMonCS);
	{
		char * strSys = (char*)&pSWMonHandlerContext->sysMonInfo;
		char * strProc = (char*)&pSWMonHandlerContext->prcMonInfoList;
		len = Parser_get_spec_info_rep(strSys,strProc,pSWMonHandlerContext->gatherLevel,pSWMonHandlerContext->isUserLogon,&repJsonStr);
		//len = jsonStrlen+1;
	}
	app_os_LeaveCriticalSection(&pSWMonHandlerContext->swPrcMonCS);
	app_os_LeaveCriticalSection(&pSWMonHandlerContext->swSysMonCS);
	if(len > 0 && repJsonStr != NULL)
	{
		len = strlen(repJsonStr) +1;
		*pOutReply = (char *)(malloc(len));
		if(*pOutReply)
		{
			memset(*pOutReply, 0, len);
			strcpy(*pOutReply, repJsonStr);
		}
	}
	if(repJsonStr)free(repJsonStr);
	return len;
}

/* **************************************************************************************
 *  Function Name: Handler_AutoReportStart
 *  Description: Start Auto Report
 *  Input : char *pInQuery
 *  Output: char * pOutReply
 *  Return: None
 * ***************************************************************************************/
void HANDLER_API Handler_AutoReportStart(char *pInQuery)
{
/*{"susiCommData":{"catalogID":4,"autoUploadIntervalSec":30,"requestID":1001,"requestItems":["HWM"],"commCmd":2053,"type":"WSN"}}*/
	//{"susiCommData":{"autoUploadIntervalSec":10,"commCmd":2053,"requestID":0,"agentID":"","handlerName":"general","sendTS":1432186317}}
	unsigned int intervalTimeS = 0; //sec
	bool bRet = FALSE;
	sw_mon_handler_context_t * pSWMonHandlerContext = (sw_mon_handler_context_t *)&SWMonHandlerContext;
	app_os_EnterCriticalSection(&pSWMonHandlerContext->autoUploadItemsListCS);
	if(pSWMonHandlerContext->autoUploadItemsList)
	{
		DestroySensorInfoList(pSWMonHandlerContext->autoUploadItemsList);
		pSWMonHandlerContext->autoUploadItemsList = NULL;
	}
	pSWMonHandlerContext->autoUploadItemsList = CreateSensorInfoList();
	pSWMonHandlerContext->isAutoUpdateAll = false;
	bRet = Parser_ParseAutoUploadCmd(pInQuery, pSWMonHandlerContext->autoUploadItemsList, &(pSWMonHandlerContext->isAutoUpdateAll),&intervalTimeS);
	app_os_LeaveCriticalSection(&pSWMonHandlerContext->autoUploadItemsListCS);
	if(bRet){
		app_os_EnterCriticalSection(&pSWMonHandlerContext->autoUploadCS);
		pSWMonHandlerContext->autoUploadIntervalMs = intervalTimeS*1000;
		if(!pSWMonHandlerContext->isAutoUpload)
		{
			pSWMonHandlerContext->isAutoUpload = TRUE; 
			app_os_cond_signal(&pSWMonHandlerContext->autoUploadSyncCond);
		}
		app_os_LeaveCriticalSection(&pSWMonHandlerContext->autoUploadCS);
	}
	return;
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
	//{"susiCommData":{"catalogID":4,"requestID":1001,"commCmd":2056,"type":"WSN"}}
	sw_mon_handler_context_t * pSWMonHandlerContext = (sw_mon_handler_context_t *)&SWMonHandlerContext;
	bool bRet = Parser_ParseAutoReportStopCmd(pInQuery);
	if(bRet)
	{
		app_os_EnterCriticalSection(&pSWMonHandlerContext->autoUploadCS);
		if(pSWMonHandlerContext->isAutoUpload)
		{
			pSWMonHandlerContext->isAutoUpload = FALSE; 
		}
		app_os_LeaveCriticalSection(&pSWMonHandlerContext->autoUploadCS);
		app_os_EnterCriticalSection(&pSWMonHandlerContext->autoUploadItemsListCS);
		DestroySensorInfoList(pSWMonHandlerContext->autoUploadItemsList);
		pSWMonHandlerContext->autoUploadItemsList = NULL;
		app_os_LeaveCriticalSection(&pSWMonHandlerContext->autoUploadItemsListCS);
	}
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
