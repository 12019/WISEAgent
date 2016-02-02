/****************************************************************************/
/* Copyright(C) : Advantech Technologies, Inc.								*/
/* Create Date  : 2014/07/07 by Scott68 Chang								*/
/* Modified Date: 2014/07/07 by Scott68 Chang								*/
/* Abstract     : Handler API                                     			*/
/* Reference    : None														*/
/****************************************************************************/
#include "platform.h"
#include "susiaccess_handler_api.h"
#include "common.h"
#include "MonitoringHandler.h"
#include "thresholdhelper.h"
#include "parser.h"
#include "monitorlog.h"


//-----------------------------------------------------------------------------
// Types and defines:
//-----------------------------------------------------------------------------
const char strPluginName[MAX_TOPIC_LEN] = {"device_monitoring"};
const int iRequestID = cagent_request_device_monitoring;
const int iActionID = cagent_action_device_monitoring;
//-----------------------------------------------------------------------------
// Internal Prototypes:
//-----------------------------------------------------------------------------
#define HWM_CHECK_PERIOD 1000
#define HDD_CHECK_PERIOD 1000

typedef struct{
	hwm_info_t hwminfo;
	CAGENT_MUTEX_TYPE hwmmutex;
	CAGENT_MUTEX_TYPE reportmutex;
	int iPeriodUploadInterval;
	int iPeriodUploadTimeout;
	bool bReportOnce;
}hwm_context_t;

typedef struct{
	hdd_info_t hddinfo;
	CAGENT_MUTEX_TYPE hddmutex;
	CAGENT_MUTEX_TYPE reportmutex;
	int iPeriodUploadInterval;
	int iPeriodUploadTimeout;
	bool bReportOnce;
}hdd_context_t;

typedef struct{
	CAGENT_MUTEX_TYPE reportmutex;
	int iAutoReportInterval;
	cJSON* reqItems;
}report_context_t;

typedef struct{
   void* threadHandler;
   bool isThreadRunning;
   bool bHasSUSI;
   bool bHasSQFlash;
   hwm_context_t hwmctx;
   hdd_context_t hddctx;
   report_context_t reportctx;
}handler_context_t;



//-----------------------------------------------------------------------------
// Variables
//-----------------------------------------------------------------------------

static Handler_info          g_PluginInfo;
static handler_context_t     g_HandlerContex;
static CAGENT_MUTEX_TYPE     g_thrmutex;
static thr_items_t           g_thritems;

static HANDLER_THREAD_STATUS g_status = handler_status_no_init;
static char*                 g_platformname = NULL;
static char*                 g_version    = NULL;
static time_t                g_monitortime;
static void*                 g_loghandle  = NULL;
static bool                  g_bEnableLog = true;
static bool                  g_bTrace     = true;



static HandlerSendCbf           g_sendcbf           = NULL;				// Client Send information (in JSON format) to Cloud Server	
static HandlerSendCustCbf       g_sendcustcbf       = NULL;			    // Client Send information (in JSON format) to Cloud Server with custom topic	
static HandlerAutoReportCbf     g_sendreportcbf     = NULL;				// Client Send report (in JSON format) to Cloud Server with AutoReport topic	
static HandlerSendCapabilityCbf g_sendcapabilitycbf = NULL;	
static HandlerSubscribeCustCbf  g_subscribecustcbf  = NULL;
//-----------------------------------------------------------------------------
// UTIL Function:
//-----------------------------------------------------------------------------
#ifdef _MSC_VER
BOOL WINAPI DllMain(HINSTANCE module_handle, DWORD reason_for_call, LPVOID reserved)
{
	if (reason_for_call == DLL_PROCESS_ATTACH) // Self-explanatory
	{
		printf("DllInitializer\r\n");
		DisableThreadLibraryCalls(module_handle); // Disable DllMain calls for DLL_THREAD_*
		if (reserved == NULL) // Dynamic load
		{
			// Initialize your stuff or whatever
			// Return FALSE if you don't want your module to be dynamically loaded
		}
		else // Static load
		{
			// Return FALSE if you don't want your module to be statically loaded
			return FALSE;
		}
	}

	if (reason_for_call == DLL_PROCESS_DETACH) // Self-explanatory
	{
		printf("DllFinalizer\r\n");
		if (reserved == NULL) // Either loading the DLL has failed or FreeLibrary was called
		{
			// Cleanup
			Handler_Uninitialize();
		}
		else // Process is terminating
		{
			// Cleanup
			Handler_Uninitialize();
		}
	}
	return TRUE;
}
#else
__attribute__((constructor))
/**
 * initializer of the shared lib.
 */
static void Initializer(int argc, char** argv, char** envp)
{
    printf("DllInitializer\r\n");
}

__attribute__((destructor))
/** 
 * It is called when shared lib is being unloaded.
 * 
 */
static void Finalizer()
{
    printf("DllFinalizer\r\n");
	Handler_Uninitialize();
}
#endif

void Handler_SendHWMInfo(hwm_info_t * pHWMInfo)
{
	cJSON* hwminfoJSON = NULL;
	char* hwminfo = NULL;
	/*send HWM value*/
	hwminfoJSON = parser_CreateHWMInfo(pHWMInfo);
	hwminfo = cJSON_PrintUnformatted(hwminfoJSON);
	cJSON_Delete(hwminfoJSON);
	MonitorLog(g_loghandle, Normal, " %s>HWM Info: %s", strPluginName, hwminfo);
	if(g_sendcbf)
		g_sendcbf(&g_PluginInfo, devmon_hwminfo_rep, hwminfo, strlen(hwminfo), NULL, NULL);
	//g_PluginInfo.sendcbf(&g_PluginInfo, devmon_hwminfo_rep, hwminfo, strlen(hwminfo), NULL, NULL);
	free(hwminfo);
}

