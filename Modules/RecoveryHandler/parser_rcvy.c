#include "parser_rcvy.h"

#define cJSON_AddRcvyStatusInfoToObject(object, name, pR) cJSON_AddItemToObject(object, name, cJSON_CreateRcvyStatusInfo(pR))

//message send functions
void SendReplyMsg_Rcvy(char *rcvyMsg, rcvy_reply_status_code statusCode, susi_comm_cmd_t rcvy_rep_id)
{
	char * pSendVal = NULL;
	char * str = rcvyMsg;
	int jsonStrlen = Pack_replyMsg(str, statusCode, &pSendVal);
	if(jsonStrlen > 0 && pSendVal != NULL)
	{
		g_sendcbf(&g_PluginInfo, rcvy_rep_id, pSendVal, strlen(pSendVal)+1, NULL, NULL);
	}
	if(pSendVal)free(pSendVal);	
}

void SendReplySuccessMsg_Rcvy(char *rcvyMsg, susi_comm_cmd_t rcvy_rep_id)
{
	SendReplyMsg_Rcvy(rcvyMsg, oprt_success, rcvy_rep_id);
}

void SendReplyFailMsg_Rcvy(char *rcvyMsg, susi_comm_cmd_t rcvy_rep_id)
{
	SendReplyMsg_Rcvy(rcvyMsg, oprt_fail, rcvy_rep_id);
}

//message pack functionsint 
int RcvyReplyMsgPack(cJSON *pInReply, char ** pOutReply)
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

