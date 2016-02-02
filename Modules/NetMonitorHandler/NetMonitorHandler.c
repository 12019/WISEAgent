/****************************************************************************/
/* Copyright(C) : Advantech Technologies, Inc.								*/
/* Create Date  : 2015/01/23 by lige									*/
/* Abstract     : NetMonitor API interface definition   					*/
/* Reference    : None														*/
/****************************************************************************/
#include "network.h"
#include "platform.h"
#include "susiaccess_handler_api.h"
#include "NetMonitorHandler.h"
#include "NetMonitorLog.h"
#include "Parser.h"
#include "common.h"
#include "cJSON.h"
#include <string.h>
#include <errno.h>

#ifdef WIN32
#include <IPHlpApi.h> 
#include <Mprapi.h>
#include <ifmib.h> 
#pragma comment(lib, "IPHLPAPI.lib") 
#pragma comment(lib, "Mprapi.lib") 
#else
#include <pthread.h>
#endif

//-----------------------------------------------------------------------------
// Types and defines:
//-----------------------------------------------------------------------------
const char strPluginName[MAX_TOPIC_LEN] = {"NetMonitor"};
const int iRequestID = cagent_request_net_monitoring; 
const int iActionID = cagent_reply_net_monitoring;

netmon_handler_context_t NetMonHandlerContext;

static HandlerSendCbf  g_sendcbf = NULL;						// Client Send information (in JSON format) to Cloud Server	
static HandlerSendCustCbf  g_sendcustcbf = NULL;			    // Client Send information (in JSON format) to Cloud Server with custom topic	
static HandlerAutoReportCbf g_sendreportcbf = NULL;				// Client Send report (in JSON format) to Cloud Server with AutoReport topic	
static HandlerSendCapabilityCbf g_sendcapabilitycbf = NULL;	
static HandlerSubscribeCustCbf g_subscribecustcbf = NULL;
static Handler_info  g_PluginInfo;
static void* g_loghandle = NULL;
static bool g_bEnableLog = TRUE;
bool g_sendreportcbfFlag = FALSE;
int g_indexForWin7 = 0;

//------------------------------user variable define---------------------------
bool IsHandlerStart = false;

typedef enum{
	rep_data_unknown,
	rep_data_query,
	rep_data_auto,
}report_type_t;
typedef struct report_params_t{
	//report_type_t repType;
	unsigned int intervalTimeMs;
	unsigned int continueTimems;
	unsigned int tmeoutMs;
}report_params_t;

typedef struct upload_params_t{
	//report_type_t repType;
	unsigned int intervalTimeMs;
	unsigned int continueTimems;
	unsigned int tmeoutMs;
}upload_params_t;


static report_params_t NMReportParams;
static upload_params_t NMUploadParams;
static CAGENT_THREAD_HANDLE NMAutoUploadThreadHandle = NULL;
static CAGENT_THREAD_HANDLE NMAutoReportThreadHandle = NULL;

#define DEF_NM_THR_CONFIG_NAME                "NetworkThrInfoCfg"
static char NMThrCfgPath[MAX_PATH] = {0};
static char NMThrTmpCfgPath[MAX_PATH] = {0};
static nm_thr_item_list NMThrItemList = NULL;
static bool IsNMThrCheckThreadRunning = false;
static CAGENT_THREAD_HANDLE NMThrCheckThreadHandle = NULL;
static bool IsSetThrThreadRunning = false;
static CAGENT_THREAD_HANDLE SetThrThreadHandle = NULL;

#ifdef WIN32
//static CAGENT_MUTEX_TYPE NMReportParamsMutex = NULL;
static CAGENT_MUTEX_TYPE NMThrInfoMutex = NULL;
#else
//static CAGENT_MUTEX_TYPE NMReportParamsMutex = PTHREAD_MUTEX_INITIALIZER; 
static CAGENT_MUTEX_TYPE NMThrInfoMutex = PTHREAD_MUTEX_INITIALIZER;
#endif
//------------------------------------------------------------------------------

//---------------------------net monitor net info item list function define---------------------
static net_info_list CreateNetInfoList();
static void DestroyNetInfoList(net_info_list netInfoList);
static int InsertNetInfoNode(net_info_list netInfoList, net_info_t * pNetInfo);
static net_info_node_t * FindNetInfoNodeWithIndex(net_info_list netInfoList, int index);
static net_info_node_t * FindNetInfoNodeWithAdapterName(net_info_list netInfoList, char * adpName);
static int DeleteNetInfoNodeWithIndex(net_info_list netInfoList, int index);
static int DeleteAllNetInfoNode(net_info_list netInfoList);
static BOOL IsNetInfoListEmpty(net_info_list netInfoList);
static void GetCapability(int sendLevel, int repCommandID);
static void SendErrMsg(char * errorStr,int sendLevel);
static BOOL ResetNetinfoListNodeisFoundFlag(net_info_list netInfoList);
static BOOL DelNodeWhenAdaperIsDisabled(net_info_list netInfoList);

//----------------------------------------------------------------------------------------------

//---------------------------net monitor thr item list function define--------------------------
static nm_thr_item_list CreateNMThrItemList();
static void DestroyNMThrItemList(nm_thr_item_list thrList);
static int InsertNMThrItemNode(nm_thr_item_list thrList, nm_thr_item_t * pThrItem);
static nm_thr_item_node_t * FindNMThrItemNodeWithName(nm_thr_item_list thrList, char * adpName, char * tagName);
static int DeleteNMThrItemNodeWithName(nm_thr_item_list thrList, char * adpName, char * tagName);
static int DeleteAllNMThrItemNode(nm_thr_item_list thrList);
static BOOL IsNMThrItemListEmpty(nm_thr_item_list thrList);
void UpdateNMThrItemList();
//----------------------------------------------------------------------------------------------

//--------------------------------------other function define-----------------------------------
BOOL GetNetInfoWithIndex(net_info_t * pNetInfo, DWORD index);
//BOOL GetAdapterFriendlyNameWithGUIDName(wchar_t * guidName, char * friendlyName, int index);
BOOL GetAdapterFriendlyNameWithGUIDName(wchar_t * guidName, char * friendlyName);
static BOOL GetNetInfo(net_info_list netInfoList);
static CAGENT_PTHREAD_ENTRY(GetNetInfoThreadStart, args);
static BOOL ReplyNetMonInfo(cagent_handle_t cagentHandle);
static CAGENT_PTHREAD_ENTRY(NetMonUploadThreadStart30, args);
unsigned long getCurrentTime();
void getRecvSendBytes(char *adaptername, int *recvBytes, int *sendBytes);
void getRecvBytes(char *adaptername, char c);
void getSendBytes(char *adaptername, char c);

static CAGENT_PTHREAD_ENTRY(NMAutoReport_API_ThreadStart, args);
static char * GetNMCapabilityString();
static void RepNMData(net_info_list netInfoList);//not used

static BOOL InitNetMonThrFromConfig(nm_thr_item_list thrList, char * cfgPath);
static void NetMonThrFillConfig(nm_thr_item_list thrList, char * cfgPath);
static BOOL NetMonCheckSrcVal(nm_thr_item_t * pThrItemInfo, float * pCheckValue, check_value_type_t valType);
static BOOL NetMonCheckThrItem(nm_thr_item_t * pThrItemInfo, check_value_type_t valType, float * checkVal, char * checkRetMsg);
static BOOL NetMonCheckThr(nm_thr_item_list curThrItemList, net_info_list curNetInfoList, char ** checkRetMsg, unsigned int bufLen);
static BOOL NetMonIsThrNormal(nm_thr_item_list thrList, BOOL * isNormal);
static CAGENT_PTHREAD_ENTRY(NMThrCheckThreadStart, args);
static void NetMonWhenDelThrCheckNormal(nm_thr_item_list thrItemList, char ** checkMsg, unsigned int bufLen);
//static void NetMonUpdateThr(nm_thr_item_list newNMThrList)
static void NetMonUpdateThr(nm_thr_item_list curThrList, nm_thr_item_list newNMThrList);
static CAGENT_PTHREAD_ENTRY(SetThrThreadStart, args);
static void NetMonSetThr(char *thrJsonStr);
static void NetMonDelAllThr();

static sensor_info_list CreateSensorInfoList();
static void UploadSysMonData(sensor_info_list sensorInfoList, char * pSessionID);
static int DeleteAllSensorInfoNode(sensor_info_list sensorInfoList);
static void DestroySensorInfoList(sensor_info_list sensorInfoList);
static BOOL IsSensorInfoListEmpty(sensor_info_list sensorInfoList);

//----------------------------------------------------------------------------------------------

//--------------------------------------linux function define-----------------------------------
void getNetUsage(char *adapterName, DWORD *netSpeedMbps, DWORD *RecvBytes, double *recvThroughput, DWORD *sendBytes, double *sendThroughput, double *netUsage);
void getAdapterNameList(char *str, char c);
void getAdapterNameListEx(char *str, char c);
void getNetworkCardList(char *str, char c);
void getWiredStatus(char *adapterName, char c);
void getWiredSpeedMbpsTemp(char *adapterName, char c);
void getWirelessAdapterNameList(char *str, char c);
void getWirelessSpeedMbps(char *adapterName, char c);
void getWirelessStatus(char *adapterName, char c);
int  getNumberOfAdapters(char* fileName);
BOOL whetherExistWirelessCard(char* fileName);
BOOL getNetStatus(char *adaptername, char c);
BOOL isNetDisconnect(char *fileName);
//----------------------------------------------------------------------------------------------


//-----------------------------net monitor net info item list function -------------------------
static net_info_list CreateNetInfoList()
{
	net_info_node_t * head = NULL;
	head = (net_info_node_t *)malloc(sizeof(net_info_node_t));
	if(head)
	{
		memset(head, 0, sizeof(net_info_node_t));
		head->next = NULL;
		head->netInfo.index = DEF_INVALID_VALUE;
		head->netInfo.netSpeedMbps = DEF_INVALID_VALUE;
		head->netInfo.netUsage = DEF_INVALID_VALUE;
		head->netInfo.recvDateByte = DEF_INVALID_VALUE;
		head->netInfo.recvThroughput = DEF_INVALID_VALUE;
		head->netInfo.sendDateByte = DEF_INVALID_VALUE;
		head->netInfo.sendThroughput = DEF_INVALID_VALUE;
	}
	return head;
}

static void DestroyNetInfoList(net_info_list netInfoList)
{
	if(NULL == netInfoList) return;
	DeleteAllNetInfoNode(netInfoList);
	free(netInfoList); 
	netInfoList = NULL;
}

