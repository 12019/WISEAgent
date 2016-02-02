/****************************************************************************/
/* Copyright(C) : Advantech Technologies, Inc.								*/
/* Create Date  : 2014/12/30 by lige									*/
/* Abstract     : SoftwareMonitor API interface definition   					*/
/* Reference    : None														*/
/****************************************************************************/

#include "platform.h"
#include "susiaccess_handler_api.h"
#include "software_config.h"
#include "SoftwareMonitorHandler.h"
#include "SoftwareMonitorLog.h"
//#include "configuration.h"
#include "Parser.h"
#include "common.h"
#include "cJSON.h"
#ifdef WIN32
#include <psapi.h>
#pragma comment(lib,"Psapi.lib")
#endif
#ifdef IS_SA_WATCH
#include "SUSIAgentWatch.h"
#endif

static void* g_loghandle = NULL;
static BOOL g_bEnableLog = true;

const char strPluginName[MAX_TOPIC_LEN] = {"software_monitoring"};
const int iRequestID = cagent_request_software_monitoring;
const int iActionID = cagent_reply_software_monitoring;

static Handler_info  g_PluginInfo;

static BOOL SetSWMThrThreadRunning = FALSE;

CAGENT_THREAD_HANDLE SetSWMThrThreadHandle = 0;//NULL;

static HandlerSendCbf  g_sendcbf = NULL;						// Client Send information (in JSON format) to Cloud Server	
static HandlerSendCustCbf  g_sendcustcbf = NULL;			    // Client Send information (in JSON format) to Cloud Server with custom topic	
static HandlerAutoReportCbf g_sendreportcbf = NULL;				// Client Send report (in JSON format) to Cloud Server with AutoReport topic	
static HandlerSendCapabilityCbf g_sendcapabilitycbf = NULL;	
static HandlerSubscribeCustCbf g_subscribecustcbf = NULL;

char MonObjConfigPath[MAX_PATH] = {0};
char MonObjTmpConfigPath[MAX_PATH] = {0};
static char agentConfigFilePath[MAX_PATH] = {0};

static BOOL UploadSysMonInfo();
static int DeleteAllMonObjInfoNode(mon_obj_info_node_t * head);
static BOOL UpdateLogonUserPrcList(prc_mon_info_node_t * head);
static mon_obj_info_node_t * CreateMonObjInfoList();
static BOOL InitMonObjInfoListFromConfig(mon_obj_info_list monObjList, char *monObjConfigPath);
BOOL KillProcessWithID(DWORD prcID);
DWORD RestartProcessWithID(DWORD prcID);

sw_mon_handler_context_t  SWMonHandlerContext;
static void ClearMonObjThr(mon_obj_info_t * curMonObj);
static void DestroyMonObjInfoList(mon_obj_info_node_t * head);
static void SWMWhenDelThrSetToNormal(mon_obj_info_list monObjInfoList);
static void MonObjInfoListFillConfig(mon_obj_info_list monObjList, char *monObjConfigPath);
static mon_obj_info_node_t * CreateMonObjInfoList();
static prc_mon_info_node_t * FindPrcMonInfoNode(prc_mon_info_node_t * head, DWORD prcID);

static BOOL IsSWMThrNormal(BOOL * isNormal);
//static BOOL UploadSysMonInfo();
//static BOOL UpdateLogonUserPrcList(prc_mon_info_node_t * head);
//static int DeleteAllMonObjInfoNode(mon_obj_info_node_t * head);
//static BOOL InitMonObjInfoListFromConfig(mon_obj_info_list monObjList, char *monObjConfigPath);

/******************function**************************/

static CAGENT_PTHREAD_ENTRY(SysMonInfoUploadThreadStart, args)
{
	sw_mon_handler_context_t *pSoftwareMonHandlerContext = (sw_mon_handler_context_t *)args;
	clock_t timeoutNow = 0;
	BOOL isAuto = FALSE;
	int timeoutMS = 0;
	int intervalMS = 0;
	while (pSoftwareMonHandlerContext->isSysMonInfoUploadThreadRunning)
	{
		app_os_EnterCriticalSection(&pSoftwareMonHandlerContext->sysMonInfoAutoCS);
		isAuto = pSoftwareMonHandlerContext->isSysAutoUpload;
		timeoutMS = pSoftwareMonHandlerContext->sysAutoUploadTimeoutMs;
		intervalMS = pSoftwareMonHandlerContext->sysUploadIntervalMs;
		app_os_LeaveCriticalSection(&pSoftwareMonHandlerContext->sysMonInfoAutoCS);
		if(!isAuto)
		{
			app_os_cond_wait(&pSoftwareMonHandlerContext->sysMonInfoUploadSyncCond, &pSoftwareMonHandlerContext->sysMonInfoUploadSyncMutex);
			if(!pSoftwareMonHandlerContext->isSysMonInfoUploadThreadRunning)
			{
				break;
			}
			UploadSysMonInfo();
		}
		else
		{
			timeoutNow = clock();
			if(pSoftwareMonHandlerContext->sysTimeoutStart == 0)
			{
				pSoftwareMonHandlerContext->sysTimeoutStart = timeoutNow;
			}

			if(timeoutNow - pSoftwareMonHandlerContext->sysTimeoutStart >= timeoutMS)
			{
				app_os_EnterCriticalSection(&pSoftwareMonHandlerContext->sysMonInfoAutoCS);
				pSoftwareMonHandlerContext->isSysAutoUpload = FALSE;
				app_os_LeaveCriticalSection(&pSoftwareMonHandlerContext->sysMonInfoAutoCS);
				pSoftwareMonHandlerContext->sysTimeoutStart = 0;
			}
			else
			{
				app_os_sleep(intervalMS);
				UploadSysMonInfo();
			}
		}    
		app_os_sleep(100);
	}
	return 0;
}

static mon_obj_info_node_t * FindMonObjInfoNodeWithID(mon_obj_info_node_t * head, int prcID)
{
	mon_obj_info_node_t * findNode = NULL;
	if(head == NULL || prcID == 0) return findNode;
	findNode = head->next;
	while(findNode)
	{
		if(findNode->monObjInfo.prcID == prcID) break;
		else
		{
			findNode = findNode->next;
		}
	}

	return findNode;
}

