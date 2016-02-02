#include "Parser.h"
#include <stdio.h>


#define cJSON_AddSWMThrInfoToObject(object, name, pT) cJSON_AddItemToObject(object, name, cJSON_CreateMonObjList(pT))
#define cJSON_AddSensorInfoListToObject(object, name, sN) cJSON_AddItemToObject(object, name, cJSON_CreateSensorInfoList(sN))
//#define cJSON_AddSysMonInfoToObject(object, name, pS) cJSON_AddItemToObject(object, name, cJSON_CreateSysMonInfo(pS))
static int cJSON_GetMonObjListThr(cJSON * pSWMThrArrayItem, mon_obj_info_list monObjList);
static cJSON * cJSON_CreateSensorInfoList(sensor_info_node_t * sensorInfoList);

int cJSON_GetObjectCount(cJSON *object)	{cJSON *c=object->child; int i = 0; while(c)i++, c=c->next; return i;}

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
/*
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
}*/

static cJSON * cJSON_CreateThrInfoItem(mon_obj_info_t * pMonObjInfo)
{
	cJSON * pThrObjInfoItem = NULL;
	if(!pMonObjInfo || !pMonObjInfo->prcName) return NULL;
	pThrObjInfoItem = cJSON_CreateObject();
	//cJSON_AddStringToObject(pThrObjInfoItem, SWM_THR_PRC_NAME, pMonObjInfo->prcName);
	//cJSON_AddNumberToObject(pThrObjInfoItem, SWM_THR_PRC_ID, pMonObjInfo->prcID);
	cJSON_AddStringToObject(pThrObjInfoItem, SWM_N_FLAG, pMonObjInfo->path);
	if(!(pMonObjInfo->prcID < 0))
		cJSON_AddNumberToObject(pThrObjInfoItem, SWM_THR_PRC_ID, pMonObjInfo->prcID);
	cJSON_AddNumberToObject(pThrObjInfoItem, SWM_THR_MAX, pMonObjInfo->thrItem.maxThreshold);
	cJSON_AddNumberToObject(pThrObjInfoItem, SWM_THR_MIN, pMonObjInfo->thrItem.minThreshold);
	cJSON_AddNumberToObject(pThrObjInfoItem, SWM_THR_LASTING_TIME_S, pMonObjInfo->thrItem.lastingTimeS);
	cJSON_AddNumberToObject(pThrObjInfoItem, SWM_THR_INTERVAL_TIME_S, pMonObjInfo->thrItem.intervalTimeS);
	cJSON_AddNumberToObject(pThrObjInfoItem, SWM_THR_TYPE, pMonObjInfo->thrItem.thresholdType);
	if(pMonObjInfo->thrItem.isEnable == 1)
	{
		cJSON_AddStringToObject(pThrObjInfoItem, SWM_THR_ENABLE, "true");
	}
	else
	{
		cJSON_AddStringToObject(pThrObjInfoItem, SWM_THR_ENABLE, "false");
	}
	cJSON_AddNumberToObject(pThrObjInfoItem, SWM_THR_ACTTYPE, pMonObjInfo->act);
	if(pMonObjInfo->actCmd == NULL || strlen(pMonObjInfo->actCmd) <= 0)
	{
		cJSON_AddStringToObject(pThrObjInfoItem, SWM_THR_ACTCMD, "None");
	}
	else
	{
		cJSON_AddStringToObject(pThrObjInfoItem, SWM_THR_ACTCMD, pMonObjInfo->actCmd);
	}

	return pThrObjInfoItem;
}

static cJSON * cJSON_CreateThrInofArray(mon_obj_info_list monObjList)
{
	cJSON * pThrArray = NULL;
	if(!monObjList) return NULL;
	pThrArray = cJSON_CreateArray();//cJSON_CreateObject();

	if(monObjList->next)
	{
			mon_obj_info_node_t * curMonObjNode = monObjList->next;
			while(curMonObjNode)
			{
				mon_obj_info_t * pMonObj = &curMonObjNode->monObjInfo;
				if(pMonObj->prcName)
				{
					cJSON * pThrObjItem = cJSON_CreateThrInfoItem(pMonObj);
					if(pThrObjItem)cJSON_AddItemToArray(pThrArray , pThrObjItem);
				}
				curMonObjNode = curMonObjNode->next;
			}
	}
	return pThrArray;
}