int Pack_replyMsg(char *pReplyMsg, rcvy_reply_status_code statusCode, char **outputStr)
{
	char * out = NULL;
	int outLen = 0;
	cJSON *pSUSICommDataItem = NULL;
	if(pReplyMsg == NULL || outputStr == NULL) return outLen;
	pSUSICommDataItem = cJSON_CreateObject();

	cJSON_AddStringToObject(pSUSICommDataItem, RCVY_REP_MSG, pReplyMsg);
	cJSON_AddItemToObject(pSUSICommDataItem, OPRATION_STATUS_CODE, cJSON_CreateNumber(statusCode));

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

static cJSON * cJSON_CreateRcvyStatusInfo(recovery_status_t * pRcvyStatus)
{
	cJSON * pRcvyStatusInfoItem = NULL;
	if(!pRcvyStatus) return NULL;
	pRcvyStatusInfoItem = cJSON_CreateObject();
	if(pRcvyStatus->isInstalled) cJSON_AddStringToObject(pRcvyStatusInfoItem, RCVY_STATUS_IS_INSTALLED, "True");
	else cJSON_AddStringToObject(pRcvyStatusInfoItem, RCVY_STATUS_IS_INSTALLED, "False");

	if(pRcvyStatus->isActivated) cJSON_AddStringToObject(pRcvyStatusInfoItem, RCVY_STATUS_IS_ACTIVATED, "True");
	else cJSON_AddStringToObject(pRcvyStatusInfoItem, RCVY_STATUS_IS_ACTIVATED, "False");

	if(pRcvyStatus->isExpired) cJSON_AddStringToObject(pRcvyStatusInfoItem, RCVY_STATUS_IS_EXPIRED, "True");
	else cJSON_AddStringToObject(pRcvyStatusInfoItem, RCVY_STATUS_IS_EXPIRED, "False");

	if(pRcvyStatus->isExistASZ) cJSON_AddStringToObject(pRcvyStatusInfoItem, RCVY_STATUS_IS_EXIST_ASZ, "True");
	else cJSON_AddStringToObject(pRcvyStatusInfoItem, RCVY_STATUS_IS_EXIST_ASZ, "False");

	if(pRcvyStatus->isExistNewerVer) cJSON_AddStringToObject(pRcvyStatusInfoItem, RCVY_STATUS_IS_NEWERVER, "True");
	else cJSON_AddStringToObject(pRcvyStatusInfoItem, RCVY_STATUS_IS_NEWERVER, "False");

	if(pRcvyStatus->isAcrReady) cJSON_AddStringToObject(pRcvyStatusInfoItem, RCVY_STATUS_IS_ACRREADY, "True");
	else cJSON_AddStringToObject(pRcvyStatusInfoItem, RCVY_STATUS_IS_ACRREADY, "False");

	if(strlen(pRcvyStatus->version)) cJSON_AddStringToObject(pRcvyStatusInfoItem, RCVY_STATUS_VERSION, pRcvyStatus->version);
	else cJSON_AddStringToObject(pRcvyStatusInfoItem, RCVY_STATUS_VERSION, "1.0.0");

	cJSON_AddStringToObject(pRcvyStatusInfoItem, RCVY_STATUS_LATEST_BK_TIME, pRcvyStatus->lastBackupTime);
	cJSON_AddStringToObject(pRcvyStatusInfoItem, RCVY_STATUS_ACTION_MSG, pRcvyStatus->actionMsg);
	cJSON_AddStringToObject(pRcvyStatusInfoItem, RCVY_STATUS_LWARNING_MSG, pRcvyStatus->lastWarningMsg);

	return pRcvyStatusInfoItem;
}

int Pack_rcvy_status_rep(char * inputstr, char ** outputStr)
{	
	recovery_status_t * pRcvyStatus;

	char * out = NULL;
	int outLen = 0;
	cJSON *pSUSICommDataItem = NULL;
	if(inputstr == NULL || outputStr == NULL) return FALSE;
	pSUSICommDataItem = cJSON_CreateObject();

	pRcvyStatus = (recovery_status_t *)inputstr;
	cJSON_AddRcvyStatusInfoToObject(pSUSICommDataItem, RCVY_STATUS_INFO, pRcvyStatus);
	//status code: success always
	cJSON_AddItemToObject(pSUSICommDataItem, OPRATION_STATUS_CODE, cJSON_CreateNumber(oprt_success));

	out = cJSON_PrintUnformatted(pSUSICommDataItem);
	outLen = strlen(out) + 1;
	*outputStr = (char *)malloc(outLen);
	memset(*outputStr, 0, outLen);
	strcpy(*outputStr, out);
	cJSON_Delete(pSUSICommDataItem);	
	printf("%s\n",out);	
	free(out);
	return outLen;
}

//message parse functions
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

int parse_rcvy_install_req(char *inputstr, recovery_install_params * OutParams)
{
	int iRet = 0;
	cJSON* root = NULL;
	cJSON* pSUSICommDataItem = NULL;

	if(!inputstr) return false;
	root = cJSON_Parse(inputstr);
	if(!root) return false;

	pSUSICommDataItem = cJSON_GetObjectItem(root, AGENTINFO_BODY_STRUCT);
	if(!pSUSICommDataItem)
	{
		cJSON_Delete(root);
		return false;
	}

	{
		cJSON *pParamsItem = NULL;
		int parseOK = 0;
		recovery_install_params * pInstallParams = NULL;
		pInstallParams = OutParams;
		pParamsItem = cJSON_GetObjectItem(pSUSICommDataItem, RCVY_IS_INSTALL_THEN_ACTIVE);
		if(pParamsItem != NULL)
		{
			if(!_stricmp(pParamsItem->valuestring, "True"))
			{
				pInstallParams->isThenActive = 1;
				parseOK = 1;
			}
			else if(!_stricmp(pParamsItem->valuestring, "False"))
			{
				pInstallParams->isThenActive = 0;
				parseOK = 1;
			}
			if(parseOK)
			{
				parseOK = 0;
				pParamsItem = cJSON_GetObjectItem(pSUSICommDataItem, RCVY_INSTALL_ASZ_PERSENT);
				if(pParamsItem != NULL)
				{
					pInstallParams->ASZPersent = pParamsItem->valueint;
					pParamsItem = cJSON_GetObjectItem(pSUSICommDataItem, RCVY_FTP_DL_USERNAME);
					if(pParamsItem)
					{
						strcpy(pInstallParams->ftpuserName, pParamsItem->valuestring);
						pParamsItem = cJSON_GetObjectItem(pSUSICommDataItem, RCVY_FTP_DL_PASSWORD);
						if(pParamsItem)
						{
							strcpy(pInstallParams->ftpPassword, pParamsItem->valuestring);
							pParamsItem = cJSON_GetObjectItem(pSUSICommDataItem, RCVY_FTP_DL_PORT);
							if(pParamsItem)
							{
								pInstallParams->port = pParamsItem->valueint;
								pParamsItem = cJSON_GetObjectItem(pSUSICommDataItem, RCVY_FTP_DL_PATH);
								if(pParamsItem)
								{
									strcpy(pInstallParams->installerPath, pParamsItem->valuestring);
									pParamsItem = cJSON_GetObjectItem(pSUSICommDataItem, RCVY_FTP_DL_MD5);
									if(pParamsItem)
									{
										strcpy(pInstallParams->md5, pParamsItem->valuestring);
										parseOK = 1;
									}
								}
							}
						}
					}
				}
			}
		}
		if(parseOK) 
			iRet = 1;
		return iRet;

	}
}


int parse_rcvy_update_req(char *inputstr, recovery_install_params * OutParams)
{
	int iRet = 0;
	cJSON* root = NULL;
	cJSON* pSUSICommDataItem = NULL;
	if(!inputstr) return iRet;

	root = cJSON_Parse(inputstr);
	if(!root) return iRet;

	pSUSICommDataItem = cJSON_GetObjectItem(root, AGENTINFO_BODY_STRUCT);
	if(!pSUSICommDataItem)
	{
		cJSON_Delete(root);
		return iRet;
	}

	{
		cJSON *pParamsItem = NULL;
		int parseOK = 0;
		recovery_install_params * pInstallParams = NULL;
		//if(!OutParams) break;
		pInstallParams = OutParams;
		pParamsItem = cJSON_GetObjectItem(pSUSICommDataItem, RCVY_FTP_DL_USERNAME);
		if(pParamsItem)
		{
			strcpy(pInstallParams->ftpuserName, pParamsItem->valuestring);
			pParamsItem = cJSON_GetObjectItem(pSUSICommDataItem, RCVY_FTP_DL_PASSWORD);
			if(pParamsItem)
			{
				strcpy(pInstallParams->ftpPassword, pParamsItem->valuestring);
				pParamsItem = cJSON_GetObjectItem(pSUSICommDataItem, RCVY_FTP_DL_PORT);
				if(pParamsItem)
				{
					pInstallParams->port = pParamsItem->valueint;
					pParamsItem = cJSON_GetObjectItem(pSUSICommDataItem, RCVY_FTP_DL_PATH);
					if(pParamsItem)
					{
						strcpy(pInstallParams->installerPath, pParamsItem->valuestring);
						pParamsItem = cJSON_GetObjectItem(pSUSICommDataItem, RCVY_FTP_DL_MD5);
						if(pParamsItem)
						{
							strcpy(pInstallParams->md5, pParamsItem->valuestring);
							parseOK = 1;
						}
					}
				}
			}
		}
		if(parseOK) iRet = 1;
	}
		return iRet;
}


int parse_rcvy_create_asz_req(char *inputstr, char * outputStr)
{
	int iRet = 0;
	cJSON* root = NULL;
	cJSON* pSUSICommDataItem = NULL;
	if(!inputstr) return iRet;

	root = cJSON_Parse(inputstr);
	if(!root) return iRet;

	pSUSICommDataItem = cJSON_GetObjectItem(root, AGENTINFO_BODY_STRUCT);
	if(!pSUSICommDataItem)
	{
		cJSON_Delete(root);
		return iRet;
	}

	{
		cJSON *pParamsItem = NULL;
		char *pCreateASZParams = NULL;
		pCreateASZParams = outputStr;
		pParamsItem = cJSON_GetObjectItem(pSUSICommDataItem, RCVY_CREATE_ASZ_PARAMS);
		if(pCreateASZParams && pParamsItem)
		{
			strcpy(pCreateASZParams, pParamsItem->valuestring);
			iRet = 1;
		}
	}
	return iRet;
}
