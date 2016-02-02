#include "Parser.h"
#include <stdio.h>


#define cJSON_AddSWMThrInfoToObject(object, name, pT) cJSON_AddItemToObject(object, name, cJSON_CreateMonObjList(pT))
#define cJSON_AddPrcMonInfoListToObject(object, name, pP) cJSON_AddItemToObject(object, name, cJSON_CreatePrcMonInfoList(pP))
#define cJSON_AddPrcMonInfoToObject(object, name, pP) cJSON_AddItemToObject(object, name, cJSON_CreatePrcMonInfo(pP))
#define cJSON_AddEventLogListToObject(object, name, pE) cJSON_AddItemToObject(object, name, cJSON_CreateEventLogList(pE))
#define cJSON_AddEventLogInfoToObject(object, name, pE) cJSON_AddItemToObject(object, name, cJSON_CreateEventLogInfo(pE))
#define cJSON_AddSysMonInfoToObject(object, name, pS) cJSON_AddItemToObject(object, name, cJSON_CreateSysMonInfo(pS))

static int cJSON_GetMonObjListMemThr(cJSON * pSWMThrArrayItem, mon_obj_info_list monObjList);

int ParseReceivedData(void* data, int datalen, int * cmdID)
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


int Parse_swm_get_event_log_req(char *pCommData, swm_get_sys_event_log_param_t* OutParams)
{
	//cJSON *pParamsItem = NULL;
	swm_get_sys_event_log_param_t * pEventParams = NULL;

	cJSON *pGetEventLogParamsItem = NULL;
	cJSON* root = NULL;
	cJSON* body = NULL;
	//cJSON* params = NULL;
	//cJSON* target = NULL;
	cJSON* pSubItem = NULL;

	if(!pCommData) return false;

	root = cJSON_Parse(pCommData);
	if(!root) return false;

	body = cJSON_GetObjectItem(root, AGENTINFO_BODY_STRUCT);
	if(!body)
	{
		cJSON_Delete(root);
		return false;
	}

	pGetEventLogParamsItem = cJSON_GetObjectItem(body, SWM_GET_EVENT_LOG_PARAMS);  
	pEventParams = OutParams;


	if(pEventParams && pGetEventLogParamsItem)
	{
		pSubItem = cJSON_GetObjectItem(pGetEventLogParamsItem, SWM_EVENT_LOG_GROUP);
		if(pSubItem)
		{
			char * tmpStr = pSubItem->valuestring;
			memcpy(pEventParams->eventGroup, tmpStr, strlen(tmpStr));
			pSubItem = cJSON_GetObjectItem(pGetEventLogParamsItem, SWM_START_TIME_PARAM);
			if(pSubItem)
			{
				char * tmpStr = pSubItem->valuestring;
				memcpy(pEventParams->startTimeStr, tmpStr, strlen(tmpStr));
				pSubItem = cJSON_GetObjectItem(pGetEventLogParamsItem, SWM_END_TIME_PARAM);
				if(pSubItem)
				{
					char * tmpStr = pSubItem->valuestring;
					memcpy(pEventParams->endTimeStr, tmpStr, strlen(tmpStr));
				}
			}
		}
	}

	cJSON_Delete(root);
	return true;
}


BOOL Parse_swm_kill_prc_req(char *inputstr, unsigned int * outputVal)
{
	cJSON *pParamsItem = NULL;
	cJSON* root = NULL;
	cJSON* body = NULL;
	unsigned int *pPrcID = NULL;

	if(!inputstr) return false;

	root = cJSON_Parse(inputstr);
	if(!root) return false;

	body = cJSON_GetObjectItem(root, AGENTINFO_BODY_STRUCT);
	if(!body)
	{
		cJSON_Delete(root);
		return false;
	}

	pPrcID = outputVal;
	pParamsItem = cJSON_GetObjectItem(body, SWM_PRC_ID);
	if(pPrcID && pParamsItem)
	{
		*pPrcID = pParamsItem->valueint;
	}
	cJSON_Delete(root);
	return true;
}

BOOL Parse_swm_restart_prc_req(char *inputstr, unsigned int* outputVal)
{
	cJSON *pParamsItem = NULL;
	cJSON* root = NULL;
	cJSON* body = NULL;
	unsigned int *pPrcID = NULL;

	if(!inputstr) return false;

	root = cJSON_Parse(inputstr);
	if(!root) return false;

	body = cJSON_GetObjectItem(root, AGENTINFO_BODY_STRUCT);
	if(!body)
	{
		cJSON_Delete(root);
		return false;
	}

	pPrcID = outputVal;
	pParamsItem = cJSON_GetObjectItem(body, SWM_PRC_ID);
	if(pPrcID && pParamsItem)
	{
		*pPrcID = pParamsItem->valueint;
	}
	cJSON_Delete(root);
	return true;
}

