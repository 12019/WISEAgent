/****************************************************************************/
/* Copyright(C) : Advantech Technologies, Inc.								       */
/* Create Date  : 2014/1/28 by hailong.dang								          */
/* Modified Date: 2014/1/28 by hailong.dang								          */
/* Abstract     : SUSI Control Handler                                      */
/* Reference    : None														             */
/****************************************************************************/
#include "platform.h"
#include "SusiIoTAPI.h"
#include "susiaccess_handler_api.h"
#include "common.h"
#include "SUSIControlHandler.h"
#include "SUSIControlLog.h"
#include "Parser.h"

//-----------------------------------------------------------------------------
// Internal Prototypes:
//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
// Types and defines:
//-----------------------------------------------------------------------------
#define DEF_HANDLER_NAME         "SUSIControl"
const char strPluginName[MAX_TOPIC_LEN] = {"SUSIControl"};
const int iRequestID = cagent_request_susi_control;
const int iActionID = cagent_reply_susi_control;
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Variables
//-----------------------------------------------------------------------------
static Handler_info  g_PluginInfo;
static void* g_loghandle = NULL;
static bool g_bEnableLog = true;
static HandlerSendCbf  g_sendcbf = NULL;						// Client Send information (in JSON format) to Cloud Server	
static HandlerSendCustCbf  g_sendcustcbf = NULL;			    // Client Send information (in JSON format) to Cloud Server with custom topic	
static HandlerAutoReportCbf g_sendreportcbf = NULL;				// Client Send report (in JSON format) to Cloud Server with AutoReport topic	
static HandlerSendCapabilityCbf g_sendcapabilitycbf = NULL;	
static HandlerSubscribeCustCbf g_subscribecustcbf = NULL;
//-----------------------------------------------------------------------------

//------------------------------user variable define---------------------------
#define DEF_SUSICTRL_THR_CONFIG_NAME                "SUSICtrlThrInfoCfg"
static char SUSICtrlThrCfgPath[MAX_PATH] = {0};
static char SUSICtrlThrTmpCfgPath[MAX_PATH] = {0};

bool IsHandlerStart = false;
static CAGENT_MUTEX_TYPE CurIotDataMutex;
static bool IsGetIotDataThreadRunning = false;
static CAGENT_THREAD_HANDLE GetIotDataThreadHandler;
static char * CurIotDataJsonStr = NULL;
static CAGENT_MUTEX_TYPE IotPFIDataMutex;
static char * IotPFIDataJsonStr = NULL;

typedef enum{
	rep_data_unknown,
	rep_data_query,
	rep_data_auto,
}report_data_type_t;
typedef struct report_data_params_t{
	report_data_type_t repType;
	unsigned int intervalTimeMs;
	unsigned int continueTimeMs;
	char repFilter[4096];
}report_data_params_t;
static CAGENT_MUTEX_TYPE ReportParamsMutex;
static report_data_params_t ReportDataParams;
static bool IsReportIotDataThreadRunning = false;
static CAGENT_THREAD_HANDLE ReportIotDataThreadHandle;

static CAGENT_MUTEX_TYPE AutoUploadParamsMutex;
static report_data_params_t AutoUploadParams;
static bool IsAutoUploadThreadRunning = false;
static CAGENT_THREAD_HANDLE AutoUploadThreadHandle;

static susictrl_thr_item_list SUSICtrlThrItemList = NULL;
static CAGENT_MUTEX_TYPE ThrInfoMutex;
static bool IsThrCheckThreadRunning = false;
static CAGENT_THREAD_HANDLE ThrCheckThreadHandle;
static bool IsSetThrThreadRunning = false;
static CAGENT_THREAD_HANDLE SetThrThreadHandle;

//-----------------------------------------------------------------------------

//------------------------------user func define------------------------------------
//----------------------sensor info item list function define------------------
static sensor_info_list CreateSensorInfoList();
static void DestroySensorInfoList(sensor_info_list sensorInfoList);
static int InsertSensorInfoNode(sensor_info_list sensorInfoList, sensor_info_t * pSensorInfo);
static sensor_info_node_t * FindSensorInfoNodeWithID(sensor_info_list sensorInfoList, int id);
static int DeleteSensorInfoNodeWithID(sensor_info_list sensorInfoList, int id);
static int DeleteAllSensorInfoNode(sensor_info_list sensorInfoList);
static BOOL IsSensorInfoListEmpty(sensor_info_list sensorInfoList);
//-----------------------------------------------------------------------------
//---------------------------iot data list function define---------------------
static iot_data_list CreateIotDataList();
static void DestroyIotDataList(iot_data_list iotDataList);
static int InsertIotDataNode(iot_data_list iotDataList, iot_data_info_t * pIotDataInfo);
static iot_data_node_t * FindIotDataNodeWithID(iot_data_list iotDataList, unsigned int id);
static int DeleteIotDataNodeWithID(iot_data_list iotDataList, unsigned int id);
static int DeleteAllIotDataNode(iot_data_list iotDataList);
static BOOL IsIotDataListEmpty(iot_data_list iotDataList);
//-----------------------------------------------------------------------------

//--------------------------- thr item list function define--------------------
static susictrl_thr_item_list CreateSUSICtrlThrList();
static void DestroySUSICtrlThrList(susictrl_thr_item_list thrList);
static int InsertSUSICtrlThrList(susictrl_thr_item_list thrList, susictrl_thr_item_info_t * pThrItem);
static susictrl_thr_item_node_t * FindSUSICtrlThrNode(susictrl_thr_item_list thrList, unsigned int id);
static int DeleteSUSICtrlThrNode(susictrl_thr_item_list thrList, unsigned int id);
static int DeleteAllSUSICtrlThrNode(susictrl_thr_item_list thrList);
static BOOL IsSUSICtrlThrListEmpty(susictrl_thr_item_list thrList);
//-----------------------------------------------------------------------------
#ifdef WIN32
#define DEF_SUSIIOT_LIB_NAME    "SusiIoT.dll"
#else
#define DEF_SUSIIOT_LIB_NAME    "libSusiIoT.so"
#endif
typedef SusiIoTStatus_t (SUSI_IOT_API *PSusiIoTInitialize)();
typedef SusiIoTStatus_t (SUSI_IOT_API *PSusiIoTUninitialize)();
typedef SusiIoTStatus_t (SUSI_IOT_API *PSusiIoTSetPFData)(json_t *data);
typedef const char * (SUSI_IOT_API *PSusiIoTGetPFCapabilityString)();
typedef const char * (SUSI_IOT_API *PSusiIoTGetPFDataString)(SusiIoTId_t id);
typedef SusiIoTStatus_t (SUSI_IOT_API *PSusiIoTMemFree)(void *address);
void * hSUSIIOTDll = NULL;
PSusiIoTInitialize pSusiIoTInitialize = NULL;
PSusiIoTUninitialize pSusiIoTUninitialize = NULL;
PSusiIoTSetPFData pSusiIoTSetPFData = NULL;
PSusiIoTGetPFCapabilityString pSusiIoTGetPFCapabilityString = NULL;
PSusiIoTGetPFDataString  pSusiIoTGetPFDataString = NULL;
PSusiIoTMemFree pSusiIoTMemFree = NULL;
#ifdef WIN32
#define DEF_JANSSON_LIB_NAME    "jansson.dll"
#else
#define DEF_JANSSON_LIB_NAME    "libjansson.so"
#endif
typedef json_t * (*Pjson_loads)(const char *input, size_t flags, json_error_t *error);
void * hJanssonDll = NULL;
Pjson_loads pjson_loads = NULL;

static void GetSUSIIOTFunction(void * hSUSI4DLL);
static BOOL IsExistSUSIIOTLib();
static BOOL StartupSUSIIOTLib();
static BOOL CleanupSUSIIOTLib();
static void GetJanssonFunction(void * hJanssonDLL);
static BOOL IsExistJanssonLib();
static BOOL StartupJanssonLib();
static BOOL CleanupJanssonLib();
static void GetCapability();
static void GetSensorsDataEx(sensor_info_list sensorInfoList, char * pSessionID);
static void GetSensorsData(char * sensorsID, char * pSessionID);
static void SetSensorsData(sensor_info_list sensorInfoList, char * pCurSessionID);
static CAGENT_PTHREAD_ENTRY(GetIotDataThreadStart, args);
static CAGENT_PTHREAD_ENTRY(ReportIotDataThreadStart, args);
static BOOL InitSUSICtrlThrFromConfig(susictrl_thr_item_list thrItemList, char * cfgPath);
static void SUSICtrlThrFillConfig(susictrl_thr_item_list thrItemList, char * cfgPath);
static BOOL GetIotDataInfoWithID(int id, iot_data_info_t * pIotDataInfo);
static BOOL SUSICtrlCheckSrcVal(susictrl_thr_item_info_t * pThrItemInfo, float checkValue);
static BOOL SUSICtrlCheckThrItem(susictrl_thr_item_info_t * pThrItemInfo, float checkValue, char * checkRetMsg);
static BOOL SUSICtrlCheckThr(susictrl_thr_item_list curThrItemList, char ** checkRetMsg, unsigned int bufLen, BOOL * isNormal);
static void SUSICtrlIsThrItemListNormal(susictrl_thr_item_list curThrItemList, BOOL * isNormal);
static CAGENT_PTHREAD_ENTRY(ThrCheckThreadStart, args);
static CAGENT_PTHREAD_ENTRY(SetThrThreadStart, args);
static void SUSICtrlWhenDelThrCheckNormal(susictrl_thr_item_list thrItemList, char **checkMsg, unsigned int bufLen);
static void SUSICtrlSetThr(char *thrJsonStr);
static void SUSICtrlDelAllThr();
//-----------------------------------------------------------------------------

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

static void DestroySensorInfoList(sensor_info_list sensorInfoList)
{
	if(NULL == sensorInfoList) return;
	DeleteAllSensorInfoNode(sensorInfoList);
	free(sensorInfoList); 
	sensorInfoList = NULL;
}

