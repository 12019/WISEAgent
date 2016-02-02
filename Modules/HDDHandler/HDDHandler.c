/****************************************************************************/
/* Copyright( C ) : Advantech Technologies, Inc.								*/
/* Create Date  : 2014/07/07 by Scott68 Chang								*/
/* Modified Date: 2015/02/12 by tang.tao							*/
/* Abstract     : Handler API                                     			*/
/* Reference    : None														*/
/*****************************************************************************/

#include "platform.h"
#include "susiaccess_handler_api.h"
#include "HDDHandler.h"
#include "HDDLog.h"
//#include <vld.h>


const int RequestID = cagent_request_hdd_monitoring;
const int ActionID = cagent_action_hdd_monitoring;

//-----------------------------------------------------------------------------
// Variables
//-----------------------------------------------------------------------------
static void *                  g_LogHandle = NULL;
static bool                    g_bEnableLog = true;
static Handler_info            g_PluginInfo;
static HANDLER_THREAD_STATUS   g_Status = handler_status_no_init;
static time_t                  g_MonitorTime;

static handler_context_t       g_HandlerContex;
static HandlerSendCbf          g_sendcbf = NULL;       // Client Send information (in JSON format) to Cloud Server	
static HandlerSendCustCbf      g_sendcustcbf = NULL;   // Client Send information (in JSON format) to Cloud Server with custom topic	
static HandlerAutoReportCbf    g_sendreportcbf = NULL; // Client Send report (in JSON format) to Cloud Server with AutoReport topic	
static HandlerSendCapabilityCbf  g_sendcapabilitycbf = NULL;
static HandlerSubscribeCustCbf g_subscribecustcbf = NULL;

//#define MEM_LEAK_CHECK_HDD
#ifdef MEM_LEAK_CHECK_HDD
	#define _CRTDBG_MAP_ALLOC  
	#include <stdlib.h>  
	#include <crtdbg.h>
	static bool bMemDetail = false;
	static bool bMemCheck[100] = { 1 };
	static _CrtMemState memSnapA[100];
	static _CrtMemState memSnapB[100];
	static _CrtMemState memDiff;
	#define MEM_SNAP_START(snapIndex) { _CrtMemCheckpoint( & memSnapA[snapIndex]);}
	#define MEM_SNAP_STOP(snapIndex)  {	_CrtMemCheckpoint( & memSnapB[snapIndex]);						\
			if ( _CrtMemDifference( & memDiff, & memSnapA[snapIndex], & memSnapB[snapIndex]) ){			\
			_CrtMemDumpStatistics( & memDiff );															\
			if (bMemDetail) _CrtDumpMemoryLeaks(); }}
#else
	#define MEM_SNAP_START(snapIndex)
	#define MEM_SNAP_STOP(snapIndex)
#endif