BOOL Parse_swm_set_pmi_auto_upload_req(char *inputstr, swm_auto_upload_params_t *outputVal)
{
	cJSON *pAutoUploadParamsItem = NULL;
	swm_auto_upload_params_t * pAutoUploadParams = NULL;

	//cJSON *pParamsItem = NULL;
	cJSON* root = NULL;
	cJSON* body = NULL;
	//unsigned int *pPrcID = NULL;
	cJSON* pSubItem = NULL;
	if(!inputstr) return false;

	root = cJSON_Parse(inputstr);
	if(!root) return false;

	body = cJSON_GetObjectItem(root, AGENTINFO_BODY_STRUCT);
	if(!body)
	{
		cJSON_Delete(root);
		return false;
	}

	pAutoUploadParams = outputVal;

	pAutoUploadParamsItem = cJSON_GetObjectItem(body, SWM_AUTO_UPLOAD_PARAMS);
	if(pAutoUploadParamsItem && pAutoUploadParams)
	{
		pSubItem = cJSON_GetObjectItem(pAutoUploadParamsItem, SWM_AUTO_UPLOAD_INTERVAL_MS);
		if(pSubItem)
		{
			pAutoUploadParams->auto_upload_interval_ms = pSubItem->valueint;
			pSubItem = cJSON_GetObjectItem(pAutoUploadParamsItem, SWM_AUTO_UPLOAD_TIMEOUT_MS);
			if(pSubItem)
			{
				pAutoUploadParams->auto_upload_timeout_ms = pSubItem->valueint;
			}
		}
	}
	cJSON_Delete(root);
	return true;
}


static cJSON * cJSON_CreateMonObjCpuThr(mon_obj_info_t * pMonObjInfo)
{
	cJSON * pThrObjInfoItem = NULL;
	if(!pMonObjInfo || !pMonObjInfo->prcName) return NULL;
	pThrObjInfoItem = cJSON_CreateObject();
	cJSON_AddStringToObject(pThrObjInfoItem, SWM_THR_PRC_NAME, pMonObjInfo->prcName);
	cJSON_AddNumberToObject(pThrObjInfoItem, SWM_THR_PRC_ID, pMonObjInfo->prcID);
	cJSON_AddNumberToObject(pThrObjInfoItem, SWM_THR_MAX, pMonObjInfo->cpuThrItem.maxThreshold);
	cJSON_AddNumberToObject(pThrObjInfoItem, SWM_THR_MIN, pMonObjInfo->cpuThrItem.minThreshold);
	cJSON_AddNumberToObject(pThrObjInfoItem, SWM_THR_LASTING_TIME_S, pMonObjInfo->cpuThrItem.lastingTimeS);
	cJSON_AddNumberToObject(pThrObjInfoItem, SWM_THR_INTERVAL_TIME_S, pMonObjInfo->cpuThrItem.intervalTimeS);
	cJSON_AddNumberToObject(pThrObjInfoItem, SWM_THR_TYPE, pMonObjInfo->cpuThrItem.thresholdType);
	if(pMonObjInfo->cpuThrItem.isEnable == 1)
	{
		cJSON_AddStringToObject(pThrObjInfoItem, SWM_THR_ENABLE, "true");
	}
	else
	{
		cJSON_AddStringToObject(pThrObjInfoItem, SWM_THR_ENABLE, "false");
	}
	cJSON_AddNumberToObject(pThrObjInfoItem, SWM_THR_ACTTYPE, pMonObjInfo->cpuAct);
	if(pMonObjInfo->cpuActCmd == NULL || strlen(pMonObjInfo->cpuActCmd) <= 0)
	{
		cJSON_AddStringToObject(pThrObjInfoItem, SWM_THR_ACTCMD, "None");
	}
	else
	{
		cJSON_AddStringToObject(pThrObjInfoItem, SWM_THR_ACTCMD, pMonObjInfo->cpuActCmd);
	}

	return pThrObjInfoItem;
}


static cJSON * cJSON_CreateMonObjMemThr(mon_obj_info_t * pMonObjInfo)
{
	cJSON * pThrObjInfoItem = NULL;
	if(!pMonObjInfo || !pMonObjInfo->prcName) return NULL;
	pThrObjInfoItem = cJSON_CreateObject();
	cJSON_AddStringToObject(pThrObjInfoItem, SWM_THR_PRC_NAME, pMonObjInfo->prcName);
	cJSON_AddNumberToObject(pThrObjInfoItem, SWM_THR_PRC_ID, pMonObjInfo->prcID);
	cJSON_AddNumberToObject(pThrObjInfoItem, SWM_THR_MAX, pMonObjInfo->memThrItem.maxThreshold);
	cJSON_AddNumberToObject(pThrObjInfoItem, SWM_THR_MIN, pMonObjInfo->memThrItem.minThreshold);
	cJSON_AddNumberToObject(pThrObjInfoItem, SWM_THR_LASTING_TIME_S, pMonObjInfo->memThrItem.lastingTimeS);
	cJSON_AddNumberToObject(pThrObjInfoItem, SWM_THR_INTERVAL_TIME_S, pMonObjInfo->memThrItem.intervalTimeS);
	cJSON_AddNumberToObject(pThrObjInfoItem, SWM_THR_TYPE, pMonObjInfo->memThrItem.thresholdType);
	if(pMonObjInfo->memThrItem.isEnable == 1)
	{
		cJSON_AddStringToObject(pThrObjInfoItem, SWM_THR_ENABLE, "true");
	}
	else
	{
		cJSON_AddStringToObject(pThrObjInfoItem, SWM_THR_ENABLE, "false");
	}
	cJSON_AddNumberToObject(pThrObjInfoItem, SWM_THR_ACTTYPE, pMonObjInfo->memAct);
	if(pMonObjInfo->memActCmd == NULL || strlen(pMonObjInfo->memActCmd) <= 0)
	{
		cJSON_AddStringToObject(pThrObjInfoItem, SWM_THR_ACTCMD, "None");
	}
	else
	{
		cJSON_AddStringToObject(pThrObjInfoItem, SWM_THR_ACTCMD, pMonObjInfo->memActCmd);
	}

	return pThrObjInfoItem;
}

