#include "susiaccess_def.h"
#include "version.h"
#include "agentlog.h"
#include <SAManager.h>
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
#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once

#ifndef TEST_API
#define TEST_API WINAPI
#endif
#else
#define TEST_API
#endif

typedef void (*TEST_MESSAGE_RECV_CALLBACK)(char* topic, susiaccess_packet_body_t *pkt, void *pRev1, void* pRev2);

int TEST_API test_publish(char const * topic, susiaccess_packet_body_t const * pkt)
{

}

int TEST_API test_subscribe(char const * topic, TEST_MESSAGE_RECV_CALLBACK msg_recv_callback)
{

}

int main(int argc, char *argv[])
{
	int iRet = 0;
	char moudlePath[MAX_PATH] = {0};

	susiaccess_agent_conf_body_t config;
	susiaccess_agent_profile_body_t profile;

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
	strcpy(profile.workdir, moudlePath);

	SAManager_Initialize(&config, &profile, SUSIAccessAgentLogHandle);
	SAManager_SetPublishCB(test_publish);
	SAManager_SetSubscribeCB(test_subscribe);
	SAManager_InternalSubscribe(&profile);
	SAManager_UpdateConnectState(&config, AGENT_STATUS_ONLINE);

	printf("Click enter to stop");
	fgetc(stdin);
	SAManager_Uninitialize();
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