void Handler_PeriodReportHWMInfo(hwm_context_t * pHWMCtx, int interval, int * repInterval, int * repTimeout)
{
	bool bHWMReportOnce = false;
	if(repInterval == NULL || repTimeout == NULL) return;
	*repTimeout = pHWMCtx->iPeriodUploadTimeout;
	bHWMReportOnce = pHWMCtx->bReportOnce;
	pHWMCtx->bReportOnce = false;

	if(bHWMReportOnce) {
		Handler_SendHWMInfo(&pHWMCtx->hwminfo);
	}

	if (*repTimeout > 0) {
		if(*repInterval <= 0) {
			// prevent send twice.
			if(! bHWMReportOnce)  {
				Handler_SendHWMInfo(&pHWMCtx->hwminfo);
			}
			*repInterval = pHWMCtx->iPeriodUploadInterval;
		}
		*repInterval -= interval;
		pHWMCtx->iPeriodUploadTimeout -= interval;
	}
	else {
		pHWMCtx->iPeriodUploadInterval = *repInterval = 0;
		pHWMCtx->iPeriodUploadTimeout  = 0;
	}
}

void Handler_SendThrReport(bool bResult, char* msg)
{
	cJSON* reportJSON = NULL;
	char* report = NULL;
	/*send HWM value*/
	reportJSON = parser_CreateThresholdCheckResult(bResult, msg);
	report = cJSON_PrintUnformatted(reportJSON);
	cJSON_Delete(reportJSON);
	MonitorLog(g_loghandle, Normal, " %s> HWM Info: %s", strPluginName, report);
	if(g_sendcbf)
		g_sendcbf(&g_PluginInfo, devmon_report_rep, report, strlen(report), NULL, NULL);
	//g_PluginInfo.sendcbf(&g_PluginInfo, devmon_report_rep, report, strlen(report), NULL, NULL);
	free(report);
}


bool Handler_ChkHWMThrRule(thr_items_t* pRules, hwm_info_t * pHWMInfo, char * msg, bool *bRetNormal)
{
	bool bRetReport = false;
	sa_thr_item_t * rule = pRules->hwmRule;
	hwm_item_t * item = NULL;
	char tmpMsg[4096] = {0};
	*bRetNormal = true;

	while(rule != NULL)
	{
		char chkMsg[1024] = {0};
		bool bOneReport = false;
		bool bOneNormal = true;
		if (rule->isEnable)
		{
			item = hwm_FindItem(pHWMInfo, rule->tagName);
			if (!item) {
				rule = rule->next;
				continue;
			}
			bOneReport = threshold_ChkOneRule(rule, item->value, chkMsg, &bOneNormal);
			if (bOneReport) {
				if (tmpMsg[0]) sprintf(tmpMsg, "%s; ;%s", tmpMsg, chkMsg);
				else           sprintf(tmpMsg, "%s", chkMsg);
			}	
			bRetReport |= bOneReport;
			*bRetNormal &= bOneNormal;
		}
		rule = rule->next;
	}
	memset(msg, 0, sizeof(msg));
	if ( tmpMsg[0] ) strncpy(msg, tmpMsg, strlen(tmpMsg)+1);
	return bRetReport;
}

void Handler_SendHDDInfo(hdd_info_t * pHDDInfo)
{
	cJSON* hddinfoJSON = NULL;
	char* hddinfo = NULL;
	/*send HWM value*/
	hddinfoJSON = parser_CreateHDDInfo(pHDDInfo);
	hddinfo = cJSON_PrintUnformatted(hddinfoJSON);
	cJSON_Delete(hddinfoJSON);
	//MonitorLog(g_loghandle, Normal, " %s> HDD Info: %s", strPluginName, hddinfo);
	if(g_sendcbf)
		g_sendcbf(&g_PluginInfo, devmon_hddinfo_rep, hddinfo, strlen(hddinfo), NULL, NULL);
	//g_PluginInfo.sendcbf(&g_PluginInfo, devmon_hddinfo_rep, hddinfo, strlen(hddinfo), NULL, NULL);
	free(hddinfo);
}

void Handler_PeriodReportHDDInfo(hdd_context_t * pHDDCtx, int loopInterval, int * repInterval, int * repTimeout)
{
	bool bHDDReportOnce = false;
	if(repInterval == NULL || repTimeout == NULL) return;
	*repTimeout = pHDDCtx->iPeriodUploadTimeout;
	bHDDReportOnce = pHDDCtx->bReportOnce;
	pHDDCtx->bReportOnce = false;

	if(bHDDReportOnce) {
		Handler_SendHDDInfo(&pHDDCtx->hddinfo);
	}

	if(*repTimeout>0) {
		if(*repInterval <= 0) {
			// prevent send twice.
			if(!bHDDReportOnce)  {
				Handler_SendHDDInfo(&pHDDCtx->hddinfo);
			}
			*repInterval = pHDDCtx->iPeriodUploadInterval;
		}
		*repInterval -= loopInterval;
		pHDDCtx->iPeriodUploadTimeout -= loopInterval;
	}
	else {
		pHDDCtx->iPeriodUploadInterval = *repInterval = 0;
		pHDDCtx->iPeriodUploadTimeout  = 0;
	}
}

bool GetHddSmartInfo()
{
	bool bRet = false;
	cJSON* hddsmartJSON = NULL;
	char* hddsmart = NULL;
	AGENT_SEND_STATUS status = cagent_success;

	if(!g_HandlerContex.bHasSQFlash) return bRet;

	app_os_mutex_lock(&g_HandlerContex.hddctx.hddmutex);
	hddsmartJSON = parser_CreateHDDSMART(&g_HandlerContex.hddctx.hddinfo);
	app_os_mutex_unlock(&g_HandlerContex.hddctx.hddmutex);

	if(hddsmartJSON != NULL)
	{
		hddsmart = cJSON_PrintUnformatted(hddsmartJSON);
		cJSON_Delete(hddsmartJSON);

		MonitorLog(g_loghandle, Normal, " %s> HDD S.M.A.R.T Info: %s", strPluginName, hddsmart);
		if(g_sendcbf)
			status = g_sendcbf(&g_PluginInfo, devmon_get_hdd_smart_info_rep, hddsmart, strlen(hddsmart), NULL, NULL);
		//status = g_PluginInfo.sendcbf(&g_PluginInfo, devmon_get_hdd_smart_info_rep, hddsmart, strlen(hddsmart), NULL, NULL);
		free(hddsmart);

		if(status == cagent_success)
		{
			bRet = true;
		}
	}
	return bRet;
}


