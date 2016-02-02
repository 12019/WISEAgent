#include "Parser.h"

bool ParseReceivedData(void* data, int datalen, int * cmdID)
{
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

bool Parser_ParseSessionCmd(char * jsonStr, int *sesID, char ** cmdStr)
{
	bool bRet = false;
	cJSON* root = NULL;
	cJSON* body = NULL;
	cJSON* target = NULL;

	if(jsonStr == NULL || NULL == sesID || cmdStr == NULL) return false;

	root = cJSON_Parse(jsonStr);
	if(root)
	{
		body = cJSON_GetObjectItem(root, AGENTINFO_BODY_STRUCT);
		if(body)
		{
			target = cJSON_GetObjectItem(body, TERMINAL_CMDSTR);
			if(target)
			{
				int len = strlen(target->valuestring) + 1;
				*cmdStr = (char*)malloc(len);
				memset(*cmdStr, 0, len);
				strcpy(*cmdStr, target->valuestring);
			}
			target = cJSON_GetObjectItem(body, TERMINAL_CMDID);
			if(target)
			{
				*sesID = target->valueint;
				bRet = true;
			}
		}
		cJSON_Delete(root);
	}
	return bRet;
}

bool Parser_ParseSessionCmdEx(char * jsonStr, int *sesID, int *width, int *height, char ** cmdStr)
{
	bool bRet = false;
	cJSON* root = NULL;
	cJSON* body = NULL;
	cJSON* target = NULL;

	if(jsonStr == NULL || NULL == sesID || cmdStr == NULL) return false;

	root = cJSON_Parse(jsonStr);
	if(root)
	{
		body = cJSON_GetObjectItem(root, AGENTINFO_BODY_STRUCT);
		if(body)
		{
			target = cJSON_GetObjectItem(body, TERMINAL_CMDSTR);
			if(target)
			{
				int len = strlen(target->valuestring) + 1;
				*cmdStr = (char*)malloc(len);
				memset(*cmdStr, 0, len);
				strcpy(*cmdStr, target->valuestring);
			}
			target = cJSON_GetObjectItem(body, TERMINAL_CMDID);
			if(target)
			{
				*sesID = target->valueint;
				bRet = true;
			}
			if(width)
			{
				target = cJSON_GetObjectItem(body, TERMINAL_WIDTH);
				if(target)
				{
					*width = target->valueint;
					bRet = true;
				}
			}
			if(height)
			{
				target = cJSON_GetObjectItem(body, TERMINAL_HEIGHT);
				if(target)
				{
					*height = target->valueint;
					bRet = true;
				}
			}
		}
		cJSON_Delete(root);
	}
	return bRet;
}

bool Parser_ParseSessionStopParams(char * jsonStr, int *sesID)
{
	bool bRet = false;
	cJSON* root = NULL;
	cJSON* body = NULL;
	cJSON* target = NULL;

	if(jsonStr == NULL || NULL == sesID) return false;

	root = cJSON_Parse(jsonStr);
	if(root)
	{
		body = cJSON_GetObjectItem(root, AGENTINFO_BODY_STRUCT);
		if(body)
		{
			target = cJSON_GetObjectItem(body, TERMINAL_CMDID);
			if(target)
			{
				*sesID = target->valueint;
				bRet = true;
			}
		}
		cJSON_Delete(root);
	}
	return bRet;
}

int Parser_PackSessionStopRep(int sesID, char * repMsg, char **outputStr)
{
	char * out = NULL;
	int outLen = 0;
	cJSON *pSUSICommDataItem = NULL, *subItem = NULL;
	if(repMsg == NULL || outputStr == NULL) return outLen;

	pSUSICommDataItem = cJSON_CreateObject();
	if(pSUSICommDataItem)
	{
		subItem = cJSON_CreateObject();
		if(subItem)
		{
			cJSON_AddItemToObject(pSUSICommDataItem, TERMINAL_SET_REQP_REP, subItem);
			cJSON_AddNumberToObject(subItem, TERMINAL_PID, sesID);   
			cJSON_AddStringToObject(subItem, TERMINAL_STOP_CONTENT, repMsg);
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

int Parser_PackSessionStartRep(int sesID, char * repMsg, char **outputStr)
{
	char * out = NULL;
	int outLen = 0;
	cJSON *pSUSICommDataItem = NULL;
	if(repMsg == NULL || outputStr == NULL) return outLen;

	pSUSICommDataItem = cJSON_CreateObject();
	if(pSUSICommDataItem)
	{
		cJSON_AddStringToObject(pSUSICommDataItem, TERMINAL_CMDSTR, repMsg);
		cJSON_AddNumberToObject(pSUSICommDataItem, TERMINAL_PID, sesID);
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

int Parser_PackTerminalError(char * errorStr, char **outputStr)
{
	char * out = NULL;
	int outLen = 0;
	cJSON *pSUSICommDataItem = NULL;
	if(errorStr == NULL || outputStr == NULL) return outLen;

	pSUSICommDataItem = cJSON_CreateObject();
	if(pSUSICommDataItem)
	{
		cJSON_AddStringToObject(pSUSICommDataItem, TERMINAL_ERROR_REP, errorStr);
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

int Parser_PackSesRet(int sesID, char * retStr, char **outputStr)
{
	char * out = NULL;
	int outLen = 0;
	cJSON *pSUSICommDataItem = NULL, *subItem = NULL;
	if(retStr == NULL || outputStr == NULL) return outLen;

	pSUSICommDataItem = cJSON_CreateObject();
	if(pSUSICommDataItem)
	{
		subItem = cJSON_CreateObject();
		if(subItem)
		{
			cJSON_AddItemToObject(pSUSICommDataItem, TERMINAL_TI_INFO, subItem);
			cJSON_AddNumberToObject(subItem, TERMINAL_PID, sesID);
			cJSON_AddStringToObject(subItem, TERMINAL_CONTENT, retStr);
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

int Parser_PackCpbInfo(tmn_capability_info_t * cpbInfo, char **outputStr)
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
			cJSON_AddItemToObject(pSUSICommDataItem, TERMINAL_INFOMATION, subItem);
			cJSON_AddStringToObject(subItem, TERMINAL_BN_FLAG, TERMINAL_INFOMATION);
			cJSON_AddBoolToObject(subItem, TERMINAL_NS_DATA, 1);
			eItem = cJSON_CreateArray();
			if(eItem)
			{
				cJSON_AddItemToObject(subItem, TERMINAL_E_FLAG, eItem);
				if(strlen(cpbInfo->sshId))
				{
					subItem = cJSON_CreateObject();
					cJSON_AddStringToObject(subItem, TERMINAL_N_FLAG, TERMINAL_SSH_ID);
					cJSON_AddStringToObject(subItem, TERMINAL_SV_FLAG, cpbInfo->sshId);
					cJSON_AddItemToArray(eItem, subItem);
				}
				if(strlen(cpbInfo->sshPwd))
				{
					subItem = cJSON_CreateObject();
					cJSON_AddStringToObject(subItem, TERMINAL_N_FLAG, TERMINAL_SSH_PWD);
					cJSON_AddStringToObject(subItem, TERMINAL_SV_FLAG, cpbInfo->sshPwd);
					cJSON_AddItemToArray(eItem, subItem);
				}
				if(strlen(cpbInfo->funcsStr))
				{
					subItem = cJSON_CreateObject();
					cJSON_AddStringToObject(subItem, TERMINAL_N_FLAG, TERMINAL_FUNCTION_LIST);
					cJSON_AddStringToObject(subItem, TERMINAL_SV_FLAG, cpbInfo->funcsStr);
					cJSON_AddItemToArray(eItem, subItem);
				}
				{
					subItem = cJSON_CreateObject();
					cJSON_AddStringToObject(subItem, TERMINAL_N_FLAG, TERMINAL_FUNCTION_CODE);
					cJSON_AddNumberToObject(subItem, TERMINAL_V_FLAG, cpbInfo->funcsCode);
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
