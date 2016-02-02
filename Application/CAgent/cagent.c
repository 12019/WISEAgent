/* This provides a crude manner of testing the performance of a broker in messages/s. */

#include <configuration.h>
#include <profile.h>
#include <XMLBase.h>
#include <SAGatherInfo.h>
#include <SAClient.h>
#include <pthread.h>
#include <string.h>
#include <stdio.h>
#include "util_path.h"
#include "AdvPlatform.h"

#include "cagentlog.h"
#include "version.h"
#include "service.h"
#include "cagent.h"

#define DEF_SUSIACCESSAGENT_CONFIG_NAME "agent_config.xml"
#define DEF_SUSIACCESSAGENT_DEFAULT_CONFIG_NAME "agent_config_def.xml"
#define DEF_SUSIACCESSAGENT_STATUS_NAME "agent_status"
typedef struct{
	pthread_t threadHandler;
	bool isThreadRunning;
	susiaccess_agent_conf_body_t config;
	susiaccess_agent_profile_body_t profile;
}connect_context_t;

bool g_bConnectRetry = false;
connect_context_t g_ConnContex;
char g_CAgentStatusPath[MAX_PATH] = {0};

void on_connect_cb();
void on_lost_connect_cb();
void on_disconnect_cb();


static void* ConnectThreadStart(void* args)
{
	bool bRet = false;
	connect_context_t *pConnContex = (connect_context_t *)args;

	if(pConnContex == NULL)
	{
		pthread_exit(0);
		return 0;
	}

	//SUSIAccessAgentLog(Normal, "Agent Connecting to broker: %s", pConnContex->config.serverIP);

//CONN_RETRY:
	/*if(!pConnContex->isThreadRunning)
	{
		app_os_thread_exit(-1);
		return -1;
	}*/

	bRet = saclient_connect() == saclient_success;

	/*if(!bRet){
		SUSIAccessAgentLog(Error, "Unable to connect to broker: %s.", pConnContex->config.serverIP);

		if(strcasecmp(pConnContex->config.autoStart, "True") == 0)
		{
			int i=5;
			while(pConnContex->isThreadRunning)
			{
				app_os_sleep(1000);
				if(i<=0)
					break;
				i--;
			}
			goto CONN_RETRY;
		}
	} else {
		SUSIAccessAgentLog(Normal, "Connect to broker: %s", pConnContex->config.serverIP);
		while(pConnContex->isThreadRunning)
		{
			app_os_sleep(1000);
		}
	}*/
	
	pthread_exit(0);
	return 0;
}

bool ConnectToServer(connect_context_t *pConnContex)
{
	bool bRet =false;
	if(pConnContex->isThreadRunning)
		return bRet;

	g_bConnectRetry = strcasecmp(pConnContex->config.autoStart, "True") == 0;
	if (pthread_create(&pConnContex->threadHandler, NULL, ConnectThreadStart, pConnContex) != 0)
	{
		pConnContex->isThreadRunning = false;
		SUSIAccessAgentLog(Error, "start Connecting thread failed!\n");	
		bRet = false;
	}
	else
	{
		pConnContex->isThreadRunning = true;
		bRet = true;
	}
	return bRet;
}

bool Disconnect()
{
	bool bRet = false;
	g_bConnectRetry = false;

	if(g_ConnContex.isThreadRunning == true)
	{
		g_ConnContex.isThreadRunning = false;
		pthread_join(g_ConnContex.threadHandler, NULL);
		g_ConnContex.threadHandler = 0;
	}
	saclient_disconnect();
	SUSIAccessAgentLog(Normal, "Agent Disconnected!");
	return bRet;
}

void WriteStatus(char * statusFileName, char* status)
{
	FILE * statusFile = NULL;
    statusFile = fopen(statusFileName, "w+");
	if(statusFile)
	{
		//fputs(status, statusFile);
		fprintf(statusFile, "%s\r\n", status);
		fflush(statusFile);
		fclose(statusFile);
	}
}

void on_connect_cb()
{
	WriteStatus(g_CAgentStatusPath, "1");
	SUSIAccessAgentLog(Normal, "Broker connected!");
}

void on_lost_connect_cb()
{
	WriteStatus(g_CAgentStatusPath, "0");
	if(g_bConnectRetry)
	{
		SUSIAccessAgentLog(Normal, "Broker Lostconnect, retry!");
		//saclient_reconnect();
	}
	else
	{
		SUSIAccessAgentLog(Normal, "Broker Lostconnect!");
	}
}

void on_disconnect_cb()
{
	WriteStatus(g_CAgentStatusPath, "0");
	SUSIAccessAgentLog(Normal, "Broker Disconnected!");
}

