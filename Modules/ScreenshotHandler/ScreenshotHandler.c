/****************************************************************************/
/* Copyright(C) : Advantech Technologies, Inc.								       */
/* Create Date  : 2014/10/28 by hailong.dang								          */
/* Modified Date: 2014/10/28 by hailong.dang								          */
/* Abstract     : Handler API                                     			 */
/* Reference    : None														             */
/****************************************************************************/
#include "platform.h"
#include "susiaccess_handler_api.h"
#include "ScreenshotHandler.h"
#include "ScreenshotLog.h"
#include "common.h"
#include "Parser.h"
#include "Base64.h"

//-----------------------------------------------------------------------------
// Internal Prototypes:
//-----------------------------------------------------------------------------
//
typedef struct{
   CAGENT_THREAD_HANDLE threadHandler;
   bool isThreadRunning;
	bool threadSyncFlag;
}handler_context_t;

//-----------------------------------------------------------------------------
// Types and defines:
//-----------------------------------------------------------------------------
const char strPluginName[MAX_TOPIC_LEN] = MyTopic;
const int iRequestID = cagent_request_screenshot;
const int iActionID = cagent_reply_screenshot;
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Sample handler variables define
//-----------------------------------------------------------------------------
static Handler_info  g_PluginInfo;
static handler_context_t g_HandlerContex;
static void* g_loghandle = NULL;
static bool g_bEnableLog = true;
static char g_FlagCode = 0;
static HandlerSendCbf           g_sendcbf = NULL;						// Client Send information (in JSON format) to Cloud Server	
static HandlerSendCustCbf       g_sendcustcbf = NULL;			    // Client Send information (in JSON format) to Cloud Server with custom topic	
static HandlerAutoReportCbf     g_sendreportcbf = NULL;				// Client Send report (in JSON format) to Cloud Server with AutoReport topic	
static HandlerSendCapabilityCbf g_sendcapabilitycbf = NULL;	
static HandlerSubscribeCustCbf  g_subscribecustcbf = NULL;

//------------------------------User variables define--------------------------
#ifdef WIN32
#define DEF_SCREENSHOT_HELPER      "ScreenshotHelper.exe"
#else
#define DEF_SCREENSHOT_HELPER      "ScreenshotHelper"
#endif
static bool IsHandlerStart = false;
static char TempPath[MAX_PATH] = {0};
static char ModulePath[MAX_PATH] = {0};
static char DevID[128]={0};
static char ScreenshotHelperPath[MAX_PATH] = {0};
static char ScreenshotHelperTempPath[MAX_PATH] = {0};
//-----------------------------------------------------------------------------

//------------------------------user func define-------------------------------
static BOOL ScreenshotRun(char * cmdLine);
static CAGENT_PTHREAD_ENTRY(ScreenshotTransferThreadStart, args);
static void ScreenshotTransfer();
//-----------------------------------------------------------------------------