static cJSON * cJSON_CreateMonObjList(mon_obj_info_list monObjList)
{
	cJSON * pSWMThrInfoItem = NULL;
	cJSON * pCpuThrListItem = NULL, *pMemThrListItem = NULL;
	//int index = 0;
	if(!monObjList) return NULL;
	pSWMThrInfoItem = cJSON_CreateObject();

	if(monObjList->next)
	{
		pCpuThrListItem = cJSON_CreateArray();
		cJSON_AddItemToObject(pSWMThrInfoItem, SWM_THR_CPU, pCpuThrListItem);
		pMemThrListItem = cJSON_CreateArray();
		cJSON_AddItemToObject(pSWMThrInfoItem, SWM_THR_MEM, pMemThrListItem);
		{
			mon_obj_info_node_t * curMonObjNode = monObjList->next;
			while(curMonObjNode)
			{
				mon_obj_info_t * pMonObj = &curMonObjNode->monObjInfo;
				if(pMonObj->prcName)
				{
					cJSON * pThrObjItem = cJSON_CreateMonObjCpuThr(pMonObj);
					if(pThrObjItem)cJSON_AddItemToArray(pCpuThrListItem , pThrObjItem);
					pThrObjItem = cJSON_CreateMonObjMemThr(pMonObj);
					if(pThrObjItem)cJSON_AddItemToArray(pMemThrListItem , pThrObjItem);
				}
				curMonObjNode = curMonObjNode->next;
			}
		}
	}

	return pSWMThrInfoItem;
}


int Pack_swm_set_mon_objs_req(susi_comm_data_t * pCommData, char ** outputStr)
{
	char * out = NULL;
	int outLen = 0;
	cJSON *root = NULL, *pSUSICommDataItem = NULL;
	if(pCommData == NULL || outputStr == NULL) return outLen;
	root = cJSON_CreateObject();
	pSUSICommDataItem = cJSON_CreateObject();
	cJSON_AddItemToObject(root, AGENTINFO_BODY_STRUCT, pSUSICommDataItem);
	cJSON_AddNumberToObject(pSUSICommDataItem, AGENTINFO_CMDTYPE, pCommData->comm_Cmd);
#ifdef COMM_DATA_WITH_JSON
	cJSON_AddNumberToObject(pSUSICommDataItem, AGENTINFO_REQID, pCommData->reqestID);
#endif

	{
		mon_obj_info_list monObjList = NULL;
		mon_obj_info_list * pMonObjList = &monObjList;
		if(!pCommData->message) return false;
		memcpy(pMonObjList, pCommData->message, sizeof(mon_obj_info_list));
		if(!monObjList) return false;
		cJSON_AddSWMThrInfoToObject(root, SWM_THR_INFO, monObjList);
	}

	out = cJSON_PrintUnformatted(root);
	outLen = strlen(out) + 1;
	*outputStr = (char *)(malloc(outLen));
	memset(*outputStr, 0, outLen);
	strcpy(*outputStr, out);
	cJSON_Delete(root);	
	printf("%s\n",out);	
	free(out);
	return outLen;
}


///////////////////

static int cJSON_GetMonObjItemCpuThr(cJSON * pSWMThrItem, mon_obj_info_t * pMonObjInfo)
{
	int iRet = 0;
	if(!pSWMThrItem|| !pMonObjInfo) return iRet;
	{
		cJSON * pSubItem = NULL;
		pSubItem = cJSON_GetObjectItem(pSWMThrItem, SWM_THR_PRC_NAME);
		if(pSubItem)
		{
			int prcNameLen = strlen(pSubItem->valuestring) + 1;
			pMonObjInfo->prcName = (char *)malloc(prcNameLen);
			memset(pMonObjInfo->prcName, 0, prcNameLen);
			strcpy(pMonObjInfo->prcName, pSubItem->valuestring);

			pSubItem = cJSON_GetObjectItem(pSWMThrItem, SWM_THR_PRC_ID);
			if(pSubItem)
			{
				pMonObjInfo->prcID = pSubItem->valueint;
			}
			else
			{
				pMonObjInfo->prcID = 0;
			}

			memset(pMonObjInfo->cpuThrItem.tagName, 0, sizeof(pMonObjInfo->cpuThrItem.tagName));
			strcpy(pMonObjInfo->cpuThrItem.tagName, "CpuUsage");
			pMonObjInfo->cpuThrItem.isEnable = 0;
			pSubItem = cJSON_GetObjectItem(pSWMThrItem, SWM_THR_ENABLE);
			if(pSubItem)
			{
				if(!_stricmp(pSubItem->valuestring, "true"))
				{
					pMonObjInfo->cpuThrItem.isEnable = 1;
				}
			}
			pSubItem = cJSON_GetObjectItem(pSWMThrItem, SWM_THR_MAX);
			if(pSubItem)
			{
				pMonObjInfo->cpuThrItem.maxThreshold = (float)pSubItem->valuedouble;
			}
			else
			{
				pMonObjInfo->cpuThrItem.maxThreshold = DEF_INVALID_VALUE;
			}

			pSubItem = cJSON_GetObjectItem(pSWMThrItem, SWM_THR_MIN);
			if(pSubItem)
			{
				pMonObjInfo->cpuThrItem.minThreshold = (float)pSubItem->valuedouble;
			}
			else
			{
				pMonObjInfo->cpuThrItem.minThreshold = DEF_INVALID_VALUE;
			}

			pSubItem = cJSON_GetObjectItem(pSWMThrItem, SWM_THR_TYPE);
			if(pSubItem)
			{
				pMonObjInfo->cpuThrItem.thresholdType = pSubItem->valueint;
			}
			else
			{
				pMonObjInfo->cpuThrItem.thresholdType = DEF_THR_UNKNOW_TYPE;
			}

			pSubItem = cJSON_GetObjectItem(pSWMThrItem, SWM_THR_LASTING_TIME_S);
			if(pSubItem)
			{
				pMonObjInfo->cpuThrItem.lastingTimeS = pSubItem->valueint;
			}
			else
			{
				pMonObjInfo->cpuThrItem.lastingTimeS = DEF_INVALID_TIME;
			}

			pSubItem = cJSON_GetObjectItem(pSWMThrItem, SWM_THR_INTERVAL_TIME_S);
			if(pSubItem)
			{
				pMonObjInfo->cpuThrItem.intervalTimeS = pSubItem->valueint;
			}
			else
			{
				pMonObjInfo->cpuThrItem.intervalTimeS = DEF_INVALID_TIME;
			}
			pMonObjInfo->cpuThrItem.checkResultValue.vi = DEF_INVALID_VALUE;
			pMonObjInfo->cpuThrItem.checkSourceValueList.head = NULL;
			pMonObjInfo->cpuThrItem.checkSourceValueList.nodeCnt = 0;
			pMonObjInfo->cpuThrItem.checkType = ck_type_avg;
			pMonObjInfo->cpuThrItem.isNormal = 1;
			pMonObjInfo->cpuThrItem.repThrTime = 0;


			pSubItem = cJSON_GetObjectItem(pSWMThrItem, SWM_THR_ACTTYPE);
			if(pSubItem)
			{
				pMonObjInfo->cpuAct = pSubItem->valueint;
			}
			else
			{
				pMonObjInfo->cpuAct = prc_act_unknown;
			}

			pSubItem = cJSON_GetObjectItem(pSWMThrItem, SWM_THR_ACTCMD);
			if(pSubItem)
			{
				int actCmdLen = strlen(pSubItem->valuestring) + 1;
				pMonObjInfo->cpuActCmd = (char *)malloc(actCmdLen);
				memset(pMonObjInfo->cpuActCmd, 0, actCmdLen);
				strcpy(pMonObjInfo->cpuActCmd, pSubItem->valuestring);
			}
			else
			{
				int actCmdLen = strlen("None") + 1;
				pMonObjInfo->cpuActCmd = (char *)malloc(actCmdLen);
				memset(pMonObjInfo->cpuActCmd, 0, actCmdLen);
				strcpy(pMonObjInfo->cpuActCmd, "None");
			}

			pMonObjInfo->isValid = 1;
			iRet = 1;
		}
	}
	return iRet;
}