static int InsertSensorInfoNode(sensor_info_list sensorInfoList, sensor_info_t * pSensorInfo)
{
	int iRet = -1;
	sensor_info_node_t * newNode = NULL, * findNode = NULL, *head = NULL;
	if(pSensorInfo == NULL || sensorInfoList == NULL) return iRet;
	head = sensorInfoList;
	findNode = FindSensorInfoNodeWithID(head, pSensorInfo->id);
	if(findNode == NULL)
	{
		int i = 0;
		newNode = (sensor_info_node_t *)malloc(sizeof(sensor_info_node_t));
		memset(newNode, 0, sizeof(sensor_info_node_t));
		memcpy((char *)&newNode->sensorInfo, (char *)pSensorInfo, sizeof(sensor_info_t));
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

static sensor_info_node_t * FindSensorInfoNodeWithID(sensor_info_list sensorInfoList, int id)
{
	sensor_info_node_t * findNode = NULL, *head = NULL;
	if(sensorInfoList == NULL) return findNode;
	head = sensorInfoList;
	findNode = head->next;
	while(findNode)
	{
		if(findNode->sensorInfo.id == id) break;
		else
		{
			findNode = findNode->next;
		}
	}

	return findNode;
}

static int DeleteSensorInfoNodeWithID(sensor_info_list sensorInfoList, int id)
{
	int iRet = -1;
	sensor_info_node_t * delNode = NULL, *head = NULL;
	sensor_info_node_t * p = NULL;
	if(sensorInfoList == NULL) return iRet;
	head = sensorInfoList;
	p = head;
	delNode = head->next;
	while(delNode)
	{
		if(delNode->sensorInfo.id == id)
		{
			int i = 0;
			p->next = delNode->next;
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

static iot_data_list CreateIotDataList()
{
	iot_data_node_t * head = NULL;
	head = (iot_data_node_t *)malloc(sizeof(iot_data_node_t));
	if(head)
	{
		memset(head, 0, sizeof(iot_data_node_t));
		head->next = NULL;
	}
	return head;
}
static void DestroyIotDataList(iot_data_list iotDataList)
{
	if(NULL == iotDataList) return;
	DeleteAllIotDataNode(iotDataList);
	free(iotDataList); 
	iotDataList = NULL;
}

static int InsertIotDataNode(iot_data_list iotDataList, iot_data_info_t * pIotDataInfo)
{
	int iRet = -1;
	iot_data_node_t * newNode = NULL, * findNode = NULL, *head = NULL;
	if(pIotDataInfo == NULL || iotDataList == NULL) return iRet;
	head = iotDataList;
	findNode = FindIotDataNodeWithID(head, pIotDataInfo->id);
	if(findNode == NULL)
	{
		int i = 0;
		newNode = (iot_data_node_t *)malloc(sizeof(iot_data_node_t));
		memset(newNode, 0, sizeof(iot_data_node_t));
		memcpy((char *)&newNode->dataInfo, (char *)pIotDataInfo, sizeof(iot_data_info_t));
		{
			int len = strlen(pIotDataInfo->name) + 1;
			newNode->dataInfo.name = (char *)malloc(len);
			memset(newNode->dataInfo.name, 0, len);
			strcpy(newNode->dataInfo.name, pIotDataInfo->name);
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

static iot_data_node_t * FindIotDataNodeWithID(iot_data_list iotDataList, unsigned int id)
{
	iot_data_node_t * findNode = NULL, *head = NULL;
	if(iotDataList == NULL) return findNode;
	head = iotDataList;
	findNode = head->next;
	while(findNode)
	{
		if(findNode->dataInfo.id == id) break;
		else
		{
			findNode = findNode->next;
		}
	}

	return findNode;
}

static int DeleteIotDataNodeWithID(iot_data_list iotDataList, unsigned int id)
{
	int iRet = -1;
	iot_data_node_t * delNode = NULL, *head = NULL;
	iot_data_node_t * p = NULL;
	if(iotDataList == NULL) return iRet;
	head = iotDataList;
	p = head;
	delNode = head->next;
	while(delNode)
	{
		if(delNode->dataInfo.id == id)
		{
			int i = 0;
			p->next = delNode->next;
			if(delNode->dataInfo.name != NULL)
			{
				free(delNode->dataInfo.name);
				delNode->dataInfo.name = NULL;
			}
			if(delNode->dataInfo.val.vType == VT_S && delNode->dataInfo.val.uVal.vs != NULL)
			{
				free(delNode->dataInfo.val.uVal.vs);
				delNode->dataInfo.val.uVal.vs = NULL;
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

static int DeleteAllIotDataNode(iot_data_list iotDataList)
{
	int iRet = -1;
	iot_data_node_t * delNode = NULL, *head = NULL;
	int i = 0;
	if(iotDataList == NULL) return iRet;
	head = iotDataList;
	delNode = head->next;
	while(delNode)
	{
		head->next = delNode->next;
		if(delNode->dataInfo.name != NULL)
		{
			free(delNode->dataInfo.name);
			delNode->dataInfo.name = NULL;
		}
		if(delNode->dataInfo.val.vType == VT_S && delNode->dataInfo.val.uVal.vs != NULL)
		{
			free(delNode->dataInfo.val.uVal.vs);
			delNode->dataInfo.val.uVal.vs = NULL;
		}
		free(delNode);
		delNode = head->next;
	}

	iRet = 0;
	return iRet;
}

static BOOL IsIotDataListEmpty(iot_data_list iotDataList)
{
	BOOL bRet = TRUE;
	iot_data_node_t * curNode = NULL, *head = NULL;
	if(iotDataList == NULL) return bRet;
	head = iotDataList;
	curNode = head->next;
	if(curNode != NULL) bRet = FALSE;
	return bRet;
}

static susictrl_thr_item_list CreateSUSICtrlThrList()
{
	susictrl_thr_item_node_t * head = NULL;
	head = (susictrl_thr_item_node_t *)malloc(sizeof(susictrl_thr_item_node_t));
	if(head)
	{
		head->next = NULL;
		head->thrItemInfo.id = 0;
		head->thrItemInfo.name = NULL;
		head->thrItemInfo.desc = NULL;
		head->thrItemInfo.isEnable = FALSE;
		head->thrItemInfo.maxThr = DEF_INVALID_VALUE;
		head->thrItemInfo.minThr = DEF_INVALID_VALUE;
		head->thrItemInfo.thrType = DEF_THR_UNKNOW_TYPE;
		head->thrItemInfo.lastingTimeS = DEF_INVALID_TIME;
		head->thrItemInfo.intervalTimeS = DEF_INVALID_TIME;
		head->thrItemInfo.checkRetValue = DEF_INVALID_VALUE;
		head->thrItemInfo.checkSrcValList.head = NULL;
		head->thrItemInfo.checkSrcValList.nodeCnt = 0;
		head->thrItemInfo.checkType = ck_type_unknow;
		head->thrItemInfo.repThrTime = DEF_INVALID_VALUE;
		head->thrItemInfo.isNormal = TRUE;
		head->thrItemInfo.isInvalid = 0;
	}
	return head;
}

static void DestroySUSICtrlThrList(susictrl_thr_item_list thrList)
{
	if(NULL == thrList) return;
	DeleteAllSUSICtrlThrNode(thrList);
	free(thrList); 
	thrList = NULL;
}

static int InsertSUSICtrlThrList(susictrl_thr_item_list thrList, susictrl_thr_item_info_t * pThrItem)
{
	int iRet = -1;
	susictrl_thr_item_node_t * newNode = NULL, * findNode = NULL, *head = NULL;
	if(pThrItem == NULL || thrList == NULL) return iRet;
	head = thrList;
	findNode = FindSUSICtrlThrNode(head, pThrItem->id);
	if(findNode == NULL)
	{
		newNode = (susictrl_thr_item_node_t *)malloc(sizeof(susictrl_thr_item_node_t));
		memset(newNode, 0, sizeof(susictrl_thr_item_node_t));

		newNode->thrItemInfo.id = pThrItem->id;
		newNode->thrItemInfo.name = NULL;
		if(pThrItem->name && strlen(pThrItem->name))
		{
			int len = strlen(pThrItem->name)+1;
			newNode->thrItemInfo.name = (char *)malloc(len);
			memset(newNode->thrItemInfo.name, 0, len);
			strcpy(newNode->thrItemInfo.name, pThrItem->name);
		}
		if(pThrItem->desc && strlen(pThrItem->desc))
		{
			int len = strlen(pThrItem->desc)+1;
			newNode->thrItemInfo.desc = (char *)malloc(len);
			memset(newNode->thrItemInfo.desc, 0, len);
			strcpy(newNode->thrItemInfo.desc, pThrItem->desc);
		}
		newNode->thrItemInfo.isEnable = pThrItem->isEnable;
		newNode->thrItemInfo.maxThr = pThrItem->maxThr;
		newNode->thrItemInfo.minThr = pThrItem->minThr;
		newNode->thrItemInfo.thrType = pThrItem->thrType;
		newNode->thrItemInfo.lastingTimeS = pThrItem->lastingTimeS;
		newNode->thrItemInfo.intervalTimeS = pThrItem->intervalTimeS;
		newNode->thrItemInfo.checkType = pThrItem->checkType;
		newNode->thrItemInfo.checkRetValue = DEF_INVALID_VALUE;
		newNode->thrItemInfo.checkSrcValList.head = NULL;
		newNode->thrItemInfo.checkSrcValList.nodeCnt = 0;
		newNode->thrItemInfo.repThrTime = 0;
		newNode->thrItemInfo.isNormal = pThrItem->isNormal;
		newNode->thrItemInfo.isInvalid = pThrItem->isInvalid;
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

static susictrl_thr_item_node_t * FindSUSICtrlThrNode(susictrl_thr_item_list thrList, unsigned int id)
{
	susictrl_thr_item_node_t * findNode = NULL, *head = NULL;
	if(thrList == NULL) return findNode;
	head = thrList;
	findNode = head->next;
	while(findNode)
	{
		if(findNode->thrItemInfo.id == id) break;
		else
		{
			findNode = findNode->next;
		}
	}

	return findNode;
}

static int DeleteSUSICtrlThrNode(susictrl_thr_item_list thrList, unsigned int id)
{
	int iRet = -1;
	susictrl_thr_item_node_t * delNode = NULL, *head = NULL;
	susictrl_thr_item_node_t * p = NULL;
	if(thrList == NULL) return iRet;
	head = thrList;
	p = head;
	delNode = head->next;
	while(delNode)
	{
		if(delNode->thrItemInfo.id == id)
		{
			p->next = delNode->next;
			if(delNode->thrItemInfo.checkSrcValList.head)
			{
				check_value_node_t * frontValueNode = delNode->thrItemInfo.checkSrcValList.head;
				check_value_node_t * delValueNode = frontValueNode->next;
				while(delValueNode)
				{
					frontValueNode->next = delValueNode->next;
					free(delValueNode);
					delValueNode = frontValueNode->next;
				}
				free(delNode->thrItemInfo.checkSrcValList.head);
				delNode->thrItemInfo.checkSrcValList.head = NULL;
			}
			if(delNode->thrItemInfo.name) free(delNode->thrItemInfo.name);
			if(delNode->thrItemInfo.desc) free(delNode->thrItemInfo.desc);
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

static int DeleteAllSUSICtrlThrNode(susictrl_thr_item_list thrList)
{
	int iRet = -1;
	susictrl_thr_item_node_t * delNode = NULL, *head = NULL;
	if(thrList == NULL) return iRet;
	head = thrList;

	delNode = head->next;
	while(delNode)
	{
		head->next = delNode->next;
		if(delNode->thrItemInfo.checkSrcValList.head)
		{
			check_value_node_t * frontValueNode = delNode->thrItemInfo.checkSrcValList.head;
			check_value_node_t * delValueNode = frontValueNode->next;
			while(delValueNode)
			{
				frontValueNode->next = delValueNode->next;
				free(delValueNode);
				delValueNode = frontValueNode->next;
			}
			free(delNode->thrItemInfo.checkSrcValList.head);
			delNode->thrItemInfo.checkSrcValList.head = NULL;
		}
		if(delNode->thrItemInfo.name) free(delNode->thrItemInfo.name);
		if(delNode->thrItemInfo.desc) free(delNode->thrItemInfo.desc);
		free(delNode);
		delNode = head->next;
	}

	iRet = 0;
	return iRet;
}

static BOOL IsSUSICtrlThrListEmpty(susictrl_thr_item_list thrList)
{
	BOOL bRet = TRUE;
	susictrl_thr_item_node_t * curNode = NULL, *head = NULL;
	if(thrList == NULL) return bRet;
	head = thrList;
	curNode = head->next;
	if(curNode != NULL) bRet = FALSE;
	return bRet;
}

static void GetSUSIIOTFunction(void * hSUSIIOTDLL)
{
	if(hSUSIIOTDLL!=NULL)
	{
		pSusiIoTInitialize = (PSusiIoTInitialize)app_get_proc_address(hSUSIIOTDLL, "SusiIoTInitialize");
		pSusiIoTUninitialize = (PSusiIoTUninitialize)app_get_proc_address(hSUSIIOTDLL, "SusiIoTUninitialize");
		pSusiIoTSetPFData = (PSusiIoTSetPFData)app_get_proc_address(hSUSIIOTDLL, "SusiIoTSetPFData");
		pSusiIoTGetPFCapabilityString = (PSusiIoTGetPFCapabilityString)app_get_proc_address(hSUSIIOTDLL, "SusiIoTGetPFCapabilityString");
		pSusiIoTGetPFDataString = (PSusiIoTGetPFDataString)app_get_proc_address(hSUSIIOTDLL, "SusiIoTGetPFDataString");
		pSusiIoTMemFree = (PSusiIoTMemFree)app_get_proc_address(hSUSIIOTDLL, "SusiIoTMemFree");
	}
}

static BOOL IsExistSUSIIOTLib()
{
	BOOL bRet = FALSE;
	void * hSUSIIOT = NULL;
	hSUSIIOT = app_load_library(DEF_SUSIIOT_LIB_NAME);
	if(hSUSIIOT != NULL)
	{
		bRet = TRUE;
		app_free_library(hSUSIIOT);
		hSUSIIOT = NULL;
	}
	return bRet;
}

static BOOL StartupSUSIIOTLib()
{
	BOOL bRet = FALSE;
	hSUSIIOTDll = app_load_library(DEF_SUSIIOT_LIB_NAME);
	if(hSUSIIOTDll != NULL)
	{
		GetSUSIIOTFunction(hSUSIIOTDll);
		if(pSusiIoTInitialize)
		{
			SusiIoTStatus_t ret = pSusiIoTInitialize();
			if(ret == SUSIIOT_STATUS_SUCCESS || ret == SUSIIOT_STATUS_INITIALIZED)
			{
				bRet = TRUE;
			}
		}
	}
	return bRet;
}

static BOOL CleanupSUSIIOTLib()
{
	BOOL bRet = FALSE;
	if(pSusiIoTUninitialize)
	{
		SusiIoTStatus_t ret = pSusiIoTUninitialize();
		if(ret == SUSIIOT_STATUS_SUCCESS)
		{
			bRet = TRUE;
		}
	}
	if(hSUSIIOTDll != NULL)
	{
		app_free_library(hSUSIIOTDll);
		hSUSIIOTDll = NULL;
		pSusiIoTInitialize = NULL;
		pSusiIoTUninitialize = NULL;
		pSusiIoTSetPFData = NULL;
		pSusiIoTGetPFCapabilityString = NULL;
		pSusiIoTGetPFDataString = NULL;
		pSusiIoTMemFree = NULL;
	}
	return bRet;
}

static void GetJanssonFunction(void * hJanssonDLL)
{
	if(hJanssonDLL!=NULL)
	{
		pjson_loads = (Pjson_loads)app_get_proc_address(hJanssonDLL, "json_loads");
	}
}

static BOOL IsExistJanssonLib()
{
	BOOL bRet = FALSE;
	void * hJansson = NULL;
	hJansson = app_load_library(DEF_JANSSON_LIB_NAME);
	if(hJansson != NULL)
	{
		bRet = TRUE;
		app_free_library(hJansson);
		hJansson = NULL;
	}
	return bRet;
}

static BOOL StartupJanssonLib()
{
	BOOL bRet = FALSE;
	hJanssonDll = app_load_library(DEF_JANSSON_LIB_NAME);
	if(hJanssonDll != NULL)
	{
		GetJanssonFunction(hJanssonDll);
		bRet = TRUE;
	}
	return bRet;
}

static BOOL CleanupJanssonLib()
{
	BOOL bRet = FALSE;
	if(hJanssonDll != NULL)
	{
		app_free_library(hJanssonDll);
		hJanssonDll = NULL;
		pjson_loads = NULL;
	}
	bRet = TRUE;
	return bRet;
}

static void GetCapability()
{
	if(pSusiIoTGetPFCapabilityString != NULL)
	{
		char * cpbStr = NULL;
		cpbStr = (char *)pSusiIoTGetPFCapabilityString();
		if(cpbStr != NULL)
		{
			char * repJsonStr = NULL;
			int jsonStrlen = Parser_PackCapabilityStrRep(cpbStr, &repJsonStr);
			if(jsonStrlen > 0 && repJsonStr != NULL)
			{
				g_sendcbf(&g_PluginInfo, susictrl_get_capability_rep, repJsonStr, strlen(repJsonStr)+1, NULL, NULL);
			}
			if(repJsonStr)free(repJsonStr);
			pSusiIoTMemFree(cpbStr);
		}
		else
		{
			char * errorRepJsonStr = NULL;
			char errorStr[128];
			sprintf(errorStr, "Command(%d), Get capability error!", susictrl_get_capability_req);
			int jsonStrlen = Parser_PackSUSICtrlError(errorStr, &errorRepJsonStr);
			if(jsonStrlen > 0 && errorRepJsonStr != NULL)
			{
				g_sendcbf(&g_PluginInfo, susictrl_error_rep, errorRepJsonStr, strlen(errorRepJsonStr)+1, NULL, NULL);
			}
			if(errorRepJsonStr)free(errorRepJsonStr);
		}
	}
	else
	{
		char * errorRepJsonStr = NULL;
		char errorStr[128];
		sprintf(errorStr, "Command(%d), Get capability error!", susictrl_get_capability_req);
		int jsonStrlen = Parser_PackSUSICtrlError(errorStr, &errorRepJsonStr);
		if(jsonStrlen > 0 && errorRepJsonStr != NULL)
		{
			g_sendcbf(&g_PluginInfo, susictrl_error_rep, errorRepJsonStr, strlen(errorRepJsonStr)+1, NULL, NULL);
		}
		if(errorRepJsonStr)free(errorRepJsonStr);
	}
}

static void GetSensorsDataEx(sensor_info_list sensorInfoList, char * pSessionID)
{
	if(NULL == sensorInfoList || NULL == pSessionID) return;
	{
		if(!IsSensorInfoListEmpty(sensorInfoList) && pSusiIoTGetPFDataString!=NULL)
		{
			sensor_info_node_t * curNode = NULL;
			curNode = sensorInfoList->next;
			while(curNode)
			{
				app_os_mutex_lock(&CurIotDataMutex);
				app_os_mutex_lock(&IotPFIDataMutex);
				Parser_GetSensorJsonStr(CurIotDataJsonStr, IotPFIDataJsonStr, &curNode->sensorInfo);
				app_os_mutex_unlock(&IotPFIDataMutex);
				app_os_mutex_unlock(&CurIotDataMutex);
				curNode = curNode->next;
			}
			{
				char * repJsonStr = NULL;
				int jsonStrlen = Parser_PackGetSensorDataRep(sensorInfoList, pSessionID, &repJsonStr);
				if(jsonStrlen > 0 && repJsonStr != NULL)
				{
					g_sendcbf(&g_PluginInfo, susictrl_get_sensors_data_rep, repJsonStr, strlen(repJsonStr)+1, NULL, NULL);
				}
				if(repJsonStr)free(repJsonStr);
			}
		}
		else
		{
			char * errorRepJsonStr = NULL;
			char errorStr[128];
			sprintf(errorStr, "Command(%d), Get sensors data empty!", susictrl_get_sensors_data_req);
			int jsonStrlen = Parser_PackGetSensorDataError(errorStr, pSessionID, &errorRepJsonStr);
			if(jsonStrlen > 0 && errorRepJsonStr != NULL)
			{
				g_sendcbf(&g_PluginInfo, susictrl_get_sensors_data_error_rep, errorRepJsonStr, strlen(errorRepJsonStr)+1, NULL, NULL);
			}
			if(errorRepJsonStr)free(errorRepJsonStr);
		}
	}
}

static void GetSensorsData(char * sensorsID, char * pSessionID)
{
	if(NULL == sensorsID || NULL == pSessionID) return;
	{
		int sensorID = 0;
		sensor_info_t sensorInfo;
		char * tmpStr = sensorsID;
		char * tmpIdStr = NULL;
		sensor_info_list sensorInfoList = CreateSensorInfoList();
		while((tmpIdStr = strtok(tmpStr, ","))!=NULL)
		{
			sensorID = atoi(tmpIdStr);
			memset((char*)&sensorInfo, 0, sizeof(sensor_info_t));
			sensorInfo.id = sensorID;
			InsertSensorInfoNode(sensorInfoList, &sensorInfo);
			tmpStr = NULL;
		}
		if(!IsSensorInfoListEmpty(sensorInfoList) && pSusiIoTGetPFDataString!=NULL)
		{
			sensor_info_node_t * curNode = NULL;
			curNode = sensorInfoList->next;
			while(curNode)
			{
				char * sensorDataStr = NULL;
				sensorDataStr = (char *)pSusiIoTGetPFDataString(curNode->sensorInfo.id);
				if(sensorDataStr)
				{
					int len = strlen(sensorDataStr) + 1;
					curNode->sensorInfo.jsonStr = (char*)malloc(len);
					memset(curNode->sensorInfo.jsonStr, 0, len);
					strcpy(curNode->sensorInfo.jsonStr, sensorDataStr);
					pSusiIoTMemFree(sensorDataStr);
				}
				curNode = curNode->next;
			}
			{
				char * repJsonStr = NULL;
				int jsonStrlen = Parser_PackGetSensorDataRep(sensorInfoList, pSessionID, &repJsonStr);
				if(jsonStrlen > 0 && repJsonStr != NULL)
				{
					g_sendcbf(&g_PluginInfo, susictrl_get_sensors_data_rep, repJsonStr, strlen(repJsonStr)+1, NULL, NULL);
				}
				if(repJsonStr)free(repJsonStr);
			}
		}
		else
		{
			char * errorRepJsonStr = NULL;
			char errorStr[128];
			sprintf(errorStr, "Command(%d), Get sensors data empty!", susictrl_get_sensors_data_req);
			int jsonStrlen = Parser_PackGetSensorDataError(errorStr, pSessionID, &errorRepJsonStr);
			if(jsonStrlen > 0 && errorRepJsonStr != NULL)
			{
				g_sendcbf(&g_PluginInfo, susictrl_get_sensors_data_error_rep, errorRepJsonStr, strlen(errorRepJsonStr)+1, NULL, NULL);
			}
			if(errorRepJsonStr)free(errorRepJsonStr);
		}
		DestroySensorInfoList(sensorInfoList);
	}
}

static void SetSensorsDataEx(sensor_info_list sensorInfoList, char * pCurSessionID)
{
	if(sensorInfoList == NULL || pCurSessionID == NULL) return;
	{
		sensor_info_node_t * curNode = sensorInfoList->next;
		if(pSusiIoTSetPFData != NULL && pjson_loads != NULL)
		{
			while(curNode)
			{
				app_os_mutex_lock(&CurIotDataMutex);
				app_os_mutex_lock(&IotPFIDataMutex);
				Parser_GetSensorSetJsonStr(CurIotDataJsonStr, IotPFIDataJsonStr, &curNode->sensorInfo);
				app_os_mutex_unlock(&IotPFIDataMutex);
				app_os_mutex_unlock(&CurIotDataMutex);
				if(curNode->sensorInfo.setRet != SSR_NOT_FOUND && curNode->sensorInfo.setRet != SSR_FAIL &&
					curNode->sensorInfo.setRet != SSR_READ_ONLY && curNode->sensorInfo.setRet != SSR_WRONG_FORMAT)
				{
					json_error_t error;
					SusiIoTStatus_t status;
					json_t * jData = NULL;
					jData = pjson_loads(curNode->sensorInfo.jsonStr, 0, &error);
					if(jData)
					{
						SUSICtrlLog(g_loghandle, Normal, " curNode->sensorInfo.jsonStr: %s", curNode->sensorInfo.jsonStr);
						status = pSusiIoTSetPFData(jData);
						if(status != SUSIIOT_STATUS_SUCCESS)
						{
							curNode->sensorInfo.setRet = SSR_FAIL;
						}
						else
						{
							curNode->sensorInfo.setRet = SSR_SUCCESS;
						}
					}
					else
					{
						curNode->sensorInfo.setRet = SSR_WRONG_FORMAT;
					}
				}
				curNode = curNode->next;
			}
		}
		{
			char * repJsonStr = NULL;
			int jsonStrlen = Parser_PackSetSensorDataRepEx(sensorInfoList, pCurSessionID, &repJsonStr);
			if(jsonStrlen > 0 && repJsonStr != NULL)
			{
				g_sendcbf(&g_PluginInfo, susictrl_set_sensors_data_rep, repJsonStr, strlen(repJsonStr)+1, NULL, NULL);
			}
			if(repJsonStr)free(repJsonStr);
		}
	}
}

static void SetSensorsData(sensor_info_list sensorInfoList, char * pCurSessionID)
{
	if(sensorInfoList == NULL || pCurSessionID == NULL) return;
	{
		char setRetStr[1024] = {0};
		sensor_info_node_t * curNode = sensorInfoList->next;
		if(pSusiIoTSetPFData != NULL && pjson_loads != NULL)
		{
			while(curNode)
			{
				json_error_t error;
				SusiIoTStatus_t status;
				json_t * jData = NULL;
				jData = pjson_loads(curNode->sensorInfo.jsonStr, 0, &error);
				if(jData)
				{
					SUSICtrlLog(g_loghandle, Normal, " curNode->sensorInfo.jsonStr: %s", curNode->sensorInfo.jsonStr);
					status = pSusiIoTSetPFData(jData);
					if(status != SUSIIOT_STATUS_SUCCESS)
					{
						if(strlen(setRetStr))
						{
							sprintf(setRetStr, "%s,%d", setRetStr, curNode->sensorInfo.id);
						}
						else
						{
							sprintf(setRetStr, "%d", curNode->sensorInfo.id);
						}
					}
				}
				else
				{
					if(strlen(setRetStr))
					{
						sprintf(setRetStr, "%s,%d", setRetStr, curNode->sensorInfo.id);
					}
					else
					{
						sprintf(setRetStr, "%d", curNode->sensorInfo.id);
					}
				}
				curNode = curNode->next;
			}
		}
		else
		{
			sprintf(setRetStr, "All", setRetStr);
		}

		if(strlen(setRetStr))
		{
			sprintf(setRetStr, "%s set failed!", setRetStr);
		}
		else
		{
			sprintf(setRetStr, "%s","success");
		}
		{
			char * repJsonStr = NULL;
			int jsonStrlen = Parser_PackSetSensorDataRep(setRetStr, pCurSessionID, &repJsonStr);
			if(jsonStrlen > 0 && repJsonStr != NULL)
			{
				g_sendcbf(&g_PluginInfo, susictrl_set_sensors_data_rep, repJsonStr, strlen(repJsonStr)+1, NULL, NULL);
			}
			if(repJsonStr)free(repJsonStr);
		}
	}
}

static CAGENT_PTHREAD_ENTRY(GetIotDataThreadStart, args)
{
#define DEF_SLEEP_MS    1000
	while(IsGetIotDataThreadRunning)
	{
		if(pSusiIoTGetPFDataString)
		{
			char * tmpDataStr = NULL;
			int dataLen = 0;
			tmpDataStr = (char *)pSusiIoTGetPFDataString(0);
			if(tmpDataStr)
			{
				dataLen = strlen(tmpDataStr)+1;
				app_os_mutex_lock(&CurIotDataMutex);
				if(CurIotDataJsonStr == NULL || strlen(CurIotDataJsonStr) + 1 != dataLen)
				{
					if(CurIotDataJsonStr) free(CurIotDataJsonStr);
					CurIotDataJsonStr = (char*)malloc(dataLen);
				}
				memset(CurIotDataJsonStr, 0, dataLen);
				strcpy(CurIotDataJsonStr, tmpDataStr);
				app_os_mutex_unlock(&CurIotDataMutex);
				pSusiIoTMemFree(tmpDataStr);
			}
		}
		{
			int i = 0;
			for(i=0; i<DEF_SLEEP_MS/10;i++)
			{
				if(!IsGetIotDataThreadRunning)break;
				app_os_sleep(10);
			}
		}
	}
	IsGetIotDataThreadRunning = false;
	return 0;
}

void RepIotData(char * iotDataJsonStr, char * repFilter)
{
	if(iotDataJsonStr == NULL || repFilter == NULL) return;
	{
		char * repJsonStr = NULL;
		int jsonStrlen = Parser_PackReportIotData(iotDataJsonStr, repFilter, DEF_HANDLER_NAME , &repJsonStr);
		if(jsonStrlen > 0 && repJsonStr != NULL)
		{
 			if(g_sendreportcbf)
 				g_sendreportcbf(&g_PluginInfo, repJsonStr, strlen(repJsonStr)+1, NULL, NULL);
		}	
		if(repJsonStr)free(repJsonStr);
	}
}

static CAGENT_PTHREAD_ENTRY(ReportIotDataThreadStart, args)
{
	int curIntervalTimeMs = 0;
	report_data_type_t curRepType = rep_data_unknown;
	char tmpRepFilter[4096] = {0};
	while(IsReportIotDataThreadRunning)
	{
      app_os_mutex_lock(&ReportParamsMutex);
      curIntervalTimeMs = ReportDataParams.intervalTimeMs;
		if(curRepType != ReportDataParams.repType)
		{
			curRepType = ReportDataParams.repType;
		}
		if(curRepType == rep_data_auto)
		{
			memset(tmpRepFilter, 0, sizeof(tmpRepFilter));
			strcpy(tmpRepFilter, ReportDataParams.repFilter);
		}
		app_os_mutex_unlock(&ReportParamsMutex);
		if(curRepType == rep_data_auto && strlen(tmpRepFilter))
		{
			app_os_mutex_lock(&CurIotDataMutex);
			RepIotData(CurIotDataJsonStr, tmpRepFilter);
			app_os_mutex_unlock(&CurIotDataMutex);
			app_os_sleep(curIntervalTimeMs);
		}
		app_os_sleep(10);
	}
	return 0;
}

void UploadIotData(char * iotDataJsonStr, char * repFilter)
{
	if(iotDataJsonStr == NULL || repFilter == NULL) return;
	{
		char * repJsonStr = NULL;
		int jsonStrlen = Parser_PackAutoUploadIotData(iotDataJsonStr, repFilter, DEF_HANDLER_NAME , &repJsonStr);
		if(jsonStrlen > 0 && repJsonStr != NULL)
		{
			if(g_sendcbf)
				g_sendcbf(&g_PluginInfo, susictrl_auto_upload_rep, repJsonStr, strlen(repJsonStr)+1, NULL, NULL);
		}	
		if(repJsonStr)free(repJsonStr);
	}
}

static CAGENT_PTHREAD_ENTRY(AutoUploadThreadStart, args)
{
	clock_t timeoutNow = 0;
	unsigned int timeoutStart = 0;
	unsigned int curIntervalTimeMs = 0;
	unsigned int curContinueTimeMs = 0;
	report_data_type_t curRepType = rep_data_unknown;
	char tmpRepFilter[4096] = {0};
	while(IsAutoUploadThreadRunning)
	{
		app_os_mutex_lock(&AutoUploadParamsMutex);
		if(AutoUploadParams.repType != rep_data_unknown)
		{
			curIntervalTimeMs = AutoUploadParams.intervalTimeMs;
			curContinueTimeMs = AutoUploadParams.continueTimeMs;
			if(curRepType != AutoUploadParams.repType)
			{
				curRepType = AutoUploadParams.repType;
			}
			if(curRepType == rep_data_auto)
			{
				timeoutStart = 0;
				memset(tmpRepFilter, 0, sizeof(tmpRepFilter));
				strcpy(tmpRepFilter, AutoUploadParams.repFilter);
			}
			AutoUploadParams.repType = rep_data_unknown;
		}
		app_os_mutex_unlock(&AutoUploadParamsMutex);

		if(curRepType == rep_data_auto && strlen(tmpRepFilter))
		{
			timeoutNow = clock();
			if(timeoutStart == 0)
			{
				timeoutStart = timeoutNow;
			}
         
			if(timeoutNow - timeoutStart < curContinueTimeMs)
			{
				app_os_mutex_lock(&CurIotDataMutex);
				UploadIotData(CurIotDataJsonStr, tmpRepFilter);
				app_os_mutex_unlock(&CurIotDataMutex);
				app_os_sleep(curIntervalTimeMs);
			}
			else
			{
				curRepType = rep_data_unknown;
				timeoutStart = 0;
			}
		}
		app_os_sleep(10);
	}
	return 0;
}

static BOOL InitSUSICtrlThrFromConfig(susictrl_thr_item_list thrItemList, char * cfgPath)
{
	BOOL bRet = FALSE;
	FILE *fptr = NULL;
	char * pTmpThrInfoStr = NULL;
	if(thrItemList == NULL || cfgPath == NULL) return bRet;
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
				if(Parser_ParseThrInfo(pTmpThrInfoStr, thrItemList))
				{
					bRet = TRUE;
				}
			}
			if(pTmpThrInfoStr) free(pTmpThrInfoStr);
		}
	}
	fclose(fptr);
	return bRet;
}

static void SUSICtrlThrFillConfig(susictrl_thr_item_list thrItemList, char * cfgPath)
{
	if(thrItemList == NULL || cfgPath == NULL) return;
	{
		char * pJsonStr = NULL;
		int jsonLen = 0;
		jsonLen = Parser_PackThrInfo(thrItemList, &pJsonStr);
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

static BOOL GetIotDataInfoWithID(int id, iot_data_info_t * pIotDataInfo)
{
	BOOL bRet = FALSE;
	if(NULL == pIotDataInfo) return bRet;
	if(pSusiIoTGetPFDataString)
	{
		char * tmpDataStr = NULL;
		tmpDataStr = (char *)pSusiIoTGetPFDataString(id);
      if(tmpDataStr)
		{
			if(Parser_ParseIotDataInfo(tmpDataStr, pIotDataInfo))
			{
				bRet = TRUE;
			}
			pSusiIoTMemFree(tmpDataStr);
		}
	}
	return bRet;
}

static BOOL SUSICtrlCheckSrcVal(susictrl_thr_item_info_t * pThrItemInfo, float checkValue)
{
	BOOL bRet = FALSE;
	if(pThrItemInfo == NULL) return bRet;
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
						while(curNode)
						{
							if((int)curNode->ckV != DEF_INVALID_VALUE) 
							{
								avgTmpF += curNode->ckV;
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
						while(curNode)
						{
							if(curNode->ckV > maxTmpF) maxTmpF = curNode->ckV;
							curNode = curNode->next; 
						}
						if(maxTmpF > -999)
						{
							pThrItemInfo->checkRetValue = maxTmpF;
							bRet = TRUE;
						}
						break;
					}
				case ck_type_min:
					{
						check_value_node_t * curNode = pThrItemInfo->checkSrcValList.head->next;
						float minTmpF = 99999;
						while(curNode)
						{
							if(curNode->ckV < minTmpF) minTmpF = curNode->ckV;
							curNode = curNode->next; 
						}
						if(minTmpF < 99999)
						{
							pThrItemInfo->checkRetValue = minTmpF;
							bRet = TRUE;
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
		}
		{
			check_value_node_t * head = pThrItemInfo->checkSrcValList.head;
			check_value_node_t * newNode = (check_value_node_t *)malloc(sizeof(check_value_node_t));
			newNode->checkValTime = nowTime;
			newNode->ckV = checkValue;
			newNode->next = head->next;
			head->next = newNode;
			pThrItemInfo->checkSrcValList.nodeCnt++;
		}
	}
	return bRet;
}

static BOOL SUSICtrlCheckThrItem(susictrl_thr_item_info_t * pThrItemInfo, float checkValue, char * checkRetMsg)
{
	BOOL bRet = FALSE;
	BOOL isTrigger = FALSE;
	BOOL triggerMax = FALSE;
	BOOL triggerMin = FALSE;
	char tmpRetMsg[1024] = {0};
	char checkTypeStr[64] = {0};
	char descEventStr[256] = {0};
	if(pThrItemInfo == NULL || checkRetMsg== NULL) return bRet;
	{
		switch(pThrItemInfo->checkType)
		{
		case ck_type_avg:
			{
				sprintf(checkTypeStr, DEF_AVG_EVENT_STR);
				break;
			}
		case ck_type_max:
			{
				sprintf(checkTypeStr, DEF_MAX_EVENT_STR);
				break;
			}
		case ck_type_min:
			{
				sprintf(checkTypeStr, DEF_MIN_EVENT_STR);
				break;
			}
		default: break;
		}
	}
	{
		if(SUSICtrlCheckSrcVal(pThrItemInfo, checkValue) && (int)pThrItemInfo->checkRetValue != DEF_INVALID_VALUE)
		{  
			if(pThrItemInfo->desc && strlen(pThrItemInfo->desc))
			{
				char tmpDescStr[512]={0};
				char tmpStr[128] = {0};
				int i = 0, j = 0, n = 0;
				char * buf = NULL, *buf1 = NULL, *itemToken[20] = {NULL}, *itemToken1[3] = {NULL};
				strcpy(tmpDescStr, pThrItemInfo->desc);
				buf = tmpDescStr;
				while(itemToken[i] = strtok(buf, "|"))
				{
					i++;
					if(i>20)break;
					buf = NULL;
				}
				for(n = 0; n<i; n++)
				{
					memset(tmpStr, 0, sizeof(tmpStr));
					strcpy(tmpStr, itemToken[n]);
					buf1 = tmpStr;
					j=0;
					while(itemToken1[j] = strtok(buf1, "="))
					{
						j++;
						if(j>2)break;
						buf1 = NULL;
					}
					if(j == 2)
					{
						if((int)pThrItemInfo->checkRetValue == atoi(itemToken1[0]))
						{
							//strcpy(descEventStr, itemToken1[1]);
							sprintf(descEventStr,"#tk#%s#tk#", itemToken1[1]);
							break;
						}
					}
				}
			}

			if(pThrItemInfo->thrType & DEF_THR_MAX_TYPE)
			{
				if(pThrItemInfo->maxThr != DEF_INVALID_VALUE && (pThrItemInfo->checkRetValue > pThrItemInfo->maxThr))
				{
					//sprintf(tmpRetMsg, "%d(%s:%.0f)>maxThreshold(%.0f)", pThrItemInfo->id, checkTypeStr, pThrItemInfo->checkRetValue.vf, pThrItemInfo->maxThr);
					sprintf(tmpRetMsg, "%s(%s:%f)>%s(%f)", pThrItemInfo->name, checkTypeStr, pThrItemInfo->checkRetValue, DEF_MAX_THR_EVENT_STR, pThrItemInfo->maxThr);
					if(strlen(descEventStr)) sprintf(tmpRetMsg, "%s:%s", tmpRetMsg, descEventStr);
					triggerMax = TRUE;
				}
			}
			if(pThrItemInfo->thrType & DEF_THR_MIN_TYPE)
			{
				if(pThrItemInfo->minThr != DEF_INVALID_VALUE && (pThrItemInfo->checkRetValue  < pThrItemInfo->minThr))
				{
					//if(strlen(tmpRetMsg)) sprintf(tmpRetMsg, "%s and %d(%s:%.0f)<minThreshold(%.0f)", tmpRetMsg, pThrItemInfo->id, checkTypeStr, pThrItemInfo->checkRetValue.vf, pThrItemInfo->minThr);
					//else sprintf(tmpRetMsg, "%d(%s:%.0f)<minThreshold(%.0f)", pThrItemInfo->id, checkTypeStr, pThrItemInfo->checkRetValue.vf, pThrItemInfo->minThr);
					if(strlen(tmpRetMsg)) sprintf(tmpRetMsg, "%s %s %s(%s:%f)<%s(%f)", tmpRetMsg, DEF_AND_EVENT_STR, pThrItemInfo->name, checkTypeStr, pThrItemInfo->checkRetValue, DEF_MIN_THR_EVENT_STR, pThrItemInfo->minThr);
					else sprintf(tmpRetMsg, "%s(%s:%f)<%s(%f)", pThrItemInfo->name, checkTypeStr, pThrItemInfo->checkRetValue, DEF_MIN_THR_EVENT_STR, pThrItemInfo->minThr);
					if(strlen(descEventStr)) sprintf(tmpRetMsg, "%s:%s", tmpRetMsg, descEventStr);
					triggerMin = TRUE;
				}
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
		if(!pThrItemInfo->isNormal && (int)pThrItemInfo->checkRetValue != DEF_INVALID_VALUE)
		{
			memset(tmpRetMsg, 0, sizeof(tmpRetMsg));
			//sprintf(tmpRetMsg, "%d(%s:%.0f) normal", pThrItemInfo->id, checkTypeStr, pThrItemInfo->checkRetValue.vf);
			sprintf(tmpRetMsg, "%s(%s:%f) %s", pThrItemInfo->name, checkTypeStr, pThrItemInfo->checkRetValue, DEF_NOR_EVENT_STR);
			if(strlen(descEventStr)) sprintf(tmpRetMsg, "%s:%s", tmpRetMsg, descEventStr);
			pThrItemInfo->isNormal = TRUE;
			bRet = TRUE;
		}
	}

	if(!bRet) sprintf(checkRetMsg,"");
	else sprintf(checkRetMsg, "%s", tmpRetMsg);

	return bRet;
}

static BOOL SUSICtrlCheckThr(susictrl_thr_item_list curThrItemList, char ** checkRetMsg, unsigned int bufLen, BOOL * isNormal)
{
	BOOL bRet = FALSE;
	if(curThrItemList == NULL || checkRetMsg == NULL || (char*)*checkRetMsg == NULL || bufLen ==0) return bRet;
	{
		susictrl_thr_item_node_t * curThrItemNode = NULL;
		char tmpMsg[1024] = {0};

		curThrItemNode = curThrItemList->next;
		while(curThrItemNode)
		{
			if(curThrItemNode->thrItemInfo.isEnable)
			{
				iot_data_info_t curIotDataInfo;
				memset((char *)&curIotDataInfo, 0, sizeof(iot_data_info_t));
				if(!GetIotDataInfoWithID(curThrItemNode->thrItemInfo.id, &curIotDataInfo) || curIotDataInfo.val.vType != VT_F)
				{
					if(bufLen<strlen(*checkRetMsg)+strlen(curThrItemNode->thrItemInfo.name)+strlen(DEF_NOT_SUPT_EVENT_STR)+16)
					{
						int newLen = strlen(*checkRetMsg)+strlen(curThrItemNode->thrItemInfo.name)+strlen(DEF_NOT_SUPT_EVENT_STR)+2*1024;
						*checkRetMsg = (char*)realloc(*checkRetMsg, newLen);
					}
					if(strlen(*checkRetMsg))sprintf(*checkRetMsg, "%s;%s %s", *checkRetMsg, curThrItemNode->thrItemInfo.name, DEF_NOT_SUPT_EVENT_STR);
					else sprintf(*checkRetMsg, "%s %s", curThrItemNode->thrItemInfo.name, DEF_NOT_SUPT_EVENT_STR);
					curThrItemNode->thrItemInfo.isEnable = FALSE;
					curThrItemNode = curThrItemNode->next;
					if(curIotDataInfo.name)
					{
						free(curIotDataInfo.name);
						curIotDataInfo.name = NULL;
					}
					app_os_sleep(10);
					continue;
				}
				if(curThrItemNode->thrItemInfo.desc == NULL)
				{
					char tmpDescStr[512] = {0};
					app_os_mutex_lock(&IotPFIDataMutex);
					Parser_GetItemDescWithPath(curThrItemNode->thrItemInfo.name, IotPFIDataJsonStr, tmpDescStr);
					app_os_mutex_unlock(&IotPFIDataMutex);
					if(strlen(tmpDescStr))
					{
						int len = strlen(tmpDescStr) +1;
						curThrItemNode->thrItemInfo.desc = (char *)malloc(len);
						memset(curThrItemNode->thrItemInfo.desc, 0, len);
						strcpy(curThrItemNode->thrItemInfo.desc, tmpDescStr);
					}
				}

				memset(tmpMsg, 0, sizeof(tmpMsg));
				SUSICtrlCheckThrItem(&curThrItemNode->thrItemInfo, curIotDataInfo.val.uVal.vf, tmpMsg);
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
				if(*isNormal && !curThrItemNode->thrItemInfo.isNormal)
				{
					*isNormal = curThrItemNode->thrItemInfo.isNormal; 
				}
				if(curIotDataInfo.name)
				{
					free(curIotDataInfo.name);
					curIotDataInfo.name = NULL;
				}
			}
			curThrItemNode = curThrItemNode->next;
			app_os_sleep(10);
		}
	}
	return bRet = TRUE;
}

static void SUSICtrlIsThrItemListNormal(susictrl_thr_item_list curThrItemList, BOOL * isNormal)
{
	if(NULL == isNormal || curThrItemList == NULL) return;
	{
		susictrl_thr_item_node_t * curThrItemNode = NULL;
		curThrItemNode = curThrItemList->next;
		while(curThrItemNode)
		{
			if(curThrItemNode->thrItemInfo.isEnable && !curThrItemNode->thrItemInfo.isNormal)
			{
				*isNormal = FALSE;
				break;
			}
			curThrItemNode = curThrItemNode->next;
		}
	}
}

static CAGENT_PTHREAD_ENTRY(ThrCheckThreadStart, args)
{
	unsigned int defRepMsg = 4*1024;
	char *repMsg = NULL;
	bool bRet = false;
	BOOL isNormal = TRUE;
	repMsg = (char *)malloc(defRepMsg);
	while (IsThrCheckThreadRunning)
	{	
		isNormal = TRUE;
		memset(repMsg, 0, sizeof(repMsg));
		app_os_mutex_lock(&ThrInfoMutex);
		SUSICtrlCheckThr(SUSICtrlThrItemList, &repMsg, defRepMsg, &isNormal);
		app_os_mutex_unlock(&ThrInfoMutex);
	
		if(strlen(repMsg))
		{
			char * repJsonStr = NULL;
			int jsonStrlen = 0, len = 0;
			susictrl_thr_rep_t thrRepInfo;
			thrRepInfo.isTotalNormal = isNormal;
			len = strlen(repMsg) +1;
			thrRepInfo.repInfo = (char*)malloc(len);
			memset(thrRepInfo.repInfo, 0, len);
			strcpy(thrRepInfo.repInfo, repMsg);
			jsonStrlen = Parser_PackThrCheckRep(&thrRepInfo, &repJsonStr);
			if(jsonStrlen > 0 && repJsonStr != NULL)
			{
				g_sendcbf(&g_PluginInfo, susictrl_thr_check_rep, repJsonStr, strlen(repJsonStr)+1, NULL, NULL);
			}
			if(repJsonStr)free(repJsonStr);
			if(thrRepInfo.repInfo) free(thrRepInfo.repInfo);
		}
      
		{//app_os_sleep(5000);
			int i = 0;
			for(i = 0; IsThrCheckThreadRunning && i<10; i++)
			{
				app_os_sleep(100);
			}
		}
	}
	if(repMsg)free(repMsg);
	return 0;
}

static void SUSICtrlWhenDelThrCheckNormal(susictrl_thr_item_list thrItemList, char** checkMsg, unsigned int bufLen)
{
	if(NULL == thrItemList || NULL == checkMsg|| NULL == (char*)(*checkMsg) || bufLen == 0) return;
	{
		susictrl_thr_item_list curThrItemList = thrItemList;
		susictrl_thr_item_node_t * curThrItemNode = curThrItemList->next;
		char *tmpMsg = NULL;
		while(curThrItemNode)
		{
			if(curThrItemNode->thrItemInfo.isEnable && !curThrItemNode->thrItemInfo.isNormal)
			{
				curThrItemNode->thrItemInfo.isNormal = TRUE;
				//sprintf(tmpMsg, "%d normal", curThrItemNode->thrItemInfo.id);
				{
					int len = strlen(curThrItemNode->thrItemInfo.name)+strlen(DEF_NOR_EVENT_STR)+32;
					tmpMsg = (char*)malloc(len);
					memset(tmpMsg, 0, len);
				}
				sprintf(tmpMsg, "%s %s", curThrItemNode->thrItemInfo.name, DEF_NOR_EVENT_STR);
			}
			if(tmpMsg && strlen(tmpMsg))
			{
				if(bufLen<strlen(tmpMsg)+strlen(*checkMsg)+1)
				{
					int newLen = strlen(tmpMsg) + strlen(*checkMsg) + 1024;
					*checkMsg = (char *)realloc(*checkMsg, newLen);
				}	
				if(strlen(*checkMsg))
				{
					sprintf(*checkMsg, "%s;%s", *checkMsg, tmpMsg);
				}
				else
				{
					sprintf(*checkMsg, "%s", tmpMsg);
				}
			}
         if(tmpMsg)free(tmpMsg);
			tmpMsg = NULL;
			curThrItemNode = curThrItemNode->next;
		}
	}
}

static BOOL SUSICtrlUpdateThrInfoList(susictrl_thr_item_list curThrList, susictrl_thr_item_list newThrList)
{
	BOOL bRet = FALSE;
	if(NULL == newThrList || NULL == curThrList) return bRet;
	{
		susictrl_thr_item_node_t * newThrItemNode = NULL, * findThrItemNode = NULL;
		susictrl_thr_item_node_t * curThrItemNode = curThrList->next;
		while(curThrItemNode) //first all thr node set invalid
		{
			curThrItemNode->thrItemInfo.isInvalid = 1;
			curThrItemNode = curThrItemNode->next;
		}
		newThrItemNode = newThrList->next;
		while(newThrItemNode)  //merge old&new thr list
		{
         findThrItemNode = FindSUSICtrlThrNode(curThrList, newThrItemNode->thrItemInfo.id);
			if(findThrItemNode) //exist then update thr argc
			{
				findThrItemNode->thrItemInfo.isInvalid = 0;
				findThrItemNode->thrItemInfo.intervalTimeS = newThrItemNode->thrItemInfo.intervalTimeS;
				findThrItemNode->thrItemInfo.lastingTimeS = newThrItemNode->thrItemInfo.lastingTimeS;
				findThrItemNode->thrItemInfo.isEnable = newThrItemNode->thrItemInfo.isEnable;
				findThrItemNode->thrItemInfo.maxThr = newThrItemNode->thrItemInfo.maxThr;
				findThrItemNode->thrItemInfo.minThr = newThrItemNode->thrItemInfo.minThr;
				findThrItemNode->thrItemInfo.thrType = newThrItemNode->thrItemInfo.thrType;
			}
			else  //not exist then insert to old list
			{
				InsertSUSICtrlThrList(curThrList, &newThrItemNode->thrItemInfo);
			}
			newThrItemNode = newThrItemNode->next;
		}
		{
			unsigned int defRepMsg = 2*1024;
			char *repMsg= (char*)malloc(defRepMsg);
			memset(repMsg, 0, defRepMsg);
			susictrl_thr_item_node_t * preNode = curThrList,* normalRepNode = NULL, *delNode = NULL;
			curThrItemNode = preNode->next;
			while(curThrItemNode) //check need delete&normal report node
			{
				normalRepNode = NULL;
				delNode = NULL;
				if(curThrItemNode->thrItemInfo.isInvalid == 1)
				{
					preNode->next = curThrItemNode->next;
					delNode = curThrItemNode;
					if(curThrItemNode->thrItemInfo.isNormal == FALSE)
					{
						normalRepNode = curThrItemNode;
					}
				}
				else
				{
					preNode = curThrItemNode;
				}
				if(normalRepNode == NULL && curThrItemNode->thrItemInfo.isEnable == FALSE && curThrItemNode->thrItemInfo.isNormal == FALSE)
				{
					normalRepNode = curThrItemNode;
				}
				if(normalRepNode)
				{
					char *tmpMsg = NULL;
					int len = strlen(curThrItemNode->thrItemInfo.name)+strlen(DEF_NOR_EVENT_STR)+32;
					tmpMsg = (char*)malloc(len);
					memset(tmpMsg, 0, len);
					sprintf(tmpMsg, "%s %s", curThrItemNode->thrItemInfo.name, DEF_NOR_EVENT_STR);
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
					normalRepNode->thrItemInfo.isNormal = TRUE;
				}
				if(delNode)
				{
					if(delNode->thrItemInfo.checkSrcValList.head)
					{
						check_value_node_t * frontValueNode = delNode->thrItemInfo.checkSrcValList.head;
						check_value_node_t * delValueNode = frontValueNode->next;
						while(delValueNode)
						{
							frontValueNode->next = delValueNode->next;
							free(delValueNode);
							delValueNode = frontValueNode->next;
						}
						free(delNode->thrItemInfo.checkSrcValList.head);
						delNode->thrItemInfo.checkSrcValList.head = NULL;
					}
					if(delNode->thrItemInfo.name) free(delNode->thrItemInfo.name);
					if(delNode->thrItemInfo.desc) free(delNode->thrItemInfo.desc);
					free(delNode);
					delNode = NULL;
				}
				curThrItemNode = preNode->next;
			}
			if(strlen(repMsg))
			{
				char * repJsonStr = NULL;
				int jsonStrlen = 0;
				susictrl_thr_rep_t thrRepInfo;
				int repInfoLen = strlen(repMsg)+1;
				thrRepInfo.isTotalNormal = TRUE;
				SUSICtrlIsThrItemListNormal(curThrList, &thrRepInfo.isTotalNormal);
				thrRepInfo.repInfo = (char*)malloc(repInfoLen);
				memset(thrRepInfo.repInfo, 0, repInfoLen);
				strcpy(thrRepInfo.repInfo, repMsg);
				jsonStrlen = Parser_PackThrCheckRep(&thrRepInfo, &repJsonStr);
				if(jsonStrlen > 0 && repJsonStr != NULL)
				{
					g_sendcbf(&g_PluginInfo, susictrl_thr_check_rep, repJsonStr, strlen(repJsonStr)+1, NULL, NULL);
				}
				if(repJsonStr)free(repJsonStr);
				if(thrRepInfo.repInfo) free(thrRepInfo.repInfo);
			}
			if(repMsg) free(repMsg);
		}
	}
	bRet = TRUE;
	return bRet;
}

static CAGENT_PTHREAD_ENTRY(SetThrThreadStart, args)
{
	char repMsg[1024] = {0};
	BOOL bRet = FALSE;
	susictrl_thr_item_list tmpThrItemList = CreateSUSICtrlThrList();
	bRet = InitSUSICtrlThrFromConfig(tmpThrItemList, SUSICtrlThrTmpCfgPath);
	if(!bRet)
	{
		sprintf(repMsg, "%s", "SUSICtrl threshold apply failed!");
	}
	else
	{
		app_os_mutex_lock(&ThrInfoMutex);
		SUSICtrlUpdateThrInfoList(SUSICtrlThrItemList, tmpThrItemList);	
		SUSICtrlThrFillConfig(SUSICtrlThrItemList, SUSICtrlThrCfgPath);
		app_os_mutex_unlock(&ThrInfoMutex);
		sprintf(repMsg,"SUSICtrl threshold apply OK!");
	}
	DestroySUSICtrlThrList(tmpThrItemList);
	if(strlen(repMsg))
	{
		char * repJsonStr = NULL;
		int jsonStrlen = Parser_PackSetThrRep(repMsg, &repJsonStr);
		if(jsonStrlen > 0 && repJsonStr != NULL)
		{
			g_sendcbf(&g_PluginInfo, susictrl_set_thr_rep, repJsonStr, strlen(repJsonStr)+1, NULL, NULL);
		}
		if(repJsonStr)free(repJsonStr);
	}
	remove(SUSICtrlThrTmpCfgPath);
	//app_os_CloseHandle(SetThrThreadHandle);
	//SetThrThreadHandle = NULL;
	IsSetThrThreadRunning = FALSE;
	return 0;
}

static void SUSICtrlSetThr(char *thrJsonStr)
{
	if(NULL == thrJsonStr) return;
	{
		char repMsg[256] = {0};
		if(!IsSetThrThreadRunning)
		{
			FILE * fPtr = fopen(SUSICtrlThrTmpCfgPath, "wb");
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
				remove(SUSICtrlThrTmpCfgPath);
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
				g_sendcbf(&g_PluginInfo, susictrl_set_thr_rep, repJsonStr, strlen(repJsonStr)+1, NULL, NULL);
			}
			if(repJsonStr)free(repJsonStr);
		}
	}
}

static void SUSICtrlDelAllThr()
{
	susictrl_thr_item_list curThrItemList = NULL;
	unsigned int defCheckRepMsgLen = 2*1024;
	char * checkRepMsg = (char*)malloc(defCheckRepMsgLen);
	memset(checkRepMsg, 0, defCheckRepMsgLen);
	app_os_mutex_lock(&ThrInfoMutex);
	SUSICtrlWhenDelThrCheckNormal(SUSICtrlThrItemList, &checkRepMsg, defCheckRepMsgLen);
   DeleteAllSUSICtrlThrNode(SUSICtrlThrItemList);
	SUSICtrlThrFillConfig(SUSICtrlThrItemList, SUSICtrlThrCfgPath);
	app_os_mutex_unlock(&ThrInfoMutex);

	if(strlen(checkRepMsg))
	{
		char * repJsonStr = NULL;
		int jsonStrlen = 0;
		susictrl_thr_rep_t thrRepInfo;
		int repInfoLen = strlen(checkRepMsg)+1;
		thrRepInfo.isTotalNormal = TRUE;
		thrRepInfo.repInfo = (char*)malloc(repInfoLen);
		memset(thrRepInfo.repInfo, 0, repInfoLen);
		strcpy(thrRepInfo.repInfo, checkRepMsg);
		jsonStrlen = Parser_PackThrCheckRep(&thrRepInfo, &repJsonStr);
		if(jsonStrlen > 0 && repJsonStr != NULL)
		{
			g_sendcbf(&g_PluginInfo, susictrl_thr_check_rep, repJsonStr, strlen(repJsonStr)+1, NULL, NULL);
		}
		if(repJsonStr)free(repJsonStr);
		if(thrRepInfo.repInfo)free(thrRepInfo.repInfo);
	}
   if(checkRepMsg) free(checkRepMsg);
	{
		char * repJsonStr = NULL;
		int jsonStrlen = 0;
		char delRepMsg[256] = {0};
		sprintf_s(delRepMsg, sizeof(delRepMsg), "%s", "Delete all threshold successfully!");
      jsonStrlen = Parser_PackDelAllThrRep(delRepMsg, &repJsonStr);
		if(jsonStrlen > 0 && repJsonStr != NULL)
		{
			g_sendcbf(&g_PluginInfo, susictrl_del_thr_rep, repJsonStr, strlen(repJsonStr)+1, NULL, NULL);
		}
		if(repJsonStr)free(repJsonStr);
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

	// 3. Callback function -> Send JSON Data by this callback function
	g_sendcbf = g_PluginInfo.sendcbf = pluginfo->sendcbf;
	g_sendcustcbf = g_PluginInfo.sendcustcbf = pluginfo->sendcustcbf;
	g_subscribecustcbf = g_PluginInfo.subscribecustcbf = pluginfo->subscribecustcbf;
	g_sendreportcbf = g_PluginInfo.sendreportcbf = pluginfo->sendreportcbf;
	g_sendcapabilitycbf =g_PluginInfo.sendcapabilitycbf = pluginfo->sendcapabilitycbf;
	
	char modulePath[MAX_PATH] = {0};     
	if(app_os_get_module_path(modulePath))
	{
		sprintf(SUSICtrlThrCfgPath, "%s/%s", modulePath, DEF_SUSICTRL_THR_CONFIG_NAME);
		sprintf(SUSICtrlThrTmpCfgPath, "%s/%sTmp", modulePath, DEF_SUSICTRL_THR_CONFIG_NAME);
	}
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
	BOOL bRet = FALSE;
	if(IsHandlerStart)
	{
		bRet = TRUE;
	}
	else
	{
		if(IsExistSUSIIOTLib())
		{
			bRet = StartupSUSIIOTLib();
			if(bRet)
			{
				bRet = FALSE;
				if(IsExistJanssonLib())
				{
					bRet = StartupJanssonLib();
				}
			}
		}
		if(bRet)
		{
			bRet = FALSE;
			SUSICtrlThrItemList = CreateSUSICtrlThrList();
			/*if(app_os_is_file_exist(SUSICtrlThrCfgPath))
			{
				InitSUSICtrlThrFromConfig(SUSICtrlThrItemList, SUSICtrlThrCfgPath);
			}*/

			if(app_os_mutex_setup(&CurIotDataMutex) != 0)
			{
				SUSICtrlLog(g_loghandle, Normal, " %s> Create CurIotDataMutex failed!", strPluginName);
				goto done1;
			}

			if(app_os_mutex_setup(&IotPFIDataMutex) != 0)
			{
				SUSICtrlLog(g_loghandle, Normal, " %s> Create IotPFIDataMutex failed!", strPluginName);
				goto done1;
			}
			if(pSusiIoTGetPFCapabilityString != NULL)
			{
				char * cpbStr = NULL;
				cpbStr = (char *)pSusiIoTGetPFCapabilityString();
				if(cpbStr != NULL)
				{
					int dataLen = strlen(cpbStr)+1;
					app_os_mutex_lock(&IotPFIDataMutex);
					IotPFIDataJsonStr = (char*)malloc(dataLen);
					memset(IotPFIDataJsonStr, 0, dataLen);
					strcpy(IotPFIDataJsonStr, cpbStr);
					app_os_mutex_unlock(&IotPFIDataMutex);
					pSusiIoTMemFree(cpbStr);
				}
			}
			IsGetIotDataThreadRunning = true;
			if (app_os_thread_create(&GetIotDataThreadHandler, GetIotDataThreadStart, NULL) != 0)
			{
				IsGetIotDataThreadRunning = false;
				SUSICtrlLog(g_loghandle, Normal, " %s> Create GetIotDataThread failed!", strPluginName);	
				goto done1;
			}

			memset(&ReportDataParams, 0, sizeof(report_data_params_t));
			ReportDataParams.repType = rep_data_query;
			if(app_os_mutex_setup(&ReportParamsMutex) != 0)
			{
				SUSICtrlLog(g_loghandle, Normal, " %s> Create ReportParamsMutex failed!", strPluginName);
				goto done1;
			}
			IsReportIotDataThreadRunning = true;
			if (app_os_thread_create(&ReportIotDataThreadHandle, ReportIotDataThreadStart, NULL) != 0)
			{
				IsReportIotDataThreadRunning = false;
				SUSICtrlLog(g_loghandle, Normal, " %s> Create ReportIotDataThread failed!", strPluginName);	
				goto done1;
			}

			memset(&AutoUploadParams, 0, sizeof(report_data_params_t));
			AutoUploadParams.repType = rep_data_unknown;
			if(app_os_mutex_setup(&AutoUploadParamsMutex) != 0)
			{
				SUSICtrlLog(g_loghandle, Normal, " %s> Create AutoUploadParamsMutex failed!", strPluginName);
				goto done1;
			}
			IsAutoUploadThreadRunning = true;
			if (app_os_thread_create(&AutoUploadThreadHandle, AutoUploadThreadStart, NULL) != 0)
			{
				IsAutoUploadThreadRunning = false;
				SUSICtrlLog(g_loghandle, Normal, " %s> Create AutoUploadThread failed!", strPluginName);	
				goto done1;
			}

			if(app_os_mutex_setup(&ThrInfoMutex) != 0)
			{
				SUSICtrlLog(g_loghandle, Normal, " %s> Create ThrInfoMutex failed!", strPluginName);
				goto done1;
			}
			IsThrCheckThreadRunning = true;
			if(app_os_thread_create(&ThrCheckThreadHandle, ThrCheckThreadStart, NULL) != 0)
			{
				IsThrCheckThreadRunning = false;
				SUSICtrlLog(g_loghandle, Normal, " %s> Create ThrCheckThreadStart failed!", strPluginName);
				goto done1;
			}
			bRet = TRUE;
			IsHandlerStart = true;
		}
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
 *  Return:  0  : Success Init Handler
 *              -1 : Fail Init Handler
 * ***************************************************************************************/
int HANDLER_API Handler_Stop( void )
{
	if(IsHandlerStart)
	{
		if(IsThrCheckThreadRunning)
		{
			IsThrCheckThreadRunning = false;
			app_os_thread_join(ThrCheckThreadHandle);
		}
		app_os_mutex_lock(&ThrInfoMutex);
		SUSICtrlThrFillConfig(SUSICtrlThrItemList, SUSICtrlThrCfgPath);
		DestroySUSICtrlThrList(SUSICtrlThrItemList);
		SUSICtrlThrItemList = NULL;
		app_os_mutex_unlock(&ThrInfoMutex);
		app_os_mutex_cleanup(&ThrInfoMutex);
		if(IsReportIotDataThreadRunning)
		{
			IsReportIotDataThreadRunning = false;
			app_os_thread_join(ReportIotDataThreadHandle);
		}
		app_os_mutex_cleanup(&ReportParamsMutex);

		if(IsAutoUploadThreadRunning)
		{
			IsAutoUploadThreadRunning = false;
			app_os_thread_join(AutoUploadThreadHandle);
		}
		app_os_mutex_cleanup(&AutoUploadParamsMutex);

		if(IsGetIotDataThreadRunning)
		{
			IsGetIotDataThreadRunning = false;
			app_os_thread_join(GetIotDataThreadHandler);
		}
		app_os_mutex_cleanup(&CurIotDataMutex);
		app_os_mutex_cleanup(&IotPFIDataMutex);

		CleanupSUSIIOTLib();
		CleanupJanssonLib();
		if(CurIotDataJsonStr)
		{
			free(CurIotDataJsonStr);
			CurIotDataJsonStr = NULL;
		}
		if(IotPFIDataJsonStr)
		{
			free(IotPFIDataJsonStr);
			IotPFIDataJsonStr = NULL;
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
void HANDLER_API Handler_Recv(char * const topic, void* const data, const size_t datalen, void *pRev1, void* pRev2  )
{
	int cmdID = 0;
	char errorStr[128] = {0};
	SUSICtrlLog(g_loghandle, Normal, " %s>Recv Topic [%s] Data %s", strPluginName, topic, (char*) data );
	printf(" >Recv Topic [%s] Data %s", topic, (char*) data );

	if(!ParseReceivedData(data, datalen, &cmdID))
		return;
	switch(cmdID)
	{
	case susictrl_get_capability_req:
		{
			GetCapability();
			break;
		}
#if 0
	case susictrl_get_sensors_data_req:
		{
			char curSessionID[256] = {0};
			char sensersID[1024] = {0};
			if(Parser_ParseGetSensorDataReq(data, sensersID, curSessionID))
			{
				if(strlen(sensersID))
				{
					GetSensorsData(sensersID, curSessionID);
				}
			}
			else
			{
				char * errorRepJsonStr = NULL;
				char errorStr[128];
				sprintf(errorStr, "Command(%d) parse error!", susictrl_get_sensors_data_req);
				int jsonStrlen = Parser_PackSUSICtrlError(errorStr, &errorRepJsonStr);
				if(jsonStrlen > 0 && errorRepJsonStr != NULL)
				{
					g_sendcbf(&g_PluginInfo, susictrl_error_rep, errorRepJsonStr, strlen(errorRepJsonStr)+1, NULL, NULL);
				}
				if(errorRepJsonStr)free(errorRepJsonStr);
			}
			break;
		}
	case susictrl_get_sensors_data_req:
		{
			char curSessionID[256] = {0};
			sensor_info_list sensorInfoList = CreateSensorInfoList();
			if(Parser_ParseGetSensorDataReqEx(data, sensorInfoList, curSessionID))
			{
				if(strlen(curSessionID))
				{
					GetSensorsDataEx(sensorInfoList, curSessionID);
				}
			}
			else
			{
				char * errorRepJsonStr = NULL;
				char errorStr[128];
				sprintf(errorStr, "Command(%d) parse error!", susictrl_get_sensors_data_req);
				int jsonStrlen = Parser_PackSUSICtrlError(errorStr, &errorRepJsonStr);
				if(jsonStrlen > 0 && errorRepJsonStr != NULL)
				{
					g_sendcbf(&g_PluginInfo, susictrl_error_rep, errorRepJsonStr, strlen(errorRepJsonStr)+1, NULL, NULL);
				}
				if(errorRepJsonStr)free(errorRepJsonStr);
			}
			DestroySensorInfoList(sensorInfoList);
			break;
		}
#else
	case susictrl_get_sensors_data_req:
		{
			char curSessionID[256] = {0};
			sensor_info_list sensorInfoList = CreateSensorInfoList();
			if(Parser_ParseGetSensorDataReqEx(data, sensorInfoList, curSessionID))
			{
				if(strlen(curSessionID))
				{
					GetSensorsDataEx(sensorInfoList, curSessionID);
				}
			}
			else
			{
				char curSessionID[256] = {0};
				char sensersID[1024] = {0};
				if(Parser_ParseGetSensorDataReq(data, sensersID, curSessionID))
				{
					if(strlen(sensersID))
					{
						GetSensorsData(sensersID, curSessionID);
					}
				}
				else
				{
					char * errorRepJsonStr = NULL;
					char errorStr[128];
					sprintf(errorStr, "Command(%d) parse error!", susictrl_get_sensors_data_req);
					int jsonStrlen = Parser_PackSUSICtrlError(errorStr, &errorRepJsonStr);
					if(jsonStrlen > 0 && errorRepJsonStr != NULL)
					{
						g_sendcbf(&g_PluginInfo, susictrl_error_rep, errorRepJsonStr, strlen(errorRepJsonStr)+1, NULL, NULL);
					}
					if(errorRepJsonStr)free(errorRepJsonStr);
				}
			}
			DestroySensorInfoList(sensorInfoList);
			break;
		}
#endif
#if 0
	case susictrl_set_sensors_data_req:
		{
			char curSessionID[256] = {0};
			sensor_info_list sensorInfoList = CreateSensorInfoList();
			if(Parser_ParseSetSensorDataReq(data, sensorInfoList, curSessionID))
			{
				SetSensorsData(sensorInfoList, curSessionID);
			}
			else
			{
				char * errorRepJsonStr = NULL;
				char errorStr[128];
				sprintf(errorStr, "Command(%d) parse error!", susictrl_set_sensors_data_req);
				int jsonStrlen = Parser_PackSUSICtrlError(errorStr, &errorRepJsonStr);
				if(jsonStrlen > 0 && errorRepJsonStr != NULL)
				{
					g_sendcbf(&g_PluginInfo, susictrl_error_rep, errorRepJsonStr, strlen(errorRepJsonStr)+1, NULL, NULL);
				}
				if(errorRepJsonStr)free(errorRepJsonStr);
			}
			DestroySensorInfoList(sensorInfoList);
			break;
		}
	case susictrl_set_sensors_data_req:
		{
			char curSessionID[256] = {0};
			sensor_info_list sensorInfoList = CreateSensorInfoList();
			if(Parser_ParseSetSensorDataReq(data, sensorInfoList, curSessionID))
			{
				SetSensorsDataEx(sensorInfoList, curSessionID);
			}
			else
			{
				char * errorRepJsonStr = NULL;
				char errorStr[128];
				sprintf(errorStr, "Command(%d) parse error!", susictrl_set_sensors_data_req);
				int jsonStrlen = Parser_PackSUSICtrlError(errorStr, &errorRepJsonStr);
				if(jsonStrlen > 0 && errorRepJsonStr != NULL)
				{
					g_sendcbf(&g_PluginInfo, susictrl_error_rep, errorRepJsonStr, strlen(errorRepJsonStr)+1, NULL, NULL);
				}
				if(errorRepJsonStr)free(errorRepJsonStr);
			}
			DestroySensorInfoList(sensorInfoList);
			break;
		}
#else
	case susictrl_set_sensors_data_req:
		{
			char curSessionID[256] = {0};
			sensor_info_list sensorInfoListEx = CreateSensorInfoList();
			if(Parser_ParseSetSensorDataReqEx(data, sensorInfoListEx, curSessionID))
			{
				SetSensorsDataEx(sensorInfoListEx, curSessionID);
			}
			else
			{
				char curSessionID[256] = {0};
				sensor_info_list sensorInfoList = CreateSensorInfoList();
				if(Parser_ParseSetSensorDataReq(data, sensorInfoList, curSessionID))
				{
					SetSensorsData(sensorInfoList, curSessionID);
				}
				else
				{
					char * errorRepJsonStr = NULL;
					char errorStr[128];
					sprintf(errorStr, "Command(%d) parse error!", susictrl_set_sensors_data_req);
					int jsonStrlen = Parser_PackSUSICtrlError(errorStr, &errorRepJsonStr);
					if(jsonStrlen > 0 && errorRepJsonStr != NULL)
					{
						g_sendcbf(&g_PluginInfo, susictrl_error_rep, errorRepJsonStr, strlen(errorRepJsonStr)+1, NULL, NULL);
					}
					if(errorRepJsonStr)free(errorRepJsonStr);
				}
				DestroySensorInfoList(sensorInfoList);
			}
			DestroySensorInfoList(sensorInfoListEx);
			break;
		}
#endif
	case susictrl_set_thr_req:
		{
			SUSICtrlSetThr((char *)data);
			break;
		}
	case susictrl_del_thr_req:
		{
			SUSICtrlDelAllThr();
			break;
		}
	case susictrl_auto_upload_req:
		{
			unsigned int intervalTimeMs = 0; //ms
			unsigned int continueTimeMs = 0;
			char tmpRepFilter[4096] = {0};
			bool bRet = Parser_ParseAutoUploadCmd((char *)data, &intervalTimeMs, &continueTimeMs, tmpRepFilter);
			if(bRet)
			{
				app_os_mutex_lock(&AutoUploadParamsMutex);
				AutoUploadParams.intervalTimeMs = intervalTimeMs; //ms
				AutoUploadParams.continueTimeMs = continueTimeMs;
				AutoUploadParams.repType = rep_data_auto;
				memset(AutoUploadParams.repFilter, 0, sizeof(AutoUploadParams.repFilter));
				if(strlen(tmpRepFilter)) strcpy(AutoUploadParams.repFilter, tmpRepFilter);
				app_os_mutex_unlock(&AutoUploadParamsMutex);
			}
			else
			{
				char * errorRepJsonStr = NULL;
				char errorStr[128];
				sprintf(errorStr, "Command(%d) parse error!", susictrl_auto_upload_req);
				int jsonStrlen = Parser_PackSUSICtrlError(errorStr, &errorRepJsonStr);
				if(jsonStrlen > 0 && errorRepJsonStr != NULL)
				{
					g_sendcbf(&g_PluginInfo, susictrl_error_rep, errorRepJsonStr, strlen(errorRepJsonStr)+1, NULL, NULL);
				}
				if(errorRepJsonStr)free(errorRepJsonStr);
			}
			break;
		}
	default:
		{
			char * errorRepJsonStr = NULL;
			char errorStr[128];
			sprintf(errorStr, "%s", "Unknown cmd!");
			int jsonStrlen = Parser_PackSUSICtrlError(errorStr, &errorRepJsonStr);
			if(jsonStrlen > 0 && errorRepJsonStr != NULL)
			{
				g_sendcbf(&g_PluginInfo, susictrl_error_rep, errorRepJsonStr, strlen(errorRepJsonStr)+1, NULL, NULL);
			}
			if(errorRepJsonStr)free(errorRepJsonStr);
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
	unsigned int intervalTimeS = 0; //sec
	char tmpRepFilter[4096] = {0};
	bool bRet = Parser_ParseAutoReportCmd(pInQuery, &intervalTimeS, tmpRepFilter);
	if(bRet)
	{
		app_os_mutex_lock(&ReportParamsMutex);
      ReportDataParams.intervalTimeMs = intervalTimeS*1000; //ms
		ReportDataParams.repType = rep_data_auto;
		memset(ReportDataParams.repFilter, 0, sizeof(ReportDataParams.repFilter));
		if(strlen(tmpRepFilter)) strcpy(ReportDataParams.repFilter, tmpRepFilter);
		app_os_mutex_unlock(&ReportParamsMutex);
	}
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
	unsigned int intervalTimeS = 0; //sec
	bool bRet = Parser_ParseAutoReportStopCmd(pInQuery);
	if(bRet)
	{
		app_os_mutex_lock(&ReportParamsMutex);
		//ReportDataParams.intervalTimeMs = 0; //ms
		ReportDataParams.repType = rep_data_query;
		app_os_mutex_unlock(&ReportParamsMutex);
	}
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
	if(pSusiIoTGetPFCapabilityString != NULL)
	{
		char * cpbStr = NULL;
		cpbStr = (char *)pSusiIoTGetPFCapabilityString();
		if(cpbStr != NULL)
		{
			char * repJsonStr = NULL;
			int jsonStrlen = Parser_PackSpecInfoRep(cpbStr, DEF_HANDLER_NAME , &repJsonStr);
			if(jsonStrlen > 0 && repJsonStr != NULL)
			{
				/*len = strlen(repJsonStr) +1;
				strcpy(pOutReply, repJsonStr);*/
				len = strlen(repJsonStr);
				*pOutReply = (char *)malloc(len + 1);
				memset(*pOutReply, 0, len + 1);
				strcpy(*pOutReply, repJsonStr);
			}
			if(repJsonStr)free(repJsonStr);
			pSusiIoTMemFree(cpbStr);
		}
	}
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