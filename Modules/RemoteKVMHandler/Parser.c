#include "Parser.h"
#include <stdio.h>

//---------------------------------------------------RemoteKVM---------------------------------------------------------------------
static cJSON * cJSON_CreateVNCConnectParams(kvm_vnc_connect_params * pVNCConnectParams);
#define cJSON_AddVNCConnectParamsToObject(object, name, pV)  cJSON_AddItemToObject(object, name, cJSON_CreateVNCConnectParams(pV))
#define cJSON_AddKVMModeParamsToObject(object, name, pV)  cJSON_AddItemToObject(object, name, cJSON_CreateVNCModeParams(pV))

bool ParseReceivedData(void* data, int datalen, int * cmdID)
{
	/*{"susiCommData":{"commCmd":251,"catalogID":4,"requestID":10}}*/

	cJSON* root = NULL;
	cJSON* body = NULL;
	cJSON* target = NULL;

	if(!data) return false;
	if(datalen<=0) return false;

	root = cJSON_Parse(data);
	if(!root) return false;

	body = cJSON_GetObjectItem(root, AGENTINFO_BODY_STRUCT);
	if(!body)
	{
		cJSON_Delete(root);
		return false;
	}

	target = cJSON_GetObjectItem(body, AGENTINFO_CMDTYPE);
	if(target)
	{
		*cmdID = target->valueint;
	}
	cJSON_Delete(root);
	return true;
}

bool ParseKVMRecvCmd(void* data, kvm_vnc_server_start_params * kvmParms)
{
	cJSON *pParamsItem = NULL;
	kvm_vnc_server_start_params * pVNCStartParams = NULL;
	cJSON* root = NULL;
	cJSON* body = NULL;

	if(!data) return false;

	root = cJSON_Parse(data);
	if(!root) return false;

	body = cJSON_GetObjectItem(root, AGENTINFO_BODY_STRUCT);
	if(!body)
	{
		cJSON_Delete(root);
		return false;
	}

	pParamsItem = cJSON_GetObjectItem(body, KVM_VNC_SERVER_NEEDCHANGEPASS);  
	pVNCStartParams = kvmParms;

	if(pParamsItem != NULL)
	{
		pVNCStartParams->need_change_password = pParamsItem->valueint;
	}
	else
	{
		pVNCStartParams->need_change_password = 1;
	}

	pParamsItem = cJSON_GetObjectItem(body, KVM_VNC_SERVER_START_MODE);                  
	if(pParamsItem != NULL)
	{
		pVNCStartParams->mode = pParamsItem->valueint;
	}
	else
	{
		pVNCStartParams->mode = 1;
	}

	if(pVNCStartParams->mode == 2)
	{
		pParamsItem = cJSON_GetObjectItem(body, KVM_VNC_SERVER_START_LISTEN_PORT);                  
		if(pParamsItem != NULL)
		{
			pVNCStartParams->vnc_server_listen_port = pParamsItem->valueint;
		}
		else
		{
			pVNCStartParams->vnc_server_listen_port = 5500;
		}

		pParamsItem = cJSON_GetObjectItem(body, KVM_VNC_SERVER_START_LISTEN_HOST);                  
		if(pParamsItem != NULL)
		{
			sprintf(pVNCStartParams->vnc_server_start_listen_ip, "%s", pParamsItem->valuestring);
		}
		else
		{
			sprintf(pVNCStartParams->vnc_server_start_listen_ip, "%s", "127.0.0.1");
		}
	}
	else if(pVNCStartParams->mode == 3)
	{
		pParamsItem = cJSON_GetObjectItem(body, KVM_VNC_SERVER_START_REPEATER_PORT);                  
		if(pParamsItem != NULL)
		{
			pVNCStartParams->vnc_server_repeater_port = pParamsItem->valueint;
		}
		else
		{
			pVNCStartParams->vnc_server_repeater_port = 5501;
		}

		pParamsItem = cJSON_GetObjectItem(body, KVM_VNC_SERVER_START_REPEATER_ID);                  
		if(pParamsItem != NULL)
		{
			pVNCStartParams->vnc_server_repeater_id = pParamsItem->valueint;
		}
		else
		{
			pVNCStartParams->vnc_server_repeater_id = 0;
		}
	}

	cJSON_Delete(root);
	return true;
}