static int cJSON_GetMonObjListCpuThr(cJSON * pSWMThrArrayItem, mon_obj_info_list monObjList)
{
	int iRet = 0;
	if(pSWMThrArrayItem == NULL || monObjList == NULL) return iRet;
	{
		int i = 0;
		int nCount = cJSON_GetArraySize(pSWMThrArrayItem);
		for(i = 0; i < nCount; i++)
		{
			cJSON *pSWMThrItem = cJSON_GetArrayItem(pSWMThrArrayItem, i);
			if(pSWMThrItem)
			{
				int iRet = 0;
				mon_obj_info_node_t * pCurMonObjInfoNode = NULL;
				pCurMonObjInfoNode = (mon_obj_info_node_t *)malloc(sizeof(mon_obj_info_node_t));
				memset(pCurMonObjInfoNode, 0, sizeof(mon_obj_info_node_t));

				iRet = cJSON_GetMonObjItemCpuThr(pSWMThrItem, &(pCurMonObjInfoNode->monObjInfo));
				if(!iRet)
				{
					if(pCurMonObjInfoNode)
					{

						free(pCurMonObjInfoNode);
						pCurMonObjInfoNode = NULL;
					}
					continue;
				}
				pCurMonObjInfoNode->monObjInfo.prcResponse = 1;
				memset(pCurMonObjInfoNode->monObjInfo.memThrItem.tagName, 0, sizeof(pCurMonObjInfoNode->monObjInfo.memThrItem.tagName));
				strcpy(pCurMonObjInfoNode->monObjInfo.memThrItem.tagName, "MemUsage");
				pCurMonObjInfoNode->monObjInfo.memThrItem.isEnable = 0;
				pCurMonObjInfoNode->monObjInfo.memThrItem.maxThreshold = DEF_INVALID_VALUE;
				pCurMonObjInfoNode->monObjInfo.memThrItem.minThreshold = DEF_INVALID_VALUE;
				pCurMonObjInfoNode->monObjInfo.memThrItem.thresholdType = DEF_THR_UNKNOW_TYPE;
				pCurMonObjInfoNode->monObjInfo.memThrItem.lastingTimeS = DEF_INVALID_TIME;
				pCurMonObjInfoNode->monObjInfo.memThrItem.intervalTimeS = DEF_INVALID_TIME;
				pCurMonObjInfoNode->monObjInfo.memThrItem.checkResultValue.vi = DEF_INVALID_VALUE;
				pCurMonObjInfoNode->monObjInfo.memThrItem.checkSourceValueList.head = NULL;
				pCurMonObjInfoNode->monObjInfo.memThrItem.checkSourceValueList.nodeCnt = 0;
				pCurMonObjInfoNode->monObjInfo.memThrItem.checkType = ck_type_unknow;
				pCurMonObjInfoNode->monObjInfo.memThrItem.repThrTime = 0;
				pCurMonObjInfoNode->monObjInfo.memThrItem.isNormal = 1;
				pCurMonObjInfoNode->monObjInfo.memAct = prc_act_unknown;
				pCurMonObjInfoNode->monObjInfo.memActCmd = NULL;
				pCurMonObjInfoNode->next = monObjList->next;
				monObjList->next = pCurMonObjInfoNode;
			}
		}
		iRet = 1;
	}
	return iRet;
}

