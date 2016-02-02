/****************************************************************************/
/* Copyright(C) : Advantech Technologies, Inc.								*/
/* Create Date  : 								*/
/* Abstract     : ProcessMonitor API interface definition   					*/
/* Reference    : None														*/
/****************************************************************************/
#include "common.h"
#include "platform.h"
//#include "susiaccess_handler_api.h"
//#include "susiaccess_handler_ex_api.h"
#include "ServerRedundancyHandler.h"
#include "ServerRedundancyLog.h"
#include "Parser.h"
#include "cJSON.h"

#define IP_LIST_BUF_LEN             32
#define MAX_CONNECT_FAILED_CNT      2

static void* g_loghandle = NULL;
static BOOL g_bEnableLog = true;
const char strPluginName[MAX_TOPIC_LEN] = {"ServerRedundancy"};
const int iRequestID = cagent_request_server_redundancy;
const int iActionID = cagent_reply_server_redundancy;

static Handler_info_ex  g_PluginInfo;

static HandlerSendCbf  g_sendcbf = NULL;						// Client Send information (in JSON format) to Cloud Server	
static HandlerSendCustCbf  g_sendcustcbf = NULL;			    // Client Send information (in JSON format) to Cloud Server with custom topic	
static HandlerAutoReportCbf g_sendreportcbf = NULL;				// Client Send report (in JSON format) to Cloud Server with AutoReport topic	
static HandlerSendCapabilityCbf g_sendcapabilitycbf = NULL;	
static HandlerSubscribeCustCbf g_subscribecustcbf = NULL;
static HandlerConnectServerCbf g_connectservercbf = NULL;	
static HandlerDisconnectCbf g_disconnectcbf = NULL;	

static char agentConfigFilePath[MAX_PATH] = {0};
int g_connectedFailedCnt = 0;

struct SERVER_LIST g_ServerList;
struct SERVER_INFO* g_pCurServer = NULL;

void Handler_Uninitialize();

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

void ServercontrolAction(Response_Msg *pRespMsg)
{
	//ServerRedundancyLog(Normal, " %s> Server Response: %s", genreral_Topic, pRespMsg->msg);
	ServerRedundancyLog(g_loghandle, Normal, " %s> Server Response: %s", strPluginName, pRespMsg->strMsg);
	switch (pRespMsg->iStatusCode)
	{
	case SERVER_LOST_CONNECTION:
	case SERVER_AUTH_SUCCESS:
	case SERVER_PRESERVED_MESSAGE:
		break;
	case SERVER_AUTH_FAILED:
	case SERVER_CONNECTION_FULL:
		{
			if(g_disconnectcbf)
				g_disconnectcbf();
		}
		break;
	case SERVER_RECONNECT:
		{
			if(g_connectservercbf)
				g_connectservercbf(g_PluginInfo.ServerIP, g_PluginInfo.ServerPort, g_PluginInfo.serverAuth, g_PluginInfo.TLSType, g_PluginInfo.PSK);
		}
		break;
	case SERVER_CONNECT_TO_MASTER:
		{
			/*connect to master*/
			struct SERVER_INFO *pMasterServer = g_ServerList.server;
			if(g_connectservercbf)
				g_connectservercbf(pMasterServer->sServerIP, pMasterServer->iServerPort, pMasterServer->sServerAuth, pMasterServer->eTLSType, pMasterServer->sPSK);
		}
		break;
	case SERVER_CONNECT_TO_SEPCIFIC:
		{
			/*connect to server*/
			struct SERVER_INFO *pServer = pRespMsg->pServer;
			if(!pServer)
				return;
			if(g_connectservercbf)
				g_connectservercbf(pServer->sServerIP, pServer->iServerPort, strlen(pServer->sServerAuth)>0?pServer->sServerAuth:NULL, pServer->eTLSType, strlen(pServer->sPSK)>0?pServer->sPSK:NULL);
		}
		break;
	default:
		break;
	}
}