static int InsertNetInfoNode(net_info_list netInfoList, net_info_t * pNetInfo)
{
	int iRet = -1;
	net_info_node_t * newNode = NULL, * findNode = NULL, *head = NULL;
	if(pNetInfo == NULL || netInfoList == NULL) 
		return iRet;
	head = netInfoList;
	findNode = FindNetInfoNodeWithIndex(head, pNetInfo->index);
	if(findNode == NULL)
	{
		newNode = (net_info_node_t *)malloc(sizeof(net_info_node_t));
		memset(newNode, 0, sizeof(net_info_node_t));
		memcpy((char *)&newNode->netInfo, (char *)pNetInfo, sizeof(net_info_t));
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

static net_info_node_t * FindNetInfoNodeWithIndex(net_info_list netInfoList, int index)
{
	net_info_node_t * findNode = NULL, *head = NULL;
	if(netInfoList == NULL) 
		return findNode;
	head = netInfoList;
	findNode = head->next;
	while(findNode)
	{
		if(findNode->netInfo.id == index) break;
		else
		{
			findNode = findNode->next;
		}
	}

	return findNode;
}

static net_info_node_t * FindNetInfoNodeWithAdapterName(net_info_list netInfoList, char * adpName)
{
	net_info_node_t * findNode = NULL, *head = NULL;
	if(netInfoList == NULL || adpName == NULL) return findNode;
	head = netInfoList;
	findNode = head->next;

	while(findNode)
	{
		if(!strcmp(findNode->netInfo.adapterName, adpName)) break;
		else
		{
			findNode = findNode->next;
		}
	}
	return findNode;
}

static int DeleteNetInfoNodeWithIndex(net_info_list netInfoList, int index)
{
	int iRet = -1;
	net_info_node_t * delNode = NULL, *head = NULL;
	net_info_node_t * p = NULL;
	if(netInfoList == NULL) return iRet;
	head = netInfoList;
	p = head;
	delNode = head->next;
	while(delNode)
	{
		if(delNode->netInfo.index == index)
		{
			p->next = delNode->next;
			free(delNode);
			delNode = NULL;
			iRet = 0;
			break;
		}
		else
		{
			p = delNode;
			delNode = delNode->next;
		}
	}
	if(iRet == -1) iRet = 1;
	return iRet;
}

static int DeleteAllNetInfoNode(net_info_list netInfoList)
{
	int iRet = -1;
	net_info_node_t * delNode = NULL, *head = NULL;
	if(netInfoList == NULL) return iRet;
	head = netInfoList;
	delNode = head->next;
	while(delNode)
	{
		head->next = delNode->next;
		free(delNode);
		delNode = head->next;
	}

	iRet = 0;
	return iRet;
}

static BOOL IsNetInfoListEmpty(net_info_list netInfoList)
{
	BOOL bRet = TRUE;
	net_info_node_t * curNode = NULL, *head = NULL;
	if(netInfoList == NULL) return bRet;
	head = netInfoList;
	curNode = head->next;
	if(curNode != NULL) bRet = FALSE;
	return bRet;
}


void formatNetStr(char * s)
{
	int len = strlen(s);
	char * p =s;
	int i = 0;
	char t[100] = {0};

	while(*p == ' ')
	{
		p++;
		i++;
	}
	if(s[len -1] == '\n')
		s[strlen(s)-1] = 0;
	if(s[len -1] =='\r') 
		s[strlen(s)-1] = 0;

	if(i>0)
	{
		strncpy(t, s+i,strlen(s)-i);
		memset(s, 0, strlen(s)+1);
		strncpy(s, t, strlen(t)+1);
	}
}

//----------------------------------------------------------------------------------------------


//---------------------------net monitor Sensor list function define--------------------------
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

static void UploadSysMonData(sensor_info_list sensorInfoList, char * pSessionID)
{
	if(NULL == sensorInfoList || NULL == pSessionID) return;
	{
		netmon_handler_context_t * pNetworkMonitorHandlerContext = (netmon_handler_context_t *)&NetMonHandlerContext;
		if(!IsSensorInfoListEmpty(sensorInfoList))
		{
			sensor_info_node_t * curNode = NULL;
			curNode = sensorInfoList->next;
			while(curNode)
			{
				app_os_mutex_lock(&pNetworkMonitorHandlerContext->netInfoListMutex);
				{
					char * pNetInfoList = (char*)&pNetworkMonitorHandlerContext->netInfoList;
					Parser_GetSensorJsonStr(pNetInfoList, &curNode->sensorInfo);
				}
				app_os_mutex_unlock(&pNetworkMonitorHandlerContext->netInfoListMutex);

				curNode = curNode->next;
			}
			{
				char * repJsonStr = NULL;
				int jsonStrlen = Parser_PackGetSensorDataRep(sensorInfoList, pSessionID, &repJsonStr);
				if(jsonStrlen > 0 && repJsonStr != NULL)
				{
					g_sendcbf(&g_PluginInfo, netmon_get_sensors_data_rep, repJsonStr, strlen(repJsonStr)+1, NULL, NULL);
				}
				if(repJsonStr)free(repJsonStr);
			}
		}
		else
		{
			char * errorRepJsonStr = NULL;
			char errorStr[128];
			int jsonStrlen = 0;
			sprintf(errorStr, "Command(%d), Get sensors data empty!", netmon_get_sensors_data_req);
			jsonStrlen = Parser_PackGetSensorDataError(errorStr, pSessionID, &errorRepJsonStr);
			if(jsonStrlen > 0 && errorRepJsonStr != NULL)
			{
				g_sendcbf(&g_PluginInfo, netmon_get_sensors_data_error_rep, errorRepJsonStr, strlen(errorRepJsonStr)+1, NULL, NULL);
			}
			if(errorRepJsonStr)free(errorRepJsonStr);
		}
	}
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

//----------------------------------------------------------------------------------------------


//-------------------------------net monitor thr item list function ----------------------------
static nm_thr_item_list CreateNMThrItemList()
{
	nm_thr_item_node_t * head = NULL;
	head = (nm_thr_item_node_t *)malloc(sizeof(nm_thr_item_node_t));
	if(head)
	{
		memset(head, 0, sizeof(nm_thr_item_node_t));
		head->next = NULL;
		head->thrItem.isEnable = FALSE;
		head->thrItem.maxThr = DEF_INVALID_VALUE;
		head->thrItem.minThr = DEF_INVALID_VALUE;
		head->thrItem.thrType = DEF_THR_UNKNOW_TYPE;
		head->thrItem.lastingTimeS = DEF_INVALID_TIME;
		head->thrItem.intervalTimeS = DEF_INVALID_TIME;
		head->thrItem.checkRetValue = DEF_INVALID_VALUE;
		head->thrItem.checkSrcValList.head = NULL;
		head->thrItem.checkSrcValList.nodeCnt = 0;
		head->thrItem.checkType = ck_type_unknow;
		head->thrItem.repThrTime = DEF_INVALID_VALUE;
		head->thrItem.isNormal = TRUE;
		head->thrItem.isValid = TRUE;
	}
	return head;
}

static void DestroyNMThrItemList(nm_thr_item_list thrList)
{
	if(NULL == thrList) return;
	DeleteAllNMThrItemNode(thrList);
	free(thrList); 
	thrList = NULL;
}


static int InsertNMThrItemNode(nm_thr_item_list thrList, nm_thr_item_t * pThrItem)
{
	int iRet = -1;
	nm_thr_item_node_t * newNode = NULL, * findNode = NULL, *head = NULL;
	netmon_handler_context_t * pNetworkMonitorHandlerContext = (netmon_handler_context_t *)&NetMonHandlerContext;
	//char * pNetInfoList = (char*)&pNetworkMonitorHandlerContext->netInfoList;
	net_info_list pNetInfoList = pNetworkMonitorHandlerContext->netInfoList;
	net_info_node_t *netInfoHead = pNetInfoList->next;
	net_info_node_t *curNode = netInfoHead;

	if(pThrItem == NULL || thrList == NULL) return iRet;
	head = thrList;

	{
		char * buf = NULL;
		char n[DEF_N_LEN] = {0};
		int i = 0;
		char * sliceStr[16] = {NULL};
		strcpy(n, pThrItem->n);
		buf = n;
		while(sliceStr[i] = strtok(buf, "/"))
		{
			i++;
			buf = NULL;
		}

		if((i==4) && (!strcmp(sliceStr[0], DEF_HANDLER_NAME)) && (!strcmp(sliceStr[1], NETMON_INFO_LIST)))
		{
			//"n":"NetMonitor/netMonInfoList/All/recvDateByte"
			//"n":"NetMonitor/netMonInfoList/Index0/sendThroughput"
			//"n":"NetMonitor/netMonInfoList/Index0-本地连接/netUsage"
			if(!strcmp(sliceStr[2], NETMON_Group_THR_ALL))
			{
				app_os_mutex_lock(&pNetworkMonitorHandlerContext->netInfoListMutex);
				{
					while(curNode)
					{
						newNode = (nm_thr_item_node_t *)malloc(sizeof(nm_thr_item_node_t));
						memset(newNode, 0, sizeof(nm_thr_item_node_t));

						strcpy(newNode->thrItem.n, pThrItem->n);
						strcpy(newNode->thrItem.adapterName, curNode->netInfo.adapterName);
						//strcpy(newNode->thrItem.tagName, pThrItem->tagName);
						strcpy(newNode->thrItem.tagName, sliceStr[3]);
						newNode->thrItem.index = curNode->netInfo.index;
						newNode->thrItem.isEnable = pThrItem->isEnable;
						newNode->thrItem.maxThr = pThrItem->maxThr;
						newNode->thrItem.minThr = pThrItem->minThr;
						newNode->thrItem.thrType = pThrItem->thrType;
						newNode->thrItem.lastingTimeS = pThrItem->lastingTimeS;
						newNode->thrItem.intervalTimeS = pThrItem->intervalTimeS;
						newNode->thrItem.checkType = pThrItem->checkType;
						newNode->thrItem.checkRetValue = DEF_INVALID_VALUE;
						newNode->thrItem.checkSrcValList.head = NULL;
						newNode->thrItem.checkSrcValList.nodeCnt = 0;
						newNode->thrItem.repThrTime = 0;
						//newNode->thrItem.isNormal = TRUE;
						//newNode->thrItem.isValid = TRUE;
						newNode->thrItem.isNormal = pThrItem->isNormal;
						newNode->thrItem.isValid = pThrItem->isValid; 
						newNode->next = head->next;
						head->next = newNode;
						curNode = curNode->next;
					}
				}
				app_os_mutex_unlock(&pNetworkMonitorHandlerContext->netInfoListMutex);
			}
			else if( NULL != strstr(sliceStr[2], NETMON_GROUP_NETINDEX) && NULL == strstr(sliceStr[2], "-"))
			{
				char buf[128] = {0};
				int val = 0;
				strcpy(buf, sliceStr[2]+strlen(NETMON_GROUP_NETINDEX));
				val = atoi(buf);
				app_os_mutex_lock(&pNetworkMonitorHandlerContext->netInfoListMutex);
				while(curNode)
				{
					if(curNode->netInfo.index == val)
					{
						newNode = (nm_thr_item_node_t *)malloc(sizeof(nm_thr_item_node_t));
						memset(newNode, 0, sizeof(nm_thr_item_node_t));

						strcpy(newNode->thrItem.n, pThrItem->n);
						strcpy(newNode->thrItem.adapterName, curNode->netInfo.adapterName);
						strcpy(newNode->thrItem.tagName, sliceStr[3]);
						newNode->thrItem.index = curNode->netInfo.index;
						newNode->thrItem.isEnable = pThrItem->isEnable;
						newNode->thrItem.maxThr = pThrItem->maxThr;
						newNode->thrItem.minThr = pThrItem->minThr;
						newNode->thrItem.thrType = pThrItem->thrType;
						newNode->thrItem.lastingTimeS = pThrItem->lastingTimeS;
						newNode->thrItem.intervalTimeS = pThrItem->intervalTimeS;
						newNode->thrItem.checkType = pThrItem->checkType;
						newNode->thrItem.checkRetValue = DEF_INVALID_VALUE;
						newNode->thrItem.checkSrcValList.head = NULL;
						newNode->thrItem.checkSrcValList.nodeCnt = 0;
						newNode->thrItem.repThrTime = 0;
						//newNode->thrItem.isNormal = TRUE;
						//newNode->thrItem.isValid = TRUE;
						newNode->thrItem.isNormal = pThrItem->isNormal;
						newNode->thrItem.isValid = pThrItem->isValid; 
						newNode->next = head->next;
						head->next = newNode;
						break;
					}
					else
						curNode = curNode->next;
				}
				app_os_mutex_unlock(&pNetworkMonitorHandlerContext->netInfoListMutex);
			}
			else if( NULL != strstr(sliceStr[2], NETMON_GROUP_NETINDEX) && NULL != strstr(sliceStr[2], "-"))
			{
				char buf[128] = {0};
				//strcpy(buf, sliceStr[2]+strlen(NETMON_GROUP_NETINDEX)+1);
				strcpy(buf, strstr(sliceStr[2], "-")+1);

				newNode = (nm_thr_item_node_t *)malloc(sizeof(nm_thr_item_node_t));
				memset(newNode, 0, sizeof(nm_thr_item_node_t));

				strcpy(newNode->thrItem.n, pThrItem->n);
				strcpy(pThrItem->adapterName, buf);
				strcpy(pThrItem->tagName, sliceStr[3]);
				newNode->thrItem.index = curNode->netInfo.index;
				newNode->thrItem.isEnable = pThrItem->isEnable;
				newNode->thrItem.maxThr = pThrItem->maxThr;
				newNode->thrItem.minThr = pThrItem->minThr;
				newNode->thrItem.thrType = pThrItem->thrType;
				newNode->thrItem.lastingTimeS = pThrItem->lastingTimeS;
				newNode->thrItem.intervalTimeS = pThrItem->intervalTimeS;
				newNode->thrItem.checkType = pThrItem->checkType;
				newNode->thrItem.checkRetValue = DEF_INVALID_VALUE;
				newNode->thrItem.checkSrcValList.head = NULL;
				newNode->thrItem.checkSrcValList.nodeCnt = 0;
				newNode->thrItem.repThrTime = 0;
				//newNode->thrItem.isNormal = TRUE;
				//newNode->thrItem.isValid = TRUE;
				newNode->thrItem.isNormal = pThrItem->isNormal;
				newNode->thrItem.isValid = pThrItem->isValid; 
				newNode->next = head->next;
				head->next = newNode;
			}
		}

	}
	return iRet;
}

static nm_thr_item_node_t * FindNMThrItemNodeWithName(nm_thr_item_list thrList, char * adpName, char * tagName)
{
	nm_thr_item_node_t * findNode = NULL, *head = NULL;
	if(thrList == NULL || adpName == NULL || tagName == NULL) return findNode;
	head = thrList;
	findNode = head->next;
	while(findNode)
	{
		if(!strcmp(findNode->thrItem.adapterName, adpName) && !strcmp(findNode->thrItem.tagName, tagName)) break;
		else
		{
			findNode = findNode->next;
		}
	}
	return findNode;
}

static int DeleteNMThrItemNodeWithName(nm_thr_item_list thrList, char * adpName, char * tagName)
{
	int iRet = -1;
	nm_thr_item_node_t * delNode = NULL, *head = NULL;
	nm_thr_item_node_t * p = NULL;
	if(thrList == NULL || adpName == NULL || tagName == NULL) return iRet;
	head = thrList;
	p = head;
	delNode = head->next;
	while(delNode)
	{
		if(!strcmp(delNode->thrItem.adapterName, adpName) && !strcmp(delNode->thrItem.tagName, tagName))
		{
			p->next = delNode->next;
			if(delNode->thrItem.checkSrcValList.head)
			{
				check_value_node_t * frontValueNode = delNode->thrItem.checkSrcValList.head;
				check_value_node_t * delValueNode = frontValueNode->next;
				while(delValueNode)
				{
					frontValueNode->next = delValueNode->next;
					free(delValueNode);
					delValueNode = frontValueNode->next;
				}
				free(delNode->thrItem.checkSrcValList.head);
				delNode->thrItem.checkSrcValList.head = NULL;
			}
			free(delNode);
			delNode = NULL;
			iRet = 0;
			break;
		}
		else
		{
			p = delNode;
			delNode = delNode->next;
		}
	}
	if(iRet == -1) iRet = 1;
	return iRet;
}

static int DeleteAllNMThrItemNode(nm_thr_item_list thrList)
{
	int iRet = -1;
	nm_thr_item_node_t * delNode = NULL, *head = NULL;
	if(thrList == NULL) return iRet;
	head = thrList;

	delNode = head->next;
	while(delNode)
	{
		head->next = delNode->next;
		if(delNode->thrItem.checkSrcValList.head)
		{
			check_value_node_t * frontValueNode = delNode->thrItem.checkSrcValList.head;
			check_value_node_t * delValueNode = frontValueNode->next;
			while(delValueNode)
			{
				frontValueNode->next = delValueNode->next;
				free(delValueNode);
				delValueNode = frontValueNode->next;
			}
			free(delNode->thrItem.checkSrcValList.head);
			delNode->thrItem.checkSrcValList.head = NULL;
		}
		free(delNode);
		delNode = head->next;
	}

	iRet = 0;
	return iRet;
}

static BOOL IsNMThrItemListEmpty(nm_thr_item_list thrList)
{
	BOOL bRet = TRUE;
	nm_thr_item_node_t * curNode = NULL, *head = NULL;
	if(thrList == NULL) return bRet;
	head = thrList;
	curNode = head->next;
	if(curNode != NULL) bRet = FALSE;
	return bRet;
}
//----------------------------------------------------------------------------------------------


//--------------------------------------linux function define-----------------------------------
#ifndef WIN32

unsigned long getCurrentTime()    
{    
	struct timeval tv;    
	gettimeofday(&tv,NULL);    
	return tv.tv_sec * 1000 + tv.tv_usec / 1000;    
}    

void getRecvSendBytes(char *adaptername, int *recvBytes, int *sendBytes)
{
	int a[10]={0};
	FILE *fp; 
	char str[1000];
	char *adapter[20]={9};

	strcpy(adapter, adaptername);
	if((fp = fopen("/proc/net/dev", "r+")) == NULL)  
	{  
		fprintf(stderr, "~~~~ note: %s~~~~ \n", strerror(errno));
		return;
	} 

	while( fgets(str, sizeof(str), fp) )
	{
		{
			int i = 0;
			int j = 0;
			int m=0;
			char p[20] = {0};
			char temp[20] = {0};
			if(strstr(str, adapter) != NULL)
			{
				while(str[i] != ':')
					i++;
				while(str[i]!= '\n' && m<8)
				{
					while(str[i] >= '0'&& str[i] <= '9')
					{
						p[j]=str[i];
						i++;
						j++;
					}
					if(j>0 && m<8)
					{
						memcpy(temp, p,j);
						temp[j]='\0';
						a[m++] = atoi(temp);
					}
					i++;
					j=0;
				}

			}

		}

	}
	fclose(fp);

	*recvBytes = a[0];
	*sendBytes = a[7];
}

void getNetUsage(char *adapterName, DWORD *netSpeedMbps, DWORD *RecvBytes, double *recvThroughput, DWORD *sendBytes, double *sendThroughput, double *netUsage)
{
	int recvBytes1 = 0;
	int sendBytes1 = 0;
	int recvBytes2 = 0;
	int sendBytes2 = 0;

	unsigned long startTime;
	unsigned long endTime;
	unsigned long bytes = 0;
	float interval = 0.0;
	int intervalSecond = 1;
	unsigned long BandWidth = 0;

	//startTime  = getCurrentTime();
	getRecvSendBytes(adapterName, &recvBytes1, &sendBytes1);
	app_os_sleep(intervalSecond *1000);//ms
	//endTime  = getCurrentTime();
	getRecvSendBytes(adapterName, &recvBytes2, &sendBytes2);
	//interval = (float)(endTime - startTime)/1000;

	*RecvBytes =recvBytes2 - recvBytes1;
	*sendBytes = sendBytes2 - sendBytes1;

	if( (*netSpeedMbps) != 0)
	{
		BandWidth = (*netSpeedMbps)*1024*1024;
		*recvThroughput = (float)(*RecvBytes)*8/intervalSecond/BandWidth;
		*sendThroughput = (float)(*sendBytes)*8/intervalSecond/BandWidth;
		*netUsage = (float)((*sendBytes) + (*RecvBytes))*8/intervalSecond/BandWidth;
	}
	else
	{
		*recvThroughput = 0.0;
		*sendThroughput = 0.0;
		*netUsage = 0.0;
	}
	return ;
}


void getAdapterNameList(char *str, char c)
{
	char s[20] = {0};
	char t[10] = {0};
	memcpy(t,&c,1);
	sprintf(s, "%s", "./netInfo.sh ");
	strcat(s, str);
	strcat(s, " ");
	strcat(s,t);
	system(s);
}

void getAdapterNameListEx(char *str, char c)
{
	char s[20] = {0};
	char t[10] = {0};
	memcpy(t,&c,1);
	sprintf(s, "%s", "./netInfo.sh ");
	strcat(s, str);
	strcat(s, " ");
	strcat(s,t);
	system(s);
}
void getNetworkCardList(char *str, char c)
{
	char s[20] = {0};
	char t[10] = {0};
	memcpy(t,&c,1);
	sprintf(s, "%s", "./netInfo.sh ");
	strcat(s, str);
	strcat(s, " ");
	strcat(s,t);
	system(s);
}

void getWiredStatus(char *adapterName, char c)
{
	char s[20] = {0};
	char t[10] = {0};
	memcpy(t,&c,1);
	sprintf(s, "%s", "./netInfo.sh ");
	strcat(s, adapterName);
	strcat(s, " ");
	strcat(s,t);
	system(s);
}

void getWiredSpeedMbpsTemp(char *adapterName, char c)
{
	char s[20] = {0};
	char t[10] = {0};
	memcpy(t,&c,1);
	sprintf(s, "%s", "./netInfo.sh ");
	strcat(s, adapterName);
	strcat(s, " ");
	strcat(s,t);
	system(s);
}

void getWirelessAdapterNameList(char *str, char c)
{
	char s[20] = {0};
	char t[10] = {0};
	memcpy(t,&c,1);
	sprintf(s, "%s", "./netInfo.sh ");
	strcat(s, str);
	strcat(s, " ");
	strcat(s,t);
	system(s);
}

void getWirelessSpeedMbps(char *adapterName, char c)
{
	char s[20] = {0};
	char t[10] = {0};
	memcpy(t,&c,1);
	sprintf(s, "%s", "./netInfo.sh ");
	strcat(s, adapterName);
	strcat(s, " ");
	strcat(s,t);
	system(s);
}

void getWirelessStatus(char *adapterName, char c)
{
	char s[20] = {0};
	char t[10] = {0};
	memcpy(t,&c,1);
	sprintf(s, "%s", "./netInfo.sh ");
	strcat(s, adapterName);
	strcat(s, " ");
	strcat(s,t);
	system(s);
}

void getRecvBytes(char *adaptername, char c)
{	
	char s[20] = {0};
	char t[10] = {0};
	memcpy(t,&c,1);
	sprintf(s, "%s", "./netInfo.sh ");
	strcat(s, adaptername);
	strcat(s, " ");
	strcat(s,t);
	system(s);
}

void getSendBytes(char *adaptername, char c)
{	
	char s[20] = {0};
	char t[10] = {0};
	memcpy(t,&c,1);
	sprintf(s, "%s", "./netInfo.sh ");
	strcat(s, adaptername);
	strcat(s, " ");
	strcat(s,t);
	system(s);
}

int getNumberOfAdapters(char* fileName)
{
	int lineNum = 0;
	FILE *fp = NULL;
	char str[128] = {0};

	if((fp=fopen(fileName,"r+")) == NULL)
	{
		fprintf(stderr, "~~~~ note: %s~~~~ \n", strerror(errno));
		return false;
	}
	while(fgets(str,sizeof(str),fp))
	{
		lineNum++;
	}
	fclose(fp);
	return lineNum;
}

BOOL whetherExistWirelessCard(char* fileName)
{
	FILE *fp = NULL;
	char str[128] = {0};
	BOOL bRet = false;
	char ch;

	if((fp=fopen(fileName,"r+")) == NULL)
	{
		fprintf(stderr, "~~~~ note: %s~~~~ \n", strerror(errno));
		return false;
	}
	ch=fgetc(fp);
	if(ch==EOF || (ch=='\n')) 
	{
		bRet = TRUE;
	}
	fclose(fp);
	return bRet;
}

BOOL isNetDisconnect(char* fileName)
{
	FILE *fp = NULL;
	char str[128] = {0};
	BOOL bRet = false;
	char ch;

	if((fp=fopen(fileName,"r+")) == NULL)
	{
		fprintf(stderr, "~~~~ note: %s~~~~ \n", strerror(errno));
		return false;
	}
	ch=fgetc(fp);
	if(ch==EOF || (ch=='\n')) 
	{
		bRet = TRUE;
	}
	fclose(fp);
	return bRet;
}

BOOL getNetStatus(char *adaptername, char c)
{
	char s[20] = {0};
	char t[10] = {0};
	memcpy(t,&c,1);
	sprintf(s, "%s", "./netInfo.sh ");
	strcat(s, adaptername);
	strcat(s, " ");
	strcat(s,t);
	system(s);
}

#endif 
//----------------------------------------------------------------------------------------------

//--------------------------------------other function define-----------------------------------
#ifdef WIN32
BOOL GetAdapterNameEx(char* index, char* str)
{
	BOOL isFind = FALSE;

	FILE *fd = NULL;
	char buf[512] = {0};
	char buf1[512] = {0};
	char Idx[512] = {0};
	char Met[512] = {0};
	char MTU[512] = {0};
	char status[512] = {0};
	char name[512] = {0};

	sprintf(buf, "cmd.exe /c netsh interface ip show interface");
	fd = _popen(buf, "r");
	while (fgets(buf1, sizeof(buf1), fd))
	{
		//puts(buf);
		//Idx     Met         MTU          state               name
		sscanf(buf1, "%s%s%s%s%[^/n/r]", Idx,Met,MTU,status, name);

		if( !strcmp(Idx, index) )
		{
			isFind = TRUE;
			formatNetStr(name);
			strcpy(str, name);
			break;
		}
	}
	_pclose(fd);
	return isFind;
}

//BOOL GetAdapterFriendlyNameWithGUIDName(wchar_t * guidName, char * friendlyName, int index)
BOOL GetAdapterFriendlyNameWithGUIDName(wchar_t * guidName, char * friendlyName)
{
	BOOL bRet = FALSE;
	if(guidName == NULL || friendlyName == NULL) return bRet;
	{
		HANDLE hMprConfig; 
		DWORD dwRet=0; 
		PIP_INTERFACE_INFO plfTable = NULL; 
		DWORD dwBufferSize=0; 
		char szFriendName[256] = {0}; 
		DWORD tchSize = sizeof(TCHAR) * 256;
		UINT i = 0;
		dwRet = app_os_MprConfigServerConnect(NULL, &hMprConfig);
		if(dwRet == NO_ERROR)
		{
			dwRet = app_os_MprConfigGetFriendlyName(hMprConfig, guidName,  
				(PWCHAR)szFriendName, tchSize);
			if(dwRet == NO_ERROR) 
			{
				char * tmpName = UnicodeToANSI(szFriendName);
				strcpy(friendlyName, tmpName);
				free(tmpName);
				bRet = TRUE;
			}
			else if(dwRet = ERROR_NOT_FOUND) //1168L 0x490
			{
				char osName[DEF_OS_NAME_LEN] = {0};
				if(!app_os_get_os_name(osName)) return;
				if((strcmp(osName, OS_WINDOWS_XP)==0) || (strcmp(osName, OS_WINDOWS_SERVER_2003)==0))
				{
					;
				}
				else if((strcmp(osName, OS_WINDOWS_7)==0) || (strcmp(osName, OS_WINDOWS_VISTA)==0)
					|| (strcmp(osName, OS_WINDOWS_8)==0) || (strcmp(osName, OS_WINDOWS_8_1)==0))
				{
					char Idx[512] = {0};
					itoa(g_indexForWin7, Idx, 10);
					GetAdapterNameEx(Idx, friendlyName);
				}
			}
		}

	}
	return bRet;
}
#endif

BOOL GetNetInfoWithIndex(net_info_t * pNetInfo, DWORD index)
{
	BOOL bRet = FALSE;

#ifdef WIN32

	MIB_IFTABLE *pIfTable;
	MIB_IFROW *pIfRow;
	DWORD dwSize = 0;
	DWORD dwRetVal = 0;
	pIfTable = (MIB_IFTABLE *)malloc(sizeof (MIB_IFTABLE));
	memset(pIfTable, 0, sizeof(MIB_IFTABLE));
	dwSize = sizeof (MIB_IFTABLE);
	if (GetIfTable(pIfTable, &dwSize, FALSE) == ERROR_INSUFFICIENT_BUFFER) 
	{
		free(pIfTable);
		pIfTable = (MIB_IFTABLE *)malloc(dwSize);
		memset(pIfTable, 0, dwSize);
	}

	if ((dwRetVal = GetIfTable(pIfTable, &dwSize, FALSE)) == NO_ERROR)
	{
		unsigned int i = 0;
		for (i = 0; i < pIfTable->dwNumEntries; i++) 
		{
			pIfRow = (MIB_IFROW *)&pIfTable->table[i];
			if(pIfRow->dwIndex == index)
			{
				if(strlen(pNetInfo->adapterName) == 0)
				{
					g_indexForWin7 = index;
					GetAdapterFriendlyNameWithGUIDName(pIfRow->wszName, pNetInfo->adapterName);
				}
				pNetInfo->id = index;
				if(strlen(pNetInfo->adapterDescription) == 0)
				{
					strcpy(pNetInfo->adapterDescription, pIfRow->bDescr);
				}
				if(strlen(pNetInfo->type) == 0)
				{
					switch (pIfRow->dwType) 
					{
					case IF_TYPE_OTHER:
						strcpy(pNetInfo->type, "Other");
						break;
					case IF_TYPE_ETHERNET_CSMACD:
						strcpy(pNetInfo->type, "Ethernet");
						break;
					case IF_TYPE_ISO88025_TOKENRING:
						strcpy(pNetInfo->type, "Token Ring");
						break;
					case IF_TYPE_PPP:
						strcpy(pNetInfo->type, "PPP");
						break;
					case IF_TYPE_SOFTWARE_LOOPBACK:
						strcpy(pNetInfo->type, "Software Lookback");
						break;
					case IF_TYPE_ATM:
						strcpy(pNetInfo->type, "ATM");
						break;
					case IF_TYPE_IEEE80211:
						strcpy(pNetInfo->type, "IEEE 802.11 Wireless");
						break;
					case IF_TYPE_TUNNEL:
						strcpy(pNetInfo->type, "Tunnel type encapsulation");
						break;
					case IF_TYPE_IEEE1394:
						strcpy(pNetInfo->type, "IEEE 1394 Firewire");
						break;
					default:
						strcpy(pNetInfo->type, "Unknown type");
						break;
					}
				}

				pNetInfo->netSpeedMbps = pIfRow->dwSpeed/1000000;
				pNetInfo->netStatus;
#if 0
				switch (pIfRow->dwOperStatus) 
				{
				case IF_OPER_STATUS_NON_OPERATIONAL:
					strcpy(pNetInfo->netStatus, "Non Operational");
					break;
				case IF_OPER_STATUS_UNREACHABLE:
					strcpy(pNetInfo->netStatus, "Unreachable");
					break;
				case IF_OPER_STATUS_DISCONNECTED:
					strcpy(pNetInfo->netStatus, "Disconnected");
					break;
				case IF_OPER_STATUS_CONNECTING:
					strcpy(pNetInfo->netStatus, "Connecting");
					break;
				case IF_OPER_STATUS_CONNECTED:
					strcpy(pNetInfo->netStatus, "Connected");
					break;
				case IF_OPER_STATUS_OPERATIONAL:
					strcpy(pNetInfo->netStatus, "Operational");
					break;
				default:
					strcpy(pNetInfo->netStatus, "Unknown status");
					break;
				}
#endif
				switch (pIfRow->dwOperStatus) 
				{
				case IF_OPER_STATUS_CONNECTED:
				case IF_OPER_STATUS_OPERATIONAL:
					strcpy(pNetInfo->netStatus, "Connected");
					break;
				default:
					strcpy(pNetInfo->netStatus, "Disconnect");
					break;
				}

				if(pIfRow->dwOutOctets > 0)
				{
					if(pNetInfo->initOutDataByte == 0) pNetInfo->initOutDataByte = pIfRow->dwOutOctets;
					pNetInfo->sendDateByte = pIfRow->dwOutOctets - pNetInfo->initOutDataByte;
					if((pNetInfo->oldOutDataByte != 0) && (pIfRow->dwSpeed != 0))
					{
						pNetInfo->sendThroughput = (pIfRow->dwOutOctets - pNetInfo->oldOutDataByte)*100/(pIfRow->dwSpeed/8.00);
					}
					else
					{
						//pNetInfo->sendThroughput = 0;
					}
					pNetInfo->oldOutDataByte = pIfRow->dwOutOctets;
				}
				else
				{
					pNetInfo->sendDateByte = 0;
					//pNetInfo->sendThroughput = 0;
				}

				if(pIfRow->dwInOctets)
				{
					if(pNetInfo->initInDataByte == 0) pNetInfo->initInDataByte = pIfRow->dwInOctets;
					pNetInfo->recvDateByte = pIfRow->dwInOctets - pNetInfo->initInDataByte;
					if((pNetInfo->oldInDataByte != 0) && (pIfRow->dwSpeed != 0))
					{
						pNetInfo->recvThroughput = (pIfRow->dwInOctets - pNetInfo->oldInDataByte)*100/(pIfRow->dwSpeed/8.00);
					}
					else
					{
						//pNetInfo->recvThroughput = 0;
					}
					pNetInfo->oldInDataByte = pIfRow->dwInOctets;
				}
				else
				{
					pNetInfo->recvDateByte = 0;
					//pNetInfo->recvThroughput = 0;
				}
				pNetInfo->netUsage = pNetInfo->sendThroughput+pNetInfo->recvThroughput;
				bRet = TRUE;
				break;
			}
		}
	}
	if (pIfTable != NULL) 
	{
		free(pIfTable);
		pIfTable = NULL;
	}

#else

	FILE *fp;
	char str[100] = {0};
	int line = 0;
	int i = 0;
	char ch = 0;

	pNetInfo->index = index;

	if(strcmp(pNetInfo->type, "Ethernet") ==0 )//wired
	{
		if(strcmp(pNetInfo->netStatus, "Connected") == 0)
		{
			//get BitRate
			char digitNum[20] = {0};
			getWiredSpeedMbpsTemp(pNetInfo->adapterName, 'b');
			if((fp=fopen("out.txt","r+")) == NULL)
			{
				fprintf(stderr, "~~~~ note: %s~~~~ \n", strerror(errno));
				return false;
			}
			fgets(str,sizeof(str),fp);
			formatNetStr(str);
			sscanf(str, "%[0-9]", digitNum);
			pNetInfo->netSpeedMbps = atoi(digitNum);	
			fclose(fp);

			//getNetUsage(pNetInfo->adapterName, &(pNetInfo->netSpeedMbps), &(pNetInfo->recvDateByte), &(pNetInfo->recvThroughput), &(pNetInfo->sendDateByte), &(pNetInfo->sendThroughput), &(pNetInfo->netUsage));
			goto done1;
		}
		else
		{
			pNetInfo->netSpeedMbps = 0;
			pNetInfo->netUsage = 0.0;
			pNetInfo->recvDateByte = 0;
			pNetInfo->recvThroughput = 0.0;
			pNetInfo->sendDateByte = 0;
			pNetInfo->sendThroughput = 0.0;
		}
	}
	else if(strcmp(pNetInfo->type, "Wireless") ==0 )//wireless
	{
		if(strcmp(pNetInfo->netStatus, "Connected") == 0)
		{
			//get BitRate 
			getWirelessSpeedMbps(pNetInfo->adapterName, '1');
			if((fp=fopen("out.txt","r+")) == NULL)
			{
				fprintf(stderr, "~~~~ note: %s~~~~ \n", strerror(errno));
				return false;
			}
			fgets(str,sizeof(str),fp);
			pNetInfo->netSpeedMbps = atoi(str);
			fclose(fp);

			//getNetUsage(pNetInfo->adapterName, &(pNetInfo->netSpeedMbps), &(pNetInfo->recvDateByte), &(pNetInfo->recvThroughput), &(pNetInfo->sendDateByte), &(pNetInfo->sendThroughput), &(pNetInfo->netUsage));
			goto done1;
		}
		else
		{
			pNetInfo->netSpeedMbps = 0;
			pNetInfo->netUsage = 0.0;
			pNetInfo->recvDateByte = 0;
			pNetInfo->recvThroughput = 0.0;
			pNetInfo->sendDateByte = 0;
			pNetInfo->sendThroughput = 0.0;
		}
	}


done1:
	{
		int recvBytes1 = 0;
		int sendBytes1 = 0;

		getRecvBytes(pNetInfo->adapterName, '3');
		if((fp=fopen("out.txt","r+")) == NULL)
		{
			fprintf(stderr, "~~~~ note: %s~~~~ \n", strerror(errno));
			return false;
		}
		fgets(str,sizeof(str),fp);
		formatNetStr(str);
		recvBytes1 = atoi(str);	
		if(recvBytes1)
		{
			if(pNetInfo->initInDataByte == 0) pNetInfo->initInDataByte = recvBytes1;
			pNetInfo->recvDateByte = recvBytes1 - pNetInfo->initInDataByte;
			if((pNetInfo->oldInDataByte != 0) && (pNetInfo->netSpeedMbps != 0))
			{
				//pNetInfo->recvThroughput = (recvBytes1 - pNetInfo->oldInDataByte)*100/(pNetInfo->netSpeedMbps/8.00);
				pNetInfo->recvThroughput = (float)(recvBytes1 - pNetInfo->oldInDataByte)*8*100/(pNetInfo->netSpeedMbps*1000*1000);
			}
			else
			{
				pNetInfo->recvThroughput = 0;
			}
			pNetInfo->oldInDataByte = recvBytes1;
		}
		else
		{
			pNetInfo->recvDateByte = 0;
			pNetInfo->recvThroughput = 0;
		}
		fclose(fp);


		getSendBytes(pNetInfo->adapterName, '4');
		if((fp=fopen("out.txt","r+")) == NULL)
		{
			fprintf(stderr, "~~~~ note: %s~~~~ \n", strerror(errno));
			return false;
		}
		fgets(str,sizeof(str),fp);
		formatNetStr(str);
		sendBytes1 = atoi(str);	
		if(sendBytes1 > 0)
		{
			if(pNetInfo->initOutDataByte == 0) pNetInfo->initOutDataByte = sendBytes1;
			pNetInfo->sendDateByte = sendBytes1 - pNetInfo->initOutDataByte;
			if((pNetInfo->oldOutDataByte != 0) && (pNetInfo->netSpeedMbps != 0))
			{
				//pNetInfo->sendThroughput = (sendBytes1 - pNetInfo->oldOutDataByte)*100/(pNetInfo->netSpeedMbps/8.00);
				pNetInfo->sendThroughput = (float)(sendBytes1 - pNetInfo->oldOutDataByte)*8*100/(pNetInfo->netSpeedMbps*1000*1000);
			}
			else
			{
				pNetInfo->sendThroughput = 0;
			}
			pNetInfo->oldOutDataByte = sendBytes1;
		}
		else
		{
			pNetInfo->sendDateByte = 0;
			pNetInfo->sendThroughput = 0;
		}
		fclose(fp);

		pNetInfo->netUsage = pNetInfo->recvThroughput + pNetInfo->sendThroughput;
	}


	getNetworkCardList("none",'L');
	if((fp=fopen("out.txt","r+")) == NULL)
	{
		fprintf(stderr, "~~~~ note: %s~~~~ \n", strerror(errno));
		return false;
	}
	//if netcard supports "lspci"
	fgets(str,sizeof(str),fp);
	formatNetStr(str);
	if(strcmp(str, "default") == 0)
	{
		if(strcmp(pNetInfo->type, "Ethernet") ==0 )//wired
		{
			strcpy(pNetInfo->adapterDescription, "Ethernet Network Adapter");
		}
		else if(strcmp(pNetInfo->type, "Wireless") ==0 )//wireless
		{
			strcpy(pNetInfo->adapterDescription, "Wireless Network Adapter");
		}
	}
	/*	
	else  
	{
	fclose(fp);
	fp=fopen("out.txt","r+");
	line = 0;
	while(fgets(str,sizeof(str),fp))
	{
	line++;
	}
	fclose(fp);
	if((fp=fopen("out.txt","r+")) == NULL)
	{
	fprintf(stderr, "~~~~ note: %s~~~~ \n", strerror(errno));
	return false;
	}
	while(fgets(str,sizeof(str),fp))
	{
	formatNetStr(str);
	if(strcmp(pNetInfo->adapterName, str) == 0)
	{
	for(i=0; i<line/2;i++)
	{
	fgets(str,sizeof(str),fp);
	}
	strcpy(pNetInfo->adapterDescription, str);
	break;
	}
	}
	fclose(fp);
	}
	*/	
	fclose(fp);


	bRet = TRUE;

#endif

	return bRet;
}

wchar_t * ANSITOUnicode(const char * str)
{
#ifdef WIN32

	int textLen = 0;
	wchar_t * wcRet = NULL;
	textLen = MultiByteToWideChar(CP_ACP, 0, str, -1, NULL, 0);
	wcRet = (wchar_t*)malloc((textLen+1)*sizeof(wchar_t));
	memset(wcRet, 0, (textLen+1)*sizeof(wchar_t));
	MultiByteToWideChar(CP_ACP, 0, str, -1, (LPWSTR)wcRet, textLen);
	return wcRet;
#endif
}

static BOOL ResetNetinfoListNodeisFoundFlag(net_info_list netInfoList)
{
	net_info_node_t * curNode = NULL, *head = NULL;
	if(netInfoList == NULL) 
		return FALSE;
	head = netInfoList;
	curNode = head->next;
	while(curNode)
	{
		curNode->netInfo.isFoundFlag = FALSE;
		curNode = curNode->next;
	}
	return TRUE;
}

static BOOL DelNodeWhenAdaperIsDisabled(net_info_list netInfoList)
{
	BOOL changeFlag = FALSE;
	net_info_list preNetNode = NULL;
	net_info_list curNetNode = NULL;

	if(!netInfoList)
		return FALSE;
	{
		preNetNode = netInfoList;
		curNetNode = preNetNode->next;
		while(curNetNode)
		{
			if(!curNetNode->netInfo.isFoundFlag)
			{
				changeFlag = TRUE;
				preNetNode->next = curNetNode->next;
				if(curNetNode)
				{
					free(curNetNode);		
					curNetNode = NULL;
				}
				curNetNode = preNetNode;
			}
			curNetNode = curNetNode->next;
			preNetNode = preNetNode->next;
		}
	}
	return changeFlag;
}

static BOOL GetNetInfo(net_info_list netInfoList)
{
#ifdef WIN32

	BOOL bRet = FALSE;
	PIP_ADAPTER_INFO pAdapterInfo = NULL;
	PIP_ADAPTER_INFO pAdInfo = NULL;
	ULONG ulSizeAdapterInfo = 0;
	DWORD dwStatus;
	BOOL changeFlag = FALSE;

	if(netInfoList == NULL) return bRet;
	ulSizeAdapterInfo = sizeof(IP_ADAPTER_INFO); 
	pAdapterInfo = (PIP_ADAPTER_INFO)malloc(ulSizeAdapterInfo);
	if (GetAdaptersInfo( pAdapterInfo, &ulSizeAdapterInfo) != ERROR_SUCCESS) 
	{
		free (pAdapterInfo);
		pAdapterInfo = (PIP_ADAPTER_INFO)malloc(ulSizeAdapterInfo);
	}

	dwStatus = GetAdaptersInfo(pAdapterInfo,   &ulSizeAdapterInfo);

	if(dwStatus != ERROR_SUCCESS)  
	{  
		free(pAdapterInfo);  
		return bRet;  
	}
	pAdInfo = pAdapterInfo; 

	ResetNetinfoListNodeisFoundFlag(netInfoList);

	while(pAdInfo)  
	{  	
		if(pAdInfo->Type != MIB_IF_TYPE_ETHERNET && pAdInfo->Type != IF_TYPE_IEEE80211) 
		{
			pAdInfo = pAdInfo->Next; 
			continue;
		}
		{
			net_info_t * pNetInfo = NULL;
			net_info_node_t * findNode = NULL;
			findNode = FindNetInfoNodeWithIndex(netInfoList, pAdInfo->Index);
			if(findNode)
			{
				findNode->netInfo.isFoundFlag = TRUE;
				pNetInfo = &findNode->netInfo;
				if(GetNetInfoWithIndex(pNetInfo, pAdInfo->Index))
				{
					if(strlen(pNetInfo->adapterName)==0)
					{
						char tmpName[256] = {0};
						char guidName[256] = {0};
						wchar_t * wcGUIDName = NULL;
						sprintf_s(guidName, sizeof(guidName), "\\DEVICE\\TCPIP_%s", pAdInfo->AdapterName);
						wcGUIDName = ANSITOUnicode(guidName);
						GetAdapterFriendlyNameWithGUIDName(wcGUIDName, tmpName);
						strcpy(pNetInfo->adapterName, tmpName);
						if(wcGUIDName != NULL) free(wcGUIDName);
					}
				}
			}
			else
			{
				net_info_t netInfo;
				memset(&netInfo, 0, sizeof(net_info_t));
				if(GetNetInfoWithIndex(&netInfo, pAdInfo->Index))
				{
					netInfo.isFoundFlag = TRUE;
					if(strlen(netInfo.adapterName)==0)
					{
						char tmpName[256] = {0};
						char guidName[256] = {0};
						wchar_t * wcGUIDName = NULL;
						sprintf_s(guidName, sizeof(guidName), "\\DEVICE\\TCPIP_%s", pAdInfo->AdapterName);
						wcGUIDName = ANSITOUnicode(guidName);
						GetAdapterFriendlyNameWithGUIDName(wcGUIDName, tmpName);
						strcpy(netInfo.adapterName, tmpName);
						if(wcGUIDName != NULL) free(wcGUIDName);
					}
					InsertNetInfoNode(netInfoList, &netInfo);
				}

				changeFlag = TRUE;
			}
		}	
		pAdInfo = pAdInfo->Next; 
	}
	free(pAdapterInfo);

	if( DelNodeWhenAdaperIsDisabled(netInfoList) )
		changeFlag = TRUE;

	if(changeFlag)
	{
		GetCapability(netmon_send_capability_infospec_cbf, NULL);
	}

#else
	BOOL changeFlag = FALSE;
	BOOL bRet = FALSE;
	int adapterNum = 0;
	static int preAdapterNum = 0;
	int i = 0;
	FILE *fp;
	int line = 0;
	char str[100] = {0};
	char aliasStr[256] = {0};
	int index = 0;
	char ch;
	net_info_t netInfoTemp;
	net_info_t * pNetInfo = &netInfoTemp;
	BOOL bFlag = false;

	memset( &netInfoTemp, 0, sizeof(net_info_t));
	ResetNetinfoListNodeisFoundFlag(netInfoList);
	getAdapterNameList("none",'M');
	adapterNum = getNumberOfAdapters("out.txt");

	for(i=0; i<adapterNum; i++)
	{
		memset(pNetInfo, 0, sizeof(net_info_t));
		getAdapterNameList("none",'M');
		if((fp=fopen("out.txt","r+")) == NULL)
		{
			fprintf(stderr, "~~~~ note: %s~~~~ \n", strerror(errno));
			return false;
		}
		line = 0;
		while(fgets(str,sizeof(str),fp))
		{
			if(line == i)
				break;
			line++;
		}
		formatNetStr(str);
		strcpy(pNetInfo->adapterName, str);
		fclose(fp);

		memset(aliasStr,0,sizeof(aliasStr));
		getAdapterNameListEx("none",'N');
		if((fp=fopen("out.txt","r+")) == NULL)
		{
			fprintf(stderr, "~~~~ note: %s~~~~ \n", strerror(errno));
			return false;
		}
		while(fgets(str,sizeof(str),fp))
		{
			formatNetStr(str);
			if((strstr(str,pNetInfo->adapterName)!=NULL) && (strstr(str, ":")!=NULL))
			{
				if(strlen(aliasStr))
					sprintf(aliasStr, "%s;%s", aliasStr, str);
				else 
					sprintf(aliasStr, "%s", str);
			}
			continue;
		}
		strcpy(pNetInfo->alias, aliasStr);
		fclose(fp);

		getNetStatus(pNetInfo->adapterName,'5');
		strcpy(pNetInfo->netStatus, "DisConnect");
		bFlag = false;
		bFlag = isNetDisconnect("out.txt");
		if(!bFlag)
			strcpy(pNetInfo->netStatus, "Connected");

		index = i;
		//is wired or wireless
		bFlag = false;
		strcpy(pNetInfo->type, "Ethernet");
		getWirelessAdapterNameList("none", '0');
		bFlag = whetherExistWirelessCard("out.txt");
		if(!bFlag)//exist:0
		{
			getWirelessAdapterNameList("none", '0');
			if((fp=fopen("out.txt","r+")) == NULL)
			{
				fprintf(stderr, "~~~~ note: %s~~~~ \n", strerror(errno));
				return false;
			}
			while(fgets(str,sizeof(str),fp))
			{
				formatNetStr(str);
				if(strcmp(pNetInfo->adapterName, str) == 0)
				{
					strcpy(pNetInfo->type, "Wireless");
					break;
				}	
			}
			fclose(fp);
		}

		{
			net_info_node_t * findNode = NULL;
			findNode = FindNetInfoNodeWithAdapterName(netInfoList, pNetInfo->adapterName);
			if(findNode)
			{
				net_info_t netInfo;
				net_info_t * ppNetInfo = &netInfo;
				memset( ppNetInfo, 0, sizeof(net_info_t));
				ppNetInfo = &findNode->netInfo;
				ppNetInfo->isFoundFlag = TRUE;

				if(GetNetInfoWithIndex(ppNetInfo, index) != TRUE)
					printf("GetNetInfoWithIndex() function error. \n");
			}
			else
			{
				changeFlag = TRUE;
				pNetInfo->isFoundFlag = TRUE;

				if(GetNetInfoWithIndex(pNetInfo, index))
				{
					InsertNetInfoNode(netInfoList, pNetInfo);
					{
						net_info_t * qNetInfo = NULL;
						qNetInfo = netInfoList->next;
					}
				}
			}

		}
	}

	if( DelNodeWhenAdaperIsDisabled(netInfoList) ) 
	{
		changeFlag = TRUE;
	}

	if(changeFlag)
	{
		GetCapability(netmon_send_capability_infospec_cbf, NULL);
	}

#endif 

	return bRet = TRUE;
}

static CAGENT_PTHREAD_ENTRY(GetNetInfoThreadStart, args)
{
	netmon_handler_context_t * pNetMonHandlerContext = (netmon_handler_context_t*)args;
	net_info_list curNetInfoList = NULL;
	app_os_mutex_lock(&pNetMonHandlerContext->netInfoListMutex);
	if(pNetMonHandlerContext->netInfoList)
	{
		DeleteAllNetInfoNode(pNetMonHandlerContext->netInfoList);
	}
	else
	{
		pNetMonHandlerContext->netInfoList = CreateNetInfoList();
	}
	curNetInfoList = pNetMonHandlerContext->netInfoList;
	app_os_mutex_unlock(&pNetMonHandlerContext->netInfoListMutex);

	while(pNetMonHandlerContext->isGetNetInfoThreadRunning)
	{
		app_os_mutex_lock(&pNetMonHandlerContext->netInfoListMutex); 
		GetNetInfo(pNetMonHandlerContext->netInfoList);
		app_os_mutex_unlock(&pNetMonHandlerContext->netInfoListMutex);
		{
			//app_os_sleep(1000)
			int i = 0;
			for(i=0; pNetMonHandlerContext->isGetNetInfoThreadRunning && i<10; i++)
			{
				app_os_sleep(100);
			}
		}
	}
	return 0;
}

static BOOL ReplyNetMonInfo(cagent_handle_t cagentHandle)
{
	BOOL bRet = FALSE;
	netmon_handler_context_t *pNetMonHandlerContext = (netmon_handler_context_t *)&NetMonHandlerContext;
	cagent_status_t status = cagent_success;
	app_os_mutex_lock(&pNetMonHandlerContext->netInfoListMutex);
	{
		char * pSendVal = NULL;
		int jsonStrlen;
		char * str;

		str = (char*)&pNetMonHandlerContext->netInfoList;
		jsonStrlen = Pack_netmon_netinfo_rep(str, &pSendVal);

		if(jsonStrlen > 0 && pSendVal != NULL)
		{
			g_sendcbf(&g_PluginInfo, netmon_netinfo_rep, pSendVal, strlen(pSendVal)+1, NULL, NULL);
		}
		if(pSendVal)free(pSendVal);	
	}
	app_os_mutex_unlock(&pNetMonHandlerContext->netInfoListMutex);
	if(status == cagent_success)
	{
		bRet = TRUE;
	}
	return bRet;
}

static CAGENT_PTHREAD_ENTRY(NetMonUploadThreadStart30, args)
{
	netmon_handler_context_t *pNetMonHandlerContext = (netmon_handler_context_t *)&NetMonHandlerContext;
	clock_t timeoutNow = 0;
	BOOL isAuto = FALSE;
	int intervalMS = 0;
	int timeoutMS = 0;
	while (pNetMonHandlerContext->isNetMonUploadThreadRunning30)
	{
		app_os_mutex_lock(&pNetMonHandlerContext->autoParamsMutex30);
		isAuto = pNetMonHandlerContext->isNetMonAutoUpload30;
		intervalMS = pNetMonHandlerContext->dataUploadIntervalMs30;
		timeoutMS = pNetMonHandlerContext->dataUploadTimeoutMs30;
		app_os_mutex_unlock(&pNetMonHandlerContext->autoParamsMutex30);

		if(!isAuto)
		{
			app_os_cond_wait(&pNetMonHandlerContext->netMonUploadSyncObj30, &pNetMonHandlerContext->netMonUploadSyncMutex30);
			if(!pNetMonHandlerContext->isNetMonUploadThreadRunning30)
			{
				break;
			}
			ReplyNetMonInfo(pNetMonHandlerContext->susiHandlerContext.cagentHandle);
		}
		else
		{
			timeoutNow = clock();
			if(pNetMonHandlerContext->autoUploadTimeoutStart == 0)
			{
				pNetMonHandlerContext->autoUploadTimeoutStart = timeoutNow;
			}

			if(timeoutNow - pNetMonHandlerContext->autoUploadTimeoutStart >= timeoutMS)
			{
				app_os_mutex_lock(&pNetMonHandlerContext->autoParamsMutex30);
				pNetMonHandlerContext->isNetMonAutoUpload30 = FALSE;
				app_os_mutex_unlock(&pNetMonHandlerContext->autoParamsMutex30);
				pNetMonHandlerContext->autoUploadTimeoutStart = 0;
			}
			else
			{
				app_os_sleep(intervalMS);
				ReplyNetMonInfo(pNetMonHandlerContext->susiHandlerContext.cagentHandle);
			}
		}
		app_os_sleep(10);
	}
	return 0;
}

static void RepNMData(net_info_list netInfoList)
{
	if(netInfoList == NULL) return;
	{
		char * nmJsonStr = NULL;
		int nmStrlen = 0;
		nmStrlen = Parser_PackNMData(netInfoList, &nmJsonStr);
		if(nmStrlen > 0 && nmJsonStr != NULL)
		{
			char * repJsonStr = NULL;
			int jsonStrlen = Parser_PackReportNMData(nmJsonStr, DEF_HANDLER_NAME , &repJsonStr);
			if(jsonStrlen > 0 && repJsonStr != NULL)
			{
				if(g_sendreportcbf)
					g_sendreportcbf(&g_PluginInfo, repJsonStr, strlen(repJsonStr)+1, NULL, NULL);
			}	
			if(repJsonStr)free(repJsonStr);
		}
		if(nmJsonStr)free(nmJsonStr);
	}
}


bool AutoReport_API_NetInfo()
{
	int len = 0;
	bool bRet = false;
	char *repJsonStr = NULL;
	cagent_status_t status;
	netmon_handler_context_t *pNetMonHandlerContext = (netmon_handler_context_t *)&NetMonHandlerContext;
	net_info_list pNetInfoList = NULL;
	pNetInfoList = pNetMonHandlerContext->netInfoList;

	if(pNetMonHandlerContext->isAutoUploadAll)
	{
		g_sendreportcbfFlag = TRUE;
		GetCapability(netmon_send_report_cbf, NULL);
	}
	else if( pNetMonHandlerContext->autoUploadRequestItemsList && !IsSensorInfoListEmpty(pNetMonHandlerContext->autoUploadRequestItemsList))
	{
		bRet = Parser_get_request_items_rep(pNetInfoList, pNetMonHandlerContext->autoUploadRequestItemsList, &pNetMonHandlerContext->isAutoUploadAllNet, &repJsonStr);

		if(!bRet || repJsonStr == NULL)
		{
			int sendLevel = netmon_send_report_cbf;
			char errorStr[128] = {0};
			//sprintf(errorStr, "Command(%d), Get capability error!", netmon_get_capability_req);
			sprintf(errorStr, "NetMonitor Get capability error!");
			g_sendreportcbf(&g_PluginInfo, errorStr, strlen(errorStr)+1, NULL, NULL);
			SendErrMsg( errorStr, sendLevel);
		}

		if( pNetMonHandlerContext->isAutoUploadAllNet)
		{
			g_sendreportcbfFlag = TRUE;
			GetCapability(netmon_send_report_cbf, NULL);
		}
		else
		{
			status = g_sendreportcbf(&g_PluginInfo, repJsonStr, strlen(repJsonStr)+1, NULL, NULL);
			if(repJsonStr)free(repJsonStr);
		}

		if(status == cagent_success)
		{
			bRet = TRUE;
		}
	}
	return bRet;
}


bool AutoReport_Recv_NetInfo()
{
	int len = 0;
	bool bRet = false;
	char *repJsonStr = NULL;
	cagent_status_t status;
	netmon_handler_context_t *pNetMonHandlerContext = (netmon_handler_context_t *)&NetMonHandlerContext;
	net_info_list pNetInfoList = NULL;
	pNetInfoList = pNetMonHandlerContext->netInfoList;

	if(pNetMonHandlerContext->isAutoReportAll)
	{
		GetCapability(netmon_send_cbf, netmon_auto_report_rep);
	}
	else if( pNetMonHandlerContext->autoReportRequestItemsList && !IsSensorInfoListEmpty(pNetMonHandlerContext->autoReportRequestItemsList))
	{
		bRet = Parser_get_request_items_rep(pNetInfoList, pNetMonHandlerContext->autoReportRequestItemsList, &pNetMonHandlerContext->isAutoReportAllNet, &repJsonStr);

		if(!bRet || repJsonStr == NULL)
		{
			int sendLevel = netmon_send_report_cbf;
			char errorStr[128] = {0};
			//sprintf(errorStr, "Command(%d), Get capability error!", netmon_get_capability_req);
			sprintf(errorStr, "NetMonitor Get capability error!");
			g_sendcbf(&g_PluginInfo, netmon_error_rep, errorStr, strlen(errorStr)+1, NULL, NULL);
			SendErrMsg( errorStr, sendLevel);
		}

		if( pNetMonHandlerContext->isAutoReportAllNet)
		{
			GetCapability(netmon_send_cbf, NULL);
		}
		else
		{
			status = g_sendcbf(&g_PluginInfo, netmon_auto_report_rep, repJsonStr, strlen(repJsonStr)+1, NULL, NULL);
			if(repJsonStr)free(repJsonStr);
		}

		if(status == cagent_success)
		{
			bRet = TRUE;
		}
	}
	return bRet;
}

static CAGENT_PTHREAD_ENTRY(NMAutoReport_API_ThreadStart, args)
{
	netmon_handler_context_t *pNetMonHandlerContext = (netmon_handler_context_t *)&NetMonHandlerContext;
	BOOL isAuto = FALSE;
	int intervalMS = 0;

	while (pNetMonHandlerContext->IsNMAutoUploadThreadRunning)
	{
		app_os_mutex_lock(&pNetMonHandlerContext->NMAutoUploadParamsMutex);
		isAuto = pNetMonHandlerContext->isNetMonAutoUpload;
		intervalMS = NMUploadParams.intervalTimeMs;
		app_os_mutex_unlock(&pNetMonHandlerContext->NMAutoUploadParamsMutex);
		if(!isAuto)
		{
			app_os_cond_wait(&pNetMonHandlerContext->netAutoUploadSyncObj, &pNetMonHandlerContext->netMonUploadSyncMutex);

			if(!pNetMonHandlerContext->IsNMAutoUploadThreadRunning)
			{
				break;
			}

			AutoReport_API_NetInfo();
		}  
		else
		{
			app_os_sleep(intervalMS);
			AutoReport_API_NetInfo();
		}
		app_os_sleep(100);
	}
	return 0;
}

static CAGENT_PTHREAD_ENTRY(NMAutoReport_Recv_ThreadStart, args)
{
	netmon_handler_context_t *pNetMonHandlerContext = (netmon_handler_context_t *)&NetMonHandlerContext;
	BOOL isAuto = FALSE;
	int intervalMS = 0;
	int timeoutMS = 0;
	clock_t timeoutNow = 0;

	while (pNetMonHandlerContext->IsNMAutoReportThreadRunning)
	{
		app_os_mutex_lock(&pNetMonHandlerContext->NMAutoReportParamsMutex);
		isAuto = pNetMonHandlerContext->isNetMonAutoReport;
		intervalMS = NMReportParams.intervalTimeMs;
		timeoutMS = NMReportParams.tmeoutMs;
		app_os_mutex_unlock(&pNetMonHandlerContext->NMAutoReportParamsMutex);
		if(!isAuto)
		{
			app_os_cond_wait(&pNetMonHandlerContext->netAutoReportSyncObj, &pNetMonHandlerContext->netMonReportSyncMutex);
			if(!pNetMonHandlerContext->IsNMAutoReportThreadRunning)
			{
				break;
			}
			AutoReport_Recv_NetInfo();
		}  
		else
		{
			timeoutNow = clock();
			if(pNetMonHandlerContext->autoReportTimeoutStart == 0)
			{
				pNetMonHandlerContext->autoReportTimeoutStart = timeoutNow;
			}

			if(timeoutNow - pNetMonHandlerContext->autoReportTimeoutStart >= timeoutMS)
			{
				app_os_mutex_lock(&pNetMonHandlerContext->NMAutoReportParamsMutex);
				pNetMonHandlerContext->isNetMonAutoReport = FALSE;
				app_os_mutex_unlock(&pNetMonHandlerContext->NMAutoReportParamsMutex);
				pNetMonHandlerContext->autoReportTimeoutStart = 0;
			}
			else
			{
				app_os_sleep(intervalMS);
				AutoReport_Recv_NetInfo();
			}
		}
		app_os_sleep(100);
	}
	return 0;
}

static char * GetNMCapabilityString()
{
	char * cpStr = NULL;
	netmon_handler_context_t *pNetMonHandlerContext = (netmon_handler_context_t *)&NetMonHandlerContext;
	//app_os_mutex_lock(&pNetMonHandlerContext->netInfoListMutex); 
	if(pNetMonHandlerContext->netInfoList && pNetMonHandlerContext->netInfoList->next)
	{
		char * repJsonStr = NULL;
		int jsonStrlen = 0;
		jsonStrlen = Parser_PackCapabilityStrRep(pNetMonHandlerContext->netInfoList, &repJsonStr);
		if(jsonStrlen > 0 && repJsonStr != NULL)
		{
			int len;
			len = strlen(repJsonStr) +1;
			cpStr = (char*)malloc(len);
			memset(cpStr, 0, len);
			strcpy(cpStr, repJsonStr);
		}
		if(repJsonStr)free(repJsonStr);	
	}
	//app_os_mutex_unlock(&pNetMonHandlerContext->netInfoListMutex);
	return cpStr;
}

static void GetCapability(int sendLevel, int repCommandID)
{
	char * cpbStr = NULL;
	cpbStr = GetNMCapabilityString();
	if(cpbStr != NULL)
	{
		switch(sendLevel)
		{
		case netmon_send_report_cbf:
			g_sendreportcbf(&g_PluginInfo, cpbStr, strlen(cpbStr)+1, NULL, NULL);
			break;
		case netmon_send_capability_infospec_cbf:
			g_sendcapabilitycbf(&g_PluginInfo, cpbStr, strlen(cpbStr)+1, NULL, NULL);
			break;
		case netmon_send_cbf:
			g_sendcbf(&g_PluginInfo, repCommandID, cpbStr, strlen(cpbStr)+1, NULL, NULL);
			break;
		default:
			g_sendcbf(&g_PluginInfo, repCommandID, cpbStr, strlen(cpbStr)+1, NULL, NULL);
			break;
		}
		if(cpbStr)
			free(cpbStr);
	}
	else
	{
		int sendLevel = netmon_send_cbf;
		char * errorRepJsonStr = NULL;
		char errorStr[128];
		int jsonStrlen = 0;
		sprintf(errorStr, "Command(%d), Get capability error!", netmon_get_capability_rep);
		if(g_sendreportcbfFlag)
			sendLevel = netmon_send_report_cbf;
		SendErrMsg( errorStr, sendLevel);
	}
}

static void SendErrMsg(char * errorStr,int sendLevel)
{
	char * uploadRepJsonStr = NULL;
	char * str = errorStr;
	int jsonStrlen = 0;

	jsonStrlen = Pack_netmon_error_rep(errorStr, &uploadRepJsonStr);
	if(jsonStrlen > 0 && uploadRepJsonStr != NULL)
	{
		switch(sendLevel)
		{
		case netmon_send_cbf:
			{
				g_sendcbf(&g_PluginInfo, netmon_error_rep, uploadRepJsonStr, strlen(uploadRepJsonStr)+1, NULL, NULL);
				break;
			}
		case netmon_send_report_cbf:
			{
				g_sendreportcbf(&g_PluginInfo, uploadRepJsonStr, strlen(uploadRepJsonStr)+1, NULL, NULL);
				break;
			}
		default:
			{
				g_sendcbf(&g_PluginInfo, netmon_error_rep, uploadRepJsonStr, strlen(uploadRepJsonStr)+1, NULL, NULL);
				break;
			}
		}
	}
	if(uploadRepJsonStr)free(uploadRepJsonStr);	
}


static BOOL InitNetMonThrFromConfig(nm_thr_item_list thrList, char * cfgPath)
{
	netmon_handler_context_t *pNetMonHandlerContext = (netmon_handler_context_t *)&NetMonHandlerContext;
	net_info_list curNetInfoList;
	BOOL bRet = FALSE;
	FILE *fptr = NULL;
	char * pTmpThrInfoStr = NULL;
	if(thrList == NULL || cfgPath == NULL) return bRet;
	if ((fptr = fopen(cfgPath, "rb")) == NULL) return bRet;
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
				app_os_mutex_lock(&pNetMonHandlerContext->netInfoListMutex);
				curNetInfoList = pNetMonHandlerContext->netInfoList;
				if(Parser_ParseThrInfo(pTmpThrInfoStr, curNetInfoList, thrList)) //in in out
				{
					bRet = TRUE;
				}
				app_os_mutex_unlock(&pNetMonHandlerContext->netInfoListMutex);
			}
			if(pTmpThrInfoStr) free(pTmpThrInfoStr);
		}
	}
	fclose(fptr);
	return bRet;
}

