/****************************************************************************/
/* Copyright(C) : Advantech Technologies, Inc.								       */
/* Create Date  : 2014/11/25 by li.ge								                */
/* Modified Date: 2014/11/25 by li.ge								                */
/* Abstract     : RemoteKVM  Handler API                                    */
/* Reference    : None														             */
/****************************************************************************/
#include "network.h"
#include "platform.h"
#include "susiaccess_handler_api.h"
#include "RemoteKVMHandler.h"
#include "RemoteKVMLog.h"
#include "Parser.h"
#include "common.h"
#include "cJSON.h"
#include "kvmconfig.h"

//-----------------------------------------------------------------------------
// Types and defines:
//-----------------------------------------------------------------------------
#define DEF_HANDLER_NAME             "remote_kvm"
const char strPluginName[MAX_TOPIC_LEN] = {"remote_kvm"};
const int iRequestID = cagent_request_remote_kvm;
const int iActionID = cagent_reply_remote_kvm;
static Handler_info  g_PluginInfo;
static void* g_loghandle = NULL;
static bool g_bEnableLog = true;
static HandlerSendCbf  g_sendcbf = NULL;						// Client Send information (in JSON format) to Cloud Server	
static HandlerSendCustCbf  g_sendcustcbf = NULL;			    // Client Send information (in JSON format) to Cloud Server with custom topic	
static HandlerAutoReportCbf g_sendreportcbf = NULL;				// Client Send report (in JSON format) to Cloud Server with AutoReport topic	
static HandlerSendCapabilityCbf g_sendcapabilitycbf = NULL;	
static HandlerSubscribeCustCbf g_subscribecustcbf = NULL;

kvm_capability_info_t cpbInfo;
susiaccess_kvm_conf_body_t kvm_conf;

#define  vncServerIP                         vncConnectParams.vnc_server_ip      
#define  vncServerPort                       vncConnectParams.vnc_server_port    
#define  vncPassword                         vncConnectParams.vnc_password 
 
#define  DEF_VNC_SERVER_PORT                 5902
#define  DEF_VNC_SERVER_PWD                  "na"
#define  DEF_VNC_FOLDER_NAME                 "VNC"
static void * KVMGetConnectParamsThreadHandle = NULL;
static BOOL IsKVMGetConnectParamsThreadRunning = FALSE;
#ifdef WIN32
#define  DEF_VNC_INI_FILE_NAME               "ultravnc.ini"
#define  DEF_VNC_SERVICE_NAME                "uvnc_sa30"
#define  DEF_VNC_SERVER_EXE_NAME             "winvnc.exe"
#define  DEF_VNC_CHANGE_PWD_EXE_NAME         "vncpwdchg.exe"
#else
#include <sys/wait.h>
#define  DEF_VNC_SERVER_EXE_NAME             "x11vnc"
static int X11vncPid = 0;
#endif
#define  DEF_CONFIG_FILE_NAME         "agent_config.xml"

#define  DEF_VNC_SERVER_INSTALL              0
#define  DEF_VNC_SERVER_START				 1
#define  DEF_VNC_SERVER_STOP                 2
#define  DEF_VNC_SERVER_UNINSTALL            3

char VNCServerPath[MAX_PATH] = {0};
char ConfigFilePath[MAX_PATH] = {0};
#ifdef WIN32
char VNCIniFilePath[MAX_PATH] = {0};
char VNCChangePwdExePath[MAX_PATH] = {0};
#endif

kvm_vnc_server_start_params* g_pkvmVNCServerStartParms = NULL;

static void InitRemoteKVMPath();
static void KVMGetConnectParams(kvm_vnc_server_start_params * pVNCStartServerParams);
#ifdef WIN32
static BOOL IsSvcRun(LPCTSTR  lpszSvcName);
static BOOL IsSvcStop(LPCTSTR  lpszSvcName);
static void VNCServiceAction(int action);
static BOOL GetIniValue(char * iniPath, char * keyWord, char * valueStr);
static BOOL GetVNCServerPort(unsigned int * srvPort);
static BOOL StopWinVNCServer();
static BOOL ChangeVNCPassword(char* vncPwd, kvm_vnc_server_start_params* pVNCStartServerParams);
static void GenVNCPassword(kvm_vnc_server_start_params * pVNCStartServerParams);
#else