bool LoadServerList(char *workDir, struct SERVER_LIST* pServerList)
{
	FILE *fp = NULL;
	char filepath[MAX_PATH] = {0};
	char* buffer = NULL;
	long lSize = 0;
	long result = 0;
	cJSON* pServerListNode = NULL;

	if(!pServerList)
		return false;

	path_combine(filepath, workDir, DEF_SERVER_IP_LIST_FILE);//g_PluginInfo.WorkDir, 

	if(fp=fopen(filepath,"r"))
	{
		// obtain file size:
		fseek (fp , 0 , SEEK_END);
		lSize = ftell (fp);
		rewind (fp);

		// allocate memory to contain the whole file:
		buffer = (char*) malloc (sizeof(char)*lSize);
		if (!buffer) 
		{
			fclose(fp);
			return false;
		}

		// copy the file into the buffer:
		result = fread (buffer,1,lSize,fp);
		if (result != lSize)
		{
			free(buffer);
			fclose(fp);
			return false;
		}

		pServerListNode = cJSON_Parse(buffer);

		if(pServerListNode)
		{
			ParseServerList(pServerListNode, pServerList);
			cJSON_Delete(pServerListNode);
		}
		free(buffer);
		fclose(fp);
	}
	return true;
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
	HANDLER_INFO_EX* tmpinfo = NULL;
	if(pluginfo == NULL)
	{
		return handler_fail;
	}
	tmpinfo = (HANDLER_INFO_EX*)pluginfo;

	if(g_bEnableLog)
	{
		g_loghandle = tmpinfo->loghandle;
	}

	// 1. Topic of this handler
	snprintf( tmpinfo->Name, sizeof(tmpinfo->Name), "%s", strPluginName );
	tmpinfo->RequestID = iRequestID;
	tmpinfo->ActionID = iActionID;
	printf(" >Name: %s\n", strPluginName);
	// 2. Copy agent info 
	memcpy(&g_PluginInfo, tmpinfo, sizeof(HANDLER_INFO_EX));
	g_PluginInfo.agentInfo = tmpinfo->agentInfo;

	{
		char mdPath[MAX_PATH] = {0};
		app_os_get_module_path(mdPath);
		path_combine(agentConfigFilePath, mdPath, DEF_CONFIG_FILE_NAME);

		if(g_bEnableLog)
		{
			g_loghandle = tmpinfo->loghandle;
		}
	}

	// 3. Callback function -> Send JSON Data by this callback function
	g_sendcbf = g_PluginInfo.sendcbf = tmpinfo->sendcbf;
	g_sendcustcbf = g_PluginInfo.sendcustcbf = tmpinfo->sendcustcbf;
	g_subscribecustcbf = g_PluginInfo.subscribecustcbf = tmpinfo->subscribecustcbf;
	g_sendreportcbf = g_PluginInfo.sendreportcbf = tmpinfo->sendreportcbf;
	g_sendcapabilitycbf =g_PluginInfo.sendcapabilitycbf = tmpinfo->sendcapabilitycbf;
	g_connectservercbf = g_PluginInfo.connectservercbf = tmpinfo->connectservercbf;	
	g_disconnectcbf = g_PluginInfo.disconnectcbf = tmpinfo->disconnectcbf;

	LoadServerList(g_PluginInfo.WorkDir, &g_ServerList);
	if(g_ServerList.serverCount==0)
	{
		g_ServerList.server = malloc(sizeof(struct SERVER_INFO));
		memset(g_ServerList.server, 0, sizeof(struct SERVER_INFO));
		g_ServerList.serverCount = 1;

		strncpy(g_ServerList.server->sServerIP, tmpinfo->ServerIP, strlen(tmpinfo->ServerIP)+1);
		g_ServerList.server->iServerPort = tmpinfo->ServerPort;
		strncpy(g_ServerList.server->sServerAuth, tmpinfo->serverAuth, strlen(tmpinfo->serverAuth)+1);
		g_ServerList.server->eTLSType = tmpinfo->TLSType;
		strncpy(g_ServerList.server->sPSK, tmpinfo->PSK, strlen(tmpinfo->PSK)+1);
	}

	return handler_success;
}