bool RestoreConfigDefault(char* workdir)
{
	char CAgentConfigPath[MAX_PATH] = {0};
	char CAgentDefConfigPath[MAX_PATH] = {0};
	FILE *pDefFile = NULL;
	FILE *pToFile = NULL;

	util_path_combine(CAgentConfigPath, workdir, DEF_SUSIACCESSAGENT_CONFIG_NAME);
	util_path_combine(CAgentDefConfigPath, workdir, DEF_SUSIACCESSAGENT_DEFAULT_CONFIG_NAME);

	pToFile = fopen( CAgentConfigPath, "w+" );
	if ( pToFile == 0 )
	{
		return false;
	}

	pDefFile = fopen( CAgentDefConfigPath, "r" );
	if ( pDefFile == 0 )
	{
		return false;
	}
	else
	{
		int x;
		while  ( ( x = fgetc( pDefFile ) ) != EOF )
		{
			fputc( x, pToFile);
		}
		fclose( pDefFile );
	}
	fclose( pToFile );
	return true;
}

int CAgentStart()
{
	int iRet = -1;
	//int port = 10001;
	char moudlePath[MAX_PATH] = {0};
	char CAgentConfigPath[MAX_PATH] = {0};
	bool bCfgRetry = true;
	//susiaccess_agent_t agentinfo;

	if(!SUSIAccessAgentLogHandle)
	{
		SUSIAccessAgentLogHandle = InitLog(moudlePath);
		SUSIAccessAgentLog(Normal, "Current path: %s", moudlePath);
	}
	memset(moudlePath, 0 , sizeof(moudlePath));
	util_module_path_get(moudlePath);

	util_path_combine(CAgentConfigPath, moudlePath, DEF_SUSIACCESSAGENT_CONFIG_NAME);
	util_path_combine(g_CAgentStatusPath, moudlePath, DEF_SUSIACCESSAGENT_STATUS_NAME);

	WriteStatus(g_CAgentStatusPath, "0");
	memset(&g_ConnContex, 0, sizeof(connect_context_t));

CFG_RETRY:
	if(!cfg_load(CAgentConfigPath, &g_ConnContex.config))
	{
		if(bCfgRetry)
		{
			SUSIAccessAgentLog(Error, "CFG load failed: %s, restore default!", CAgentConfigPath);
			RestoreConfigDefault(moudlePath);
			bCfgRetry = false;
			goto CFG_RETRY;
		}
		SUSIAccessAgentLog(Error, "Resotire CFG file failed, create new one!");
		if(!cfg_create(CAgentConfigPath, &g_ConnContex.config))
			SUSIAccessAgentLog(Error, "CFG create failed: %s", CAgentConfigPath);
		else
			SUSIAccessAgentLog(Normal, "CFG Created: %s", CAgentConfigPath);
	}
PROFILE_RETRY:
	if(!profile_load(CAgentConfigPath, &g_ConnContex.profile))
	{
		if(bCfgRetry)
		{
			SUSIAccessAgentLog(Error, "Profile load failed: %s, restore default!", CAgentConfigPath);
			RestoreConfigDefault(moudlePath);
			bCfgRetry = false;
			goto PROFILE_RETRY;
		}
		SUSIAccessAgentLog(Error, "Resotire Profile file failed, create new one!");

		if(!profile_create(CAgentConfigPath, &g_ConnContex.profile))
			SUSIAccessAgentLog(Error, "Profile create failed: %s", CAgentConfigPath);
		else
			SUSIAccessAgentLog(Normal, "Profile Created: %s", CAgentConfigPath);
	}
	else
	{
		char version[DEF_MAX_STRING_LENGTH] = {0};
		strcpy(g_ConnContex.profile.workdir, moudlePath);
		sagetInfo_gatherplatforminfo(&g_ConnContex.profile);
		snprintf(version, DEF_VERSION_LENGTH, "%d.%d.%d.%d", VER_MAJOR, VER_MINOR, VER_BUILD, VER_FIX);
		if(strcmp(g_ConnContex.profile.version, version))
		{
			strcpy(g_ConnContex.profile.version, version);
			profile_set(CAgentConfigPath, "SWVersion", g_ConnContex.profile.version);
		}

		if(strlen(g_ConnContex.config.identity)==0)
			strcpy(g_ConnContex.config.identity, g_ConnContex.profile.devId);
	}
	if(strlen(g_ConnContex.config.serverIP) <=0 )
	{
		SUSIAccessAgentLog(Error, "No Server IP!");	
		goto EXIT_START;
	}
	if(saclient_initialize(&g_ConnContex.config, &g_ConnContex.profile, SUSIAccessAgentLogHandle) != saclient_success)
	{
		SUSIAccessAgentLog(Error, "Unable to initialize AgentCore.");		
		saclient_uninitialize();
		SUSIAccessAgentLog(Error, "Agent initialize failed!");
		goto EXIT_START;
	}
	SUSIAccessAgentLog(Normal, "Agent Initialized");

	saclient_connection_callback_set(on_connect_cb, on_lost_connect_cb, on_disconnect_cb);

	iRet = ConnectToServer(&g_ConnContex)?0:-1;

EXIT_START:
	return iRet;
}