#endif

static void CollectCpbInfo(kvm_capability_info_t * pCpbInfo);
static void GetCapability();

#ifdef WIN32
static void InitRemoteKVMPath()
{
	char modulePath[MAX_PATH] = {0};
	char vncPath[MAX_PATH] = {0};
	if(strlen(g_PluginInfo.WorkDir)<=0) return;
	strcpy(modulePath, g_PluginInfo.WorkDir);
	path_combine(vncPath, modulePath, DEF_VNC_FOLDER_NAME);

	path_combine(VNCIniFilePath, vncPath, DEF_VNC_INI_FILE_NAME);
	path_combine(VNCServerPath, vncPath, DEF_VNC_SERVER_EXE_NAME);
	path_combine(VNCChangePwdExePath, vncPath, DEF_VNC_CHANGE_PWD_EXE_NAME);

	path_combine(ConfigFilePath, modulePath, DEF_CONFIG_FILE_NAME);
	/*sprintf(VNCIniFilePath, "%s\\%s\\%s", modulePath, DEF_VNC_FOLDER_NAME, DEF_VNC_INI_FILE_NAME);
	sprintf(VNCServerPath, "%s\\%s\\%s", modulePath, DEF_VNC_FOLDER_NAME, DEF_VNC_SERVER_EXE_NAME);
	sprintf(VNCChangePwdExePath, "%s\\%s\\%s", modulePath, DEF_VNC_FOLDER_NAME, DEF_VNC_CHANGE_PWD_EXE_NAME);*/
}

static void VNCServiceAction(int action)
{
	STARTUPINFO si;
	PROCESS_INFORMATION pi;
	char cmdLine[BUFSIZ] = "cmd.exe /c ";
	memset(&si, 0, sizeof(si));
	si.dwFlags = STARTF_USESHOWWINDOW;  
	si.wShowWindow = SW_HIDE;
	si.cb = sizeof(si);
	memset(&pi, 0, sizeof(pi));

	switch(action)
	{
	case DEF_VNC_SERVER_INSTALL:
		{
			sprintf(cmdLine, "%s \"%s\"  %s", cmdLine, VNCServerPath, "-install");
			break;
		}
	case DEF_VNC_SERVER_UNINSTALL:
		{
			sprintf(cmdLine, "%s \"%s\"  %s", cmdLine, VNCServerPath, "-uninstall");
			break;
		}
	case DEF_VNC_SERVER_START:
		{
			sprintf(cmdLine, "%s \"%s\"  %s", cmdLine, VNCServerPath, "-startservice");
			break;
		}
	case DEF_VNC_SERVER_STOP:
		{
			sprintf(cmdLine, "%s \"%s\"  %s", cmdLine, VNCServerPath, "-stopservice");
			break;
		}
	}

	app_os_CreateProcess(cmdLine);
	app_os_WaitForSingleObject(pi.hProcess, INFINITE);

	switch(action)
	{
	case DEF_VNC_SERVER_START:
		while(IsSvcRun(DEF_VNC_SERVICE_NAME) != 1)
		{
			app_os_sleep(100);
		}
		break;
	case DEF_VNC_SERVER_STOP:
		while(IsSvcStop(DEF_VNC_SERVICE_NAME) != 1)
		{
			app_os_sleep(100);
		}
		break;
	}
	app_os_CloseHandle(pi.hProcess);
	app_os_CloseHandle(pi.hThread);
}

static BOOL IsSvcRun(LPCTSTR  lpszSvcName)
{	
	SERVICE_STATUS svcStatus = {0};
	return QueryServiceStatus(OpenService(OpenSCManager(NULL, NULL, GENERIC_READ), lpszSvcName, GENERIC_READ), &svcStatus) ? (svcStatus.dwCurrentState == SERVICE_RUNNING) : 0;	
}