void Handler_SendAutoReport(handler_context_t *pHandlerContex)
{
	cJSON *root = NULL, *pHWM = NULL;
	cJSON* pENode = NULL;
	char *autoReport = NULL;
	//time_t curtime;
	/*send HWM value*/

	root = cJSON_CreateObject();
	pHWM = cJSON_CreateObject();
	pENode = cJSON_CreateArray();
	cJSON_AddItemToObject(root, HWM_SPEC_TITLE, pHWM);
	cJSON_AddItemToObject(pHWM, HWM_SPEC_EVENT, pENode);
		

	if(pHandlerContex->bHasSUSI)
	{
		parser_CreateHWMReport(&pHandlerContex->hwmctx.hwminfo, pENode);
		//cJSON_AddItemToArray(pHWM, hwminfoJSON);
		//cJSON_Delete(hwminfoJSON);
	}
	if(pHandlerContex->bHasSQFlash)
	{
		parser_CreateHDDReport(&pHandlerContex->hddctx.hddinfo, pENode);
		//cJSON_AddItemToArray(pHWM, hddinfoJSON);
		//cJSON_Delete(hddinfoJSON);
	}
	
	//time(&curtime);
	//cJSON_AddStringToObject(pHWM, "ts", ctime(&curtime));
	autoReport = cJSON_PrintUnformatted(root);
	cJSON_Delete(root);
	
	//MonitorLog(g_loghandle, Normal, " %s> Auto Report: %s", strPluginName, autoReport);
	if(g_sendreportcbf)
		g_sendreportcbf(&g_PluginInfo, autoReport, strlen(autoReport), NULL, NULL);
	free(autoReport);
}


void Handler_AutoReport(handler_context_t *pHandlerContex, int loopInterval, int * repInterval)
{
	cJSON* reqitems = NULL;
	if (repInterval == NULL) return;
	reqitems = pHandlerContex->reportctx.reqItems;  if (!reqitems) return;
	if (*repInterval <= 0) {
		Handler_SendAutoReport(pHandlerContex);
		*repInterval = pHandlerContex->reportctx.iAutoReportInterval;
	}
	*repInterval -= loopInterval;
	return;
}