static int cJSON_GetSWMThrInfo(cJSON * pFatherItem, mon_obj_info_list monObjList)
{
	int iRet = 0;//, iFlag = 0;
	if(!pFatherItem|| !monObjList) return iRet;
	{
		cJSON * pSWMThrInfoItem = cJSON_GetObjectItem(pFatherItem, SWM_THR_INFO);
		if(pSWMThrInfoItem)
		{
			cJSON * pSWMThresholdObjListItem = NULL;
			iRet = 1;
			pSWMThresholdObjListItem = cJSON_GetObjectItem(pSWMThrInfoItem, SWM_THR_CPU);
			if(pSWMThresholdObjListItem)
			{
				cJSON_GetMonObjListCpuThr(pSWMThresholdObjListItem, monObjList);
			}

			pSWMThresholdObjListItem = cJSON_GetObjectItem(pSWMThrInfoItem, SWM_THR_MEM);
			if(pSWMThresholdObjListItem)
			{
				cJSON_GetMonObjListMemThr(pSWMThresholdObjListItem, monObjList);
			}
		}
	}
	return iRet;
}


static int cJSON_GetMonObjItemMemThr(cJSON * pSWMThrItem, mon_obj_info_t * pMonObjInfo)
{
	int iRet = 0;
	if(!pSWMThrItem|| !pMonObjInfo) return iRet;
	{
		cJSON * pSubItem = NULL;
		pSubItem = cJSON_GetObjectItem(pSWMThrItem, SWM_THR_PRC_NAME);
		if(pSubItem)
		{
			if(pMonObjInfo->prcName == NULL)
			{
				int prcNameLen = strlen(pSubItem->valuestring) + 1;
				pMonObjInfo->prcName = (char *)malloc(prcNameLen);
				memset(pMonObjInfo->prcName, 0, prcNameLen);
				strcpy(pMonObjInfo->prcName, pSubItem->valuestring);
			}

			if(pMonObjInfo->prcID == 0)
			{
				pSubItem = cJSON_GetObjectItem(pSWMThrItem, SWM_THR_PRC_ID);
				if(pSubItem)
				{
					pMonObjInfo->prcID= pSubItem->valueint;
				}
				else
				{
					pMonObjInfo->prcID = 0;
				}
			}

			memset(pMonObjInfo->memThrItem.tagName, 0, sizeof(pMonObjInfo->memThrItem.tagName));
			strcpy(pMonObjInfo->memThrItem.tagName, "MemUsage");
			pMonObjInfo->memThrItem.isEnable = 0;
			pSubItem = cJSON_GetObjectItem(pSWMThrItem, SWM_THR_ENABLE);
			if(pSubItem)
			{
				if(!_stricmp(pSubItem->valuestring, "true"))
				{
					pMonObjInfo->memThrItem.isEnable = 1;
				}
			}
			pSubItem = cJSON_GetObjectItem(pSWMThrItem, SWM_THR_MAX);
			if(pSubItem)
			{
				pMonObjInfo->memThrItem.maxThreshold = (float)pSubItem->valuedouble;
			}
			else
			{
				pMonObjInfo->memThrItem.maxThreshold = DEF_INVALID_VALUE;
			}

			pSubItem = cJSON_GetObjectItem(pSWMThrItem, SWM_THR_MIN);
			if(pSubItem)
			{
				pMonObjInfo->memThrItem.minThreshold = (float)pSubItem->valuedouble;
			}
			else
			{
				pMonObjInfo->memThrItem.minThreshold = DEF_INVALID_VALUE;
			}

			pSubItem = cJSON_GetObjectItem(pSWMThrItem, SWM_THR_TYPE);
			if(pSubItem)
			{
				pMonObjInfo->memThrItem.thresholdType = pSubItem->valueint;
			}
			else
			{
				pMonObjInfo->memThrItem.thresholdType = DEF_THR_UNKNOW_TYPE;
			}

			pSubItem = cJSON_GetObjectItem(pSWMThrItem, SWM_THR_LASTING_TIME_S);
			if(pSubItem)
			{
				pMonObjInfo->memThrItem.lastingTimeS = pSubItem->valueint;
			}
			else
			{
				pMonObjInfo->memThrItem.lastingTimeS = DEF_INVALID_TIME;
			}

			pSubItem = cJSON_GetObjectItem(pSWMThrItem, SWM_THR_INTERVAL_TIME_S);
			if(pSubItem)
			{
				pMonObjInfo->memThrItem.intervalTimeS = pSubItem->valueint;
			}
			else
			{
				pMonObjInfo->memThrItem.intervalTimeS = DEF_INVALID_TIME;
			}
			pMonObjInfo->memThrItem.checkResultValue.vi = DEF_INVALID_VALUE;
			pMonObjInfo->memThrItem.checkSourceValueList.head = NULL;
			pMonObjInfo->memThrItem.checkSourceValueList.nodeCnt = 0;
			pMonObjInfo->memThrItem.checkType = ck_type_avg;
			pMonObjInfo->memThrItem.isNormal = 1;
			pMonObjInfo->memThrItem.repThrTime = 0;

			pSubItem = cJSON_GetObjectItem(pSWMThrItem, SWM_THR_ACTTYPE);
			if(pSubItem)
			{
				pMonObjInfo->memAct = pSubItem->valueint;
			}
			else
			{
				pMonObjInfo->memAct = prc_act_unknown;
			}

			if(pMonObjInfo->memActCmd != NULL) free(pMonObjInfo->memActCmd);
			pSubItem = cJSON_GetObjectItem(pSWMThrItem, SWM_THR_ACTCMD);
			if(pSubItem)
			{
				int actCmdLen = strlen(pSubItem->valuestring) + 1;
				pMonObjInfo->memActCmd = (char *)malloc(actCmdLen);
				memset(pMonObjInfo->memActCmd, 0, actCmdLen);
				strcpy(pMonObjInfo->memActCmd, pSubItem->valuestring);
			}
			else
			{
				int actCmdLen = strlen("None") + 1;
				pMonObjInfo->memActCmd = (char *)malloc(actCmdLen);
				memset(pMonObjInfo->memActCmd, 0, actCmdLen);
				strcpy(pMonObjInfo->memActCmd, "None");
			}

			pMonObjInfo->isValid = 1;
			iRet = 1;
		}
	}
	return iRet;
}

