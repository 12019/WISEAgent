#include "Parser.h"
#include "common.h"
#include "DES.h"
#include "Base64.h"

#define DEF_DES_KEY					"29B4B9C5"
#define DEF_DES_IV					"42b19631"

bool DES_BASE64Decode(char * srcBuf,char **destBuf)
{
	bool bRet = false;
	char plaintext[512] = {0};
	int iRet = 0;
	if(srcBuf == NULL || destBuf == NULL) return bRet;
	{
		char *base64Dec = NULL;
		int decLen = 0;
		int len = strlen(srcBuf);
		iRet = Base64Decode(srcBuf, len, &base64Dec, &decLen);
		if(iRet == 0)
		{
			iRet = DESDecodeEx(DEF_DES_KEY, DEF_DES_IV,  base64Dec, decLen, plaintext);
			if(iRet == 0)
			{
				*destBuf = (char *)malloc(len + 1);
				memset(*destBuf, 0, len + 1);
				strcpy(*destBuf, plaintext);
				bRet = true;
			}
		}
		if(base64Dec) free(base64Dec);
	}

	return bRet;
}

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

struct SERVER_INFO* ParseServer(cJSON * pServerIP)
{
	cJSON * valItem = NULL;
	struct SERVER_INFO* pServer = NULL;
	if(!pServerIP)
		return;

	pServer = malloc(sizeof(struct SERVER_INFO));
	if(!pServer)
		return NULL;

	memset(pServer, 0, sizeof(struct SERVER_INFO));

	valItem = cJSON_GetObjectItem(pServerIP, SERVERREDUNDANCY_SERVER_ADDRESS);
	if(valItem)
	{
		if(valItem->valuestring)
			strncpy(pServer->sServerIP, valItem->valuestring, strlen(valItem->valuestring)+1);
	}
	else
	{
		free(pServer);
		pServer = NULL;
		return pServer;
	}

	valItem = cJSON_GetObjectItem(pServerIP, SERVERREDUNDANCY_SERVER_PORT);
	if(valItem)
	{
		pServer->iServerPort = valItem->valueint;
	}
	else
	{
		free(pServer);
		pServer = NULL;
		return pServer;
	}

	valItem = cJSON_GetObjectItem(pServerIP, SERVERREDUNDANCY_SERVER_AUTH);
	if(valItem)
	{
		if(valItem->valuestring)
			strncpy(pServer->sServerAuth, valItem->valuestring, strlen(valItem->valuestring)+1);
	}
	else
	{
		free(pServer);
		pServer = NULL;
		return pServer;
	}

	valItem = cJSON_GetObjectItem(pServerIP, SERVERREDUNDANCY_TLSTYPE_FLAG);
	if(valItem)
	{
		pServer->eTLSType = valItem->valueint;
	}

	valItem = cJSON_GetObjectItem(pServerIP, SERVERREDUNDANCY_PSK_FLAG);
	if(valItem)
	{
		if(valItem->valuestring)
		{
			char* desSrc = NULL;
			if(DES_BASE64Decode(valItem->valuestring, &desSrc))
				strncpy(pServer->sPSK, desSrc, strlen(desSrc)+1);
			else
				strncpy(pServer->sPSK, valItem->valuestring, strlen(valItem->valuestring)+1);
			if(desSrc)
				free(desSrc);
		}
	}
	return pServer;
}

void FreeServerList(struct SERVER_INFO* pServerList)
{
	struct SERVER_INFO* pCurServer = NULL;
	struct SERVER_INFO* pNextServer = NULL;
	if(!pServerList)
		return;

	pCurServer = pServerList;
	while(pCurServer)
	{
		pNextServer = pCurServer->next;
		free(pCurServer);
		pCurServer = pNextServer;
	}
	pServerList = NULL;
}

void ParseServerList(cJSON * pServerIPList, struct SERVER_LIST* pServerList)
{
	cJSON * subItem = NULL;
	cJSON * valItem = NULL;
	int i = 0;
	int nCount = cJSON_GetArraySize(pServerIPList);

	struct SERVER_INFO* pLastServer = NULL;

	if(!pServerList)
		return;

	if(pServerList->server)
	{
		FreeServerList(pServerList->server);
		pServerList->serverCount = 0;
	}

	memset(pServerList, 0, sizeof(struct SERVER_LIST));
	
	if(!pServerIPList)
		return;

	for(i = 0; i<nCount; i++)
	{
		subItem = cJSON_GetArrayItem(pServerIPList, i);
		if(subItem)
		{
			struct SERVER_INFO* pServer = ParseServer(subItem);
			if(!pServer)
				continue;

			if(!pServerList->server)
			{
				pServerList->server = pServer;
				pServer->bMaster = true; //first one is master
			}
			pServerList->serverCount++;
			if(pLastServer)
				pLastServer->next = pServer;
			pLastServer = pServer;
		}
	}
	return pServerList;
}

int ParseServerCtrl(void* data, int datalen, RESPONSE_MESSAGE *pMessage, char *workDir)
{
	/*{"susiCommData":{"commCmd":125,"handlerName":"general","catalogID":4,"response":{"statuscode":0,"msg":"Server losyconnection"}}}*/
	cJSON *root = NULL, *body = NULL, *pSubItem = NULL, *pTarget = NULL, *pServer = NULL,*pServerIPList = NULL; 
	bool bRet = false;
	if(!data) return false;
	if(datalen<=0) return false;
	if(!pMessage) return false;

	root = cJSON_Parse(data);
	if(!root) return false;

	body = cJSON_GetObjectItem(root, AGENTINFO_BODY_STRUCT);
	if(!body)
	{
		cJSON_Delete(root);
		return false;
	}

	pSubItem = cJSON_GetObjectItem(body, SERVERREDUNDANCY_RESPONSE);
	if(pSubItem)
	{
		pTarget = cJSON_GetObjectItem(pSubItem, SERVERREDUNDANCY_STATUSCODE);
		if(pTarget)
		{
			pMessage->iStatusCode = pTarget->valueint;
		}

		pTarget = cJSON_GetObjectItem(pSubItem, SERVERREDUNDANCY_RESPONSEMESSAGE);
		if(pTarget)
		{
			if(pTarget->valuestring)
			{
				if(strlen(pTarget->valuestring)<=0)
					pMessage->strMsg = NULL;
				else
				{
					pMessage->strMsg = malloc(strlen(pTarget->valuestring)+1);
					memset(pMessage->strMsg, 0, strlen(pTarget->valuestring)+1);
					strcpy(pMessage->strMsg, pTarget->valuestring);
				}
			}
		}

		pServer = cJSON_GetObjectItem(pSubItem, SERVERREDUNDANCY_SERVER_NODE);
		if(pServer)
		{
			pMessage->pServer = ParseServer(pServer);
		}
		pServerIPList = cJSON_GetObjectItem(pSubItem, SERVERREDUNDANCY_SERVER_IP_LIST);
		if(pServerIPList)
		{
			FILE *fp = NULL;
			char filepath[MAX_PATH] = {0};
			char* strServer = NULL;
			path_combine(filepath, workDir, DEF_SERVER_IP_LIST_FILE);//g_PluginInfo.WorkDir, 
			if(fp=fopen(filepath,"wt+"))
			{
				strServer = cJSON_PrintUnformatted(pServerIPList);
				if(strServer)
				{
					fputs(strServer,fp);
					free(strServer);
				}
				fclose(fp);
			}
			ParseServerList(pServerIPList, pMessage->pServerList);
		}
		bRet = true;
	}
	cJSON_Delete(root);
	return bRet;
}