//-----------------------------------------------------------------------------
// Main Function:
//-----------------------------------------------------------------------------
static CAGENT_PTHREAD_ENTRY(MonitoringHandlerThreadStart, args)
{
	
	//bool bHasSUSI = false;
	bool bHDDNormal = true;
	bool bHWMNormal = true;
	bool bHDDReport = false;
	bool bHWMReport = false;
	char sHDDThresholdMsg[4096];
	char sHWMThresholdMsg[4096];
	int hwmcheckperiod     = 0;
	int hddcheckperiod     = 0;
	int uploadHWMInterval  = 0;
	int uploadHWMTimeout   = 0;
	int uploadHDDInterval  = 0;
	int uploadHDDTimeout   = 0;
	int autoReportInterval = 0;
	int loopInterval       = 100;
	handler_context_t *pHandlerContex = (handler_context_t *)args;

	while(pHandlerContex->isThreadRunning)
	{
		bHDDNormal = true;
		bHWMNormal = true;
		bHDDReport = false;
		bHWMReport = false;
		memset(sHWMThresholdMsg, 0, sizeof(sHWMThresholdMsg));
		memset(sHDDThresholdMsg, 0, sizeof(sHDDThresholdMsg));
		
		app_os_mutex_lock(&g_thrmutex);
		/*Retrieve HWM info*/
		if (pHandlerContex->bHasSUSI)
		{
			if (hwmcheckperiod <= 0)
			{
				//MonitorLog(g_loghandle, Normal, " %s> Get HWM Info", MyTopic);			
				app_os_mutex_lock(&pHandlerContex->hwmctx.hwmmutex);
				hwm_GetHWMInfo(&pHandlerContex->hwmctx.hwminfo);
				bHWMReport = Handler_ChkHWMThrRule(&g_thritems, &pHandlerContex->hwmctx.hwminfo, sHWMThresholdMsg, &bHWMNormal);
				hwmcheckperiod = HWM_CHECK_PERIOD;
				app_os_mutex_unlock(&pHandlerContex->hwmctx.hwmmutex);
			}
			hwmcheckperiod -= loopInterval;
		}
		/*Retrieve HDD info*/
		if (pHandlerContex->bHasSQFlash)
		{
			if (hddcheckperiod <= 0)
			{
				//MonitorLog(g_loghandle, Normal, " %s> Get HDD Info", MyTopic);			
				app_os_mutex_lock(&pHandlerContex->hddctx.hddmutex);
				hdd_GetHDDInfo(&pHandlerContex->hddctx.hddinfo);
				bHDDReport = threshold_ChkHDD(g_thritems.hddRule, &pHandlerContex->hddctx.hddinfo, sHDDThresholdMsg, &bHDDNormal);
				hddcheckperiod = HDD_CHECK_PERIOD;
				app_os_mutex_unlock(&pHandlerContex->hddctx.hddmutex);
			}
			hddcheckperiod -= loopInterval;
		}
		app_os_mutex_unlock(&g_thrmutex);
		

		if (!pHandlerContex->isThreadRunning) break;
		//Send Threshold check result		
		if (bHWMReport || bHDDReport)
		{
			int len = strlen(sHWMThresholdMsg) + strlen(sHDDThresholdMsg) + 8;
			int offset = 0;
			char* message = malloc(len);
			//MonitorLog(g_loghandle, Normal, " %s> Send Threshold check result", MyTopic);
			//MonitorLog(g_loghandle, Normal, " %s>HWM Result [%s]", MyTopic, sHWMThresholdMsg );
			//MonitorLog(g_loghandle, Normal, " %s>HDD Result [%s]", MyTopic, sHDDThresholdMsg );		
			memset(message, 0, len);
			if (bHWMReport) offset += sprintf(message + offset, "%s; ", sHWMThresholdMsg);
			if (bHDDReport) offset += sprintf(message + offset, "%s;", sHDDThresholdMsg);
			Handler_SendThrReport(bHDDNormal && bHWMNormal, message);
			free(message);
		}
		if (!pHandlerContex->isThreadRunning) break;
		

		/*Send Current Monitor value*/
		//MonitorLog(g_loghandle, Normal, " %s> Send Current Monitor value", MyTopic);
		app_os_mutex_lock(&pHandlerContex->hddctx.hddmutex);
		Handler_PeriodReportHDDInfo(&pHandlerContex->hddctx, loopInterval, &uploadHDDInterval, &uploadHDDTimeout);
		app_os_mutex_unlock(&pHandlerContex->hddctx.hddmutex);
		if (!pHandlerContex->isThreadRunning) break;

		app_os_mutex_lock(&pHandlerContex->hwmctx.hwmmutex);
		Handler_PeriodReportHWMInfo(&pHandlerContex->hwmctx, loopInterval, &uploadHWMInterval, &uploadHWMTimeout);
		app_os_mutex_unlock(&pHandlerContex->hwmctx.hwmmutex);
		if (!pHandlerContex->isThreadRunning) break;

		app_os_mutex_lock(&pHandlerContex->reportctx.reportmutex);
		Handler_AutoReport(pHandlerContex, loopInterval, &autoReportInterval);
		app_os_mutex_unlock(&pHandlerContex->reportctx.reportmutex);
		if (!pHandlerContex->isThreadRunning) break;

		time(&g_monitortime);
		app_os_sleep(loopInterval);
		continue;
	}

	app_os_mutex_lock(&pHandlerContex->hwmctx.hwmmutex);
	hwm_ReleaseHWMInfo(&pHandlerContex->hwmctx.hwminfo);
	app_os_mutex_unlock(&pHandlerContex->hwmctx.hwmmutex);

	app_os_mutex_lock(&pHandlerContex->hddctx.hddmutex);
	hdd_DestroyHDDInfoList(pHandlerContex->hddctx.hddinfo.hddMonInfoList);
	free(pHandlerContex->hddctx.hddinfo.hddMonInfoList);
	pHandlerContex->hddctx.hddinfo.hddMonInfoList = NULL;
	pHandlerContex->hddctx.hddinfo.hddCount = 0;
	app_os_mutex_unlock(&pHandlerContex->hddctx.hddmutex);
	//g_PluginInfo.sendcbf(&g_PluginInfo, 0, "{\"hwmInfo\":{\"temp\":{\"cpuT\":43.599998,\"chipsetT\":47.900002,\"systemT\":48.200001,\"cpu2T\":48.500000,\"oem0T\":48.799999,\"oem1T\":49.200001,\"oem2T\":49.500000,\"oem3T\":49.799999,\"oem4T\":40,\"oem5T\":40.400002},\"volt\":{\"vcoreV\":3.525000,\"vcore2V\":3.525000,\"v2_5V\":3.529000,\"v3_3V\":3.532000,\"v5V\":3.535000,\"v12V\":3.538000,\"vsb5V\":3.542000,\"vsb3V\":3.545000,\"vbatV\":3.548000,\"nv5V\":3.551000,\"nv12V\":3.551000,\"vttV\":3.555000,\"v24V\":3.558000,\"oem0V\":3.561000,\"oem1V\":3.564000,\"oem2V\":3.568000,\"oem3V\":3.571000},\"fan\":{\"cpuF\":1800,\"systemF\":1803,\"cpu2F\":1806,\"oem0F\":1809,\"oem1F\":1809,\"oem2F\":1802,\"oem3F\":1805,\"oem4F\":1808,\"oem5F\":1800,\"oem6F\":1804}}}", 634, NULL, NULL);

	return 0;
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
	memset(&g_HandlerContex, 0, sizeof(handler_context_t));

	if( pluginfo == NULL )
		return handler_fail;

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
	MonitorLog(g_loghandle, Normal, " %s> Initialize", strPluginName);
	pluginfo->RequestID = iRequestID;
	pluginfo->ActionID = iActionID;

	// 2. Copy agent info 
	memcpy(&g_PluginInfo, pluginfo, sizeof(HANDLER_INFO));

	// 3. Callback function -> Send JSON Data by this callback function
	g_sendcbf = g_PluginInfo.sendcbf = pluginfo->sendcbf;
	g_sendcustcbf = g_PluginInfo.sendcustcbf = pluginfo->sendcustcbf;
	g_subscribecustcbf = g_PluginInfo.subscribecustcbf = pluginfo->subscribecustcbf;
	g_sendreportcbf = g_PluginInfo.sendreportcbf = pluginfo->sendreportcbf;
	g_sendcapabilitycbf =g_PluginInfo.sendcapabilitycbf = pluginfo->sendcapabilitycbf;
	
	// Create mutex
	if(app_os_mutex_setup(&g_HandlerContex.hwmctx.hwmmutex)!=0)
	{
		MonitorLog(g_loghandle, Normal, " %s> Create HWMInfo mutex failed!", strPluginName);
		return handler_fail;
	}

	if(app_os_mutex_setup(&g_HandlerContex.hwmctx.reportmutex)!=0)
	{
		MonitorLog(g_loghandle, Normal, " %s> Create HWM Report mutex failed!", strPluginName);
		return handler_fail;
	}

	if(app_os_mutex_setup(&g_HandlerContex.hddctx.hddmutex)!=0)
	{
		MonitorLog(g_loghandle, Normal, " %s> Create HDDInfo mutex failed!", strPluginName);
		return handler_fail;
	}

	if(app_os_mutex_setup(&g_HandlerContex.hddctx.reportmutex)!=0)
	{
		MonitorLog(g_loghandle, Normal, " %s> Create HDD Report mutex failed!", strPluginName);
		return handler_fail;
	}

	if(app_os_mutex_setup(&g_HandlerContex.reportctx.reportmutex)!=0)
	{
		MonitorLog(g_loghandle, Normal, " %s> Create Auto Report mutex failed!", strPluginName);
		return handler_fail;
	}

	if(app_os_mutex_setup(&g_thrmutex)!=0)
	{
		MonitorLog(g_loghandle, Normal, " %s> Create Threshold mutex failed!", strPluginName);
		return handler_fail;
	}
	g_status = handler_status_init;
	return handler_success;
}

void Handler_Uninitialize()
{
	Handler_Stop();

	if(g_HandlerContex.reportctx.reqItems)
	{
		cJSON_Delete(g_HandlerContex.reportctx.reqItems);
		g_HandlerContex.reportctx.reqItems = NULL;
	}
	
	// Release mutex
	app_os_mutex_cleanup(&g_HandlerContex.hwmctx.hwmmutex);

	app_os_mutex_cleanup(&g_HandlerContex.hwmctx.reportmutex);

	app_os_mutex_cleanup(&g_HandlerContex.hddctx.hddmutex);

	app_os_mutex_cleanup(&g_HandlerContex.hddctx.reportmutex);

	app_os_mutex_cleanup(&g_HandlerContex.reportctx.reportmutex);

	app_os_mutex_cleanup(&g_thrmutex);

	g_sendcbf           = NULL;
	g_sendcustcbf       = NULL;
	g_sendreportcbf     = NULL;
	g_sendcapabilitycbf = NULL;
	g_subscribecustcbf  = NULL;
}