static BOOL ScreenshotRun(char * cmdLine)
{
	BOOL bRet = FALSE;
	if(cmdLine == NULL) return bRet;
#ifdef WIN32
	bRet = app_os_run_process_as_user(cmdLine);
#else
	{
		FILE *fopen = NULL;
		if((fopen = popen(cmdLine,"r"))==NULL)
		{
			bRet = FALSE;
		}
		else
		{
			pclose(fopen);
			bRet = TRUE;
		}
	}
#endif
	return bRet;
}
static CAGENT_PTHREAD_ENTRY(ScreenshotTransferThreadStart, args)
{
#define DEF_BASE64_PER_SIZE    1200   //Common multiple of 6 and 8
	handler_context_t *pHandlerContex = (handler_context_t*)args;
	pHandlerContex->threadSyncFlag = false;
	if(strlen(ScreenshotHelperTempPath) == 0 || !app_os_is_file_exist(ScreenshotHelperTempPath))
	{
		memset(ModulePath, 0, sizeof(ModulePath));
		memset(TempPath, 0, sizeof(TempPath));
		app_os_get_module_path(ModulePath);
		app_os_get_temppath(TempPath, sizeof(TempPath));
		if(strlen(TempPath)&&strlen(ModulePath))
		{
			memset(ScreenshotHelperPath, 0, sizeof(ScreenshotHelperPath));
			sprintf(ScreenshotHelperPath, "%s%s", ModulePath, DEF_SCREENSHOT_HELPER);
			memset(ScreenshotHelperTempPath, 0, sizeof(ScreenshotHelperTempPath));
			sprintf(ScreenshotHelperTempPath, "%s/%s", TempPath, DEF_SCREENSHOT_HELPER);
		}
		if(app_os_is_file_exist(ScreenshotHelperTempPath))
		{
			remove(ScreenshotHelperTempPath);
		}
		if(app_os_is_file_exist(ScreenshotHelperPath))
		{
			app_os_file_copyEx(ScreenshotHelperPath, ScreenshotHelperTempPath);
		}
	}
	if(pHandlerContex->isThreadRunning)
	{
		BOOL bRet = FALSE;
		ScreenshotUploadRep screenshotUploadRep;
		memset(&screenshotUploadRep, 0, sizeof(ScreenshotUploadRep));
		strcpy(screenshotUploadRep.status, "False");
		if(strlen(ScreenshotHelperTempPath) && app_os_is_file_exist(ScreenshotHelperTempPath))
		{
			char cmdLine[512] = {0};
			char localFilePath[MAX_PATH] = {0};
			char screenshotFileName[64] = {0};
			sprintf(screenshotFileName, "%s.jpg", DevID);
			sprintf(localFilePath, "%s/%s", TempPath, screenshotFileName);
			//sprintf_s(cmdLine, sizeof(cmdLine), "cmd.exe /c \"%s\" %s", screenshotHelperTempPath, screenshotFileName);
			sprintf(cmdLine, "\"%s\" %s", ScreenshotHelperTempPath, screenshotFileName);
#ifndef WIN32
			if(!app_os_is_workstation_locked() && !app_os_process_check("LogonUI.exe"))
#else
			if(!app_os_is_workstation_locked() && !app_os_process_check("LogonUI.exe") && app_os_process_check("explorer.exe"))
#endif
			{
				//bRet = RunProcessAsUser(cmdLine, FALSE, FALSE, NULL);
				ScreenshotLog(g_loghandle, Normal, "cmdLine:%s, ScreenshotHelperTempPath:%s,localFilePath:%s", 
					cmdLine,ScreenshotHelperTempPath,localFilePath);
				bRet = ScreenshotRun(cmdLine);
				if(bRet && app_os_is_file_exist(localFilePath))
				{
					FILE * fp = NULL;
					DWORD fSize = 0;
					bRet = FALSE;
					fp = fopen(localFilePath, "rb+");
					if(fp != NULL)
					{
						fseek( fp, 0, SEEK_END );
						fSize = ftell(fp);
						fseek(fp, 0, SEEK_SET);
						if(fSize > 0)
						{
							int iRet = 0;
							int realReadLen = 0;
							char * base64Buf = NULL;
							int base64Len = 0;
							char srcBuf[DEF_BASE64_PER_SIZE] = {0};
							char * base64Str = NULL;
							int base64StrLen = fSize*8/6+8;
							base64Str = (char *)malloc(base64StrLen);
							memset(base64Str, 0, base64StrLen);
							bRet = TRUE;
							while((realReadLen = fread(srcBuf, 1, sizeof(srcBuf), fp)) != 0)
							{
								iRet = Base64Encode(srcBuf, realReadLen, &base64Buf, &base64Len);
								if(iRet == 0 && base64Buf != NULL)
								{
									if(strlen(base64Buf))
									{
										strcat(base64Str, base64Buf);
									}
									free(base64Buf);
									base64Buf = NULL;
								}
								else
								{
									bRet = FALSE;
									break;
								}
								memset(srcBuf, 0, sizeof(srcBuf));
							}
							if(bRet && strlen(base64Str))
							{
								screenshotUploadRep.base64Str = base64Str;
								sprintf(screenshotUploadRep.uplFileName, "%s", screenshotFileName);
								memset(screenshotUploadRep.status, 0, sizeof(screenshotUploadRep.status));
								strcpy(screenshotUploadRep.status, "True");
								memset(screenshotUploadRep.uplMsg, 0, sizeof(screenshotUploadRep.uplMsg));
								sprintf(screenshotUploadRep.uplMsg, "%s", "Screenshot success!");
								{
									char * uploadRepJsonStr = NULL;
									int jsonStrlen = Parser_PackScrrenshotUploadRep(&screenshotUploadRep, &uploadRepJsonStr);
									if(jsonStrlen > 0 && uploadRepJsonStr != NULL)
									{
										g_sendcbf(&g_PluginInfo, screenshot_transfer_rep, uploadRepJsonStr, strlen(uploadRepJsonStr)+1, NULL, NULL);
									}
									if(uploadRepJsonStr)free(uploadRepJsonStr);
								}
							}
							free(base64Str);
						}
						fclose(fp);
					}
					if(!bRet)
					{
						sprintf(screenshotUploadRep.uplMsg, "%s", "Screenshot error!Base64 encode error!");
					}
				}
				else
				{
					sprintf(screenshotUploadRep.uplMsg, "%s", "Screenshot error!");
				}
			}
			else
			{
				sprintf(screenshotUploadRep.uplMsg, "%s", "Screenshot error!OS not logged in!");
			}
			if(app_os_is_file_exist(localFilePath))
			{
				app_os_file_remove(localFilePath);
			}
		}
		else
		{
			sprintf(screenshotUploadRep.uplMsg, "%s", "Screenshot error!");
		}

		if(!bRet && strlen(screenshotUploadRep.uplMsg))
		{
			char * uploadRepJsonStr = NULL;
			int jsonStrlen = Parser_PackScrrenshotUploadRep(&screenshotUploadRep, &uploadRepJsonStr);
			if(jsonStrlen > 0 && uploadRepJsonStr != NULL)
			{
				g_sendcbf(&g_PluginInfo, screenshot_transfer_rep, uploadRepJsonStr, strlen(uploadRepJsonStr)+1, NULL, NULL);
			}
			if(uploadRepJsonStr)free(uploadRepJsonStr);
		}
	}
	pHandlerContex->isThreadRunning = false;
	return 0;
}

