#include "susiaccess_def.h"
#include "version.h"
#include <SAClient.h>
#include "agentlog.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "util_path.h"

//-------------------------Memory leak check define--------------------------------
#ifdef MEM_LEAK_CHECK
#include <crtdbg.h>
_CrtMemState memStateStart, memStateEnd, memStateDiff;
#endif
//---------------------------------------------------------------------------------

void on_connect_cb()
{
	SUSIAccessAgentLog(Normal, "CB_Connected ");
}

void on_lost_connect_cb()
{
	SUSIAccessAgentLog(Normal, "CB_Lostconnect ");
}

void on_disconnect_cb()
{
	SUSIAccessAgentLog(Normal, "CB_Disconnect ");
}

void on_msgrecv(char* topic, susiaccess_packet_body_t *pkt, void *pRev1, void* pRev2)
{
	SUSIAccessAgentLog(Normal, "Packet received, %s", pkt->content);
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

	memset(&config, 0 , sizeof(susiaccess_agent_conf_body_t));
	strcpy(config.runMode,"remote");
	strcpy(config.autoStart,"True");
	strcpy(config.serverIP,"172.22.12.225");
	strcpy(config.serverPort,"1883");
	strcpy(config.serverAuth,"F0PE1/aaU8o=");
	config.tlstype = tls_type_none;
	switch(config.tlstype)
	{
	case tls_type_none:
		break;
	case tls_type_tls:
		{
			strcpy(config.cafile, "ca.crt");
			strcpy(config.capath, "");
			strcpy(config.certfile, "server.crt");
			strcpy(config.keyfile, "server.key");
			strcpy(config.cerpasswd, "123456");
		}
		break;
	case tls_type_psk:
		{
			strcpy(config.psk, "");
			strcpy(config.identity, "SAClientSample");
			strcpy(config.ciphers, "");
		}
		break;
	}

	memset(&profile, 0 , sizeof(susiaccess_agent_profile_body_t));
	sprintf_s(profile.version, DEF_VERSION_LENGTH, "%d.%d.%d.%d", 3, 1, 0, 0);
	strcpy(profile.hostname,"SAClientSample");
	strcpy(profile.devId,"000014DAE996BE04");
	strcpy(profile.sn,"14DAE996BE04");
	strcpy(profile.mac,"14DAE996BE04");
	strcpy(profile.type,"IPC");
	strcpy(profile.product,"Sample Agent");
	strcpy(profile.manufacture,"test");
	strcpy(profile.osversion,"NA");
	strcpy(profile.biosversion,"NA");
	strcpy(profile.platformname,"NA");
	strcpy(profile.processorname,"NA");
	strcpy(profile.osarchitect,"NA");
	profile.totalmemsize = 40832;
	strcpy(profile.maclist,"14DAE996BE04");
	strcpy(profile.localip,"172.22.12.21");
	strcpy(profile.account,"anonymous");
	strcpy(profile.passwd,"");

	iRet = saclient_initialize(&config, &profile, SUSIAccessAgentLogHandle);

	if(iRet != saclient_success)
	{
		SUSIAccessAgentLog(Error, "Unable to initialize AgentCore.");
		goto EXIT;
	}
	SUSIAccessAgentLog(Normal, "Agent Initialized");

	saclient_connection_callback_set(on_connect_cb, on_lost_connect_cb, on_disconnect_cb);

	SUSIAccessAgentLog(Normal, "Agent Set Callback");
	
	iRet = saclient_connect();

	if(iRet != saclient_success){
		SUSIAccessAgentLog(Error, "Unable to connect to broker.");
		goto EXIT;
	} else {
		SUSIAccessAgentLog(Normal, "Connect to broker: %s", config.serverIP);
	}

	printf("Click enter to reconnect");
	fgetc(stdin);

	saclient_server_connect("172.22.12.208", 1883, "F0PE1/aaU8o=");

	{
		
		char topicStr[128] = {0};
		susiaccess_packet_body_t pkt;

		/* Add  subscribe topic Callback*/
		sprintf(topicStr, "/cagent/admin/%s/testreq", profile.devId);
		saclient_subscribe(topicStr, on_msgrecv);
		
		/*Send packet to specific topic*/
		strcpy(pkt.devId, profile.devId);
		strcpy(pkt.handlerName, "Test");
		//packet.catalogID = cagent_catalog_susi_func;
		pkt.requestID = 0;
		pkt.cmd = 0;
		pkt.content = malloc(strlen("{\"Test\":100}")+1);
		memset(pkt.content, 0, strlen("{\"Test\":100}")+1);
		strcpy(pkt.content, "{\"Test\":100}");
		saclient_publish(topicStr, &pkt);
		free(pkt.content);
	}

	
EXIT:
	printf("Click enter to exit");
	fgetc(stdin);

	saclient_disconnect();
	SUSIAccessAgentLog(Normal, "Send Client Info: disconnect");
	saclient_uninitialize();

	UninitLog(SUSIAccessAgentLogHandle);
#ifdef MEM_LEAK_CHECK
	_CrtMemCheckpoint( &memStateEnd );
	if ( _CrtMemDifference( &memStateDiff, &memStateStart, &memStateEnd) )
		_CrtMemDumpStatistics( &memStateDiff );
#endif

	return iRet;
}