//-----------------------------------------------------------------------------
// UTIL Function:
//-----------------------------------------------------------------------------
#ifdef _MSC_VER
BOOL WINAPI DllMain( HINSTANCE module_handle, DWORD reason_for_call, LPVOID reserved )
{
	if ( reason_for_call == DLL_PROCESS_ATTACH ) // Self-explanatory
	{
		printf( "DllInitializer\n" );
		DisableThreadLibraryCalls( module_handle ); // Disable DllMain calls for DLL_THREAD_*
		if ( reserved == NULL ) // Dynamic load
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

	if ( reason_for_call == DLL_PROCESS_DETACH ) // Self-explanatory
	{
		printf( "DllFinalizer\n" );
		if ( reserved == NULL ) // Either loading the DLL has failed or FreeLibrary was called
		{
			// Cleanup
			Handler_Uninitialize( );
		}
		else // Process is terminating
		{
			// Cleanup
			Handler_Uninitialize( );
		}
	}
	return TRUE;
}
#else
__attribute__( ( constructor ) )
/**
 * initializer of the shared lib.
 */
static void Initializer( int argc, char** argv, char** envp )
{
    printf( "DllInitializer\n" );
}

__attribute__( ( destructor ) )
/** 
 * It is called when shared lib is being unloaded.
 * 
 */
static void Finalizer( )
{
    printf( "DllFinalizer\n" );
	Handler_Uninitialize( );
}
#endif


/* **************************************************************************************
 *  Function Name: Handler_Initialize
 *  Description: Init any objects or variables of this handler
 *  Input :  HANDLER_INFO *pluginfo
 *  Output: None
 *  Return:  0  : Success Init Handler
 *              -1 : Fail Init Handler
 * ***************************************************************************************/
int HANDLER_API Handler_Initialize( HANDLER_INFO *pluginfo )
{
	memset ( & g_HandlerContex, 0, sizeof( handler_context_t ) );
	if ( pluginfo == NULL ) return handler_fail;
	if ( g_bEnableLog ) {
		g_LogHandle = pluginfo->loghandle;
	}
	
	 /*1-> Topic of this handler*/
	snprintf( pluginfo->Name, sizeof( pluginfo->Name ), "%s", MyTopic );
	pluginfo->RequestID = RequestID;
	pluginfo->ActionID = ActionID;

	/*2-> Copy agent info */
	memcpy( & g_PluginInfo, pluginfo, sizeof( HANDLER_INFO ) );

	// 3-> Callback function->Send JSON Data by this callback function
	g_sendcbf          = g_PluginInfo.sendcbf          = pluginfo->sendcbf;
	g_sendcustcbf      = g_PluginInfo.sendcustcbf      = pluginfo->sendcustcbf;
	g_subscribecustcbf = g_PluginInfo.subscribecustcbf = pluginfo->subscribecustcbf;
	g_sendreportcbf    = g_PluginInfo.sendreportcbf    = pluginfo->sendreportcbf;
	g_sendcapabilitycbf= g_PluginInfo.sendcapabilitycbf= pluginfo->sendcapabilitycbf;
	
	/*Create mutex*/
	if ( app_os_mutex_setup( & g_HandlerContex.hddCtx.hddMutex ) != 0 ) {
		HDDLog( g_LogHandle, Normal, " %s> Create HDDInfo mutex failed! ", MyTopic );
		return handler_fail;
	}
	if ( app_os_mutex_setup( & g_HandlerContex.thrCtx.thrMutex ) != 0 ) {
		HDDLog( g_LogHandle, Normal, " %s> Create Threshold mutex failed! ", MyTopic );
		return handler_fail;
	}
	if ( app_os_mutex_setup( & g_HandlerContex.reportCtx.reportMutex) != 0) {
		HDDLog(g_LogHandle, Normal, " %s> Create HDD Report mutex failed! ", MyTopic);
		return handler_fail;
	}
	if (app_os_mutex_setup(&g_HandlerContex.uploadCtx.reportMutex) != 0) {
		HDDLog( g_LogHandle, Normal, " %s> Create Upload mutex failed! ", MyTopic );
		return handler_fail;
	}
	g_Status = handler_status_init;
	return handler_success;
}

int Handler_Uninitialize( )
{
	Handler_Stop( );
	// Release mutex
	app_os_mutex_cleanup( & g_HandlerContex.hddCtx.hddMutex );
	app_os_mutex_cleanup( & g_HandlerContex.thrCtx.thrMutex );
	app_os_mutex_cleanup( & g_HandlerContex.reportCtx.reportMutex);
	app_os_mutex_cleanup( & g_HandlerContex.uploadCtx.reportMutex);

	return 0;
}


void SendHDDReport( report_context_t * reportCtx )
{
	cJSON * pCustomRoot = NULL;
	char * pInfo = NULL;
	bool bResult = false;

	// === Auto report the whole InfoSpec ===
	if (! reportCtx->pRequestPathArray)  {
		HDDLog(g_LogHandle, Normal, "%s>request is NULL, send nothing!  %s\r\n", MyTopic, pInfo);
		//pInfo = cJSON_PrintUnformatted(pMonitor);
		//if ( g_sendreportcbf )  g_sendreportcbf( & g_PluginInfo, pInfo, strlen( pInfo ) + 1, NULL, NULL );
		//HDDLog(g_LogHandle, Normal, "%s>Send HDD full auto report --> %s\r\n", MyTopic, pInfo);
	}

	// === Auto Report the parts of InfoSpec as pre-customed
	if ( reportCtx->pRequestPathArray && reportCtx->pRequestPathArray->child )
	{
		char errMsg[1024] = {0};
		app_os_mutex_lock( & g_HandlerContex.hddCtx.hddMutex );
		bResult = Parser_CreateCustomReportOverall(&pCustomRoot, g_HandlerContex.hddCtx.pMonitor, reportCtx->pRequestPathArray, errMsg);
		app_os_mutex_unlock( & g_HandlerContex.hddCtx.hddMutex );
		if ( ! bResult ) {
			HDDLog(g_LogHandle, Error, " [%s] occure error when creating custom report. [Detail]-->:%s\r\n", MyTopic, errMsg);
			//memset(errMsg,0,sizeof(errMsg));
			//app_os_mutex_lock( & g_HandlerContex.hddCtx.hddMutex );
			//bResult = Parser_CreateCustomReportOverall(&pCustomRoot, g_HandlerContex.hddCtx.pMonitor, reportCtx->pRequestPathArray, errMsg);
			//app_os_mutex_unlock( & g_HandlerContex.hddCtx.hddMutex );
		}
		else if ( pCustomRoot )
		{	
			pInfo = cJSON_PrintUnformatted(pCustomRoot);
			if (reportCtx == & g_HandlerContex.reportCtx ) {
				if (g_sendreportcbf) 
					g_sendreportcbf( & g_PluginInfo, pInfo, strlen(pInfo) + 1, NULL, NULL);
				HDDLog(g_LogHandle, Normal, "%s>Send auto report OK.\r\n", MyTopic);
			}
			if (reportCtx == & g_HandlerContex.uploadCtx) {
				if (g_sendcbf)
					g_sendcbf( &g_PluginInfo, devmon_auto_upload_rep, pInfo, strlen(pInfo) + 1, NULL, NULL);
				HDDLog(g_LogHandle, Normal, "%s>Send auto upload OK.\r\n", MyTopic);
			}
			// === Free momory ===
			cJSON_Delete(pCustomRoot);
			free(pInfo);
		}
	}

	

	return;
}

/************************************************************************/
/* description:determine whether to repeat send hdd_info   it will run in another thread as a repeat timer,
input:  interval:  the repeat interval of the thread which will call this function
        reportCD:  the Report Timer of auto report, as the server`s wish. It works as a countdown timer */
/************************************************************************/
void PlanSendHDDReport( report_context_t * pReportCtx, int checkInterval, int * checkCDTimer )
{

	if ( NULL == checkCDTimer ) return;
	if ( ! g_HandlerContex.bHasSQFlash ) return;
	
	if (pReportCtx->bEnable) {
		pReportCtx->iTimeoutMs -= checkInterval;
		*checkCDTimer          -= checkInterval;	
		if ( *checkCDTimer <= 0 ) {
			SendHDDReport(pReportCtx);
			*checkCDTimer = pReportCtx->iIntervalMs;
		}
		if (pReportCtx->iTimeoutMs < 0 && pReportCtx == &g_HandlerContex.uploadCtx) {
			pReportCtx->bEnable = false;
		}
	}
	return;
}


void SendTHRReport( bool bResult,  char* thrMsg )
{
	cJSON* pRoot = NULL;
	char* pInfo = NULL;
	/*send HWM value*/
	pRoot = Parser_CreateThrChkRet( bResult, thrMsg );
	pInfo = cJSON_PrintUnformatted( pRoot );	
	if( g_sendcbf )
		g_sendcbf( &g_PluginInfo, devmon_report_rep, pInfo, strlen(pInfo) + 1 , NULL, NULL );
	HDDLog(g_LogHandle, Normal, " %s>Send HDD Threshold Info: %s\r\n", MyTopic, pInfo);
	cJSON_Delete(pRoot);
	free(pInfo);
	return;
}

static CAGENT_PTHREAD_ENTRY(Handler_MemLeakTest,args)
{
#ifdef MEM_LEAK_CHECK_HDD
	int i = 0;
	int j = 0;
	int k = 0;
	FILE *fp1 = NULL;
	FILE *fp2 = NULL;
	char * thrCmd1 = NULL;
	char * thrCmd2 = NULL;
	char * memTemp = NULL;
	int len1 = 0;
	int len2 = 0;
	unsigned int Len = 0;
	char fullPath1[MAX_PATH] = "E:\\HDDThreshold1.txt" ;
	char fullPath2[MAX_PATH] = "E:\\HDDThreshold2.txt" ;
	cJSON * pReqRoot = NULL;
	cJSON * body = NULL;
	cJSON * target = NULL;



	if ((fp1 = fopen(fullPath1, "rb")) == NULL) return NULL;
	if ((fp2 = fopen(fullPath2, "rb")) == NULL) return NULL;
	fseek(fp1, 0, SEEK_END);
	fseek(fp2, 0, SEEK_END);
	len1 = ftell(fp1) + 1;
	len2 = ftell(fp2) + 1;
	thrCmd1 = (char *)malloc(ftell(fp1) + 1);
	thrCmd2 = (char *)malloc(ftell(fp2) + 1);
	memset(thrCmd1, 0, len1);
	memset(thrCmd2, 0, len2);
	fseek(fp1, 0, SEEK_SET);
	fseek(fp2, 0, SEEK_SET);
	fread(thrCmd1, sizeof(char), len1, fp1);
	fread(thrCmd2, sizeof(char), len2, fp2);
	fclose(fp1);
	fclose(fp2);
	Handler_Recv(NULL,thrCmd1,len1,NULL,NULL);
	app_os_sleep(15000);
	while (1)
	{
		printf("=========[HDDHandler]===================== UnitTest\n");
		Handler_Recv(NULL,thrCmd1,len1,NULL,NULL);
		app_os_sleep(10);
		Handler_Recv(NULL,thrCmd2,len2,NULL,NULL);
	}
	
	if (thrCmd1) free(thrCmd1);
	if (thrCmd2) free(thrCmd2);
#endif
}


static CAGENT_PTHREAD_ENTRY( HDDHandlerThreadStart, args )
{
	bool bThrNormal = false;
	bool bThrReport = false;
	char *pRepThrMsg = NULL;

	int interval      = 100;
	int iRefreshTimer = 0;
	int iReportTimer  = 0;
	int iUploadTimer  = 0;

	handler_context_t * pHandlerCtx = ( handler_context_t * )args;
	hdd_info_t *           pHDDInfo = &( pHandlerCtx->hddCtx.hddInfo  );
	cJSON **              ppMonitor = &( pHandlerCtx->hddCtx.pMonitor );
	hdd_thr_list_t *       pThrList = &( pHandlerCtx->thrCtx.thrList  );
	int                      hddCnt = pHDDInfo->hddCount;

	//===Start to AutoRun HDD Info query method
	while( pHandlerCtx->bThreadRunning )
	{
		app_os_sleep( interval );
		//continue; // for test, remember to delete it
		if ( g_HandlerContex.bHasSQFlash )
		{
			if ( iRefreshTimer > 0 )  iRefreshTimer -= interval;			
			else 
			{
				/*===Retrieve HDD info && create JSON object of infoSpec ===*/
				app_os_mutex_lock ( & pHandlerCtx->hddCtx.hddMutex );
				hdd_GetHDDInfo(pHDDInfo);
				if ( * ppMonitor) {
					cJSON_Delete(* ppMonitor);
					* ppMonitor = NULL;
				}
				* ppMonitor = Parser_CreateInfoSpec(pHDDInfo);
				
				/*=== Check HDDCount changed and reset the new infoSpec */
				if (hddCnt != pHDDInfo->hddCount && g_sendcapabilitycbf) {		
					char * pInfoSpec = cJSON_PrintUnformatted(*ppMonitor);
					g_sendcapabilitycbf( & g_PluginInfo, pInfoSpec, sizeof(pInfoSpec) + 1, NULL, NULL);
					if (pInfoSpec) free(pInfoSpec);	
				}
				app_os_mutex_unlock(& pHandlerCtx->hddCtx.hddMutex );
				
				/*===Check Threshold Rule===*/		
				app_os_mutex_lock( & g_HandlerContex.thrCtx.thrMutex );
				bThrReport = Threshold_ChkHDDThrList(* pThrList, * ppMonitor, & bThrNormal, & pRepThrMsg);
				app_os_mutex_unlock( & g_HandlerContex.thrCtx.thrMutex );
				if ( ! pHandlerCtx->bThreadRunning ) break;
				if (bThrReport) {
					SendTHRReport(bThrNormal, pRepThrMsg);
					free(pRepThrMsg);
				}
				iRefreshTimer = HDD_CHECK_PERIOD;
			} 


			if (* ppMonitor) {
				//===Send auto-Report ===
				app_os_mutex_lock  (& pHandlerCtx->reportCtx.reportMutex );
				PlanSendHDDReport  (& pHandlerCtx->reportCtx, interval, & iReportTimer );
				app_os_mutex_unlock(& pHandlerCtx->reportCtx.reportMutex );

				//===Send auto-Upload ===
				app_os_mutex_lock  (& pHandlerCtx->uploadCtx.reportMutex);
				PlanSendHDDReport  (& pHandlerCtx->uploadCtx, interval, & iUploadTimer);
				app_os_mutex_unlock(& pHandlerCtx->uploadCtx.reportMutex);
			}

		} // if bHasSQFlash End

		time( & g_MonitorTime );
	}

	//== free memory of hddCtx
	app_os_mutex_lock( & pHandlerCtx->hddCtx.hddMutex );
	hdd_DestroyHDDInfoList( pHDDInfo->hddMonInfoList );
	free(pHDDInfo->hddMonInfoList);
	pHDDInfo->hddMonInfoList = NULL;
	pHDDInfo->hddCount = 0;
	if (* ppMonitor) {
		cJSON_Delete(* ppMonitor);
		* ppMonitor = NULL;
	}
	app_os_mutex_unlock( & pHandlerCtx->hddCtx.hddMutex );

	//== free memory of thrCtx
	app_os_mutex_lock( & g_HandlerContex.thrCtx.thrMutex );
	Threshold_DestroyHDDThrList( * pThrList );
	* pThrList = NULL;
	app_os_mutex_unlock( & g_HandlerContex.thrCtx.thrMutex );

	//== free memory of reportCtx
	app_os_mutex_lock( & g_HandlerContex.reportCtx.reportMutex);
	if (g_HandlerContex.reportCtx.pRequestPathArray)
		cJSON_Delete( g_HandlerContex.reportCtx.pRequestPathArray );
	app_os_mutex_unlock( & g_HandlerContex.reportCtx.reportMutex );

	////== free memory of uploadCtx
	app_os_mutex_lock( & g_HandlerContex.uploadCtx.reportMutex);
	if (g_HandlerContex.uploadCtx.pRequestPathArray)
		cJSON_Delete( g_HandlerContex.uploadCtx.pRequestPathArray );
	app_os_mutex_unlock( & g_HandlerContex.uploadCtx.reportMutex );
	
	return 0;
}


/* **************************************************************************************
 *  Function Name: Handler_Start
 *  Description: Start Running
 *  Input :  None
 *  Output: None
 *  Return:  0 : Success
 *          -1 : Failed
 * ***************************************************************************************/
int HANDLER_API Handler_Start( void )
{

#ifdef MEM_LEAK_CHECK_HDD
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
	//bMemDetail = false;
	//bMemCheck[1] = true; 

#endif

	//== Init SQFlash
	app_os_mutex_lock( & g_HandlerContex.hddCtx.hddMutex );
	app_os_mutex_lock( & g_HandlerContex.thrCtx.thrMutex );

	if ( hdd_IsExistSQFlashLib( ) ) {
		g_HandlerContex.bHasSQFlash = hdd_StartupSQFlashLib( );
		if ( g_HandlerContex.bHasSQFlash ) {
			hdd_GetHDDInfo(& g_HandlerContex.hddCtx.hddInfo );
			g_HandlerContex.hddCtx.pMonitor = Parser_CreateInfoSpec( & g_HandlerContex.hddCtx.hddInfo );
		}
	}
	
	//== Init Threshold rule
	g_HandlerContex.thrCtx.thrList = Threshold_CreateHDDThrList(&g_HandlerContex.hddCtx.hddInfo);
	//{
	//	cJSON * hddThrArray = NULL;
	//	hddThrArray = Threshold_ReadHDDThrCfg(g_PluginInfo.WorkDir);
	//	if (hddThrArray) {
	//		Threshold_LoadALLRules( g_HandlerContex.thrCtx.thrList, hddThrArray);
	//		cJSON_Delete( hddThrArray );
	//	}
	//}
	app_os_mutex_unlock( & g_HandlerContex.thrCtx.thrMutex );
	app_os_mutex_unlock( & g_HandlerContex.hddCtx.hddMutex);

	//== Create Thread
	if ( app_os_thread_create( & g_HandlerContex.threadHandler, HDDHandlerThreadStart, & g_HandlerContex ) != 0 )	{
		g_HandlerContex.bThreadRunning = false;
		HDDLog( g_LogHandle, Normal, " %s> start handler thread failed! ", MyTopic );	
		return handler_fail;
	}

	g_HandlerContex.bThreadRunning = true;
	g_Status = handler_status_start;
	time( & g_MonitorTime );
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
	if ( g_HandlerContex.bThreadRunning == true ) {	
		g_HandlerContex.bThreadRunning = false;
		app_os_thread_join( g_HandlerContex.threadHandler );
		g_HandlerContex.threadHandler = NULL;
	}

	if ( g_HandlerContex.bHasSQFlash ) {
		hdd_CleanupSQFlashLib( );
	}

	if (&g_HandlerContex.hddCtx.hddInfo) {
		hdd_DestroyHDDInfoList(g_HandlerContex.hddCtx.hddInfo.hddMonInfoList);
	}

	if (g_HandlerContex.thrCtx.thrList) {
		Threshold_DestroyHDDThrList(g_HandlerContex.thrCtx.thrList);
		g_HandlerContex.thrCtx.thrList = NULL;
	}
	
	//UninitLog( g_loghandle );
	g_Status = handler_status_stop;
	return handler_success;
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
	switch (g_Status)
		{
		default:
		case handler_status_no_init:
		case handler_status_init:
		case handler_status_stop:
			*pOutStatus = g_Status;
			break;
		case handler_status_start:
		case handler_status_busy:
			{
			time_t tv;
			time(&tv);
			if(difftime(tv, g_MonitorTime)>5)
				g_Status = handler_status_busy;
			else
				g_Status = handler_status_start;
			*pOutStatus = g_Status;
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
	printf(" %s> Update Status", MyTopic);
	if(pluginfo)
		memcpy(&g_PluginInfo, pluginfo, sizeof(HANDLER_INFO));
	else
	{
		memset(&g_PluginInfo, 0, sizeof(HANDLER_INFO));
		snprintf( g_PluginInfo.Name, sizeof( g_PluginInfo.Name), "%s", MyTopic );
		g_PluginInfo.RequestID = 0;
		g_PluginInfo.ActionID = 0;
	}
}



//-----------------------------------------------------------------------------
// Main Function:
//-----------------------------------------------------------------------------




/* **************************************************************************************
 *  Function Name: Handler_Get_Capability
 *  Description: Get Handler Information specification. 
 *  Input :  None
 *  Output: char ** : pOutReply //caller should free (*pOutRelpy)
 *  Return:  int  : Length of the status information in JSON format
 *                :  0 : no data need to trans
 * **************************************************************************************/
int HANDLER_API Handler_Get_Capability( char ** pOutReply ) // JSON Format
{
	int len = 0; // Data length of the pOutReply 
	cJSON *pRoot = NULL;
	*pOutReply = NULL;
	if ( ! g_HandlerContex.bHasSQFlash ) return 0;
	app_os_mutex_lock( & g_HandlerContex.hddCtx.hddMutex );
	pRoot = Parser_CreateInfoSpec(& g_HandlerContex.hddCtx.hddInfo);
	app_os_mutex_unlock( & g_HandlerContex.hddCtx.hddMutex );
	*pOutReply  = cJSON_PrintUnformatted( pRoot );
	cJSON_Delete(pRoot);
	len = strlen(*pOutReply);
	return len;
}







/* **************************************************************************************
 *  Function Name: Handler_Recv
 *  Description: Receive Packet from MQTT Server
 *  Input :	char * const topic, 
 *				void* const data, 
 *				const size_t datalen
 *  Output: void *pRev1, 
 *				void* pRev2
 *  Return: None
 * ***************************************************************************************/
void HANDLER_API Handler_Recv( char * const topic, void* const data, const size_t datalen, void *pRev1, void* pRev2 )
{
	int cmdID = 0;
	int replyID = 0;
	char errorStr[128] = {0};
	cJSON * pReqRoot = NULL;
	cJSON * body = NULL;
	cJSON * target = NULL;
	//HDDLog(g_LogHandle, Normal, "%s --> Recv Topic [%s] Data :\r\n  %s\r\n", MyTopic, topic, (char*)data);
	if ( !data || datalen <= 0) return;

	while( ! g_HandlerContex.bThreadRunning) {
		app_os_sleep(100);
	}

	//== parse cmdID ==
	pReqRoot = cJSON_Parse(data); // remain the cJSON root(pReqRoot), which is to be ergodic and will be free in end.
		if (!pReqRoot)  return;   //Format Error 415 
	body = cJSON_GetObjectItem(pReqRoot, AGENTINFO_BODY_STRUCT);
		if (!body)   { cJSON_Delete(pReqRoot); return; }
	target = cJSON_GetObjectItem(body, AGENTINFO_CMDTYPE);
		if (!target) { cJSON_Delete(pReqRoot); return; }
	cmdID = target->valueint;


	switch ( cmdID )
	{
	case devmon_hddinfo_req:
		if ( 0 == replyID ) replyID = devmon_hddinfo_req + 1 ;

	case devmon_get_capability_req:
	{
		if (0 == replyID) replyID = devmon_get_capability_req + 1;
		if (g_HandlerContex.bHasSQFlash) {
			char * pInfo = NULL;
			app_os_mutex_lock(&g_HandlerContex.hddCtx.hddMutex);
			pInfo = cJSON_PrintUnformatted(g_HandlerContex.hddCtx.pMonitor);
			app_os_mutex_unlock(&g_HandlerContex.hddCtx.hddMutex);
			if (g_sendcbf) g_sendcbf(&g_PluginInfo, replyID, pInfo, strlen(pInfo) + 1, NULL, NULL);
			free(pInfo);
		}
		else {
			char * repMsg = "{\"hddInfo\":\"No support\"}";
			if (g_sendcbf) g_sendcbf(&g_PluginInfo, devmon_error_rep, repMsg, strlen(repMsg) + 1, NULL, NULL);
			HDDLog(g_LogHandle, Normal, " %s>HDD S.M.A.R.T no support", MyTopic);
			return;
		}
		break;
	}

	case devmon_get_sensors_data_req:
	{
		cJSON * pReplyRoot = NULL;
		char * pInfo = NULL;
		replyID = devmon_get_sensors_data_rep;
		app_os_mutex_lock(&g_HandlerContex.hddCtx.hddMutex);
		pReplyRoot = Parser_CreateSensorInfo(pReqRoot, g_HandlerContex.hddCtx.pMonitor);
		app_os_mutex_unlock(&g_HandlerContex.hddCtx.hddMutex);
		pInfo = cJSON_PrintUnformatted(pReplyRoot);
		if (g_sendcbf) g_sendcbf(&g_PluginInfo, replyID, pInfo, strlen(pInfo) + 1 , NULL, NULL);
		cJSON_Delete(pReplyRoot);
		free(pInfo);
		break;
	}

	case devmon_set_thr_req:
	{
		bool bReport = false;
		bool bNormal = true;
		char reply[128] = "{\"setThrRep\":\"HDD threshold apply OK!\"}";	
		char *            pRepMsg = NULL;
		char *       pRepErrorMsg = NULL;
		char *      pRepNormalMsg = NULL;
		char *        writeThrCfg = NULL;
		hdd_thr_list_t oldThrList = NULL;
		hdd_thr_list_t newThrList = NULL;
		cJSON *       rawThrArray = NULL;
		cJSON *    actualThrArray = NULL;
		cJSON *          pMonitor = NULL;

		app_os_mutex_lock(&g_HandlerContex.thrCtx.thrMutex);
		app_os_mutex_lock(&g_HandlerContex.hddCtx.hddMutex);
		
		
		//== backup old rules and declare new rules
		oldThrList = g_HandlerContex.thrCtx.thrList;
		newThrList = Threshold_CreateHDDThrList(& g_HandlerContex.hddCtx.hddInfo);

		//== process ThrArray , convert raw rules into actual rule, and delete invalid rule
		rawThrArray = cJSON_GetObjectItem(body, HDD_THR);
		Parser_ConvertGroupRules(rawThrArray, & actualThrArray, newThrList);
		if (pMonitor = (&g_HandlerContex)->hddCtx.pMonitor) {
			Parser_DropInvalidRules(& actualThrArray,pMonitor);
		}
		Threshold_LoadALLRules(newThrList, actualThrArray);
		Threshold_MoveCache(oldThrList, newThrList); // keep cache data of unchanged-rules.

		//== write actualThrArray into ThrCfg
		cJSON_ReplaceItemInObject(body, HDD_THR, actualThrArray);
		writeThrCfg = cJSON_PrintUnformatted(pReqRoot);
		Threshold_WriteHDDThrCfg(g_PluginInfo.WorkDir, writeThrCfg);
		free(writeThrCfg);

		//== check new rules and compare with old
		bReport |= Threshold_ChkHDDThrList(newThrList, g_HandlerContex.hddCtx.pMonitor, &bNormal, &pRepErrorMsg);
		bReport |= Threshold_ResetNormal(oldThrList, newThrList, &pRepNormalMsg);
		
		
		//== replace old with new
		Threshold_DestroyHDDThrList(oldThrList);
		g_HandlerContex.thrCtx.thrList = newThrList;
		app_os_mutex_unlock(&g_HandlerContex.hddCtx.hddMutex);
		app_os_mutex_unlock(&g_HandlerContex.thrCtx.thrMutex);

		
			
		//==send ok
		if (g_sendcbf)  g_sendcbf(&g_PluginInfo, devmon_set_thr_rep, reply, strlen(reply) + 1, NULL, NULL);

		//==send RepMsg
		if (bReport)  {
			int len    = 0;
			int offset = 0;
			if (pRepErrorMsg)  len += strlen(pRepErrorMsg);
			if (pRepNormalMsg) len += strlen(pRepNormalMsg);
			pRepMsg = (char *)malloc(len + 3);
			memset(pRepMsg,0,len);

			if (pRepErrorMsg) {
				offset += sprintf(pRepMsg + offset, pRepErrorMsg);
				free(pRepErrorMsg);
			}
			if (pRepNormalMsg) {
				if (offset) offset += sprintf(pRepMsg + offset, "; %s", pRepNormalMsg);
				else        offset += sprintf(pRepMsg + offset, pRepNormalMsg);
				free(pRepNormalMsg);
			}
			SendTHRReport(bNormal, pRepMsg);
			free(pRepMsg);
		}

		break;
	}

	case devmon_del_all_thr_req:
	{
		bool bResetNormal = false;
		char * pRepNormalMsg = NULL;
		char reply[128] = "{\"delAllThrRep\":\"Delete all threshold successfully!\"}";
		hdd_thr_list_t nullThrList = NULL;
		app_os_mutex_lock(&g_HandlerContex.thrCtx.thrMutex);
		app_os_mutex_lock(&g_HandlerContex.hddCtx.hddMutex);
		nullThrList = Threshold_CreateHDDThrList(& g_HandlerContex.hddCtx.hddInfo);
		bResetNormal = Threshold_ResetNormal(g_HandlerContex.thrCtx.thrList, nullThrList, & pRepNormalMsg);
		Threshold_DestroyHDDThrList(g_HandlerContex.thrCtx.thrList);
		g_HandlerContex.thrCtx.thrList = nullThrList;
		app_os_mutex_unlock(&g_HandlerContex.hddCtx.hddMutex);
		app_os_mutex_unlock(&g_HandlerContex.thrCtx.thrMutex);
		Threshold_WriteHDDThrCfg(g_PluginInfo.WorkDir, NULL);
		if (g_sendcbf) {
			g_sendcbf(&g_PluginInfo, devmon_del_all_thr_rep, reply, strlen(reply) + 1, NULL, NULL);
		}
		if (bResetNormal) {
			SendTHRReport(true, pRepNormalMsg);
			free(pRepNormalMsg);
		}

		break;
	}

	case devmon_auto_upload_req:
	{
app_os_mutex_lock(&g_HandlerContex.uploadCtx.reportMutex);
        if (! Parser_ParseAutoReportCmd(data, & g_HandlerContex.uploadCtx.bEnable,
			                                  & g_HandlerContex.uploadCtx.pRequestPathArray,
											  & g_HandlerContex.uploadCtx.iIntervalMs,
											  & g_HandlerContex.uploadCtx.iTimeoutMs)) 
		{
			HDDLog(g_LogHandle, Fatal, "%s > Parse autoUpload command failture.\r\n", MyTopic);
		}	
app_os_mutex_unlock(&g_HandlerContex.uploadCtx.reportMutex);
		break;

	}
	case 808:
		{
			if ( app_os_thread_create( & g_HandlerContex.threadHandler, Handler_MemLeakTest, & g_HandlerContex ) != 0 )	{
				g_HandlerContex.bThreadRunning = false;
				HDDLog( g_LogHandle, Normal, " %s> start Memory leak test failed ", MyTopic );	
			}
			break;
		}
	default:
		{
			char repMsg[32] = {0};
			int len = 0;
			sprintf( repMsg, "{\"errorRep\":\"Unknown cmd!\"}" );
			len= strlen( "{\"errorRep\":\"Unknown cmd!\"}" ) ;
			if ( g_sendcbf ) g_sendcbf( & g_PluginInfo, devmon_error_rep, repMsg, len, NULL, NULL );
			break;
		}
	}//switch End

	cJSON_Delete(pReqRoot);
	return;
}//Handler_Rec End


/* **************************************************************************************
 *  Function Name: Handler_AutoReportStart
 *  Description: Start Auto Report
 *  Input : char *pInQuery
 *  Output: char * pOutReply
 *  Return: None
 * ***************************************************************************************/
void HANDLER_API Handler_AutoReportStart( char *pInQuery )
{
	/*
	{"susiCommData":{"requestID":1001,"catalogID":4,"commCmd":2053,"handlerName":"general","autoUploadIntervalSec":30,"requestItems":{
		"SUSIControl":{"e":[{"n":"SUSIControl/Hardware Monitor"},{"n":"SUSIControl/GPIO"},{"n":"SUSIControl/Voltate/v1"}]},
		"HDDMonitor":{"e":[{"n":"HDDMonitor/hddInfoList"},{"n":"HDDMonitor/hddSmartInfoList/Disk0-Crucial_CT250MX200SSD1/PowerOnHoursPOH"},{"n":"HDDMonitor/hddSmartInfoList/Disk1-WDC WD10EZEX-08M2NA0/PowerOnHoursPOH"}]},
		"ProcessMonitor":{"e":[{"n":"ProcessMonitor/System Monitor Info"},{"n":"ProcessMonitor/Process Monitor Info/6016-conhost.exe"}]},
		"NetWork":{"e":[{"n":"NetWork/netMonInfoList/VMwareNetworkAdapterVMnet8"},{"n":"NetWork/netMonInfoList/VMwareNetworkAdapterVMnet1/netUsage"}]}
	}}}*/

	app_os_mutex_lock(& g_HandlerContex.reportCtx.reportMutex);
	if (Parser_ParseAutoReportCmd(pInQuery, & g_HandlerContex.reportCtx.bEnable,
		                                    & g_HandlerContex.reportCtx.pRequestPathArray,
											& g_HandlerContex.reportCtx.iIntervalMs,
											& g_HandlerContex.reportCtx.iTimeoutMs
		))
	{
		g_HandlerContex.reportCtx.bEnable = true;
		HDDLog(g_LogHandle, Alarm, "%s > Auto report start .\r\n", MyTopic);
	}
	app_os_mutex_unlock(& g_HandlerContex.reportCtx.reportMutex);
	return;
}

/* **************************************************************************************
 *  Function Name: Handler_AutoReportStop
 *  Description: Stop Auto Report
 *  Input : char *pInQuery, if *pInQuery = NULL, then stop all upload message.
 *  Output: char * pOutReply
 *  Return: None
 * ***************************************************************************************/
void HANDLER_API Handler_AutoReportStop( char *pInQuery )
{
	app_os_mutex_lock(& g_HandlerContex.reportCtx.reportMutex);
	if (Parser_ParseAutoReportCmd(pInQuery, & g_HandlerContex.reportCtx.bEnable,
	                                        & g_HandlerContex.reportCtx.pRequestPathArray,
	                                        & g_HandlerContex.reportCtx.iIntervalMs,
	                                        & g_HandlerContex.reportCtx.iTimeoutMs
		))
	{
		g_HandlerContex.reportCtx.bEnable = false;
		HDDLog(g_LogHandle, Alarm, "%s > Auto report stop .\r\n", MyTopic);
	}
	app_os_mutex_unlock(& g_HandlerContex.reportCtx.reportMutex);
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
	return;
}