static BOOL IsSvcStop(LPCTSTR  lpszSvcName)
{	
	SERVICE_STATUS svcStatus = {0};
	return QueryServiceStatus(OpenService(OpenSCManager(NULL, NULL, GENERIC_READ), lpszSvcName, GENERIC_READ), &svcStatus) ? (svcStatus.dwCurrentState == SERVICE_STOPPED) : 0;	
}

static BOOL GetIniValue(char * iniPath, char * keyWord, char * valueStr)
{
	BOOL bRet = FALSE;
	if(NULL == iniPath || NULL == keyWord || NULL == valueStr) return bRet;
	if(app_os_is_file_exist(iniPath))
	{
		FILE * pVNCIniFile = NULL;
		pVNCIniFile = fopen(iniPath, "rb");
		if(pVNCIniFile)
		{
			char lineBuf[512] = {0};

			while(!feof(pVNCIniFile))
			{
				memset(lineBuf, 0, sizeof(lineBuf));
				if(fgets(lineBuf, sizeof(lineBuf), pVNCIniFile))
				{
					if(strstr(lineBuf, keyWord))
					{
						char * word[16] = {NULL};
						char *buf = lineBuf;
						int i = 0, wordIndex = 0;
						while((word[i] = strtok(buf, "=")) != NULL)
						{
							i++;
							buf=NULL; 
						}
						while(word[wordIndex] != NULL)
						{
							TrimStr(word[wordIndex]);
							wordIndex++;
						}
						if(wordIndex < 2) continue;
						if(!strcmp(keyWord, word[0]))
						{
							strcpy(valueStr, word[1]);
							bRet = TRUE;
							break;
						}
					}
				}
			}
			fclose(pVNCIniFile);
		}
	}
	return bRet;
}

static BOOL GetVNCServerPort(unsigned int * srvPort)
{
	BOOL bRet = FALSE;
	if(NULL == srvPort) return bRet;
	{
		char portStr[16] = {0};
		if(GetIniValue(VNCIniFilePath, "PortNumber", portStr))
		{
			if(strlen(portStr))
			{
				*srvPort = atoi(portStr);
				bRet = TRUE;
			}
		}
	}
	return bRet;
}

static BOOL ChangeVNCPassword(char* vncPwd, kvm_vnc_server_start_params* pVNCStartServerParams)
{
	BOOL bRet = FALSE;
	char cmdLine[BUFSIZ] = "cmd.exe /c ";
	STARTUPINFO si;
	PROCESS_INFORMATION pi;
	if(NULL == vncPwd) return bRet;

	memset(&si, 0, sizeof(si));
	si.dwFlags = STARTF_USESHOWWINDOW;  
	si.wShowWindow = SW_HIDE;
	si.cb = sizeof(si);
	memset(&pi, 0, sizeof(pi));

	if(pVNCStartServerParams->mode == 1)
	{
		sprintf(cmdLine, "%s \"%s\"  %s", cmdLine, VNCChangePwdExePath, vncPwd);
	}
	else if(pVNCStartServerParams->mode == 3)
	{	
		sprintf(cmdLine, "%s \"%s\" %s %d %s %d", cmdLine, VNCChangePwdExePath, vncPwd, pVNCStartServerParams->vnc_server_repeater_id, g_PluginInfo.ServerIP, pVNCStartServerParams->vnc_server_repeater_port);
	}

	if(!app_os_CreateProcess(cmdLine))
	{
		RemoteKVMLog(g_loghandle, Normal, "%s()[%d]###Create restore bat process failed!\n", __FUNCTION__, __LINE__);
	}
	else
	{
		app_os_WaitForSingleObject(pi.hProcess, INFINITE);
		app_os_CloseHandle(pi.hProcess);
		app_os_CloseHandle(pi.hThread);
		bRet = TRUE;
	}
	return bRet;
}

static void GenVNCPassword(kvm_vnc_server_start_params * pVNCStartServerParams)
{
	remote_kvm_handler_context_t *pRemoteKVMHandlerContext = (remote_kvm_handler_context_t *)&RemoteKVMHandlerContext; 
	VNCServiceAction(DEF_VNC_SERVER_UNINSTALL);
	if(pVNCStartServerParams->need_change_password == 1)
	{
		memset(pRemoteKVMHandlerContext->vncPassword, 0, sizeof(pRemoteKVMHandlerContext->vncPassword));
		app_os_GetRandomStr(pRemoteKVMHandlerContext->vncPassword, sizeof(pRemoteKVMHandlerContext->vncPassword));
		ChangeVNCPassword(pRemoteKVMHandlerContext->vncPassword, pVNCStartServerParams);
	}	
	VNCServiceAction(DEF_VNC_SERVER_INSTALL);
}

