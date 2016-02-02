#include "susiaccess_def.h"
#include "version.h"
#include "agentlog.h"
#include <SALoader.h>
#include <SAGeneralHandler.h>
#include <string.h>
#include <stdio.h>
#include "util_path.h"
#include "AdvPlatform.h"

//-------------------------Memory leak check define--------------------------------
#ifdef MEM_LEAK_CHECK
#include <crtdbg.h>
_CrtMemState memStateStart, memStateEnd, memStateDiff;
#endif
//---------------------------------------------------------------------------------

AGENT_SEND_STATUS HandlerSendMessage( HANDLE const handle, int enum_act, 
										  char const * const requestData, unsigned int const requestLen, 
										  void *pRev1, void* pRev2 )
{
	AGENT_SEND_STATUS result = cagent_send_data_error;
	SUSIAccessAgentLog(Normal, "Send Message: %s", requestData);
	return result;
}

int main(int argc, char *argv[])
{
	int iRet = 0;
	char moudlePath[MAX_PATH] = {0};

	susiaccess_agent_conf_body_t config;
	susiaccess_agent_profile_body_t profile;
	Handler_List_t handlerList;

#ifdef MEM_LEAK_CHECK
	_CrtSetDbgFlag ( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF ); 
	//_CrtSetBreakAlloc(3719);
	_CrtMemCheckpoint( &memStateStart);
#endif

	memset(moudlePath, 0 , sizeof(moudlePath));
	util_module_path_get(moudlePath);

	SUSIAccessAgentLogHandle = InitLog(moudlePath);
	SUSIAccessAgentLog(Normal, "Current path: %s", moudlePath);

	/*Init Config and Profile*/
	memset(&config, 0, sizeof(susiaccess_agent_conf_body_t));
	strcpy(config.runMode,"remote");
	//strcpy(config.lunchConnect,"True");
	strcpy(config.autoStart,"True");
	//strcpy(config.autoReport,"False");
	strcpy(config.serverIP,"172.22.12.93");
	strcpy(config.serverPort,"10001");

	memset(&profile, 0, sizeof(susiaccess_agent_profile_body_t));
	snprintf(profile.version, DEF_VERSION_LENGTH, "%d.%d.%d.%d", VER_MAJOR, VER_MINOR, VER_BUILD, VER_FIX);
	strcpy(profile.hostname,"SAClientTest");
	strcpy(profile.devId,"000014DAE996BE04");
	strcpy(profile.sn,"14DAE996BE04");
	strcpy(profile.mac,"14DAE996BE04");
	strcpy(profile.lal,"121.3355, 25.0514");
	strcpy(profile.type,"IPC");
	strcpy(profile.product,"SUSIAccess Agent");
	strcpy(profile.manufacture,"test");
	strcpy(profile.osversion,"NA");
	strcpy(profile.biosversion,"NA");
	strcpy(profile.platformname,"NA");
	strcpy(profile.processorname,"NA");
	strcpy(profile.osarchitect,"NA");
	profile.totalmemsize = 40832;
	strcpy(profile.maclist,"14DAE996BE04");
	strcpy(profile.localip,"172.22.12.21");

	memset(&handlerList, 0, sizeof(Handler_List_t));

	Loader_Initialize(moudlePath, &config, &profile, SUSIAccessAgentLogHandle);

	Loader_LoadAllHandler(&handlerList, moudlePath);

	{
		Handler_Loader_Interface GlobalPlugin;
		memset(&GlobalPlugin, 0, sizeof(Handler_Loader_Interface));
		Loader_GetBasicHandlerLoaderInterface(&GlobalPlugin);
		strcpy(GlobalPlugin.Name, "general");
		if(GlobalPlugin.pHandlerInfo)
		{
			strncpy(GlobalPlugin.pHandlerInfo->ServerIP,  config.serverIP, strlen(config.serverIP)+1);
			GlobalPlugin.pHandlerInfo->ServerPort = atol(config.serverPort);
		}
		GlobalPlugin.Handler_Recv_API = (HANDLER_RECV)&General_HandleRecv;
		GlobalPlugin.Handler_OnStatusChange_API = (HANDLER_ONSTATUSCAHNGE)&General_OnStatusChange;
		GlobalPlugin.type = core_handler;

		General_Initialize(GlobalPlugin.pHandlerInfo);

		//General_SetSendCB(HandlerSendMessage);

		General_SetPluginHandlers(&handlerList);
		//gbl_SetSendCB((HandlerSendCbf) &core_HandlerSendMessage);
		if(Loader_AddHandler(&handlerList, &GlobalPlugin)<=0)
			SUSIAccessAgentLog(Warning, "Add Global Handler failed!");
	}
	printf("General_Start\n\r");
	General_Start();
	printf("Loader_SetAgentStatus\n\r");
	Loader_SetAgentStatus(&handlerList, &config, 1);

	printf("Click enter to stop");
	fgetc(stdin);
	General_Stop();
	General_Uninitialize();

	Loader_StopAllHandler(&handlerList);

	Loader_ReleaseAllHandler(&handlerList);

	Loader_Uninitialize();
	UninitLog(SUSIAccessAgentLogHandle);

	printf("Click enter to exit");
	fgetc(stdin);

#ifdef MEM_LEAK_CHECK
	_CrtMemCheckpoint( &memStateEnd );
	if ( _CrtMemDifference( &memStateDiff, &memStateStart, &memStateEnd) )
		_CrtMemDumpStatistics( &memStateDiff );
#endif

	return 0;
}

