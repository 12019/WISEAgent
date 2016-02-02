#include "parser_prtt.h"

static cJSON * cJSON_CreatePrttStatusInfo(prtt_status_t * pPrttStatus);

#define cJSON_AddPrttStatusInfoToObject(object, name, pP) cJSON_AddItemToObject(object, name, cJSON_CreatePrttStatusInfo(pP))

bool ParseReceivedData(void* data, int datalen, int * cmdID)
{
	/*{"susiCommData":{"commCmd":251,"catalogID":4,"requestID":10}}*/

	cJSON* root = NULL;
	cJSON* body = NULL;
	cJSON* target = NULL;

	if(!data) return false;
	if(datalen<=0) return false;
	//MonitorLog(g_loghandle, Normal, " %s>Parser_ParseReceivedData [%s]\n", MyTopic, data );
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

bool Parser_ParseInstallerDlParams(void * data, int dataLen, prtt_installer_dl_params_t * dlParams)
{
	bool bRet = false;
	int parseOK = 0;
	if(data == NULL || dataLen<=0 ||  dlParams == NULL) return bRet;
	{
		prtt_installer_dl_params_t * pInstallerDLParams = dlParams;
		cJSON* root = NULL;
		cJSON* body = NULL;
		cJSON* pParamsItem = NULL;
		//cJSON* target = NULL;
		root = cJSON_Parse(data);
		if(!root) return false;
		body = cJSON_GetObjectItem(root, AGENTINFO_BODY_STRUCT);
		if(!body)
		{
			cJSON_Delete(root);
			return false;
		}
		pParamsItem = cJSON_GetObjectItem(body, PRTT_IS_INSTALL_THEN_ACTIVE);
		if(pParamsItem)
		{
			if(!_stricmp(pParamsItem->valuestring, "True"))
			{
				pInstallerDLParams->isInstallThenActive = 1;
				parseOK = 1;
			}
			else if(!_stricmp(pParamsItem->valuestring, "False"))
			{
				pInstallerDLParams->isInstallThenActive = 0;
				parseOK = 1;
			}
		}
		if(parseOK)
		{
			parseOK = 0;
			pParamsItem = cJSON_GetObjectItem(body, PRTT_FTP_DL_USERNAME);
			if(pParamsItem)
			{
				strcpy(pInstallerDLParams->ftpuserName, pParamsItem->valuestring);
				pParamsItem = cJSON_GetObjectItem(body, PRTT_FTP_DL_PASSWORD);
				if(pParamsItem)
				{
					strcpy(pInstallerDLParams->ftpPassword, pParamsItem->valuestring);
					pParamsItem = cJSON_GetObjectItem(body, PRTT_FTP_DL_PORT);
					if(pParamsItem)
					{
						pInstallerDLParams->port = pParamsItem->valueint;
						pParamsItem = cJSON_GetObjectItem(body, PRTT_FTP_DL_PATH);
						if(pParamsItem)
						{
							strcpy(pInstallerDLParams->installerPath, pParamsItem->valuestring);
							pParamsItem = cJSON_GetObjectItem(body, PRTT_FTP_DL_MD5);
							if(pParamsItem)
							{
								strcpy(pInstallerDLParams->md5, pParamsItem->valuestring);
								parseOK = 1;
							}
						}
					}
				}
			}
		}
	}
	if(parseOK) bRet = true;
	return bRet;
}

int PrttReplyMsgPack(cJSON *pInReply, char ** pOutReply)
{
	int len = 0;
	if (pInReply && pOutReply) {		
		char *out = cJSON_PrintUnformatted(pInReply);
		len = strlen(out) + 1;

		*pOutReply = (char *)malloc(len);
		memset(*pOutReply, 0, len);
		strcpy(*pOutReply, out);
		cJSON_Delete(pInReply);	
		printf("%s\n",out);	
		free(out);
	}
	return len;
}

int PackPrttStrRep(char *repMsg, prtt_reply_status_code statusCode, char** outJsonStr)
{
	int iRet = 0;
	char * out = NULL;
	int outLen = 0;
	cJSON *pSUSICommDataItem = NULL;
	if(repMsg == NULL)return iRet;
	pSUSICommDataItem = cJSON_CreateObject();

	cJSON_AddStringToObject(pSUSICommDataItem, PRTT_REP_MSG, repMsg);
	cJSON_AddItemToObject(pSUSICommDataItem, OPRATION_STATUS_CODE, cJSON_CreateNumber(statusCode));

	out = cJSON_PrintUnformatted(pSUSICommDataItem);
	outLen = strlen(out) + 1;
	*outJsonStr = (char *)(malloc(outLen));
	memset(*outJsonStr, 0, outLen);
	strncpy(*outJsonStr, out, outLen);
	cJSON_Delete(pSUSICommDataItem);	
	printf("%s\n",out);	
	free(out);
	return iRet = outLen;
}

void SendReplyMsg_Prtt(char *repMsg, prtt_reply_status_code statusCode, susi_comm_cmd_t prtt_rep_id)
{
	char * pSendVal = NULL;
	char * str = repMsg;
	int jsonStrlen = PackPrttStrRep(str, statusCode, &pSendVal);
	if(jsonStrlen > 0 && pSendVal != NULL)
	{
		g_sendcbf(&g_PluginInfo, prtt_rep_id, pSendVal, strlen(pSendVal)+1, NULL, NULL);
	}
	if (pSendVal)	
		free(pSendVal);	
}

void SendReplySuccessMsg_Prtt(char *repMsg, susi_comm_cmd_t prtt_rep_id)
{
	SendReplyMsg_Prtt(repMsg, oprt_success, prtt_rep_id);
}

void SendReplyFailMsg_Prtt(char *repMsg, susi_comm_cmd_t prtt_rep_id)
{
	SendReplyMsg_Prtt(repMsg, oprt_fail, prtt_rep_id);
}

static cJSON * cJSON_CreatePrttStatusInfo(prtt_status_t * pPrttStatus)
{
	cJSON * pPrttStatusInfoItem = NULL;
	if(!pPrttStatus) return NULL;
	pPrttStatusInfoItem = cJSON_CreateObject();
	if(pPrttStatus->isProtection) cJSON_AddStringToObject(pPrttStatusInfoItem, PRTT_STATUS_IS_PROTECTION, "True");
	else cJSON_AddStringToObject(pPrttStatusInfoItem, PRTT_STATUS_IS_PROTECTION, "False");

	if(pPrttStatus->isActivated) cJSON_AddStringToObject(pPrttStatusInfoItem, PRTT_STATUS_IS_ACTIVATED, "True");
	else cJSON_AddStringToObject(pPrttStatusInfoItem, PRTT_STATUS_IS_ACTIVATED, "False");

	if(pPrttStatus->isExpired) cJSON_AddStringToObject(pPrttStatusInfoItem, PRTT_STATUS_IS_EXPIRED, "True");
	else cJSON_AddStringToObject(pPrttStatusInfoItem, PRTT_STATUS_IS_EXPIRED, "False");

	if(pPrttStatus->isInstalled) cJSON_AddStringToObject(pPrttStatusInfoItem, PRTT_STATUS_IS_INSTALLED, "True");
	else cJSON_AddStringToObject(pPrttStatusInfoItem, PRTT_STATUS_IS_INSTALLED, "False");

	if(pPrttStatus->isExistNewerVer) cJSON_AddStringToObject(pPrttStatusInfoItem, PRTT_STATUS_IS_NEWERVER, "True");
	else cJSON_AddStringToObject(pPrttStatusInfoItem, PRTT_STATUS_IS_NEWERVER, "False");

	if(strlen(pPrttStatus->version)) cJSON_AddStringToObject(pPrttStatusInfoItem, PRTT_STATUS_VERSION, pPrttStatus->version);
	else cJSON_AddStringToObject(pPrttStatusInfoItem, PRTT_STATUS_VERSION, "1.0.0");

	cJSON_AddStringToObject(pPrttStatusInfoItem, PRTT_STATUS_ACTION_MSG, pPrttStatus->actionMsg);
	cJSON_AddStringToObject(pPrttStatusInfoItem, PRTT_STATUS_LWARNING_MSG, pPrttStatus->lastWarningMsg);

	return pPrttStatusInfoItem;
}

int Parser_PackPrttStatusRep(prtt_status_t * pPrttStatus, char** outJsonStr)
{
	int iRet = 0;
	char * out = NULL;
	int outLen = 0;
	cJSON *pSUSICommDataItem = NULL;
	if(pPrttStatus == NULL)return iRet;
	pSUSICommDataItem = cJSON_CreateObject();
	cJSON_AddPrttStatusInfoToObject(pSUSICommDataItem, PRTT_STATUS_INFO, pPrttStatus);	
	//status code: success always
	cJSON_AddItemToObject(pSUSICommDataItem, OPRATION_STATUS_CODE, cJSON_CreateNumber(oprt_success));

	out = cJSON_PrintUnformatted(pSUSICommDataItem);
	outLen = strlen(out) + 1;
	*outJsonStr = (char *)(malloc(outLen));
	memset(*outJsonStr, 0, outLen);
	//strcpy_s(*outJsonStr, outLen, out);
	strncpy(*outJsonStr, out, outLen);
	cJSON_Delete(pSUSICommDataItem);	
	printf("%s\n",out);	
	free(out);
	return iRet = outLen;
}