static int InsertMonObjInfoList(mon_obj_info_node_t * head, mon_obj_info_t * monObjInfo)
{
	int iRet = -1;
	mon_obj_info_node_t * newNode = NULL, * findNode = NULL;
	if(monObjInfo == NULL || monObjInfo->prcName == NULL || head == NULL) return iRet;
	findNode = FindMonObjInfoNodeWithID(head, monObjInfo->prcID);
	if(findNode == NULL)
	{
		newNode = (mon_obj_info_node_t *)malloc(sizeof(mon_obj_info_node_t));
		memset(newNode, 0, sizeof(mon_obj_info_node_t));
		newNode->monObjInfo.isValid = 1;
		if(monObjInfo->prcName)
		{
			int tmpLen = strlen(monObjInfo->prcName);
			newNode->monObjInfo.prcName = (char *)malloc(tmpLen + 1);
			memset(newNode->monObjInfo.prcName, 0, tmpLen + 1);
			memcpy(newNode->monObjInfo.prcName, monObjInfo->prcName, tmpLen);
		}

		newNode->monObjInfo.prcID = monObjInfo->prcID;
		newNode->monObjInfo.prcResponse = TRUE;

		memcpy(&(newNode->monObjInfo.cpuThrItem), &(monObjInfo->cpuThrItem), sizeof(swm_thr_item_t));
		memcpy(&(newNode->monObjInfo.memThrItem), &(monObjInfo->memThrItem), sizeof(swm_thr_item_t));

		newNode->monObjInfo.cpuAct = monObjInfo->cpuAct;
		if(monObjInfo->cpuActCmd)
		{
			int tmpLen = strlen(monObjInfo->cpuActCmd);
			newNode->monObjInfo.cpuActCmd = (char *)malloc(tmpLen + 1);
			memset(newNode->monObjInfo.cpuActCmd, 0, tmpLen + 1);
			memcpy(newNode->monObjInfo.cpuActCmd, monObjInfo->cpuActCmd, tmpLen);
		}
		newNode->monObjInfo.memAct = monObjInfo->memAct;
		if(monObjInfo->memActCmd)
		{
			int tmpLen = strlen(monObjInfo->memActCmd);
			newNode->monObjInfo.memActCmd = (char *)malloc(tmpLen + 1);
			memset(newNode->monObjInfo.memActCmd, 0, tmpLen + 1);
			memcpy(newNode->monObjInfo.memActCmd, monObjInfo->memActCmd, tmpLen);
		}

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


static mon_obj_info_node_t * FindMonObjInfoNode(mon_obj_info_node_t * head, char * prcName)
{
	mon_obj_info_node_t * findNode = NULL;
	if(head == NULL || prcName == NULL) return findNode;
	findNode = head->next;
	while(findNode)
	{
		if(!strcmp(findNode->monObjInfo.prcName, prcName)) break;
		else
		{
			findNode = findNode->next;
		}
	}

	return findNode;
}

static BOOL MonObjUpdate()
{
	BOOL bRet = FALSE;
	sw_mon_handler_context_t *pSoftwareMonHandlerContext = (sw_mon_handler_context_t *)&SWMonHandlerContext;
	if(pSoftwareMonHandlerContext == NULL || pSoftwareMonHandlerContext->prcMonInfoList == NULL || 
		pSoftwareMonHandlerContext->monObjInfoList == NULL) return bRet;
	{
		mon_obj_info_node_t * curMonObjInfoNode = NULL;
		prc_mon_info_node_t * curPrcMonInfoNode = pSoftwareMonHandlerContext->prcMonInfoList->next;
		while(curPrcMonInfoNode)
		{
			app_os_EnterCriticalSection(&pSoftwareMonHandlerContext->swPrcMonCS);
			curMonObjInfoNode = FindMonObjInfoNodeWithID(pSoftwareMonHandlerContext->monObjInfoList, curPrcMonInfoNode->prcMonInfo.prcID);
			if(curMonObjInfoNode)
			{
				if((curMonObjInfoNode->monObjInfo.cpuThrItem.isEnable || curMonObjInfoNode->monObjInfo.memThrItem.isEnable)
					&& curMonObjInfoNode->monObjInfo.isValid == 0) curMonObjInfoNode->monObjInfo.isValid = 1;
			}
			else
			{
				curMonObjInfoNode = FindMonObjInfoNode(pSoftwareMonHandlerContext->monObjInfoList, curPrcMonInfoNode->prcMonInfo.prcName);
				if(curMonObjInfoNode)
				{
					if((curMonObjInfoNode->monObjInfo.cpuThrItem.isEnable || curMonObjInfoNode->monObjInfo.memThrItem.isEnable))
					{
						if(curMonObjInfoNode->monObjInfo.prcID == 0 || curMonObjInfoNode->monObjInfo.isValid == 0) 
						{
							curMonObjInfoNode->monObjInfo.prcID = curPrcMonInfoNode->prcMonInfo.prcID;
							curMonObjInfoNode->monObjInfo.isValid = 1;
							ClearMonObjThr(&curMonObjInfoNode->monObjInfo);
						}
						else
						{
							mon_obj_info_t monObjInfo;
							memset(&monObjInfo, 0, sizeof(mon_obj_info_t));
							{
								int prcNameLen = strlen(curMonObjInfoNode->monObjInfo.prcName)+1;
								monObjInfo.prcName = (char *)malloc(prcNameLen);
								memset(monObjInfo.prcName, 0, prcNameLen);
								strcpy(monObjInfo.prcName, curMonObjInfoNode->monObjInfo.prcName);
							}
							monObjInfo.isValid = 1;
							monObjInfo.prcID = curPrcMonInfoNode->prcMonInfo.prcID;
							memcpy(&(monObjInfo.cpuThrItem), &(curMonObjInfoNode->monObjInfo.cpuThrItem), sizeof(swm_thr_item_t));
							monObjInfo.cpuThrItem.isEnable = TRUE;
							monObjInfo.cpuThrItem.checkResultValue.vi = DEF_INVALID_VALUE;
							monObjInfo.cpuThrItem.checkSourceValueList.head = NULL;
							monObjInfo.cpuThrItem.checkSourceValueList.nodeCnt = 0;
							monObjInfo.cpuThrItem.repThrTime = 0;
							monObjInfo.cpuThrItem.isNormal = TRUE;

							memcpy(&(monObjInfo.memThrItem), &(curMonObjInfoNode->monObjInfo.memThrItem), sizeof(swm_thr_item_t));
							monObjInfo.memThrItem.isEnable = TRUE;
							monObjInfo.memThrItem.checkResultValue.vi = DEF_INVALID_VALUE;
							monObjInfo.memThrItem.checkSourceValueList.head = NULL;
							monObjInfo.memThrItem.checkSourceValueList.nodeCnt = 0;
							monObjInfo.memThrItem.repThrTime = 0;
							monObjInfo.memThrItem.isNormal = TRUE;

							monObjInfo.cpuAct = curMonObjInfoNode->monObjInfo.cpuAct;
							if(curMonObjInfoNode->monObjInfo.cpuActCmd)
							{
								int actCmdLen = strlen(curMonObjInfoNode->monObjInfo.cpuActCmd)+1;
								monObjInfo.cpuActCmd = (char *)malloc(actCmdLen);
								memset(monObjInfo.cpuActCmd, 0, actCmdLen);
								strcpy(monObjInfo.cpuActCmd, curMonObjInfoNode->monObjInfo.cpuActCmd);
							}

							monObjInfo.memAct = curMonObjInfoNode->monObjInfo.memAct;
							if(curMonObjInfoNode->monObjInfo.memActCmd)
							{
								int actCmdLen = strlen(curMonObjInfoNode->monObjInfo.memActCmd)+1;
								monObjInfo.memActCmd = (char *)malloc(actCmdLen);
								memset(monObjInfo.memActCmd, 0, actCmdLen);
								strcpy(monObjInfo.memActCmd, curMonObjInfoNode->monObjInfo.memActCmd);
							}
							InsertMonObjInfoList(pSoftwareMonHandlerContext->monObjInfoList, &monObjInfo);
							if(monObjInfo.prcName) free(monObjInfo.prcName);
							if(monObjInfo.cpuActCmd) free(monObjInfo.cpuActCmd);
							if(monObjInfo.memActCmd) free(monObjInfo.memActCmd);
						}
					}
				}
			}
			app_os_LeaveCriticalSection(&pSoftwareMonHandlerContext->swPrcMonCS);
			curPrcMonInfoNode = curPrcMonInfoNode->next;
			app_os_sleep(10);
		}
	}

	return bRet = TRUE;
}

void SendNormalMsg(bool isTotalNormal, char *pNormalMsg)
{
		//char normalMsg[10] = {0};
	swm_thr_rep_info_t thrRepInfo;
	thrRepInfo.repInfo = NULL;
	thrRepInfo.isTotalNormal = isTotalNormal;
	{
		if(pNormalMsg)
		{
			int normalMsgLen = strlen(pNormalMsg) + 1;
			thrRepInfo.repInfo = (char *)malloc(sizeof(char)* normalMsgLen);
			memset(thrRepInfo.repInfo, 0, sizeof(char)* normalMsgLen);
			sprintf_s(thrRepInfo.repInfo, sizeof(char)*normalMsgLen, "%s", pNormalMsg);
		}
		else
		{
			thrRepInfo.repInfo = (char *)malloc(sizeof(LONG));
			memset(thrRepInfo.repInfo, 0, sizeof(LONG));
		}
	}
	{
		char * uploadRepJsonStr = NULL;
		char * str = (char *)&thrRepInfo;
		int jsonStrlen = Parser_swm_mon_prc_event_rep(str, &uploadRepJsonStr);
		if(jsonStrlen > 0 && uploadRepJsonStr != NULL)
		{
			g_sendcbf(&g_PluginInfo, swm_mon_prc_event_rep, uploadRepJsonStr, strlen(uploadRepJsonStr)+1, NULL, NULL);
		}
		if(uploadRepJsonStr)free(uploadRepJsonStr);	
	}
	if(thrRepInfo.repInfo) free(thrRepInfo.repInfo);
}

bool ExistInTmpMonObjInfoList(mon_obj_info_list tmpMonObjInfoList, mon_obj_info_node_t * checkNode)
{
	if(tmpMonObjInfoList == NULL ||  checkNode == NULL) return FALSE;
	{
		mon_obj_info_node_t * curNode = NULL;
		curNode = tmpMonObjInfoList->next;
		while(curNode)
		{
			if((curNode->monObjInfo.cpuThrItem.isEnable || curNode->monObjInfo.memThrItem.isEnable)
				&&!strcmp(curNode->monObjInfo.prcName,checkNode->monObjInfo.prcName)) //!strcmp(delNode->monObjInfo.thrItem.tagName, tagName))
				return TRUE;
			curNode = curNode->next;
		}
	}
	return FALSE;
}

static void SWMWhenDelThrCheckNormal(mon_obj_info_list monObjInfoList, mon_obj_info_list tmpMonObjInfoList) 
{
	if(NULL == monObjInfoList) return;
	{
		//cagent_handle_t cagentHandle = SWMonHandlerContext.susiHandlerContext.cagentHandle;
		mon_obj_info_node_t * curNode = NULL;
		//char tmpRepMsg[1024*2] = {0};
		char * pRepMsg = NULL;
		int repBufLen = 0;
		int repMsgLen = 0;
		char tmpMsg[512] = {0};
		curNode = monObjInfoList->next;
		while(curNode)
		{
			memset(tmpMsg, 0, sizeof(tmpMsg));
			if(curNode->monObjInfo.cpuThrItem.isEnable && !curNode->monObjInfo.cpuThrItem.isNormal
				&& !ExistInTmpMonObjInfoList(tmpMonObjInfoList, curNode))
			{
				if(strlen(tmpMsg))sprintf_s(tmpMsg, sizeof(tmpMsg), "%s, %s normal", tmpMsg, curNode->monObjInfo.cpuThrItem.tagName);
				else sprintf_s(tmpMsg, sizeof(tmpMsg), "Process(PrcName:%s, PrcID:%d) %s normal", curNode->monObjInfo.prcName, 
					curNode->monObjInfo.prcID, curNode->monObjInfo.cpuThrItem.tagName);
				curNode->monObjInfo.cpuThrItem.isNormal = TRUE;
			}
			if(curNode->monObjInfo.memThrItem.isEnable && !curNode->monObjInfo.memThrItem.isNormal
				&& !ExistInTmpMonObjInfoList(tmpMonObjInfoList, curNode))
			{
				if(strlen(tmpMsg))sprintf_s(tmpMsg, sizeof(tmpMsg), "%s, %s normal", tmpMsg, curNode->monObjInfo.memThrItem.tagName);
				else sprintf_s(tmpMsg, sizeof(tmpMsg), "Process(PrcName:%s, PrcID:%d) %s normal", curNode->monObjInfo.prcName, 
					curNode->monObjInfo.prcID, curNode->monObjInfo.memThrItem.tagName);
				curNode->monObjInfo.memThrItem.isNormal = TRUE;
			}
			if(strlen(tmpMsg))
			{
				pRepMsg = (char *)DynamicStrCat(pRepMsg, &repBufLen,";", tmpMsg);
			}
			curNode = curNode->next;
		}
		if(pRepMsg)
		{
			BOOL isSWMNormal = TRUE;
			if(IsSWMThrNormal(&isSWMNormal))
			{
				SendNormalMsg(isSWMNormal,pRepMsg);
				free(pRepMsg);
			}
		}
	}
}

static CAGENT_PTHREAD_ENTRY(SetSWMThrThreadStart, args)
{
	//cagent_handle_t cagentHandle = SWMonHandlerContext.susiHandlerContext.cagentHandle;
	char repMsg[1024] = {0};
	BOOL bRet = FALSE;
	mon_obj_info_list tmpMonObjList = NULL;

	//Set Server total normal message as true.
	//SendNormalMsg(TRUE,NULL);
	//
	tmpMonObjList = CreateMonObjInfoList();
	bRet = InitMonObjInfoListFromConfig(tmpMonObjList, MonObjTmpConfigPath);
	if(!bRet)
	{
		sprintf(repMsg, "%s", "SWM threshold apply failed!");
	}
	else
	{
		mon_obj_info_list monObjList = SWMonHandlerContext.monObjInfoList; 
		app_os_EnterCriticalSection(&SWMonHandlerContext.swMonObjCS);
		if(monObjList && tmpMonObjList) 
		{
			SWMWhenDelThrCheckNormal(monObjList,tmpMonObjList);
			DeleteAllMonObjInfoNode(monObjList);
		}
		{
			mon_obj_info_node_t *curMonObjNode = tmpMonObjList->next;
			if(!monObjList) monObjList = CreateMonObjInfoList();
			while(curMonObjNode)
			{
				InsertMonObjInfoList(monObjList, &curMonObjNode->monObjInfo);
				curMonObjNode = curMonObjNode->next;
			}
			MonObjUpdate();
		}
		MonObjInfoListFillConfig(monObjList, MonObjConfigPath);
		app_os_LeaveCriticalSection(&SWMonHandlerContext.swMonObjCS);
		sprintf(repMsg, "SWM threshold apply OK!");
	}
	if(tmpMonObjList) DestroyMonObjInfoList(tmpMonObjList);
	if(strlen(repMsg))
	{
			char * uploadRepJsonStr = NULL;
			char * str = repMsg;
			int jsonStrlen = Parser_string(str, &uploadRepJsonStr, swm_set_mon_objs_rep);
			if(jsonStrlen > 0 && uploadRepJsonStr != NULL)
			{
				g_sendcbf(&g_PluginInfo, swm_set_mon_objs_rep, uploadRepJsonStr, strlen(uploadRepJsonStr)+1, NULL, NULL);
			}
			if(uploadRepJsonStr)free(uploadRepJsonStr);
	}
	remove(MonObjTmpConfigPath);
#ifdef WIN32
	app_os_CloseHandle(SetSWMThrThreadHandle);
	SetSWMThrThreadHandle = NULL;
#endif
	SetSWMThrThreadRunning = FALSE;
	return 0;
}
#ifdef EVENT_LOG
BOOL Str2SysTime(const char *buf, const char *format, SYSTEMTIME *tm)
{
	BOOL bRet = FALSE;
	int iYear = 0, iMon = 0, iDay = 0, iHour = 0, iMin = 0, iSec = 0;
	if(NULL == buf || NULL == format || NULL == tm) return bRet;
	if(sscanf_s(buf, format, &iYear, &iMon, &iDay, &iHour, &iMin, &iSec) != 0)
	{
		tm->wYear = iYear;
		tm->wMonth = iMon;
		tm->wDay = iDay;
		tm->wHour = iHour;
		tm->wMinute = iMin;
		tm->wSecond = iSec;
		bRet = TRUE;
	}
	return bRet;
}


EVENTLOGINFOLIST CreateEventLogInfoList()
{
	PEVENTLOGINFONODE  head = NULL;
	head = (PEVENTLOGINFONODE)malloc(sizeof(EVENTLOGINFONODE));
	if(head) 
	{
		head->next = NULL;
		head->eventLogInfo.eventComputer = NULL;
		head->eventLogInfo.eventDescription = NULL;
		head->eventLogInfo.eventGroup = NULL;
		head->eventLogInfo.eventID = 0;
		head->eventLogInfo.eventSource = NULL;
		head->eventLogInfo.eventTimestamp = NULL;
		head->eventLogInfo.eventType = NULL;
		head->eventLogInfo.eventUser = NULL;
	}
	return head;
}


BOOL GetEventWrittenFTime(DWORD dwWrittenTime, FILETIME * fWrittenTime)
{
	BOOL bRet = FALSE;
	ULONGLONG ullTimeStamp = 0;
	ULONGLONG SecsTo1970 = 116444736000000000;
	FILETIME ft;
	SYSTEMTIME st;
	if(fWrittenTime == NULL) return bRet;
	ullTimeStamp = Int32x32To64(dwWrittenTime, 10000000) + SecsTo1970;
	ft.dwHighDateTime = (DWORD)((ullTimeStamp >> 32) & 0xFFFFFFFF);
	ft.dwLowDateTime = (DWORD)(ullTimeStamp & 0xFFFFFFFF);

	app_os_FileTimeToLocalFileTime(&ft, fWrittenTime);
	app_os_FileTimeToSystemTime(fWrittenTime, &st);
	return bRet = TRUE;
}

BOOL GetEventTimestamp(FILETIME * fTime, char * timestampStr)
{
	BOOL bRet = FALSE;
	SYSTEMTIME sTime;
	if(fTime == NULL || timestampStr == NULL) return bRet;
	app_os_FileTimeToSystemTime(fTime, &sTime);
	sprintf_s(timestampStr, MAX_TIMESTAMP_LEN, "%04d-%02d-%02d %02d:%02d:%02d", sTime.wYear, sTime.wMonth, sTime.wDay,
		sTime.wHour, sTime.wMinute, sTime.wSecond);
	return bRet = TRUE;
}


BOOL GetEventType(WORD * pType, char * typeStr)
{
	BOOL bRet = FALSE;
	char * tmpTypeStr = NULL;
	if(pType == NULL || typeStr == NULL) return bRet;
	switch(*pType)
	{
	case EVENTLOG_ERROR_TYPE:
		tmpTypeStr = "Error";
		break;
	case EVENTLOG_WARNING_TYPE:
		tmpTypeStr = "Warning";
		break;
	case EVENTLOG_INFORMATION_TYPE:
		tmpTypeStr = "Information";
		break;
	case EVENTLOG_AUDIT_SUCCESS:
		tmpTypeStr = "Audit Success";
		break;
	case EVENTLOG_AUDIT_FAILURE:
		tmpTypeStr = "Audit Failure";
		break;
	default:
		tmpTypeStr = "Unknown";
		break;
	}

	memcpy(typeStr, tmpTypeStr, strlen(tmpTypeStr));

	return bRet = TRUE;
}


BOOL GetEventDll(char * pEventGroup, char * pEventSourceName, char * pEventDllPath)
{
	BOOL bRet = FALSE;
	HKEY hK;
	DWORD cbData;
	char keyname[256] = {0};
	if(pEventDllPath == NULL || pEventGroup == NULL || pEventSourceName == NULL) return bRet;
	_snprintf(keyname, sizeof(keyname), "System\\CurrentControlSet\\Services\\EventLog\\%s\\%s", pEventGroup, pEventSourceName);
	if(app_os_RegOpenKeyExA(HKEY_LOCAL_MACHINE, keyname, 0, KEY_ALL_ACCESS, &hK) != ERROR_SUCCESS)
	{
		return bRet;   
	}
	//cbData = MAX_PATH; 
	if (app_os_RegQueryValueExA(hK, "EventMessageFile", NULL, NULL, (LPBYTE)pEventDllPath, &cbData) == ERROR_SUCCESS)
	{
		bRet = TRUE;
	}
	app_os_RegCloseKey(hK);
	return bRet;
}

char * GetEventDescription(EVENTLOGRECORD * pEventLogRecord,  char * pEventGroup, char * pEventSourceName, LPTSTR * eventSStr)
{
	LPSTR descriptionStr = NULL;
	DWORD fmFlags = 0;
	char eventDllPaths[MAX_PATH] = {0};
	char dllRealPath[MAX_PATH] = {0};
	if(pEventLogRecord == NULL || pEventGroup == NULL || pEventSourceName == NULL || eventSStr == NULL) return descriptionStr;

	fmFlags |= FORMAT_MESSAGE_FROM_HMODULE;
	fmFlags |= FORMAT_MESSAGE_ALLOCATE_BUFFER;
	fmFlags |= FORMAT_MESSAGE_ARGUMENT_ARRAY;

	if(!GetEventDll(pEventGroup, pEventSourceName, eventDllPaths))
	{
		return descriptionStr;
	}

	{
#define DEF_ITEM_TOKEN_CNT       16
		char * eventDllPathToken[DEF_ITEM_TOKEN_CNT] = {NULL};
		int i = 0, j = 0;
		char * tmpBuf = eventDllPaths;
		HMODULE hEvt = NULL;
		while(eventDllPathToken[i] = strtok(tmpBuf, ";"))
		{
			i++;
			if(i >= DEF_ITEM_TOKEN_CNT) break;
			tmpBuf = NULL;
		}

		for(j = 0; j < i; i++)
		{
			descriptionStr = NULL;
			memset(dllRealPath, 0, sizeof(dllRealPath));
			app_os_ExpandEnvironmentStringsA(eventDllPathToken[j], dllRealPath, sizeof(dllRealPath));
			hEvt = app_os_LoadLibraryExA(dllRealPath, NULL, DONT_RESOLVE_DLL_REFERENCES);
			if(hEvt)
			{
				if(!app_os_FormatMessageA(fmFlags, hEvt, pEventLogRecord->EventID, 0,
					(LPTSTR)&descriptionStr, 0, eventSStr))
				{
					descriptionStr = NULL;   
				}
				app_os_FreeLibrary(hEvt);

				if(descriptionStr) return descriptionStr;
			}
		}
	}
	return NULL;
}


int InsertEventLogInfoNode(EVENTLOGINFOLIST eventLogInfoList, PEVENTLOGINFO pEventLogInfo)
{
	int iRet = -1;
	PEVENTLOGINFONODE head = eventLogInfoList;
	PEVENTLOGINFONODE newNode = NULL;
	if(pEventLogInfo == NULL || head == NULL) return iRet;
	if(pEventLogInfo->eventType)
	{
		newNode = (PEVENTLOGINFONODE)malloc(sizeof(EVENTLOGINFONODE));
		memset(newNode, 0, sizeof(EVENTLOGINFONODE));
		if(pEventLogInfo->eventType)
		{
			int tmpLen = strlen(pEventLogInfo->eventType);
			newNode->eventLogInfo.eventType = (char *)malloc(tmpLen + 1);
			memset(newNode->eventLogInfo.eventType, 0, tmpLen + 1);
			memcpy(newNode->eventLogInfo.eventType, pEventLogInfo->eventType, tmpLen);
		}
		newNode->eventLogInfo.eventID = pEventLogInfo->eventID;
		if(pEventLogInfo->eventComputer)
		{
			int tmpLen = strlen(pEventLogInfo->eventComputer);
			newNode->eventLogInfo.eventComputer = (char *)malloc(tmpLen + 1);
			memset(newNode->eventLogInfo.eventComputer, 0, tmpLen + 1);
			memcpy(newNode->eventLogInfo.eventComputer, pEventLogInfo->eventComputer, tmpLen);
		}
		if(pEventLogInfo->eventDescription)
		{
			int tmpLen = strlen(pEventLogInfo->eventDescription);
			newNode->eventLogInfo.eventDescription = (char *)malloc(tmpLen + 1);
			memset(newNode->eventLogInfo.eventDescription, 0, tmpLen + 1);
			memcpy(newNode->eventLogInfo.eventDescription, pEventLogInfo->eventDescription, tmpLen);
		}
		if(pEventLogInfo->eventGroup)
		{
			int tmpLen = strlen(pEventLogInfo->eventGroup);
			newNode->eventLogInfo.eventGroup = (char *)malloc(tmpLen + 1);
			memset(newNode->eventLogInfo.eventGroup, 0, tmpLen + 1);
			memcpy(newNode->eventLogInfo.eventGroup, pEventLogInfo->eventGroup, tmpLen);
		}
		if(pEventLogInfo->eventSource)
		{
			int tmpLen = strlen(pEventLogInfo->eventSource);
			newNode->eventLogInfo.eventSource = (char *)malloc(tmpLen + 1);
			memset(newNode->eventLogInfo.eventSource, 0, tmpLen + 1);
			memcpy(newNode->eventLogInfo.eventSource, pEventLogInfo->eventSource, tmpLen);
		}
		if(pEventLogInfo->eventTimestamp)
		{
			int tmpLen = strlen(pEventLogInfo->eventTimestamp);
			newNode->eventLogInfo.eventTimestamp = (char *)malloc(tmpLen + 1);
			memset(newNode->eventLogInfo.eventTimestamp, 0, tmpLen + 1);
			memcpy(newNode->eventLogInfo.eventTimestamp, pEventLogInfo->eventTimestamp, tmpLen);
		}
		if(pEventLogInfo->eventUser)
		{
			int tmpLen = strlen(pEventLogInfo->eventUser);
			newNode->eventLogInfo.eventUser = (char *)malloc(tmpLen + 1);
			memset(newNode->eventLogInfo.eventUser, 0, tmpLen + 1);
			memcpy(newNode->eventLogInfo.eventUser, pEventLogInfo->eventUser, tmpLen);
		}

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


int QueryEventLog(char * pEventGroup, SYSTEMTIME * pStartTime, SYSTEMTIME * pEndTime, EVENTLOGINFOLIST eventLogList)
{
	int iRet = -1;
	HANDLE hEventLog = NULL;
	DWORD status = ERROR_SUCCESS;
	FILETIME startFTime, endFTime;
	DWORD dwBytesToRead = 0;
	DWORD dwBytesRead = 0;
	DWORD dwMinimumBytesToRead = 0;
	PBYTE pBuffer = NULL;
	PBYTE pTemp = NULL;

	static int queryStartIndex = 0;
	int curQueryIndex = 0;
	int perQueryCnt = DEF_PER_QUERY_CNT;
	int curQueryCnt = 0;

	if(pEventGroup == NULL || pStartTime == NULL || pEndTime == NULL || eventLogList == NULL) return iRet;
	memset(&startFTime, 0, sizeof(FILETIME));
	memset(&endFTime, 0, sizeof(FILETIME));
	app_os_SystemTimeToFileTime(pStartTime, &startFTime);
	app_os_SystemTimeToFileTime(pEndTime, &endFTime);
	if(app_os_CompareFileTime(&endFTime, &startFTime) != 1) return iRet = 2;
	hEventLog = app_os_OpenEventLogA(NULL, pEventGroup);
	if(!hEventLog) return iRet = 3;

	{
		dwBytesToRead = MAX_RECORD_BUFFER_SIZE;
		pBuffer = (PBYTE)malloc(dwBytesToRead);
		memset(pBuffer, 0, dwBytesToRead);
		while(status == ERROR_SUCCESS)
		{
			if(curQueryCnt >= DEF_PER_QUERY_CNT) break;

			if (!app_os_ReadEventLogA(hEventLog, 
				EVENTLOG_SEQUENTIAL_READ | EVENTLOG_BACKWARDS_READ,
				0, 
				pBuffer,
				dwBytesToRead,
				&dwBytesRead,
				&dwMinimumBytesToRead))
			{
				status = app_os_GetLastError();
				if (ERROR_INSUFFICIENT_BUFFER == status)
				{
					status = ERROR_SUCCESS;

					pTemp = (PBYTE)realloc(pBuffer, dwMinimumBytesToRead);
					if (NULL == pTemp)
					{
						iRet = 4;
						goto cleanup;
					}
					memset(pTemp, 0, dwMinimumBytesToRead);
					pBuffer = pTemp;
					dwBytesToRead = dwMinimumBytesToRead;
				}
				else 
				{
					if (ERROR_HANDLE_EOF != status)
					{
						iRet = 5;
						goto cleanup;
					}
				}
			}
			else
			{
				//ParseEventInfoFromBuf(pBuffer, dwBytesRead, pEventGroup, &startFTime, &endFTime, eventLogList);
				{
					FILETIME * pStartFTime = &startFTime;
					FILETIME * pEndFTime = &endFTime;
					EVENTLOGINFOLIST eventLogInfoList = eventLogList;
					PBYTE pRecord = pBuffer;
					PBYTE pEndOfRecords = pBuffer + dwBytesRead;
					LPSTR pMessage = NULL;
					LPSTR pFinalMessage = NULL;
					PEVENTLOGRECORD pEventLogRecord = NULL;
					FILETIME eventWrittenFTime;
					while (pRecord < pEndOfRecords)
					{
						if(curQueryCnt >= DEF_PER_QUERY_CNT) break;

						pEventLogRecord = (PEVENTLOGRECORD)pRecord;
						memset(&eventWrittenFTime, 0, sizeof(FILETIME));
						GetEventWrittenFTime(pEventLogRecord->TimeWritten, &eventWrittenFTime);
						if((app_os_CompareFileTime(&eventWrittenFTime, pStartFTime) != 1)) break;
						if(app_os_CompareFileTime(pEndFTime, &eventWrittenFTime) == -1) 
						{
							pRecord += pEventLogRecord->Length;
							continue;
						}

						{
							PEVENTLOGINFO pEventLogInfo = NULL;
							char timeStampStr[MAX_TIMESTAMP_LEN] = {0};
							char eventTypeStr[MAX_EVENT_TYPE_LEN] = {0};
							char eventUserStr[MAX_EVENT_USER_LEN] = {0};
							char eventDomainStr[MAX_EVENT_DOMAIN_LEN] = {0};
							char tmpEventDespStr[MAX_EVENT_DESCRIPTION_LEN] = {0};
							LPSTR eventDespSStr[MAX_EVENT_DESP_SSTR_CNT] = {NULL};
							char * realEventDescription = NULL;
							int tmpLen = 0;
							char * tmpStr = NULL;
							char * tmpSourceName = NULL;
							char * tmpComputerName = NULL;
							pEventLogInfo = (PEVENTLOGINFO)malloc(sizeof(EVENTLOGINFO));
							if(!pEventLogInfo) return iRet = 6;
							GetEventTimestamp(&eventWrittenFTime, timeStampStr);
							tmpLen = strlen(timeStampStr);
							pEventLogInfo->eventTimestamp = (char *)malloc(tmpLen + 1);
							memset(pEventLogInfo->eventTimestamp, 0, tmpLen + 1);
							memcpy(pEventLogInfo->eventTimestamp, timeStampStr, tmpLen);

							pEventLogInfo->eventID = (WORD)pEventLogRecord->EventID;

							GetEventType(&pEventLogRecord->EventType, eventTypeStr);
							tmpLen = strlen(eventTypeStr);
							pEventLogInfo->eventType = (char *)malloc(tmpLen + 1);
							memset(pEventLogInfo->eventType, 0, tmpLen + 1);
							memcpy(pEventLogInfo->eventType, eventTypeStr, tmpLen);

							tmpSourceName = (LPSTR)((LPBYTE)pEventLogRecord + sizeof(EVENTLOGRECORD));
							tmpLen = strlen(tmpSourceName);
							pEventLogInfo->eventSource = (char *)malloc(tmpLen + 1);
							memset(pEventLogInfo->eventSource, 0, tmpLen + 1);
							memcpy(pEventLogInfo->eventSource, tmpSourceName, tmpLen);

							tmpComputerName = tmpSourceName + strlen(tmpSourceName) + 1;
							tmpLen = strlen(tmpComputerName);
							pEventLogInfo->eventComputer = (char *)malloc(tmpLen + 1);
							memset(pEventLogInfo->eventComputer, 0, tmpLen + 1);
							memcpy(pEventLogInfo->eventComputer, tmpComputerName, tmpLen);

							tmpLen = strlen(pEventGroup);
							pEventLogInfo->eventGroup = (char *)malloc(tmpLen + 1);
							memset(pEventLogInfo->eventGroup, 0, tmpLen + 1);
							memcpy(pEventLogInfo->eventGroup, pEventGroup, tmpLen);

							if(pEventLogRecord->UserSidLength)
							{
								SID_NAME_USE sidNameUse;
								DWORD userSize = MAX_EVENT_USER_LEN, domainSize = MAX_EVENT_DOMAIN_LEN;
								if(!app_os_LookupAccountSidA(NULL, (SID*)((LPSTR)pEventLogRecord + pEventLogRecord->UserSidOffset), eventUserStr, &userSize,
									eventDomainStr, &domainSize, &sidNameUse))
								{
									strncpy(eventUserStr, "no user", MAX_EVENT_USER_LEN);
									strncpy(eventDomainStr, "no domain", MAX_EVENT_DOMAIN_LEN);
								}
							}
							else
							{
								strncpy(eventUserStr, "A", MAX_EVENT_USER_LEN); 
								strncpy(eventDomainStr, "N", MAX_EVENT_DOMAIN_LEN);
							}
							tmpLen = strlen(eventUserStr) + strlen(eventDomainStr) + 1;
							pEventLogInfo->eventUser = (char *)malloc(tmpLen + 1);
							memset(pEventLogInfo->eventUser, 0, tmpLen + 1);
							sprintf_s(pEventLogInfo->eventUser, tmpLen + 1, "%s\\%s", eventDomainStr, eventUserStr);

							if(pEventLogRecord->NumStrings)
							{
								int sizeLeft = MAX_EVENT_DESCRIPTION_LEN - 1;
								LPSTR sStr = NULL;
								int nStr = 0;
								int tmpStrLen = 0;
								char * tmpStr = NULL;
								sStr = (LPSTR)((LPBYTE)pEventLogRecord + pEventLogRecord->StringOffset);
								for(nStr = 0; nStr < pEventLogRecord->NumStrings; nStr ++)
								{
									tmpStrLen = strlen(sStr);
									strncat(tmpEventDespStr, sStr, sizeLeft);

									tmpStr = strchr(tmpEventDespStr, '\0');
									if(tmpStr)
									{
										*tmpStr = ' ';
										tmpStr ++;
										*tmpStr = '\0';
									}

									sizeLeft -= tmpStrLen + 1;
									if(nStr < MAX_EVENT_DESP_SSTR_CNT)
										eventDespSStr[nStr] = (LPSTR)sStr;
									sStr = strchr( (LPSTR)sStr, '\0');
									sStr++;
								}
								realEventDescription = GetEventDescription(pEventLogRecord, pEventGroup, pEventLogInfo->eventSource, eventDespSStr);
								if(realEventDescription)
								{
									tmpStr = realEventDescription;
									while(tmpStr = strchr(tmpStr, '\n'))
									{
										*tmpStr = ' ';
										tmpStr ++;
									}
									tmpStr = realEventDescription;
									while(tmpStr = strchr(tmpStr, '\r'))
									{
										*tmpStr = ' ';
										tmpStr ++;
									}
								}
							}
							else
							{
								strncpy(tmpEventDespStr, "no description", MAX_EVENT_DESCRIPTION_LEN);
							}
							{
								char * despStr = (realEventDescription != NULL ? realEventDescription : tmpEventDespStr);
								tmpLen = strlen(despStr);
								pEventLogInfo->eventDescription = (char *)malloc(tmpLen + 1);
								memset(pEventLogInfo->eventDescription, 0, tmpLen + 1);
								memcpy(pEventLogInfo->eventDescription, despStr, tmpLen);
							}
							if(realEventDescription) 
							{
								app_os_LocalFree(realEventDescription);
								realEventDescription = NULL;
							}

							curQueryIndex++;
							if(curQueryIndex > queryStartIndex)
							{
								InsertEventLogInfoNode(eventLogInfoList, pEventLogInfo);
								curQueryCnt ++;
							}

							if(pEventLogInfo->eventComputer)
							{
								free(pEventLogInfo->eventComputer);
								pEventLogInfo->eventComputer = NULL;
							}
							if(pEventLogInfo->eventDescription)
							{
								free(pEventLogInfo->eventDescription);
								pEventLogInfo->eventDescription = NULL;
							}
							if(pEventLogInfo->eventGroup)
							{
								free(pEventLogInfo->eventGroup);
								pEventLogInfo->eventGroup = NULL;
							}
							if(pEventLogInfo->eventSource)
							{
								free(pEventLogInfo->eventSource);
								pEventLogInfo->eventSource = NULL;
							}
							if(pEventLogInfo->eventTimestamp)
							{
								free(pEventLogInfo->eventTimestamp);
								pEventLogInfo->eventTimestamp = NULL;
							}
							if(pEventLogInfo->eventType)
							{
								free(pEventLogInfo->eventType);
								pEventLogInfo->eventType = NULL;
							}
							if(pEventLogInfo->eventUser)
							{
								free(pEventLogInfo->eventUser);
								pEventLogInfo->eventUser = NULL;
							}
							free(pEventLogInfo);
							pEventLogInfo = NULL;

						}
						pRecord += pEventLogRecord->Length;
					}
				}
			}
		}
	}

	if(curQueryCnt < DEF_PER_QUERY_CNT) iRet = 0;
	else iRet = 1;

cleanup:

	if(iRet == 1) queryStartIndex += curQueryCnt;
	else queryStartIndex = 0;

	if(hEventLog) app_os_CloseEventLog(hEventLog);
	if(pBuffer) free(pBuffer);

	return iRet;
}


int DeleteAllEventLogInfoNode(EVENTLOGINFOLIST eventLogInfoList)
{
	int iRet = -1;
	PEVENTLOGINFONODE head = eventLogInfoList;
	PEVENTLOGINFONODE delNode = NULL;
	if(head == NULL) return iRet;

	delNode = head->next;
	while(delNode)
	{
		head->next = delNode->next;
		if(delNode->eventLogInfo.eventComputer)
		{
			free(delNode->eventLogInfo.eventComputer);
			delNode->eventLogInfo.eventComputer = NULL;
		}
		if(delNode->eventLogInfo.eventDescription)
		{
			free(delNode->eventLogInfo.eventDescription);
			delNode->eventLogInfo.eventDescription = NULL;
		}
		if(delNode->eventLogInfo.eventGroup)
		{
			free(delNode->eventLogInfo.eventGroup);
			delNode->eventLogInfo.eventGroup = NULL;
		}
		if(delNode->eventLogInfo.eventSource)
		{
			free(delNode->eventLogInfo.eventSource);
			delNode->eventLogInfo.eventSource = NULL;
		}
		if(delNode->eventLogInfo.eventTimestamp)
		{
			free(delNode->eventLogInfo.eventTimestamp);
			delNode->eventLogInfo.eventTimestamp = NULL;
		}
		if(delNode->eventLogInfo.eventType)
		{
			free(delNode->eventLogInfo.eventType);
			delNode->eventLogInfo.eventType = NULL;
		}
		if(delNode->eventLogInfo.eventUser)
		{
			free(delNode->eventLogInfo.eventUser);
			delNode->eventLogInfo.eventUser = NULL;
		}

		free(delNode);
		delNode = head->next;
	}

	iRet = 0;
	return iRet;
}


void DestroyEventLogInfoList(EVENTLOGINFOLIST eventLogInfoList)
{
	PEVENTLOGINFONODE head = eventLogInfoList;
	if(NULL == eventLogInfoList) return;
	DeleteAllEventLogInfoNode(eventLogInfoList);
	free(head); 
	head = NULL;
}


static CAGENT_PTHREAD_ENTRY(EventLogQueryThreadStart, args)
{
	sw_mon_handler_context_t *pSWMonHandlerContext =  &SWMonHandlerContext;
	cagent_handle_t cagentHandle = pSWMonHandlerContext->susiHandlerContext.cagentHandle;
	swm_get_sys_event_log_param_t * pGetEventLogParams = (swm_get_sys_event_log_param_t *)args;
	{
		char repMsg[2*1024] = {0};
		int iRet = 0;
		BOOL firstFlag = TRUE;
		SYSTEMTIME startSTime, endSTime;
		memset(&startSTime, 0, sizeof(SYSTEMTIME));
		memset(&endSTime, 0, sizeof(SYSTEMTIME));
		Str2SysTime(pGetEventLogParams->startTimeStr, "%d-%d-%d %d:%d:%d", &startSTime);
		Str2SysTime(pGetEventLogParams->endTimeStr, "%d-%d-%d %d:%d:%d", &endSTime);
		do 
		{
			EVENTLOGINFOLIST eventLogInfoList = CreateEventLogInfoList();
			if(firstFlag)
			{
				sprintf_s(repMsg, sizeof(repMsg), "%s", "START");
				{
					char * uploadRepJsonStr = NULL;
					char * str = repMsg;
					int jsonStrlen = Parser_string(str, &uploadRepJsonStr);
					if(jsonStrlen > 0 && uploadRepJsonStr != NULL)
					{
						g_sendcbf(&g_PluginInfo, swm_get_event_log_rep, uploadRepJsonStr, strlen(uploadRepJsonStr)+1, NULL, NULL);
					}
					if(uploadRepJsonStr)free(uploadRepJsonStr);	
				}

				memset(repMsg, 0, sizeof(repMsg));
				firstFlag = FALSE;
			}
			iRet = QueryEventLog(pGetEventLogParams->eventGroup, &startSTime, &endSTime, eventLogInfoList);
			switch(iRet)
			{
			case 0:
				{
					{
						char * uploadRepJsonStr = NULL;
						char * str = (char *)&eventLogInfoList;
						int jsonStrlen = Parser_swm_event_log_list_rep(str, &uploadRepJsonStr);
						if(jsonStrlen > 0 && uploadRepJsonStr != NULL)
						{
							g_sendcbf(&g_PluginInfo, swm_event_log_list_rep, uploadRepJsonStr, strlen(uploadRepJsonStr)+1, NULL, NULL);
						}
						if(uploadRepJsonStr)free(uploadRepJsonStr);	
					}

					sprintf_s(repMsg, sizeof(repMsg), "%s", "END");
					break;
				}
			case 1:
				{
					{
						char * uploadRepJsonStr = NULL;
						char * str = (char *)&eventLogInfoList;
						int jsonStrlen = Parser_swm_event_log_list_rep(str, &uploadRepJsonStr);
						if(jsonStrlen > 0 && uploadRepJsonStr != NULL)
						{
							g_sendcbf(&g_PluginInfo, swm_event_log_list_rep, uploadRepJsonStr, strlen(uploadRepJsonStr)+1, NULL, NULL);
						}
						if(uploadRepJsonStr)free(uploadRepJsonStr);	
					}

					sprintf_s(repMsg, sizeof(repMsg), "%s", "CONTINUE");
					break;
				}
			case 2:
				{
					sprintf_s(repMsg, sizeof(repMsg), "%s", "FAILED");
					break;
				}
			default:
				{
					sprintf_s(repMsg, sizeof(repMsg), "%s", "FAILED");
					break;
				}
			}

			if(strlen(repMsg) > 0) 
			{
				{
					char * uploadRepJsonStr = NULL;
					char * str = repMsg;
					int jsonStrlen = Parser_string(str, &uploadRepJsonStr);
					if(jsonStrlen > 0 && uploadRepJsonStr != NULL)
					{
						g_sendcbf(&g_PluginInfo, swm_get_event_log_rep, uploadRepJsonStr, strlen(uploadRepJsonStr)+1, NULL, NULL);
					}
					if(uploadRepJsonStr)free(uploadRepJsonStr);	
				}

				memset(repMsg, 0, sizeof(repMsg));
			}

			DestroyEventLogInfoList(eventLogInfoList);

		} while (iRet == 1);    
	}
	if(pGetEventLogParams) free(pGetEventLogParams);
	return 0;
}

static void SWMGetSysEventLog(sw_mon_handler_context_t * pSWMonHandlerContext, char* paramInfo)
{
	if(paramInfo == NULL || pSWMonHandlerContext == NULL) return;
	{
		char repMsg[2*1024] = {0};
		char errorStr[128] = {0};
		cagent_handle_t cagentHandle = pSWMonHandlerContext->susiHandlerContext.cagentHandle;
		int dataLen = sizeof(susi_comm_data_t) + sizeof(swm_get_sys_event_log_param_t);

		swm_get_sys_event_log_param_t eventParams;

		if(Parse_swm_get_event_log_req(paramInfo, &eventParams))
		{
			swm_get_sys_event_log_param_t *pGetEventLogParams = NULL;
			BOOL bRet = FALSE;
			SYSTEMTIME startSTime, endSTime;
			pGetEventLogParams = (swm_get_sys_event_log_param_t *)malloc(sizeof(swm_get_sys_event_log_param_t));
			memset(pGetEventLogParams, 0, sizeof(swm_get_sys_event_log_param_t));
			memcpy(pGetEventLogParams, &eventParams, sizeof(swm_get_sys_event_log_param_t));

			memset(&startSTime, 0, sizeof(SYSTEMTIME));
			memset(&endSTime, 0, sizeof(SYSTEMTIME));
			bRet = Str2SysTime(pGetEventLogParams->startTimeStr, "%d-%d-%d %d:%d:%d", &startSTime);
			if(bRet)
			{
				bRet = Str2SysTime(pGetEventLogParams->endTimeStr, "%d-%d-%d %d:%d:%d", &endSTime);
				if(bRet)
				{
					CAGENT_THREAD_TYPE eventLogQueryThreadHandle;
					if (app_os_thread_create(&eventLogQueryThreadHandle, EventLogQueryThreadStart, pGetEventLogParams) != 0)
					{
						sprintf_s(repMsg, sizeof(repMsg), "%s", "FAILED");
					}
				}
				else
				{
					sprintf_s(repMsg, sizeof(repMsg), "%s, endTimeStr:%s error!", "FAILED", pGetEventLogParams->endTimeStr);
				}
			}
			else
			{
				sprintf_s(repMsg, sizeof(repMsg), "%s, startTimeStr:%s error!", "FAILED", pGetEventLogParams->startTimeStr);
			}
		}
		else
		{
			memset(errorStr, 0, sizeof(errorStr));
			sprintf_s(errorStr, sizeof(errorStr), "Command(%d) parse error!", swm_get_event_log_req);
			{
				char * uploadRepJsonStr = NULL;
				char * str = errorStr;
				int jsonStrlen = Parser_string(str, &uploadRepJsonStr);
				if(jsonStrlen > 0 && uploadRepJsonStr != NULL)
				{
					g_sendcbf(&g_PluginInfo, swm_error_rep, uploadRepJsonStr, strlen(uploadRepJsonStr)+1, NULL, NULL);
				}
				if(uploadRepJsonStr)free(uploadRepJsonStr);	
			}

		}

		if(strlen(repMsg) > 0) 
		{
			char * uploadRepJsonStr = NULL;
			char * str = repMsg;
			int jsonStrlen = Parser_string(str, &uploadRepJsonStr);
			if(jsonStrlen > 0 && uploadRepJsonStr != NULL)
			{
				g_sendcbf(&g_PluginInfo, swm_get_event_log_rep, uploadRepJsonStr, strlen(uploadRepJsonStr)+1, NULL, NULL);
			}
			if(uploadRepJsonStr)free(uploadRepJsonStr);	
		}


	}
}
#endif //EVENT_LOG end
static void SWMKillPrc(sw_mon_handler_context_t * pSWMonHandlerContext, char * killPrcInfo)
{
	if(killPrcInfo == NULL || pSWMonHandlerContext == NULL) return;
	{
		unsigned int temp = 0;
		char repMsg[2*1024] = {0};
		char errorStr[128] = {0};
		//cagent_handle_t cagentHandle = pSWMonHandlerContext->susiHandlerContext.cagentHandle;
		//int dataLen = sizeof(susi_comm_data_t) + sizeof(unsigned int);

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
						sprintf_s(repMsg, sizeof(repMsg), "Process(PrcID-%d) kill failed!", *pPrcID);
					}
					else
					{
						sprintf_s(repMsg, sizeof(repMsg), "Process(PrcID-%d) kill success!", *pPrcID);
					}
				}
				else
				{
					sprintf_s(repMsg, sizeof(repMsg), "Process(PrcID-%d) not exist!", *pPrcID);
				}
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
		//cagent_handle_t cagentHandle = pSWMonHandlerContext->susiHandlerContext.cagentHandle;

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
					//mon_obj_info_node_t * findMonObjInfoNode = NULL;
					DWORD newPrcID = 0;
					newPrcID = RestartProcessWithID(*pPrcID);
					if(newPrcID > 0)
					{
						UpdateLogonUserPrcList(pSWMonHandlerContext->prcMonInfoList);
						sprintf_s(repMsg, sizeof(repMsg), "Process(PrcID-%d) restart success!", *pPrcID);
					}
					else
					{
						sprintf_s(repMsg, sizeof(repMsg), "Process(PrcID-%d) restart failed!", *pPrcID);
					}
				}
				else
				{
					sprintf_s(repMsg, sizeof(repMsg), "Process(PrcID-%d) not exist!", *pPrcID);
				}
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

static void SWMWhenDelThrSetToNormal(mon_obj_info_list monObjInfoList) 
{
	if(NULL == monObjInfoList) return;
	{
		//cagent_handle_t cagentHandle = SWMonHandlerContext.susiHandlerContext.cagentHandle;
		mon_obj_info_node_t * curNode = NULL;
		//char tmpRepMsg[1024*2] = {0};
		char * pRepMsg = NULL;
		int repBufLen = 0;
		int repMsgLen = 0;
		char tmpMsg[512] = {0};
		curNode = monObjInfoList->next;
		while(curNode)
		{
			memset(tmpMsg, 0, sizeof(tmpMsg));
			if(curNode->monObjInfo.cpuThrItem.isEnable && !curNode->monObjInfo.cpuThrItem.isNormal)
			{
				if(strlen(tmpMsg))sprintf_s(tmpMsg, sizeof(tmpMsg), "%s, %s normal", tmpMsg, curNode->monObjInfo.cpuThrItem.tagName);
				else sprintf_s(tmpMsg, sizeof(tmpMsg), "Process(PrcName:%s, PrcID:%d) %s normal", curNode->monObjInfo.prcName, 
					curNode->monObjInfo.prcID, curNode->monObjInfo.cpuThrItem.tagName);
				curNode->monObjInfo.cpuThrItem.isNormal = TRUE;
			}
			if(curNode->monObjInfo.memThrItem.isEnable && !curNode->monObjInfo.memThrItem.isNormal)
			{
				if(strlen(tmpMsg))sprintf_s(tmpMsg, sizeof(tmpMsg), "%s, %s normal", tmpMsg, curNode->monObjInfo.memThrItem.tagName);
				else sprintf_s(tmpMsg, sizeof(tmpMsg), "Process(PrcName:%s, PrcID:%d) %s normal", curNode->monObjInfo.prcName, 
					curNode->monObjInfo.prcID, curNode->monObjInfo.memThrItem.tagName);
				curNode->monObjInfo.memThrItem.isNormal = TRUE;
			}
			if(strlen(tmpMsg))
			{
				pRepMsg = (char *)DynamicStrCat(pRepMsg, &repBufLen,";", tmpMsg);
				/*if(pRepMsg)
					repMsgLen = strlen(pRepMsg) + 1; 
				//if(strlen(tmpRepMsg))
				if(repMsgLen > 1)
				{
					pRepMsg = (char *)DynamicStrCat(pRepMsg, ";", tmpMsg);
					//sprintf_s(tmpRepMsg, sizeof(tmpRepMsg), "%s;%s", tmpRepMsg, tmpMsg);
				}
				else
				{
					int tmpMsgLen = strlen(tmpMsg) + 1;
					pRepMsg = (char *)malloc(tmpMsgLen);
					if(pRepMsg)
					{
						memset(pRepMsg, 0, sizeof(char)*tmpMsgLen);
						sprintf_s(pRepMsg, sizeof(char)*tmpMsgLen, "%s",tmpMsg);
					}
					//sprintf_s(tmpRepMsg, sizeof(tmpRepMsg), "%s", tmpMsg);
				}*/
			}
			curNode = curNode->next;
		}
		//if(strlen(tmpRepMsg))
		if(pRepMsg)
		{
			BOOL isSWMNormal = TRUE;
			if(IsSWMThrNormal(&isSWMNormal))
			{
				SendNormalMsg(isSWMNormal,pRepMsg);
				free(pRepMsg);
				/*swm_thr_rep_info_t thrRepInfo;
				thrRepInfo.isTotalNormal = TRUE;
				memset(thrRepInfo.repInfo, 0, sizeof(thrRepInfo.repInfo));
				strcpy(thrRepInfo.repInfo, tmpRepMsg);
				{
					char * uploadRepJsonStr = NULL;
					char * str =  (char *)&thrRepInfo;
					int jsonStrlen = Parser_swm_mon_prc_event_rep(str, &uploadRepJsonStr);
					if(jsonStrlen > 0 && uploadRepJsonStr != NULL)
					{
						g_sendcbf(&g_PluginInfo, swm_mon_prc_event_rep, uploadRepJsonStr, strlen(uploadRepJsonStr)+1, NULL, NULL);
					}
					if(uploadRepJsonStr)free(uploadRepJsonStr);	
				}*/
			}
		}
	}
}

static void SWMDelAllMonObjs()
{
	char repMsg[32] = {0};
	sw_mon_handler_context_t *pSWMonHandlerContext =  &SWMonHandlerContext;
	app_os_EnterCriticalSection(&pSWMonHandlerContext->swMonObjCS);
	if(pSWMonHandlerContext->monObjInfoList)
	{
		SWMWhenDelThrSetToNormal(pSWMonHandlerContext->monObjInfoList);
		DeleteAllMonObjInfoNode(pSWMonHandlerContext->monObjInfoList);
	}
	MonObjInfoListFillConfig(SWMonHandlerContext.monObjInfoList, MonObjConfigPath);
	app_os_LeaveCriticalSection(&pSWMonHandlerContext->swMonObjCS);
	sprintf_s(repMsg, sizeof(repMsg), "%s", "Success");
	{
		char * uploadRepJsonStr = NULL;
		char * str = repMsg;
		int jsonStrlen = Parser_string(str, &uploadRepJsonStr,swm_del_all_mon_objs_rep);
		if(jsonStrlen > 0 && uploadRepJsonStr != NULL)
		{
			g_sendcbf(&g_PluginInfo, swm_del_all_mon_objs_rep, uploadRepJsonStr, strlen(uploadRepJsonStr)+1, NULL, NULL);
		}
		if(uploadRepJsonStr)free(uploadRepJsonStr);	
	}

}

static void SWMSetMonObjs(sw_mon_handler_context_t * pSWMonHandlerContext, char * requestData)
{
	if(requestData == NULL || pSWMonHandlerContext == NULL) return;
	{
		char repMsg[256] = {0};
		//cagent_handle_t cagentHandle = pSWMonHandlerContext->susiHandlerContext.cagentHandle;
		if(!SetSWMThrThreadRunning)
		{
			FILE * fPtr = fopen(MonObjTmpConfigPath, "wb");
			if(fPtr)
			{
				fwrite(requestData, 1, strlen(requestData)+1, fPtr);
				fclose(fPtr);
			}
			SetSWMThrThreadRunning = TRUE;
			if (app_os_thread_create(&SetSWMThrThreadHandle, SetSWMThrThreadStart, NULL) != 0)
			{
				SetSWMThrThreadRunning = FALSE;
				sprintf(repMsg, "%s", "Set swm threshold thread start error!");
				remove(MonObjTmpConfigPath);
			}
		}
		else
		{
			sprintf(repMsg, "%s", "Set swm threshold thread running!");
		}

		if(strlen(repMsg))
		{
			{
				char * uploadRepJsonStr = NULL;
				char * str = repMsg;
				int jsonStrlen = Parser_string(str, &uploadRepJsonStr,swm_set_mon_objs_rep);
				if(jsonStrlen > 0 && uploadRepJsonStr != NULL)
				{
					g_sendcbf(&g_PluginInfo, swm_set_mon_objs_rep, uploadRepJsonStr, strlen(uploadRepJsonStr)+1, NULL, NULL);
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

static BOOL UploadSysMonInfo()
{
	sw_mon_handler_context_t * pSWMonHandlerContext = (sw_mon_handler_context_t *)&SWMonHandlerContext;
	BOOL bRet = FALSE;
	cagent_status_t status;
	if(pSWMonHandlerContext == NULL) return bRet;
	app_os_EnterCriticalSection(&pSWMonHandlerContext->swSysMonCS);
	{
		char * uploadRepJsonStr = NULL;
		char * str = (char*)&pSWMonHandlerContext->sysMonInfo;
		int jsonStrlen = Parser_swm_get_smi_rep(str, &uploadRepJsonStr);
		if(jsonStrlen > 0 && uploadRepJsonStr != NULL)
		{
			status = g_sendcbf(&g_PluginInfo, swm_get_smi_rep, uploadRepJsonStr, strlen(uploadRepJsonStr)+1, NULL, NULL);
		}
		if(uploadRepJsonStr)free(uploadRepJsonStr);	
	}

	app_os_LeaveCriticalSection(&pSWMonHandlerContext->swSysMonCS);
	if(status == cagent_success)
	{
		bRet = TRUE;
	}
	return bRet;
}

static void DestroyPrcMonInfoList(prc_mon_info_node_t * head)
{
	if(NULL == head) return;
	DeleteAllPrcMonInfoNode(head);
	free(head); 
	head = NULL;
}


static int DeleteAllMonObjInfoNode(mon_obj_info_node_t * head)
{
	int iRet = -1;
	mon_obj_info_node_t * delNode = NULL;
	if(head == NULL) return iRet;

	delNode = head->next;
	while(delNode)
	{
		head->next = delNode->next;
		if(delNode->monObjInfo.prcName)
		{
			free(delNode->monObjInfo.prcName);
			delNode->monObjInfo.prcName = NULL;
		}
		if(delNode->monObjInfo.cpuActCmd)
		{
			free(delNode->monObjInfo.cpuActCmd);
			delNode->monObjInfo.cpuActCmd = NULL;
		}
		if(delNode->monObjInfo.memActCmd)
		{
			free(delNode->monObjInfo.memActCmd);
			delNode->monObjInfo.memActCmd = NULL;
		}
		ClearMonObjThr(&delNode->monObjInfo);
		if(delNode->monObjInfo.cpuThrItem.checkSourceValueList.head)
		{
			free(delNode->monObjInfo.cpuThrItem.checkSourceValueList.head);
			delNode->monObjInfo.cpuThrItem.checkSourceValueList.head = NULL;
		}
		if(delNode->monObjInfo.memThrItem.checkSourceValueList.head)
		{
			free(delNode->monObjInfo.memThrItem.checkSourceValueList.head);
			delNode->monObjInfo.memThrItem.checkSourceValueList.head = NULL;
		}
		free(delNode);
		delNode = head->next;
	}

	iRet = 0;
	return iRet;
}

static void DestroyMonObjInfoList(mon_obj_info_node_t * head)
{
	if(NULL == head) return;
	DeleteAllMonObjInfoNode(head);
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
	//static __int64 lastKernelTime = 0, lastUserTime = 0, lastTime = 0;
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
		//*memUsageKB = (long)(pmc.PagefileUsage/DIV);
      bRet = TRUE;
   }

   app_os_CloseHandle(hPrc);

   return bRet;
}
#endif

static void GetPrcMonInfo(prc_mon_info_node_t * head)
{
	prc_mon_info_node_t * curNode;
	long prcMemUsage = -1;
	if(NULL == head) return;
	curNode = head->next;
	while(curNode)
	{
		app_os_EnterCriticalSection(&SWMonHandlerContext.swPrcMonCS);
		if(curNode->prcMonInfo.prcName)
		{
			curNode->prcMonInfo.cpuUsage = GetProcessCPUUsageWithID(curNode->prcMonInfo.prcID, &curNode->prcMonInfo.prcCpuUsageLastTimes);
			curNode->prcMonInfo.isActive = IsProcessActiveWithIDEx(curNode->prcMonInfo.prcID);   
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

static void SWMWhenDelPrcCheckNormal(prc_mon_info_t * pDelPrcInfo, char * pNormalMsg)
{
	if(NULL == pDelPrcInfo|| pNormalMsg == NULL) return;
	{
		sw_mon_handler_context_t *pSoftwareMonHandlerContext = (sw_mon_handler_context_t *)&SWMonHandlerContext;
		mon_obj_info_node_t * curMonObjNode = NULL;
		char tmpMsg[512] = {0};
		app_os_EnterCriticalSection(&pSoftwareMonHandlerContext->swMonObjCS);
		curMonObjNode = FindMonObjInfoNodeWithID(pSoftwareMonHandlerContext->monObjInfoList, pDelPrcInfo->prcID);
		if(curMonObjNode)
		{
			if(curMonObjNode->monObjInfo.cpuThrItem.isEnable && !curMonObjNode->monObjInfo.cpuThrItem.isNormal)
			{
				if(strlen(tmpMsg))sprintf_s(tmpMsg, sizeof(tmpMsg), "%s, %s normal", tmpMsg, curMonObjNode->monObjInfo.cpuThrItem.tagName);
				else sprintf_s(tmpMsg, sizeof(tmpMsg), "Process(PrcName:%s, PrcID:%d) %s normal", curMonObjNode->monObjInfo.prcName, 
					curMonObjNode->monObjInfo.prcID, curMonObjNode->monObjInfo.cpuThrItem.tagName);
				curMonObjNode->monObjInfo.cpuThrItem.isNormal = TRUE;
			}
			if(curMonObjNode->monObjInfo.memThrItem.isEnable && !curMonObjNode->monObjInfo.memThrItem.isNormal)
			{
				if(strlen(tmpMsg))sprintf_s(tmpMsg, sizeof(tmpMsg), "%s, %s normal", tmpMsg, curMonObjNode->monObjInfo.memThrItem.tagName);
				else sprintf_s(tmpMsg, sizeof(tmpMsg), "Process(PrcName:%s, PrcID:%d) %s normal", curMonObjNode->monObjInfo.prcName, 
					curMonObjNode->monObjInfo.prcID, curMonObjNode->monObjInfo.memThrItem.tagName);
				curMonObjNode->monObjInfo.memThrItem.isNormal = TRUE;
			}
		} 
		app_os_LeaveCriticalSection(&pSoftwareMonHandlerContext->swMonObjCS);
		if(strlen(tmpMsg))
		{
			sprintf(pNormalMsg, "%s", tmpMsg);
		}
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
		//Wei.Gang add
		//char normalMsg[4*1024] = {0};
		char * pNormalMsg = NULL;
		int normalBufLen = 0;
		int normalMsgLen = 0;
		char tmpMsg[512] = {0};
		//Wei.Gang add end
		while(curNode)
		{
			if(curNode->prcMonInfo.isValid == 0)
			{
				frontNode->next = curNode->next;
				delNode = curNode;
				//Wei.gang add 
				memset(tmpMsg, 0, sizeof(tmpMsg));
				SWMWhenDelPrcCheckNormal(&delNode->prcMonInfo, tmpMsg);
				if(strlen(tmpMsg))
				{
					pNormalMsg = (char *)DynamicStrCat(pNormalMsg, &normalBufLen,";", tmpMsg);
					/*if(pNormalMsg)
						normalMsgLen = strlen(pNormalMsg)+1;
					//if(strlen(normalMsg))
					if(normalMsgLen > 1)
					{
						pNormalMsg = (char *)DynamicStrCat(pNormalMsg, ";", tmpMsg);
						//sprintf(normalMsg, "%s;%s", normalMsg, tmpMsg);
					}
					else
					{
						int tmpMsgLen = strlen(tmpMsg) + 1;
						pNormalMsg = (char *)malloc(tmpMsgLen);
						if(pNormalMsg)
						{
							memset(pNormalMsg, 0, sizeof(char)*tmpMsgLen);
							sprintf_s(pNormalMsg, sizeof(char)*tmpMsgLen, "%s",tmpMsg);
						}
						//sprintf(normalMsg, "%s", tmpMsg);
					}*/
				}
				//Wei.gang add end
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
			}
			else
			{
				frontNode = curNode;
			}
			curNode = frontNode->next;
		}
		//if(strlen(normalMsg))
		if(pNormalMsg)
		{
			BOOL isSWMNormal = TRUE;
			if(IsSWMThrNormal(&isSWMNormal))
			{
				SendNormalMsg(isSWMNormal,pNormalMsg);
				free(pNormalMsg);
				/*swm_thr_rep_info_t thrRepInfo;
				thrRepInfo.isTotalNormal = true;
				memset(thrRepInfo.repInfo, 0, sizeof(thrRepInfo.repInfo));
				strcpy(thrRepInfo.repInfo, normalMsg);
				{
					char * uploadRepJsonStr = NULL;
					char * str = (char *)&thrRepInfo;
					int jsonStrlen = Parser_swm_mon_prc_event_rep(str, &uploadRepJsonStr);
					if(jsonStrlen > 0 && uploadRepJsonStr != NULL)
					{
						g_sendcbf(&g_PluginInfo, swm_mon_prc_event_rep, uploadRepJsonStr, strlen(uploadRepJsonStr)+1, NULL, NULL);
					}
					if(uploadRepJsonStr)free(uploadRepJsonStr);	
				}*/
			}
		}
	}
	return bRet = TRUE;
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
/*#else
BOOL IsWorkStationLocked()
{
	// note: we can't call OpenInputDesktop directly because it's not
	BOOL nRet = FALSE;
	char *tmpName = getlogin();
	nRet = (tmpName == NULL)?TRUE:FALSE;
	return nRet;
}*/
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
		memset(newNode, 0, sizeof(prc_mon_info_node_t));
		if(prcMonInfo->prcName)
		{
			int prcNameLen = strlen(prcMonInfo->prcName);
			newNode->prcMonInfo.prcName = (char *)malloc(prcNameLen + 1);
			memset(newNode->prcMonInfo.prcName, 0, prcNameLen + 1);
			memcpy(newNode->prcMonInfo.prcName, prcMonInfo->prcName, prcNameLen);
		}
		if(prcMonInfo->ownerName)
		{
			int ownerNameLen = strlen(prcMonInfo->ownerName);
			newNode->prcMonInfo.ownerName = (char *)malloc(ownerNameLen + 1);
			memset(newNode->prcMonInfo.ownerName, 0, ownerNameLen + 1);
			memcpy(newNode->prcMonInfo.ownerName, prcMonInfo->ownerName, ownerNameLen);
		}
		newNode->prcMonInfo.prcID = prcMonInfo->prcID;
		newNode->prcMonInfo.isValid = prcMonInfo->isValid;

		newNode->prcMonInfo.cpuUsage = 0;
		newNode->prcMonInfo.memUsage = 0;
		newNode->prcMonInfo.isActive = FALSE;
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
/*
#ifdef WIN32
HANDLE GetProcessHandle(char * processName)
{
	HANDLE hPrc = NULL;
	PROCESSENTRY32 pe;
	HANDLE hSnapshot=NULL;
	if(NULL == processName) return hPrc;
	hSnapshot=app_os_CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS,0);
	pe.dwSize=sizeof(PROCESSENTRY32);
	if(!app_os_Process32First(hSnapshot,&pe))
		return hPrc;
	while(TRUE)
	{
		pe.dwSize=sizeof(PROCESSENTRY32);
		if(app_os_Process32Next(hSnapshot,&pe)==FALSE)
			break;
		if(strcmp(pe.szExeFile, processName)==0)
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
			break;
		}
	}
	if(hSnapshot) app_os_CloseHandle(hSnapshot);
	return hPrc;
}
#endif
*/
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

void UpdatePrcList_CheckAllUsers(prc_mon_info_node_t * head, sw_mon_handler_context_t * pSoftwareMonHandlerContext)
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
						memset(prcMonInfo.prcName, 0, tmpLen + 1);
						memcpy(prcMonInfo.prcName, pe.szExeFile, tmpLen);
						tmpLen = strlen(procUserName);
						if(tmpLen)
						{
							prcMonInfo.ownerName = (char*)malloc(tmpLen + 1);
							memset(prcMonInfo.ownerName, 0, tmpLen + 1);
							memcpy(prcMonInfo.ownerName, procUserName, tmpLen);
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

void UpdatePrcList_CheckUser(prc_mon_info_node_t * head, sw_mon_handler_context_t * pSoftwareMonHandlerContext, char * chkUserName)
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
						memset(prcMonInfo.prcName, 0, tmpLen + 1);
						memcpy(prcMonInfo.prcName, pe.szExeFile, tmpLen);
						tmpLen = strlen(procUserName);
						if(tmpLen)
						{
							prcMonInfo.ownerName = (char*)malloc(tmpLen + 1);
							memset(prcMonInfo.ownerName, 0, tmpLen + 1);
							memcpy(prcMonInfo.ownerName, procUserName, tmpLen);
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

void UpdatePrcList_All(prc_mon_info_node_t * head, sw_mon_handler_context_t * pSoftwareMonHandlerContext)
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
						memset(prcMonInfo.prcName, 0, tmpLen + 1);
						memcpy(prcMonInfo.prcName, pe.szExeFile, tmpLen);
						tmpLen = strlen(procUserName);
						if(tmpLen)
						{
							prcMonInfo.ownerName = (char*)malloc(tmpLen + 1);
							memset(prcMonInfo.ownerName, 0, tmpLen + 1);
							memcpy(prcMonInfo.ownerName, procUserName, tmpLen);
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
				UpdatePrcList_All(head, pSoftwareMonHandlerContext);
				break;
			case CFG_FLAG_SEND_PRCINFO_BY_USER:
				UpdatePrcList_CheckUser(head, pSoftwareMonHandlerContext, pSoftwareMonHandlerContext->sysUserName);
				break;
			default://CFG_FLAG_SEND_PRCINFO_BY_ALLUSERS:
				UpdatePrcList_CheckAllUsers(head, pSoftwareMonHandlerContext);
				break;
		}
			/*
			char logonUserName[32] = {0};
			char procUserName[32] = {0};
			app_os_GetSysLogonUserName(logonUserName, sizeof(logonUserName));
			if(strlen(logonUserName))
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
						if(strlen(procUserName) && !strcmp(logonUserName, procUserName))
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
								memset(prcMonInfo.prcName, 0, tmpLen + 1);
								memcpy(prcMonInfo.prcName, pe.szExeFile, tmpLen);
								app_os_EnterCriticalSection(&pSoftwareMonHandlerContext->swPrcMonCS);
								InsertPrcMonInfoNode(head, &prcMonInfo);
								app_os_LeaveCriticalSection(&pSoftwareMonHandlerContext->swPrcMonCS);
								if(prcMonInfo.prcName)
								{
									free(prcMonInfo.prcName);
									prcMonInfo.prcName = NULL;
								}
							}
						}
						if(procHandle) app_os_CloseHandle(procHandle);
						app_os_sleep(10);
						pe.dwSize=sizeof(PROCESSENTRY32);
						if(app_os_Process32Next(hSnapshot,&pe)==FALSE)
							break;
					}
				}
				if(hSnapshot) app_os_CloseSnapShot32Handle(hSnapshot);//app_os_CloseHandle(hSnapshot);
			}
			else
				pSoftwareMonHandlerContext->isUserLogon = FALSE;*/
		}
//#ifdef WIN32
		//if(!IsWorkStationLocked() && !ProcessCheck("LogonUI.exe"))
//		else
//			pSoftwareMonHandlerContext->isUserLogon = FALSE;
//#endif
		app_os_EnterCriticalSection(&pSoftwareMonHandlerContext->swPrcMonCS);
		UpdatePrcList(head);
		app_os_LeaveCriticalSection(&pSoftwareMonHandlerContext->swPrcMonCS);
		bRet = TRUE;
	}
	return bRet;
}

static BOOL IsSWMThrNormal(BOOL * isNormal)
{
	BOOL bRet = FALSE;
	sw_mon_handler_context_t *pSoftwareMonHandlerContext = (sw_mon_handler_context_t *)&SWMonHandlerContext;
	if(isNormal == NULL || pSoftwareMonHandlerContext == NULL || pSoftwareMonHandlerContext->monObjInfoList == NULL) return bRet;
	{
		mon_obj_info_node_t * curMonObjInfoNode = pSoftwareMonHandlerContext->monObjInfoList->next;
		*isNormal = TRUE;
		while(curMonObjInfoNode)
		{
			if(curMonObjInfoNode->monObjInfo.isValid)
			{
				if((curMonObjInfoNode->monObjInfo.cpuThrItem.isEnable && !curMonObjInfoNode->monObjInfo.cpuThrItem.isNormal)
					|| (curMonObjInfoNode->monObjInfo.memThrItem.isEnable && !curMonObjInfoNode->monObjInfo.memThrItem.isNormal))
				{
					*isNormal = FALSE;
					break;
				}
				if(!curMonObjInfoNode->monObjInfo.prcResponse)
				{
					*isNormal = FALSE;
					break;
				}
			}
			curMonObjInfoNode = curMonObjInfoNode->next;
		}
	}
	return bRet = TRUE;
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

static void ClearMonObjThr(mon_obj_info_t * curMonObj)
{
	if(curMonObj != NULL && curMonObj->cpuThrItem.checkSourceValueList.head)
	{
		check_value_node_t * frontValueNode = curMonObj->cpuThrItem.checkSourceValueList.head;
		check_value_node_t * delValueNode = frontValueNode->next;
		while(delValueNode)
		{
			frontValueNode->next = delValueNode->next;
			free(delValueNode);
			delValueNode = frontValueNode->next;
		}
		curMonObj->cpuThrItem.checkSourceValueList.nodeCnt = 0;
		curMonObj->cpuThrItem.checkResultValue.vi = DEF_INVALID_VALUE;
		curMonObj->cpuThrItem.repThrTime = 0;
		curMonObj->cpuThrItem.isNormal = TRUE;
	}
	if(curMonObj != NULL && curMonObj->memThrItem.checkSourceValueList.head)
	{
		check_value_node_t * frontValueNode = curMonObj->memThrItem.checkSourceValueList.head;
		check_value_node_t * delValueNode = frontValueNode->next;
		while(delValueNode)
		{
			frontValueNode->next = delValueNode->next;
			free(delValueNode);
			delValueNode = frontValueNode->next;
		}
		curMonObj->memThrItem.checkSourceValueList.nodeCnt = 0;
		curMonObj->memThrItem.checkResultValue.vi = DEF_INVALID_VALUE;
		curMonObj->memThrItem.repThrTime = 0;
		curMonObj->memThrItem.isNormal = TRUE;
	}
}

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
		//WeiGang add to get DISPLAY eviroment, but service saagent have not it. Keep src for future.
		/*fp = NULL;
	    sprintf(cmdBuf,"su -c 'env|grep DISPLAY' %s",logonUserName);//DISPLAY=:0 
	    //sprintf(cmdBuf,"su -c env %s",logonUserName);//DISPLAY=:0 test
	    if((fp=popen(cmdBuf,"r"))==NULL)
	    {
		printf("Get Display port failed,%s\n",cmdBuf);
		SoftwareMonitorLog(g_loghandle, Normal,"Get Display port failed");	
	    }
	    else
	    {
		char buf_tmp[32] = {0}; 
		//while(fgets(buf_tmp,sizeof(buf_tmp),fp))
		//{
		//    SoftwareMonitorLog(g_loghandle, Normal, "[Wei.Gang env] env is %s!\n" ,buf_tmp);
		//}
		if(fgets(buf_tmp,sizeof(buf_tmp),fp))
		{
		    if(strstr(buf_tmp,"DISPLAY"))
		      strcpy(displayPort,buf_tmp);
		    SoftwareMonitorLog(g_loghandle, Normal, "[restartPro] get DISPLAY dport is %s, buf_tmp is %s!\n"
		                      ,displayPort,buf_tmp);
		}
	    }
	    pclose(fp);*/// Wei.Gang add end

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
		dwPrcID = getPIDByName(cmdLine);
	}
	else
		printf("restart process failed,%s",cmdBuf);
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
			{
				if(curMonObjInfo->cpuAct != prc_act_unknown)
				{
					prcAct = curMonObjInfo->cpuAct;
					if(curMonObjInfo->cpuActCmd)
					{
						strcpy(prcActCmd, curMonObjInfo->cpuActCmd);
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
		case prc_thr_type_mem:
			{
				prcAct = curMonObjInfo->memAct;
				if(curMonObjInfo->memActCmd)
				{
					strcpy(prcActCmd, curMonObjInfo->memActCmd);
				}
				break;
			}
		case prc_thr_type_unknown:
			break;
		default:
			break;
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
					ClearMonObjThr(curMonObjInfo);
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
		case prc_act_unknown:
			break;
		}
	}
	return bRet;
}

static int DeleteMonObjInfoNodeWithID(mon_obj_info_node_t * head, int prcID)
{
	int iRet = -1;
	mon_obj_info_node_t * delNode = NULL;
	mon_obj_info_node_t * p = NULL;
	if(head == NULL) return iRet;
	iRet = 1;
	p = head;
	delNode = head->next;
	while(delNode)
	{
		if(delNode->monObjInfo.prcID == prcID)
		{
			p->next = delNode->next;
			if(delNode->monObjInfo.prcName)
			{
				free(delNode->monObjInfo.prcName);
				delNode->monObjInfo.prcName = NULL;
			}
			if(delNode->monObjInfo.cpuActCmd)
			{
				free(delNode->monObjInfo.cpuActCmd);
				delNode->monObjInfo.cpuActCmd = NULL;
			}
			if(delNode->monObjInfo.memActCmd)
			{
				free(delNode->monObjInfo.memActCmd);
				delNode->monObjInfo.memActCmd = NULL;
			}
			ClearMonObjThr(&delNode->monObjInfo);
			if(delNode->monObjInfo.cpuThrItem.checkSourceValueList.head)
			{
				free(delNode->monObjInfo.cpuThrItem.checkSourceValueList.head);
				delNode->monObjInfo.cpuThrItem.checkSourceValueList.head = NULL;
			}
			if(delNode->monObjInfo.memThrItem.checkSourceValueList.head)
			{
				free(delNode->monObjInfo.memThrItem.checkSourceValueList.head);
				delNode->monObjInfo.memThrItem.checkSourceValueList.head = NULL;
			}
			free(delNode);
			delNode = p->next;
			iRet = 0;
			continue;
		}
		else
		{
			p = delNode;
			delNode = delNode->next;
		}
	}
	return iRet;
}


static BOOL CheckSourceValue(hwm_thr_item_t * pThrItem, check_value_t * pCheckValue, value_type_t valueType)
{
	BOOL bRet = FALSE;
	if(pThrItem == NULL || pCheckValue == NULL) return bRet;
	{
		long long nowTime = time(NULL);
		pThrItem->checkResultValue.vi = DEF_INVALID_VALUE;
		if(pThrItem->checkSourceValueList.head == NULL)
		{
			pThrItem->checkSourceValueList.head = (check_value_node_t *)malloc(sizeof(check_value_node_t));
			pThrItem->checkSourceValueList.nodeCnt = 0;
			pThrItem->checkSourceValueList.head->checkValueTime = DEF_INVALID_TIME;
			pThrItem->checkSourceValueList.head->ckV.vi = DEF_INVALID_VALUE;
			pThrItem->checkSourceValueList.head->next = NULL;
		}

		if(pThrItem->checkSourceValueList.nodeCnt > 0)
		{
			long long minCkvTime = 0;
			check_value_node_t * curNode = pThrItem->checkSourceValueList.head->next;
			minCkvTime = curNode->checkValueTime;
			while(curNode)
			{
				if(curNode->checkValueTime < minCkvTime)  minCkvTime = curNode->checkValueTime;
				curNode = curNode->next; 
			}

			if(nowTime - minCkvTime >= pThrItem->lastingTimeS)
			{
				switch(pThrItem->checkType)
				{
				case ck_type_avg:
					{
						check_value_node_t * curNode = pThrItem->checkSourceValueList.head->next;
						float avgTmpF = 0;
						int avgTmpI = 0;
						while(curNode)
						{
							if(curNode->ckV.vi != DEF_INVALID_VALUE) 
							{
								switch(valueType)
								{
								case value_type_float:
									{
										avgTmpF += curNode->ckV.vf;
										break;
									}
								case value_type_int:
									{
										avgTmpI += curNode->ckV.vi;
										break;
									}
								default: break;
								}
							}
							curNode = curNode->next; 
						}
						if(pThrItem->checkSourceValueList.nodeCnt > 0)
						{
							switch(valueType)
							{
							case value_type_float:
								{
									avgTmpF = avgTmpF/pThrItem->checkSourceValueList.nodeCnt;
									pThrItem->checkResultValue.vf = avgTmpF;
									bRet = TRUE;
									break;
								}
							case value_type_int:
								{
									avgTmpI = avgTmpI/pThrItem->checkSourceValueList.nodeCnt;
									pThrItem->checkResultValue.vi = avgTmpI;
									bRet = TRUE;
									break;
								}
							default: break;
							}
						}
						break;
					}
				case ck_type_max:
					{
						check_value_node_t * curNode = pThrItem->checkSourceValueList.head->next;
						float maxTmpF = -999;
						int maxTmpI = -999;
						while(curNode)
						{
							switch(valueType)
							{
							case value_type_float:
								{
									if(curNode->ckV.vf > maxTmpF) maxTmpF = curNode->ckV.vf;
									break;
								}
							case value_type_int:
								{
									if(curNode->ckV.vi > maxTmpI) maxTmpI = curNode->ckV.vi;
									break;
								}
							default: break;
							}
							curNode = curNode->next; 
						}
						switch(valueType)
						{
						case value_type_float:
							{
								if(maxTmpF > -999)
								{
									pThrItem->checkResultValue.vf = maxTmpF;
									bRet = TRUE;
								}
								break;
							}
						case value_type_int:
							{
								if(maxTmpI > -999)
								{
									pThrItem->checkResultValue.vi = maxTmpI;
									bRet = TRUE;
								}
								break;
							}
						default: break;
						}
						break;
					}
				case ck_type_min:
					{
						check_value_node_t * curNode = pThrItem->checkSourceValueList.head->next;
						float minTmpF = 99999;
						int minTmpI = 99999;
						while(curNode)
						{
							switch(valueType)
							{
							case value_type_float:
								{
									if(curNode->ckV.vf < minTmpF) minTmpF = curNode->ckV.vf;
									break;
								}
							case value_type_int:
								{
									if(curNode->ckV.vi < minTmpI) minTmpI = curNode->ckV.vi;
									break;
								}
							default: break;
							}
							curNode = curNode->next; 
						}
						switch(valueType)
						{
						case value_type_float:
							{
								if(minTmpF < 99999)
								{
									pThrItem->checkResultValue.vf = minTmpF;
									bRet = TRUE;
								}
								break;
							}
						case value_type_int:
							{
								if(minTmpI < 99999)
								{
									pThrItem->checkResultValue.vi = minTmpI;
									bRet = TRUE;
								}
								break;
							}
						default: break;
						}

						break;
					}
				default: break;
				}

				{
					check_value_node_t * frontNode = pThrItem->checkSourceValueList.head;
					check_value_node_t * curNode = frontNode->next;
					check_value_node_t * delNode = NULL;
					while(curNode)
					{
						if(nowTime - curNode->checkValueTime >= pThrItem->lastingTimeS)
						{
							delNode = curNode;
							frontNode->next  = curNode->next;
							curNode = frontNode->next;
							free(delNode);
							pThrItem->checkSourceValueList.nodeCnt--;
							delNode = NULL;
						}
						else
						{
							frontNode = curNode;
							curNode = frontNode->next;
						}
					}
				}
			}
		}
		{
			check_value_node_t * head = pThrItem->checkSourceValueList.head;
			check_value_node_t * newNode = (check_value_node_t *)malloc(sizeof(check_value_node_t));
			newNode->checkValueTime = nowTime;
			newNode->ckV.vi = DEF_INVALID_VALUE;
			switch(valueType)
			{
			case value_type_float:
				{
					newNode->ckV.vf = (*pCheckValue).vf;
					break;
				}
			case value_type_int:
				{
					newNode->ckV.vi = (*pCheckValue).vi;
					break;
				}
			default: break;
			}
			newNode->next = head->next;
			head->next = newNode;
			pThrItem->checkSourceValueList.nodeCnt++;
		}
	}
	return bRet;
}

static BOOL ThrItemCheck(swm_thr_item_t * pThrItem, int valueI, char * checkMsg, BOOL * checkRet)
{
	BOOL bRet = FALSE;
	if(pThrItem == NULL || checkRet == NULL || checkMsg == NULL) return bRet;
	if(pThrItem->isEnable)
	{
		BOOL isTrigger = FALSE;
		BOOL triggerMax = FALSE;
		BOOL triggerMin = FALSE;
		char tmpRetMsg[1024] = {0};
		char checkTypeStr[32] = {0};
		switch(pThrItem->checkType)
		{
		case ck_type_avg:
			{
				sprintf(checkTypeStr, "avg");
				break;
			}
		case ck_type_max:
			{
				sprintf(checkTypeStr, "max");
				break;
			}
		case ck_type_min:
			{
				sprintf(checkTypeStr, "min");
				break;
			}
		default: break;
		}
		{
			check_value_t checkValue;
			checkValue.vi = valueI;
			if(CheckSourceValue(pThrItem, &checkValue, value_type_int) && pThrItem->checkResultValue.vi != DEF_INVALID_VALUE)
			{
				int usageV = pThrItem->checkResultValue.vi;
				if(pThrItem->thresholdType & DEF_THR_MAX_TYPE)
				{
					if(pThrItem->maxThreshold != DEF_INVALID_VALUE && (usageV > pThrItem->maxThreshold))
					{
						sprintf(tmpRetMsg, "%s(%s:%d)>maxThreshold(%.0f)", pThrItem->tagName, checkTypeStr, usageV, pThrItem->maxThreshold);
						triggerMax = TRUE;
					}
				}
				if(pThrItem->thresholdType & DEF_THR_MIN_TYPE)
				{
					if(pThrItem->minThreshold != DEF_INVALID_VALUE && (usageV < pThrItem->minThreshold))
					{
						if(strlen(tmpRetMsg)) sprintf(tmpRetMsg, "%s and %s(%s:%d)<minThreshold(%.0f)", tmpRetMsg, pThrItem->tagName, checkTypeStr, usageV, pThrItem->minThreshold);
						else sprintf(tmpRetMsg, "%s(%s:%d)<minThreshold(%.0f)", pThrItem->tagName, checkTypeStr, usageV, pThrItem->minThreshold);
						triggerMin = TRUE;
					}
				}
			}
		}

		switch(pThrItem->thresholdType)
		{
		case DEF_THR_MAX_TYPE:
			{
				isTrigger = triggerMax;
				break;
			}
		case DEF_THR_MIN_TYPE:
			{
				isTrigger = triggerMin;
				break;
			}
		case DEF_THR_MAXMIN_TYPE:
			{
				isTrigger = triggerMin || triggerMax;
				break;
			}
		}

		if(isTrigger)
		{
			long long nowTime = time(NULL);
			if(pThrItem->repThrTime == 0 || pThrItem->intervalTimeS == DEF_INVALID_TIME || pThrItem->intervalTimeS == 0 || nowTime - pThrItem->repThrTime > pThrItem->intervalTimeS)
			{
				pThrItem->repThrTime = nowTime;
				pThrItem->isNormal = FALSE;
				*checkRet = TRUE;
				bRet = TRUE;
			}
		}
		else
		{
			if(!pThrItem->isNormal && pThrItem->checkResultValue.vi != DEF_INVALID_VALUE)
			{
				memset(tmpRetMsg, 0, sizeof(tmpRetMsg));
				sprintf(tmpRetMsg, "%s(%s) normal!", pThrItem->tagName, checkTypeStr);
				pThrItem->isNormal = TRUE;
				*checkRet = FALSE;
				bRet = TRUE;
			}
		}
		if(!bRet) sprintf(checkMsg,"");
		else sprintf(checkMsg, "%s", tmpRetMsg);
	}
	return bRet;
}

static prc_thr_type_t MonObjThrCheck(sw_thr_check_params_t * pThrCheckParams, BOOL *checkRet)
{
	prc_thr_type_t tRet = prc_thr_type_unknown;
	BOOL bRet = FALSE;
	mon_obj_info_t * pMonObjInfo = NULL;
	prc_mon_info_t * pPrcMonInfo = NULL;
	if(pThrCheckParams == NULL) return bRet;
	pMonObjInfo = pThrCheckParams->pMonObjInfo;
	pPrcMonInfo = pThrCheckParams->pPrcMonInfo;
	if(pMonObjInfo == NULL || pPrcMonInfo == NULL) return bRet;
	{
		memset(pThrCheckParams->checkMsg, 0, sizeof(pThrCheckParams->checkMsg));
		if(!bRet)
		{
			if(!pPrcMonInfo->isActive && pMonObjInfo->prcResponse)
			{
				sprintf(pThrCheckParams->checkMsg, "%s", "Process no response!");
				pThrCheckParams->pMonObjInfo->prcResponse = FALSE;
				bRet = TRUE;
				tRet = prc_thr_type_active;
			}
			else if(pPrcMonInfo->isActive && !pMonObjInfo->prcResponse)
			{
				sprintf(pThrCheckParams->checkMsg,"%s", "Process recovery response!");
				pThrCheckParams->pMonObjInfo->prcResponse = TRUE;
				bRet = TRUE;
				tRet = prc_thr_type_active;
			}
		}

		if(!bRet && pMonObjInfo->cpuThrItem.isEnable)
		{
			*checkRet = FALSE;
			bRet = ThrItemCheck(&pMonObjInfo->cpuThrItem, pPrcMonInfo->cpuUsage, pThrCheckParams->checkMsg, checkRet);
			if(bRet)
			{
				tRet = prc_thr_type_cpu;
			}
		} 
		if(!bRet && pMonObjInfo->memThrItem.isEnable)
		{
			*checkRet = FALSE;
			bRet = ThrItemCheck(&pMonObjInfo->memThrItem, pPrcMonInfo->memUsage, pThrCheckParams->checkMsg, checkRet);
			if(bRet)
			{
				tRet = prc_thr_type_mem;
			}
		}
	}
	return tRet;
}


static BOOL GetPrcMonEventMsg(sw_thr_check_params_t * pCurThrCheckParams, prc_thr_type_t prcThrType, BOOL checkRet, char * eventMsg)
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
	sprintf_s(eventStr, sizeof(eventStr), "Process(PrcName:%s, PrcID:%d) CpuUsage:%d, MemUsage:%ld", curPrcMonInfo->prcName, curMonObjInfo->prcID, 
		curPrcMonInfo->cpuUsage, curPrcMonInfo->memUsage);
	if(curPrcMonInfo->isActive)
	{
		sprintf_s(eventStr, sizeof(eventStr), "%s, PrcStatus:Running", eventStr);
	}
	else
	{
		sprintf_s(eventStr, sizeof(eventStr), "%s, PrcStatus:NoResponse", eventStr);
	}

	if(strlen(pCurThrCheckParams->checkMsg))
	{
		sprintf_s(eventStr, sizeof(eventStr), "%s (%s)", eventStr, pCurThrCheckParams->checkMsg);
	}

	if(checkRet)
	{
		switch(prcThrType)
		{
		case prc_thr_type_active:
			{
				if(curMonObjInfo->cpuAct != prc_act_unknown)
				{
					prcAct = curMonObjInfo->cpuAct;
					if(curMonObjInfo->cpuActCmd)
					{
						strcpy(prcActCmd, curMonObjInfo->cpuActCmd);
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
		case prc_thr_type_mem:
			{
				prcAct = curMonObjInfo->memAct;
				if(curMonObjInfo->memActCmd)
				{
					strcpy(prcActCmd, curMonObjInfo->memActCmd);
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
				sprintf_s(eventStr, sizeof(eventStr), "%s, will stop", eventStr);
				break;
			}
		case prc_act_restart:
			{
				sprintf_s(eventStr, sizeof(eventStr), "%s, will restart", eventStr);
				break;
			}
		case prc_act_with_cmd:
			{
				sprintf_s(eventStr, sizeof(eventStr), "%s, will execute cmd: %s", eventStr, prcActCmd);
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
		strcpy(eventMsg, eventStr);
	}


	// 	if(strlen(eventStr))
	// 	{
	// 		BOOL isSWMNormal = TRUE;
	// 		if(IsSWMThrNormal(&isSWMNormal))
	// 		{
	// 			swm_thr_rep_info_t thrRepInfo;
	// 			int repInfoLen = strlen(eventStr)+1;
	// 			thrRepInfo.isTotalNormal = isSWMNormal;
	// 			memset(thrRepInfo.repInfo, 0, sizeof(thrRepInfo.repInfo));
	// 			strcpy(thrRepInfo.repInfo, eventStr);
	// 			SoftwareMonSend(pSoftwareMonHandlerContext->susiHandlerContext.cagentHandle, swm_mon_prc_event_rep, (char *)&thrRepInfo, sizeof(swm_thr_rep_info_t));
	// 		}
	// 	}
	return bRet = TRUE;
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
		int repMsgLen = 0;
		//char repMsg[2*1024] = {0};
		char *pRepMsg = NULL;
		int repMsgBufLen = 0;
		char tmpEventMsg[512] = {0};
		//BOOL isSWMThrNormal = TRUE;
		mon_obj_info_node_t * curMonObjInfoNode = pSoftwareMonHandlerContext->monObjInfoList->next;
		prc_mon_info_node_t * curPrcMonInfoNode = NULL;
		BOOL isPrcMonInfoFind = FALSE;
		app_os_EnterCriticalSection(&pSoftwareMonHandlerContext->swMonObjCS);
		while(curMonObjInfoNode)
		{
			memset(tmpEventMsg, 0, sizeof(tmpEventMsg));
			if(!pSoftwareMonHandlerContext->isSoftwareMonThreadRunning) break;
			if(curMonObjInfoNode->monObjInfo.cpuThrItem.isEnable || curMonObjInfoNode->monObjInfo.memThrItem.isEnable)
			{
				if(curMonObjInfoNode->monObjInfo.isValid == 1)
				{
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
									bRet = GetPrcMonEventMsg(&swThrCheckParams, prcThrType, checkRet, tmpEventMsg);
									if(bRet)
									{
										if(checkRet)
										{
											//isSWMThrNormal = FALSE;
											bRet = PrcMonOnEvent(&curMonObjInfoNode->monObjInfo, prcThrType);
											if(!bRet)
											{
												if(((prcThrType == prc_thr_type_active || prcThrType == prc_thr_type_cpu) && (curMonObjInfoNode->monObjInfo.cpuAct > prc_act_unknown && curMonObjInfoNode->monObjInfo.cpuAct<=prc_act_with_cmd)) ||
													(prcThrType == prc_thr_type_mem && (curMonObjInfoNode->monObjInfo.memAct > prc_act_unknown && curMonObjInfoNode->monObjInfo.memAct<=prc_act_with_cmd)))
												{
													sprintf_s(tmpEventMsg, sizeof(tmpEventMsg), "%s,the act failed", tmpEventMsg);
												}
											}
											else
											{
												if(((prcThrType == prc_thr_type_active || prcThrType == prc_thr_type_cpu) && (curMonObjInfoNode->monObjInfo.cpuAct > prc_act_unknown && curMonObjInfoNode->monObjInfo.cpuAct<=prc_act_with_cmd)) ||
													(prcThrType == prc_thr_type_mem && (curMonObjInfoNode->monObjInfo.memAct > prc_act_unknown && curMonObjInfoNode->monObjInfo.memAct<=prc_act_with_cmd)))
												{
													sprintf_s(tmpEventMsg, sizeof(tmpEventMsg), "%s,the act successfully", tmpEventMsg);
												}
											}
										}
									}
									else
									{
										sprintf_s(tmpEventMsg, sizeof(tmpEventMsg), "Process(PrcName:%s, PrcID:%d) threshold is reached, but report event msg failed!", curMonObjInfoNode->monObjInfo.prcName,
											curMonObjInfoNode->monObjInfo.prcID);
									}
								}
							}
						}
						curPrcMonInfoNode = curPrcMonInfoNode->next;
						app_os_LeaveCriticalSection(&pSoftwareMonHandlerContext->swPrcMonCS);
						app_os_sleep(10);
					}

					if(!isPrcMonInfoFind)
					{
						if(curMonObjInfoNode->monObjInfo.isValid == 1)
						{
							curMonObjInfoNode->monObjInfo.isValid = 0;
							//sprintf_s(repMsg, sizeof(repMsg), "Process(PrcName-%s) is not found, the monobj invalid!", curMonObjInfoNode->monObjInfo.prcName);
						}
						{//delete invalid mon obj(but must keep one<same prcName> exist)
							mon_obj_info_node_t * tmpMonObjInfoNode = pSoftwareMonHandlerContext->monObjInfoList->next;
							while(tmpMonObjInfoNode)
							{
								if(!strcmp(tmpMonObjInfoNode->monObjInfo.prcName, curMonObjInfoNode->monObjInfo.prcName) &&
									tmpMonObjInfoNode->monObjInfo.prcID != curMonObjInfoNode->monObjInfo.prcID) break;
								tmpMonObjInfoNode = tmpMonObjInfoNode->next;
							}
							if(tmpMonObjInfoNode)
							{
								int delPrcId = curMonObjInfoNode->monObjInfo.prcID;
								curMonObjInfoNode = curMonObjInfoNode->next;
								DeleteMonObjInfoNodeWithID(pSoftwareMonHandlerContext->monObjInfoList, delPrcId);
								continue;
							}
						}
					}	
				}
			}
			if(strlen(tmpEventMsg))
			{
				pRepMsg = (char *)DynamicStrCat(pRepMsg, &repMsgBufLen, ";",tmpEventMsg);
				/*if(pRepMsg)
					repMsgLen = strlen(pRepMsg) + 1;
				if(repMsgLen > 1)
				{
					pRepMsg = (char *)DynamicStrCat(pRepMsg,";",tmpEventMsg);
					//sprintf_s(pRepMsg, sizeof(pRepMsg), "%s;%s", pRepMsg, tmpEventMsg);
				}
				else
				{
					int tmpEventMsgLen = strlen(tmpEventMsg) + 1;
					pRepMsg = (char *)malloc(tmpEventMsgLen);
					if(pRepMsg)
					{
						memset(pRepMsg, 0, sizeof(char)*tmpEventMsgLen);
						sprintf_s(pRepMsg, sizeof(char)*tmpEventMsgLen, "%s",tmpEventMsg);
					}
				}*/
			}
			app_os_sleep(10);
			curMonObjInfoNode = curMonObjInfoNode->next;
		}
		app_os_LeaveCriticalSection(&pSoftwareMonHandlerContext->swMonObjCS);

		if(pRepMsg)
			repMsgLen = strlen(pRepMsg)+1;
		//if(strlen(repMsg))
		if(repMsgLen  > 1)
		{
			BOOL isSWMNormal = TRUE;
			if(IsSWMThrNormal(&isSWMNormal))
			{
				swm_thr_rep_info_t thrRepInfo;
				//int repInfoLen = strlen(repMsg)+1;
				thrRepInfo.repInfo = NULL;
				thrRepInfo.isTotalNormal = isSWMNormal;
				{
					thrRepInfo.repInfo = (char *)malloc(repMsgLen);
					if(thrRepInfo.repInfo)
					{
						memset(thrRepInfo.repInfo, 0, sizeof(char)*repMsgLen);
						sprintf_s(thrRepInfo.repInfo, sizeof(char)*repMsgLen, "%s", pRepMsg);
					}
					if(pRepMsg) free(pRepMsg);
				}
				//memset(thrRepInfo.repInfo, 0, sizeof(thrRepInfo.repInfo));
				//strcpy(thrRepInfo.repInfo, repMsg);
				{
					char * uploadRepJsonStr = NULL;
					char * str = (char *)&thrRepInfo;
					int jsonStrlen = Parser_swm_mon_prc_event_rep(str, &uploadRepJsonStr);
					if(jsonStrlen > 0 && uploadRepJsonStr != NULL)
					{
						g_sendcbf(&g_PluginInfo, swm_mon_prc_event_rep, uploadRepJsonStr, strlen(uploadRepJsonStr)+1, NULL, NULL);
					}
					if(uploadRepJsonStr)free(uploadRepJsonStr);	
				}
				if(thrRepInfo.repInfo) free(thrRepInfo.repInfo);
			}
		}

	}
	return bRet = TRUE;
}

static void MonObjInfoListFillConfig(mon_obj_info_list monObjList, char *monObjConfigPath)
{
	if(monObjList == NULL || monObjConfigPath == NULL) return;
	{
		char * pJsonStr = NULL;
		int jsonLen = 0;
		int dataLen = sizeof(mon_obj_info_list);
		int sendLen = sizeof(susi_comm_data_t) + dataLen;
		char * sendData = (char *)malloc(sendLen);
		susi_comm_data_t *pSusiCommData = (susi_comm_data_t *) sendData;
		pSusiCommData->comm_Cmd = swm_set_mon_objs_req;
#ifdef COMM_DATA_WITH_JSON
		pSusiCommData->reqestID = cagent_request_software_monitoring;
#endif
		pSusiCommData->message_length = dataLen;
		memcpy(pSusiCommData->message, &monObjList, dataLen);

		//jsonLen = PackSUSICommData(pSusiCommData, &pJsonStr);
		jsonLen = Pack_swm_set_mon_objs_req(pSusiCommData, &pJsonStr);


		if(jsonLen > 0 && pJsonStr != NULL)
		{
			FILE * fPtr = fopen(monObjConfigPath, "wb");
			if(fPtr)
			{
				fwrite(pJsonStr, 1, jsonLen, fPtr);
				fclose(fPtr);
			}
			free(pJsonStr);
		}
		if(sendData) free(sendData);
	}
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
		head->prcMonInfo.isActive = FALSE;
		head->prcMonInfo.prcCpuUsageLastTimes.lastKernelTime = 0;
		head->prcMonInfo.prcCpuUsageLastTimes.lastUserTime = 0;
		head->prcMonInfo.prcCpuUsageLastTimes.lastTime = 0;
	}
	return head;
}


//---------------------------------monitor object info list function define-----------------------------------
static mon_obj_info_node_t * CreateMonObjInfoList()
{
	mon_obj_info_node_t * head = NULL;
	head = (mon_obj_info_node_t *)malloc(sizeof(mon_obj_info_node_t));
	if(head) 
	{
		head->next = NULL;
		head->monObjInfo.isValid = TRUE;
		head->monObjInfo.prcName = NULL;
		head->monObjInfo.prcID = 0;
		head->monObjInfo.prcResponse = TRUE;
		memset(head->monObjInfo.cpuThrItem.tagName, 0, sizeof(head->monObjInfo.cpuThrItem.tagName));
		head->monObjInfo.cpuThrItem.maxThreshold = DEF_INVALID_VALUE;
		head->monObjInfo.cpuThrItem.minThreshold = DEF_INVALID_VALUE;
		head->monObjInfo.cpuThrItem.thresholdType = DEF_THR_UNKNOW_TYPE;
		head->monObjInfo.cpuThrItem.lastingTimeS = DEF_INVALID_TIME;
		head->monObjInfo.cpuThrItem.intervalTimeS = DEF_INVALID_TIME;
		head->monObjInfo.cpuThrItem.isEnable = FALSE;
		head->monObjInfo.cpuThrItem.checkType = ck_type_unknow;
		head->monObjInfo.cpuThrItem.checkSourceValueList.head = NULL;
		head->monObjInfo.cpuThrItem.checkSourceValueList.nodeCnt = 0;
		head->monObjInfo.cpuThrItem.checkResultValue.vi = DEF_INVALID_VALUE;
		head->monObjInfo.cpuThrItem.isNormal = TRUE;
		head->monObjInfo.cpuThrItem.repThrTime = 0;
		head->monObjInfo.cpuAct = prc_act_unknown;
		head->monObjInfo.cpuActCmd = NULL;
		memset(head->monObjInfo.memThrItem.tagName, 0, sizeof(head->monObjInfo.memThrItem.tagName));
		head->monObjInfo.memThrItem.maxThreshold = DEF_INVALID_VALUE;
		head->monObjInfo.memThrItem.minThreshold = DEF_INVALID_VALUE;
		head->monObjInfo.memThrItem.thresholdType = DEF_THR_UNKNOW_TYPE;
		head->monObjInfo.memThrItem.lastingTimeS = DEF_INVALID_TIME;
		head->monObjInfo.memThrItem.intervalTimeS = DEF_INVALID_TIME;
		head->monObjInfo.memThrItem.isEnable = FALSE;
		head->monObjInfo.memThrItem.checkType = ck_type_unknow;
		head->monObjInfo.memThrItem.checkSourceValueList.head = NULL;
		head->monObjInfo.memThrItem.checkSourceValueList.nodeCnt = 0;
		head->monObjInfo.memThrItem.checkResultValue.vi = DEF_INVALID_VALUE;
		head->monObjInfo.memThrItem.isNormal = TRUE;
		head->monObjInfo.memThrItem.repThrTime = 0;
		head->monObjInfo.memAct = prc_act_unknown;
		head->monObjInfo.memActCmd = NULL;
	}
	return head;
}



static BOOL InitMonObjInfoListFromConfig(mon_obj_info_list monObjList, char *monObjConfigPath)
{
	BOOL bRet = FALSE;
	FILE *fptr = NULL;
	char * pTmpThrInfoStr = NULL;
	if(monObjList == NULL || monObjConfigPath == NULL) return bRet;
	if ((fptr = fopen(monObjConfigPath, "rb")) == NULL)return bRet; 
	//{
	//	SoftwareMonitorLog(g_loghandle, Normal, "%s()[%d]###Error: open %s failed!\n", __FUNCTION__, __LINE__, monObjConfigPath);
	//	return bRet;
	//}
	{
		unsigned int fileLen = 0;
		fseek(fptr, 0, SEEK_END);
		fileLen = ftell(fptr);
		if(fileLen > 0)
		{
			unsigned int readLen = fileLen + 1, realReadLen = 0;
			pTmpThrInfoStr = (char *) malloc(readLen);
			memset(pTmpThrInfoStr, 0, readLen);
			fseek(fptr, 0, SEEK_SET);
			realReadLen =  fread(pTmpThrInfoStr, sizeof(char), readLen, fptr);
			if(realReadLen > 0)
			{
				char *monStr = (char*)malloc(sizeof(mon_obj_info_list));
				memcpy(monStr, (char *)&monObjList, sizeof(mon_obj_info_list));
				bRet = Parse_swm_set_mon_objs_req(pTmpThrInfoStr, monStr);
				free(monStr);//Wei.Gang add to fix  mem leak.
			}
			if(pTmpThrInfoStr) free(pTmpThrInfoStr);
		}
	}
	fclose(fptr);
	return bRet;
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


static BOOL UploadPrcMonInfo()
{
	sw_mon_handler_context_t * pSWMonHandlerContext = (sw_mon_handler_context_t *)&SWMonHandlerContext;
	BOOL bRet = FALSE;
	char errorStr[128] = {0};
	cagent_status_t status = cagent_success; 
	if(pSWMonHandlerContext == NULL) return bRet;
	else if(!(pSWMonHandlerContext->isUserLogon))
	{
		memset(errorStr, 0, sizeof(errorStr));
		sprintf(errorStr, "Not logged on!");
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
		return bRet;
	}

	app_os_EnterCriticalSection(&pSWMonHandlerContext->swPrcMonCS);
	{
		char * uploadRepJsonStr = NULL;
		char * str = (char*)&pSWMonHandlerContext->prcMonInfoList;
	
		int jsonStrlen = Parser_swm_get_pmi_list_rep(str, &uploadRepJsonStr);
		if(jsonStrlen > 0 && uploadRepJsonStr != NULL)
		{
			status = g_sendcbf(&g_PluginInfo, swm_get_pmi_list_rep, uploadRepJsonStr, strlen(uploadRepJsonStr)+1, NULL, NULL);
		}
		if(uploadRepJsonStr)free(uploadRepJsonStr);
	}

	app_os_LeaveCriticalSection(&pSWMonHandlerContext->swPrcMonCS);
	if(status == cagent_success)
	{
		bRet = TRUE;
	}
	return bRet;
}

static CAGENT_PTHREAD_ENTRY(PrcMonInfoUploadThreadStart, args)
{
	sw_mon_handler_context_t *pSoftwareMonHandlerContext = (sw_mon_handler_context_t *)args;
	clock_t timeoutNow = 0;
	BOOL isAuto = FALSE;
	int timeoutMS = 0;
	int intervalMS = 0;
	while (pSoftwareMonHandlerContext->isPrcMonInfoUploadThreadRunning)
	{
		app_os_EnterCriticalSection(&pSoftwareMonHandlerContext->prcMonInfoAutoCS);
		isAuto = pSoftwareMonHandlerContext->isAutoUpload;
		timeoutMS = pSoftwareMonHandlerContext->autoUploadTimeoutMs;
		intervalMS = pSoftwareMonHandlerContext->uploadIntervalMs;
		app_os_LeaveCriticalSection(&pSoftwareMonHandlerContext->prcMonInfoAutoCS);
		if(!isAuto)
		{
			app_os_cond_wait(&pSoftwareMonHandlerContext->prcMonInfoUploadSyncCond, &pSoftwareMonHandlerContext->prcMonInfoUploadSyncMutex);
			if(!pSoftwareMonHandlerContext->isPrcMonInfoUploadThreadRunning)
			{
				break;
			}
			UploadPrcMonInfo();
		}
		else
		{
			timeoutNow = clock();
			if(pSoftwareMonHandlerContext->timeoutStart == 0)
			{
				pSoftwareMonHandlerContext->timeoutStart = timeoutNow;
			}

			if(timeoutNow - pSoftwareMonHandlerContext->timeoutStart >= timeoutMS)
			{
				app_os_EnterCriticalSection(&pSoftwareMonHandlerContext->prcMonInfoAutoCS);
				pSoftwareMonHandlerContext->isAutoUpload = FALSE;
				app_os_LeaveCriticalSection(&pSoftwareMonHandlerContext->prcMonInfoAutoCS);
				pSoftwareMonHandlerContext->timeoutStart = 0;
			}
			else
			{
				app_os_sleep(intervalMS);
				UploadPrcMonInfo();
			}
		}    
		app_os_sleep(10);
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
	SWMonHandlerContext.susiHandlerContext.cagentHandle = &g_PluginInfo;

	SWMonHandlerContext.prcMonInfoList = CreatePrcMonInfoList();
	SWMonHandlerContext.monObjInfoList = CreateMonObjInfoList();

	{
		char modulePath[MAX_PATH] = {0};     
		if(!app_os_get_module_path(modulePath)) goto done;
		sprintf(MonObjConfigPath, "%s%s", modulePath, DEF_PRC_MON_CONFIG_NAME);
		sprintf(MonObjTmpConfigPath, "%s%sTmp", modulePath, DEF_PRC_MON_CONFIG_NAME);
		InitMonObjInfoListFromConfig(SWMonHandlerContext.monObjInfoList, MonObjConfigPath);
		//DeleteMonObjInfoNodeWithID(SWMonHandlerContext.monObjInfoList, 0);
	}
	app_os_InitializeCriticalSection(&SWMonHandlerContext.swPrcMonCS);
	app_os_InitializeCriticalSection(&SWMonHandlerContext.swMonObjCS);
	app_os_InitializeCriticalSection(&SWMonHandlerContext.swSysMonCS);
	app_os_InitializeCriticalSection(&SWMonHandlerContext.prcMonInfoAutoCS);
	app_os_InitializeCriticalSection(&SWMonHandlerContext.sysMonInfoAutoCS);

	if(strlen(tmpCfg_gatherLev) == 0)
	{
		if(proc_config_get(agentConfigFilePath, CFG_PROCESS_GATHER_LEVEL, tmpCfg_gatherLev, sizeof(tmpCfg_gatherLev)) <= 0)
		{
			SWMonHandlerContext.gatherLevel = 1; //1 indicate gather logon user processes, 0 gather all processes.
		}
		else
		{
			SWMonHandlerContext.gatherLevel = atoi(tmpCfg_gatherLev);
			if(SWMonHandlerContext.gatherLevel > 1)
			{
				memset(SWMonHandlerContext.sysUserName, 0 , sizeof(SWMonHandlerContext.sysUserName));
				proc_config_get(agentConfigFilePath, CFG_SYSTEM_LOGON_USER, SWMonHandlerContext.sysUserName, sizeof(SWMonHandlerContext.sysUserName));
			}
		}
	}

	SWMonHandlerContext.isSoftwareMonThreadRunning = TRUE;
	if (app_os_thread_create(&SWMonHandlerContext.softwareMonThreadHandle, SoftwareMonThreadStart, &SWMonHandlerContext) != 0)
	{
		SoftwareMonitorLog(g_loghandle, Normal, "%s()[%d]###Start software monitor thread failed!\n",__FUNCTION__, __LINE__);
		SWMonHandlerContext.isSoftwareMonThreadRunning = FALSE;
		goto done;
	}

	SWMonHandlerContext.isPrcMonInfoUploadThreadRunning = FALSE;
	SWMonHandlerContext.isAutoUpload = FALSE;
	SWMonHandlerContext.timeoutStart = 0;
	SWMonHandlerContext.uploadIntervalMs = DEF_PRC_MON_INFO_UPLOAD_INTERVAL_MS;
	SWMonHandlerContext.autoUploadTimeoutMs = DEF_PRC_MON_INFO_UPLOAD_TIMEOUT_MS;
	SWMonHandlerContext.isUserLogon = FALSE;
	if(app_os_cond_setup(&SWMonHandlerContext.prcMonInfoUploadSyncCond) != 0)
	{
		SoftwareMonitorLog(g_loghandle, Normal, "%s()[%d]###Create prcMonInfo upload sync cond failed!\n",__FUNCTION__, __LINE__);

		goto done;
	}

	if(app_os_mutex_setup(&SWMonHandlerContext.prcMonInfoUploadSyncMutex) != 0)
	{
		SoftwareMonitorLog(g_loghandle, Normal, "%s()[%d]###Create prcMonInfo upload sync mutex failed!\n",__FUNCTION__, __LINE__);
		goto done;
	}

	SWMonHandlerContext.isPrcMonInfoUploadThreadRunning = TRUE;
	if (app_os_thread_create(&SWMonHandlerContext.prcMonInfoUploadThreadHandle, PrcMonInfoUploadThreadStart, &SWMonHandlerContext) != 0)
	{
		SoftwareMonitorLog(g_loghandle, Normal, "%s()[%d]###Start prcMonInfo upload thread failed!\n",__FUNCTION__, __LINE__);
		SWMonHandlerContext.isPrcMonInfoUploadThreadRunning = FALSE;
		goto done;
	}

	SWMonHandlerContext.isSysMonInfoUploadThreadRunning = FALSE;
	SWMonHandlerContext.isSysAutoUpload = FALSE;
	SWMonHandlerContext.sysTimeoutStart = 0;
	SWMonHandlerContext.sysUploadIntervalMs = DEF_PRC_MON_INFO_UPLOAD_INTERVAL_MS;
	SWMonHandlerContext.sysAutoUploadTimeoutMs = DEF_PRC_MON_INFO_UPLOAD_TIMEOUT_MS;
	if(app_os_cond_setup(&SWMonHandlerContext.sysMonInfoUploadSyncCond) != 0)
	{
		SoftwareMonitorLog(g_loghandle, Normal, "%s()[%d]###Create sysMonInfo upload sync cond failed!\n",__FUNCTION__, __LINE__);

		goto done;
	}

	if(app_os_mutex_setup(&SWMonHandlerContext.sysMonInfoUploadSyncMutex) != 0)
	{
		SoftwareMonitorLog(g_loghandle, Normal, "%s()[%d]###Create sysMonInfo upload sync mutex failed!\n",__FUNCTION__, __LINE__);
		goto done;
	}

	SWMonHandlerContext.isSysMonInfoUploadThreadRunning = TRUE;
	if (app_os_thread_create(&SWMonHandlerContext.sysMonInfoUploadThreadHandle, SysMonInfoUploadThreadStart, &SWMonHandlerContext) != 0)
	{
		SoftwareMonitorLog(g_loghandle, Normal,"%s()[%d]###Start sysMonInfo upload thread failed!\n",__FUNCTION__, __LINE__);

		SWMonHandlerContext.isSysMonInfoUploadThreadRunning = FALSE;
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

	if(SWMonHandlerContext.isPrcMonInfoUploadThreadRunning)
	{
		SWMonHandlerContext.isPrcMonInfoUploadThreadRunning = FALSE;
		app_os_cond_signal(&SWMonHandlerContext.prcMonInfoUploadSyncCond);
		app_os_thread_join(SWMonHandlerContext.prcMonInfoUploadThreadHandle);
	}
	app_os_cond_cleanup(&SWMonHandlerContext.prcMonInfoUploadSyncCond);
	app_os_mutex_cleanup(&SWMonHandlerContext.prcMonInfoUploadSyncMutex);

	if(SWMonHandlerContext.isSysMonInfoUploadThreadRunning)
	{
		SWMonHandlerContext.isSysMonInfoUploadThreadRunning = FALSE;
		app_os_cond_signal(&SWMonHandlerContext.sysMonInfoUploadSyncCond);
		app_os_thread_join(SWMonHandlerContext.sysMonInfoUploadThreadHandle);
	}
	app_os_cond_cleanup(&SWMonHandlerContext.sysMonInfoUploadSyncCond);
	app_os_mutex_cleanup(&SWMonHandlerContext.sysMonInfoUploadSyncMutex);

	if(SWMonHandlerContext.monObjInfoList)
	{
		MonObjInfoListFillConfig(SWMonHandlerContext.monObjInfoList, MonObjConfigPath);
		DestroyMonObjInfoList(SWMonHandlerContext.monObjInfoList);
		SWMonHandlerContext.monObjInfoList = NULL;
	}
	if(SWMonHandlerContext.prcMonInfoList)
	{
		DestroyPrcMonInfoList(SWMonHandlerContext.prcMonInfoList);
		SWMonHandlerContext.prcMonInfoList = NULL;
	}

	app_os_DeleteCriticalSection(&SWMonHandlerContext.swPrcMonCS);
	app_os_DeleteCriticalSection(&SWMonHandlerContext.swMonObjCS);
	app_os_DeleteCriticalSection(&SWMonHandlerContext.swSysMonCS);
	app_os_DeleteCriticalSection(&SWMonHandlerContext.prcMonInfoAutoCS);
	app_os_DeleteCriticalSection(&SWMonHandlerContext.sysMonInfoAutoCS);

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
	//cagent_callback_status_t status = cagent_callback_continue; 
	//cagent_handle_t cagentHandle = pSWMonHandlerContext->susiHandlerContext.cagentHandle;
	int commCmd = unknown_cmd;
	//susi_comm_data_t *pSusiCommData = NULL;
	char errorStr[128] = {0};
	if(!data || datalen <= 0) return;

	if(!ParseReceivedData(data, datalen, &commCmd))
		return;
	switch(commCmd)
	{
	case swm_set_pmi_auto_upload_req:
		{
			swm_auto_upload_params_t tempVal;

			if(Parse_swm_set_pmi_auto_upload_req((char*)data, &tempVal))
			{
				swm_auto_upload_params_t *pSWAutoUploadParams = &tempVal;
				if(pSWAutoUploadParams->auto_upload_interval_ms > 0 && pSWAutoUploadParams->auto_upload_timeout_ms >0)
				{
					app_os_EnterCriticalSection(&pSWMonHandlerContext->prcMonInfoAutoCS);
					pSWMonHandlerContext->timeoutStart = 0;
					pSWMonHandlerContext->uploadIntervalMs = pSWAutoUploadParams->auto_upload_interval_ms;
					pSWMonHandlerContext->autoUploadTimeoutMs = pSWAutoUploadParams->auto_upload_timeout_ms;
					if(!pSWMonHandlerContext->isAutoUpload)
					{
						pSWMonHandlerContext->isAutoUpload = TRUE; 
						app_os_cond_signal(&pSWMonHandlerContext->prcMonInfoUploadSyncCond);
					}
					app_os_LeaveCriticalSection(&pSWMonHandlerContext->prcMonInfoAutoCS);
					{
						char * uploadRepJsonStr = NULL;
						char * str = "SUCCESS";
						int jsonStrlen = Parser_string(str, &uploadRepJsonStr,swm_set_pmi_auto_upload_rep);
						if(jsonStrlen > 0 && uploadRepJsonStr != NULL)
						{
							g_sendcbf(&g_PluginInfo, swm_set_pmi_auto_upload_rep, uploadRepJsonStr, strlen(uploadRepJsonStr)+1, NULL, NULL);
						}
						if(uploadRepJsonStr)free(uploadRepJsonStr);	
					}

				}
				else
				{
					memset(errorStr, 0, sizeof(errorStr));
					sprintf(errorStr, "Command(%d) params error!", swm_set_pmi_auto_upload_req);
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
				sprintf(errorStr, "Command(%d) parse error!", swm_set_pmi_auto_upload_req);
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
	case swm_set_pmi_reqp_req:
		{
			pSWMonHandlerContext->isAutoUpload = FALSE;
			{
				char * uploadRepJsonStr = NULL;
				char * str = "SUCCESS";
				int jsonStrlen = Parser_string(str, &uploadRepJsonStr,swm_set_pmi_reqp_rep);
				if(jsonStrlen > 0 && uploadRepJsonStr != NULL)
				{
					g_sendcbf(&g_PluginInfo, swm_set_pmi_reqp_rep, uploadRepJsonStr, strlen(uploadRepJsonStr)+1, NULL, NULL);
				}
				if(uploadRepJsonStr)free(uploadRepJsonStr);	
			}

			break;
		}
	case swm_get_pmi_list_req:
		{
			UploadPrcMonInfo();
			break;
		}
	case swm_set_mon_objs_req:
		{
			SWMSetMonObjs(pSWMonHandlerContext, (char*)data);
			break;
		}
	case swm_del_all_mon_objs_req:
		{
			SWMDelAllMonObjs();
			break;
		}
	case swm_restart_prc_req:
		{
			SWMRestartPrc(pSWMonHandlerContext, (char*)data);
			break;
		}
	case swm_kill_prc_req:
		{
			SWMKillPrc(pSWMonHandlerContext, (char*)data);
			break;
		}
#ifdef EVENT_LOG
	case swm_get_event_log_req:
		{
			SWMGetSysEventLog(pSWMonHandlerContext, (char*)data);
			break;
		}
#endif
	case swm_set_smi_auto_upload_req:
		{
			swm_auto_upload_params_t temp;
			if(Parse_swm_set_pmi_auto_upload_req((char*)data, &temp))
			{
				swm_auto_upload_params_t *pSWAutoUploadParams = &temp;
				if(pSWAutoUploadParams->auto_upload_interval_ms > 0 && pSWAutoUploadParams->auto_upload_timeout_ms >0)
				{
					app_os_EnterCriticalSection(&pSWMonHandlerContext->sysMonInfoAutoCS);
					pSWMonHandlerContext->sysTimeoutStart = 0;
					pSWMonHandlerContext->sysUploadIntervalMs = pSWAutoUploadParams->auto_upload_interval_ms;
					pSWMonHandlerContext->sysAutoUploadTimeoutMs = pSWAutoUploadParams->auto_upload_timeout_ms;
					if(!pSWMonHandlerContext->isSysAutoUpload)
					{
						pSWMonHandlerContext->isSysAutoUpload = TRUE; 
						app_os_cond_signal(&pSWMonHandlerContext->sysMonInfoUploadSyncCond);
					}
					app_os_LeaveCriticalSection(&pSWMonHandlerContext->sysMonInfoAutoCS);
					{
						char * uploadRepJsonStr = NULL;
						char * str = "SUCCESS";
						int jsonStrlen = Parser_string(str, &uploadRepJsonStr,swm_set_smi_auto_upload_rep);
						if(jsonStrlen > 0 && uploadRepJsonStr != NULL)
						{
							g_sendcbf(&g_PluginInfo, swm_set_smi_auto_upload_rep, uploadRepJsonStr, strlen(uploadRepJsonStr)+1, NULL, NULL);
						}
						if(uploadRepJsonStr)free(uploadRepJsonStr);	
					}

				}
				else
				{
					memset(errorStr, 0, sizeof(errorStr));
					sprintf(errorStr, "Command(%d) params error!", swm_set_smi_auto_upload_req);
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
				sprintf(errorStr, "Command(%d) parse error!", swm_set_smi_auto_upload_req);
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
	case swm_set_smi_reqp_req:
		{
			pSWMonHandlerContext->isSysAutoUpload = FALSE;
			{
				char * uploadRepJsonStr = NULL;
				char * str = "SUCCESS";
				int jsonStrlen = Parser_string(str, &uploadRepJsonStr,swm_set_smi_reqp_rep);
				if(jsonStrlen > 0 && uploadRepJsonStr != NULL)
				{
					g_sendcbf(&g_PluginInfo, swm_set_smi_reqp_rep, uploadRepJsonStr, strlen(uploadRepJsonStr)+1, NULL, NULL);
				}
				if(uploadRepJsonStr)free(uploadRepJsonStr);	
			}

			break;
		}
	case swm_get_smi_req:
		{
			UploadSysMonInfo();
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
int HANDLER_API Handler_Get_Capability( char ** pOutReply )
{
	int len = 0; // Data length of the pOutReply 
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