static int cJSON_GetMonObjListMemThr(cJSON * pSWMThrArrayItem, mon_obj_info_list monObjList)
{
	int iRet = 0;
	if(pSWMThrArrayItem == NULL || monObjList == NULL) return iRet;
	{
		int i = 0;
		int nCount = cJSON_GetArraySize(pSWMThrArrayItem);
		for(i = 0; i < nCount; i++)
		{
			cJSON *pSWMThrItem = cJSON_GetArrayItem(pSWMThrArrayItem, i);
			if(pSWMThrItem)
			{
				int prcID = 0;
				char prcName[128] = {0};
				cJSON * pSubItem = NULL;
				pSubItem = cJSON_GetObjectItem(pSWMThrItem, SWM_THR_PRC_NAME);
				if(pSubItem)
				{
					strcpy(prcName, pSubItem->valuestring);
				}
				pSubItem = cJSON_GetObjectItem(pSWMThrItem, SWM_THR_PRC_ID);
				if(pSubItem)
				{
					prcID = pSubItem->valueint;
				}

				if(strlen(prcName))
				{
					int iRet = 0;
					int isNew = 0;
					mon_obj_info_node_t * pCurMonObjInfoNode = NULL;
					mon_obj_info_node_t * pTmpMonObjInfoNode = monObjList->next;
					while(pTmpMonObjInfoNode)
					{
						if(pTmpMonObjInfoNode->monObjInfo.prcName && !strcmp(prcName, pTmpMonObjInfoNode->monObjInfo.prcName) && prcID == pTmpMonObjInfoNode->monObjInfo.prcID)
						{
							pCurMonObjInfoNode = pTmpMonObjInfoNode;
							break;
						}
						pTmpMonObjInfoNode = pTmpMonObjInfoNode->next;
					}

					if(pCurMonObjInfoNode == NULL)
					{
						pCurMonObjInfoNode = (mon_obj_info_node_t *)malloc(sizeof(mon_obj_info_node_t));
						memset(pCurMonObjInfoNode, 0, sizeof(mon_obj_info_node_t));
						isNew = 1;
					}

					iRet = cJSON_GetMonObjItemMemThr(pSWMThrItem, &(pCurMonObjInfoNode->monObjInfo));
					if(!iRet)
					{
						if(pCurMonObjInfoNode && isNew)
						{
							free(pCurMonObjInfoNode);
							pCurMonObjInfoNode = NULL;
						}
						continue;
					}

					if(isNew)
					{
						pCurMonObjInfoNode->monObjInfo.prcResponse = 1;
						memset(pCurMonObjInfoNode->monObjInfo.cpuThrItem.tagName, 0, sizeof(pCurMonObjInfoNode->monObjInfo.cpuThrItem.tagName));
						strcpy(pCurMonObjInfoNode->monObjInfo.cpuThrItem.tagName, "CpuUsage");
						pCurMonObjInfoNode->monObjInfo.cpuThrItem.isEnable = 0;
						pCurMonObjInfoNode->monObjInfo.cpuThrItem.maxThreshold = DEF_INVALID_VALUE;
						pCurMonObjInfoNode->monObjInfo.cpuThrItem.minThreshold = DEF_INVALID_VALUE;
						pCurMonObjInfoNode->monObjInfo.cpuThrItem.thresholdType = DEF_THR_UNKNOW_TYPE;
						pCurMonObjInfoNode->monObjInfo.cpuThrItem.lastingTimeS = DEF_INVALID_TIME;
						pCurMonObjInfoNode->monObjInfo.cpuThrItem.intervalTimeS = DEF_INVALID_TIME;
						pCurMonObjInfoNode->monObjInfo.cpuThrItem.checkResultValue.vi = DEF_INVALID_VALUE;
						pCurMonObjInfoNode->monObjInfo.cpuThrItem.checkSourceValueList.head = NULL;
						pCurMonObjInfoNode->monObjInfo.cpuThrItem.checkSourceValueList.nodeCnt = 0;
						pCurMonObjInfoNode->monObjInfo.cpuThrItem.checkType = ck_type_unknow;
						pCurMonObjInfoNode->monObjInfo.cpuThrItem.repThrTime = 0;
						pCurMonObjInfoNode->monObjInfo.cpuThrItem.isNormal = 1;
						pCurMonObjInfoNode->monObjInfo.cpuAct = prc_act_unknown;
						pCurMonObjInfoNode->monObjInfo.cpuActCmd = NULL;
						pCurMonObjInfoNode->next = monObjList->next;
						monObjList->next = pCurMonObjInfoNode;
					}
				}
			}
		}
		iRet = 1;
	}
	return iRet;
}