void Handler_Uninitialize()
{
	Handler_Stop();
	
	FreeServerList(g_ServerList.server);

	g_sendcbf           = NULL;
	g_sendcustcbf       = NULL;
	g_sendreportcbf     = NULL;
	g_sendcapabilitycbf = NULL;
	g_subscribecustcbf  = NULL;
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
	HANDLER_INFO_EX* tmpinfo = NULL;
	if(pluginfo == NULL) return;
	tmpinfo = (HANDLER_INFO_EX*)pluginfo;
	printf(" %s> Update Status\n", strPluginName);
	if(tmpinfo)
		memcpy(&g_PluginInfo, tmpinfo, sizeof(HANDLER_INFO_EX));
	else
	{
		memset(&g_PluginInfo, 0, sizeof(HANDLER_INFO_EX));
		snprintf( g_PluginInfo.Name, sizeof( g_PluginInfo.Name), "%s", strPluginName );
		g_PluginInfo.RequestID = iRequestID;
		g_PluginInfo.ActionID = iActionID;
	}
	if(tmpinfo->agentInfo->status == AGENT_STATUS_ONLINE)
	{
		printf(" [ServerRedundancy] on status online redundant Server is %d , conencted failed count is %d!\n",g_PluginInfo.ServerIP, g_connectedFailedCnt);
		g_connectedFailedCnt = 0;
		g_pCurServer = NULL;
	}
	else if(tmpinfo->agentInfo->status == AGENT_STATUS_OFFLINE)
	{
		g_connectedFailedCnt++;
		printf(" [ServerRedundancy] on status offline redundant Server is %d , conencted failed count is %d!\n",g_PluginInfo.ServerIP, g_connectedFailedCnt);
		g_pCurServer = NULL;
	}
	else if(tmpinfo->agentInfo->status == AGENT_STATUS_CONNECTION_FAILED)
	{
		g_connectedFailedCnt++;
		printf("[ServerRedundancy] On status changed connected failed count is %d\n", g_connectedFailedCnt);
	}

	if(g_connectedFailedCnt > MAX_CONNECT_FAILED_CNT)
	{
		if(!g_pCurServer)
			g_pCurServer = g_ServerList.server;
		
		if(!g_pCurServer->next)
			g_pCurServer = g_ServerList.server;
		else
			g_pCurServer = g_pCurServer->next;

		printf("[ServerRedundancy] connect server failed! Reconnect server %s : %d !\n", g_pCurServer->sServerIP, g_pCurServer->iServerPort);
		g_connectedFailedCnt = 0;
		if(g_connectservercbf)
			g_connectservercbf( g_pCurServer->sServerIP, g_pCurServer->iServerPort, g_pCurServer->sServerAuth, g_pCurServer->eTLSType, g_pCurServer->sPSK);
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
	if(g_PluginInfo.agentInfo->status == AGENT_STATUS_ONLINE)
	{
	}
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
	BOOL bRet = TRUE;
	//memset(&PowerOnOffHandlerContext, 0, sizeof(void *));
	return bRet;
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
	//{"susiCommData":{"catalogID":4,"handlerName":"ServerRedundancy","catalogID":4,"commCmd":521}}
	int commCmd = unknown_cmd;
	char errorStr[128] = {0};
	
	ServerRedundancyLog(g_loghandle, Normal, " %s>Recv Topic [%s] Data %s", strPluginName, topic, (char*) data );
	printf(" >Recv Topic [%s] Data %s", topic, (char*) data );

	if(!ParseReceivedData(data, datalen, &commCmd))
		return;

	switch(commCmd)
	{
		case server_redundancy_control_req:
		{
			int statuscode = SERVER_UNDEFINED;
			char *message = NULL;
			Response_Msg pRespMsg;
			memset(&pRespMsg,0,sizeof(Response_Msg));
			pRespMsg.pServerList = &g_ServerList;
			ParseServerCtrl(data, datalen, &pRespMsg, g_PluginInfo.WorkDir);
			ServercontrolAction(&pRespMsg);
			if(pRespMsg.strMsg)
				free(pRespMsg.strMsg);
			if(pRespMsg.pServer)
				free(pRespMsg.pServer);
			break;
		}
		default: 
		{
			g_sendcbf(&g_PluginInfo, SERVERREDUNDANCY_ERROR_REP , "Unknown cmd!", strlen("Unknown cmd!")+1, NULL, NULL);
			break;
		}
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
int HANDLER_API Handler_Get_Capability( char ** pOutReply )
{
	int len = 0; // Data length of the pOutReply 
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
	return ;
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
	return ;
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