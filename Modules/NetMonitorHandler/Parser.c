#include "Parser.h"


//------------------------------------------------------net------------------------------------------------------------

static cJSON * cJSON_CreateSensorInfoList(sensor_info_node_t * sensorInfoList)
{
	cJSON *pSensorInfoListItem = NULL, *eItem = NULL;
	if(!sensorInfoList) return NULL;
	pSensorInfoListItem = cJSON_CreateObject();
	eItem = cJSON_CreateArray();
	cJSON_AddItemToObject(pSensorInfoListItem, NETMON_E_FLAG, eItem);
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

static cJSON * cJSON_CreateNetMonInfo(net_info_t * pNetInfo)
{
	cJSON * pNetInfoItem = NULL;
	if(!pNetInfo) return NULL;
	pNetInfoItem = cJSON_CreateObject();
	if(pNetInfo->adapterName)
	{
		cJSON_AddStringToObject(pNetInfoItem, NET_INFO_ADAPTER_NAME, pNetInfo->adapterName);
	}
	else
	{
		cJSON_AddStringToObject(pNetInfoItem, NET_INFO_ADAPTER_NAME, "");
	}
	if(pNetInfo->adapterDescription)
	{
		cJSON_AddStringToObject(pNetInfoItem, NET_INFO_ADAPTER_DESCRI, pNetInfo->adapterDescription);
	}
	else
	{
		cJSON_AddStringToObject(pNetInfoItem, NET_INFO_ADAPTER_DESCRI, "");
	}
	if(pNetInfo->type)
	{
		cJSON_AddStringToObject(pNetInfoItem, NET_INFO_TYPE, pNetInfo->type);
	}
	else
	{
		cJSON_AddStringToObject(pNetInfoItem, NET_INFO_TYPE, "");
	}
	cJSON_AddNumberToObject(pNetInfoItem, NET_INFO_INDEX, pNetInfo->index);//SA3.0 必须需要
	if(pNetInfo->netStatus)
	{
		//cJSON_AddStringToObject(pNetInfoItem, NET_INFO_STATUS, pNetInfo->netStatus);
		cJSON_AddStringToObject(pNetInfoItem, NET_INFO_STATUS_30, pNetInfo->netStatus);//SA3.0,下同
	}
	else
	{
		cJSON_AddStringToObject(pNetInfoItem, NET_INFO_STATUS_30, "");
	}
	cJSON_AddNumberToObject(pNetInfoItem, NET_INFO_SPEED_MBPS_30, pNetInfo->netSpeedMbps);
	cJSON_AddNumberToObject(pNetInfoItem, NET_INFO_USAGE, pNetInfo->netUsage);
	cJSON_AddNumberToObject(pNetInfoItem, NET_INFO_SEND_BYTE, pNetInfo->sendDateByte);
	cJSON_AddNumberToObject(pNetInfoItem, NET_INFO_SEND_THROUGHPUT, pNetInfo->sendThroughput);
	cJSON_AddNumberToObject(pNetInfoItem, NET_INFO_RECV_BYTE, pNetInfo->recvDateByte);
	cJSON_AddNumberToObject(pNetInfoItem, NET_INFO_RECV_THROUGHPUT, pNetInfo->recvThroughput);
	return pNetInfoItem;
}

static cJSON * cJSON_CreateNetMonInfoList(net_info_node_t * netMonInfoList)
{
	cJSON *pNetMonInfoListItem = NULL;
	if(!netMonInfoList) return NULL;
	pNetMonInfoListItem = cJSON_CreateArray();
	{
		net_info_node_t * head = netMonInfoList;
		net_info_node_t * curNode = head->next;
		while(curNode)
		{
			cJSON * pNetMonInfoItem = cJSON_CreateObject();
			cJSON_AddNetMonInfoToObject(pNetMonInfoItem, NET_MON_INFO, &curNode->netInfo);
			cJSON_AddItemToArray(pNetMonInfoListItem, pNetMonInfoItem);
			curNode = curNode->next;
		}
	}
	return pNetMonInfoListItem;
}

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

int Pack_netmon_netinfo_rep(char * pCommData, char** outputStr)
{
	net_info_node_t * pNetInfoHead = NULL;
	net_info_node_t ** ppNetInfoHead = &pNetInfoHead;

	char * out = NULL;
	int outLen = 0;
	cJSON *pSUSICommDataItem = NULL;

	if(pCommData == NULL || outputStr == NULL) return outLen;
	pSUSICommDataItem = cJSON_CreateObject();

	memcpy(ppNetInfoHead, pCommData, sizeof(net_info_node_t *));

	cJSON_AddNetMonInfoListToObject(pSUSICommDataItem, NET_MON_INFO_LIST, pNetInfoHead);

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


int Pack_netmon_set_netinfo_auto_upload_rep(char *pCommData, char **outputStr)
{
	char * out = NULL;
	int outLen = 0;
	cJSON *pSUSICommDataItem = NULL;
	if(pCommData == NULL || outputStr == NULL) return outLen;
	pSUSICommDataItem = cJSON_CreateObject();

	cJSON_AddStringToObject(pSUSICommDataItem, NETMON_SET_AUTO_UPLOAD_REP, pCommData);

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


int Pack_netmon_error_rep(char *pCommData, char **outputStr)
{
	char * out = NULL;
	int outLen = 0;
	cJSON *pSUSICommDataItem = NULL;
	if(pCommData == NULL || outputStr == NULL) return outLen;
	pSUSICommDataItem = cJSON_CreateObject();

	cJSON_AddStringToObject(pSUSICommDataItem, NETMON_ERROR_REP, pCommData);

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


int Pack_netmon_set_netinfo_reqp_rep(char *pCommData, char **outputStr)
{
	char * out = NULL;
	int outLen = 0;
	cJSON *pSUSICommDataItem = NULL;
	if(pCommData == NULL || outputStr == NULL) return outLen;
	pSUSICommDataItem = cJSON_CreateObject();

	cJSON_AddStringToObject(pSUSICommDataItem, NETMON_SET_REQP_REP, pCommData);

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


int Parse_netmon_set_netinfo_auto_upload_req(char *inputstr,  auto_upload_params_t* outVal)
{
	int iRet = 0;
	cJSON* root = NULL;
	cJSON* pSUSICommDataItem = NULL;
	cJSON *pSubItem = NULL; 
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
		cJSON *pAutoUploadParamsItem = NULL;
		auto_upload_params_t * pAutoUploadParams = NULL;
		if(!outVal) return false;

		pAutoUploadParams = outVal;

		pAutoUploadParamsItem = cJSON_GetObjectItem(pSUSICommDataItem, AUTO_UPLOAD_PARAMS);
		if(pAutoUploadParams && pAutoUploadParamsItem)
		{
			pSubItem = cJSON_GetObjectItem(pAutoUploadParamsItem, AUTO_UPLOAD_INTERVAL_MS);
			if(pSubItem)
			{
				pAutoUploadParams->auto_upload_interval_ms = pSubItem->valueint;
				pSubItem = cJSON_GetObjectItem(pAutoUploadParamsItem, AUTO_UPLOAD_TIMEOUT_MS);
				if(pSubItem)
				{
					pAutoUploadParams->auto_upload_timeout_ms = pSubItem->valueint;
					iRet = 1;
				}
			}
		}
	}
	cJSON_Delete(root);
	return iRet;
}

bool Parser_ParseAutoUploadCmd(char * cmdJsonStr, sensor_info_list siList, bool * isAutoUpdateAll,unsigned int * intervalTimeS)
{
	bool bRet = false;
	cJSON* root = NULL;
	cJSON* body = NULL;
	cJSON* target = NULL;

	if(cmdJsonStr == NULL || !siList || NULL == intervalTimeS) return false;

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
				bRet = true;
			}
			target = cJSON_GetObjectItem(body, NETMON_REQUEST_ITEMS);
			if(target)
			{
				cJSON * handlerItem = NULL;
				handlerItem = cJSON_GetObjectItem(target, NETMON_AUTOUPDATE_ALL);
				if(handlerItem)
					*isAutoUpdateAll = true;
				else
				{
					handlerItem = cJSON_GetObjectItem(target, DEF_HANDLER_NAME);
					if(handlerItem)
					{
						cJSON * eItem = NULL;
						eItem = cJSON_GetObjectItem(handlerItem, NETMON_E_FLAG);
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
									valItem = cJSON_GetObjectItem(subItem, NETMON_N_FLAG);
									if(valItem)
									{
										int len = 0;
										char *tmpPathStr = NULL;
										char * sliceStr[16] = {NULL};
										char * buf = NULL;
										int i = 0;
										char *p = NULL;
										char * tempStr = NULL;
#ifdef WIN32   
										tempStr=UTF8ToANSI(valItem->valuestring);
										len = strlen(tempStr)+1;
										p = (char *)malloc(len);
										memcpy(p, tempStr, len);
										memcpy(valItem->valuestring, p, len);
										free(p);
										free(tempStr);
										p = NULL;
										tempStr = NULL;
#else
										len = strlen(valItem->valuestring)+1;

#endif


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
										if(i == 2 && !strcmp(sliceStr[0], DEF_HANDLER_NAME) && !strcmp(sliceStr[1], NET_MON_INFO_LIST))
										{
											*isAutoUpdateAll = true;
											free(tmpPathStr);
											break;
										}

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

bool Parser_ParseAutoUploadStopCmd(char * cmdJsonStr)
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


bool Parser_ParseAutoReportCmd(char * cmdJsonStr, sensor_info_list siList, bool * isAutoUpdateAll,unsigned int * intervalTimeS, unsigned int *timeoutMs)
{
	bool bRet = false;
	cJSON* root = NULL;
	cJSON* body = NULL;
	cJSON* target = NULL;
	cJSON* targetTmp = NULL;

	if(cmdJsonStr == NULL || !siList || NULL == intervalTimeS) return false;

	root = cJSON_Parse(cmdJsonStr);
	if(root)
	{
		body = cJSON_GetObjectItem(root, AGENTINFO_BODY_STRUCT);
		if(body)
		{
			//target = cJSON_GetObjectItem(body, SUSICTRL_AUTOREP_INTERVAL_SEC);
			target = cJSON_GetObjectItem(body, AUTO_UPLOAD_INTERVAL_MS);
			targetTmp = cJSON_GetObjectItem(body, AUTO_UPLOAD_TIMEOUT_MS);

			if(target)
			{
				*intervalTimeS = target->valueint;
				*timeoutMs = targetTmp->valueint;
				bRet = true;
			}
			target = cJSON_GetObjectItem(body, NETMON_REQUEST_ITEMS);
			if(target)
			{
				cJSON * handlerItem = NULL;
				handlerItem = cJSON_GetObjectItem(target, NETMON_AUTOUPDATE_ALL);
				if(handlerItem)
					*isAutoUpdateAll = true;
				else
				{
					//handlerItem = cJSON_GetObjectItem(target, DEF_HANDLER_NAME);
					//if(handlerItem)
					//{
					if(target)
					{
						cJSON * eItem = NULL;
						//eItem = cJSON_GetObjectItem(handlerItem, NETMON_E_FLAG);
						eItem = cJSON_GetObjectItem(target, NETMON_E_FLAG);
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
									valItem = cJSON_GetObjectItem(subItem, NETMON_N_FLAG);
									if(valItem)
									{
										int len = 0;
										char *tmpPathStr = NULL;
										char * sliceStr[16] = {NULL};
										char * buf = NULL;
										int i = 0;
										char *p = NULL;
										char * tempStr = NULL;

#ifdef WIN32   
										tempStr=UTF8ToANSI(valItem->valuestring);
										len = strlen(tempStr)+1;
										p = (char *)malloc(len);
										memcpy(p, tempStr, len);
										memcpy(valItem->valuestring, p, len);
										free(p);
										free(tempStr);
										p = NULL;
										tempStr = NULL;
#else
										len = strlen(valItem->valuestring)+1;

#endif
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
											tmpPathStr = NULL;
											break;
										}
										if(i == 2 && !strcmp(sliceStr[0], DEF_HANDLER_NAME) && !strcmp(sliceStr[1], NET_MON_INFO_LIST))
										{
											*isAutoUpdateAll = true;
											free(tmpPathStr);
											tmpPathStr = NULL;
											break;
										}

										newNode = (sensor_info_node_t *)malloc(sizeof(sensor_info_node_t));
										memset(newNode, 0, sizeof(sensor_info_node_t));
										newNode->sensorInfo.pathStr = (char *)malloc(len);
										memset(newNode->sensorInfo.pathStr, 0, len);
										strcpy(newNode->sensorInfo.pathStr, valItem->valuestring);
										curNode->next = newNode;
										curNode = newNode;

										if(tmpPathStr)
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

int Parser_PackCapabilityStrRep(net_info_list netInfoList, char ** outputStr)
{
	int IDX = 0;
	char * out = NULL;
	int outLen = 0;
	cJSON * rootItem = NULL, * cpbItem = NULL, * NMItem = NULL;
	if(netInfoList == NULL || outputStr == NULL) return outLen;
	rootItem = cJSON_CreateObject();
	NMItem = cJSON_CreateObject();
	cpbItem = cJSON_CreateArray();
	cJSON_AddItemToObject(rootItem, DEF_HANDLER_NAME, NMItem);
	cJSON_AddItemToObject(NMItem, NETMON_INFO_LIST, cpbItem);
	{
		net_info_node_t * curNode = netInfoList->next;
		while(curNode)
		{
			unsigned long tick = 0;
			cJSON *nmNodeItem = NULL, *eItem = NULL, *eSubItem = NULL;
			nmNodeItem = cJSON_CreateObject();
			cJSON_AddItemToArray(cpbItem, nmNodeItem);
			//cJSON_AddStringToObject(nmNodeItem, IPSO_BN, curNode->netInfo.adapterName);
			{
				char bnStr[256] = {0};
				curNode->netInfo.index = IDX++;
				sprintf(bnStr, "%s%d-%s", NETMON_GROUP_NETINDEX, curNode->netInfo.index, curNode->netInfo.adapterName);
				cJSON_AddStringToObject(nmNodeItem, IPSO_BN, bnStr);
			}
			tick = (unsigned long)time((time_t *) NULL);
			cJSON_AddNumberToObject(nmNodeItem, IPSO_BT, tick);
			cJSON_AddStringToObject(nmNodeItem, IPSO_BU, "");
			cJSON_AddNumberToObject(nmNodeItem, IPSO_VER, DEF_IPSO_VER);
			eItem = cJSON_CreateArray();
			cJSON_AddItemToObject(nmNodeItem, IPSO_E, eItem);
			eSubItem = cJSON_CreateObject();
			cJSON_AddItemToArray(eItem, eSubItem);
			cJSON_AddStringToObject(eSubItem, IPSO_N, NET_INFO_ADAPTER_NAME);
			cJSON_AddStringToObject(eSubItem, IPSO_SV, curNode->netInfo.adapterName);
			if(strlen(curNode->netInfo.alias))
			{
				eSubItem = cJSON_CreateObject();
				cJSON_AddItemToArray(eItem, eSubItem);
				cJSON_AddStringToObject(eSubItem, IPSO_N, IPSO_ALIAS);
				cJSON_AddStringToObject(eSubItem, IPSO_SV, curNode->netInfo.alias);
			}
			eSubItem = cJSON_CreateObject();
			cJSON_AddItemToArray(eItem, eSubItem);
			cJSON_AddStringToObject(eSubItem, IPSO_N, NET_INFO_ADAPTER_DESCRI);
			cJSON_AddStringToObject(eSubItem, IPSO_SV, curNode->netInfo.adapterDescription);
			eSubItem = cJSON_CreateObject();
			cJSON_AddItemToArray(eItem, eSubItem);
			cJSON_AddStringToObject(eSubItem, IPSO_N, NET_INFO_TYPE);
			cJSON_AddStringToObject(eSubItem, IPSO_SV, curNode->netInfo.type);
			//eSubItem = cJSON_CreateObject();
			//cJSON_AddItemToArray(eItem, eSubItem);
			//cJSON_AddStringToObject(eSubItem, IPSO_N, NET_INFO_INDEX);
			//cJSON_AddNumberToObject(eSubItem, IPSO_V, curNode->netInfo.index);
			eSubItem = cJSON_CreateObject();
			cJSON_AddItemToArray(eItem, eSubItem);
			cJSON_AddStringToObject(eSubItem, IPSO_N, NET_INFO_STATUS);
			cJSON_AddStringToObject(eSubItem, IPSO_SV, curNode->netInfo.netStatus);
			eSubItem = cJSON_CreateObject();
			cJSON_AddItemToArray(eItem, eSubItem);
			cJSON_AddStringToObject(eSubItem, IPSO_N, NET_INFO_SPEED_MBPS);
			cJSON_AddNumberToObject(eSubItem, IPSO_V, curNode->netInfo.netSpeedMbps);
			eSubItem = cJSON_CreateObject();
			cJSON_AddItemToArray(eItem, eSubItem);
			cJSON_AddStringToObject(eSubItem, IPSO_N, NET_INFO_USAGE);
			cJSON_AddNumberToObject(eSubItem, IPSO_V, curNode->netInfo.netUsage);
			eSubItem = cJSON_CreateObject();
			cJSON_AddItemToArray(eItem, eSubItem);
			cJSON_AddStringToObject(eSubItem, IPSO_N, NET_INFO_SEND_BYTE);
			cJSON_AddNumberToObject(eSubItem, IPSO_V, curNode->netInfo.sendDateByte);
			eSubItem = cJSON_CreateObject();
			cJSON_AddItemToArray(eItem, eSubItem);
			cJSON_AddStringToObject(eSubItem, IPSO_N, NET_INFO_SEND_THROUGHPUT);
			cJSON_AddNumberToObject(eSubItem, IPSO_V, curNode->netInfo.sendThroughput);
			eSubItem = cJSON_CreateObject();
			cJSON_AddItemToArray(eItem, eSubItem);
			cJSON_AddStringToObject(eSubItem, IPSO_N, NET_INFO_RECV_BYTE);
			cJSON_AddNumberToObject(eSubItem, IPSO_V, curNode->netInfo.recvDateByte);
			eSubItem = cJSON_CreateObject();
			cJSON_AddItemToArray(eItem, eSubItem);
			cJSON_AddStringToObject(eSubItem, IPSO_N, NET_INFO_RECV_THROUGHPUT);
			cJSON_AddNumberToObject(eSubItem, IPSO_V, curNode->netInfo.recvThroughput);
			curNode = curNode->next;
		}
	}

	out = cJSON_PrintUnformatted(rootItem);
	outLen = strlen(out) + 1;
	*outputStr = (char *)(malloc(outLen));
	memset(*outputStr, 0, outLen);
	strcpy(*outputStr, out);
	cJSON_Delete(rootItem);	
	printf("%s\n",out);	
	free(out);
	return outLen;
}


int Parser_PackCapabilityStrRepNew(net_info_list netInfoList, char *adapterName, char ** outputStr)
{
	BOOL flag = FALSE;
	char * out = NULL;
	int outLen = 0;
	cJSON * rootItem = NULL, * cpbItem = NULL;
	if(netInfoList == NULL || outputStr == NULL) return outLen;
	rootItem = cJSON_CreateObject();
	cpbItem = cJSON_CreateArray();
	cJSON_AddItemToObject(rootItem, NETMON_INFO_LIST, cpbItem);
	{
		net_info_node_t * curNode = netInfoList->next;
		while(curNode && !flag)
		{
			unsigned long tick = 0;
			cJSON *nmNodeItem = NULL, *eItem = NULL, *eSubItem = NULL;

			if( !strcmp(curNode->netInfo.adapterName, adapterName ))
			{
				flag = TRUE;
				nmNodeItem = cJSON_CreateObject();
				cJSON_AddItemToArray(cpbItem, nmNodeItem);
				cJSON_AddStringToObject(nmNodeItem, IPSO_BN, curNode->netInfo.adapterName);
				tick = (unsigned long)time((time_t *) NULL);
				cJSON_AddNumberToObject(nmNodeItem, IPSO_BT, tick);
				cJSON_AddStringToObject(nmNodeItem, IPSO_BU, "");
				cJSON_AddNumberToObject(nmNodeItem, IPSO_VER, DEF_IPSO_VER);
				eItem = cJSON_CreateArray();
				cJSON_AddItemToObject(nmNodeItem, IPSO_E, eItem);
				eSubItem = cJSON_CreateObject();
				cJSON_AddItemToArray(eItem, eSubItem);
				cJSON_AddStringToObject(eSubItem, IPSO_N, NET_INFO_ADAPTER_NAME);
				cJSON_AddStringToObject(eSubItem, IPSO_SV, curNode->netInfo.adapterName);
				eSubItem = cJSON_CreateObject();
				cJSON_AddItemToArray(eItem, eSubItem);
				cJSON_AddStringToObject(eSubItem, IPSO_N, NET_INFO_ADAPTER_DESCRI);
				cJSON_AddStringToObject(eSubItem, IPSO_SV, curNode->netInfo.adapterDescription);
				eSubItem = cJSON_CreateObject();
				cJSON_AddItemToArray(eItem, eSubItem);
				cJSON_AddStringToObject(eSubItem, IPSO_N, NET_INFO_TYPE);
				cJSON_AddStringToObject(eSubItem, IPSO_SV, curNode->netInfo.type);
				//eSubItem = cJSON_CreateObject();
				//cJSON_AddItemToArray(eItem, eSubItem);
				//cJSON_AddStringToObject(eSubItem, IPSO_N, NET_INFO_INDEX);
				//cJSON_AddNumberToObject(eSubItem, IPSO_V, curNode->netInfo.index);
				eSubItem = cJSON_CreateObject();
				cJSON_AddItemToArray(eItem, eSubItem);
				cJSON_AddStringToObject(eSubItem, IPSO_N, NET_INFO_STATUS);
				cJSON_AddStringToObject(eSubItem, IPSO_SV, curNode->netInfo.netStatus);
				eSubItem = cJSON_CreateObject();
				cJSON_AddItemToArray(eItem, eSubItem);
				cJSON_AddStringToObject(eSubItem, IPSO_N, NET_INFO_SPEED_MBPS);
				cJSON_AddNumberToObject(eSubItem, IPSO_V, curNode->netInfo.netSpeedMbps);
				eSubItem = cJSON_CreateObject();
				cJSON_AddItemToArray(eItem, eSubItem);
				cJSON_AddStringToObject(eSubItem, IPSO_N, NET_INFO_USAGE);
				cJSON_AddNumberToObject(eSubItem, IPSO_V, curNode->netInfo.netUsage);
				eSubItem = cJSON_CreateObject();
				cJSON_AddItemToArray(eItem, eSubItem);
				cJSON_AddStringToObject(eSubItem, IPSO_N, NET_INFO_SEND_BYTE);
				cJSON_AddNumberToObject(eSubItem, IPSO_V, curNode->netInfo.sendDateByte);
				eSubItem = cJSON_CreateObject();
				cJSON_AddItemToArray(eItem, eSubItem);
				cJSON_AddStringToObject(eSubItem, IPSO_N, NET_INFO_SEND_THROUGHPUT);
				cJSON_AddNumberToObject(eSubItem, IPSO_V, curNode->netInfo.sendThroughput);
				eSubItem = cJSON_CreateObject();
				cJSON_AddItemToArray(eItem, eSubItem);
				cJSON_AddStringToObject(eSubItem, IPSO_N, NET_INFO_RECV_BYTE);
				cJSON_AddNumberToObject(eSubItem, IPSO_V, curNode->netInfo.recvDateByte);
				eSubItem = cJSON_CreateObject();
				cJSON_AddItemToArray(eItem, eSubItem);
				cJSON_AddStringToObject(eSubItem, IPSO_N, NET_INFO_RECV_THROUGHPUT);
				cJSON_AddNumberToObject(eSubItem, IPSO_V, curNode->netInfo.recvThroughput);
			}
			else
			{
				curNode = curNode->next;
			}
		}
	}

	if(flag)
	{
		out = cJSON_PrintUnformatted(rootItem);
		outLen = strlen(out) + 1;
		*outputStr = (char *)(malloc(outLen));
		memset(*outputStr, 0, outLen);
		strcpy(*outputStr, out);
		cJSON_Delete(rootItem);	
		printf("%s\n",out);	
		free(out);
	}
	return outLen;
}

//int Parser_PackNMData(net_info_list netInfoList, char ** outputStr)
//{
//	return Parser_PackCapabilityStrRep(netInfoList, outputStr);
//}

int Parser_PackReportNMData(char * nmJsonStr, char * handlerName,char ** outputStr)
{
	char * out = NULL;
	int outLen = 0;
	cJSON *pSUSICommDataItem = NULL, * specInfoItem = NULL;
	if(nmJsonStr == NULL ||  handlerName == NULL || outputStr == NULL) return outLen;
	pSUSICommDataItem = cJSON_CreateObject();

	cJSON_AddItemToObject(pSUSICommDataItem, handlerName, cJSON_Parse(nmJsonStr));

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

bool Parser_ParseGetSensorDataReq(void* data, char * pSessionID)
{
	cJSON* root = NULL;
	cJSON* body = NULL;
	cJSON* params = NULL;

	if(!data || pSessionID==NULL) return false;

	root = cJSON_Parse((char *)data);
	if(!root) return false;

	body = cJSON_GetObjectItem(root, AGENTINFO_BODY_STRUCT);
	if(!body)
	{
		cJSON_Delete(root);
		return false;
	}

	params = cJSON_GetObjectItem(body, NETMON_SESSION_ID);
	if(params)
	{
		strcpy(pSessionID, params->valuestring);
	}
	cJSON_Delete(root);
	return true;
}


int Parser_PackGetSensorDataRep(sensor_info_list sensorInfoList, char * pSessionID, char** outputStr)
{
	char * out = NULL;
	int outLen = 0;
	cJSON *pSUSICommDataItem = NULL;
	if(sensorInfoList == NULL || outputStr == NULL || pSessionID == NULL) return outLen;
	pSUSICommDataItem = cJSON_CreateObject();

	cJSON_AddSensorInfoListToObject(pSUSICommDataItem, NETMON_SENSOR_INFO_LIST, sensorInfoList);
	cJSON_AddStringToObject(pSUSICommDataItem, NETMON_SESSION_ID, pSessionID);

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

	cJSON_AddStringToObject(pSUSICommDataItem, NETMON_SESSION_ID, pSessionID);
	cJSON_AddStringToObject(pSUSICommDataItem, NETMON_ERROR_REP, errorStr);

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

cJSON * Parser_PackThrItemInfo(nm_thr_item_t * pThrItemInfo)
{
	cJSON * infoItem = NULL;
	char pathStr[256] = {0};

	if(pThrItemInfo == NULL) return infoItem;
	{
		infoItem = cJSON_CreateObject();
		sprintf(pathStr, "%s/%s/%s/%s", DEF_HANDLER_NAME, NETMON_INFO_LIST, pThrItemInfo->adapterName, pThrItemInfo->tagName);

		cJSON_AddStringToObject(infoItem, NETMON_THR_N, pathStr);
		//cJSON_AddStringToObject(infoItem, NETMON_THR_N, pThrItemInfo->n);
		cJSON_AddNumberToObject(infoItem, NETMON_THR_MAX, pThrItemInfo->maxThr);
		cJSON_AddNumberToObject(infoItem, NETMON_THR_MIN, pThrItemInfo->minThr);
		cJSON_AddNumberToObject(infoItem, NETMON_THR_TYPE, pThrItemInfo->thrType);
		cJSON_AddNumberToObject(infoItem, NETMON_THR_LTIME, pThrItemInfo->lastingTimeS);
		cJSON_AddNumberToObject(infoItem, NETMON_THR_ITIME, pThrItemInfo->intervalTimeS);
		if(pThrItemInfo->isEnable)
		{
			cJSON_AddStringToObject(infoItem, NETMON_THR_ENABLE, "true");
		}
		else
		{
			cJSON_AddStringToObject(infoItem, NETMON_THR_ENABLE, "false");
		}
	}
	return infoItem;
}

cJSON * Parser_PackThrItemList(nm_thr_item_list thrList)
{
	cJSON * listItem = NULL;
	if(thrList == NULL) return listItem;
	{
		char curAdpName[DEF_ADP_NAME_LEN] = {0};
		nm_thr_item_node_t * curNode = thrList->next;
		listItem = cJSON_CreateArray();
		while(curNode)
		{
			cJSON * infoItem = Parser_PackThrItemInfo(&curNode->thrItem);
			if(infoItem)
			{
				cJSON_AddItemToArray(listItem, infoItem);
			}
			curNode = curNode->next;
		}
	}
	return listItem;
}

int Parser_PackThrInfo(nm_thr_item_list thrList, char ** outputStr)
{
	char * out = NULL;
	int outLen = 0;
	cJSON *root = NULL, *pSUSICommDataItem = NULL, * thrListItem = NULL;
	if(thrList == NULL || outputStr == NULL) return outLen;
	root = cJSON_CreateObject();
	pSUSICommDataItem = cJSON_CreateObject();
	cJSON_AddItemToObject(root, NETMON_JSON_ROOT_NAME, pSUSICommDataItem);
	thrListItem = Parser_PackThrItemList(thrList);
	if(thrListItem)
	{
		cJSON_AddItemToObject(pSUSICommDataItem, NETMON_THR, thrListItem);
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


bool Parser_ParseThrItemInfo(cJSON * jsonObj, nm_thr_item_t * pThrItemInfo, bool *nIsValidPnt)
{
	bool bRet = false;
	bool *nIsValid = nIsValidPnt;
	bool allThrFlag = false;

	if(jsonObj == NULL || pThrItemInfo == NULL) return bRet;
	{
		cJSON * pSubItem = NULL;
		pSubItem = jsonObj;
		if(pSubItem)
		{			
			pSubItem = cJSON_GetObjectItem(jsonObj, NETMON_THR_N);
			if(pSubItem)
			{
#ifdef WIN32
				/*
				//解决pSubItem->valuestring中“本地连接”显示乱码
				int len = 0;
				char *tmpPathStr = NULL;
				char *p = NULL;
				char * tempStr = NULL;
				//"本地连接"->"??????";  "AAAAA"->"AAAAA"
				tempStr=UTF8ToANSI(pSubItem->valuestring);
				len = strlen(tempStr)+1;
				p = (char *)malloc(len);
				memcpy(p, tempStr, len);
				memcpy(pSubItem->valuestring, p, len);
				free(p);
				free(tempStr);
				p = NULL;
				tempStr = NULL;
				*/
#endif
				strcpy(pThrItemInfo->n, pSubItem->valuestring);
			}
			else
			{
				pThrItemInfo->maxThr = DEF_INVALID_VALUE;
			}

			pThrItemInfo->isEnable = 0;
			pSubItem = cJSON_GetObjectItem(jsonObj, NETMON_THR_ENABLE);
			if(pSubItem)
			{
				if(!_stricmp(pSubItem->valuestring, "true"))
				{
					pThrItemInfo->isEnable = 1;
				}
			}

			pSubItem = cJSON_GetObjectItem(jsonObj, NETMON_THR_MAX);
			if(pSubItem)
			{
				pThrItemInfo->maxThr = (float)pSubItem->valuedouble;
			}
			else
			{
				pThrItemInfo->maxThr = DEF_INVALID_VALUE;
			}

			pSubItem = cJSON_GetObjectItem(jsonObj, NETMON_THR_MIN);
			if(pSubItem)
			{
				pThrItemInfo->minThr = (float)pSubItem->valuedouble;
			}
			else
			{
				pThrItemInfo->minThr = DEF_INVALID_VALUE;
			}

			pSubItem = cJSON_GetObjectItem(jsonObj, NETMON_THR_TYPE);
			if(pSubItem)
			{
				pThrItemInfo->thrType = pSubItem->valueint;
			}
			else
			{
				pThrItemInfo->thrType = DEF_THR_UNKNOW_TYPE;
			}

			pSubItem = cJSON_GetObjectItem(jsonObj, NETMON_THR_LTIME);
			if(pSubItem)
			{
				pThrItemInfo->lastingTimeS = pSubItem->valueint;
			}
			else
			{
				pThrItemInfo->lastingTimeS = DEF_INVALID_TIME;
			}

			pSubItem = cJSON_GetObjectItem(jsonObj, NETMON_THR_ITIME);
			if(pSubItem)
			{
				pThrItemInfo->intervalTimeS = pSubItem->valueint;
			}
			else
			{
				pThrItemInfo->intervalTimeS = DEF_INVALID_TIME;
			}

			//pThrItemInfo->checkRetValue.vi = DEF_INVALID_VALUE;
			pThrItemInfo->checkSrcValList.head = NULL;
			pThrItemInfo->checkSrcValList.nodeCnt = 0;
			pThrItemInfo->checkType = ck_type_avg;
			pThrItemInfo->isNormal = 1;
			pThrItemInfo->repThrTime = 0;
			pThrItemInfo->isValid = 1;

			bRet = true;
		}
	}
	*nIsValid = true;//此参数可删除;
	return bRet;
}

bool Parser_ParseThrInfo(char * thrJsonStr, net_info_list curNetInfoList, nm_thr_item_list thrList)
{
	bool bRet = false;
	if(thrJsonStr == NULL || thrList == NULL) return bRet;
	{
		cJSON * root = NULL;
		root = cJSON_Parse(thrJsonStr);
		if(root)
		{
			cJSON * commDataItem = cJSON_GetObjectItem(root, NETMON_JSON_ROOT_NAME);
			if(commDataItem)
			{
				cJSON * thrItem = NULL; 
				thrItem = cJSON_GetObjectItem(commDataItem, NETMON_THR);
				if(thrItem)
				{
					nm_thr_item_node_t * head = thrList;
					int nCount = cJSON_GetArraySize(thrItem);
					int i = 0;
					cJSON * subItem = NULL;
					for(i=0; i<nCount; i++)
					{
						subItem = cJSON_GetArrayItem(thrItem, i);
						if(subItem)
						{
							bool nIsValid = false;
							nm_thr_item_node_t * pThrItemNode = NULL;
							pThrItemNode = (nm_thr_item_node_t *)malloc(sizeof(nm_thr_item_node_t));
							memset(pThrItemNode, 0, sizeof(nm_thr_item_node_t));
							if(Parser_ParseThrItemInfo(subItem, &pThrItemNode->thrItem, &nIsValid))//in in out
							{
								if(nIsValid)
								{
									pThrItemNode->next = head->next;
									head->next = pThrItemNode;
								}
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
			}
		}
		cJSON_Delete(root);
	}
	return bRet;
}

int Parser_PackThrCheckRep(nm_thr_rep_info_t * pThrCheckRep, char ** outputStr)
{
	char * out = NULL;
	int outLen = 0;
	cJSON *pSUSICommDataItem = NULL;
	if(pThrCheckRep == NULL || outputStr == NULL) return outLen;
	pSUSICommDataItem = cJSON_CreateObject();

	if(pThrCheckRep->isTotalNormal)
	{
		cJSON_AddStringToObject(pSUSICommDataItem, NETMON_THR_CHECK_STATUS, "True");
	}
	else
	{
		cJSON_AddStringToObject(pSUSICommDataItem, NETMON_THR_CHECK_STATUS, "False");
	}
	if(pThrCheckRep->repInfo)
	{
		cJSON_AddStringToObject(pSUSICommDataItem, NETMON_THR_CHECK_MSG, pThrCheckRep->repInfo);
	}
	else
	{
		cJSON_AddStringToObject(pSUSICommDataItem, NETMON_THR_CHECK_MSG, "");
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

int Parser_PackSetThrRep(char * repStr, char ** outputStr)
{
	char * out = NULL;
	int outLen = 0;
	cJSON *pSUSICommDataItem = NULL;
	if(repStr == NULL || outputStr == NULL) return outLen;
	pSUSICommDataItem = cJSON_CreateObject();

	cJSON_AddStringToObject(pSUSICommDataItem, NETMON_SET_THR_REP, repStr);

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

	cJSON_AddStringToObject(pSUSICommDataItem, NETMON_DEL_ALL_THR_REP, repStr);

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
			params = cJSON_GetObjectItem(body, NETMON_SENSOR_ID_LIST);
			if(params)
			{
				cJSON * eItem = NULL;
				eItem = cJSON_GetObjectItem(params, NETMON_E_FLAG);
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
							valItem = cJSON_GetObjectItem(subItem, NETMON_N_FLAG);
							if(valItem)
							{
#ifdef WIN32
								/*//解决pathStr中“本地连接”显示乱码
								int len = 0;
								char *tmpPathStr = NULL;
								char *p = NULL;

								char * tempStr=UTF8ToANSI(valItem->valuestring);
								len = strlen(tempStr)+1;
								p = (char *)malloc(len);
								memcpy(p, tempStr, len);
								memcpy(valItem->valuestring, p, len);
								free(p);
								free(tempStr);
								p = NULL;
								tempStr = NULL;*/
#endif
								int len = strlen(valItem->valuestring) + 1;
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
					params = cJSON_GetObjectItem(body, NETMON_SESSION_ID);
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


bool Parser_GetSensorJsonStr(char *pNetInfoList, sensor_info_t * pSensorInfo)
{
	net_info_node_t * head = NULL;
	net_info_node_t * curNode = NULL;
	net_info_node_t ** ppNetInfoList = &head;
	bool bRet = true;
	bool flag = false;

	if(NULL == pNetInfoList || NULL == pSensorInfo) return bRet;
	memcpy(ppNetInfoList, pNetInfoList, sizeof(net_info_node_t *));

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
			cJSON_AddStringToObject(findVItem, NETMON_N_FLAG, pSensorInfo->pathStr);
		}

		if(i>0 && !strcmp(sliceStr[0], DEF_HANDLER_NAME) && !strcmp(sliceStr[1], NETMON_INFO_LIST))
		{
			curNode = head->next;
			while(curNode && !flag)
			{
				char adapterNameStr[256] = {0};
				strcpy(adapterNameStr, strstr(sliceStr[2], "-")+1);
				if(i>2 && !strcmp(adapterNameStr, curNode->netInfo.adapterName))
				{
					if(i>3)
					{

						if(!strcmp(sliceStr[3], NET_INFO_ADAPTER_NAME))
						{
							cJSON_AddStringToObject(findVItem, NETMON_SV_FLAG, curNode->netInfo.adapterName);
							cJSON_AddItemToObject(findVItem, NETMON_STATUS_CODE, cJSON_CreateNumber( 200 ));
							flag = true;
						}
						else if(!strcmp(sliceStr[3], NET_INFO_ADAPTER_DESCRI))
						{
							cJSON_AddStringToObject(findVItem, NETMON_SV_FLAG, curNode->netInfo.adapterDescription);
							cJSON_AddItemToObject(findVItem, NETMON_STATUS_CODE, cJSON_CreateNumber( 200 ));
							flag = true;
						}
						else if(!strcmp(sliceStr[3], NET_INFO_TYPE))
						{
							cJSON_AddStringToObject(findVItem, NETMON_SV_FLAG, curNode->netInfo.type);
							cJSON_AddItemToObject(findVItem, NETMON_STATUS_CODE, cJSON_CreateNumber( 200 ));
							flag = true;
						}
						else if(!strcmp(sliceStr[3], NET_INFO_INDEX))
						{
							//cJSON_AddStringToObject(findVItem, NETMON_V_FLAG, curNode->netInfo.index);//no
							//cJSON_AddItemToObject(findVItem, NETMON_V_FLAG, cJSON_CreateNumber(curNode->netInfo.index));//yes
							cJSON_AddNumberToObject(findVItem, NETMON_V_FLAG, curNode->netInfo.index);//yes
							cJSON_AddItemToObject(findVItem, NETMON_STATUS_CODE, cJSON_CreateNumber( 200 ));
							flag = true;
						}
						else if(!strcmp(sliceStr[3], NET_INFO_STATUS))
						{
							cJSON_AddStringToObject(findVItem, NETMON_SV_FLAG, curNode->netInfo.netStatus);
							cJSON_AddItemToObject(findVItem, NETMON_STATUS_CODE, cJSON_CreateNumber( 200 ));
							flag = true;
						}
						else if(!strcmp(sliceStr[3], NET_INFO_SPEED_MBPS))
						{
							cJSON_AddNumberToObject(findVItem, NETMON_V_FLAG, curNode->netInfo.netSpeedMbps);
							cJSON_AddItemToObject(findVItem, NETMON_STATUS_CODE, cJSON_CreateNumber( 200 ));
							flag = true;
						}
						else if(!strcmp(sliceStr[3], NET_INFO_USAGE))
						{
							cJSON_AddNumberToObject(findVItem, NETMON_V_FLAG, curNode->netInfo.netUsage);
							cJSON_AddItemToObject(findVItem, NETMON_STATUS_CODE, cJSON_CreateNumber( 200 ));
							flag = true;
						}
						else if(!strcmp(sliceStr[3], NET_INFO_SEND_BYTE))
						{
							cJSON_AddItemToObject(findVItem, NETMON_V_FLAG, cJSON_CreateNumber(curNode->netInfo.sendDateByte));
							cJSON_AddItemToObject(findVItem, NETMON_STATUS_CODE, cJSON_CreateNumber( 200 ));
							flag = true;
						}
						else if(!strcmp(sliceStr[3], NET_INFO_SEND_THROUGHPUT))
						{
							cJSON_AddItemToObject(findVItem, NETMON_V_FLAG, cJSON_CreateNumber(curNode->netInfo.sendThroughput));
							cJSON_AddItemToObject(findVItem, NETMON_STATUS_CODE, cJSON_CreateNumber( 200 ));
							flag = true;
						}
						else if(!strcmp(sliceStr[3], NET_INFO_RECV_BYTE))
						{
							cJSON_AddItemToObject(findVItem, NETMON_V_FLAG, cJSON_CreateNumber(curNode->netInfo.recvDateByte));
							cJSON_AddItemToObject(findVItem, NETMON_STATUS_CODE, cJSON_CreateNumber( 200 ));
							flag = true;
						}
						else if(!strcmp(sliceStr[3], NET_INFO_RECV_THROUGHPUT))
						{
							cJSON_AddItemToObject(findVItem, NETMON_V_FLAG, cJSON_CreateNumber(curNode->netInfo.recvThroughput));
							cJSON_AddItemToObject(findVItem, NETMON_STATUS_CODE, cJSON_CreateNumber( 200 ));
							flag = true;
						}	
						break;

					}
				}
				else
				{
					curNode = curNode->next;
				}
			}
		}

		if(!flag)
		{
			cJSON_Delete(findVItem);
			findVItem = NULL;
		}

		{
			char * tmpJsonStr = NULL;
			if(findVItem == NULL)
			{
				findVItem =  cJSON_CreateObject();
				cJSON_AddStringToObject(findVItem, NETMON_N_FLAG, pSensorInfo->pathStr);
				cJSON_AddStringToObject(findVItem, NETMON_SV_FLAG, "not found");
				cJSON_AddItemToObject(findVItem, NETMON_STATUS_CODE, cJSON_CreateNumber( 404 ));
			}
			tmpJsonStr = cJSON_PrintUnformatted(findVItem);
			len = strlen(tmpJsonStr) + 1;
			pSensorInfo->jsonStr = (char *)(malloc(len));
			memset(pSensorInfo->jsonStr, 0, len);
			strcpy(pSensorInfo->jsonStr, tmpJsonStr);	
			free(tmpJsonStr);
			cJSON_Delete(findVItem);
		}
		free(tmpPathStr);
	}
	return bRet=flag;
}


bool Parser_get_request_items_rep(net_info_list pNetInfoList, sensor_info_list sensorInfoList, char *isAutoUpdateAllNet, char **outputStr)
{
	BOOL bRet = false;
	bool flag = false;
	sensor_info_node_t * headSensor = sensorInfoList;
	sensor_info_node_t * curNode = NULL;
	sensor_info_node_t * sensorInfoListCurNode = NULL;	
	char * out = NULL;
	int outLen = 0;
	int j = 0;
	cJSON *pSUSICommDataItem = NULL;
	cJSON *pRoot = NULL;
	net_info_node_t * curNodeNetInfo = NULL;

	if(pNetInfoList == NULL || outputStr == NULL || sensorInfoList == NULL) return outLen;
	pSUSICommDataItem = cJSON_CreateObject();
	cJSON_AddItemToObject(pSUSICommDataItem, NET_MON_INFO_LIST, pRoot = cJSON_CreateObject());
	sensorInfoListCurNode = sensorInfoList->next;

	while(sensorInfoListCurNode)
	{
		sensor_info_t *pSensorInfo = &(sensorInfoListCurNode->sensorInfo);
		flag = FALSE;

		if(pSensorInfo->pathStr != NULL && strlen(pSensorInfo->pathStr))
		{
			int len = strlen(pSensorInfo->pathStr)+1;
			char *tmpPathStr = NULL;
			char * sliceStr[16] = {NULL};
			char * buf = NULL;
			int i = 0;
			cJSON * findVItem = NULL;
			cJSON *pSensorInfoListItem = NULL, *eItem = NULL;
			pSensorInfoListItem = cJSON_CreateObject();
			eItem = cJSON_CreateArray();

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
				if(i == 1)
				{
					isAutoUpdateAllNet = TRUE;
					bRet = TRUE;
				}
				else if(i == 2) 
				{
					if( !strcmp(sliceStr[1], NET_MON_INFO_LIST) )
					{
						isAutoUpdateAllNet = TRUE;
						bRet = TRUE;
					}
				}
				else if(i == 3 )
				{
					int temp = 0;
					temp = Parser_PackCapabilityStrRepNew(pNetInfoList, sliceStr[2], outputStr);
					if(temp > 0)
						bRet = true;
				}
				else if(i == 4) 
				{	
					cJSON * rootItem = NULL, * cpbItem = NULL;
					char * out = NULL;
					unsigned long tick = 0;
					cJSON *nmNodeItem = NULL, *eItem = NULL, *eSubItem = NULL;

					curNodeNetInfo = pNetInfoList->next;
					while(curNodeNetInfo && !flag)
					{
						if(!strcmp(sliceStr[2], curNodeNetInfo->netInfo.adapterName))							
						{
							rootItem = cJSON_CreateObject();
							cpbItem = cJSON_CreateArray();
							cJSON_AddItemToObject(rootItem, NETMON_INFO_LIST, cpbItem);
							nmNodeItem = cJSON_CreateObject();
							cJSON_AddItemToArray(cpbItem, nmNodeItem);
							cJSON_AddStringToObject(nmNodeItem, IPSO_BN, curNodeNetInfo->netInfo.adapterName);
							tick = (unsigned long)time((time_t *) NULL);
							cJSON_AddNumberToObject(nmNodeItem, IPSO_BT, tick);
							cJSON_AddStringToObject(nmNodeItem, IPSO_BU, "");
							cJSON_AddNumberToObject(nmNodeItem, IPSO_VER, DEF_IPSO_VER);

							eItem = cJSON_CreateArray();
							cJSON_AddItemToObject(nmNodeItem, IPSO_E, eItem);
							eSubItem = cJSON_CreateObject();
							cJSON_AddItemToArray(eItem, eSubItem);

							if(!strcmp(sliceStr[3], NET_INFO_ADAPTER_NAME))
							{
								cJSON_AddStringToObject(eSubItem, IPSO_N, NET_INFO_ADAPTER_NAME);
								cJSON_AddStringToObject(eSubItem, IPSO_SV, curNodeNetInfo->netInfo.adapterName);
								flag = true;
							}
							else if(!strcmp(sliceStr[3], NET_INFO_ADAPTER_DESCRI))
							{
								cJSON_AddStringToObject(eSubItem, IPSO_N, NET_INFO_ADAPTER_DESCRI);
								cJSON_AddStringToObject(eSubItem, IPSO_SV, curNodeNetInfo->netInfo.adapterDescription);
								flag = true;
							}
							else if(!strcmp(sliceStr[3], NET_INFO_TYPE))
							{
								cJSON_AddStringToObject(eSubItem, IPSO_N, NET_INFO_TYPE);
								cJSON_AddStringToObject(eSubItem, IPSO_SV, curNodeNetInfo->netInfo.type);
								flag = true;
							}
							else if(!strcmp(sliceStr[3], NET_INFO_INDEX))
							{
								cJSON_AddStringToObject(eSubItem, IPSO_N, NET_INFO_INDEX);
								cJSON_AddNumberToObject(eSubItem, IPSO_V, curNodeNetInfo->netInfo.index);
								flag = true;
							}
							else if(!strcmp(sliceStr[3], NET_INFO_STATUS))
							{
								cJSON_AddStringToObject(eSubItem, IPSO_N, NET_INFO_STATUS);
								cJSON_AddStringToObject(eSubItem, IPSO_SV, curNodeNetInfo->netInfo.netStatus);
								flag = true;
							}
							else if(!strcmp(sliceStr[3], NET_INFO_SPEED_MBPS))
							{
								cJSON_AddStringToObject(eSubItem, IPSO_N, NET_INFO_SPEED_MBPS);
								cJSON_AddNumberToObject(eSubItem, IPSO_V, curNodeNetInfo->netInfo.netSpeedMbps);
								flag = true;
							}
							else if(!strcmp(sliceStr[3], NET_INFO_USAGE))
							{
								cJSON_AddStringToObject(eSubItem, IPSO_N, NET_INFO_USAGE);
								cJSON_AddNumberToObject(eSubItem, IPSO_V, curNodeNetInfo->netInfo.netUsage);
								flag = true;
							}
							else if(!strcmp(sliceStr[3], NET_INFO_SEND_BYTE))
							{
								cJSON_AddStringToObject(eSubItem, IPSO_N, NET_INFO_SEND_BYTE);
								cJSON_AddNumberToObject(eSubItem, IPSO_V, curNodeNetInfo->netInfo.sendDateByte);
								flag = true;
							}
							else if(!strcmp(sliceStr[3], NET_INFO_SEND_THROUGHPUT))
							{
								cJSON_AddStringToObject(eSubItem, IPSO_N, NET_INFO_SEND_THROUGHPUT);
								cJSON_AddNumberToObject(eSubItem, IPSO_V, curNodeNetInfo->netInfo.sendThroughput);
								flag = true;
							}
							else if(!strcmp(sliceStr[3], NET_INFO_RECV_BYTE))
							{
								cJSON_AddStringToObject(eSubItem, IPSO_N, NET_INFO_RECV_BYTE);
								cJSON_AddNumberToObject(eSubItem, IPSO_V, curNodeNetInfo->netInfo.recvDateByte);
								flag = true;
							}
							else if(!strcmp(sliceStr[3], NET_INFO_RECV_THROUGHPUT))
							{
								cJSON_AddStringToObject(eSubItem, IPSO_N, NET_INFO_RECV_THROUGHPUT);
								cJSON_AddNumberToObject(eSubItem, IPSO_V, curNodeNetInfo->netInfo.recvThroughput);
								flag = true;
							}	
							break;
						}
						else
						{
							curNodeNetInfo = curNodeNetInfo->next;
						}
					}

					if(!flag)
					{
						bRet = false;
					}
					else
					{
						out = cJSON_PrintUnformatted(rootItem);
						outLen = strlen(out) + 1;
						*outputStr = (char *)(malloc(outLen));
						memset(*outputStr, 0, outLen);
						strcpy(*outputStr, out);
						printf("%s\n", out);
						cJSON_Delete(rootItem);	
						free(out);
						//return outLen;
						bRet = true;
					}
				}
				else
				{
					bRet = false;
				}
			}

		}
		sensorInfoListCurNode = sensorInfoListCurNode->next;
	}
	return bRet;
}