static CAGENT_PTHREAD_ENTRY(KVMGetConnectParamsThreadStart, args)
{
	if(IsKVMGetConnectParamsThreadRunning)
	{
		remote_kvm_handler_context_t *pRemoteKVMHandlerContext = (remote_kvm_handler_context_t *)&RemoteKVMHandlerContext;
        kvm_vnc_server_start_params * pVNCStartServerParams = (kvm_vnc_server_start_params *)args;

		GenVNCPassword(pVNCStartServerParams);

		if(strlen(pRemoteKVMHandlerContext->vncServerIP) == 0)
		{
			//app_os_GetHostIP(pRemoteKVMHandlerContext->vncServerIP);
			network_ip_get(pRemoteKVMHandlerContext->vncServerIP, sizeof(pRemoteKVMHandlerContext->vncServerIP));
		}

		if(pRemoteKVMHandlerContext->vncServerPort == 0)
		{
			pRemoteKVMHandlerContext->vncServerPort = DEF_VNC_SERVER_PORT;
			GetVNCServerPort(&pRemoteKVMHandlerContext->vncServerPort);
		}

		{
			char * paramsRepJsonStr = NULL;
			kvm_vnc_connect_params * pConnectParams = &pRemoteKVMHandlerContext->vncConnectParams;
			int jsonStrlen = Parser_PackKVMGetConnectParamsRep(pConnectParams, &paramsRepJsonStr);
			if(jsonStrlen > 0 && paramsRepJsonStr != NULL)
			{
				g_sendcbf(&g_PluginInfo, kvm_get_connect_params_rep, paramsRepJsonStr, strlen(paramsRepJsonStr)+1, NULL, NULL);
			}
			if(paramsRepJsonStr)free(paramsRepJsonStr);	
		}
	}
	if(g_pkvmVNCServerStartParms)
	{
		free(g_pkvmVNCServerStartParms);
		g_pkvmVNCServerStartParms = NULL;
	}
	IsKVMGetConnectParamsThreadRunning = FALSE;
	return 0;
}

static void KVMGetConnectParams(kvm_vnc_server_start_params * pVNCStartServerParams)
{
	if(!IsKVMGetConnectParamsThreadRunning)
	{
		IsKVMGetConnectParamsThreadRunning = TRUE;
		if(!g_pkvmVNCServerStartParms)
			g_pkvmVNCServerStartParms = malloc(sizeof(kvm_vnc_server_start_params));
		memset(g_pkvmVNCServerStartParms, 0, sizeof(kvm_vnc_server_start_params));
		memcpy(g_pkvmVNCServerStartParms, pVNCStartServerParams,  sizeof(kvm_vnc_server_start_params));

		if (app_os_thread_create(&KVMGetConnectParamsThreadHandle, KVMGetConnectParamsThreadStart, g_pkvmVNCServerStartParms) != 0)
		{
			IsKVMGetConnectParamsThreadRunning = FALSE;
		}
	}
}
#else
static void InitRemoteKVMPath()
{
	char modulePath[MAX_PATH] = {0};
	/*if(!app_os_get_module_path(modulePath)) return;
	sprintf(VNCServerPath, "%s/%s/%s", modulePath, DEF_VNC_FOLDER_NAME, DEF_VNC_SERVER_EXE_NAME);*/
	char vncPath[MAX_PATH] = {0};
	if(strlen(g_PluginInfo.WorkDir)<=0) return;
	strcpy(modulePath, g_PluginInfo.WorkDir);
	path_combine(vncPath, modulePath, DEF_VNC_FOLDER_NAME);

	path_combine(VNCServerPath, vncPath, DEF_VNC_SERVER_EXE_NAME);

	path_combine(ConfigFilePath, modulePath, DEF_CONFIG_FILE_NAME);
}