int CAgentStop()
{
	int iRet = 0;

	//Disconnect();
	saclient_uninitialize();
	WriteStatus(g_CAgentStatusPath, "0");

	if(g_ConnContex.isThreadRunning == true)
	{
		g_ConnContex.isThreadRunning = false;
		pthread_join(g_ConnContex.threadHandler, NULL);
		g_ConnContex.threadHandler = 0;
	}

	if(SUSIAccessAgentLogHandle!=NULL)
	{
		UninitLog(SUSIAccessAgentLogHandle);
		SUSIAccessAgentLogHandle = NULL;
	}
	return iRet;
}

int LoadServerName(char* configFile, char* name, int size)
{
	int iRet = 0;
	xml_doc_info * doc = xml_Loadfile(configFile, "XMLConfigSettings", "Customize");
	
	if(doc == NULL)
		return iRet;

	memset(name, 0, size);

	if(!xml_GetItemValue(doc, "ServiceName", name, size))
		memset(name, 0, size);
	else
		iRet = strlen(name);
	xml_FreeDoc(doc);
	return iRet;
}

int main(int argc, char *argv[])
{
	bool isSrvcInit = false;
	char moudlePath[MAX_PATH] = {0};
	char CAgentConfigPath[MAX_PATH] = {0};
	char serverName[DEF_OSVERSION_LEN] = {0};
	char version[DEF_VERSION_LENGTH] = {0};
#ifdef MEM_LEAK_CHECK
	_CrtSetDbgFlag ( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF ); 
	//_CrtSetBreakAlloc(3719);
	_CrtMemCheckpoint( &memStateStart);
#endif  
	sprintf(version, "%d.%d.%d.%d",VER_MAJOR, VER_MINOR, VER_BUILD, VER_FIX);

	memset(moudlePath, 0 , sizeof(moudlePath));
	
	util_module_path_get(moudlePath);

	
	/*Enable Log*/
	/*{
		FILE * logFile = NULL;
		util_path_combine(CAgentLogPath, "c:\\", DEF_SUSIACCESSAGENT_LOG_NAME);
		logFile = fopen(CAgentLogPath, "ab");
		if(logFile != NULL)
		{
			char logstring[MAX_PATH] = {0};
			sprintf(logstring, "Path: %s\r\n", moudlePath);
			fputs(logstring, logFile);
			fflush(logFile);
			fclose(logFile);
		}
	}*/
	SUSIAccessAgentLogHandle = InitLog(moudlePath);
	SUSIAccessAgentLog(Normal, "Current path: %s", moudlePath);

	/*Load service name fomr config and init service*/
	util_path_combine(CAgentConfigPath, moudlePath, DEF_SUSIACCESSAGENT_CONFIG_NAME);
	if(LoadServerName(CAgentConfigPath, serverName, sizeof(serverName))>0)
	{
		isSrvcInit = ServiceInit(serverName, version, CAgentStart, CAgentStop, SUSIAccessAgentLogHandle)==0?true:false;
	}
	else 
	{
		isSrvcInit = ServiceInit(NULL, version, CAgentStart, CAgentStop, SUSIAccessAgentLogHandle)==0?true:false;
	}

	
	if(!isSrvcInit) return -1;

	if(argv[1] != NULL)
	{
		bool bRet = false;
		char cmdChr[MAX_CMD_LEN] = {'\0'};
		memcpy(cmdChr, argv[1], strlen(argv[1]));
		do 
		{
			bRet = ExecuteCmd(strtok(cmdChr, "\n"))==0?true:false;
			if(!bRet) break;
			memset(cmdChr, 0, sizeof(cmdChr));
			fgets(cmdChr, sizeof(cmdChr), stdin);			
		} while (true);
	}
	else
	{
		if(isSrvcInit)
		{
			LaunchService();
		}
	}
	if(isSrvcInit) ServiceUninit();

	if(SUSIAccessAgentLogHandle!=NULL)
	{
		UninitLog(SUSIAccessAgentLogHandle);
		SUSIAccessAgentLogHandle = NULL;
	}

#ifdef MEM_LEAK_CHECK
	_CrtMemCheckpoint( &memStateEnd );
	if ( _CrtMemDifference( &memStateDiff, &memStateStart, &memStateEnd) )
		_CrtMemDumpStatistics( &memStateDiff );
#endif
	return 0;
}