static void NetMonThrFillConfig(nm_thr_item_list thrList, char * cfgPath)
{
	if(thrList == NULL || cfgPath == NULL) return;
	{
		char * pJsonStr = NULL;
		int jsonLen = 0;
		jsonLen = Parser_PackThrInfo(thrList, &pJsonStr);
		if(jsonLen > 0 && pJsonStr != NULL)
		{
			FILE * fPtr = fopen(cfgPath, "wb");
			if(fPtr)
			{
				fwrite(pJsonStr, 1, jsonLen, fPtr);
				fclose(fPtr);
			}
			free(pJsonStr);
		}
	}
}

static BOOL NetMonCheckSrcVal(nm_thr_item_t * pThrItemInfo, float * pCheckValue, check_value_type_t valType)
{
	BOOL bRet = FALSE;
	if(pThrItemInfo == NULL || pCheckValue == NULL) return bRet;
	{
		long long nowTime = time(NULL);
		pThrItemInfo->checkRetValue = DEF_INVALID_VALUE;
		if(pThrItemInfo->checkSrcValList.head == NULL)
		{
			pThrItemInfo->checkSrcValList.head = (check_value_node_t *)malloc(sizeof(check_value_node_t));
			pThrItemInfo->checkSrcValList.nodeCnt = 0;
			pThrItemInfo->checkSrcValList.head->checkValTime = DEF_INVALID_TIME;
			pThrItemInfo->checkSrcValList.head->ckV = DEF_INVALID_VALUE;
			pThrItemInfo->checkSrcValList.head->next = NULL;
		}

		if(pThrItemInfo->checkSrcValList.nodeCnt > 0)
		{
			long long minCkvTime = 0;
			check_value_node_t * curNode = pThrItemInfo->checkSrcValList.head->next;
			minCkvTime = curNode->checkValTime;
			while(curNode)
			{
				if(curNode->checkValTime < minCkvTime)  minCkvTime = curNode->checkValTime;
				curNode = curNode->next; 
			}

			if(nowTime - minCkvTime >= pThrItemInfo->lastingTimeS)
			{
				switch(pThrItemInfo->checkType)
				{
				case ck_type_avg:
					{
						check_value_node_t * curNode = pThrItemInfo->checkSrcValList.head->next;
						float avgTmpF = 0;
						int avgTmpI = 0;
						while(curNode)
						{
							if(curNode->ckV != DEF_INVALID_VALUE) 
							{
								switch(valType)
								{
								case VT_F:
									{
										avgTmpF += curNode->ckV;
										break;
									}
								default: break;
								}
							}
							curNode = curNode->next; 
						}
						if(pThrItemInfo->checkSrcValList.nodeCnt > 0)
						{
							avgTmpF = avgTmpF/pThrItemInfo->checkSrcValList.nodeCnt;
							pThrItemInfo->checkRetValue = avgTmpF;
							bRet = TRUE;
						}
						break;
					}
				case ck_type_max:
					{
						check_value_node_t * curNode = pThrItemInfo->checkSrcValList.head->next;
						float maxTmpF = -999;
						int maxTmpI = -999;
						while(curNode)
						{
							switch(valType)
							{
							case VT_F:
								{
									if(curNode->ckV > maxTmpF) maxTmpF = curNode->ckV;
									break;
								}
							default: break;
							}
							curNode = curNode->next; 
						}
						switch(valType)
						{
						case VT_F:
							{
								if(maxTmpF > -999)
								{
									pThrItemInfo->checkRetValue = maxTmpF;
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
						check_value_node_t * curNode = pThrItemInfo->checkSrcValList.head->next;
						float minTmpF = 99999;
						int minTmpI = 99999;
						while(curNode)
						{
							switch(valType)
							{
							case VT_F:
								{
									if(curNode->ckV < minTmpF) minTmpF = curNode->ckV;
									break;
								}
							default: break;
							}
							curNode = curNode->next; 
						}
						switch(valType)
						{
						case VT_F:
							{
								if(minTmpF < 99999)
								{
									pThrItemInfo->checkRetValue = minTmpF;
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
					check_value_node_t * frontNode = pThrItemInfo->checkSrcValList.head;
					check_value_node_t * curNode = frontNode->next;
					check_value_node_t * delNode = NULL;
					while(curNode)
					{
						if(nowTime - curNode->checkValTime >= pThrItemInfo->lastingTimeS)
						{
							delNode = curNode;
							frontNode->next  = curNode->next;
							curNode = frontNode->next;
							free(delNode);
							pThrItemInfo->checkSrcValList.nodeCnt--;
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
		} //end if(pThrItemInfo->checkSrcValList.nodeCnt > 0)
		{
			check_value_node_t * head = pThrItemInfo->checkSrcValList.head;
			check_value_node_t * newNode = (check_value_node_t *)malloc(sizeof(check_value_node_t));
			newNode->checkValTime = nowTime;
			newNode->ckV = (*pCheckValue);
			newNode->next = head->next;
			head->next = newNode;
			pThrItemInfo->checkSrcValList.nodeCnt++;
		}
	}
	return bRet;
}

static BOOL NetMonCheckThrItem(nm_thr_item_t * pThrItemInfo, check_value_type_t valType, float * checkVal, char * checkRetMsg)
{
	BOOL bRet = FALSE;
	BOOL isTrigger = FALSE;
	BOOL triggerMax = FALSE;
	BOOL triggerMin = FALSE;
	char tmpRetMsg[1024] = {0};
	char checkTypeStr[32] = {0};
	if(pThrItemInfo == NULL || valType == VT_Unknown || checkVal == NULL || checkRetMsg== NULL) return bRet;
	{
		switch(pThrItemInfo->checkType)
		{
		case ck_type_avg:
			{
				sprintf(checkTypeStr, "average");
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
	}

	if(NetMonCheckSrcVal(pThrItemInfo, checkVal, valType) && pThrItemInfo->checkRetValue != DEF_INVALID_VALUE)
	{  
		switch(valType)
		{
		case VT_F:
			{
				if(pThrItemInfo->thrType & DEF_THR_MAX_TYPE)
				{
					if(pThrItemInfo->maxThr != DEF_INVALID_VALUE && (pThrItemInfo->checkRetValue > pThrItemInfo->maxThr))
					{

						char pathStr[256] = {0};
						char bnStr[256] = {0};
						sprintf(bnStr, "%s%d-%s", NETMON_GROUP_NETINDEX, pThrItemInfo->index, pThrItemInfo->adapterName);
						sprintf(pathStr, "%s/%s/%s/%s", DEF_HANDLER_NAME, NETMON_INFO_LIST, bnStr, pThrItemInfo->tagName);
						sprintf(tmpRetMsg, "%s #tk#%s#tk# (%f)> #tk#maxThreshold#tk# (%f)", pathStr, checkTypeStr, pThrItemInfo->checkRetValue, pThrItemInfo->maxThr);
						//sprintf(tmpRetMsg, "%s (#tk#%s#tk#:%f)> #tk#maxThreshold#tk# (%f)", pThrItemInfo->n, checkTypeStr, pThrItemInfo->checkRetValue, pThrItemInfo->maxThr);
						triggerMax = TRUE;
					}
				}
				if(pThrItemInfo->thrType & DEF_THR_MIN_TYPE)
				{
					if(pThrItemInfo->minThr != DEF_INVALID_VALUE && (pThrItemInfo->checkRetValue  < pThrItemInfo->minThr))
					{
						char pathStr[256] = {0};
						char bnStr[256] = {0};
						sprintf(bnStr, "%s%d-%s", NETMON_GROUP_NETINDEX, pThrItemInfo->index, pThrItemInfo->adapterName);
						sprintf(pathStr, "%s/%s/%s/%s", DEF_HANDLER_NAME, NETMON_INFO_LIST, bnStr, pThrItemInfo->tagName);
						sprintf(tmpRetMsg, "%s #tk#%s#tk# (%f)< #tk#minThreshold#tk# (%f)", pathStr, checkTypeStr, pThrItemInfo->checkRetValue, pThrItemInfo->minThr);
						//sprintf(tmpRetMsg, "%s (#tk#%s#tk#:%.0f)< #tk#minThreshold#tk# (%f)", pThrItemInfo->n, checkTypeStr, pThrItemInfo->checkRetValue, pThrItemInfo->minThr);
						triggerMin = TRUE;
					}
				}
				break;
			}
		}
	}

	switch(pThrItemInfo->thrType)
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
		if(pThrItemInfo->intervalTimeS == DEF_INVALID_TIME || pThrItemInfo->intervalTimeS == 0 || nowTime - pThrItemInfo->repThrTime >= pThrItemInfo->intervalTimeS)
		{
			pThrItemInfo->repThrTime = nowTime;
			pThrItemInfo->isNormal = FALSE;
			bRet = TRUE;
		}
	}
	else
	{
		if(!pThrItemInfo->isNormal && pThrItemInfo->checkRetValue != DEF_INVALID_VALUE)
		{
			memset(tmpRetMsg, 0, sizeof(tmpRetMsg));
			sprintf(tmpRetMsg, "%s %s", pThrItemInfo->n,DEF_NOR_EVENT_STR);
			//sprintf(tmpRetMsg, "%s (#tk#%s#tk#:%f) %s", pThrItemInfo->n, checkTypeStr, pThrItemInfo->checkRetValue, DEF_NOR_EVENT_STR);
			pThrItemInfo->isNormal = TRUE;
			bRet = TRUE;
		}
	}

	if(!bRet) sprintf(checkRetMsg,"");
	else sprintf(checkRetMsg, "%s", tmpRetMsg);

	return bRet;
}

static BOOL NetMonCheckThr(nm_thr_item_list curThrItemList, net_info_list curNetInfoList, char ** checkRetMsg, unsigned int bufLen)
{
	BOOL bRet = FALSE;
	if(curThrItemList == NULL || curNetInfoList == NULL || NULL == (char*)(*checkRetMsg)) return bRet;
	{
		nm_thr_item_node_t * curThrItemNode = NULL;
		char tmpMsg[1024*2] = {0};
		check_value_type_t valType = VT_Unknown;
		float curCheckValue;
		curThrItemNode = curThrItemList->next;
		while(curThrItemNode)
		{
			if(curThrItemNode->thrItem.isEnable && strlen(curThrItemNode->thrItem.adapterName)>0)
			{
				net_info_node_t * findNetInfoNode  = NULL;
				findNetInfoNode = FindNetInfoNodeWithAdapterName(curNetInfoList, curThrItemNode->thrItem.adapterName);
				if(findNetInfoNode == NULL)
				{
					if(bufLen<strlen(*checkRetMsg)+strlen(DEF_NOT_SUPT_EVENT_STR)+16)
					{
						int newLen = strlen(*checkRetMsg)+strlen(DEF_NOT_SUPT_EVENT_STR)+2*1024;
						*checkRetMsg = (char*)realloc(*checkRetMsg, newLen);
					}
					if(strlen(*checkRetMsg))sprintf(*checkRetMsg, "%s;adapterName:%s not support", *checkRetMsg, curThrItemNode->thrItem.adapterName);
					else sprintf(*checkRetMsg, "%s not support", curThrItemNode->thrItem.adapterName);
					curThrItemNode->thrItem.isEnable = FALSE;
					curThrItemNode = curThrItemNode->next;
					app_os_sleep(10);
					continue;
				}
				memset(tmpMsg, 0, sizeof(tmpMsg));
				if(!strcmp(curThrItemNode->thrItem.tagName, NET_INFO_INDEX))
				{
					valType = VT_F;
					curCheckValue = (float)findNetInfoNode->netInfo.index;
				}
				if(!strcmp(curThrItemNode->thrItem.tagName, NET_INFO_SPEED_MBPS))
				{
					valType = VT_F;
					curCheckValue = (float)findNetInfoNode->netInfo.netSpeedMbps;
				}
				if(!strcmp(curThrItemNode->thrItem.tagName, NET_INFO_SEND_BYTE))
				{
					valType = VT_F;
					curCheckValue = (float)findNetInfoNode->netInfo.sendDateByte;
				}

				if(!strcmp(curThrItemNode->thrItem.tagName, NET_INFO_RECV_BYTE))
				{
					valType = VT_F;
					curCheckValue = (float)findNetInfoNode->netInfo.recvDateByte;
				}
				if(!strcmp(curThrItemNode->thrItem.tagName, NET_INFO_USAGE))
				{
					valType = VT_F;
					curCheckValue = (float)findNetInfoNode->netInfo.netUsage;
				}
				if(!strcmp(curThrItemNode->thrItem.tagName, NET_INFO_SEND_THROUGHPUT))
				{
					valType = VT_F;
					curCheckValue = (float)findNetInfoNode->netInfo.sendThroughput;
				}
				if(!strcmp(curThrItemNode->thrItem.tagName, NET_INFO_RECV_THROUGHPUT))
				{
					valType = VT_F;
					curCheckValue = (float)findNetInfoNode->netInfo.recvThroughput;
				}
				NetMonCheckThrItem(&curThrItemNode->thrItem, valType, &curCheckValue, tmpMsg);//最终out

				if(strlen(tmpMsg))
				{
					if(bufLen<strlen(*checkRetMsg)+strlen(tmpMsg)+16)
					{
						int newLen = strlen(*checkRetMsg)+strlen(tmpMsg)+2*1024;
						*checkRetMsg = (char*)realloc(*checkRetMsg, newLen);
					}
					if(strlen(*checkRetMsg))sprintf(*checkRetMsg, "%s;%s", *checkRetMsg, tmpMsg);
					else sprintf(*checkRetMsg, "%s", tmpMsg);
				}

			}
			curThrItemNode = curThrItemNode->next;
			app_os_sleep(10);
		}
	}
	return bRet = TRUE;
}

static BOOL NetMonIsThrNormal(nm_thr_item_list thrList, BOOL * isNormal)
{
	BOOL bRet = FALSE;
	if(isNormal == NULL || thrList == NULL) return bRet;
	{
		nm_thr_item_node_t * curThrItemNode = NULL;
		curThrItemNode = thrList->next;
		while(curThrItemNode)
		{
			if(curThrItemNode->thrItem.isEnable && !curThrItemNode->thrItem.isNormal && curThrItemNode->thrItem.isValid)
			{
				*isNormal = FALSE;
				break;
			}
			curThrItemNode = curThrItemNode->next;
		}
	}
	bRet = TRUE;
	return bRet;
}

static CAGENT_PTHREAD_ENTRY(NMThrCheckThreadStart, args)
{
	char *repMsg = NULL;
	unsigned int bufLen = 4*1024;
	bool bRet = false;
	netmon_handler_context_t * pNetMonHandlerContext = &NetMonHandlerContext; 
	net_info_list curNetInfoList = NULL;
	repMsg = (char *)malloc(bufLen);
	memset(repMsg, 0, bufLen);
	while (IsNMThrCheckThreadRunning)
	{	
		app_os_mutex_lock(&pNetMonHandlerContext->netInfoListMutex);
		curNetInfoList = pNetMonHandlerContext->netInfoList;
		if(curNetInfoList!=NULL && curNetInfoList->next != NULL && NMThrItemList!=NULL && NMThrItemList->next != NULL)
		{
			app_os_mutex_lock(&NMThrInfoMutex);
			//if(strlen(NMThrItemList->next->thrItem.adapterName) == 0) 
			//	NetMonUpdateThr(NULL,NULL); 
			memset(repMsg, 0, bufLen);
			NetMonCheckThr(NMThrItemList, curNetInfoList, &repMsg, bufLen);
			app_os_mutex_unlock(&NMThrInfoMutex);
		}
		app_os_mutex_unlock(&pNetMonHandlerContext->netInfoListMutex);

		if(strlen(repMsg))
		{
			BOOL bRet = FALSE;
			BOOL isNormal = TRUE;
			app_os_mutex_lock(&NMThrInfoMutex);
			bRet = NetMonIsThrNormal(NMThrItemList, &isNormal);
			app_os_mutex_unlock(&NMThrInfoMutex);
			if(bRet)
			{
				char * repJsonStr = NULL;
				int jsonStrlen = 0;
				nm_thr_rep_info_t thrRepInfo;
				unsigned int repMsgLen = 0;
				thrRepInfo.isTotalNormal = isNormal;

				repMsgLen = strlen(repMsg)+1;
				thrRepInfo.repInfo = (char*)malloc(repMsgLen);
				memset(thrRepInfo.repInfo, 0, repMsgLen);
				strcpy(thrRepInfo.repInfo, repMsg);
				jsonStrlen = Parser_PackThrCheckRep(&thrRepInfo, &repJsonStr);
				if(jsonStrlen > 0 && repJsonStr != NULL)
				{
					g_sendcbf(&g_PluginInfo, netmon_thr_check_rep, repJsonStr, strlen(repJsonStr)+1, NULL, NULL);
				}
				if(repJsonStr)free(repJsonStr);
				if(thrRepInfo.repInfo) free(thrRepInfo.repInfo);
			}
		}

		{
			//Sleep(5000);
			int i = 0;
			for(i = 0; IsNMThrCheckThreadRunning && i<10; i++)
			{
				app_os_sleep(100);
			}
		}
	}
	if(repMsg) free(repMsg);

	return 0;
}

static void NetMonWhenDelThrCheckNormal(nm_thr_item_list thrItemList, char ** checkMsg, unsigned int bufLen)
{
	if(NULL == thrItemList ||NULL == (char*)(*checkMsg)) return;
	{
		nm_thr_item_list curThrItemList = thrItemList;
		nm_thr_item_node_t * curThrItemNode = curThrItemList->next;
		char tmpMsg[256] = {0};
		while(curThrItemNode)
		{
			memset(tmpMsg, 0, sizeof(tmpMsg));
			if(curThrItemNode->thrItem.isEnable && !curThrItemNode->thrItem.isNormal)
			{
				curThrItemNode->thrItem.isNormal = TRUE;
				sprintf(tmpMsg, "%s %s", curThrItemNode->thrItem.n,DEF_NOR_EVENT_STR);
			}

			if(strlen(tmpMsg))
			{				
				if(bufLen<strlen(*checkMsg)+strlen(tmpMsg)+16)
				{
					int newLen = strlen(*checkMsg)+strlen(tmpMsg)+2*1024;
					*checkMsg = (char*)realloc(*checkMsg, newLen);
				}
				if(strlen(*checkMsg))
					sprintf(*checkMsg, "%s;%s", *checkMsg, tmpMsg);
				else
					sprintf(*checkMsg, "%s", tmpMsg);
			}
			curThrItemNode = curThrItemNode->next;
		}
	}
}

void UpdateNMThrItemList()
{
	nm_thr_item_node_t * curThrItemNode = NMThrItemList->next;
	nm_thr_item_node_t * newNode = NULL, * findNode = NULL, *head = NULL;

	while(curThrItemNode)
	{
		char * buf = NULL;
		char n[DEF_N_LEN] = {0};
		int i = 0;
		char * sliceStr[16] = {NULL};
		strcpy(n, curThrItemNode->thrItem.n);
		buf = n;
		while(sliceStr[i] = strtok(buf, "/"))
		{
			i++;
			buf = NULL;
		}
		//"n":"NetMonitor/netMonInfoList/Index0-本地连接/netUsage"
		if( NULL != strstr(sliceStr[2], NETMON_GROUP_NETINDEX) && NULL != strstr(sliceStr[2], "-"))
		{
			char buf[128] = {0};
			strcpy(buf, strstr(sliceStr[2], "-")+1);
			strcpy(curThrItemNode->thrItem.adapterName, buf);
			strcpy(curThrItemNode->thrItem.bn, sliceStr[2]);
			strcpy(curThrItemNode->thrItem.tagName, sliceStr[3]);
		}

		curThrItemNode = curThrItemNode->next;
	}
}

static void NetMonitorIsThrItemListNormal(nm_thr_item_list curThrItemList, BOOL * isNormal)
{
	if(NULL == isNormal || curThrItemList == NULL) return;
	{
		nm_thr_item_node_t * curThrItemNode = curThrItemList->next;
		while(curThrItemNode)
		{
			if(curThrItemNode->thrItem.isEnable && !curThrItemNode->thrItem.isNormal)
			{
				*isNormal = FALSE;
				break;
			}
			curThrItemNode = curThrItemNode->next;
		}
	}
}


static void NetMonUpdateThr(nm_thr_item_list curThrList, nm_thr_item_list newThrList)
{
	BOOL bRet = FALSE;
	if(NULL == newThrList || NULL == curThrList) return bRet;
	{
		nm_thr_item_node_t * newThrItemNode = NULL, * findThrItemNode = NULL;
		nm_thr_item_node_t * curThrItemNode = curThrList->next;
		while(curThrItemNode) //first all thr node set invalid
		{
			curThrItemNode->thrItem.isValid = 0;
			curThrItemNode = curThrItemNode->next;
		}
		newThrItemNode = newThrList->next;
		while(newThrItemNode)  //merge old&new thr list
		{
			{
				if(strlen(newThrItemNode->thrItem.n) >0)
				{
					char * buf = NULL;
					char n[DEF_N_LEN] = {0};
					int i = 0;
					char * sliceStr[16] = {NULL};
					char adapterNameStr[128] = {0};
					strcpy(n, newThrItemNode->thrItem.n);
					buf = n;
					while(sliceStr[i] = strtok(buf, "/"))
					{
						i++;
						buf = NULL;
					}

					if( NULL != strstr(sliceStr[2], NETMON_GROUP_NETINDEX) && NULL != strstr(sliceStr[2], "-"))
						strcpy(adapterNameStr, strstr(sliceStr[2], "-")+1);
					else
						strcpy(adapterNameStr, sliceStr[2]);

					strcpy(newThrItemNode->thrItem.bn, sliceStr[2]);
					strcpy(newThrItemNode->thrItem.adapterName, adapterNameStr);
					strcpy(newThrItemNode->thrItem.tagName, sliceStr[3]);    
				}

			}
			findThrItemNode = FindNMThrItemNodeWithName(curThrList, newThrItemNode->thrItem.adapterName, newThrItemNode->thrItem.tagName);
			if(findThrItemNode) //exist then update thr argc
			{
				findThrItemNode->thrItem.isValid = 1;
				findThrItemNode->thrItem.intervalTimeS = newThrItemNode->thrItem.intervalTimeS;
				findThrItemNode->thrItem.lastingTimeS = newThrItemNode->thrItem.lastingTimeS;
				findThrItemNode->thrItem.isEnable = newThrItemNode->thrItem.isEnable;
				findThrItemNode->thrItem.maxThr = newThrItemNode->thrItem.maxThr;
				findThrItemNode->thrItem.minThr = newThrItemNode->thrItem.minThr;
				findThrItemNode->thrItem.thrType = newThrItemNode->thrItem.thrType;
			}
			else  //not exist then insert to old list
			{
				InsertNMThrItemNode(curThrList, &newThrItemNode->thrItem);
			}
			newThrItemNode = newThrItemNode->next;
		}
		{
			unsigned int defRepMsg = 2*1024;
			char *repMsg= (char*)malloc(defRepMsg);
			nm_thr_item_node_t * preNode = curThrList,* normalRepNode = NULL, *delNode = NULL;
			memset(repMsg, 0, defRepMsg);
			curThrItemNode = preNode->next;
			while(curThrItemNode) //check need delete&normal report node
			{
				normalRepNode = NULL;
				delNode = NULL;
				if(curThrItemNode->thrItem.isValid == 0)
				{
					preNode->next = curThrItemNode->next;
					delNode = curThrItemNode;
					if(curThrItemNode->thrItem.isNormal == FALSE)
					{
						normalRepNode = curThrItemNode;
					}
				}
				else
				{
					preNode = curThrItemNode;
				}
				if(normalRepNode == NULL && curThrItemNode->thrItem.isEnable == FALSE && curThrItemNode->thrItem.isNormal == FALSE)
				{
					normalRepNode = curThrItemNode;
				}
				if(normalRepNode)
				{
					char *tmpMsg = NULL;
					int len = strlen(curThrItemNode->thrItem.n)+strlen(DEF_NOR_EVENT_STR)+32;//if分配大小thrItem.adapterName，free时出错
					tmpMsg = (char*)malloc(len);
					memset(tmpMsg, 0, len);
					sprintf(tmpMsg, "%s %s", curThrItemNode->thrItem.n,DEF_NOR_EVENT_STR);
					if(tmpMsg && strlen(tmpMsg))
					{
						if(defRepMsg<strlen(tmpMsg)+strlen(repMsg)+1)
						{
							int newLen = strlen(tmpMsg) + strlen(repMsg) + 1024;
							repMsg = (char *)realloc(repMsg, newLen);
						}	
						if(strlen(repMsg))
						{
							sprintf(repMsg, "%s;%s", repMsg, tmpMsg);
						}
						else
						{
							sprintf(repMsg, "%s", tmpMsg);
						}
					}
					if(tmpMsg)free(tmpMsg);
					tmpMsg = NULL;
					normalRepNode->thrItem.isNormal = TRUE;
				}
				if(delNode)
				{
					if(delNode->thrItem.checkSrcValList.head)
					{
						check_value_node_t * frontValueNode = delNode->thrItem.checkSrcValList.head;
						check_value_node_t * delValueNode = frontValueNode->next;
						while(delValueNode)
						{
							frontValueNode->next = delValueNode->next;
							free(delValueNode);
							delValueNode = frontValueNode->next;
						}
						free(delNode->thrItem.checkSrcValList.head);
						delNode->thrItem.checkSrcValList.head = NULL;
					}
					//if(delNode->thrItem.name) free(delNode->thrItem.name);
					//if(delNode->thrItem.desc) free(delNode->thrItem.desc);
					free(delNode);
					delNode = NULL;
				}
				curThrItemNode = preNode->next;
			}
			if(strlen(repMsg))
			{
				char * repJsonStr = NULL;
				int jsonStrlen = 0;
				nm_thr_rep_info_t thrRepInfo;
				int repInfoLen = strlen(repMsg)+1;
				unsigned int repMsgLen = 0;

				repMsgLen = strlen(repMsg)+1;
				thrRepInfo.isTotalNormal = TRUE;
				//add
				NetMonitorIsThrItemListNormal(curThrList, &thrRepInfo.isTotalNormal);
				thrRepInfo.repInfo = (char*)malloc(repMsgLen);
				memset(thrRepInfo.repInfo, 0, repMsgLen);
				strcpy(thrRepInfo.repInfo, repMsg);
				jsonStrlen = Parser_PackThrCheckRep(&thrRepInfo, &repJsonStr);
				if(jsonStrlen > 0 && repJsonStr != NULL)
				{
					g_sendcbf(&g_PluginInfo, netmon_thr_check_rep, repJsonStr, strlen(repJsonStr)+1, NULL, NULL);
				}
				if(repJsonStr) free(repJsonStr);
				if(thrRepInfo.repInfo) free(thrRepInfo.repInfo);
			}
			if(repMsg) free(repMsg);
		}


#if true
		{
			netmon_handler_context_t * pNetMonHandlerContext = &NetMonHandlerContext;
			net_info_list curNetInfoList = pNetMonHandlerContext->netInfoList;
			app_os_mutex_lock(&NMThrInfoMutex);
			if(NMThrItemList != NULL && NMThrItemList->next != NULL && strlen(NMThrItemList->next->thrItem.adapterName) == 0)
			{
				UpdateNMThrItemList();//must need!
			}
			app_os_mutex_unlock(&NMThrInfoMutex);
		}
#endif
	}
}

#if 0
static void NetMonUpdateThr(nm_thr_item_list newNMThrList)
{
	char *repMsg;
	unsigned int bufLen = 1024*2;
	repMsg = (char*)malloc(bufLen);
	memset(repMsg, 0, bufLen);
	if(newNMThrList != NULL && newNMThrList->next != NULL)
	{
		app_os_mutex_lock(&NMThrInfoMutex);
		NetMonWhenDelThrCheckNormal(NMThrItemList, &repMsg, bufLen);
		//delete all old thr node
		DeleteAllNMThrItemNode(NMThrItemList);
		app_os_mutex_unlock(&NMThrInfoMutex);
		{
			nm_thr_item_node_t * curThrNode = newNMThrList->next;
			while(curThrNode)
			{
				InsertNMThrItemNode(NMThrItemList, &curThrNode->thrItem);//out in
				curThrNode = curThrNode->next;
			}
		}
		app_os_mutex_unlock(&NMThrInfoMutex);
		if(strlen(repMsg))
		{
			char * repJsonStr = NULL;
			int jsonStrlen = 0;
			nm_thr_rep_info_t thrRepInfo;
			int repInfoLen = strlen(repMsg)+1;
			unsigned int repMsgLen = 0;

			repMsgLen = strlen(repMsg)+1;
			thrRepInfo.isTotalNormal = TRUE;
			thrRepInfo.repInfo = (char*)malloc(repMsgLen);
			memset(thrRepInfo.repInfo, 0, repMsgLen);
			strcpy(thrRepInfo.repInfo, repMsg);
			jsonStrlen = Parser_PackThrCheckRep(&thrRepInfo, &repJsonStr);
			if(jsonStrlen > 0 && repJsonStr != NULL)
			{
				g_sendcbf(&g_PluginInfo, netmon_thr_check_rep, repJsonStr, strlen(repJsonStr)+1, NULL, NULL);
			}
			if(repJsonStr) free(repJsonStr);
			if(thrRepInfo.repInfo) free(thrRepInfo.repInfo);
		}
	}
	if(repMsg) free(repMsg);

#if true
	{
		netmon_handler_context_t * pNetMonHandlerContext = &NetMonHandlerContext;
		net_info_list curNetInfoList = pNetMonHandlerContext->netInfoList;
		app_os_mutex_lock(&NMThrInfoMutex);
		if(NMThrItemList != NULL && NMThrItemList->next != NULL && strlen(NMThrItemList->next->thrItem.adapterName) == 0)
		{
			UpdateNMThrItemList();
		}
		app_os_mutex_unlock(&NMThrInfoMutex);
	}
#endif
}
#endif

static CAGENT_PTHREAD_ENTRY(SetThrThreadStart, args)
{
	char repMsg[1024] = {0};
	BOOL bRet = FALSE;
	nm_thr_item_list tmpThrItemList = NULL;;
	tmpThrItemList = CreateNMThrItemList();
	bRet = InitNetMonThrFromConfig(tmpThrItemList, NMThrTmpCfgPath);
	if(!bRet)
	{
		sprintf(repMsg, "%s", "Threshold apply failed!");
	}
	else
	{
		NetMonUpdateThr(NMThrItemList, tmpThrItemList);//in
		//app_os_mutex_lock(&NMThrInfoMutex);
		//NetMonThrFillConfig(NMThrItemList, NMThrCfgPath);//in out
		//app_os_mutex_unlock(&NMThrInfoMutex);
		sprintf(repMsg,"Threshold apply OK!");
	}
	if(strlen(repMsg))
	{
		char * repJsonStr = NULL;
		int jsonStrlen = Parser_PackSetThrRep(repMsg, &repJsonStr);
		if(jsonStrlen > 0 && repJsonStr != NULL)
		{
			g_sendcbf(&g_PluginInfo, netmon_set_thr_rep, repJsonStr, strlen(repJsonStr)+1, NULL, NULL);
		}
		if(repJsonStr)free(repJsonStr);
	}
	if(tmpThrItemList) DestroyNMThrItemList(tmpThrItemList);
	remove(NMThrTmpCfgPath);
	app_os_CloseHandle(SetThrThreadHandle);
	SetThrThreadHandle = NULL;
	IsSetThrThreadRunning = FALSE;
	return 0;
}

static void NetMonSetThr(char *thrJsonStr)
{
	if(NULL == thrJsonStr) return;
	{
		char repMsg[256] = {0};
		if(!IsSetThrThreadRunning)
		{
			FILE * fPtr = fopen(NMThrTmpCfgPath, "wb");//open or new
			if(fPtr)
			{
				fwrite(thrJsonStr, 1, strlen(thrJsonStr)+1, fPtr);
				fclose(fPtr);
			}
			IsSetThrThreadRunning = TRUE;
			if (app_os_thread_create(&SetThrThreadHandle, SetThrThreadStart, NULL) != 0) 
			{
				IsSetThrThreadRunning = FALSE;
				sprintf(repMsg, "%s", "Set threshold thread start error!");
				remove(NMThrTmpCfgPath);
			}
		}
		else
		{
			sprintf(repMsg, "%s", "Set threshold thread running!");
		}
		if(strlen(repMsg))
		{
			char * repJsonStr = NULL;
			int jsonStrlen = Parser_PackSetThrRep(repMsg, &repJsonStr);
			if(jsonStrlen > 0 && repJsonStr != NULL)
			{
				g_sendcbf(&g_PluginInfo, netmon_set_thr_rep, repJsonStr, strlen(repJsonStr)+1, NULL, NULL);
			}
			if(repJsonStr)free(repJsonStr);
		}
	}
}

static void NetMonDelAllThr()
{
	nm_thr_item_list curThrItemList = NULL;
	char *tmpMsg;
	unsigned int bufLen = 1024*2;
	tmpMsg = (char*)malloc(bufLen);
	memset(tmpMsg, 0, bufLen);
	app_os_mutex_lock(&NMThrInfoMutex);
	curThrItemList = NMThrItemList;
	if(curThrItemList)
	{
		NetMonWhenDelThrCheckNormal(curThrItemList, &tmpMsg, bufLen);
		DeleteAllNMThrItemNode(curThrItemList);
	}

	//NetMonThrFillConfig(curThrItemList, NMThrCfgPath);
	app_os_mutex_unlock(&NMThrInfoMutex);

	if(strlen(tmpMsg))
	{
		char * repJsonStr = NULL;
		int jsonStrlen = 0;
		nm_thr_rep_info_t thrRepInfo;
		unsigned int tmpMsgLen = 0;

		tmpMsgLen = strlen(tmpMsg)+1;
		thrRepInfo.isTotalNormal = TRUE;
		thrRepInfo.repInfo = (char*)malloc(tmpMsgLen);
		memset(thrRepInfo.repInfo, 0, tmpMsgLen);
		strcpy(thrRepInfo.repInfo, tmpMsg);
		jsonStrlen = Parser_PackThrCheckRep(&thrRepInfo, &repJsonStr);
		if(jsonStrlen > 0 && repJsonStr != NULL)
		{
			g_sendcbf(&g_PluginInfo, netmon_thr_check_rep, repJsonStr, strlen(repJsonStr)+1, NULL, NULL);
		}
		if(repJsonStr) free(repJsonStr);
		if(thrRepInfo.repInfo) free(thrRepInfo.repInfo);
	}
	if(tmpMsg) free(tmpMsg);

	{
		char * repJsonStr = NULL;
		int jsonStrlen = 0;
		char delRepMsg[256] = {0};
		sprintf_s(delRepMsg, sizeof(delRepMsg), "%s", "Delete all threshold successfully!");
		jsonStrlen = Parser_PackDelAllThrRep(delRepMsg, &repJsonStr);
		if( repJsonStr != NULL)
		{
			g_sendcbf(&g_PluginInfo, netmon_del_thr_rep, repJsonStr, strlen(repJsonStr)+1, NULL, NULL);
		}
		if(repJsonStr)free(repJsonStr);
	}
} 
//----------------------------------------------------------------------------------------------



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

	// 3. Callback function -> Send JSON Data by this callback function
	g_sendcbf = g_PluginInfo.sendcbf = pluginfo->sendcbf;
	g_sendcustcbf = g_PluginInfo.sendcustcbf = pluginfo->sendcustcbf;
	g_subscribecustcbf = g_PluginInfo.subscribecustcbf = pluginfo->subscribecustcbf;
	g_sendreportcbf = g_PluginInfo.sendreportcbf = pluginfo->sendreportcbf;
	g_sendcapabilitycbf =g_PluginInfo.sendcapabilitycbf = pluginfo->sendcapabilitycbf;

	{
		char modulePath[MAX_PATH] = {0};     
		if(app_os_get_module_path(modulePath))
		{
			sprintf(NMThrCfgPath, "%s%s", modulePath, DEF_NM_THR_CONFIG_NAME);
			sprintf(NMThrTmpCfgPath, "%s%sTmp", modulePath, DEF_NM_THR_CONFIG_NAME);
		}
	}

	return handler_success;
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
	return;
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
	BOOL bRet = FALSE;
	if(IsHandlerStart)
	{
		bRet = TRUE;
	}
	else
	{
		memset(&NetMonHandlerContext, 0, sizeof(netmon_handler_context_t));
		NetMonHandlerContext.susiHandlerContext.cagentHandle = &g_PluginInfo;

		NetMonHandlerContext.netInfoList = CreateNetInfoList();
		if(NetMonHandlerContext.netInfoList == NULL) goto done1;
		if(app_os_mutex_setup(&NetMonHandlerContext.netInfoListMutex) != 0)goto done1;
		NetMonHandlerContext.isGetNetInfoThreadRunning = TRUE;
		if (app_os_thread_create(&NetMonHandlerContext.getNetInfoThreadHandle, GetNetInfoThreadStart, &NetMonHandlerContext) != 0)
		{
			NetMonHandlerContext.isGetNetInfoThreadRunning = FALSE;
			goto done1;
		}

		if(app_os_cond_setup(&NetMonHandlerContext.netMonUploadSyncObj30) != 0) goto done1;
		if(app_os_mutex_setup(&NetMonHandlerContext.netMonUploadSyncMutex30) != 0) goto done1;
		NetMonHandlerContext.isNetMonUploadThreadRunning30 = TRUE;
		if (app_os_thread_create(&NetMonHandlerContext.netMonUploadThreadHandle30, NetMonUploadThreadStart30, NULL) != 0)//SA3.0
		{
			NetMonHandlerContext.isNetMonUploadThreadRunning30 = FALSE;
			goto done1;
		}

		if(app_os_cond_setup(&NetMonHandlerContext.netAutoUploadSyncObj) != 0) goto done1;
		memset(&NMUploadParams, 0, sizeof(upload_params_t));
		//NMUploadParams.repType = rep_data_query;
		if(app_os_mutex_setup(&NetMonHandlerContext.NMAutoUploadParamsMutex) != 0)
		{
			goto done1;
		}
		NetMonHandlerContext.IsNMAutoUploadThreadRunning = true;
		if (app_os_thread_create(&NMAutoUploadThreadHandle, NMAutoReport_API_ThreadStart, NULL) != 0)//SA3.1 Handler_AutoReportStart
		{
			NetMonHandlerContext.IsNMAutoUploadThreadRunning = false;
			goto done1;
		}

		if(app_os_cond_setup(&NetMonHandlerContext.netAutoReportSyncObj) != 0) goto done1;
		memset(&NMReportParams, 0, sizeof(report_params_t));
		//NMReportParams.repType = rep_data_query;
		if(app_os_mutex_setup(&NetMonHandlerContext.NMAutoReportParamsMutex) != 0)
		{
			goto done1;
		}
		NetMonHandlerContext.IsNMAutoReportThreadRunning = true;
		if (app_os_thread_create(&NMAutoReportThreadHandle, NMAutoReport_Recv_ThreadStart, NULL) != 0)//SA3.1 Handler_Recv
		{
			NetMonHandlerContext.IsNMAutoReportThreadRunning = false;
			goto done1;
		}

		if(app_os_mutex_setup(&NMThrInfoMutex) != 0)
		{
			goto done1;
		}
		app_os_mutex_lock(&NMThrInfoMutex);
		NMThrItemList = CreateNMThrItemList();
		//if(app_os_is_file_exist(NMThrCfgPath))
		//{
		//	InitNetMonThrFromConfig(NMThrItemList, NMThrCfgPath); //out in
		//}
		app_os_mutex_unlock(&NMThrInfoMutex);
		IsNMThrCheckThreadRunning = true;
		if(app_os_thread_create(&NMThrCheckThreadHandle, NMThrCheckThreadStart, NULL) != 0)
		{
			IsNMThrCheckThreadRunning = false;
			goto done1;
		}

		bRet = TRUE;
		IsHandlerStart = true;
	}
done1:
	if(!bRet)
	{
		Handler_Stop();
		return handler_fail;
	}
	else
	{
		return handler_success;
	}
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
	if(IsHandlerStart)
	{
		if(IsNMThrCheckThreadRunning)
		{
			IsNMThrCheckThreadRunning = false;
			app_os_thread_join(NMThrCheckThreadHandle);
		}
		app_os_mutex_lock(&NMThrInfoMutex);
		//NetMonThrFillConfig(NMThrItemList, NMThrCfgPath);
		DestroyNMThrItemList(NMThrItemList);
		app_os_mutex_unlock(&NMThrInfoMutex);
		app_os_mutex_cleanup(&NMThrInfoMutex);

		if(NetMonHandlerContext.isGetNetInfoThreadRunning)
		{
			NetMonHandlerContext.isGetNetInfoThreadRunning = FALSE;
			app_os_thread_join(NetMonHandlerContext.getNetInfoThreadHandle);
		}
		if(NetMonHandlerContext.netInfoList)
		{
			DestroyNetInfoList(NetMonHandlerContext.netInfoList);
			NetMonHandlerContext.netInfoList = NULL;
		}
		app_os_mutex_cleanup(&NetMonHandlerContext.netInfoListMutex);

		if(NetMonHandlerContext.isNetMonUploadThreadRunning30)//SA3.0
		{
			NetMonHandlerContext.isNetMonUploadThreadRunning30 = FALSE;
			app_os_cond_signal(&NetMonHandlerContext.netMonUploadSyncObj30);
			app_os_thread_join(NetMonHandlerContext.netMonUploadThreadHandle30); 
		}
		app_os_cond_cleanup(&NetMonHandlerContext.netMonUploadSyncObj30);
		app_os_mutex_cleanup(&NetMonHandlerContext.netMonUploadSyncMutex30);

		if(NetMonHandlerContext.IsNMAutoUploadThreadRunning)//SA3.1  Handler_AutoReportStart
		{
			NetMonHandlerContext.IsNMAutoUploadThreadRunning = false;
			app_os_cond_signal(&NetMonHandlerContext.netAutoUploadSyncObj); 
			app_os_thread_join(NMAutoUploadThreadHandle); 
		}
		app_os_cond_cleanup(&NetMonHandlerContext.netAutoUploadSyncObj);
		app_os_mutex_cleanup(&NetMonHandlerContext.NMAutoUploadParamsMutex);
		app_os_DeleteCriticalSection(&NetMonHandlerContext.autoUploadRequestItemsListCS);

		if(NetMonHandlerContext.IsNMAutoReportThreadRunning)//SA3.1 Handler_Recv
		{
			NetMonHandlerContext.IsNMAutoReportThreadRunning = false; 
			app_os_cond_signal(&NetMonHandlerContext.netAutoReportSyncObj);
			app_os_thread_join(NMAutoReportThreadHandle);
		}
		app_os_cond_cleanup(&NetMonHandlerContext.netAutoReportSyncObj);
		app_os_mutex_cleanup(&NetMonHandlerContext.NMAutoReportParamsMutex);
		app_os_DeleteCriticalSection(&NetMonHandlerContext.autoReportRequestItemsListCS);

		//must add (mem leak)
		if(NetMonHandlerContext.autoUploadRequestItemsList) 
		{
			DestroySensorInfoList(NetMonHandlerContext.autoUploadRequestItemsList);
			NetMonHandlerContext.autoUploadRequestItemsList = NULL;
		}
		if(NetMonHandlerContext.autoReportRequestItemsList)
		{
			DestroySensorInfoList(NetMonHandlerContext.autoReportRequestItemsList);
			NetMonHandlerContext.autoReportRequestItemsList = NULL;
		}
	}
	IsHandlerStart = false;
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
	cagent_callback_status_t status = cagent_callback_continue;
	netmon_handler_context_t *pNetMonHandlerContext =  &NetMonHandlerContext;
	cagent_handle_t cagentHandle = pNetMonHandlerContext->susiHandlerContext.cagentHandle;
	int commCmd = unknown_cmd;
	susi_comm_data_t *pSusiCommData = NULL;
	char errorStr[128] = {0};
	if(!data || datalen <= 0) return;

	if(!ParseReceivedData(data, datalen, &commCmd))
		return;

	switch(commCmd)
	{
	case netmon_auto_report_req://SA3.1
		{
			unsigned int intervalMs = 0; 
			unsigned int timeoutMs = 0;
			bool bRet = FALSE;
			netmon_handler_context_t* pNetMonHandlerContext = (netmon_handler_context_t*)&NetMonHandlerContext;

			app_os_InitializeCriticalSection(&pNetMonHandlerContext->autoReportRequestItemsListCS);
			app_os_EnterCriticalSection(&pNetMonHandlerContext->autoReportRequestItemsListCS);
			if(pNetMonHandlerContext->autoReportRequestItemsList)
			{
				DestroySensorInfoList(pNetMonHandlerContext->autoReportRequestItemsList);
				pNetMonHandlerContext->autoReportRequestItemsList = NULL;
			}
			pNetMonHandlerContext->autoReportRequestItemsList = CreateSensorInfoList();
			pNetMonHandlerContext->isAutoReportAll = FALSE;
			bRet = Parser_ParseAutoReportCmd(data, pNetMonHandlerContext->autoReportRequestItemsList, &(pNetMonHandlerContext->isAutoReportAll), &intervalMs, &timeoutMs);
			app_os_LeaveCriticalSection(&pNetMonHandlerContext->autoReportRequestItemsListCS);

			if(bRet && intervalMs> 0 && timeoutMs > 0 && !(timeoutMs < intervalMs))
			{
				app_os_mutex_lock(&pNetMonHandlerContext->NMAutoReportParamsMutex);
				NMReportParams.intervalTimeMs = intervalMs; //ms
				NMReportParams.tmeoutMs = timeoutMs;
				if(!pNetMonHandlerContext->isNetMonAutoReport)
				{
					pNetMonHandlerContext->isNetMonAutoReport = TRUE;
					app_os_cond_signal(&pNetMonHandlerContext->netAutoReportSyncObj);
				}
				app_os_mutex_unlock(&pNetMonHandlerContext->NMAutoReportParamsMutex);
			}
			break;
		}

	case netmon_set_netinfo_auto_upload_req://SA3.0
		{  
			auto_upload_params_t Params;
			int dataLen = sizeof(susi_comm_data_t) + sizeof(auto_upload_params_t);
			pSusiCommData = (susi_comm_data_t *)malloc(dataLen);

			memset(pSusiCommData, 0, dataLen);
			if(Parse_netmon_set_netinfo_auto_upload_req((char*)data, &Params))
			{
				auto_upload_params_t *pAutoUploadParams = &Params;
				if(pAutoUploadParams->auto_upload_interval_ms > 0 && pAutoUploadParams->auto_upload_timeout_ms >0)
				{
					app_os_mutex_lock(&pNetMonHandlerContext->autoParamsMutex30);
					pNetMonHandlerContext->timeoutStart30 = 0;
					pNetMonHandlerContext->dataUploadIntervalMs30 = pAutoUploadParams->auto_upload_interval_ms;
					pNetMonHandlerContext->dataUploadTimeoutMs30 = pAutoUploadParams->auto_upload_timeout_ms;
					if(!pNetMonHandlerContext->isNetMonAutoUpload30)
					{
						pNetMonHandlerContext->isNetMonAutoUpload30 = TRUE; 
						app_os_cond_signal(&pNetMonHandlerContext->netMonUploadSyncObj30);
					}
					{
						char * pSendVal = NULL;
						char * str = "SUCCESS";
						int jsonStrlen = Pack_netmon_set_netinfo_auto_upload_rep(str, &pSendVal);
						if(jsonStrlen > 0 && pSendVal != NULL)
						{
							g_sendcbf(&g_PluginInfo, netmon_set_netinfo_auto_upload_rep, pSendVal, strlen(pSendVal)+1, NULL, NULL);
						}
						if(pSendVal)free(pSendVal);	
					}
					app_os_mutex_unlock(&pNetMonHandlerContext->autoParamsMutex30);
				}
				else
				{
					memset(errorStr, 0, sizeof(errorStr));
					sprintf(errorStr, "Command(%d) params error!", netmon_set_netinfo_auto_upload_req);
					{
						char * pSendVal = NULL;
						char * str = errorStr;
						int jsonStrlen = Pack_netmon_error_rep(str, &pSendVal);
						if(jsonStrlen > 0 && pSendVal != NULL)
						{
							g_sendcbf(&g_PluginInfo, netmon_error_rep, pSendVal, strlen(pSendVal)+1, NULL, NULL);
						}
						if(pSendVal)free(pSendVal);	
					}
				}
			}
			else
			{
				memset(errorStr, 0, sizeof(errorStr));
				sprintf(errorStr, "Command(%d) parse error!", netmon_set_netinfo_auto_upload_req);
				{
					char * pSendVal = NULL;
					char * str = errorStr;
					int jsonStrlen = Pack_netmon_error_rep(str, &pSendVal);
					if(jsonStrlen > 0 && pSendVal != NULL)
					{
						g_sendcbf(&g_PluginInfo, netmon_error_rep, pSendVal, strlen(pSendVal)+1, NULL, NULL);
					}
					if(pSendVal)free(pSendVal);	
				}
			}
			if(pSusiCommData) free(pSusiCommData);

			break;
		}

	case netmon_set_netinfo_reqp_req:
		{
			pNetMonHandlerContext->isNetMonAutoUpload30 = FALSE;
			{
				char * pSendVal = NULL;
				char * str = "SUCCESS";
				int jsonStrlen = Pack_netmon_set_netinfo_reqp_rep(str, &pSendVal);
				if(jsonStrlen > 0 && pSendVal != NULL)
				{
					g_sendcbf(&g_PluginInfo, netmon_set_netinfo_reqp_rep, pSendVal, strlen(pSendVal)+1, NULL, NULL);
				}
				if(pSendVal)free(pSendVal);	
			}
			break;
		}
	case netmon_netinfo_req:
		{
			app_os_cond_signal(&pNetMonHandlerContext->netMonUploadSyncObj30);
			break;
		}
	case netmon_get_capability_req: //SA3.1
		{
			GetCapability(netmon_send_cbf, netmon_get_capability_rep);
			break;
		}
	case netmon_get_sensors_data_req:
		{
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
	case netmon_set_thr_req:  //457 257
		{
			NetMonSetThr((char *)data);
			break;
		}
	case netmon_del_thr_req:
		{
			NetMonDelAllThr();
			break;
		}
	default:
		{
			{
				char * pSendVal = NULL;
				char * str = "Unknown cmd!";
				int jsonStrlen = Pack_netmon_error_rep(str, &pSendVal);
				if(jsonStrlen > 0 && pSendVal != NULL)
				{
					g_sendcbf(&g_PluginInfo, netmon_error_rep, pSendVal, strlen(pSendVal)+1, NULL, NULL);
				}
				if(pSendVal)free(pSendVal);	
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
*  Output: char * : pOutReply       // JSON Format
*  Return:  int  : Length of the status information in JSON format
*                :  0 : no data need to trans
* **************************************************************************************/
int HANDLER_API Handler_Get_Capability( char ** pOutReply )
{
	int len = 0; // Data length of the pOutReply 
	char * cpbStr = NULL;
	cpbStr = GetNMCapabilityString();
	if(cpbStr != NULL)
	{
		len = strlen(cpbStr);
		*pOutReply = (char *)malloc(len + 1);
		memset(*pOutReply, 0, len + 1);
		strcpy(*pOutReply, cpbStr);
		if(cpbStr)
			free(cpbStr);
	}
	return len;
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
	/*
	pInQuery = {"susiCommData":{"autoUploadIntervalSec":10,"requestItems":{"NetMonitor":{"e":[{"n":"NetMonitor"}]}},
	"commCmd":2053,"requestID":0,"agentID":"","handlerName":"general","sendTS":1444293454}}
	*/

	unsigned int intervalTimeS = 0; //sec
	bool bRet = FALSE;
	netmon_handler_context_t* pNetMonHandlerContext = (netmon_handler_context_t*)&NetMonHandlerContext;

	app_os_InitializeCriticalSection(&pNetMonHandlerContext->autoUploadRequestItemsListCS);
	app_os_EnterCriticalSection(&pNetMonHandlerContext->autoUploadRequestItemsListCS);
	if(pNetMonHandlerContext->autoUploadRequestItemsList)
	{
		DestroySensorInfoList(pNetMonHandlerContext->autoUploadRequestItemsList);
		pNetMonHandlerContext->autoUploadRequestItemsList = NULL;
	}
	pNetMonHandlerContext->autoUploadRequestItemsList = CreateSensorInfoList();
	pNetMonHandlerContext->isAutoUploadAll = FALSE;
	bRet = Parser_ParseAutoUploadCmd(pInQuery, pNetMonHandlerContext->autoUploadRequestItemsList, &(pNetMonHandlerContext->isAutoUploadAll), &intervalTimeS);
	app_os_LeaveCriticalSection(&pNetMonHandlerContext->autoUploadRequestItemsListCS);

	if(bRet)
	{
		app_os_mutex_lock(&pNetMonHandlerContext->NMAutoUploadParamsMutex); 
		NMUploadParams.intervalTimeMs = intervalTimeS*1000; //ms
		if(!pNetMonHandlerContext->isNetMonAutoUpload)
		{
			pNetMonHandlerContext->isNetMonAutoUpload = TRUE;
			app_os_cond_signal(&pNetMonHandlerContext->netAutoUploadSyncObj);
		}
		app_os_mutex_unlock(&pNetMonHandlerContext->NMAutoUploadParamsMutex);
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
	netmon_handler_context_t* pNetMonHandlerContext = (netmon_handler_context_t*)&NetMonHandlerContext;

	unsigned int intervalTimeS = 0; //sec
	bool bRet = Parser_ParseAutoUploadStopCmd(pInQuery);
	if(bRet)
	{
		app_os_mutex_lock(&pNetMonHandlerContext->NMAutoUploadParamsMutex);
		//ReportDataParams.intervalTimeMs = 0; //ms
		//NMUploadParams.repType = rep_data_query;
		if(pNetMonHandlerContext->isNetMonAutoUpload)
		{
			pNetMonHandlerContext->isNetMonAutoUpload = FALSE;
		}
		app_os_mutex_unlock(&pNetMonHandlerContext->NMAutoUploadParamsMutex);
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