int Parse_swm_set_mon_objs_req(char * inputStr, char * monVal)
{
	int iRet = 0;
	//cJSON *pParamsItem = NULL;
	cJSON* root = NULL;
	cJSON* body = NULL;
	//unsigned int *pPrcID = NULL;

	if(!inputStr) return iRet;

	root = cJSON_Parse(inputStr);
	if(!root) return iRet;

	body = cJSON_GetObjectItem(root, AGENTINFO_BODY_STRUCT);
	if(!body)
	{
		cJSON_Delete(root);
		return iRet;
	}

	{
		mon_obj_info_list monObjList = NULL;
		mon_obj_info_list * pMonObjList = &monObjList;
		if(!monVal) return false;
		memcpy(pMonObjList, monVal, sizeof(mon_obj_info_list));
		if(!monObjList) return false;              
		iRet = cJSON_GetSWMThrInfo(body, monObjList);
	}
	cJSON_Delete(root);
	return iRet;
}


////////////////////
static cJSON * cJSON_CreatePrcMonInfo(prc_mon_info_t * pPrcMonInfo)
{
	cJSON * pPrcMonInfoItem = NULL;
	if(!pPrcMonInfo) return NULL;
	pPrcMonInfoItem = cJSON_CreateObject();
	if(pPrcMonInfo->prcName)
	{
		cJSON_AddStringToObject(pPrcMonInfoItem, SWM_PRC_NAME, pPrcMonInfo->prcName);
	}
	if(pPrcMonInfo->ownerName)
	{
		cJSON_AddStringToObject(pPrcMonInfoItem, SWM_PRC_OWNER, pPrcMonInfo->ownerName);
	}
	cJSON_AddNumberToObject(pPrcMonInfoItem, SWM_PRC_ID, pPrcMonInfo->prcID);
	cJSON_AddNumberToObject(pPrcMonInfoItem, SWM_PRC_CPU_USAGE, pPrcMonInfo->cpuUsage);
	cJSON_AddNumberToObject(pPrcMonInfoItem, SWM_PRC_MEM_USAGE, pPrcMonInfo->memUsage);
	cJSON_AddNumberToObject(pPrcMonInfoItem, SWM_PRC_IS_ACTIVE, pPrcMonInfo->isActive);
	return pPrcMonInfoItem;
}

static cJSON * cJSON_CreatePrcMonInfoList(prc_mon_info_node_t * prcMonInfoList)
{
	cJSON *pPrcMonInfoListItem = NULL;
	if(!prcMonInfoList) return NULL;
	pPrcMonInfoListItem = cJSON_CreateArray();
	{
		prc_mon_info_node_t * head = prcMonInfoList;
		prc_mon_info_node_t * curNode = head->next;
		while(curNode)
		{
			cJSON * pPrcMonInfoItem = cJSON_CreateObject();
			cJSON_AddPrcMonInfoToObject(pPrcMonInfoItem, SWM_PRC_MON_INFO, &curNode->prcMonInfo);
			cJSON_AddItemToArray(pPrcMonInfoListItem, pPrcMonInfoItem);
			curNode = curNode->next;
		}
	}
	return pPrcMonInfoListItem;
}