static cJSON * cJSON_CreateVNCConnectParams(kvm_vnc_connect_params * pVNCConnectParams)
{
	/*
	{"vncConnectParams":{"vncServerIP":"xxx","vncServerPort":5900,"vncServerPassword":"na"}}
	*/
	cJSON * pVNCConnectParamsItem = NULL;
	if(!pVNCConnectParams) return NULL;
	pVNCConnectParamsItem = cJSON_CreateObject();
	cJSON_AddStringToObject(pVNCConnectParamsItem, KVM_VNC_SERVER_IP, pVNCConnectParams->vnc_server_ip);
	cJSON_AddNumberToObject(pVNCConnectParamsItem, KVM_VNC_SERVER_PORT, pVNCConnectParams->vnc_server_port);
	cJSON_AddStringToObject(pVNCConnectParamsItem, KVM_VNC_SERVER_PWD, pVNCConnectParams->vnc_password);
	return pVNCConnectParamsItem;
}

int Parser_PackKVMGetConnectParamsRep(kvm_vnc_connect_params *pConnectParams, char** outputStr)
{
   char * out = NULL;
   int outLen = 0;
   cJSON *pSUSICommDataItem = NULL;
   if(pConnectParams == NULL || outputStr == NULL) return outLen;
   pSUSICommDataItem = cJSON_CreateObject();

   cJSON_AddVNCConnectParamsToObject(pSUSICommDataItem, KVM_VNC_CONNECT_PARAMS, pConnectParams);

   out = cJSON_PrintUnformatted(pSUSICommDataItem);
   outLen = strlen(out) + 1;
   *outputStr = (char *)(malloc(outLen));
   memset(*outputStr, 0, outLen);
   strcpy(*outputStr, out);
   cJSON_Delete(pSUSICommDataItem);	
   printf("%s\n",out);	
   free(out);
   return outLen;
}

int Parser_PackKVMErrorRep(char * errorStr, char ** outputStr)
{
	char * out = NULL;
	int outLen = 0;
	cJSON *pSUSICommDataItem = NULL;
	if(errorStr == NULL || outputStr == NULL) return outLen;
	pSUSICommDataItem = cJSON_CreateObject();

	cJSON_AddStringToObject(pSUSICommDataItem, KVM_ERROR_REP, errorStr);

	out = cJSON_PrintUnformatted(pSUSICommDataItem);
	outLen = strlen(out) + 1;
	*outputStr = (char *)(malloc(outLen));
	memset(*outputStr, 0, outLen);
	strcpy(*outputStr, out);
	cJSON_Delete(pSUSICommDataItem);	
	printf("%s\n",out);	
	free(out);
	return outLen;
}

static cJSON * cJSON_CreateVNCModeParams(susiaccess_kvm_conf_body_t * pVNCModeParams)
{
	/*{"vncModeInfo":{"vncMode":"default","custvncPort":5900,"custvncPwd":"na"}*/
	cJSON * pVNCModeParamsItem = NULL;
	int port = 5900;
	if(!pVNCModeParams) return NULL;
	port = atoi(pVNCModeParams->custVNCPort);
	pVNCModeParamsItem = cJSON_CreateObject();
	cJSON_AddStringToObject(pVNCModeParamsItem, KVM_VNC_MODE, pVNCModeParams->kvmMode);
	cJSON_AddNumberToObject(pVNCModeParamsItem, KVM_VNC_PORT, port);
	cJSON_AddStringToObject(pVNCModeParamsItem, KVM_VNC_PWD, pVNCModeParams->custVNCPwd);
	return pVNCModeParamsItem;
}

int Parser_PackVNCModeParamsRep(susiaccess_kvm_conf_body_t *pConnectParams, char** outputStr)
{
   char * out = NULL;
   int outLen = 0;
   cJSON *pSUSICommDataItem = NULL;
   if(pConnectParams == NULL || outputStr == NULL) return outLen;
   pSUSICommDataItem = cJSON_CreateObject();

   cJSON_AddKVMModeParamsToObject(pSUSICommDataItem, KVM_VNC_MODE_INFO, pConnectParams);

   out = cJSON_PrintUnformatted(pSUSICommDataItem);
   outLen = strlen(out) + 1;
   *outputStr = (char *)(malloc(outLen));
   memset(*outputStr, 0, outLen);
   strcpy(*outputStr, out);
   cJSON_Delete(pSUSICommDataItem);	
   printf("%s\n",out);	
   free(out);
   return outLen;
}