static void KillX11Vnc()
{
	FILE *fp = NULL;
	fp = popen("ps aux | grep 'x11vnc' | cut -c 9-15 | xargs kill -9", "r");
	pclose(fp);
}

static char * GetVncAuth()
{
	char *vncauth = NULL;
	char line[64] = {0};
	FILE *fp = NULL;

	fp = popen("ps wwaux | grep '/X.*-auth' | grep -v grep | sed -e 's/^.*-auth *//' -e 's/ .*$//' | head -n 1", "r");
	while(fgets(line, sizeof(line), fp) != NULL)
	{
		vncauth = strndup(line, strlen(line) - 1);
	}
	pclose(fp);

	return vncauth;
}

static int StartX11Vnc(kvm_vnc_server_start_params * pVNCStartServerParams)
{	
	if(pVNCStartServerParams == NULL) return -1;
	{
		remote_kvm_handler_context_t *pRemoteKVMHandlerContext = (remote_kvm_handler_context_t *)&RemoteKVMHandlerContext;
		int ret = 0;
		char runVncParams[128] = {0};
 		char * vncAuth = NULL;
 		vncAuth = GetVncAuth();
		switch(pVNCStartServerParams->mode)
		{
		case 1: //normal
			{
				if(vncAuth != NULL)
				{
					sprintf(runVncParams, "-display :0 -auth %s -rfbport %d -passwd %s ", 
						vncAuth, pRemoteKVMHandlerContext->vncServerPort, pRemoteKVMHandlerContext->vncPassword);
				}
				else
				{
					sprintf(runVncParams, "-rfbport %d -passwd %s", 
						pRemoteKVMHandlerContext->vncModeParams.custvnc_port, pRemoteKVMHandlerContext->vncModeParams.custvnc_pwd);
				}
				break;
			}
		case 2://listen ./x11vnc -connect 172.21.73.71:5901
			{
				if(vncAuth != NULL)
				{
					sprintf(runVncParams, "-display :0 -auth %s -connect %s:%d", 
						vncAuth, pVNCStartServerParams->vnc_server_start_listen_ip, pVNCStartServerParams->vnc_server_listen_port);
				}
				else
				{
					sprintf(runVncParams, "-connect %s:%d", 
						pVNCStartServerParams->vnc_server_start_listen_ip, pVNCStartServerParams->vnc_server_listen_port);
				}
				break;
			}
		case 3://repeater ./x11vnc -connect repeater://172.21.73.148:5501+ID:2222
			{
				if(vncAuth != NULL)
				{
					sprintf(runVncParams, "-display :0 -auth %s -connect repeater://%s:%d+ID:%d -passwd %s", 
						vncAuth, pRemoteKVMHandlerContext->vncServerIP, pVNCStartServerParams->vnc_server_repeater_port, pVNCStartServerParams->vnc_server_repeater_id, pRemoteKVMHandlerContext->vncPassword);
				}
				else
				{
					sprintf(runVncParams, "-connect repeater://%s:%d+ID:%d -passwd %s", 
						pRemoteKVMHandlerContext->vncServerIP, pVNCStartServerParams->vnc_server_repeater_port, pVNCStartServerParams->vnc_server_repeater_id, pRemoteKVMHandlerContext->vncPassword);
				}
				break;
			}
		default: break;
		}
		if(vncAuth) free(vncAuth);
		//printf("VNCServerPath:%s, runVncParams:%s, pVNCStartServerParams->mode:%d\n",VNCServerPath, runVncParams,pVNCStartServerParams->mode);
		ret = app_os_RunX11vnc(VNCServerPath, runVncParams);
		return ret;
	}
}

