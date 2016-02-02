// SampleClient.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <SAClient.h>
#include <Windows.h>
#include <Log.h>

#define SAMPLE_LOG_ENABLE
//#define DEF_SAMPLE_LOG_MODE    (LOG_MODE_NULL_OUT)
//#define DEF_SAMPLE_LOG_MODE    (LOG_MODE_FILE_OUT)
#define DEF_SAMPLE_LOG_MODE    (LOG_MODE_CONSOLE_OUT|LOG_MODE_FILE_OUT)

LOGHANDLE g_samolelog = NULL;

#ifdef SAMPLE_LOG_ENABLE
#define SampleLog(level, fmt, ...)  do { if (g_samolelog != NULL)   \
	WriteLog(g_samolelog, DEF_SAMPLE_LOG_MODE, level, fmt, ##__VA_ARGS__); } while(0)
#else
#define SampleLog(level, fmt, ...)
#endif


void on_connect_cb()
{
	printf("CB_Connected \r\n");
}

void on_lost_connect_cb()
{
	printf("CB_Lostconnect \r\n");
}

void on_disconnect_cb()
{
	printf("CB_Disconnect \r\n");
}

void on_msgrecv(char* topic, susiaccess_packet_body_t *pkt, void *pRev1, void* pRev2)
{
	printf("Packet received, %s\r\n", pkt->content);
}

int module_path_get(char * moudlePath)
{
	int iRet = 0;
	char * lastSlash = NULL;
	char tempPath[MAX_PATH] = {0};
	if(ERROR_SUCCESS != GetModuleFileName(NULL, tempPath, sizeof(tempPath)))
	{
		lastSlash = strrchr(tempPath, '\\');
		if(NULL != lastSlash)
		{
			if(moudlePath)
				strncpy(moudlePath, tempPath, lastSlash - tempPath + 1);
			iRet = lastSlash - tempPath + 1;
		}
	}
	return iRet;
}

int _tmain(int argc, _TCHAR* argv[])
{
	int iRet = 0;
	susiaccess_agent_conf_body_t config;
	susiaccess_agent_profile_body_t profile;
	char moudlePath[MAX_PATH] = {0};

	memset(moudlePath, 0 , sizeof(moudlePath));
	module_path_get(moudlePath);
		
	//path_combine(CAgentLogPath, moudlePath, DEF_SUSIACCESSAGENT_LOG_NAME);
	
	g_samolelog = InitLog(moudlePath);
	SampleLog(Debug, "Current path: %s", moudlePath);

	memset(&config, 0 , sizeof(susiaccess_agent_conf_body_t));
	strcpy(config.runMode,"remote");
	strcpy(config.autoStart,"True");
	strcpy(config.serverIP,"172.22.12.199");
	strcpy(config.serverPort,"10001");
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

	iRet = saclient_initialize(&config, &profile, g_samolelog);

	if(iRet != saclient_success)
	{
		printf("Unable to initialize AgentCore.\r\n");
		goto EXIT;
	}
	printf("Agent Initialized\r\n");

	saclient_connection_callback_set(on_connect_cb, on_lost_connect_cb, on_disconnect_cb);

	printf("Agent Set Callback");
	
	iRet = saclient_connect();

	if(iRet != saclient_success){
		printf("Unable to connect to broker.\r\n");
		goto EXIT;
	} else {
		printf("Connect to broker: %s\r\n", config.serverIP);
	}

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
		pkt.content = (char*)malloc(strlen("{\"Test\":100}")+1);
		memset(pkt.content, 0, strlen("{\"Test\":100}")+1);
		strcpy(pkt.content, "{\"Test\":100}");
		saclient_publish(topicStr, &pkt);
		free(pkt.content);
	}

EXIT:
	printf("Click enter to exit");
	fgetc(stdin);

	saclient_disconnect();
	printf("Send Client Info: disconnect\r\n");
	saclient_uninitialize();

	return iRet;
}