int Parser_swm_get_pmi_list_rep(char *pCommData, char **outputStr)
{
	prc_mon_info_node_t * pPrcMonInfoHead = NULL;
	prc_mon_info_node_t ** ppPrcMonInfoHead = &pPrcMonInfoHead;

	char * out = NULL;
	int outLen = 0;
	cJSON *pSUSICommDataItem = NULL;
	if(pCommData == NULL || outputStr == NULL) return outLen;
	pSUSICommDataItem = cJSON_CreateObject();

	memcpy(ppPrcMonInfoHead, pCommData, sizeof(prc_mon_info_node_t *));
	cJSON_AddPrcMonInfoListToObject(pSUSICommDataItem, SWM_PRC_MON_INFO_LIST, pPrcMonInfoHead);//I'm sure

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


static cJSON * cJSON_CreateEventLogInfo(PEVENTLOGINFO pEventLogInfo)
{
	cJSON *pEventLogInfoItem = NULL;
	if(!pEventLogInfo) return pEventLogInfoItem;
	pEventLogInfoItem = cJSON_CreateObject();

	if(pEventLogInfo->eventGroup)
	{
		cJSON_AddStringToObject(pEventLogInfoItem, SWM_EVENT_LOG_GROUP, pEventLogInfo->eventGroup);
	}
	if(pEventLogInfo->eventType)
	{
		cJSON_AddStringToObject(pEventLogInfoItem, SWM_EVENT_LOG_TYPE, pEventLogInfo->eventType);
	}
	if(pEventLogInfo->eventTimestamp)
	{
		cJSON_AddStringToObject(pEventLogInfoItem, SWM_EVENT_LOG_TIMESTAMP, pEventLogInfo->eventTimestamp);
	}
	cJSON_AddNumberToObject(pEventLogInfoItem, SWM_EVENT_LOG_ID, pEventLogInfo->eventID);
	if(pEventLogInfo->eventSource)
	{
		cJSON_AddStringToObject(pEventLogInfoItem, SWM_EVENT_LOG_SOURCE, pEventLogInfo->eventSource);
	}
	if(pEventLogInfo->eventUser)
	{
		cJSON_AddStringToObject(pEventLogInfoItem, SWM_EVENT_LOG_USER, pEventLogInfo->eventUser);
	}
	if(pEventLogInfo->eventComputer)
	{
		cJSON_AddStringToObject(pEventLogInfoItem, SWM_EVENT_LOG_COMPUTER, pEventLogInfo->eventComputer);
	}
	if(pEventLogInfo->eventDescription)
	{
		cJSON_AddStringToObject(pEventLogInfoItem, SWM_EVENT_LOG_DESCRIPTION, pEventLogInfo->eventDescription);
	}
	return pEventLogInfoItem;
}

static cJSON * cJSON_CreateEventLogList(EVENTLOGINFOLIST eventLogInfoList)
{
	cJSON *pEventLogInfoListItem = NULL;
	if(!eventLogInfoList) return NULL;
	pEventLogInfoListItem = cJSON_CreateArray();
	{
		PEVENTLOGINFONODE head = eventLogInfoList;
		PEVENTLOGINFONODE curNode = head->next;
		while(curNode)
		{
			cJSON * pEventLogInfoItem = cJSON_CreateObject();
			cJSON_AddEventLogInfoToObject(pEventLogInfoItem, SWM_EVENT_LOG_INFO, &curNode->eventLogInfo);
			cJSON_AddItemToArray(pEventLogInfoListItem, pEventLogInfoItem);
			curNode = curNode->next;
		}
	}
	return pEventLogInfoListItem;
}

int Parser_swm_event_log_list_rep(char *pCommData, char **outputStr)
{
	EVENTLOGINFOLIST pEventLogList = NULL;
	EVENTLOGINFOLIST* ppEventLogList = &pEventLogList;

	char * out = NULL;
	int outLen = 0;
	cJSON *pSUSICommDataItem = NULL;
	if(pCommData == NULL || outputStr == NULL) return outLen;
	pSUSICommDataItem = cJSON_CreateObject();

	memcpy(ppEventLogList, pCommData, sizeof(EVENTLOGINFOLIST));
	cJSON_AddEventLogListToObject(pSUSICommDataItem, SWM_EVENT_LOG_LIST, pEventLogList);

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

int Parser_swm_mon_prc_event_rep(char *pCommData, char **outputStr)
{
	swm_thr_rep_info_t *pThrRepInfo = (swm_thr_rep_info_t *)pCommData;

	char * out = NULL;
	int outLen = 0;
	cJSON *pSUSICommDataItem = NULL;
	if(pCommData == NULL || outputStr == NULL) return outLen;
	pSUSICommDataItem = cJSON_CreateObject();

	if(pThrRepInfo)
	{
		if(pThrRepInfo->isTotalNormal)
		{
			cJSON_AddStringToObject(pSUSICommDataItem, SWM_NORMAL_STATUS, "True");
		}
		else
		{
			cJSON_AddStringToObject(pSUSICommDataItem, SWM_NORMAL_STATUS, "False");
		}

		if(pThrRepInfo->repInfo)
		{
			cJSON_AddStringToObject(pSUSICommDataItem, SWM_PRC_MON_EVENT_MSG, pThrRepInfo->repInfo);
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

static cJSON * cJSON_CreateSysMonInfo(sys_mon_info_t * pSysMonInfo)
{
	cJSON * pSysMonInfoItem = NULL;
	if(!pSysMonInfo) return NULL;
	pSysMonInfoItem = cJSON_CreateObject();
	cJSON_AddNumberToObject(pSysMonInfoItem, SWM_SYS_CPU_USAGE, pSysMonInfo->cpuUsage);
	cJSON_AddNumberToObject(pSysMonInfoItem, SWM_SYS_TOTAL_PHYS_MEM, pSysMonInfo->totalPhysMemoryKB);
	cJSON_AddNumberToObject(pSysMonInfoItem, SWM_SYS_AVAIL_PHYS_MEM, pSysMonInfo->availPhysMemoryKB);
	return pSysMonInfoItem;
}

int Parser_swm_get_smi_rep(char *pCommData, char **outputStr)
{
	sys_mon_info_t * pSysMonInfo = (sys_mon_info_t *)pCommData;

	char * out = NULL;
	int outLen = 0;
	cJSON *pSUSICommDataItem = NULL;
	if(pCommData == NULL || outputStr == NULL) return outLen;
	pSUSICommDataItem = cJSON_CreateObject();

	cJSON_AddSysMonInfoToObject(pSUSICommDataItem, SWM_SYS_MON_INFO, pSysMonInfo);

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


int Parser_string(char *pCommData, char **outputStr, int repCommandID)
{
	char * out = NULL;
	int outLen = 0;
	cJSON *pSUSICommDataItem = NULL;
	if(pCommData == NULL || outputStr == NULL) return outLen;
	pSUSICommDataItem = cJSON_CreateObject();

	switch(repCommandID)
	{
		case swm_set_mon_objs_rep:
			{
				cJSON_AddStringToObject(pSUSICommDataItem, SWM_SET_MON_OBJS_REP, pCommData);
								break;
			}
		case swm_error_rep:
			{
				cJSON_AddStringToObject(pSUSICommDataItem, SWM_ERROR_REP, pCommData);
				break;
			}
		//case swm_kill_prc_rep:
		//	{
		//		break;
		//	}
		//case swm_restart_prc_rep:
		//case swm_del_all_mon_objs_rep:
		//case swm_set_pmi_auto_upload_rep:
		//case swm_set_pmi_reqp_req:
		default:
			{
				cJSON_AddStringToObject(pSUSICommDataItem, SWM_SET_MON_OBJS_REP, pCommData);
				break;
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