static CAGENT_PTHREAD_ENTRY(KVMGetConnectParamsThreadStart, args)
{
	if(IsKVMGetConnectParamsThreadRunning)
	{
		remote_kvm_handler_context_t *pRemoteKVMHandlerContext = (remote_kvm_handler_context_t *)&RemoteKVMHandlerContext;
		kvm_vnc_server_start_params * pVNCStartServerParams = (kvm_vnc_server_start_params *)args;
		if(pVNCStartServerParams->need_change_password == 1)
		{
			memset(pRemoteKVMHandlerContext->vncPassword, 0, sizeof(pRemoteKVMHandlerContext->vncPassword));
			app_os_GetRandomStr(pRemoteKVMHandlerContext->vncPassword, sizeof(pRemoteKVMHandlerContext->vncPassword));
		}
		if(strlen(pRemoteKVMHandlerContext->vncServerIP) == 0)
		{
			//strcpy(pRemoteKVMHandlerContext->vncServerIP, "172.21.73.200");
			//app_os_GetHostIP(pRemoteKVMHandlerContext->vncServerIP);
			network_ip_get(pRemoteKVMHandlerContext->vncServerIP, sizeof(pRemoteKVMHandlerContext->vncServerIP));
			printf("ip:%s\n", pRemoteKVMHandlerContext->vncServerIP);
		}
		X11vncPid = StartX11Vnc(pVNCStartServerParams);
		{
			char * paramsRepJsonStr = NULL;
			kvm_vnc_connect_params * pConnectParams = &pRemoteKVMHandlerContext->vncConnectParams;
			int jsonStrlen = Parser_PackKVMGetConnectParamsRep(pConnectParams, &paramsRepJsonStr);
			if(jsonStrlen > 0 && paramsRepJsonStr != NULL)
			{
				g_sendcbf(&g_PluginInfo, kvm_get_connect_params_rep, paramsRepJsonStr, strlen(paramsRepJsonStr)+1, NULL, NULL);
			}
			if(paramsRepJsonStr)free(paramsRepJsonStr);	
		}
		if(X11vncPid > 0)
		{
			int wPid = 0;
			wPid = waitpid(X11vncPid,NULL, WNOHANG);
			if(wPid == X11vncPid) X11vncPid = 0;
		}
	}
	if(g_pkvmVNCServerStartParms)
	{
		free(g_pkvmVNCServerStartParms);
		g_pkvmVNCServerStartParms = NULL;
	}
	IsKVMGetConnectParamsThreadRunning = FALSE;
	return 0;
}

static void KVMGetConnectParams(kvm_vnc_server_start_params * pVNCStartServerParams)
{
	if(!IsKVMGetConnectParamsThreadRunning)
	{
		if(!g_pkvmVNCServerStartParms)
			g_pkvmVNCServerStartParms = malloc(sizeof(kvm_vnc_server_start_params));
		memset(g_pkvmVNCServerStartParms, 0, sizeof(kvm_vnc_server_start_params));
		memcpy(g_pkvmVNCServerStartParms, pVNCStartServerParams,  sizeof(kvm_vnc_server_start_params));

		KillX11Vnc();

		/*if(X11vncPid > 0)
		{
			int wPid = 0;
			kill(X11vncPid, SIGKILL);
			wPid = waitpid(X11vncPid,NULL, WNOHANG);
			//if(wPid == X11vncPid) X11vncPid = 0;
			X11vncPid = 0;
		}*/
		IsKVMGetConnectParamsThreadRunning = TRUE;
		if (app_os_thread_create(&KVMGetConnectParamsThreadHandle, KVMGetConnectParamsThreadStart, g_pkvmVNCServerStartParms) != 0)
		{
			printf("KVMGetConnectParamsThreadStart failed!\n");
			IsKVMGetConnectParamsThreadRunning = FALSE;
		}
		else
		{
			printf("KVMGetConnectParamsThreadStart ok!\n");
		}
	}
}
#endif

void GetVNCMode() 
{
	char * paramsRepJsonStr = NULL;
	int jsonStrlen = 0;
	/*susiaccess_kvm_conf_body_t kvm_conf;
	memset(&kvm_conf, 0, sizeof(susiaccess_kvm_conf_body_t));
	kvm_load(ConfigFilePath, &kvm_conf);*/
	jsonStrlen = Parser_PackVNCModeParamsRep(&kvm_conf, &paramsRepJsonStr);
	if(jsonStrlen > 0 && paramsRepJsonStr != NULL)
	{
		g_sendcbf(&g_PluginInfo, kvm_get_vnc_mode_rep, paramsRepJsonStr, strlen(paramsRepJsonStr)+1, NULL, NULL);
	}
	if(paramsRepJsonStr)free(paramsRepJsonStr);	
}