int Parser_PackCpbInfo(kvm_capability_info_t * cpbInfo, char **outputStr)
{
	char * out = NULL;
	int outLen = 0;
	cJSON *pSUSICommDataItem = NULL, *subItem = NULL, *eItem = NULL;
	if(cpbInfo == NULL || outputStr == NULL) return outLen;

	pSUSICommDataItem = cJSON_CreateObject();
	if(pSUSICommDataItem)
	{
		subItem = cJSON_CreateObject();
		if(subItem)
		{
			cJSON_AddItemToObject(pSUSICommDataItem, KVM_INFOMATION, subItem);
			cJSON_AddStringToObject(subItem, KVM_BN_FLAG, KVM_INFOMATION);
			cJSON_AddBoolToObject(subItem, KVM_NS_DATA, 1);
			eItem = cJSON_CreateArray();
			if(eItem)
			{
				cJSON_AddItemToObject(subItem, KVM_E_FLAG, eItem);
				if(strlen(cpbInfo->vncMode))
				{
					subItem = cJSON_CreateObject();
					cJSON_AddStringToObject(subItem, KVM_N_FLAG, KVM_VNC_MODE);
					cJSON_AddStringToObject(subItem, KVM_SV_FLAG, cpbInfo->vncMode);
					cJSON_AddItemToArray(eItem, subItem);
				}
				if(cpbInfo->vncPort > 0)
				{
					subItem = cJSON_CreateObject();
					cJSON_AddStringToObject(subItem, KVM_N_FLAG, KVM_VNC_PORT);
					cJSON_AddNumberToObject(subItem, KVM_V_FLAG, cpbInfo->vncPort);
					cJSON_AddItemToArray(eItem, subItem);
				}
				if(strlen(cpbInfo->vncPwd))
				{
					subItem = cJSON_CreateObject();
					cJSON_AddStringToObject(subItem, KVM_N_FLAG, KVM_VNC_PWD);
					cJSON_AddStringToObject(subItem, KVM_SV_FLAG, cpbInfo->vncPwd);
					cJSON_AddItemToArray(eItem, subItem);
				}
				if(strlen(cpbInfo->funcsStr))
				{
					subItem = cJSON_CreateObject();
					cJSON_AddStringToObject(subItem, KVM_N_FLAG, KVM_FUNCTION_LIST);
					cJSON_AddStringToObject(subItem, KVM_SV_FLAG, cpbInfo->funcsStr);
					cJSON_AddItemToArray(eItem, subItem);
				}
				{
					subItem = cJSON_CreateObject();
					cJSON_AddStringToObject(subItem, KVM_N_FLAG, KVM_FUNCTION_CODE);
					cJSON_AddNumberToObject(subItem, KVM_V_FLAG, cpbInfo->funcsCode);
					cJSON_AddItemToArray(eItem, subItem);
				}
			}
		}
	}

	out = cJSON_PrintUnformatted(pSUSICommDataItem);
	outLen = strlen(out) + 1;
	*outputStr = (char *)(malloc(outLen));
	memset(*outputStr, 0, outLen);
	strcpy(*outputStr, out);
	cJSON_Delete(pSUSICommDataItem);	
	printf("%s\n",out);	
	free(out);
	return outLen;
}

int Parser_PackSpecInfoRep(char * cpbStr, char * handlerName, char ** outputStr)
{
	char * out = NULL;
	int outLen = 0;
	cJSON *pSUSICommDataItem = NULL, * specInfoItem = NULL;
	if(cpbStr == NULL || handlerName == NULL || outputStr == NULL) return outLen;
	pSUSICommDataItem = cJSON_CreateObject();

	cJSON_AddItemToObject(pSUSICommDataItem, handlerName, cJSON_Parse(cpbStr));

	out = cJSON_PrintUnformatted(pSUSICommDataItem);
	outLen = strlen(out) + 1;
	*outputStr = (char *)(malloc(outLen));
	memset(*outputStr, 0, outLen);
	strcpy(*outputStr, out);
	cJSON_Delete(pSUSICommDataItem);	
	printf("%s\n",out);	
	free(out);
	return outLen;
}

