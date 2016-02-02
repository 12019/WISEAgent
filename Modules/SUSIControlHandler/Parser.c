#include "Parser.h"

static cJSON * cJSON_CreateSensorInfoList(sensor_info_node_t * sensorInfoList);
static int cJSON_GetSensorInfoList(cJSON * pFatherItem, sensor_info_list sensorInfoList);
static int cJSON_GetSensorInfoListEx(cJSON * pFatherItem, sensor_info_list sensorInfoList);
static int cJSON_GetSensorInfo(cJSON * pSensorInfoItem, sensor_info_t * pSensorInfo);
static int cJSON_GetSensorInfoEx(cJSON * pSensorInfoItem, sensor_info_t * pSensorInfo);
#define cJSON_AddSensorInfoListToObject(object, name, sN) cJSON_AddItemToObject(object, name, cJSON_CreateSensorInfoList(sN))

bool ParseReceivedData(void* data, int datalen, int * cmdID)
{
	/*{"susiCommData":{"commCmd":251,"catalogID":4,"requestID":10}}*/

	cJSON* root = NULL;
	cJSON* body = NULL;
	cJSON* target = NULL;

	if(!data) return false;
	if(datalen<=0) return false;
	root = cJSON_Parse((char *)data);
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

int Parser_PackCapabilityStrRep(char *cpbStr, char ** outputStr)
{
	char * out = NULL;
	int outLen = 0;
	cJSON *pSUSICommDataItem = NULL, *cpbRoot = NULL, *cpbItem = NULL;
	if(cpbStr == NULL || outputStr == NULL) return outLen;

	pSUSICommDataItem = cJSON_CreateObject();
	cpbRoot = cJSON_Parse(cpbStr);
	if(cpbRoot)
	{	
		cJSON_AddItemToObject(pSUSICommDataItem, DEF_HANDLER_NAME, cpbRoot);
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

int Parser_PackSUSICtrlError(char * errorStr, char** outputStr)
{
	char * out = NULL;
	int outLen = 0;
	cJSON *pSUSICommDataItem = NULL;
	if(errorStr == NULL || outputStr == NULL) return outLen;
	pSUSICommDataItem = cJSON_CreateObject();

	cJSON_AddStringToObject(pSUSICommDataItem, SUSICTRL_ERROR_REP, errorStr);

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

/*
static cJSON * cJSON_CreateSensorInfoList(sensor_info_node_t * sensorInfoList)
{
	cJSON *pSensorInfoListItem = NULL;
	if(!sensorInfoList) return NULL;
	pSensorInfoListItem = cJSON_CreateArray();
	{
		sensor_info_node_t * head = sensorInfoList;
		sensor_info_node_t * curNode = head->next;
		while(curNode)
		{
			cJSON * pSensorInfoItem = cJSON_Parse(curNode->sensorInfo.jsonStr);
			cJSON_AddItemToArray(pSensorInfoListItem, pSensorInfoItem);
			curNode = curNode->next;
		}
	}
	return pSensorInfoListItem;
}*/

static cJSON * cJSON_CreateSensorInfoList(sensor_info_node_t * sensorInfoList)
{
	cJSON *pSensorInfoListItem = NULL, *eItem = NULL;
	if(!sensorInfoList) return NULL;
	pSensorInfoListItem = cJSON_CreateObject();
	eItem = cJSON_CreateArray();
	cJSON_AddItemToObject(pSensorInfoListItem, SUSICTRL_E_FLAG, eItem);
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

	cJSON_AddSensorInfoListToObject(pSUSICommDataItem, SUSICTRL_SENSOR_INFO_LIST, sensorInfoList);
	cJSON_AddStringToObject(pSUSICommDataItem, SUSICTRL_SESSION_ID, pSessionID);

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

	cJSON_AddStringToObject(pSUSICommDataItem, SUSICTRL_SESSION_ID, pSessionID);
	cJSON_AddStringToObject(pSUSICommDataItem, SUSICTRL_ERROR_REP, errorStr);

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

int Parser_PackSetSensorDataRepEx(sensor_info_list sensorInfoList, char * sessionID, char** outputStr)
{
	char * out = NULL;
	int outLen = 0;
	cJSON *pSUSICommDataItem = NULL, *sensorInfoListItem = NULL, *eItem = NULL, *subItem = NULL;
	if(sensorInfoList == NULL || outputStr == NULL || sessionID == NULL) return outLen;
	pSUSICommDataItem = cJSON_CreateObject();
   sensorInfoListItem = cJSON_CreateObject();
	cJSON_AddItemToObject(pSUSICommDataItem, SUSICTRL_SENSOR_INFO_LIST, sensorInfoListItem);
	eItem = cJSON_CreateArray();
	cJSON_AddItemToObject(sensorInfoListItem, SUSICTRL_E_FLAG, eItem);
	{
		sensor_info_node_t * curNode = sensorInfoList->next;
		char tmpRetStr[32] = {0};
		int statusCode = IOT_SGRC_FAIL;
		while(curNode)
		{
			statusCode = IOT_SGRC_FAIL;
			memset(tmpRetStr, 0, sizeof(tmpRetStr));
			subItem = cJSON_CreateObject();
			cJSON_AddStringToObject(subItem, SUSICTRL_N_FLAG, curNode->sensorInfo.pathStr);
			switch(curNode->sensorInfo.setRet)
			{
			case SSR_SUCCESS:
				{
					strcpy(tmpRetStr, IOT_SGRC_STR_SUCCESS);
					statusCode = IOT_SGRC_SUCCESS;
					break;
				}
			case SSR_FAIL:
				{
					strcpy(tmpRetStr, IOT_SGRC_STR_FAIL);
					statusCode = IOT_SGRC_FAIL;
					break;
				}
			case SSR_READ_ONLY:
				{
					strcpy(tmpRetStr, IOT_SGRC_STR_READ_ONLY);
					statusCode = IOT_SGRC_READ_ONLY;
					break;
				}
			case SSR_WRITE_ONLY:
				{
					strcpy(tmpRetStr, IOT_SGRC_STR_WRIT_ONLY);
					statusCode = IOT_SGRC_WRIT_ONLY;
					break;
				}
			case SSR_WRONG_FORMAT:
				{
					strcpy(tmpRetStr, IOT_SGRC_STR_FORMAT_ERROR);
					statusCode = IOT_SGRC_FORMAT_ERROR;
					break;
				}
			case SSR_SYS_BUSY:
				{
					strcpy(tmpRetStr, IOT_SGRC_STR_SYS_BUSY);
					statusCode = IOT_SGRC_SYS_BUSY;
					break;
				}
			case SSR_OVER_RANGE:
				{
					strcpy(tmpRetStr, IOT_SGRC_STR_OUT_RANGE);
					statusCode = IOT_SGRC_OUT_RANGE;
					break;
				}
			case SSR_NOT_FOUND:
				{
					strcpy(tmpRetStr, IOT_SGRC_STR_NOT_FOUND);
					statusCode = IOT_SGRC_NOT_FOUND;
					break;
				}
			default:
				{
					strcpy(tmpRetStr, IOT_SGRC_STR_FAIL);
					statusCode = IOT_SGRC_FAIL;
					break;
				}
			}
			cJSON_AddStringToObject(subItem, SUSICTRL_SV_FLAG, tmpRetStr);
			cJSON_AddNumberToObject(subItem, SUSICTRL_STATUS_CODE_FLAG, statusCode);
			cJSON_AddItemToArray(eItem, subItem);
			curNode = curNode->next;
		}
	}
	cJSON_AddStringToObject(pSUSICommDataItem, SUSICTRL_SESSION_ID, sessionID);

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

int Parser_PackSetSensorDataRep(char * repStr, char * sessionID, char** outputStr)
{
	char * out = NULL;
	int outLen = 0;
	cJSON *pSUSICommDataItem = NULL;
	if(repStr == NULL || outputStr == NULL || sessionID == NULL) return outLen;
	pSUSICommDataItem = cJSON_CreateObject();

	cJSON_AddStringToObject(pSUSICommDataItem, SUSICTRL_SESSION_ID, sessionID);
	cJSON_AddStringToObject(pSUSICommDataItem, SUSICTRL_SENSOR_SET_RET, repStr);

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

cJSON * Parser_PackFilterRepData(char * iotDataJsonStr, cJSON * repFilterItem)
{
	cJSON * repDataItem = NULL, *curIotDataItem = NULL;
	if(NULL != iotDataJsonStr && NULL != repFilterItem)
	{
		cJSON * eItem = cJSON_GetObjectItem(repFilterItem, SUSICTRL_E_FLAG);
		if(eItem)
		{
			cJSON * subEItem = NULL, *nItem = NULL;
			char tmpPath[256] = {0}; 
			int i = 0, cnt = 0;
			curIotDataItem = cJSON_Parse(iotDataJsonStr);
			cnt = cJSON_GetArraySize(eItem);
			for(i=0; i<cnt; i++)
			{
				memset(tmpPath, 0, sizeof(tmpPath));
				subEItem = cJSON_GetArrayItem(eItem, i);
				nItem = cJSON_GetObjectItem(subEItem, SUSICTRL_N_FLAG);
				if(nItem)
				{
					strcpy(tmpPath, nItem->valuestring);
				}
				if(strlen(tmpPath))
				{
					if(!strcmp(tmpPath, DEF_HANDLER_NAME))
					{
						if(repDataItem) cJSON_Delete(repDataItem);
						repDataItem = cJSON_Duplicate(curIotDataItem, 1);
						continue;
					}
					else
					{
						char * sliceStr[6] = {0};
						char * buf = NULL;
						int i=0, j=0, sliceCnt = 0;
						buf = tmpPath;
						while(sliceStr[i] = strtok(buf, "/"))
						{
							i++;
							buf = NULL;
						}
						sliceCnt = i;

						if(!strcmp(sliceStr[0], DEF_HANDLER_NAME) && sliceCnt >= 2)
						{
							cJSON * fatherItem = NULL, *findItem = NULL, *eItem = NULL, 
								*nItem = NULL, *eSubItem = NULL, *targetItem = NULL;
							int j = 0, m=0, isLastNItem = 0;
							fatherItem = curIotDataItem;
							for(j = 1; j<sliceCnt; j++)
							{
								findItem = cJSON_GetObjectItem(fatherItem, sliceStr[j]);
								if(findItem)
								{
									if(j == sliceCnt-1)
									{
										targetItem = cJSON_Duplicate(findItem, 1);
									}
									else
									{
										fatherItem = findItem;
									}
								}
								else
								{
									if(j == sliceCnt-1)
									{
										int eCnt = 0, k = 0;
										eItem = cJSON_GetObjectItem(fatherItem, SUSICTRL_E_FLAG);
										if(eItem)
										{
											eCnt = cJSON_GetArraySize(eItem);
											for(k=0; k<eCnt; k++)
											{
												eSubItem = cJSON_GetArrayItem(eItem, k);
												nItem = cJSON_GetObjectItem(eSubItem, SUSICTRL_N_FLAG);
												if(!strcmp(nItem->valuestring, sliceStr[j]))
												{
													isLastNItem = 1;
													targetItem = cJSON_Duplicate(eSubItem, 1);
													break;
												}
											}
										}
									}
								}
							}
							
							if(targetItem)
							{
								cJSON * tmpFatherItem = NULL, *tmpFindItem = NULL, *tmpEItem = NULL, *newItem = NULL;
								if(repDataItem == NULL)
								{
									repDataItem = cJSON_CreateObject();
								}
								tmpFatherItem = repDataItem;
								for(m=1; m<sliceCnt; m++)
								{
									tmpFindItem = cJSON_GetObjectItem(tmpFatherItem, sliceStr[m]);
									if(!tmpFindItem)
									{
										if(m != sliceCnt-1)
										{
											newItem = cJSON_CreateObject();
											cJSON_AddItemToObject(tmpFatherItem, sliceStr[m], newItem);
											tmpFatherItem = newItem;
										}
										else
										{
											if(!isLastNItem)
											{
												newItem = cJSON_Duplicate(findItem, 1);
												cJSON_AddItemToObject(tmpFatherItem, sliceStr[m], newItem);
											}
											else
											{
												tmpEItem = cJSON_GetObjectItem(tmpFatherItem, SUSICTRL_E_FLAG);
												if(!tmpEItem)
												{
													newItem = cJSON_CreateArray();
													cJSON_AddItemToObject(tmpFatherItem, SUSICTRL_E_FLAG, newItem);
												}
												else
												{
													newItem = tmpEItem;
												}
												cJSON_AddItemToArray(newItem, targetItem);
											}
										}
									}
									else
									{
										tmpFatherItem = tmpFindItem;
									}
								}
							}
						}
					}
				}
			}
			if(curIotDataItem) cJSON_Delete(curIotDataItem);
		}
	}
	return repDataItem;
}

int Parser_PackReportIotData(char * iotDataJsonStr, char * repFilter, char * handlerName,char ** outputStr)
{
	char * out = NULL;
	int outLen = 0;
	cJSON *pSUSICommDataItem = NULL, * filterItem = NULL, *subItem = NULL;
	if(iotDataJsonStr == NULL || repFilter==NULL ||  handlerName == NULL || outputStr == NULL) return outLen;
	
	filterItem = cJSON_Parse(repFilter);
   subItem = cJSON_GetObjectItem(filterItem, SUSICTRL_AUTOREP_ALL);
	if(subItem)
	{
		pSUSICommDataItem = cJSON_CreateObject();
		cJSON_AddItemToObject(pSUSICommDataItem, handlerName, cJSON_Parse(iotDataJsonStr));
	}
	else 
	{
		cJSON * repDataItem = NULL;
		subItem = cJSON_GetObjectItem(filterItem, DEF_HANDLER_NAME);
		if(subItem)
		{
			repDataItem = Parser_PackFilterRepData(iotDataJsonStr, subItem);
		}
		if(repDataItem)
		{
			pSUSICommDataItem = cJSON_CreateObject();
			cJSON_AddItemToObject(pSUSICommDataItem, handlerName, repDataItem);
		}
	}
	cJSON_Delete(filterItem);	

	if(pSUSICommDataItem != NULL)
	{
		out = cJSON_PrintUnformatted(pSUSICommDataItem);
		outLen = strlen(out) + 1;
		*outputStr = (char *)(malloc(outLen));
		memset(*outputStr, 0, outLen);
		strcpy(*outputStr, out);
		cJSON_Delete(pSUSICommDataItem);	
		printf("%s\n",out);	
		free(out);
	}
	return outLen;
}

int Parser_PackAutoUploadIotData(char * iotDataJsonStr, char * repFilter, char * handlerName,char ** outputStr)
{
	char * out = NULL;
	int outLen = 0;
	cJSON *pSUSICommDataItem = NULL, * filterItem = NULL, *subItem = NULL;
	if(iotDataJsonStr == NULL || repFilter==NULL ||  handlerName == NULL || outputStr == NULL) return outLen;

	filterItem = cJSON_Parse(repFilter);
	if(filterItem) 
	{
		cJSON * repDataItem = NULL;
		repDataItem = Parser_PackFilterRepData(iotDataJsonStr, filterItem);
		if(repDataItem)
		{
			pSUSICommDataItem = cJSON_CreateObject();
			cJSON_AddItemToObject(pSUSICommDataItem, handlerName, repDataItem);
		}
		cJSON_Delete(filterItem);
	}
		
	if(pSUSICommDataItem != NULL)
	{
		out = cJSON_PrintUnformatted(pSUSICommDataItem);
		outLen = strlen(out) + 1;
		*outputStr = (char *)(malloc(outLen));
		memset(*outputStr, 0, outLen);
		strcpy(*outputStr, out);
		cJSON_Delete(pSUSICommDataItem);	
		printf("%s\n",out);	
		free(out);
	}
	return outLen;
}

cJSON * Parser_PackThrItemInfo(susictrl_thr_item_info_t * pThrItemInfo)
{
	cJSON * infoItem = NULL;
	if(pThrItemInfo == NULL) return infoItem;
	{
		infoItem = cJSON_CreateObject();
		cJSON_AddNumberToObject(infoItem, SUSICTRL_THR_ID, pThrItemInfo->id);
		cJSON_AddStringToObject(infoItem, SUSICTRL_N_FLAG, pThrItemInfo->name);
		if(pThrItemInfo->desc) cJSON_AddStringToObject(infoItem, SUSICTRL_THR_DESC, pThrItemInfo->desc);
		cJSON_AddNumberToObject(infoItem, SUSICTRL_THR_MAX, pThrItemInfo->maxThr);
		cJSON_AddNumberToObject(infoItem, SUSICTRL_THR_MIN, pThrItemInfo->minThr);
		cJSON_AddNumberToObject(infoItem, SUSICTRL_THR_TYPE, pThrItemInfo->thrType);
		cJSON_AddNumberToObject(infoItem, SUSICTRL_THR_LTIME, pThrItemInfo->lastingTimeS);
		cJSON_AddNumberToObject(infoItem, SUSICTRL_THR_ITIME, pThrItemInfo->intervalTimeS);
		if(pThrItemInfo->isEnable)
		{
			cJSON_AddStringToObject(infoItem, SUSICTRL_THR_ENABLE, "true");
		}
		else
		{
			cJSON_AddStringToObject(infoItem, SUSICTRL_THR_ENABLE, "false");
		}
	}
	return infoItem;
}

cJSON * Parser_PackThrItemList(susictrl_thr_item_list thrItemList)
{
	cJSON * listItem = NULL;
	if(thrItemList == NULL) return listItem;
	{
		susictrl_thr_item_node_t * curNode = thrItemList->next;
		listItem = cJSON_CreateArray();
		while(curNode)
		{
			cJSON * infoItem = Parser_PackThrItemInfo(&curNode->thrItemInfo);
			if(infoItem)
			{
				cJSON_AddItemToArray(listItem, infoItem);
			}
			curNode = curNode->next;
		}
	}
	return listItem;
}

int Parser_PackThrInfo(susictrl_thr_item_list thrList, char ** outputStr)
{
	char * out = NULL;
	int outLen = 0;
	cJSON *root = NULL, *pSUSICommDataItem = NULL, * thrListItem = NULL;
	if(thrList == NULL || outputStr == NULL) return outLen;
	root = cJSON_CreateObject();
	pSUSICommDataItem = cJSON_CreateObject();
	cJSON_AddItemToObject(root, SUSICTRL_JSON_ROOT_NAME, pSUSICommDataItem);
	thrListItem = Parser_PackThrItemList(thrList);
	if(thrListItem)
	{
		cJSON_AddItemToObject(pSUSICommDataItem, SUSICTRL_THR, thrListItem);
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

int Parser_PackSetThrRep(char * repStr, char ** outputStr)
{
	char * out = NULL;
	int outLen = 0;
	cJSON *pSUSICommDataItem = NULL;
	if(repStr == NULL || outputStr == NULL) return outLen;
	pSUSICommDataItem = cJSON_CreateObject();

	cJSON_AddStringToObject(pSUSICommDataItem, SUSICTRL_SET_THR_REP, repStr);

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

int Parser_PackDelAllThrRep(char * repStr, char ** outputStr)
{
	char * out = NULL;
	int outLen = 0;
	cJSON *pSUSICommDataItem = NULL;
	if(repStr == NULL || outputStr == NULL) return outLen;
	pSUSICommDataItem = cJSON_CreateObject();

	cJSON_AddStringToObject(pSUSICommDataItem, SUSICTRL_DEL_ALL_THR_REP, repStr);

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

int Parser_PackThrCheckRep(susictrl_thr_rep_t * pThrCheckRep, char ** outputStr)
{
	char * out = NULL;
	int outLen = 0;
	cJSON *pSUSICommDataItem = NULL;
	if(pThrCheckRep == NULL || outputStr == NULL) return outLen;
	pSUSICommDataItem = cJSON_CreateObject();

	if(pThrCheckRep->isTotalNormal)
	{
		cJSON_AddStringToObject(pSUSICommDataItem, SUSICTRL_THR_CHECK_STATUS, "True");
	}
	else
	{
		cJSON_AddStringToObject(pSUSICommDataItem, SUSICTRL_THR_CHECK_STATUS, "False");
	}
	if(pThrCheckRep->repInfo)
	{
		cJSON_AddStringToObject(pSUSICommDataItem, SUSICTRL_THR_CHECK_MSG, pThrCheckRep->repInfo);
	}
	else
	{
		cJSON_AddStringToObject(pSUSICommDataItem, SUSICTRL_THR_CHECK_MSG, "");
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
			params = cJSON_GetObjectItem(body, SUSICTRL_SENSOR_ID_LIST);
			if(params)
			{
				cJSON * eItem = NULL;
				eItem = cJSON_GetObjectItem(params, SUSICTRL_E_FLAG);
				if(eItem)
				{
					cJSON * subItem = NULL;
					cJSON * valItem = NULL;
					sensor_info_node_t * head = siList;
					sensor_info_node_t * newNode = NULL;
					int i = 0;
					int nCount = cJSON_GetArraySize(eItem);
					for(i = 0; i<nCount; i++)
					{
						subItem = cJSON_GetArrayItem(eItem, i);
						if(subItem)
						{
							valItem = cJSON_GetObjectItem(subItem, SUSICTRL_N_FLAG);
							if(valItem)
							{
								int len = strlen(valItem->valuestring)+1;
								newNode = (sensor_info_node_t *)malloc(sizeof(sensor_info_node_t));
								memset(newNode, 0, sizeof(sensor_info_node_t));
								newNode->sensorInfo.pathStr = (char *)malloc(len);
								memset(newNode->sensorInfo.pathStr, 0, len);
								strcpy(newNode->sensorInfo.pathStr, valItem->valuestring);
								newNode->next = head->next;
								head->next = newNode;
							}
						}
					}
					params = cJSON_GetObjectItem(body, SUSICTRL_SESSION_ID);
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

bool Parser_ParseGetSensorDataReq(void* data, char*outputStr, char * pSessionID)
{
	bool bRet = false;
	cJSON* root = NULL;
	cJSON* body = NULL;
	cJSON* params = NULL;

	if(!data || !outputStr || pSessionID==NULL) return bRet;

	root = cJSON_Parse((char *)data);
	if(!root) return bRet;

	body = cJSON_GetObjectItem(root, AGENTINFO_BODY_STRUCT);
	if(!body)
	{
		cJSON_Delete(root);
		return bRet;
	}

	params = cJSON_GetObjectItem(body, SUSICTRL_SENSORS_ID);
	if(params)
	{
		if(outputStr)
		{
			strcpy(outputStr, params->valuestring);
			params = cJSON_GetObjectItem(body, SUSICTRL_SESSION_ID);
			if(params)
			{
				strcpy(pSessionID, params->valuestring);
				bRet = true;
			}
		}
	}
	cJSON_Delete(root);
	return bRet;
}

static int cJSON_GetSensorInfoList(cJSON * pFatherItem, sensor_info_list sensorInfoList)
{
	int iRet = 0, iFlag = 0;
	if(pFatherItem == NULL || sensorInfoList == NULL) return iRet;
	{
		cJSON *pParamArrayItem = NULL;
		pParamArrayItem = cJSON_GetObjectItem(pFatherItem, SUSICTRL_SENSOR_SET_PARAMS);
		if(pParamArrayItem)
		{
			int i = 0;
			int nCount = cJSON_GetArraySize(pParamArrayItem);
			for(i = 0; i < nCount; i++)
			{
				cJSON *pParamItem = cJSON_GetArrayItem(pParamArrayItem, i);
				if(pParamItem)
				{
					char * jsonStr = NULL;
					int iRet = 0;
					sensor_info_node_t * pSensorInfoNode = NULL;
					pSensorInfoNode = (sensor_info_node_t *)malloc(sizeof(sensor_info_node_t));
					memset(pSensorInfoNode, 0, sizeof(sensor_info_node_t));
					jsonStr = cJSON_PrintUnformatted(pParamItem);
					if(jsonStr)
					{
						int len = strlen(jsonStr) +1;
						pSensorInfoNode->sensorInfo.jsonStr = (char *)malloc(len);
						memset(pSensorInfoNode->sensorInfo.jsonStr, 0, len);
						strcpy(pSensorInfoNode->sensorInfo.jsonStr, jsonStr);
						free(jsonStr);
						iRet = cJSON_GetSensorInfo(pParamItem, &(pSensorInfoNode->sensorInfo));
						if(!iRet)
						{
							if(pSensorInfoNode)
							{
								free(pSensorInfoNode);
								pSensorInfoNode = NULL;
							}
							continue;
						}
					}
					pSensorInfoNode->next = sensorInfoList->next;
					sensorInfoList->next = pSensorInfoNode;
				}
			}
			iRet = 1;
		}
	}
	return iRet;
}

static int cJSON_GetSensorInfoListEx(cJSON * pFatherItem, sensor_info_list sensorInfoList)
{
	int iRet = 0, iFlag = 0;
	if(pFatherItem == NULL || sensorInfoList == NULL) return iRet;
	{
		cJSON * sensorIDListItem = NULL;
		sensorIDListItem = cJSON_GetObjectItem(pFatherItem, SUSICTRL_SENSOR_ID_LIST);
		if(sensorIDListItem)
		{
			cJSON * eItem = NULL;
			eItem = cJSON_GetObjectItem(sensorIDListItem, SUSICTRL_E_FLAG);
			if(eItem)
			{
				int i = 0;
				int nCount = cJSON_GetArraySize(eItem);
				for(i = 0; i < nCount; i++)
				{
					cJSON *pParamItem = cJSON_GetArrayItem(eItem, i);
					if(pParamItem)
					{
						char * jsonStr = NULL;
						int iRet = 0;
						sensor_info_node_t * pSensorInfoNode = NULL;
						pSensorInfoNode = (sensor_info_node_t *)malloc(sizeof(sensor_info_node_t));
						memset(pSensorInfoNode, 0, sizeof(sensor_info_node_t));
						jsonStr = cJSON_PrintUnformatted(pParamItem);
						if(jsonStr)
						{
							int len = strlen(jsonStr) +1;
							pSensorInfoNode->sensorInfo.jsonStr = (char *)malloc(len);
							memset(pSensorInfoNode->sensorInfo.jsonStr, 0, len);
							strcpy(pSensorInfoNode->sensorInfo.jsonStr, jsonStr);
							free(jsonStr);
							iRet = cJSON_GetSensorInfoEx(pParamItem, &(pSensorInfoNode->sensorInfo));
							if(!iRet)
							{
								if(pSensorInfoNode)
								{
									free(pSensorInfoNode);
									pSensorInfoNode = NULL;
								}
								continue;
							}
						}
						pSensorInfoNode->next = sensorInfoList->next;
						sensorInfoList->next = pSensorInfoNode;
					}
				}
				iRet = 1;
			}
		}
	}
	return iRet;
}

static int cJSON_GetSensorInfo(cJSON * pSensorInfoItem, sensor_info_t * pSensorInfo)
{
	int iRet = 0;
	if(!pSensorInfoItem|| !pSensorInfo) return iRet;
	{
		cJSON * pSubItem = NULL;
		pSubItem = cJSON_GetObjectItem(pSensorInfoItem, SUSICTRL_SENSOR_ID);
		if(pSubItem)
		{
			pSensorInfo->id = pSubItem->valueint;
			iRet = 1;
		}
	}
	return iRet;
}

static int cJSON_GetSensorInfoEx(cJSON * pSensorInfoItem, sensor_info_t * pSensorInfo)
{
	int iRet = 0;
	if(!pSensorInfoItem|| !pSensorInfo) return iRet;
	{
		cJSON * pSubItem = NULL;
		pSubItem = cJSON_GetObjectItem(pSensorInfoItem, SUSICTRL_N_FLAG);
		if(pSubItem)
		{
			int len = strlen(pSubItem->valuestring) +1;
			pSensorInfo->pathStr = (char *)malloc(len);
			memset(pSensorInfo->pathStr, 0, len);
			strcpy(pSensorInfo->pathStr, pSubItem->valuestring);
			iRet = 1;
		}
	}
	return iRet;
}

bool Parser_ParseSetSensorDataReq(void* data, sensor_info_list sensorInfoList, char * sessionID)
{
	bool bRet = false;
	cJSON* root = NULL;
	cJSON* body = NULL;
	cJSON* params = NULL;
	if(!data || sensorInfoList ==NULL || sessionID == NULL) return bRet;

	root = cJSON_Parse((char *)data);
	if(root)
	{
		body = cJSON_GetObjectItem(root, AGENTINFO_BODY_STRUCT);
		if(body)
		{
			int iRet = 0;
			iRet = cJSON_GetSensorInfoList(body, sensorInfoList);
			if(iRet == 1)
			{
				params = cJSON_GetObjectItem(body, SUSICTRL_SESSION_ID);
				if(params)
				{
					strcpy(sessionID, params->valuestring);
					bRet = true;
				}
			}
		}
		cJSON_Delete(root);
	}
	return bRet;
}

bool Parser_ParseSetSensorDataReqEx(void* data, sensor_info_list sensorInfoList, char * sessionID)
{
	bool bRet = false;
	cJSON* root = NULL;
	cJSON* body = NULL;
	cJSON* params = NULL;
	if(!data || sensorInfoList ==NULL || sessionID == NULL) return bRet;

	root = cJSON_Parse((char *)data);
	if(root)
	{
		body = cJSON_GetObjectItem(root, AGENTINFO_BODY_STRUCT);
		if(body)
		{
			int iRet = 0;
			iRet = cJSON_GetSensorInfoListEx(body, sensorInfoList);
			if(iRet == 1)
			{
				params = cJSON_GetObjectItem(body, SUSICTRL_SESSION_ID);
				if(params)
				{
					strcpy(sessionID, params->valuestring);
					bRet = true;
				}
			}
		}
		cJSON_Delete(root);
	}
	return bRet;
}

bool Parser_ParseAutoReportCmd(char * cmdJsonStr, unsigned int * intervalTimeS, char * repFilter)
{
	/*{"susiCommData":{"catalogID":4,"autoUploadIntervalSec":30,"requestID":1001,"requestItems":["HWM"],"commCmd":2053,"type":"WSN"}}*/
   bool bRet = false;
	cJSON* root = NULL;
	cJSON* body = NULL;
	cJSON* target = NULL;

	if(cmdJsonStr == NULL || NULL == intervalTimeS || repFilter == NULL) return false;

	root = cJSON_Parse(cmdJsonStr);
	if(root)
	{
		body = cJSON_GetObjectItem(root, AGENTINFO_BODY_STRUCT);
		if(body)
		{
			target = cJSON_GetObjectItem(body, SUSICTRL_AUTOREP_INTERVAL_SEC);
			if(target)
			{
				*intervalTimeS = target->valueint;
				target = cJSON_GetObjectItem(body, SUSICTRL_AUTOREP_REQ_ITEMS);
				if(target)
				{
					char * tmpJsonStr = NULL;
					int len = 0;
					tmpJsonStr = cJSON_PrintUnformatted(target);
					len = strlen(tmpJsonStr) + 1;
					strcpy(repFilter, tmpJsonStr);	
					free(tmpJsonStr);
					bRet = true;
				}
			}
		}
		cJSON_Delete(root);
	}
	return bRet;
}

bool Parser_ParseAutoUploadCmd(char * cmdJsonStr, unsigned int * intervalTimeMs, unsigned int * continueTimeMs, char * repFilter)
{
	bool bRet = false;
	cJSON* root = NULL;
	cJSON* body = NULL;
	cJSON* target = NULL;

	if(cmdJsonStr == NULL || NULL == intervalTimeMs || continueTimeMs == NULL || repFilter == NULL) return false;

	root = cJSON_Parse(cmdJsonStr);
	if(root)
	{
		body = cJSON_GetObjectItem(root, AGENTINFO_BODY_STRUCT);
		if(body)
		{
			target = cJSON_GetObjectItem(body, SUSICTRL_AUTOUPLOAD_INTERVAL_MS);
			if(target)
			{
				*intervalTimeMs = target->valueint;
				target = cJSON_GetObjectItem(body, SUSICTRL_AUTOUPLOAD_CONTINUE_MS);
				if(target)
				{
					*continueTimeMs = target->valueint;
					target = cJSON_GetObjectItem(body, SUSICTRL_AUTOREP_REQ_ITEMS);
					if(target)
					{
						char * tmpJsonStr = NULL;
						int len = 0;
						tmpJsonStr = cJSON_PrintUnformatted(target);
						len = strlen(tmpJsonStr) + 1;
						strcpy(repFilter, tmpJsonStr);	
						free(tmpJsonStr);
						bRet = true;
					}
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
	cJSON* target = NULL;

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

bool Parser_ParseIotDataInfo(char * iotDataJsonStr, iot_data_info_t * pIotDataInfo)
{
	bool bRet = false;
	if(iotDataJsonStr == NULL || pIotDataInfo == NULL) return bRet;
	{
		cJSON * iotDataItem = NULL, *subItem = NULL;
		iotDataItem = cJSON_Parse(iotDataJsonStr);
		if(iotDataItem)
		{
			subItem = cJSON_GetObjectItem(iotDataItem, SUSICTRL_N_FLAG);
			if(subItem)
			{
				int len = strlen(subItem->valuestring)+1;
				pIotDataInfo->name = (char *)malloc(len);
				memset(pIotDataInfo->name, 0, len);
				strcpy(pIotDataInfo->name, subItem->valuestring);
				subItem = cJSON_GetObjectItem(iotDataItem, SUSICTRL_ID_FLAG);
				if(subItem)
				{
					pIotDataInfo->id = subItem->valueint;
					subItem = cJSON_GetObjectItem(iotDataItem, SUSICTRL_V_FLAG);
					if(!subItem) subItem = cJSON_GetObjectItem(iotDataItem, SUSICTRL_BV_FLAG);
					if(subItem)
					{
						pIotDataInfo->val.uVal.vf = (float)subItem->valuedouble;
						pIotDataInfo->val.vType = VT_F;
						bRet = true;
					}
					else
					{
						subItem = cJSON_GetObjectItem(iotDataItem, SUSICTRL_SV_FLAG);
						if(subItem)
						{
							int len = strlen(subItem->valuestring)+1;
							pIotDataInfo->val.uVal.vs = (char *)malloc(len);
							memset(pIotDataInfo->val.uVal.vs, 0, len);
							strcpy(pIotDataInfo->val.uVal.vs, subItem->valuestring);
							bRet = true;
						}
					}
				}
			}
			cJSON_Delete(iotDataItem);
		}
	}
	return bRet;
}

bool Parser_ParseDataInfo(cJSON * jsonObj, iot_data_info_t * pDataInfo)
{
	bool bRet = false;
   if(jsonObj == NULL || pDataInfo == NULL) return bRet;
	{
		cJSON * vItem = NULL;
		vItem = cJSON_GetObjectItem(jsonObj, SUSICTRL_N_FLAG);
		if(vItem)
		{
			int len = strlen(vItem->valuestring) + 1;
			pDataInfo->name = (char *)malloc(len);
			memset(pDataInfo->name, 0, len);
			strcpy(pDataInfo->name, vItem->valuestring);
			vItem = cJSON_GetObjectItem(jsonObj, SUSICTRL_ID_FLAG);
			if(vItem)
			{
				pDataInfo->id = vItem->valueint;
				vItem = cJSON_GetObjectItem(jsonObj, SUSICTRL_SV_FLAG);
				if(vItem)
				{
					int len = strlen(vItem->valuestring) + 1;
					pDataInfo->val.uVal.vs = (char *)malloc(len);
					memset(pDataInfo->val.uVal.vs, 0, len);
					strcpy(pDataInfo->val.uVal.vs, vItem->valuestring);
					pDataInfo->val.vType = VT_S;
					bRet = true;
				}
				else if(vItem = cJSON_GetObjectItem(jsonObj, SUSICTRL_V_FLAG)) 
				{
					pDataInfo->val.uVal.vf = (float)vItem->valuedouble;
					pDataInfo->val.vType = VT_F;
					bRet = true;
				}
				else if(vItem = cJSON_GetObjectItem(jsonObj, SUSICTRL_BV_FLAG)) 
				{
					pDataInfo->val.uVal.vf = (float)vItem->valuedouble;
					pDataInfo->val.vType = VT_F;
					bRet = true;
				}
			}
		}
	}
	return bRet;
}

bool Parser_ParseHWMItem(cJSON * jsonObj, iot_data_list dataList, iot_item_type_t itemType)
{
	bool bRet = false;
	if(jsonObj == NULL || dataList == NULL || itemType == IOT_IT_Unknown) return bRet;
	{
		cJSON * eItem = NULL;
		eItem = cJSON_GetObjectItem(jsonObj, SUSICTRL_E_FLAG);
		if(eItem)
		{
			int i = 0;
			int nCount = cJSON_GetArraySize(eItem);
			for(i = 0; i < nCount; i++)
			{
				cJSON * dataItem = cJSON_GetArrayItem(eItem, i);
				if(dataItem)
				{
					iot_data_node_t * newNode = NULL;
					newNode = (iot_data_node_t *)malloc(sizeof(iot_data_node_t));
					memset(newNode, 0, sizeof(iot_data_node_t));
					if(Parser_ParseDataInfo(dataItem, &newNode->dataInfo))
					{
						newNode->next = dataList->next;
						dataList->next = newNode;
					}
					else
					{
						free(newNode);
					}
				}
			}
			bRet = true;
		}
	}
	return bRet;
}

bool Parser_ParseIotHWMInfo(char * iotDatajsonStr, iot_hwm_info_t * pIotHWMInfo)
{
	bool bRet = false;
   if(iotDatajsonStr == NULL || NULL == pIotHWMInfo) return bRet;
	{
		cJSON * root = NULL;
		root = cJSON_Parse(iotDatajsonStr);
		if(root)
		{
			cJSON * hwmItem = NULL;
			hwmItem = cJSON_GetObjectItem(root, SUSICTRL_HARDWARE_MONITOR);
			if(hwmItem)
			{
				cJSON * subItem = NULL;
				subItem = cJSON_GetObjectItem(hwmItem, SUSICTRL_HWM_TEMP);
				if(subItem)
				{
					Parser_ParseHWMItem(subItem, pIotHWMInfo->tempDataList, IOT_IT_HWM_TEMP);
				}
				subItem = cJSON_GetObjectItem(hwmItem, SUSICTRL_HWM_VOLT);
				if(subItem)
				{
					Parser_ParseHWMItem(subItem, pIotHWMInfo->voltDataList, IOT_IT_HWM_VOLT);
				}
				subItem = cJSON_GetObjectItem(hwmItem, SUSICTRL_HWM_FAN);
				if(subItem)
				{
					Parser_ParseHWMItem(subItem, pIotHWMInfo->fanDataList, IOT_IT_HWM_FAN);
				}
				bRet = true;
			}
			cJSON_Delete(root);
		}
	}
	return bRet;
}

bool Parser_ParseThrItemInfo(cJSON * jsonObj, susictrl_thr_item_info_t * pThrItemInfo)
{
	bool bRet = false;
	if(jsonObj == NULL || pThrItemInfo == NULL) return bRet;
	{
		cJSON * pSubItem = NULL;
		pSubItem = cJSON_GetObjectItem(jsonObj, SUSICTRL_N_FLAG);
		if(pSubItem)
		{
			int len = strlen(pSubItem->valuestring)+1;
			pThrItemInfo->name = (char *)malloc(len);
			memset(pThrItemInfo->name, 0, len);
			strcpy(pThrItemInfo->name, pSubItem->valuestring);

			pSubItem = cJSON_GetObjectItem(jsonObj, SUSICTRL_THR_ID);
			if(pSubItem)
			{
				pThrItemInfo->id = pSubItem->valueint;

				pThrItemInfo->isEnable = 0;
				pSubItem = cJSON_GetObjectItem(jsonObj, SUSICTRL_THR_ENABLE);
				if(pSubItem)
				{
					if(!_stricmp(pSubItem->valuestring, "true"))
					{
						pThrItemInfo->isEnable = 1;
					}
				}

				pSubItem = cJSON_GetObjectItem(jsonObj, SUSICTRL_THR_DESC);
				if(pSubItem)
				{
					int len = strlen(pSubItem->valuestring)+1;
					pThrItemInfo->desc = (char *)malloc(len);
					memset(pThrItemInfo->desc, 0, len);
					strcpy(pThrItemInfo->desc, pSubItem->valuestring);
				}
				else
				{
					pThrItemInfo->desc = NULL;
				}

				pSubItem = cJSON_GetObjectItem(jsonObj, SUSICTRL_THR_MAX);
				if(pSubItem)
				{
					pThrItemInfo->maxThr = (float)pSubItem->valuedouble;
				}
				else
				{
					pThrItemInfo->maxThr = DEF_INVALID_VALUE;
				}

				pSubItem = cJSON_GetObjectItem(jsonObj, SUSICTRL_THR_MIN);
				if(pSubItem)
				{
					pThrItemInfo->minThr = (float)pSubItem->valuedouble;
				}
				else
				{
					pThrItemInfo->minThr = DEF_INVALID_VALUE;
				}

				pSubItem = cJSON_GetObjectItem(jsonObj, SUSICTRL_THR_TYPE);
				if(pSubItem)
				{
					pThrItemInfo->thrType = pSubItem->valueint;
				}
				else
				{
					pThrItemInfo->thrType = DEF_THR_UNKNOW_TYPE;
				}

				pSubItem = cJSON_GetObjectItem(jsonObj, SUSICTRL_THR_LTIME);
				if(pSubItem)
				{
					pThrItemInfo->lastingTimeS = pSubItem->valueint;
				}
				else
				{
					pThrItemInfo->lastingTimeS = DEF_INVALID_TIME;
				}

				pSubItem = cJSON_GetObjectItem(jsonObj, SUSICTRL_THR_ITIME);
				if(pSubItem)
				{
					pThrItemInfo->intervalTimeS = pSubItem->valueint;
				}
				else
				{
					pThrItemInfo->intervalTimeS = DEF_INVALID_TIME;
				}

				pThrItemInfo->checkRetValue = DEF_INVALID_VALUE;
				pThrItemInfo->checkSrcValList.head = NULL;
				pThrItemInfo->checkSrcValList.nodeCnt = 0;
				pThrItemInfo->checkType = ck_type_avg;
				pThrItemInfo->isNormal = 1;
				pThrItemInfo->isInvalid = 0;
				pThrItemInfo->repThrTime = 0;

				bRet = true;
			}
			else
			{
				free(pThrItemInfo->name);
			}
		}
	}
	return bRet;
}

bool Parser_ParseThrItemList(cJSON * jsonObj, susictrl_thr_item_list thrItemList)
{
	bool bRet = false;
   if(jsonObj == NULL || thrItemList == NULL) return bRet;
	{
		susictrl_thr_item_node_t * head = thrItemList;
		cJSON * subItem = NULL;
		int nCount = cJSON_GetArraySize(jsonObj);
		int i = 0;
		for(i=0; i<nCount; i++)
		{
			subItem = cJSON_GetArrayItem(jsonObj, i);
			if(subItem)
			{
				susictrl_thr_item_node_t * pThrItemNode = NULL;
				pThrItemNode = (susictrl_thr_item_node_t *)malloc(sizeof(susictrl_thr_item_node_t));
				memset(pThrItemNode, 0, sizeof(susictrl_thr_item_node_t));
            if(Parser_ParseThrItemInfo(subItem, &pThrItemNode->thrItemInfo))
				{
					pThrItemNode->next = head->next;
					head->next = pThrItemNode;
				}
				else
				{
					free(pThrItemNode);
					pThrItemNode = NULL;
				}
			}
		}
		bRet = true;
	}
	return bRet;
}

bool Parser_ParseThrInfo(char * thrJsonStr, susictrl_thr_item_list thrList)
{
	bool bRet = false;
	if(thrJsonStr == NULL || thrList == NULL) return bRet;
	{
		cJSON * root = NULL;
		root = cJSON_Parse(thrJsonStr);
		if(root)
		{
			cJSON * commDataItem = cJSON_GetObjectItem(root, SUSICTRL_JSON_ROOT_NAME);
			if(commDataItem)
			{
				cJSON * thrItem = NULL; 
				thrItem = cJSON_GetObjectItem(commDataItem, SUSICTRL_THR);
				if(thrItem)
				{
					Parser_ParseThrItemList(thrItem, thrList);
					bRet = true;
				}
			}
			cJSON_Delete(root);
		}
	}
	return bRet;
}

cJSON * Parser_FindTargetItem(cJSON * fatherItem, char * pathStr, int pathSliceCurPos, int findType)
{
	static char * sliceStr[16] = {NULL};
	static int sliceCnt = 0;
	char *tmpPathStr = NULL;
	cJSON * targItem = NULL;
	if(fatherItem == NULL || findType == PFT_UNKONW) return targItem;
	if(pathSliceCurPos<1)pathSliceCurPos = 1;
	if(pathStr != NULL)
	{
		char * buf = NULL;
		int len = 0, i=0, j = 0, lastNPos = 0;
		len = strlen(pathStr)+1;
		tmpPathStr = (char*)malloc(len);
		memset(tmpPathStr, 0, len);
		strcpy(tmpPathStr, pathStr);
		buf = tmpPathStr;
		while(sliceStr[i] = strtok(buf, "/"))
		{
			i++;
			buf = NULL;
		}
		sliceCnt = i;
		if(sliceCnt<2 || strcmp(sliceStr[0], DEF_HANDLER_NAME)) return targItem;
	}
	{
		cJSON * subItem = NULL;
		if(pathSliceCurPos == sliceCnt - 1) subItem = fatherItem;
		else
		{
			subItem = cJSON_GetObjectItem(fatherItem, sliceStr[pathSliceCurPos]);
			if(!subItem)
			{
				cJSON * eSubItem = NULL;
				cJSON * eItem = cJSON_GetObjectItem(fatherItem, SUSICTRL_E_FLAG);
				if(eItem)
				{
					int count = cJSON_GetArraySize(eItem);
					int i = 0;
					for(i = 0; i<count; i++)
					{
						eSubItem = cJSON_GetArrayItem(eItem, i);
						subItem = cJSON_GetObjectItem(eSubItem, sliceStr[pathSliceCurPos]);
						if(subItem)
						{
							targItem = Parser_FindTargetItem(subItem, NULL, pathSliceCurPos+1, findType);
							if(targItem) break;
						}
					}
				}
			}
			else
			{
				targItem = Parser_FindTargetItem(subItem, NULL, pathSliceCurPos+1, findType);
			}
		}
		if(targItem == NULL)
		{
			if(subItem && pathSliceCurPos == sliceCnt - 1)
			{
				cJSON *eItem = NULL, *eSubItem = NULL, *targFatherItem = NULL,*nItem = NULL, *tmpTargItem = NULL;
				nItem = cJSON_GetObjectItem(subItem, SUSICTRL_N_FLAG);
				if(!nItem)
				{
					eItem = cJSON_GetObjectItem(subItem, SUSICTRL_E_FLAG);
					if(eItem)
					{
						int count = cJSON_GetArraySize(eItem);
						int i = 0;
						for(i = 0; i<count; i++)
						{
							eSubItem = cJSON_GetArrayItem(eItem, i);
							nItem = cJSON_GetObjectItem(eSubItem, SUSICTRL_N_FLAG);
							if(nItem && !strcmp(nItem->valuestring, sliceStr[pathSliceCurPos]))
							{
								targFatherItem = eSubItem;
								break;
							}
						}
					}
				}
				else
				{
					if(!strcmp(nItem->valuestring, sliceStr[pathSliceCurPos]))
					{
						targFatherItem = subItem;
					}
				}
				if(targFatherItem)
				{
					switch(findType)
					{
					case PFT_VALUE:
						{
							tmpTargItem = cJSON_GetObjectItem(targFatherItem, SUSICTRL_V_FLAG);
							if(tmpTargItem == NULL)
							{
								tmpTargItem = cJSON_GetObjectItem(targFatherItem, SUSICTRL_SV_FLAG);
								if(tmpTargItem == NULL)
								{
									tmpTargItem = cJSON_GetObjectItem(targFatherItem, SUSICTRL_BV_FLAG);
								}
							}
							break;
						}
					case PFT_ID:
						{
							tmpTargItem = cJSON_GetObjectItem(targFatherItem, SUSICTRL_ID_FLAG);
							break;
						}
					case PFT_ASM:
						{
							tmpTargItem = cJSON_GetObjectItem(targFatherItem, SUSICTRL_ASM_FLAG);
							break;
						}
					case PFT_DESC:
						{
							tmpTargItem = cJSON_GetObjectItem(targFatherItem, SUSICTRL_DESC_FLAG);
							break;
						}
					default: 
						{
							tmpTargItem = NULL;
							break;
						}
					}
					if(tmpTargItem)
					{
						targItem = cJSON_Duplicate(tmpTargItem, 1);
					}
				}
			}
		}	
	}
	if(tmpPathStr)free(tmpPathStr);
	return targItem;
}

bool Parser_GetItemDescWithPath(char * path, char * cpbStr, char * descStr)
{
	bool bRet = false;
   if(path == NULL || NULL == cpbStr || NULL == descStr) return bRet;
	{
		cJSON *pfiRoot = NULL;
		pfiRoot = cJSON_Parse(cpbStr);
		if(pfiRoot)
		{
			cJSON *findDescItem = NULL;
			findDescItem = Parser_FindTargetItem(pfiRoot, path, 0, PFT_DESC);
			if(findDescItem)
			{
            strcpy(descStr, findDescItem->valuestring);
				bRet = true;
				cJSON_Delete(findDescItem);
			}
			cJSON_Delete(pfiRoot);
		}
	}
	return bRet;
}

bool Parser_GetSensorJsonStr(char * curIotJsonStr, char * iotPFIJsonStr, sensor_info_t * pSensorInfo)
{
	bool bRet = false;
	cJSON * root = NULL, *pfiRoot = NULL, *targetItem = NULL;
	char * tmpJsonStr = NULL;
	int len = 0;
	if(NULL == curIotJsonStr || NULL == iotPFIJsonStr || NULL == pSensorInfo) return bRet;
	targetItem = cJSON_CreateObject();
	cJSON_AddStringToObject(targetItem, SUSICTRL_N_FLAG, pSensorInfo->pathStr);
	if(pSensorInfo->pathStr != NULL && strlen(pSensorInfo->pathStr))
	{
		root = cJSON_Parse(curIotJsonStr);
		if(root)
		{
			pfiRoot = cJSON_Parse(iotPFIJsonStr);
			if(pfiRoot)
			{
				cJSON *findAsmItem = NULL;
				findAsmItem = Parser_FindTargetItem(pfiRoot, pSensorInfo->pathStr, 0, PFT_ASM);
				if(findAsmItem)
				{
					if(strstr(findAsmItem->valuestring, "r"))
					{
						cJSON *findVItem = NULL;
						findVItem = Parser_FindTargetItem(root, pSensorInfo->pathStr, 0, PFT_VALUE);
						if(findVItem == NULL)
						{
							cJSON_AddStringToObject(targetItem, SUSICTRL_SV_FLAG, IOT_SGRC_STR_NOT_FOUND);
							cJSON_AddNumberToObject(targetItem, SUSICTRL_STATUS_CODE_FLAG, IOT_SGRC_NOT_FOUND);
						}
						else
						{
							char * tmpV = NULL;
							len = strlen(findVItem->string)+1;
							tmpV = (char*)malloc(len);
							memset(tmpV, 0, len);
							strcpy(tmpV, findVItem->string);
							cJSON_AddItemToObject(targetItem, tmpV, findVItem);
							cJSON_AddNumberToObject(targetItem, SUSICTRL_STATUS_CODE_FLAG, IOT_SGRC_SUCCESS);
							free(tmpV);
							bRet = true;
						}
					}
					else
					{
						cJSON_AddStringToObject(targetItem, SUSICTRL_SV_FLAG, IOT_SGRC_STR_WRIT_ONLY);
						cJSON_AddNumberToObject(targetItem, SUSICTRL_STATUS_CODE_FLAG, IOT_SGRC_WRIT_ONLY);
					}
					cJSON_Delete(findAsmItem);
				}
				else
				{
					cJSON_AddStringToObject(targetItem, SUSICTRL_SV_FLAG, IOT_SGRC_STR_NOT_FOUND);
					cJSON_AddNumberToObject(targetItem, SUSICTRL_STATUS_CODE_FLAG, IOT_SGRC_NOT_FOUND);
				}
				cJSON_Delete(pfiRoot);
			}
			else
			{
				cJSON_AddStringToObject(targetItem, SUSICTRL_SV_FLAG, IOT_SGRC_STR_FAIL);
				cJSON_AddNumberToObject(targetItem, SUSICTRL_STATUS_CODE_FLAG, IOT_SGRC_FAIL);
			}
			cJSON_Delete(root);
		}
		else
		{
			cJSON_AddStringToObject(targetItem, SUSICTRL_SV_FLAG, IOT_SGRC_STR_FAIL);
			cJSON_AddNumberToObject(targetItem, SUSICTRL_STATUS_CODE_FLAG, IOT_SGRC_FAIL);
		}
	}
	else
	{
		cJSON_AddStringToObject(targetItem, SUSICTRL_SV_FLAG, IOT_SGRC_STR_FAIL);
		cJSON_AddNumberToObject(targetItem, SUSICTRL_STATUS_CODE_FLAG, IOT_SGRC_FAIL);
	}
	tmpJsonStr = cJSON_PrintUnformatted(targetItem);
	len = strlen(tmpJsonStr) + 1;
	pSensorInfo->jsonStr = (char *)(malloc(len));
	memset(pSensorInfo->jsonStr, 0, len);
	strcpy(pSensorInfo->jsonStr, tmpJsonStr);	
	free(tmpJsonStr);
	cJSON_Delete(targetItem);
	return bRet;
}

bool Parser_GetSensorSetJsonStr(char * curIotJsonStr, char * iotPFIJsonStr, sensor_info_t * pSensorInfo)
{
	bool bRet = false;
	if(NULL == curIotJsonStr || NULL == pSensorInfo) return bRet;
	if(pSensorInfo->pathStr != NULL && strlen(pSensorInfo->pathStr))
	{
		cJSON * root = NULL, *pfiRoot = NULL;
		pfiRoot = cJSON_Parse(iotPFIJsonStr);
		if(pfiRoot)
		{
			cJSON *findAsmItem = NULL;
			findAsmItem = Parser_FindTargetItem(pfiRoot, pSensorInfo->pathStr, 0, PFT_ASM);
			if(findAsmItem)
			{
				if(strstr(findAsmItem->valuestring, "w"))
				{
					root = cJSON_Parse(curIotJsonStr);
					if(root)
					{
						int len = 0;
						cJSON *findVItem = NULL;
						findVItem = Parser_FindTargetItem(root, pSensorInfo->pathStr, 0, PFT_ID);
						if(findVItem)
						{
							cJSON * setParamItem = cJSON_Parse(pSensorInfo->jsonStr);
							if(setParamItem)
							{
								cJSON * vItem = NULL;
								vItem = cJSON_GetObjectItem(setParamItem, SUSICTRL_V_FLAG);
								if(vItem == NULL)
								{
									vItem = cJSON_GetObjectItem(setParamItem, SUSICTRL_SV_FLAG);
									if(vItem == NULL)
									{
										vItem = cJSON_GetObjectItem(setParamItem, SUSICTRL_BV_FLAG);
									}
								}
								if(vItem)
								{
									char * tmpV = NULL;
									int len = 0;
									cJSON * tmpSetVItem = NULL;
									cJSON * setItem = cJSON_CreateObject();
									tmpSetVItem = cJSON_Duplicate(vItem, 1);
									len = strlen(tmpSetVItem->string)+1;
									tmpV = (char*)malloc(len);
									memset(tmpV, 0, len);
									strcpy(tmpV, tmpSetVItem->string);
									cJSON_AddItemToObject(setItem, tmpV, tmpSetVItem);
									free(tmpV);
									cJSON_AddNumberToObject(setItem, SUSICTRL_ID_FLAG, findVItem->valueint);
									{
										char * tmpJsonStr = NULL;
										tmpJsonStr = cJSON_PrintUnformatted(setItem);
										len = strlen(tmpJsonStr) + 1;
										if(pSensorInfo->jsonStr) free(pSensorInfo->jsonStr);
										pSensorInfo->jsonStr = (char *)(malloc(len));
										memset(pSensorInfo->jsonStr, 0, len);
										strcpy(pSensorInfo->jsonStr, tmpJsonStr);	
										free(tmpJsonStr);
									}
									cJSON_Delete(setItem);
									bRet = true;
								}
								else
								{
									pSensorInfo->setRet = SSR_WRONG_FORMAT;
								}
								cJSON_Delete(setParamItem);
							} 
							else
							{
								pSensorInfo->setRet = SSR_WRONG_FORMAT;
							}
							cJSON_Delete(findVItem);
						}
						else
						{
							pSensorInfo->setRet = SSR_NOT_FOUND;
						}
						cJSON_Delete(root);
					}
					else
					{
						pSensorInfo->setRet = SSR_FAIL;
					}
				}
				else
				{
					pSensorInfo->setRet = SSR_READ_ONLY;
				}
				cJSON_Delete(findAsmItem);
			}
			else
			{
				pSensorInfo->setRet = SSR_NOT_FOUND;
			}
			cJSON_Delete(pfiRoot);
		}
		else
		{
			pSensorInfo->setRet = SSR_FAIL;
		}
	}
	else
	{
		pSensorInfo->setRet = SSR_FAIL;
	}
	return bRet;
}

/*
bool Parser_GetSensorSetJsonStr(char * curIotJsonStr, sensor_info_t * pSensorInfo)
{
	bool bRet = false;
	if(NULL == curIotJsonStr || NULL == pSensorInfo) return bRet;
	if(pSensorInfo->pathStr != NULL && strlen(pSensorInfo->pathStr))
	{
		cJSON * root = cJSON_Parse(curIotJsonStr);
		if(root)
		{
			cJSON * subItem = NULL, *fatherItem = NULL, *nItem = NULL, *idItem = NULL, *findVItem = NULL, *targetItem=NULL;
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
			if(i>1 && !strcmp(sliceStr[0], DEF_HANDLER_NAME))
			{
				fatherItem = root;
				for(j = 1; j<i-1 && sliceStr[j]!=NULL; j++)
				{
					if(i == 2) subItem = fatherItem;
					else
					{
						subItem = cJSON_GetObjectItem(fatherItem, sliceStr[j]);
						if(!subItem)
						{
							cJSON * eSubItem = NULL;
							cJSON * eItem = cJSON_GetObjectItem(fatherItem, SUSICTRL_E_FLAG);
							if(eItem)
							{
								int count = cJSON_GetArraySize(eItem);
								int i = 0;
								for(i = 0; i<count; i++)
								{
									eSubItem = cJSON_GetArrayItem(eItem, i);
									subItem = cJSON_GetObjectItem(eSubItem, sliceStr[j]);
									if(subItem)break;
								}
							}
						}
					}

					if(subItem)
					{
						if(i == 2 || j == i-2)
						{
							cJSON * eSubItem = NULL;
							cJSON * eItem = NULL;
							if(i==2) lastNPos = j;
							else lastNPos = j+1;
							nItem = cJSON_GetObjectItem(subItem, SUSICTRL_N_FLAG);
							if(!nItem)
							{
								eSubItem = NULL;
								eItem = cJSON_GetObjectItem(subItem, SUSICTRL_E_FLAG);
								if(eItem)
								{
									int count = cJSON_GetArraySize(eItem);
									int i = 0;
									for(i = 0; i<count; i++)
									{
										eSubItem = cJSON_GetArrayItem(eItem, i);
										nItem = cJSON_GetObjectItem(eSubItem, SUSICTRL_N_FLAG);
										if(nItem && !strcmp(nItem->valuestring, sliceStr[lastNPos]))break;
									}
								}
							}
							if(nItem && !strcmp(nItem->valuestring, sliceStr[lastNPos]))
							{
								idItem = cJSON_GetObjectItem(eSubItem, SUSICTRL_ID_FLAG);
								if(idItem)
								{
									cJSON * setParamItem = cJSON_Parse(pSensorInfo->jsonStr);
									if(setParamItem)
									{
										cJSON * vItem = NULL;
										vItem = cJSON_GetObjectItem(eSubItem, SUSICTRL_V_FLAG);
										if(vItem == NULL)
										{
											vItem = cJSON_GetObjectItem(eSubItem, SUSICTRL_SV_FLAG);
											if(vItem == NULL)
											{
												vItem = cJSON_GetObjectItem(eSubItem, SUSICTRL_BV_FLAG);
											}
										}
										if(vItem)
										{
											char * tmpV = NULL;
											int len = 0;
											targetItem = cJSON_CreateObject();
											findVItem = cJSON_Duplicate(vItem, 1);
											len = strlen(findVItem->string)+1;
											tmpV = (char*)malloc(len);
											memset(tmpV, 0, len);
											strcpy(tmpV, findVItem->string);
											cJSON_AddItemToObject(targetItem, tmpV, findVItem);
											free(tmpV);
											cJSON_AddNumberToObject(targetItem, SUSICTRL_ID_FLAG, idItem->valueint);
											bRet = true;
										}
										cJSON_Delete(setParamItem);
									}
									pSensorInfo->id = idItem->valueint;
								}
							}
							break;
						}
						fatherItem = subItem;
					}
					else break;
				}
			}
			if(targetItem == NULL)
			{
				pSensorInfo->setRet = SSR_NOT_FOUND;
			}
			else
			{
				char * tmpJsonStr = NULL;
				tmpJsonStr = cJSON_PrintUnformatted(targetItem);
				len = strlen(tmpJsonStr) + 1;
				if(pSensorInfo->jsonStr) free(pSensorInfo->jsonStr);
				pSensorInfo->jsonStr = (char *)(malloc(len));
				memset(pSensorInfo->jsonStr, 0, len);
				strcpy(pSensorInfo->jsonStr, tmpJsonStr);	
				free(tmpJsonStr);
				cJSON_Delete(targetItem);
			}
			free(tmpPathStr);
			cJSON_Delete(root);
		}
	}
	return bRet;
}*/