int Pack_swm_set_mon_objs_req(susi_comm_data_t * pCommData, char ** outputStr)
{
	char * out = NULL;
	int outLen = 0;
	cJSON *root = NULL, *pSUSICommDataItem = NULL;
	cJSON *pThrArray = NULL;
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
		if(!pCommData->message) 
		{
			cJSON_Delete(root);	
			return false;
		}
		memcpy(pMonObjList, pCommData->message, sizeof(mon_obj_info_list));
		if(!monObjList) 
		{
			cJSON_Delete(root);	
			return false;
		}
		//cJSON_AddSWMThrInfoToObject(root, SWM_THR_INFO, monObjList);
		pThrArray = cJSON_CreateThrInofArray(monObjList);//cJSON_CreateArray();
		cJSON_AddItemToObject(pSUSICommDataItem, SWM_THR_INFO, pThrArray);
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
/*
static int cJSON_GetMonObjItemCpuThr(cJSON * pSWMThrItem, mon_obj_info_t * pMonObjInfo)
{
	int iRet = 0;
	if(!pSWMThrItem|| !pMonObjInfo) return iRet;
	{
		cJSON * pSubItem = NULL;
//		pSubItem = cJSON_GetObjectItem(pSWMThrItem, SWM_THR_PRC_NAME);
//		if(pSubItem)
//		{
//			int prcNameLen = strlen(pSubItem->valuestring) + 1;
//			pMonObjInfo->prcName = (char *)malloc(prcNameLen);
//			memset(pMonObjInfo->prcName, 0, prcNameLen);
//			strcpy(pMonObjInfo->prcName, pSubItem->valuestring);
//
//			pSubItem = cJSON_GetObjectItem(pSWMThrItem, SWM_THR_PRC_ID);
//			if(pSubItem)
//			{
//				pMonObjInfo->prcID = pSubItem->valueint;
//			}
//			else
//			{
//				pMonObjInfo->prcID = 0;
//			}
//
			memset(pMonObjInfo->thrItem.tagName, 0, sizeof(pMonObjInfo->cpuThrItem.tagName));
			strcpy(pMonObjInfo->thrItem.tagName, "CpuUsage");
			pMonObjInfo->thrItem.isEnable = 0;
			pSubItem = cJSON_GetObjectItem(pSWMThrItem, SWM_THR_ENABLE);
			if(pSubItem)
			{
				if(!_stricmp(pSubItem->valuestring, "true"))
				{
					pMonObjInfo->thrItem.isEnable = 1;
				}
			}
			pSubItem = cJSON_GetObjectItem(pSWMThrItem, SWM_THR_MAX);
			if(pSubItem)
			{
				pMonObjInfo->thrItem.maxThreshold = (float)pSubItem->valuedouble;
			}
			else
			{
				pMonObjInfo->thrItem.maxThreshold = DEF_INVALID_VALUE;
			}

			pSubItem = cJSON_GetObjectItem(pSWMThrItem, SWM_THR_MIN);
			if(pSubItem)
			{
				pMonObjInfo->thrItem.minThreshold = (float)pSubItem->valuedouble;
			}
			else
			{
				pMonObjInfo->thrItem.minThreshold = DEF_INVALID_VALUE;
			}

			pSubItem = cJSON_GetObjectItem(pSWMThrItem, SWM_THR_TYPE);
			if(pSubItem)
			{
				pMonObjInfo->thrItem.thresholdType = pSubItem->valueint;
			}
			else
			{
				pMonObjInfo->thrItem.thresholdType = DEF_THR_UNKNOW_TYPE;
			}

			pSubItem = cJSON_GetObjectItem(pSWMThrItem, SWM_THR_LASTING_TIME_S);
			if(pSubItem)
			{
				pMonObjInfo->thrItem.lastingTimeS = pSubItem->valueint;
			}
			else
			{
				pMonObjInfo->thrItem.lastingTimeS = DEF_INVALID_TIME;
			}

			pSubItem = cJSON_GetObjectItem(pSWMThrItem, SWM_THR_INTERVAL_TIME_S);
			if(pSubItem)
			{
				pMonObjInfo->thrItem.intervalTimeS = pSubItem->valueint;
			}
			else
			{
				pMonObjInfo->thrItem.intervalTimeS = DEF_INVALID_TIME;
			}
			pMonObjInfo->thrItem.checkResultValue.vi = DEF_INVALID_VALUE;
			pMonObjInfo->thrItem.checkSourceValueList.head = NULL;
			pMonObjInfo->thrItem.checkSourceValueList.nodeCnt = 0;
			pMonObjInfo->thrItem.checkType = ck_type_avg;
			pMonObjInfo->thrItem.isNormal = 1;
			pMonObjInfo->thrItem.repThrTime = 0;


			pSubItem = cJSON_GetObjectItem(pSWMThrItem, SWM_THR_ACTTYPE);
			if(pSubItem)
			{
				pMonObjInfo->act = pSubItem->valueint;
			}
			else
			{
				pMonObjInfo->act = prc_act_unknown;
			}

			pSubItem = cJSON_GetObjectItem(pSWMThrItem, SWM_THR_ACTCMD);
			if(pSubItem)
			{
				int actCmdLen = strlen(pSubItem->valuestring) + 1;
				pMonObjInfo->actCmd = (char *)malloc(actCmdLen);
				memset(pMonObjInfo->actCmd, 0, actCmdLen);
				strcpy(pMonObjInfo->actCmd, pSubItem->valuestring);
			}
			else
			{
				int actCmdLen = strlen("None") + 1;
				pMonObjInfo->actCmd = (char *)malloc(actCmdLen);
				memset(pMonObjInfo->actCmd, 0, actCmdLen);
				strcpy(pMonObjInfo->actCmd, "None");
			}

			pMonObjInfo->isValid = 1;
			iRet = 1;
		//}
	}
	return iRet;
}
*/
/*
static int cJSON_GetMonObjListCpuThr(cJSON * pSWMThrArrayItem, mon_obj_info_list monObjList)
{
	int iRet = 0;
	if(pSWMThrArrayItem == NULL || monObjList == NULL) return iRet;
	{
		int i = 0;
		cJSON * subItem = NULL;
		cJSON * valItem = NULL;
		int nCount = cJSON_GetArraySize(pSWMThrArrayItem);
		for(i = 0; i < nCount; i++)
		{
			int prcID = 0;
			char prcName[128] = {0};
			//int iRet = 0;
			//mon_obj_info_node_t * pCurMonObjInfoNode = NULL;
			//cJSON *pSWMThrItem = cJSON_GetArrayItem(pSWMThrArrayItem, i);
			subItem = cJSON_GetArrayItem(pSWMThrArrayItem, i);
			if(subItem)
			{
				///
				valItem = cJSON_GetObjectItem(subItem, SWM_N_FLAG);
				if(valItem)
				{
					int len = strlen(valItem->valuestring)+1;
					char *tmpPathStr = NULL;
					char * sliceStr[16] = {NULL};
					char * buf = NULL;
					int j = 0;
					tmpPathStr = (char*)malloc(len);
					memset(tmpPathStr, 0, len);
					strcpy(tmpPathStr, valItem->valuestring);
					buf = tmpPathStr;
					while(j < 16 && (sliceStr[j] = strtok(buf, "/")))
					{
						j++;
						buf = NULL;
					}
					
					if(!strcmp(sliceStr[3], SWM_THR_CPU))  //"ProcessMonitor/Process Monitor Info/6016-conhost.exe/cpu"
					{
						int k = 0;
						int procID = 0;
						char *procName[2] = {NULL};
						buf = sliceStr[2];
						while(k < 2 && (procName[k] = strtok(buf, "-")))
						{
							k++;
							buf = NULL;
						}

						if(k>0)
						{
							prcID = atoi(procName[0]);
							strcpy(prcName, procName[1]);
						}
						
					}
					else
					{
						free(tmpPathStr);
						continue;
					}
					free(tmpPathStr);
				}
				////
				if(strlen(prcName))
				{
					int nRet = 0;
					mon_obj_info_node_t * pCurMonObjInfoNode = NULL;
					pCurMonObjInfoNode = (mon_obj_info_node_t *)malloc(sizeof(mon_obj_info_node_t));
					memset(pCurMonObjInfoNode, 0, sizeof(mon_obj_info_node_t));
//Wei.Gang add
					if(pCurMonObjInfoNode->monObjInfo.prcName == NULL)
					{
						int prcNameLen = strlen(prcName) + 1;
						pCurMonObjInfoNode->monObjInfo.prcName = (char *)malloc(prcNameLen);
						memset(pCurMonObjInfoNode->monObjInfo.prcName, 0, prcNameLen);
						strcpy(pCurMonObjInfoNode->monObjInfo.prcName, prcName);
					}

					if(pCurMonObjInfoNode->monObjInfo.prcID == 0)
					{
						pCurMonObjInfoNode->monObjInfo.prcID= prcID;
					}
//Wei.Gang add end

					nRet = cJSON_GetMonObjItemCpuThr(subItem, &(pCurMonObjInfoNode->monObjInfo));
					if(!nRet)
					{
						if(pCurMonObjInfoNode)
						{
							free(pCurMonObjInfoNode);
							pCurMonObjInfoNode = NULL;
						}
						continue;
					}
					pCurMonObjInfoNode->monObjInfo.prcResponse = 1;
//					memset(pCurMonObjInfoNode->monObjInfo.memThrItem.tagName, 0, sizeof(pCurMonObjInfoNode->monObjInfo.memThrItem.tagName));
//					strcpy(pCurMonObjInfoNode->monObjInfo.memThrItem.tagName, "MemUsage");
//					pCurMonObjInfoNode->monObjInfo.memThrItem.isEnable = 0;
//					pCurMonObjInfoNode->monObjInfo.memThrItem.maxThreshold = DEF_INVALID_VALUE;
//					pCurMonObjInfoNode->monObjInfo.memThrItem.minThreshold = DEF_INVALID_VALUE;
//					pCurMonObjInfoNode->monObjInfo.memThrItem.thresholdType = DEF_THR_UNKNOW_TYPE;
//					pCurMonObjInfoNode->monObjInfo.memThrItem.lastingTimeS = DEF_INVALID_TIME;
//					pCurMonObjInfoNode->monObjInfo.memThrItem.intervalTimeS = DEF_INVALID_TIME;
//					pCurMonObjInfoNode->monObjInfo.memThrItem.checkResultValue.vi = DEF_INVALID_VALUE;
//					pCurMonObjInfoNode->monObjInfo.memThrItem.checkSourceValueList.head = NULL;
//					pCurMonObjInfoNode->monObjInfo.memThrItem.checkSourceValueList.nodeCnt = 0;
//					pCurMonObjInfoNode->monObjInfo.memThrItem.checkType = ck_type_unknow;
//					pCurMonObjInfoNode->monObjInfo.memThrItem.repThrTime = 0;
//					pCurMonObjInfoNode->monObjInfo.memThrItem.isNormal = 1;
//					pCurMonObjInfoNode->monObjInfo.memAct = prc_act_unknown;
//					pCurMonObjInfoNode->monObjInfo.memActCmd = NULL;
//					pCurMonObjInfoNode->next = monObjList->next;
//					monObjList->next = pCurMonObjInfoNode;
				}
			}
		}
		iRet = 1;
	}
	return iRet;
}
*/
static int cJSON_GetSWMThrInfo(cJSON * pFatherItem, mon_obj_info_list monObjList)
{
	int iRet = 1;//, iFlag = 0;
	if(!pFatherItem|| !monObjList) return iRet;
	{
		cJSON * eItem = cJSON_GetObjectItem(pFatherItem, SWM_THR_INFO);
		if(eItem)
		{
			//cJSON_GetMonObjListCpuThr(eItem, monObjList);
			cJSON_GetMonObjListThr(eItem, monObjList);
		}
	}
	return iRet;
}
static int cJSON_GetMonObjItemThr(cJSON * pSWMThrItem, mon_obj_info_t * pMonObjInfo)
{
	int iRet = 0;
	if(!pSWMThrItem|| !pMonObjInfo) return iRet;
	{
		cJSON * pSubItem = NULL;

//		memset(pMonObjInfo->thrItem.tagName, 0, sizeof(pMonObjInfo->thrItem.tagName));
//		strcpy(pMonObjInfo->thrItem.tagName, SWM_THR_MEM);//"MemUsage");
		pMonObjInfo->thrItem.isEnable = 0;
		pSubItem = cJSON_GetObjectItem(pSWMThrItem, SWM_THR_ENABLE);
		if(pSubItem)
		{
			if(!_stricmp(pSubItem->valuestring, "true"))
			{
				pMonObjInfo->thrItem.isEnable = 1;
			}
		}
		//if enable = 0, don't add this node
		if(!pMonObjInfo->thrItem.isEnable)
			return iRet;
		//if enable end
		pSubItem = cJSON_GetObjectItem(pSWMThrItem, SWM_THR_MAX);
		if(pSubItem)
		{
			pMonObjInfo->thrItem.maxThreshold = (float)pSubItem->valuedouble;
		}
		else
		{
			pMonObjInfo->thrItem.maxThreshold = DEF_INVALID_VALUE;
		}

		pSubItem = cJSON_GetObjectItem(pSWMThrItem, SWM_THR_MIN);
		if(pSubItem)
		{
			pMonObjInfo->thrItem.minThreshold = (float)pSubItem->valuedouble;
		}
		else
		{
			pMonObjInfo->thrItem.minThreshold = DEF_INVALID_VALUE;
		}
		
		pSubItem = cJSON_GetObjectItem(pSWMThrItem, SWM_THR_TYPE);
		if(pSubItem)
		{
			pMonObjInfo->thrItem.thresholdType = pSubItem->valueint;
		}
		else
		{
			pMonObjInfo->thrItem.thresholdType = DEF_THR_UNKNOW_TYPE;
		}

		pSubItem = cJSON_GetObjectItem(pSWMThrItem, SWM_THR_LASTING_TIME_S);
		if(pSubItem)
		{
			pMonObjInfo->thrItem.lastingTimeS = pSubItem->valueint;
		}
		else
		{
			pMonObjInfo->thrItem.lastingTimeS = DEF_INVALID_TIME;
		}

		pSubItem = cJSON_GetObjectItem(pSWMThrItem, SWM_THR_INTERVAL_TIME_S);
		if(pSubItem)
		{
			pMonObjInfo->thrItem.intervalTimeS = pSubItem->valueint;
		}
		else
		{
			pMonObjInfo->thrItem.intervalTimeS = DEF_INVALID_TIME;
		}
		pMonObjInfo->thrItem.checkResultValue.vi = DEF_INVALID_VALUE;
		pMonObjInfo->thrItem.checkSourceValueList.head = NULL;
		pMonObjInfo->thrItem.checkSourceValueList.nodeCnt = 0;
		pMonObjInfo->thrItem.checkType = ck_type_avg;
		pMonObjInfo->thrItem.isNormal = 1;
		pMonObjInfo->thrItem.repThrTime = 0;

		pSubItem = cJSON_GetObjectItem(pSWMThrItem, SWM_THR_ACTTYPE);
		if(pSubItem)
		{
			pMonObjInfo->act = pSubItem->valueint;
		}
		else
		{
			pMonObjInfo->act = prc_act_unknown;
		}
		if(pMonObjInfo->actCmd != NULL) free(pMonObjInfo->actCmd);
		pSubItem = cJSON_GetObjectItem(pSWMThrItem, SWM_THR_ACTCMD);
		if(pSubItem)
		{
			int actCmdLen = strlen(pSubItem->valuestring) + 1;
			pMonObjInfo->actCmd = (char *)malloc(actCmdLen);
			memset(pMonObjInfo->actCmd, 0, actCmdLen);
			strcpy(pMonObjInfo->actCmd, pSubItem->valuestring);
		}
		else
		{
			int actCmdLen = strlen("None") + 1;
			pMonObjInfo->actCmd = (char *)malloc(actCmdLen);
			memset(pMonObjInfo->actCmd, 0, actCmdLen);
			strcpy(pMonObjInfo->actCmd, "None");
		}

		pMonObjInfo->isValid = 1;
		iRet = 1;
	}
	return iRet;
}

static int cJSON_GetMonObjListThr(cJSON * pSWMThrArrayItem, mon_obj_info_list monObjList)
{
	int iRet = 0;
	if(pSWMThrArrayItem == NULL || monObjList == NULL) return iRet;
	{
		int i = 0;
		cJSON * subItem = NULL;
		cJSON * valItem = NULL;
		int nCount = cJSON_GetArraySize(pSWMThrArrayItem);
		for(i = 0; i < nCount; i++)
		{
			subItem = cJSON_GetArrayItem(pSWMThrArrayItem, i);
			if(subItem)
			{
				int prcID = 0;
				int pathDepth = 0;
				char prcName[128] = {0};
				bool isHasPID = false;
				char * sliceStr[16] = {NULL};
				char *tmpPath = NULL;
				char *nPath = NULL;
				cJSON * pSubItem = NULL;
				/**********************/
				valItem = cJSON_GetObjectItem(subItem, SWM_N_FLAG);
				if(valItem)
				{
					int len = strlen(valItem->valuestring)+1;
					char * buf = NULL;
					int j = 0;
					tmpPath = (char*)malloc(len);
					nPath = (char*)malloc(len);
					memset(tmpPath, 0, len);
					memset(nPath, 0, len);
					strcpy(tmpPath, valItem->valuestring);
					strcpy(nPath, valItem->valuestring);
					buf = tmpPath;
					while(j<16 && (sliceStr[j] = strtok(buf, "/")))
					{
						j++;
						buf = NULL;
					}

					if(--j > 0)
						pathDepth = j;
					if(pathDepth >= PATH_DEPTH)//!strcmp(sliceStr[3], SWM_THR_MEM))  //"ProcessMonitor/Process Monitor Info/6016-conhost.exe/cpu"
					{
						//int k = 0;
						//int procID = 0;
						//char *procName[2] = {NULL};
						cJSON * prcIDItem = NULL; 
						buf = sliceStr[2];
						if(buf)
							strcpy(prcName, buf);

						prcIDItem = cJSON_GetObjectItem(subItem, SWM_THR_PRC_ID);
						if(prcIDItem)
						{
							prcID = prcIDItem->valueint; //atoi(prcIDItem->valueint);
							isHasPID = TRUE;
						}
						else
						{
							prcID = THR_PRC_SAME_NAME_ALL_FLAG;
							isHasPID = FALSE;
						}
						/*while(k < 2 && (procName[k] = strtok(buf, "-")))
						{
							k++;
							buf = NULL;
						}

						if(k>1)
						{
							prcID = atoi(procName[0]);
							strcpy(prcName, procName[1]);
							isHasPID = TRUE;
						}
						else if(k>0)
						{
							prcID = THR_PRC_SAME_NAME_ALL_FLAG;
							strcpy(prcName, procName[0]);
							isHasPID = FALSE;
						}*/

					}
					else if(pathDepth >= PATH_DEPTH - 1) //"ProcessMonitor/System Monitor Info/cpu or mem"
					{
						prcID = THR_SYSINFO_FLAG;
						strcpy(prcName, SWM_SYS_MON_INFO);
					}
					else
					{
						if(tmpPath) free(tmpPath);
						if(nPath) free(nPath);
						continue;
					}
				}
				/*********************/
				if(strlen(prcName))
				{
					int nRet = 0;
					int isNew = 0;
					mon_obj_info_node_t * pCurMonObjInfoNode = NULL;
					mon_obj_info_node_t * pTmpMonObjInfoNode = monObjList->next;
					while(pTmpMonObjInfoNode)
					{
						if(pTmpMonObjInfoNode->monObjInfo.prcName && !strcmp(prcName, pTmpMonObjInfoNode->monObjInfo.prcName) 
							&& prcID == pTmpMonObjInfoNode->monObjInfo.prcID && !strcmp(nPath, pTmpMonObjInfoNode->monObjInfo.path))
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
//Wei.Gang add
					if(pCurMonObjInfoNode->monObjInfo.path == NULL)
					{
						int pathLen = strlen(nPath) + 1;
						pCurMonObjInfoNode->monObjInfo.path = (char *)malloc(pathLen);
						memset(pCurMonObjInfoNode->monObjInfo.path, 0, pathLen);
						strcpy(pCurMonObjInfoNode->monObjInfo.path, nPath);

						//tagName: "mem" or "cpu"
						memset(pCurMonObjInfoNode->monObjInfo.thrItem.tagName, 0, sizeof(pCurMonObjInfoNode->monObjInfo.thrItem.tagName));
						if(!strcmp(sliceStr[pathDepth], SWM_THR_MEM))
							strcpy(pCurMonObjInfoNode->monObjInfo.thrItem.tagName, SWM_THR_MEM);//"MemUsage");
						else if(!strcmp(sliceStr[pathDepth], SWM_THR_CPU))
							strcpy(pCurMonObjInfoNode->monObjInfo.thrItem.tagName, SWM_THR_CPU);
					}
					if(nPath)
						free (nPath);
					if(tmpPath)
						free(tmpPath);

					if(pCurMonObjInfoNode->monObjInfo.prcName == NULL)
					{
						int prcNameLen = strlen(prcName) + 1;
						pCurMonObjInfoNode->monObjInfo.prcName = (char *)malloc(prcNameLen);
						memset(pCurMonObjInfoNode->monObjInfo.prcName, 0, prcNameLen);
						strcpy(pCurMonObjInfoNode->monObjInfo.prcName, prcName);
					}

					if(pCurMonObjInfoNode->monObjInfo.prcID == 0)
					{
						pCurMonObjInfoNode->monObjInfo.prcID= prcID;
					}

					pCurMonObjInfoNode->monObjInfo.isHasPID = isHasPID;
//Wei.Gang add end
					nRet = cJSON_GetMonObjItemThr(subItem, &(pCurMonObjInfoNode->monObjInfo));
					if(!nRet)
					{
						if(pCurMonObjInfoNode && isNew)
						{
							if(pCurMonObjInfoNode->monObjInfo.path) 
								free(pCurMonObjInfoNode->monObjInfo.path);
							if(pCurMonObjInfoNode->monObjInfo.prcName)
								free(pCurMonObjInfoNode->monObjInfo.prcName);
							free(pCurMonObjInfoNode);
							pCurMonObjInfoNode = NULL;
						}
						continue;
					}
					if(isNew){
						pCurMonObjInfoNode->monObjInfo.prcResponse = 1;
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
		if(!monVal) 
		{
			cJSON_Delete(root);
			return false;
		}
		memcpy(pMonObjList, monVal, sizeof(mon_obj_info_list));
		if(!monObjList) 
		{
			cJSON_Delete(root);
			return false;
		}
		iRet = cJSON_GetSWMThrInfo(body, monObjList);
	}
	cJSON_Delete(root);
	return iRet;
}


////////////////////
static cJSON * cJSON_CreatePrcMonInfo(prc_mon_info_t * pPrcMonInfo)
{
	cJSON * pPrcMonInfoItem = NULL;
	cJSON *pPMInfoItemArray = NULL;
	cJSON *pPMInfoItemArraySub = NULL;
	char pidPreName[128];
	if(!pPrcMonInfo) return NULL;

	pPrcMonInfoItem = cJSON_CreateObject();
	cJSON_AddItemToObject(pPrcMonInfoItem,SWM_E_FLAG,pPMInfoItemArray = cJSON_CreateArray());
	sprintf_s(pidPreName, sizeof(pidPreName), "%d-%s", pPrcMonInfo->prcID,pPrcMonInfo->prcName);
	cJSON_AddItemToObject(pPrcMonInfoItem,SWM_BN_FLAG,cJSON_CreateString(pidPreName));
	//cJSON_AddItemToObject(pPrcMonInfoItem,SWM_BN_FLAG,cJSON_CreateString(pPrcMonInfo->prcName));
	//cJSON_AddItemToObject(pPrcMonInfoItem,SWM_BN_FLAG,cJSON_CreateNumber(pPrcMonInfo->prcID));
	cJSON_AddItemToObject(pPrcMonInfoItem,SWM_BASM_FLAG,cJSON_CreateString("R"));
	cJSON_AddItemToObject(pPrcMonInfoItem,SWM_BTYPE_FLAG,cJSON_CreateString("d"));
	cJSON_AddItemToObject(pPrcMonInfoItem,SWM_BST_FLAG,cJSON_CreateString(""));
	cJSON_AddItemToObject(pPrcMonInfoItem,SWM_BEXTEND_FLAG,cJSON_CreateString(""));
	//add sub-item to process information array
	//add PROC NAME
	cJSON_AddItemToArray(pPMInfoItemArray,pPMInfoItemArraySub=cJSON_CreateObject());
	cJSON_AddItemToObject(pPMInfoItemArraySub,SWM_N_FLAG,cJSON_CreateString(SWM_PRC_NAME));
	cJSON_AddItemToObject(pPMInfoItemArraySub,SWM_SV_FLAG,cJSON_CreateString(pPrcMonInfo->prcName));
	//add PID
	cJSON_AddItemToArray(pPMInfoItemArray,pPMInfoItemArraySub=cJSON_CreateObject());
	cJSON_AddItemToObject(pPMInfoItemArraySub,SWM_N_FLAG,cJSON_CreateString(SWM_PID));
	cJSON_AddItemToObject(pPMInfoItemArraySub,SWM_V_FLAG,cJSON_CreateNumber(pPrcMonInfo->prcID));
	//add OWNER NAME
	cJSON_AddItemToArray(pPMInfoItemArray,pPMInfoItemArraySub=cJSON_CreateObject());
	cJSON_AddItemToObject(pPMInfoItemArraySub,SWM_N_FLAG,cJSON_CreateString(SWM_OWNER));
	if(pPrcMonInfo->ownerName)
		cJSON_AddItemToObject(pPMInfoItemArraySub,SWM_SV_FLAG,cJSON_CreateString(pPrcMonInfo->ownerName));
	else
		cJSON_AddItemToObject(pPMInfoItemArraySub,SWM_SV_FLAG,cJSON_CreateString(""));
	//add CPU USAGE
	pPMInfoItemArraySub = NULL;
	cJSON_AddItemToArray(pPMInfoItemArray,pPMInfoItemArraySub=cJSON_CreateObject());
	cJSON_AddItemToObject(pPMInfoItemArraySub,SWM_N_FLAG,cJSON_CreateString(SWM_PRC_CPU_USAGE));
	cJSON_AddItemToObject(pPMInfoItemArraySub,SWM_V_FLAG,cJSON_CreateNumber(pPrcMonInfo->cpuUsage));
	//Add MEM USAGE
	pPMInfoItemArraySub = NULL;
	cJSON_AddItemToArray(pPMInfoItemArray,pPMInfoItemArraySub=cJSON_CreateObject());
	cJSON_AddItemToObject(pPMInfoItemArraySub,SWM_N_FLAG,cJSON_CreateString(SWM_PRC_MEM_USAGE));
	cJSON_AddItemToObject(pPMInfoItemArraySub,SWM_V_FLAG,cJSON_CreateNumber(pPrcMonInfo->memUsage));
	// Add IsActive
	pPMInfoItemArraySub = NULL;
	cJSON_AddItemToArray(pPMInfoItemArray,pPMInfoItemArraySub=cJSON_CreateObject());
	cJSON_AddItemToObject(pPMInfoItemArraySub,SWM_N_FLAG,cJSON_CreateString(SWM_PRC_IS_ACTIVE));
	cJSON_AddItemToObject(pPMInfoItemArraySub,SWM_BV_FLAG,cJSON_CreateBool(pPrcMonInfo->isActive));

	return pPrcMonInfoItem;
}

static cJSON * cJSON_CreatePrcMonInfoList(cJSON *pPrcMonInfoListItem, prc_mon_info_node_t * prcMonInfoList, char ** ppprcName, int prcCnt)
{
	if(!prcMonInfoList || !pPrcMonInfoListItem) return NULL;
	{
		prc_mon_info_node_t * head = prcMonInfoList;
		prc_mon_info_node_t * curNode = head->next;
		if(ppprcName == NULL)
		{
			while(curNode)
			{
				cJSON * pPrcMonInfoItem = NULL;
				//cJSON_AddItemToObject(pPrcMonInfoListItem,curNode->prcMonInfo.prcName,pPrcMonInfoItem = cJSON_CreatePrcMonInfo(&curNode->prcMonInfo));//cJSON_CreateObject());
				cJSON_AddItemToArray(pPrcMonInfoListItem,pPrcMonInfoItem = cJSON_CreatePrcMonInfo(&curNode->prcMonInfo));
				curNode = curNode->next;
			}
		}
		else if(prcCnt >0)
		{
			while(curNode)
			{
				int i = 0;
				char pidPreName[128] = {0};
				sprintf_s(pidPreName, sizeof(pidPreName), "%d-%s", curNode->prcMonInfo.prcID,curNode->prcMonInfo.prcName);
				for (i = 0; i< prcCnt; i++)
				{   
					if(!strcmp(ppprcName[i], pidPreName))
					{
						cJSON * pPrcMonInfoItem = NULL;
						cJSON_AddItemToArray(pPrcMonInfoListItem,pPrcMonInfoItem = cJSON_CreatePrcMonInfo(&curNode->prcMonInfo));
						break;
					}
				}
				curNode = curNode->next;
			}
		}
	}
	return pPrcMonInfoListItem;
}

static cJSON * cJSON_CreatePrcMonNotLogon(cJSON *pPrcMonInfoListItem)
{
	if(!pPrcMonInfoListItem) return NULL;
	{
		cJSON * pPrcMonInfoItem = NULL;
		cJSON * pMonInfoItem = NULL;
		cJSON * pInfoItem = NULL;
		cJSON * pInfoArrayItem = NULL;
		
		pPrcMonInfoItem = cJSON_CreateObject();
		pMonInfoItem = cJSON_CreateObject();
		pInfoItem = cJSON_CreateObject();
		pInfoArrayItem = cJSON_CreateArray();
		cJSON_AddItemToObject(pInfoItem,SWM_N_FLAG,cJSON_CreateString(SWM_PROC_NOTIFY_MSG));
		cJSON_AddItemToObject(pInfoItem,SWM_SV_FLAG,cJSON_CreateString(SWM_PROC_USER_NOT_LOGON));
		
		cJSON_AddItemToArray(pInfoArrayItem,pInfoItem);
		cJSON_AddItemToObject(pMonInfoItem,SWM_E_FLAG,pInfoArrayItem);
		cJSON_AddItemToObject(pPrcMonInfoItem,SWM_PROC_NOTIFY_MSG,pMonInfoItem);
		cJSON_AddItemToObject(pPrcMonInfoItem,SWM_BN_FLAG,cJSON_CreateString(SWM_PROC_NOTIFY_MSG));
		cJSON_AddItemToObject(pPrcMonInfoItem,SWM_BASM_FLAG,cJSON_CreateString("R"));
		cJSON_AddItemToObject(pPrcMonInfoItem,SWM_BTYPE_FLAG,cJSON_CreateString("d"));
		cJSON_AddItemToObject(pPrcMonInfoItem,SWM_BST_FLAG,cJSON_CreateString(""));
		cJSON_AddItemToObject(pPrcMonInfoItem,SWM_BEXTEND_FLAG,cJSON_CreateString(""));
		cJSON_AddItemToArray(pPrcMonInfoListItem,pPrcMonInfoItem);
	}
	/*{
		cJSON * pPrcMonInfoItem = NULL;
		pPrcMonInfoItem = cJSON_CreateObject();
		cJSON_AddItemToObject(pPrcMonInfoItem,SWM_N_FLAG,cJSON_CreateString(SWM_PROC_NOTIFY_MSG));
		cJSON_AddItemToObject(pPrcMonInfoItem,SWM_SV_FLAG,cJSON_CreateString(SWM_PROC_USER_NOT_LOGON));
		cJSON_AddItemToArray(pPrcMonInfoListItem,pPrcMonInfoItem);
	}*/
}

/*
//Useless
int Parser_swm_get_pmi_list_rep(char *pCommData, char **outputStr)
{
	prc_mon_info_node_t * pPrcMonInfoHead = NULL;
	prc_mon_info_node_t ** ppPrcMonInfoHead = &pPrcMonInfoHead;

	char * out = NULL;
	int outLen = 0;
	int tick = 0;
	cJSON *pSUSICommDataItem = NULL;
	cJSON *pRoot = NULL;
	cJSON *pPMInfo = NULL;
	if(pCommData == NULL || outputStr == NULL) return outLen;
	pSUSICommDataItem = cJSON_CreateObject();

	memcpy(ppPrcMonInfoHead, pCommData, sizeof(prc_mon_info_node_t *));
    cJSON_AddItemToObject(pSUSICommDataItem, SWM_PRC_MON_INFO_LIST, pRoot = cJSON_CreateObject());
	cJSON_AddItemToObject(pRoot,SWM_BN_FLAG,cJSON_CreateString(SWM_PRC_MON_INFO_LIST));
	tick = time((time_t *) NULL);
	cJSON_AddItemToObject(pRoot,"bt",cJSON_CreateNumber(tick));
	cJSON_AddItemToObject(pRoot,SWM_PRC_MON_INFO,pPMInfo = cJSON_CreateObject());
	//cJSON_AddItemToObject(pSUSICommDataItem,SWM_PRC_MON_INFO,pPMInfo = cJSON_CreateObject());
	cJSON_AddItemToObject(pPMInfo,SWM_BN_FLAG,cJSON_CreateString(SWM_PRC_MON_INFO));
	cJSON_CreatePrcMonInfoList(pPMInfo,pPrcMonInfoHead);

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
*/

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

int Parser_swm_mon_prc_action_rep(char *pCommData, char **outputStr)
{
	swm_thr_rep_info_t *pThrRepInfo = (swm_thr_rep_info_t *)pCommData;

	char * out = NULL;
	int outLen = 0;
	cJSON *pSUSICommDataItem = NULL;
	if(pCommData == NULL || outputStr == NULL) return outLen;
	pSUSICommDataItem = cJSON_CreateObject();

	if(pThrRepInfo)
	{
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
	cJSON * pSMInfoItemArray = NULL;
	cJSON * pSMInfoItemArraySub = NULL;
	if(!pSysMonInfo) return NULL;
	pSysMonInfoItem = cJSON_CreateObject();
	cJSON_AddItemToObject(pSysMonInfoItem,SWM_E_FLAG,pSMInfoItemArray = cJSON_CreateArray());
	cJSON_AddItemToObject(pSysMonInfoItem,SWM_BN_FLAG,cJSON_CreateString(SWM_SYS_MON_INFO));
	cJSON_AddItemToObject(pSysMonInfoItem,SWM_BASM_FLAG,cJSON_CreateString("R"));
	cJSON_AddItemToObject(pSysMonInfoItem,SWM_BTYPE_FLAG,cJSON_CreateString("d"));
	cJSON_AddItemToObject(pSysMonInfoItem,SWM_BST_FLAG,cJSON_CreateString(""));
	cJSON_AddItemToObject(pSysMonInfoItem,SWM_BEXTEND_FLAG,cJSON_CreateString(""));
	//add sub array
	//CPU Usage
	pSMInfoItemArraySub = NULL;
	cJSON_AddItemToArray(pSMInfoItemArray,pSMInfoItemArraySub=cJSON_CreateObject());
	cJSON_AddItemToObject(pSMInfoItemArraySub,SWM_N_FLAG,cJSON_CreateString(SWM_PRC_CPU_USAGE));
	cJSON_AddItemToObject(pSMInfoItemArraySub,SWM_V_FLAG,cJSON_CreateNumber(pSysMonInfo->cpuUsage));
	//TotalPhysMemoryKB
	pSMInfoItemArraySub = NULL;
	cJSON_AddItemToArray(pSMInfoItemArray,pSMInfoItemArraySub=cJSON_CreateObject());
	cJSON_AddItemToObject(pSMInfoItemArraySub,SWM_N_FLAG,cJSON_CreateString(SWM_SYS_TOTAL_PHYS_MEM));
	cJSON_AddItemToObject(pSMInfoItemArraySub,SWM_V_FLAG,cJSON_CreateNumber(pSysMonInfo->totalPhysMemoryKB));
	//AvailPhysMemoryKB
	pSMInfoItemArraySub = NULL;
	cJSON_AddItemToArray(pSMInfoItemArray,pSMInfoItemArraySub=cJSON_CreateObject());
	cJSON_AddItemToObject(pSMInfoItemArraySub,SWM_N_FLAG,cJSON_CreateString(SWM_SYS_AVAIL_PHYS_MEM));
	cJSON_AddItemToObject(pSMInfoItemArraySub,SWM_V_FLAG,cJSON_CreateNumber(pSysMonInfo->availPhysMemoryKB));

	return pSysMonInfoItem;
}
/*
//useless
int Parser_swm_get_smi_rep(char *pCommData, char **outputStr)
{
	sys_mon_info_t * pSysMonInfo = (sys_mon_info_t *)pCommData;

	char * out = NULL;
	int outLen = 0;
	int tick = 0;
	cJSON *pSUSICommDataItem = NULL;
	cJSON *pRoot = NULL;

	if(pCommData == NULL || outputStr == NULL) return outLen;
	pSUSICommDataItem = cJSON_CreateObject();

    cJSON_AddItemToObject(pSUSICommDataItem, SWM_PRC_MON_INFO_LIST, pRoot = cJSON_CreateObject());
	cJSON_AddItemToObject(pRoot,SWM_BN_FLAG,cJSON_CreateString(SWM_PRC_MON_INFO_LIST));
	tick = time((time_t *) NULL);
	cJSON_AddItemToObject(pRoot,"bt",cJSON_CreateNumber(tick));
	cJSON_AddItemToObject(pRoot,SWM_SYS_MON_INFO,cJSON_CreateSysMonInfo(pSysMonInfo));
	//cJSON_AddItemToObject(pSUSICommDataItem,SWM_SYS_MON_INFO,cJSON_CreateSysMonInfo(pSysMonInfo));

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
*/
//
int Parser_get_spec_info_rep(char *pSysCommData ,char *pProcCommData, int gatherLevel, bool isUserLogon, char **outputStr)
{
	// proc info
	prc_mon_info_node_t * pPrcMonInfoHead = NULL;
	prc_mon_info_node_t ** ppPrcMonInfoHead = &pPrcMonInfoHead;
	// system info
	sys_mon_info_t * pSysMonInfo = (sys_mon_info_t *)pSysCommData;

	char * out = NULL;
	int outLen = 0;
	//int tick = 0;
	cJSON *pSUSICommDataItem = NULL;
	cJSON *pRoot = NULL;
	cJSON *pPMInfo = NULL;

	if(pSysCommData == NULL || pProcCommData == NULL || outputStr == NULL) return outLen;
	pSUSICommDataItem = cJSON_CreateObject();
	//system part
    cJSON_AddItemToObject(pSUSICommDataItem, SWM_PRC_MON_INFO_LIST, pRoot = cJSON_CreateObject());
	cJSON_AddItemToObject(pRoot,SWM_SYS_MON_INFO,cJSON_CreateSysMonInfo(pSysMonInfo));

	//proc part
	memcpy(ppPrcMonInfoHead, pProcCommData, sizeof(prc_mon_info_node_t *));
	cJSON_AddItemToObject(pRoot,SWM_PRC_MON_INFO,pPMInfo = cJSON_CreateArray());
	if(isUserLogon)
		cJSON_CreatePrcMonInfoList(pPMInfo,pPrcMonInfoHead,NULL,0);
	else if(gatherLevel > CFG_FLAG_SEND_PRCINFO_ALL)
		cJSON_CreatePrcMonNotLogon(pPMInfo);

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

bool Parser_ParseGetSensorDataReqEx(void * data, sensor_info_list siList, char * pSessionID)
{
	bool bRet = false;
	cJSON* root = NULL;
	cJSON* body = NULL;
	cJSON* params = NULL;

	if(!data || !siList || pSessionID==NULL) return bRet;

	root = cJSON_Parse((char *)data);
	if(root)
	{
		body = cJSON_GetObjectItem(root, AGENTINFO_BODY_STRUCT);
		if(body)
		{
			params = cJSON_GetObjectItem(body, SWM_SENSOR_ID_LIST);
			if(params)
			{
				cJSON * eItem = NULL;
				eItem = cJSON_GetObjectItem(params, SWM_E_FLAG);
				if(eItem)
				{
					cJSON * subItem = NULL;
					cJSON * valItem = NULL;
					sensor_info_node_t * head = siList;
					sensor_info_node_t * curNode = head;
					sensor_info_node_t * newNode = NULL;
					int i = 0;
					int nCount = cJSON_GetArraySize(eItem);
					for(i = 0; i<nCount; i++)
					{
						subItem = cJSON_GetArrayItem(eItem, i);
						if(subItem)
						{
							valItem = cJSON_GetObjectItem(subItem, SWM_N_FLAG);
							if(valItem)
							{
								int len = strlen(valItem->valuestring)+1;
								newNode = (sensor_info_node_t *)malloc(sizeof(sensor_info_node_t));
								memset(newNode, 0, sizeof(sensor_info_node_t));
								newNode->sensorInfo.pathStr = (char *)malloc(len);
								memset(newNode->sensorInfo.pathStr, 0, len);
								strcpy(newNode->sensorInfo.pathStr, valItem->valuestring);
								curNode->next = newNode;
								curNode = newNode;
							}
						}
					}
					params = cJSON_GetObjectItem(body, SWM_SESSION_ID);
					if(params)
					{
						strcpy(pSessionID, params->valuestring);
						bRet = true;
					}
				}
			}
		}
		cJSON_Delete(root);
	}
	return bRet;
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

bool Parser_ParseAutoUploadCmd(char * cmdJsonStr, sensor_info_list siList, bool * isAutoUpdateAll,unsigned int * intervalTimeS)
{
	/*{"susiCommData":{"catalogID":4,"autoUploadIntervalSec":30,"requestID":1001,"requestItems":["HWM"],"commCmd":2053,"type":"WSN"}}*/
    bool bRet = false;
	cJSON* root = NULL;
	cJSON* body = NULL;
	cJSON* target = NULL;
	//cJSON* params = NULL;

	if(cmdJsonStr == NULL || !siList || NULL == intervalTimeS) return false;

	root = cJSON_Parse(cmdJsonStr);
	if(root)
	{
		body = cJSON_GetObjectItem(root, AGENTINFO_BODY_STRUCT);
		if(body)
		{
			target = cJSON_GetObjectItem(body, SWM_AUTOREP_INTERVAL_SEC);
			if(target)
			{
				*intervalTimeS = target->valueint;
				bRet = true;
			}
			target = cJSON_GetObjectItem(body, SWM_REQUEST_ITEMS);
			if(target)
			{
				cJSON * handlerItem = NULL;
				handlerItem = cJSON_GetObjectItem(target, SWM_AUTOUPDATE_ALL);
				if(handlerItem)
					*isAutoUpdateAll = true;
				else
				{
					handlerItem = cJSON_GetObjectItem(target, DEF_HANDLER_NAME);
					if(handlerItem)
					{
						cJSON * eItem = NULL;
						eItem = cJSON_GetObjectItem(handlerItem, SWM_E_FLAG);
						if(eItem)
						{
							cJSON * subItem = NULL;
							cJSON * valItem = NULL;
							sensor_info_node_t * head = siList;
							sensor_info_node_t * curNode = head;
							sensor_info_node_t * newNode = NULL;
							int i = 0;
							int nCount = cJSON_GetArraySize(eItem);
							for(i = 0; i<nCount; i++)
							{
								subItem = cJSON_GetArrayItem(eItem, i);
								if(subItem)
								{
									valItem = cJSON_GetObjectItem(subItem, SWM_N_FLAG);
									if(valItem)
									{
										int len = strlen(valItem->valuestring)+1;
										char *tmpPathStr = NULL;
										char * sliceStr[16] = {NULL};
										char * buf = NULL;
										int i = 0;
										tmpPathStr = (char*)malloc(len);
										memset(tmpPathStr, 0, len);
										strcpy(tmpPathStr, valItem->valuestring);
										buf = tmpPathStr;
										while(sliceStr[i] = strtok(buf, "/"))
										{
											i++;
											buf = NULL;
										}
										if(i == 1 && !strcmp(sliceStr[0], DEF_HANDLER_NAME))
										{
											*isAutoUpdateAll = true;
											free(tmpPathStr);
											break;
										}
										
										//int len = strlen(valItem->valuestring)+1;
										newNode = (sensor_info_node_t *)malloc(sizeof(sensor_info_node_t));
										memset(newNode, 0, sizeof(sensor_info_node_t));
										newNode->sensorInfo.pathStr = (char *)malloc(len);
										memset(newNode->sensorInfo.pathStr, 0, len);
										strcpy(newNode->sensorInfo.pathStr, valItem->valuestring);
										curNode->next = newNode;
										curNode = newNode;
										
										free(tmpPathStr);
									}
								}
							}
						}//eItem end
					}//handlerItem end
				}
			}
		}
		cJSON_Delete(root);
	}
	return bRet;
}

bool Parser_ParseAutoReportStopCmd(char * cmdJsonStr)
{
	bool bRet = false;
	cJSON* root = NULL;
	cJSON* body = NULL;
	//cJSON* target = NULL;

	if(cmdJsonStr == NULL) return false;

	root = cJSON_Parse(cmdJsonStr);
	if(root)
	{
		body = cJSON_GetObjectItem(root, AGENTINFO_BODY_STRUCT);
		if(body)
		{
			bRet = true;
		}
		cJSON_Delete(root);
	}
	return bRet;
}

bool Parser_GetSensorJsonStr(char *pSysCommData ,char *pProcCommData, sensor_info_t * pSensorInfo)
{
	/*******************************************************/
	// Sensor path like:
	//"e":[{"n":"ProcessMonitor/Process Monitor Info/3896-360se.exe/CPU Usage"},{"n":"ProcessMonitor/System Monitor Info/totalPhysMemKB"}]
	/****************************************************/
	// proc info
	prc_mon_info_node_t * head = NULL;
	prc_mon_info_node_t * curNode = NULL;
	//prc_mon_info_node_t * pPrcMonInfoHead = NULL;
	prc_mon_info_node_t ** ppPrcMonInfoHead = &head;
	// sys info
	sys_mon_info_t * pSysMonInfo = (sys_mon_info_t *)pSysCommData;

	bool bRet = true;
	if(NULL == pSysCommData || NULL == pProcCommData || NULL == pSensorInfo) return bRet;

	if(pSensorInfo->pathStr != NULL && strlen(pSensorInfo->pathStr))
	{
		cJSON *findVItem = NULL;
		int len = strlen(pSensorInfo->pathStr)+1;
		char *tmpPathStr = NULL;
		char * sliceStr[16] = {NULL};
		char * buf = NULL;
		int i = 0, j = 0, lastNPos = 0;
		tmpPathStr = (char*)malloc(len);
		memset(tmpPathStr, 0, len);
		strcpy(tmpPathStr, pSensorInfo->pathStr);
		buf = tmpPathStr;
		while(sliceStr[i] = strtok(buf, "/"))
		{
			i++;
			buf = NULL;
		}
		if(i>0)
		{
			findVItem =  cJSON_CreateObject();
			cJSON_AddStringToObject(findVItem, SWM_N_FLAG, pSensorInfo->pathStr);
		}
		if(i>0 && !strcmp(sliceStr[0], DEF_HANDLER_NAME))
		{
			if(i>1 && !strcmp(sliceStr[1], SWM_SYS_MON_INFO))
			{
				if(i>2 && !strcmp(sliceStr[2], SWM_PRC_CPU_USAGE))
					cJSON_AddItemToObject(findVItem,SWM_V_FLAG,cJSON_CreateNumber(pSysMonInfo->cpuUsage));
				else if(i>2 && !strcmp(sliceStr[2], SWM_SYS_TOTAL_PHYS_MEM))
					cJSON_AddItemToObject(findVItem,SWM_V_FLAG,cJSON_CreateNumber(pSysMonInfo->totalPhysMemoryKB));
				else if(i>2 && !strcmp(sliceStr[2], SWM_SYS_AVAIL_PHYS_MEM))
					cJSON_AddItemToObject(findVItem,SWM_V_FLAG,cJSON_CreateNumber(pSysMonInfo->availPhysMemoryKB));
				else
					bRet = false;
				if(bRet) //StatusCode
					cJSON_AddItemToObject(findVItem,SWM_STATUSCODE_FLAG,cJSON_CreateNumber(SWM_GETDATA_STATUS_SUCCESS));
			}
			else if(i>1 && !strcmp(sliceStr[1], SWM_PRC_MON_INFO))
			{
				if(i>2)
				{
					int j = 0;
					int procID = 0;
					char *procName[2] = {NULL};
					buf = sliceStr[2];
					while(j<2 && (procName[j] = strtok(buf, "-")))
					{
						j++;
						buf = NULL;
					}
					if(j>0)
						procID = atoi(procName[0]);
					memcpy(ppPrcMonInfoHead, pProcCommData, sizeof(prc_mon_info_node_t *));
					//head = pPrcMonInfoHead;
					curNode = head->next;
					while(curNode)
					{
						//sprintf_s(pidPreName, sizeof(pidPreName), "%d-%s", curNode->pPrcMonInfo->prcID,curNode->pPrcMonInfo->prcName);
						if(j>0 && curNode->prcMonInfo.prcID == procID && !strcmp(procName[1], curNode->prcMonInfo.prcName))
						{
							//CPU Usage
							if(i>3 && !strcmp(sliceStr[3], SWM_PRC_CPU_USAGE))
								cJSON_AddItemToObject(findVItem,SWM_V_FLAG,cJSON_CreateNumber(curNode->prcMonInfo.cpuUsage));
							//MEM Usage
							else if(i>3 && !strcmp(sliceStr[3], SWM_PRC_MEM_USAGE))
								cJSON_AddItemToObject(findVItem,SWM_V_FLAG,cJSON_CreateNumber(curNode->prcMonInfo.memUsage));
							//IsActive
							else if(i>3 && !strcmp(sliceStr[3], SWM_PRC_IS_ACTIVE))
								cJSON_AddItemToObject(findVItem,SWM_BV_FLAG,cJSON_CreateBool(curNode->prcMonInfo.isActive));
							//PID
							else if(i>3 && !strcmp(sliceStr[3], SWM_PID))
								cJSON_AddItemToObject(findVItem,SWM_V_FLAG,cJSON_CreateNumber(curNode->prcMonInfo.prcID));
							//proc name
							else if(i>3 && !strcmp(sliceStr[3], SWM_PRC_NAME))
								cJSON_AddItemToObject(findVItem,SWM_SV_FLAG,cJSON_CreateString(curNode->prcMonInfo.prcName));
							else
								bRet = false;
							if(bRet) //StatusCode
								cJSON_AddItemToObject(findVItem,SWM_STATUSCODE_FLAG,cJSON_CreateNumber(SWM_GETDATA_STATUS_SUCCESS));
							break;
						}
						curNode = curNode->next;
					}
					if(!curNode) bRet = false;
				}
				else bRet = false;
			}
			else bRet = false;
		}
		else bRet = false;

		if(!bRet)
		{
			cJSON_Delete(findVItem);
			findVItem = NULL;
		}
		
		{
		char * tmpJsonStr = NULL;
		if(findVItem == NULL)
		{
			findVItem =  cJSON_CreateObject();
			cJSON_AddStringToObject(findVItem, SWM_N_FLAG, pSensorInfo->pathStr);
			cJSON_AddStringToObject(findVItem, SWM_SV_FLAG, "not found");
			cJSON_AddItemToObject(findVItem,SWM_STATUSCODE_FLAG,cJSON_CreateNumber(SWM_GETDATA_STATUS_NOTFOUND));
		}
		tmpJsonStr = cJSON_PrintUnformatted(findVItem);
		len = strlen(tmpJsonStr) + 1;
		pSensorInfo->jsonStr = (char *)(malloc(len));
		memset(pSensorInfo->jsonStr, 0, len);
		strcpy(pSensorInfo->jsonStr, tmpJsonStr);	
		free(tmpJsonStr);
		//cJSON_Delete(targetItem);
		cJSON_Delete(findVItem);
		}
		free(tmpPathStr);
	}
	return bRet;
}

static cJSON * cJSON_CreateSensorInfoList(sensor_info_node_t * sensorInfoList)
{
	cJSON *pSensorInfoListItem = NULL, *eItem = NULL;
	if(!sensorInfoList) return NULL;
	pSensorInfoListItem = cJSON_CreateObject();
	eItem = cJSON_CreateArray();
	cJSON_AddItemToObject(pSensorInfoListItem, SWM_E_FLAG, eItem);
	{
		sensor_info_node_t * head = sensorInfoList;
		sensor_info_node_t * curNode = head->next;
		while(curNode)
		{
			cJSON * pSensorInfoItem = cJSON_Parse(curNode->sensorInfo.jsonStr);
			cJSON_AddItemToArray(eItem, pSensorInfoItem);
			curNode = curNode->next;
		}
	}
	return pSensorInfoListItem;
}

int Parser_PackGetSensorDataRep(sensor_info_list sensorInfoList, char * pSessionID, char** outputStr)
{
	char * out = NULL;
	int outLen = 0;
	cJSON *pSUSICommDataItem = NULL;
	if(sensorInfoList == NULL || outputStr == NULL || pSessionID == NULL) return outLen;
	pSUSICommDataItem = cJSON_CreateObject();

	cJSON_AddSensorInfoListToObject(pSUSICommDataItem, SWM_SENSOR_INFO_LIST, sensorInfoList);
	cJSON_AddStringToObject(pSUSICommDataItem, SWM_SESSION_ID, pSessionID);

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

int Parser_PackGetSensorDataError(char * errorStr, char * pSessionID, char** outputStr)
{
	char * out = NULL;
	int outLen = 0;
	cJSON *pSUSICommDataItem = NULL;
	if(errorStr == NULL || outputStr == NULL || NULL == pSessionID) return outLen;
	pSUSICommDataItem = cJSON_CreateObject();

	cJSON_AddStringToObject(pSUSICommDataItem, SWM_SESSION_ID, pSessionID);
	cJSON_AddStringToObject(pSUSICommDataItem, SWM_ERROR_REP, errorStr);

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

int Parser_get_request_items_rep(char *pSysCommData ,char *pProcCommData, int gatherLevel, bool isUserLogon,sensor_info_list sensorInfoList, char **outputStr)
{
	// proc info
	prc_mon_info_node_t * pPrcMonInfoHead = NULL;
	prc_mon_info_node_t ** ppPrcMonInfoHead = &pPrcMonInfoHead;
	// system info
	sys_mon_info_t * pSysMonInfo = (sys_mon_info_t *)pSysCommData;
	sensor_info_node_t * curNode = NULL;
	char * out = NULL;
	int outLen = 0;
	int j = 0;
	const int MAX_COUNT = 256;
	char * prcName[256];
	int prcCnt = 0;
	bool sysInfoAll = false;
	bool prcInfoAll = false;
	cJSON *pSUSICommDataItem = NULL;
	cJSON *pRoot = NULL;
	cJSON *pPMInfo = NULL;
	
	if(pSysCommData == NULL || pProcCommData == NULL || outputStr == NULL || sensorInfoList == NULL) return outLen;

	memset(prcName, 0 , sizeof(char *) * MAX_COUNT);
	pSUSICommDataItem = cJSON_CreateObject();
    cJSON_AddItemToObject(pSUSICommDataItem, SWM_PRC_MON_INFO_LIST, pRoot = cJSON_CreateObject());
    curNode = sensorInfoList->next;
	//Check SWM_SYS_MON_INFO
	while(curNode)
	{
		sensor_info_t *pSensorInfo = &(curNode->sensorInfo);
		if(pSensorInfo->pathStr != NULL && strlen(pSensorInfo->pathStr))
		{
			int len = strlen(pSensorInfo->pathStr)+1;
			char *tmpPathStr = NULL;
			char * sliceStr[16] = {NULL};
			char * buf = NULL;
			int i = 0;
			tmpPathStr = (char*)malloc(len);
			memset(tmpPathStr, 0, len);
			strcpy(tmpPathStr, pSensorInfo->pathStr);
			buf = tmpPathStr;
			while(sliceStr[i] = strtok(buf, "/"))
			{
				i++;
				buf = NULL;
			}
			if(i>0 && !strcmp(sliceStr[0], DEF_HANDLER_NAME))
			{
				if(i>1 && !strcmp(sliceStr[1], SWM_PRC_MON_INFO))
				{
					if(i>2)  //Some proc info
					{
						if(prcCnt < MAX_COUNT)
						{
							int prcNameLen = strlen(sliceStr[2])+1;
							prcName[prcCnt] = (char*)malloc(prcNameLen); //malloc buffer
							strcpy(prcName[prcCnt], sliceStr[2]);
							prcCnt++;
						}
					}
					else //All of proc info
					{
						prcInfoAll = true;
					}
				}
				else if(i>1 && !strcmp(sliceStr[1], SWM_SYS_MON_INFO))
				{
					if(!sysInfoAll)
					{
						sysInfoAll = true;
					}
				}
			}
			if(tmpPathStr) free(tmpPathStr);
		}
		curNode = curNode->next;
	}
	/*******************/
	//add for old command
	/*if(curNode == NULL)
	{
		prcInfoAll = true;
		sysInfoAll = true;
	}*/
	/**************************/
	if(sysInfoAll)
	{
		cJSON_AddItemToObject(pRoot,SWM_SYS_MON_INFO,cJSON_CreateSysMonInfo(pSysMonInfo));
	}

	if(prcInfoAll)
	{
		memcpy(ppPrcMonInfoHead, pProcCommData, sizeof(prc_mon_info_node_t *));
		cJSON_AddItemToObject(pRoot,SWM_PRC_MON_INFO,pPMInfo = cJSON_CreateArray());
		if(isUserLogon)
			cJSON_CreatePrcMonInfoList(pPMInfo, pPrcMonInfoHead, NULL,0);
		else if(gatherLevel > CFG_FLAG_SEND_PRCINFO_ALL)
			cJSON_CreatePrcMonNotLogon(pPMInfo);
	}
	else if(prcName[0] != NULL)
	{
		memcpy(ppPrcMonInfoHead, pProcCommData, sizeof(prc_mon_info_node_t *));
		cJSON_AddItemToObject(pRoot, SWM_PRC_MON_INFO, pPMInfo = cJSON_CreateArray());
		if(isUserLogon)
			cJSON_CreatePrcMonInfoList(pPMInfo, pPrcMonInfoHead, prcName, prcCnt);
		else if(gatherLevel > CFG_FLAG_SEND_PRCINFO_ALL)
			cJSON_CreatePrcMonNotLogon(pPMInfo);
	}
	
	//free buffer
	for (j = 0; j< MAX_COUNT; j++)
	{
		if(prcName[prcCnt] != NULL)
			free(prcName[prcCnt]);
		else 
			break;
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

bool Parser_ParseAutoReportReq(void * data, sensor_info_list siList, bool * isAutoUpdateAll,unsigned int * intervalMs, unsigned int * timeoutMs)
{
	/*{"susiCommData":{"catalogID":4,"autoUploadIntervalSec":30,"requestID":1001,"requestItems":["HWM"],"commCmd":2053,"type":"WSN"}}*/
    bool bRet = false;
	cJSON* root = NULL;
	cJSON* body = NULL;
	cJSON* target = NULL;
	cJSON* targetTmp = NULL;
	//cJSON* params = NULL;

	if(data == NULL || !siList || NULL == intervalMs || NULL == timeoutMs) return false;

	root = cJSON_Parse((char *)data);
	if(root)
	{
		body = cJSON_GetObjectItem(root, AGENTINFO_BODY_STRUCT);
		if(body)
		{
			target = cJSON_GetObjectItem(body, SWM_AUTOREP_INTERVAL_MS);
			targetTmp = cJSON_GetObjectItem(body, SWM_AUTOREP_TIMEOUT_MS);
			if(target && targetTmp)
			{
				*intervalMs = target->valueint;
				*timeoutMs = targetTmp->valueint; 
				bRet = true;
			}
			else
				return bRet; 

			target = cJSON_GetObjectItem(body, SWM_REQUEST_ITEMS);
			if(target)
			{
				cJSON * eItem = NULL;
				eItem = cJSON_GetObjectItem(target, SWM_E_FLAG);
				if(eItem)
				{
					cJSON * subItem = NULL;
					cJSON * valItem = NULL;
					sensor_info_node_t * head = siList;
					sensor_info_node_t * curNode = head;
					sensor_info_node_t * newNode = NULL;
					int i = 0;
					int nCount = cJSON_GetArraySize(eItem);
					for(i = 0; i<nCount; i++)
					{
						subItem = cJSON_GetArrayItem(eItem, i);
						if(subItem)
						{
							valItem = cJSON_GetObjectItem(subItem, SWM_N_FLAG);
							if(valItem)
							{
								int len = strlen(valItem->valuestring)+1;
								char *tmpPathStr = NULL;
								char * sliceStr[16] = {NULL};
								char * buf = NULL;
								int i = 0;
								tmpPathStr = (char*)malloc(len);
								memset(tmpPathStr, 0, len);
								strcpy(tmpPathStr, valItem->valuestring);
								buf = tmpPathStr;
								while(sliceStr[i] = strtok(buf, "/"))
								{
									i++;
									buf = NULL;
								}
								if(i == 1 && !strcmp(sliceStr[0], DEF_HANDLER_NAME))
								{
									*isAutoUpdateAll = true;
									free(tmpPathStr);
									break;
								}
								
								//int len = strlen(valItem->valuestring)+1;
								newNode = (sensor_info_node_t *)malloc(sizeof(sensor_info_node_t));
								memset(newNode, 0, sizeof(sensor_info_node_t));
								newNode->sensorInfo.pathStr = (char *)malloc(len);
								memset(newNode->sensorInfo.pathStr, 0, len);
								strcpy(newNode->sensorInfo.pathStr, valItem->valuestring);
								curNode->next = newNode;
								curNode = newNode;
								
								free(tmpPathStr);
							}
						}
					}
				}//eItem end
			}
		}
		cJSON_Delete(root);
	}
	return bRet;
}