static void CollectCpbInfo(kvm_capability_info_t * pCpbInfo)
{
	if(NULL == pCpbInfo) return;
	{
		//add code about ssh
		//susiaccess_kvm_conf_body_t kvm_conf;
		memset(&kvm_conf, 0, sizeof(susiaccess_kvm_conf_body_t));
		kvm_load(ConfigFilePath, &kvm_conf);
		strcpy(pCpbInfo->vncMode, kvm_conf.kvmMode);
		strcpy(pCpbInfo->vncPwd, kvm_conf.custVNCPwd);
		pCpbInfo->vncPort = (unsigned int)atoi(kvm_conf.custVNCPort);
		if(!stricmp(KVM_DEFAULT_MODE_STR, pCpbInfo->vncMode))
		{
			sprintf(pCpbInfo->funcsStr, "%s,%s", KVM_DEFAULT_MODE_STR, KVM_REPEATER_FUNC_STR);
			pCpbInfo->funcsCode = KVM_DEFAULT_FUNC_FLAG|KVM_REPEATER_FUNC_FLAG;
		}
		else if(!stricmp(KVM_CUSTOM_MODE_STR, pCpbInfo->vncMode))
		{
			sprintf(pCpbInfo->funcsStr, "%s", KVM_CUSTOM_MODE_STR);
			pCpbInfo->funcsCode = KVM_CUSTOM_FUNC_FLAG;
		}
		else if(!stricmp(KVM_DISABLE_MODE_STR, pCpbInfo->vncMode))
		{
			sprintf(pCpbInfo->funcsStr, "%s", KVM_DISABLE_MODE_STR);
			pCpbInfo->funcsCode = KVM_NONE_FUNC_FLAG;
		}
	}
}