static void ScreenshotTransfer()
{
	ScreenshotUploadRep screenshotUploadRep;
	memset(&screenshotUploadRep, 0, sizeof(ScreenshotUploadRep));
	strcpy(screenshotUploadRep.status, "False");

	if(!g_HandlerContex.isThreadRunning)
	{
		g_HandlerContex.isThreadRunning = true;
		g_HandlerContex.threadSyncFlag = true;
		if (app_os_thread_create(&g_HandlerContex.threadHandler, ScreenshotTransferThreadStart, &g_HandlerContex) != 0)
		{
			g_HandlerContex.isThreadRunning = false;
			sprintf(screenshotUploadRep.uplMsg, "%s", "Create Screenshot thread failed!");
		}
		else
		{
			while(g_HandlerContex.threadSyncFlag && g_HandlerContex.isThreadRunning)
				app_os_sleep(10);
		}
	}
	else
	{
		sprintf(screenshotUploadRep.uplMsg, "%s", "Screenshot running!");
	}
	if(strlen(screenshotUploadRep.uplMsg))
	{
		char * uploadRepJsonStr = NULL;
		int jsonStrlen = Parser_PackScrrenshotUploadRep(&screenshotUploadRep, &uploadRepJsonStr);
		if(jsonStrlen > 0 && uploadRepJsonStr != NULL)
		{
			g_sendcbf(&g_PluginInfo, screenshot_transfer_rep, uploadRepJsonStr, strlen(uploadRepJsonStr)+1, NULL, NULL);
		}
		if(uploadRepJsonStr)free(uploadRepJsonStr);
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

	//g_HandlerContex.threadHandler = NULL;
	g_HandlerContex.isThreadRunning = false;
   g_HandlerContex.threadSyncFlag = false;
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

	strcpy(DevID, g_PluginInfo.agentInfo->devId);

	// 3. Callback function -> Send JSON Data by this callback function
	g_sendcbf           = g_PluginInfo.sendcbf          = pluginfo->sendcbf;
	g_sendcustcbf       = g_PluginInfo.sendcustcbf      = pluginfo->sendcustcbf;
	g_subscribecustcbf  = g_PluginInfo.subscribecustcbf = pluginfo->subscribecustcbf;
	g_sendreportcbf     = g_PluginInfo.sendreportcbf    = pluginfo->sendreportcbf;
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
		IsHandlerStart = true;
		{
			app_os_get_module_path(ModulePath);
			app_os_get_temppath(TempPath, sizeof(TempPath));
			if(strlen(TempPath)&&strlen(ModulePath))
			{
				sprintf(ScreenshotHelperPath,  "%s%s", ModulePath, DEF_SCREENSHOT_HELPER);
				sprintf(ScreenshotHelperTempPath,  "%s/%s", TempPath, DEF_SCREENSHOT_HELPER);
			}
			if(app_os_is_file_exist(ScreenshotHelperTempPath))
			{
				remove(ScreenshotHelperTempPath);
			}
			if(app_os_is_file_exist(ScreenshotHelperPath))
			{
				app_os_file_copyEx(ScreenshotHelperPath, ScreenshotHelperTempPath);
				g_FlagCode = SCREENSHOT_FLAGCODE_INTERNAL;
			}
			else {
				g_FlagCode = SCREENSHOT_FLAGCODE_NONE;
			}
		}
		bRet = TRUE;

	}
done:
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
		if(g_HandlerContex.isThreadRunning == true)
		{
			g_HandlerContex.isThreadRunning = false;
			app_os_thread_join(g_HandlerContex.threadHandler);
			//g_HandlerContex.threadHandler = NULL;
		}
		g_HandlerContex.threadSyncFlag = false;
		IsHandlerStart = false;
	}
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
	ScreenshotLog(g_loghandle, Normal, " %s>Recv Topic [%s] Data %s", strPluginName, topic, (char*) data );
	printf(" >Recv Topic [%s] Data %s", topic, (char*) data );

	if(!ParseReceivedData(data, datalen, &cmdID))
		return;
	switch(cmdID)
	{
	case screenshot_transfer_req:
		{
			ScreenshotTransfer();
			break;
		}
	case screenshot_capability_req:
		{
			int len = 0;
			char * pOutReply = NULL;
			len = Parser_CreateCapabilityRep(g_FlagCode, & pOutReply);
			if (len > 0 && NULL != pOutReply ){
				g_sendcbf( & g_PluginInfo, screenshot_capability_rep, pOutReply, len + 1, NULL, NULL);
				free(pOutReply);
			}
			break;
		}
	default: 
		{
			char * errorRepJsonStr = NULL;
			char errorStr[128] = {0};
			int jsonStrlen = 0;
			strcpy(errorStr, "Unknown cmd!");
			jsonStrlen = Parser_PackScrrenshotError(errorStr, &errorRepJsonStr);
			if(jsonStrlen > 0 && errorRepJsonStr != NULL)
			{
				g_sendcbf(&g_PluginInfo, screenshot_error_rep, errorRepJsonStr, strlen(errorRepJsonStr)+1, NULL, NULL);
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
	int len = 0;
	* pOutReply = NULL;
	len = Parser_CreateCapabilityRep(g_FlagCode, pOutReply);
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