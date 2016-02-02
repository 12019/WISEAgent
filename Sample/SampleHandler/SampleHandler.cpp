// SampleHandler.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"
#include "susiaccess_handler_api.h"
#include "DeviceMessageGenerate.h"
#include "IoTMessageGenerate.h"
#include <Log.h>

//-----------------------------------------------------------------------------
// Logger defines:
//-----------------------------------------------------------------------------
#define SAMPLEHANDLER_LOG_ENABLE
//#define DEF_SAMPLEHANDLER_LOG_MODE    (LOG_MODE_NULL_OUT)
//#define DEF_SAMPLEHANDLER_LOG_MODE    (LOG_MODE_FILE_OUT)
#define DEF_SAMPLEHANDLER_LOG_MODE    (LOG_MODE_CONSOLE_OUT|LOG_MODE_FILE_OUT)

LOGHANDLE g_samolehandlerlog = NULL;

#ifdef SAMPLEHANDLER_LOG_ENABLE
#define SampleHLog(level, fmt, ...)  do { if (g_samolehandlerlog != NULL)   \
	WriteLog(g_samolehandlerlog, DEF_SAMPLEHANDLER_LOG_MODE, level, fmt, ##__VA_ARGS__); } while(0)
#else
#define SampleHLog(level, fmt, ...)
#endif

//-----------------------------------------------------------------------------
// Types and defines:
//-----------------------------------------------------------------------------
#define cagent_request_custom 2102
#define cagent_custom_action 31002
const char strHandlerName[MAX_TOPIC_LEN] = {"Sample"};
const int iRequestID = cagent_request_custom;
const int iActionID = cagent_custom_action;
MSG_CLASSIFY_T *g_Capability = NULL;
MSG_ATTRIBUTE_T* g_attr=NULL;
//-----------------------------------------------------------------------------
// Internal Prototypes:
//-----------------------------------------------------------------------------
//
typedef struct{
   void* threadHandler;
   int interval;
   bool isThreadRunning;
}handler_context_t;

//-----------------------------------------------------------------------------
// Variables
//-----------------------------------------------------------------------------
static Handler_info  g_HandlerInfo;
static handler_context_t g_HandlerContex;
static HANDLER_THREAD_STATUS g_status = handler_status_no_init;
static time_t g_monitortime;
static HandlerSendCbf  g_sendcbf = NULL;						// Client Send information (in JSON format) to Cloud Server	
static HandlerSendCustCbf  g_sendcustcbf = NULL;			    // Client Send information (in JSON format) to Cloud Server with custom topic	
static HandlerSubscribeCustCbf g_subscribecustcbf = NULL;
static HandlerAutoReportCbf g_sendreportcbf = NULL;				// Client Send report (in JSON format) to Cloud Server with AutoReport topic
static HandlerSendCapabilityCbf g_sendcapabilitycbf = NULL;		
static HandlerSendEventCbf g_sendeventcbf = NULL;
//-----------------------------------------------------------------------------
// Function:
//-----------------------------------------------------------------------------
void Handler_Uninitialize();

#ifdef _MSC_VER
BOOL WINAPI DllMain(HINSTANCE module_handle, DWORD reason_for_call, LPVOID reserved)
{
	if (reason_for_call == DLL_PROCESS_ATTACH) // Self-explanatory
	{
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

MSG_CLASSIFY_T * CreateCapability()
{
	MSG_CLASSIFY_T* myCapability = IoT_CreateRoot((char*)strHandlerName);
	MSG_CLASSIFY_T*myGroup = IoT_AddGroup(myCapability, "TEST");
	MSG_ATTRIBUTE_T* attr1 = NULL;
	MSG_ATTRIBUTE_T* attr = IoT_AddGroupAttribute(myGroup, "bu"); /*Add base unit attribute*/
	if(attr)
		IoT_SetStringValue(attr, "%", IoT_NODEFINE);

	attr1 = IoT_AddSensorNode(myGroup, "sample");
	if(attr1)
	{
		IoT_SetFloatValueWithMaxMin(attr1, 10, IoT_READONLY, 100, 10, "%");
		g_attr = attr1;
	}
	
	return myCapability;
}

void on_msgrecv(char * const topic, void* const data, const size_t datalen, void *pRev1, void* pRev2)
{
	SampleHLog(Normal, "Packet received, %s\r\n", data);
}

void TestCustMsg()
{
	char topicStr[128] = {0};
	/* Add  subscribe topic Callback*/
	sprintf(topicStr, "/cagent/admin/%s/testreq", g_HandlerInfo.agentInfo->devId);
	g_subscribecustcbf(topicStr, on_msgrecv);

	if(g_Capability)
	{
		char* message = IoT_PrintData(g_Capability);

		if(g_sendcustcbf)
			g_sendcustcbf(&g_HandlerInfo, 123, topicStr, message, strlen(message), NULL, NULL);
		free(message);
	}
}

static DWORD WINAPI SampleHandlerReportThread(void *args)
{
	handler_context_t *pHandlerContex = (handler_context_t *)args;
	char* message = NULL;
	int mInterval = pHandlerContex->interval * 1000;
	int data = 0;
	if(g_Capability)
		g_Capability = CreateCapability();

	TestCustMsg();

	while(pHandlerContex->isThreadRunning)
	{
		int ran = (rand() % 10) -5;
		data = data + ran;
		if(g_attr)
			IoT_SetFloatValueWithMaxMin(g_attr, data, IoT_READONLY, 100, 10, "%");

		if(g_Capability)
		{
			message = IoT_PrintData(g_Capability);

			if(g_sendreportcbf)
				g_sendreportcbf(/*Handler Info*/&g_HandlerInfo, /*message data*/message, /*message length*/strlen(message), /*preserved*/NULL, /*preserved*/NULL);
		}

		free(message);
		Sleep(mInterval);
	}
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
	if( pluginfo == NULL )
		return handler_fail;

	// 1. Topic of this handler
	sprintf_s( pluginfo->Name, sizeof(pluginfo->Name), "%s", strHandlerName );
	pluginfo->RequestID = iRequestID;
	pluginfo->ActionID = iActionID;
	g_samolehandlerlog = pluginfo->loghandle;
	SampleHLog(Debug, " %s> Initialize", strHandlerName);
	// 2. Copy agent info 
	memcpy(&g_HandlerInfo, pluginfo, sizeof(HANDLER_INFO));
	g_HandlerInfo.agentInfo = pluginfo->agentInfo;

	// 3. Callback function -> Send JSON Data by this callback function
	g_sendcbf = g_HandlerInfo.sendcbf = pluginfo->sendcbf;
	g_sendcustcbf = g_HandlerInfo.sendcustcbf = pluginfo->sendcustcbf;
	g_subscribecustcbf = g_HandlerInfo.subscribecustcbf = pluginfo->subscribecustcbf;
	g_sendreportcbf = g_HandlerInfo.sendreportcbf = pluginfo->sendreportcbf;
	g_sendcapabilitycbf =g_HandlerInfo.sendcapabilitycbf = pluginfo->sendcapabilitycbf;
	g_sendeventcbf = g_HandlerInfo.sendeventcbf = pluginfo->sendeventcbf;

	g_HandlerContex.threadHandler = NULL;
	g_HandlerContex.isThreadRunning = false;
	g_status = handler_status_no_init;
	
	return handler_success;
}

/* **************************************************************************************
 *  Function Name: Handler_Uninitialize
 *  Description: Release the objects or variables used in this handler
 *  Input :  None
 *  Output: None
 *  Return:  void
 * ***************************************************************************************/
void Handler_Uninitialize()
{
	g_sendcbf = NULL;
	g_sendcustcbf = NULL;
	g_sendreportcbf = NULL;
	g_sendcapabilitycbf = NULL;
	g_subscribecustcbf = NULL;
	g_sendeventcbf = NULL;

	if(g_Capability)
	{
		IoT_ReleaseAll(g_Capability);
		g_Capability = NULL;
		g_attr = NULL;
	}
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
	int iRet = handler_fail; 
	SampleHLog(Debug, " %s> Get Status", strHandlerName);
	if(!pOutStatus) return iRet;

	*pOutStatus = g_status;
	
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
	char message[256] = {0};
	char* msg = NULL;
	MSG_CLASSIFY_T* evt = NULL;
	SampleHLog(Debug, " %s> Update Status", strHandlerName);
	if(pluginfo)
		memcpy(&g_HandlerInfo, pluginfo, sizeof(HANDLER_INFO));
	else
	{
		memset(&g_HandlerInfo, 0, sizeof(HANDLER_INFO));
		sprintf_s( g_HandlerInfo.Name, sizeof( g_HandlerInfo.Name), "%s", strHandlerName );
		g_HandlerInfo.RequestID = iRequestID;
		g_HandlerInfo.ActionID = iActionID;
	}
	sprintf(message, "Handler %s on connected", strHandlerName);

	evt = DEV_CreateEventNotify("message", message);

	msg = DEV_PrintUnformatted(evt);
	
	if(g_sendeventcbf)
		g_sendeventcbf(&g_HandlerInfo, Severity_Informational, msg, strlen(msg), NULL, NULL);
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
	SampleHLog(Debug, "> %s Start", strHandlerName);
	
	g_status = handler_status_start;
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
	SampleHLog(Debug, "> %s Stop", strHandlerName);

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
void HANDLER_API Handler_Recv(char * const topic, void* const data, const size_t datalen, void *pRev1, void* pRev2  )
{
	char message[256] = {0};
	SampleHLog(Debug, " >Recv Topic [%s] Data %s", topic, (char*) data );

	strcpy(message, "{\"message\":\"test message\"}");
	if(g_sendcbf)
		g_sendcbf(/*Handler Info*/&g_HandlerInfo, /*command id*/1, /*message data*/message, /*message length*/strlen(message), /*preserved*/NULL, /*preserved*/NULL);
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
	/*TODO: Parsing received command
	*input data format: 
	* {"susiCommData":{"catalogID":4,"autoUploadIntervalSec":30,"requestID":1001,"requestItems":["all"],"commCmd":2053}}
	*
	* "autoUploadIntervalSec":30 means report sensor data every 30 sec.
	* "requestItems":["all"] defined which handler or sensor data to report. 
	*/
	SampleHLog(Debug, "> %s Start Report", strHandlerName);
	/*create thread to report sensor data*/
	g_HandlerContex.interval = 5;
	g_HandlerContex.isThreadRunning = true;
	g_HandlerContex.threadHandler = CreateThread(NULL, 0, SampleHandlerReportThread, &g_HandlerContex, 0, NULL);
}

/* **************************************************************************************
 *  Function Name: Handler_AutoReportStop
 *  Description: Stop Auto Report
 *  Input : None
 *  Output: None
 *  Return: None
 * ***************************************************************************************/
void HANDLER_API Handler_AutoReportStop(char *pInQuery)
{
	/*TODO: Parsing received command*/
	SampleHLog(Debug, "> %s Stop Report", strHandlerName);

	g_HandlerContex.isThreadRunning = false;

	WaitForSingleObject(g_HandlerContex.threadHandler, INFINITE);
	g_HandlerContex.threadHandler = NULL;
}

/* **************************************************************************************
 *  Function Name: Handler_Get_Capability
 *  Description: Get Handler Information specification. 
 *  Input :  None
 *  Output: char ** : pOutReply       // JSON Format
 *  Return:  int  : Length of the status information in JSON format
 *                :  0 : no data need to trans
 * **************************************************************************************/
int HANDLER_API Handler_Get_Capability( char ** pOutReply ) // JSON Format
{
	char* result = NULL;
	int len = 0;

	SampleHLog(Debug, "> %s Get Capability", strHandlerName);

	if(!pOutReply) return len;

	if(g_Capability)
	{
		IoT_ReleaseAll(g_Capability);
		g_Capability = NULL;
	}

	g_Capability = CreateCapability();

	result = IoT_PrintCapability(g_Capability);

	len = strlen(result);
	*pOutReply = (char *)malloc(len + 1);
	memset(*pOutReply, 0, len + 1);
	strcpy(*pOutReply, result);
	free(result);
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
	SampleHLog(Debug, "> %s Free Allocated Memory", strHandlerName);

	if(pInData)
	{
		free(pInData);
		pInData = NULL;
	}
	return;
}