static void GetCapability()
{
	char * cpbStr = NULL;
	int jsonStrlen = 0;
	/*kvm_capability_info_t cpbInfo;
	memset((char*)&cpbInfo, 0, sizeof(kvm_capability_info_t));
	CollectCpbInfo(&cpbInfo);*/
	jsonStrlen = Parser_PackCpbInfo(&cpbInfo, &cpbStr);
	if(jsonStrlen > 0 && cpbStr != NULL)
	{
		g_sendcbf(&g_PluginInfo, kvm_get_capability_rep, cpbStr, strlen(cpbStr)+1, NULL, NULL);
		if(cpbStr)free(cpbStr);
	}
	else
	{
		char * errorRepJsonStr = NULL;
		char errorStr[128];
		int jsonStrlen = 0;
		sprintf(errorStr, "Command(%d), Get capability error!", kvm_get_capability_req);
		jsonStrlen = Parser_PackKVMErrorRep(errorStr, &errorRepJsonStr);
		if(jsonStrlen > 0 && errorRepJsonStr != NULL)
		{
			g_sendcbf(&g_PluginInfo, kvm_error_rep, errorRepJsonStr, strlen(errorRepJsonStr)+1, NULL, NULL);
		}
		if(errorRepJsonStr)free(errorRepJsonStr);
	}
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
	if(pluginfo == NULL)
	{
		return handler_fail;
	}

	if(g_bEnableLog)
	{
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

	// 3. Callback function -> Send JSON Data by this callback function
	g_sendcbf = g_PluginInfo.sendcbf = pluginfo->sendcbf;
	g_sendcustcbf = g_PluginInfo.sendcustcbf = pluginfo->sendcustcbf;
	g_subscribecustcbf = g_PluginInfo.subscribecustcbf = pluginfo->subscribecustcbf;
	g_sendreportcbf = g_PluginInfo.sendreportcbf = pluginfo->sendreportcbf;
	g_sendcapabilitycbf =g_PluginInfo.sendcapabilitycbf = pluginfo->sendcapabilitycbf;

	InitRemoteKVMPath();

	memset(RemoteKVMHandlerContext.vncPassword, 0, sizeof(RemoteKVMHandlerContext.vncPassword));
    strcpy(RemoteKVMHandlerContext.vncPassword, DEF_VNC_SERVER_PWD);

	{
		//susiaccess_kvm_conf_body_t kvm_conf;
		/*memset(&kvm_conf, 0, sizeof(susiaccess_kvm_conf_body_t));
		kvm_load(ConfigFilePath, &kvm_conf);*/
		memset((char*)&cpbInfo, 0, sizeof(kvm_capability_info_t));
		CollectCpbInfo(&cpbInfo);
		RemoteKVMHandlerContext.vncServerPort= atoi(kvm_conf.custVNCPort);
	}


	return handler_success;
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
	if(pluginfo->agentInfo->status == AGENT_STATUS_ONLINE)
	{
		GetVNCMode();
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
		GetVNCMode();
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
	memset(&RemoteKVMHandlerContext, 0, sizeof(remote_kvm_handler_context_t));
#ifndef WIN32
	KillX11Vnc();
#endif
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
	int commCmd = unknown_cmd;
	char errorStr[128] = {0};

	RemoteKVMLog(g_loghandle, Normal, " %s>Recv Topic [%s] Data %s", strPluginName, topic, (char*) data );
	printf(" >Recv Topic [%s] Data %s", topic, (char*) data );

	if(!ParseReceivedData(data, datalen, &commCmd))
		return;

	switch(commCmd)
	{
	case kvm_get_capability_req:
		{
			GetCapability();
			break;
		}
	case kvm_get_vnc_mode_req:
		{	
			GetVNCMode();
			break;
		}
	case kvm_get_connect_params_req:
		{
			kvm_vnc_server_start_params kvmVNCServerStartParms;
			memset(&kvmVNCServerStartParms, 0, sizeof(kvm_vnc_server_start_params));
			if(ParseKVMRecvCmd((char*)data, &kvmVNCServerStartParms))
			{
				KVMGetConnectParams(&kvmVNCServerStartParms);
			}
			else
			{
				memset(errorStr, 0, sizeof(errorStr));
				sprintf(errorStr, "Command(%d) parse error!",kvm_get_connect_params_req);
				{
					char * errorRepJsonStr = NULL;
					int jsonStrlen = Parser_PackKVMErrorRep(errorStr, &errorRepJsonStr);
					if(jsonStrlen > 0 && errorRepJsonStr != NULL)
					{
						g_sendcbf(&g_PluginInfo, kvm_error_rep, errorRepJsonStr, strlen(errorRepJsonStr)+1, NULL, NULL);
					}
					if(errorRepJsonStr)free(errorRepJsonStr);	
				}
			}
			break;
		}
	default: 
		{
			char * errorRepJsonStr = NULL;
			int jsonStrlen = Parser_PackKVMErrorRep("Unknown cmd!", &errorRepJsonStr);
			if(jsonStrlen > 0 && errorRepJsonStr != NULL)
			{
				g_sendcbf(&g_PluginInfo, kvm_error_rep, errorRepJsonStr, strlen(errorRepJsonStr)+1, NULL, NULL);
			}
			if(errorRepJsonStr)free(errorRepJsonStr);	
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
	char * cpbStr = NULL;
	int jsonStrlen = 0;
	/*kvm_capability_info_t cpbInfo;
	memset((char*)&cpbInfo, 0, sizeof(kvm_capability_info_t));
	CollectCpbInfo(&cpbInfo);*/
	jsonStrlen = Parser_PackCpbInfo(&cpbInfo, &cpbStr);
	if(jsonStrlen>0 && cpbStr != NULL)
	{
		char * repJsonStr = NULL;
		int jsonStrlen = Parser_PackSpecInfoRep(cpbStr, DEF_HANDLER_NAME, &repJsonStr);
		if(jsonStrlen > 0 && repJsonStr != NULL)
		{
			len = strlen(repJsonStr);
			*pOutReply = (char *)malloc(len + 1);
			memset(*pOutReply, 0, len + 1);
			strcpy(*pOutReply, repJsonStr);
		}
		if(repJsonStr)free(repJsonStr);
	}
	if(cpbStr) free(cpbStr);
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
	return;
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
	return;
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