/* **************************************************************************************
 *  Function Name: Handler_Get_Status
 *  Description: Get Handler Status 
 *  Input :  None
 *  Output: char * : pOutReply ( JSON )
 *  Return:  int  : Length of the status information in JSON format
 *                       :  0 : no data need to trans
 * **************************************************************************************/
int HANDLER_API Handler_Get_Status( HANDLER_THREAD_STATUS * pOutStatus ) // JSON Format
{
	int iRet = handler_fail; 
	
	if(!pOutStatus) return iRet;

	switch (g_status)
	{
	default:
	case handler_status_no_init:
	case handler_status_init:
	case handler_status_stop:
		*pOutStatus = g_status;
		break;
	case handler_status_start:
	case handler_status_busy:
		{
			time_t tv;
			time(&tv);
			if(difftime(tv, g_monitortime)>5)
				g_status = handler_status_busy;
			else
				g_status = handler_status_start;
			*pOutStatus = g_status;
		}
		break;
	}

	iRet = handler_success;
	return iRet;
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
	MonitorLog(g_loghandle, Normal, " %s> Update Status", strPluginName);
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
	MonitorLog(g_loghandle, Normal, " %s> Handler_Start", strPluginName);
	// Init. SUSI
	if(hwm_IsExistSUSILib())
	{
		char name[DEF_HWMNAME_LENGTH];
		char version[DEF_VERSION_LENGTH];

		/*Initialize SUSI*/
		g_HandlerContex.bHasSUSI = hwm_StartupSUSILib();

		/*Retrieve Platform Info*/
		if(g_HandlerContex.bHasSUSI)
		{
			if(hwm_GetPlatformName(name, sizeof(name)))
			{
				g_platformname = (char*) malloc(strlen(name)+1);
				memset(g_platformname, 0, strlen(name)+1);
				strcpy(g_platformname, name);
				MonitorLog(g_loghandle, Normal, " %s> Platform name: %s!", strPluginName, g_platformname);	
			}

			if(hwm_GetBIOSVersion(version, sizeof(version)))
			{
				g_version = (char*) malloc(strlen(version)+1);
				memset(g_version, 0, strlen(version)+1);
				strcpy(g_version, version);
				MonitorLog(g_loghandle, Normal, " %s> Version: %s!", strPluginName, g_version);	
			}

			hwm_GetHWMInfo(&g_HandlerContex.hwmctx.hwminfo);
		}
	}

	// Init. SQFlash
	//MonitorLog(g_loghandle, Normal, " %s> hdd_IsExistSQFlashLib!", MyTopic);	
	if(hdd_IsExistSQFlashLib())
	{
		g_HandlerContex.bHasSQFlash = hdd_StartupSQFlashLib();

		if(g_HandlerContex.bHasSQFlash)
		{
			g_HandlerContex.hddctx.hddinfo.hddMonInfoList = hdd_CreateHDDInfoList();
			app_os_mutex_lock(& g_HandlerContex.hddctx.hddmutex);
			hdd_GetHDDInfo(& g_HandlerContex.hddctx.hddinfo);
			app_os_mutex_unlock(& g_HandlerContex.hddctx.hddmutex);
		}
	}
	//MonitorLog(g_loghandle, Normal, " %s> Load Threshold rule!", MyTopic);	
	/*Load Threshold rule*/
	memset(&g_thritems,0, sizeof(thr_items_t));
	app_os_mutex_lock(&g_thrmutex);
	g_thritems.hwmRule = NULL;
	g_thritems.hddRule = threshold_CreateHDDThrDefaultNode();
	parser_ReadThresholdConfig(g_PluginInfo.WorkDir, &g_thritems);
	threshold_InitHDDThrList(&g_thritems.hddRule, &g_HandlerContex.hddctx.hddinfo);
	app_os_mutex_unlock(&g_thrmutex);

	if (app_os_thread_create(&g_HandlerContex.threadHandler, MonitoringHandlerThreadStart, &g_HandlerContex) != 0)
	{
		g_HandlerContex.isThreadRunning = false;
		MonitorLog(g_loghandle, Normal, " %s> start handler thread failed!", strPluginName);	
		return handler_fail;
	}
	g_HandlerContex.isThreadRunning = true;

	g_status = handler_status_start;
	//MonitorLog(g_loghandle, Normal, " %s> Thread Run!", MyTopic);	
	time(&g_monitortime);

	return handler_success;
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
	MonitorLog(g_loghandle, Normal, " %s> Handler_Stop!", strPluginName);	
	if(g_HandlerContex.isThreadRunning == true)
	{
		//MonitorLog(g_loghandle, Normal, " %s> Wait Trhead stop!", MyTopic);	
		g_HandlerContex.isThreadRunning = false;
		app_os_thread_join(g_HandlerContex.threadHandler);
		g_HandlerContex.threadHandler = NULL;
	}

	if(g_HandlerContex.bHasSUSI)
	{
		/*Release object*/
		//MonitorLog(g_loghandle, Normal, " %s> hwm_CleanupSUSILib!", MyTopic);	
		hwm_CleanupSUSILib();
	}

	if(g_HandlerContex.bHasSQFlash)
	{
		/*Release object*/
		//MonitorLog(g_loghandle, Normal, " %s> hdd_CleanupSQFlashLib!", MyTopic);	
		hdd_CleanupSQFlashLib();
	}

	//MonitorLog(g_loghandle, Normal, " %s> threshold_ClearAllThresholdItem!", MyTopic);	
	app_os_mutex_lock(&g_thrmutex);
	threshold_ClearAllThresholdItem(&g_thritems);
	app_os_mutex_unlock(&g_thrmutex);

	//MonitorLog(g_loghandle, Normal, " %s> Free Platform Name!", MyTopic);
	if(g_platformname)
	{
		free(g_platformname);
		g_platformname = NULL;
	}

	//MonitorLog(g_loghandle, Normal, " %s> Free Version!", MyTopic);
	if(g_version)
	{
		free(g_version);
		g_version = NULL;
	}
	
	//UninitLog(g_loghandle);
	g_status = handler_status_stop;

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
{	int cmdID = 0;
	int replyID =0;
	char errorStr[128] = {0};
	MonitorLog(g_loghandle, Normal, " %s>Recv Topic [%s] Data %s", strPluginName, topic, (char*) data );
	
	if(!parser_ParseReceivedData(data, datalen, &cmdID))
		return;
	switch (cmdID)
	{
	case devmon_set_hwminfo_auto_upload_req:
		{
			int interval = 0;
			int timeout = 0;
			if(!g_HandlerContex.bHasSUSI) 
			{
				//g_PluginInfo.sendcbf(&g_PluginInfo, devmon_set_hwminfo_auto_upload_rep, "{\"result\":\"SUSI no support\"}", strlen("{\"result\":\"SUSI no support\"}"), NULL, NULL);
				if(g_sendcbf)
					g_sendcbf(&g_PluginInfo, devmon_set_hwminfo_auto_upload_rep, "{\"hwmInfo\":\"No support\"}", strlen("{\"hwmInfo\":\"No support\"}"), NULL, NULL);
				//g_PluginInfo.sendcbf(&g_PluginInfo, devmon_set_hwminfo_auto_upload_rep, "{\"hwmInfo\":\"No support\"}", strlen("{\"hwmInfo\":\"No support\"}"), NULL, NULL);
				MonitorLog(g_loghandle, Normal, " %s> SUSI no support", strPluginName);
				return;
			}
			if(parser_ParsePeriodReportCmd(data, datalen, &interval, &timeout))
			{
				app_os_mutex_lock(&g_HandlerContex.hwmctx.reportmutex);
				g_HandlerContex.hwmctx.iPeriodUploadInterval = interval;
				g_HandlerContex.hwmctx.iPeriodUploadTimeout = timeout;
				app_os_mutex_unlock(&g_HandlerContex.hwmctx.reportmutex);
				MonitorLog(g_loghandle, Normal, " %s>Interval: %d Timeout: %d", strPluginName, interval, timeout );
				if(g_sendcbf)
					g_sendcbf(&g_PluginInfo, devmon_set_hwminfo_auto_upload_rep, "{\"hwmInfo\":\"SUCCESS\"}", strlen("{\"hwmInfo\":\"SUCCESS\"}"), NULL, NULL);
				//g_PluginInfo.sendcbf(&g_PluginInfo, devmon_set_hwminfo_auto_upload_rep, "{\"hwmInfo\":\"SUCCESS\"}", strlen("{\"hwmInfo\":\"SUCCESS\"}"), NULL, NULL);
			}
			else
			{
				memset(errorStr, 0, sizeof(errorStr));
				sprintf(errorStr, "Command(%d) params error!", devmon_set_hwminfo_auto_upload_req);
				if(g_sendcbf)
					g_sendcbf(&g_PluginInfo, devmon_error_rep, errorStr, strlen(errorStr), NULL, NULL);
				//g_PluginInfo.sendcbf(&g_PluginInfo, devmon_error_rep, errorStr, strlen(errorStr), NULL, NULL);
			}
		}
		break;
	case devmon_hwminfo_req:
		{
			if(!g_HandlerContex.bHasSUSI)
			{
				//g_PluginInfo.sendcbf(&g_PluginInfo, devmon_set_hwminfo_reqp_rep, "{\"result\":\"SUSI no support\"}", strlen("{\"result\":\"SUSI no support\"}"), NULL, NULL);
				if(g_sendcbf)
					g_sendcbf(&g_PluginInfo, devmon_set_hwminfo_reqp_rep, "{\"hwmInfo\":\"No support\"}", strlen("{\"hwmInfo\":\"No support\"}"), NULL, NULL);
				//g_PluginInfo.sendcbf(&g_PluginInfo, devmon_set_hwminfo_reqp_rep, "{\"hwmInfo\":\"No support\"}", strlen("{\"hwmInfo\":\"No support\"}"), NULL, NULL);
				MonitorLog(g_loghandle, Normal, " %s>SUSI no support", strPluginName);
				return;
			}
			else
			{
				app_os_mutex_lock(&g_HandlerContex.hwmctx.reportmutex);
				//g_HandlerContex.hwmctx.bReportOnce = true;
				g_HandlerContex.hwmctx.iPeriodUploadInterval = 0;
				g_HandlerContex.hwmctx.iPeriodUploadTimeout = 0;
				app_os_mutex_unlock(&g_HandlerContex.hwmctx.reportmutex);

				if(g_sendcbf)
					g_sendcbf(&g_PluginInfo,devmon_set_hwminfo_reqp_rep, "{\"hwmInfo\":\"SUCCESS\"}", strlen("{\"hwmInfo\":\"SUCCESS\"}"), NULL, NULL);
				//g_PluginInfo.sendcbf(&g_PluginInfo,devmon_set_hwminfo_reqp_rep, "{\"hwmInfo\":\"SUCCESS\"}", strlen("{\"hwmInfo\":\"SUCCESS\"}"), NULL, NULL);
			}
		}
		break;
	case devmon_set_hddinfo_auto_upload_req:
		{
			int interval = 0;
			int timeout = 0;
			if(!g_HandlerContex.bHasSQFlash)
			{
				//g_PluginInfo.sendcbf(&g_PluginInfo, devmon_set_hddinfo_auto_upload_rep, "{\"result\":\"HDD S.M.A.R.T no support\"}", strlen("{\"result\":\"HDD S.M.A.R.T no support\"}"), NULL, NULL);
				if(g_sendcbf)
					g_sendcbf(&g_PluginInfo, devmon_set_hddinfo_auto_upload_rep, "{\"hddInfo\":\"No support\"}", strlen("{\"hddInfo\":\"No support\"}"), NULL, NULL);
				//g_PluginInfo.sendcbf(&g_PluginInfo, devmon_set_hddinfo_auto_upload_rep, "{\"hddInfo\":\"No support\"}", strlen("{\"hddInfo\":\"No support\"}"), NULL, NULL);
				MonitorLog(g_loghandle, Normal, " %s> HDD S.M.A.R.T no support", strPluginName);
				return;
			}
			if(parser_ParsePeriodReportCmd(data, datalen, &interval, &timeout))
			{
				app_os_mutex_lock(&g_HandlerContex.hddctx.reportmutex);
				g_HandlerContex.hddctx.iPeriodUploadInterval = interval;
				g_HandlerContex.hddctx.iPeriodUploadTimeout = timeout;
				app_os_mutex_unlock(&g_HandlerContex.hddctx.reportmutex);
				MonitorLog(g_loghandle, Normal, " %s>Interval: %d Timeout: %d", strPluginName, interval, timeout );
				if(g_sendcbf)
					g_sendcbf(&g_PluginInfo, devmon_set_hddinfo_auto_upload_rep, "{\"hddInfo\":\"SUCCESS\"}", strlen("{\"hddInfo\":\"SUCCESS\"}"), NULL, NULL);
				//g_PluginInfo.sendcbf(&g_PluginInfo, devmon_set_hddinfo_auto_upload_rep, "{\"hddInfo\":\"SUCCESS\"}", strlen("{\"hddInfo\":\"SUCCESS\"}"), NULL, NULL);
			}
			else
			{
				memset(errorStr, 0, sizeof(errorStr));
				sprintf(errorStr, "{\"result\":\"Command(%d) params error!\"}", devmon_set_hddinfo_auto_upload_req);
				if(g_sendcbf)
					g_sendcbf(&g_PluginInfo, devmon_error_rep, errorStr, strlen(errorStr), NULL, NULL);
				//g_PluginInfo.sendcbf(&g_PluginInfo, devmon_error_rep, errorStr, strlen(errorStr), NULL, NULL);
			}
		}
		break;
	case devmon_hddinfo_req:
		{
			if(!g_HandlerContex.bHasSQFlash)
			{
				if(g_sendcbf)
					g_sendcbf(&g_PluginInfo, devmon_hddinfo_rep, "{\"hddInfo\":\"No support\"}", strlen("{\"hddInfo\":\"No support\"}"), NULL, NULL);
				//g_PluginInfo.sendcbf(&g_PluginInfo, devmon_hddinfo_rep, "{\"hddInfo\":\"No support\"}", strlen("{\"hddInfo\":\"No support\"}"), NULL, NULL);
				MonitorLog(g_loghandle, Normal, " %s>HDD S.M.A.R.T no support", strPluginName);
				return;
			}
			else
			{
				cJSON* hddinfoJSON = NULL;
				char* hddinfo = NULL;

				hddinfoJSON = parser_CreateHDDInfo(&g_HandlerContex.hddctx.hddinfo);
				hddinfo = cJSON_PrintUnformatted(hddinfoJSON);
				cJSON_Delete(hddinfoJSON);
				MonitorLog(g_loghandle, Normal, " %s>Platform Info: %s", strPluginName, hddinfo);

				if(g_sendcbf)
					g_sendcbf(&g_PluginInfo, devmon_hddinfo_rep, hddinfo, strlen(hddinfo), NULL, NULL);
				//g_PluginInfo.sendcbf(&g_PluginInfo, devmon_hddinfo_rep, hddinfo, strlen(hddinfo), NULL, NULL);
				free(hddinfo);
			}
		}
		break;
	case devmon_get_pfminfo_req:
		{
			cJSON* platforminfoJSON = NULL;
			char* platforminfo = NULL;
			
			if(!g_HandlerContex.bHasSUSI)
			{
				if(g_sendcbf)
					g_sendcbf(&g_PluginInfo, devmon_get_pfminfo_rep, "{\"pfInfo\":\"No support\"}", strlen("{\"pfInfo\":\"No support\"}"), NULL, NULL);
				//g_PluginInfo.sendcbf(&g_PluginInfo, devmon_get_pfminfo_rep, "{\"pfInfo\":\"No support\"}", strlen("{\"pfInfo\":\"No support\"}"), NULL, NULL);
				MonitorLog(g_loghandle, Normal, " %s>SUSI no support", strPluginName);
				return;
			}

			platforminfoJSON = parser_CreatePlatformInfo(&g_HandlerContex.hwmctx.hwminfo);
			platforminfo = cJSON_PrintUnformatted(platforminfoJSON);
			cJSON_Delete(platforminfoJSON);
			MonitorLog(g_loghandle, Normal, " %s>Platform Info: %s", strPluginName, platforminfo);
			
			if(g_sendcbf)
				g_sendcbf(&g_PluginInfo, devmon_get_pfminfo_rep, platforminfo, strlen(platforminfo), NULL, NULL);
			//g_PluginInfo.sendcbf(&g_PluginInfo, devmon_get_pfminfo_rep, platforminfo, strlen(platforminfo), NULL, NULL);
			free(platforminfo);
		}
		break;
	case devmon_del_all_thr_req:
			g_bTrace = true;
			if (! replyID)   replyID = devmon_del_all_thr_rep;    //just use the code  following, only set the correct replyID :)
	case devmon_set_thr_req:
		{
			bool bRet = true;
			char repMsg[128] = {0};
			char resetMsg[4096] = { 0 };
			thr_items_t thritems;
			g_bTrace = true;
			if (! replyID)   replyID = devmon_set_thr_rep;
			if (!parser_WriteThresholdConfig(g_PluginInfo.WorkDir, data)) {
				printf("Failed to write threshold config file, new threshold rules are not set. \r\n ");
				return;
			}
			/*sync threshold rules*/
			memset(&thritems, 0, sizeof(thr_items_t));
			thritems.total = 0;
			thritems.hwmRule = NULL;
			thritems.hddRule = threshold_CreateHDDThrDefaultNode();

			if ( ! parser_ParseThresholdCmd( data, datalen, & thritems ) && replyID == devmon_set_thr_req) {
				printf("Failed to parse Threshold Command, rules remain not changed. \r\n ");
				threshold_ClearAllThresholdItem(&thritems);
				return;
			}
			app_os_mutex_lock(& g_thrmutex);
			if (g_HandlerContex.bHasSUSI)    {
				bRet = threshold_SyncHWMRule(& thritems, & g_thritems, resetMsg);
			}
			if (g_HandlerContex.bHasSQFlash) {
				app_os_mutex_lock(& g_HandlerContex.hddctx.hddmutex);
				bRet &= threshold_SyncHDDRule(& thritems.hddRule, & g_thritems.hddRule, & g_HandlerContex.hddctx.hddinfo, resetMsg);
				app_os_mutex_unlock(& g_HandlerContex.hddctx.hddmutex);
			}
			app_os_mutex_unlock(& g_thrmutex);
			
			switch (replyID)
			{
			default:
			case devmon_set_thr_rep: 
				if (bRet) sprintf(repMsg, "{\"hwmSetThresholdResult\":\"%s\"}", "HWM threshold apply OK!");
				else      sprintf(repMsg, "{\"hwmSetThresholdResult\":\"%s\"}", "HWM threshold apply failed!");
				break;
			case devmon_del_all_thr_rep:
				if (bRet) sprintf(repMsg, "{\"delAllThresholdRep\":\"Delete all threshold successfully!\"}");
				else      sprintf(repMsg, "{\"delAllThresholdRep\":\"Delete all threshold failed!\"}");
				//sprintf(repMsg,"{\"delAllThrRep\":\"Delete all threshold successfully!\"}");
				break;
			}
			if (g_sendcbf) {
				g_sendcbf(&g_PluginInfo, replyID, repMsg, strlen(repMsg)+1, NULL, NULL);
			}

			/* ==todo: Check ThresholdRule at once and build*/
			if (resetMsg[0])
			{
				char sHWMMsg[2048] = { 0 };
				char sHDDMsg[1024] = { 0 };
				bool bHWMNormal = true;
				bool bHDDNormal = true;
				bool bHWMReport = false;
				bool bHDDReport = false;

				/*Retrieve & check HWM info*/
				app_os_mutex_lock(&g_thrmutex);
				if (g_HandlerContex.bHasSUSI)
				{
					app_os_mutex_lock(&g_HandlerContex.hwmctx.hwmmutex);
					hwm_GetHWMInfo(&g_HandlerContex.hwmctx.hwminfo);
					bHWMReport = Handler_ChkHWMThrRule(&g_thritems, &g_HandlerContex.hwmctx.hwminfo, sHWMMsg, &bHWMNormal);	
					app_os_mutex_unlock(&g_HandlerContex.hwmctx.hwmmutex);
				}
				/*Retrieve & check HDD info*/
				if (g_HandlerContex.bHasSQFlash)
				{
					app_os_mutex_lock(&g_HandlerContex.hddctx.hddmutex);
					hdd_GetHDDInfo(&g_HandlerContex.hddctx.hddinfo);
					bHDDReport = threshold_ChkHDD(g_thritems.hddRule, &g_HandlerContex.hddctx.hddinfo, sHDDMsg, &bHDDNormal);
					app_os_mutex_unlock(&g_HandlerContex.hddctx.hddmutex);
				}
				app_os_mutex_unlock(&g_thrmutex);

				//Send Threshold check result
				{
					int len=strlen(sHWMMsg)+strlen(sHDDMsg)+strlen(resetMsg)+8;
					int offset = 0;
					char* message = malloc(len);
					memset(message, 0, len);
					offset += sprintf(message + offset, "%s; ", resetMsg);
					if (bHWMReport) offset += sprintf(message + offset, "%s; ", sHWMMsg);
					if (bHDDReport) offset += sprintf(message + offset, "%s; ", sHDDMsg);
					app_os_sleep(50);
					if (message[0]) Handler_SendThrReport(bHWMNormal && bHDDNormal, message);
					free(message);
				}
			} // end if (resetMsg[0])			
			//====================
			threshold_ClearAllThresholdItem(&thritems);
			g_bTrace = false;
			break;
		}
	case devmon_get_hdd_smart_info_req:
		{
			GetHddSmartInfo();
			break;
		}
	default: break;
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
#if 1
#else
	int interval = 0; //sec
	cJSON* reqItems = cJSON_CreateObject();
	bool bRet = parser_ParseAutoUploadCmd(pInQuery, strlen(pInQuery), &interval, reqItems);
	if(bRet)
	{
		app_os_mutex_lock(&g_HandlerContex.reportctx.reportmutex);
		g_HandlerContex.reportctx.iAutoReportInterval = interval*1000;
		if(g_HandlerContex.reportctx.reqItems != NULL)
			cJSON_Delete(g_HandlerContex.reportctx.reqItems);
		g_HandlerContex.reportctx.reqItems = reqItems;
		app_os_mutex_unlock(&g_HandlerContex.reportctx.reportmutex);
	}
	else
	{
		cJSON_Delete(reqItems);
	}
#endif
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
#if 1
#else
	int interval = 0; //sec
	cJSON* reqItems = cJSON_CreateObject();
	bool bRet = true;
	if(pInQuery)
		bRet = parser_ParseAutoUploadCmd(pInQuery, strlen(pInQuery), &interval, reqItems);
	if(bRet)
	{
		cJSON_Delete(reqItems);
		app_os_mutex_lock(&g_HandlerContex.reportctx.reportmutex);
		g_HandlerContex.reportctx.iAutoReportInterval = 0;
		if(g_HandlerContex.reportctx.reqItems)
		{
			cJSON_Delete(g_HandlerContex.reportctx.reqItems);
			g_HandlerContex.reportctx.reqItems = NULL;
		}
		app_os_mutex_unlock(&g_HandlerContex.reportctx.reportmutex);
	}
#endif
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
#if 1
#else
	cJSON *platforminfoJSON = NULL;
	char *platforminfo = NULL;

	//MonitorLog(g_loghandle, Normal, " %s> Handler_Get_Capability!", MyTopic);

	platforminfoJSON = parser_CreateInfoSpec(&g_HandlerContex.hwmctx.hwminfo, &g_HandlerContex.hddctx.hddinfo);

	//MonitorLog(g_loghandle, Normal, " %s> parser_CreateInfoSpec!", MyTopic);

	platforminfo = cJSON_PrintUnformatted(platforminfoJSON);

	//MonitorLog(g_loghandle, Normal, " %s> cJSON_PrintUnformatted!", MyTopic);

	cJSON_Delete(platforminfoJSON);

	if(pOutReply != NULL)
	{
		len = strlen(platforminfo);
		*pOutReply = (char *)malloc(len + 1);
		memset(*pOutReply, 0, len + 1);
		strcpy(*pOutReply, platforminfo);
		free(platforminfo);
	}
#endif
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