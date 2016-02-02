#include "parser.h"
#include "common.h"
#define DEF_THRESHOLD_CONFIG					"HWMThresholdCfg"

bool parser_ParseReceivedData(void* data, int datalen, int * cmdID)
{
	/*{"susiCommData":{"commCmd":251,"catalogID":4,"requestID":10}}*/

	cJSON* root = NULL;
	cJSON* body = NULL;
	cJSON* target = NULL;

	if(!data) return false;
	if(datalen<=0) return false;
	//MonitorLog(g_loghandle, Normal, " %s>Parser_ParseReceivedData [%s]\r\n", MyTopic, data );
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

bool parser_ParsePeriodReportCmd(void* data, int datalen, int * interval, int * timeout)
{
	/*{"susiCommData":{"commCmd":5,"catalogID":4,"requestID":10,"autoUploadParams":{"autoUploadIntervalMs":1000, "autoUploadTimeoutMs":10000}}}*/

	cJSON* root = NULL;
	cJSON* body = NULL;
	cJSON* params = NULL;
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

	params = cJSON_GetObjectItem(body, HWM_AUTOUPLOAD_PARAM);
	if(params)
	{
		target = cJSON_GetObjectItem(params, HWM_AUTOUPLOAD_INTERVAL_MS);
		if(target)
		{
			*interval = target->valueint;
		}

		target = cJSON_GetObjectItem(params, HWM_AUTOUPLOAD_TIMEOUT_MS);
		if(target)
		{
			*timeout = target->valueint;
		}
	}
	cJSON_Delete(root);
	return true;
}


cJSON * parser_CreatePlatformInfo(hwm_info_t * pHWMInfo)
{
	/*
{"pfInfo":{"volt":[{"min":-500,"unit":"V","max":500,"tag":"vbatV","display":"CMOS Battery","type":3},{"min":-500,"unit":"V","max":500,"tag":"vsb5V","display":"5V Standby","type":3},{"min":-500,"unit":"V","max":500,"tag":"v12V","display":"12V","type":3}],"temp":[{"min":-100,"unit":"Celsius","max":100,"tag":"systemT","display":"System","type":1}],"fan":[{"min":0,"unit":"RPM","max":90000,"tag":"systemF","display":"System","type":2},{"min":0,"unit":"RPM","max":90000,"tag":"cpuF","display":"CPU","type":2}]},"requestID":103,"commCmd":252}
	*/
   //cJSON* pGlobalInfoHead = NULL;
   cJSON* root = NULL;
   cJSON *pAgentInfoHead = NULL;
   cJSON* pVoltList = NULL;
   cJSON* pTempList = NULL;
   cJSON* pFanList = NULL;
   cJSON* pCurrentList = NULL;
   cJSON* pCaseOPList = NULL;
   cJSON* pOtherList = NULL;
   hwm_item_t *item = NULL;
   //hwm_item_t *target = NULL;

   if(!pHWMInfo) return NULL;
   item = pHWMInfo->items;

   //printf("Find Topic\r\n");
   while(item != NULL)
   {
	   cJSON* pNode = cJSON_CreateObject();
	   cJSON_AddNumberToObject(pNode, HWM_PFI_MIN_THSHD, item->minThreshold);
	   cJSON_AddStringToObject(pNode, HWM_PFI_UNIT, item->unit);
	   cJSON_AddNumberToObject(pNode, HWM_PFI_MAX_THSHD, item->maxThreshold);
	   cJSON_AddStringToObject(pNode, HWM_PFI_TAG_NAME, item->tag);
	   cJSON_AddStringToObject(pNode, HWM_PFI_DISPLAY_NAME, item->name);
	   cJSON_AddNumberToObject(pNode, HWM_PFI_THSHD_TYPE, item->thresholdType);
	   if(strcmp(item->type, DEF_SENSORTYPE_TEMPERATURE) == 0)
	   {
		   if(!pTempList)
				pTempList = cJSON_CreateArray();
		   cJSON_AddItemToArray(pTempList, pNode);
	   }
	   else if(strcmp(item->type, DEF_SENSORTYPE_VOLTAGE) == 0)
	   {
		   if(!pVoltList)
			   pVoltList = cJSON_CreateArray();
		   cJSON_AddItemToArray(pVoltList, pNode);
	   }
	   else if(strcmp(item->type, DEF_SENSORTYPE_FANSPEED) == 0)
	   {
		   if(!pFanList)
			   pFanList = cJSON_CreateArray();
		   cJSON_AddItemToArray(pFanList, pNode);
	   }
	   else if(strcmp(item->type, DEF_SENSORTYPE_CURRENT) == 0)
	   {
		   if(!pCurrentList)
			   pCurrentList = cJSON_CreateArray();
		   cJSON_AddItemToArray(pCurrentList, pNode);
	   }
	   else if(strcmp(item->type, DEF_SENSORTYPE_CASEOPEN) == 0)
	   {
		   if(!pCaseOPList)
			   pCaseOPList = cJSON_CreateArray();
		   cJSON_AddItemToArray(pCaseOPList, pNode);
	   }
	   else
	   {
		   if(!pOtherList)
			   pOtherList = cJSON_CreateArray();
		   cJSON_AddItemToArray(pOtherList, pNode);
	   }
	   item = item->next;
   }
   
   root = cJSON_CreateObject();
   pAgentInfoHead = cJSON_CreateObject();
   cJSON_AddItemToObject(root, HWM_PLATFORM_INFO, pAgentInfoHead);
   if(pTempList)
	   cJSON_AddItemToObject(pAgentInfoHead, HWM_TEMP_PFI_LIST, pTempList);
   if(pVoltList)
	   cJSON_AddItemToObject(pAgentInfoHead, HWM_VOLT_PFI_LIST, pVoltList);
   if(pFanList)
	   cJSON_AddItemToObject(pAgentInfoHead, HWM_FAN_PFI_LIST, pFanList);
   if(pCurrentList)
	   cJSON_AddItemToObject(pAgentInfoHead, HWM_CURRENT_PFI_LIST, pCurrentList);
   if(pCaseOPList)
	   cJSON_AddItemToObject(pAgentInfoHead, HWM_CASEOP_PFI_LIST, pCaseOPList);
   if(pOtherList)
	   cJSON_AddItemToObject(pAgentInfoHead, HWM_OTHERS_PFI_LIST, pOtherList);

   return root;
}

cJSON * parser_CreateHWMInfo(hwm_info_t * pHWMInfo)
{
	/*
{"hwmInfo":{"volt":[{"min":-500,"unit":"V","max":500,"tag":"vbatV","display":"CMOS Battery","type":3},{"min":-500,"unit":"V","max":500,"tag":"vsb5V","display":"5V Standby","type":3},{"min":-500,"unit":"V","max":500,"tag":"v12V","display":"12V","type":3}],"temp":[{"min":-100,"unit":"Celsius","max":100,"tag":"systemT","display":"System","type":1}],"fan":[{"min":0,"unit":"RPM","max":90000,"tag":"systemF","display":"System","type":2},{"min":0,"unit":"RPM","max":90000,"tag":"cpuF","display":"CPU","type":2}]},"requestID":103,"commCmd":252}
	*/
   cJSON* root = NULL;
   cJSON *pAgentInfoHead = NULL;
   cJSON* pVoltList = NULL;
   cJSON* pTempList = NULL;
   cJSON* pFanList = NULL;
   cJSON* pCurrentList = NULL;
   cJSON* pCaseOPList = NULL;
   cJSON* pOtherList = NULL;
   hwm_item_t *item = NULL;

   if(!pHWMInfo) return NULL;
   item = pHWMInfo->items;

   //printf("Find Topic\r\n");
   while(item != NULL)
   {
	   cJSON* pNode = NULL;
	   if(strcmp(item->type, DEF_SENSORTYPE_TEMPERATURE) == 0)
	   {
		   if(!pTempList)
				pTempList = cJSON_CreateObject();	
		   pNode = pTempList;
	   }
	   else if(strcmp(item->type, DEF_SENSORTYPE_VOLTAGE) == 0)
	   {
		   if(!pVoltList)
			   pVoltList = cJSON_CreateObject();
		   pNode = pVoltList;
	   }
	   else if(strcmp(item->type, DEF_SENSORTYPE_FANSPEED) == 0)
	   {
		   if(!pFanList)
			   pFanList = cJSON_CreateObject();
		   pNode = pFanList;
	   }
	   else if(strcmp(item->type, DEF_SENSORTYPE_CURRENT) == 0)
	   {
		   if(!pCurrentList)
			   pCurrentList = cJSON_CreateObject();
		   pNode = pCurrentList;
	   }
	   else if(strcmp(item->type, DEF_SENSORTYPE_CASEOPEN) == 0)
	   {
		   if(!pCaseOPList)
			   pCaseOPList = cJSON_CreateObject();
		  pNode = pCaseOPList;
	   }
	   else
	   {
		   if(!pOtherList)
			   pOtherList = cJSON_CreateObject();
		   pNode = pOtherList;
	   }
	   cJSON_AddNumberToObject(pNode, item->tag, item->value);
	   item = item->next;
   }
   
   root = cJSON_CreateObject();
   pAgentInfoHead = cJSON_CreateObject();
   cJSON_AddItemToObject(root, HWM_HWM_INFO, pAgentInfoHead);
   if(pTempList)
	   cJSON_AddItemToObject(pAgentInfoHead, HWM_TEMP_PFI_LIST, pTempList);
   if(pVoltList)
	   cJSON_AddItemToObject(pAgentInfoHead, HWM_VOLT_PFI_LIST, pVoltList);
   if(pFanList)
	   cJSON_AddItemToObject(pAgentInfoHead, HWM_FAN_PFI_LIST, pFanList);
   if(pCurrentList)
	   cJSON_AddItemToObject(pAgentInfoHead, HWM_CURRENT_PFI_LIST, pCurrentList);
   if(pCaseOPList)
	   cJSON_AddItemToObject(pAgentInfoHead, HWM_CASEOP_PFI_LIST, pCaseOPList);
   if(pOtherList)
	   cJSON_AddItemToObject(pAgentInfoHead, HWM_OTHERS_PFI_LIST, pOtherList);

   return root;
}

bool parser_ParseThresholdCmd(void* data, int datalen, thr_items_t* itemlist)
{
	/*{"susiCommData":{"commCmd":257,"catalogID":4,"requestID":10,"hwmThr":{"temp":[{"enable":"true","tag":"V131072","min":-100,"max":100,"type":3,"lastingTimeS":1,"intervalTimeS":1}],"volt":[],"fan":[],"hdd":[]}}}*/

	cJSON* root = NULL;
	cJSON* body = NULL;
	cJSON* params = NULL;
	cJSON* catalog = NULL;
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

	params = cJSON_GetObjectItem(body, HWM_THR_INFO);
	if(!params)
	{
		cJSON_Delete(root);
		return false;
	}

	catalog = params->child;

	while(catalog)
	{
		char tag[DEF_TAG_NAME_LEN] = {0};
		double max = 0;
		double min = 0;
		bool enable = false;
		int type = 0;
		int lasting = DEF_INVALID_TIME;
		int interval = DEF_INVALID_TIME;
		sa_thr_item_t* item = NULL; 
		int i;

		for (i = 0 ; i < cJSON_GetArraySize(catalog) ; i++)
		{
			cJSON * subitem = cJSON_GetArrayItem(catalog, i);

			target = cJSON_GetObjectItem(subitem, HWM_PFI_TAG_NAME);
			if(target)
			{
				strncpy(tag, target->valuestring, strlen(target->valuestring)+1);
			}
			else 
			{
				continue;
			}

			target = cJSON_GetObjectItem(subitem, HWM_PFI_MAX_THSHD);
			if(target)
			{
				max = target->valuedouble;
			}

			target = cJSON_GetObjectItem(subitem, HWM_PFI_MIN_THSHD);
			if(target)
			{
				min = target->valuedouble;
			}

			target = cJSON_GetObjectItem(subitem, HWM_THR_ENABLE);
			if(target)
			{
				if(!_stricmp(target->valuestring, "true"))
					enable = true;
				else
					enable = false;
			}

			target = cJSON_GetObjectItem(subitem, HWM_PFI_THSHD_TYPE);
			if(target)
			{
				type = target->valueint;
			}

			target = cJSON_GetObjectItem(subitem, HWM_THR_LASTING_TIME_S);
			if(target)
			{
				lasting = target->valueint;
			}

			target = cJSON_GetObjectItem(subitem, HWM_THR_INTERVAL_TIME_S);
			if(target)
			{
				interval = target->valueint;
			}

			if (!_stricmp(catalog->string, HWM_HDD_THR_LIST)) //HDDThrList will created separately
			{
				//=== for HDDHandler Only, modified by tang.tao ===Start===
				int ruleIndex = DEF_INVALID_VALUE; //Index of sa_thr_item[2], it will be set to 0 or 1 if rule exists, otherwise will remain -999
				if (!_stricmp(tag, THR_HEALTH_TAG)) ruleIndex = 0;
				if (!_stricmp(tag, THR_TEMPH_TAG))	ruleIndex = 1;
				if (DEF_INVALID_VALUE == ruleIndex) {
					printf("encounter a undefined rule which tag = \"%s\", HDD Threshold rule tag should be \"healthH\" or \"tempH\".\r\n", tag);
					cJSON_Delete(root);
					return false;
				}
				else if (!threshold_LoadOneRule(&itemlist->hddRule->hddThrItem.thrItem[ruleIndex], tag, (float)max, (float)min, type, enable, lasting, interval)) {
					printf(">Load threshold  rule [%d] failed!", ruleIndex);
					cJSON_Delete(root);
					return false;
				}
			}
			else {
				item = threshold_AddThresholdItem(itemlist, tag, (float)max, (float)min, type, enable, lasting, interval, NULL);
				if (item == NULL)
				{
					cJSON_Delete(root);
					return false;
				}
			}
		}// for End
		catalog = catalog->next;
	}// while End

	cJSON_Delete(root);
	return true;
}

cJSON * parser_CreateThresholdCheckResult(bool bResult, char* msg)
{
	/*
{"normalStatus":"False", "monEventMsg":"tag(type:value)>maxThreshold(rule) and tag(type:value)<minThreshold(rule);...", requestID":103,"commCmd":254}
{"normalStatus":"True", "monEventMsg":"tag(type:value) normal!;...", requestID":103,"commCmd":254}
	*/
   cJSON* root = NULL;
   //cJSON* pOtherList = NULL;

   root = cJSON_CreateObject();

   if(bResult)
   {
	   cJSON_AddStringToObject(root, HWM_NORMAL_STATUS, "True");
   }
   else
   {
	   cJSON_AddStringToObject(root, HWM_NORMAL_STATUS, "False");
   }

   if(msg)
   {
	   cJSON_AddStringToObject(root, HWM_MON_EVENT_MSG, msg);
   }

   return root;
}

bool parser_ReadThresholdConfig(char const * modulepath, thr_items_t* rules)
{
	bool bRet = false;
	FILE *fp;
	char *pTmpThrInfoStr = NULL;
	unsigned int fileLen = 0;
	char thrpath[MAX_PATH] = {0};

	if(modulepath == NULL || rules == NULL) return bRet;
	path_combine(thrpath, modulepath, DEF_THRESHOLD_CONFIG);
	//sprintf_s(thrpath, sizeof(thrpath), "%s%s", modulepath, DEF_THRESHOLD_CONFIG);
	printf(">Read Threshold config FilePath: %s.\r\n", thrpath);
	if ((fp = fopen(thrpath, "rb")) == NULL) 
	{
		printf(" >Failed to open %s for reading.\r\n", thrpath);
		return bRet;
	}
	fseek(fp, 0, SEEK_END);
	fileLen = ftell(fp);
	if(fileLen > 0)
	{
		unsigned int readLen = fileLen + 1, realReadLen = 0;
		pTmpThrInfoStr = (char *) malloc(readLen);
		memset(pTmpThrInfoStr, 0, readLen);
		fseek(fp, 0, SEEK_SET);
		realReadLen =  fread(pTmpThrInfoStr, sizeof(char), readLen, fp);
		if(realReadLen > 0)
		{
			bRet = parser_ParseThresholdCmd(pTmpThrInfoStr, realReadLen, rules);
		}
		if(pTmpThrInfoStr) free(pTmpThrInfoStr);
	}

	fclose(fp);
	return bRet;
}

bool parser_WriteThresholdConfig(char const * modulepath, char const * rules)
{
	FILE *fp;
	char thrpath[MAX_PATH] = {0};

	if(modulepath == NULL  ) return false;
	path_combine(thrpath, modulepath, DEF_THRESHOLD_CONFIG);
	//sprintf_s(thrpath, sizeof(thrpath), "%s%s", modulepath, DEF_THRESHOLD_CONFIG);
	printf(">Write Threshold config FilePath: %s.\r\n", thrpath);

	fp = fopen(thrpath, "w");
	if (fp == NULL)
	{
		printf(" > Failed to open %s for writing.\r\n", thrpath);
		return false;
	}

	fputs(rules, fp);
	fclose(fp);
	return true;
}

cJSON * parser_CreateHDDInfo(hdd_info_t * pHDDInfo)
{
	/*
{"requestID":103,"hddInfo":{"hddMonInfoList":[{"hddName":"Disk0-SQF-S25M8-128G-S5C","averageProgram":1376,"powerOnTime":11074,"hddType":1,"enduranceCheck":73,"goodBlockRate":100,"maxReservedBlock":164,"currentReservedBlock":104,"eccCount":0,"hddHealthPercent":73,"maxProgram":5000,"hddTemp":0,"hddIndex":0}]},"commCmd":8}
	*/
   cJSON* root = NULL;
   cJSON *pAgentInfoHead = NULL;
   cJSON* pHDDList = NULL;

   hdd_mon_info_node_t *item = NULL;

   if(!pHDDInfo) return NULL;

   item = pHDDInfo->hddMonInfoList->next;

   //printf("Find Topic\r\n");
   while(item != NULL)
   {
	   hdd_mon_info_t * hddMonInfo = &item->hddMonInfo;
	   cJSON* pNode = cJSON_CreateObject();
	   char strName[32] = {0};
	   sprintf(strName,"Disk%d-%s",hddMonInfo->hdd_index,hddMonInfo->hdd_name);
	   cJSON_AddNumberToObject(pNode, HDD_TYPE, hddMonInfo->hdd_type);
	   cJSON_AddStringToObject(pNode, HDD_NAME, strName);
	   cJSON_AddNumberToObject(pNode, HDD_INDEX, hddMonInfo->hdd_index); 
	   if (hddMonInfo->hdd_temp               != DEF_INVALID_VALUE) cJSON_AddNumberToObject(pNode, HDD_TEMP,               hddMonInfo->hdd_temp              );
	   if (hddMonInfo->hdd_health_percent     != DEF_INVALID_VALUE) cJSON_AddNumberToObject(pNode, HDD_HEALTH_PERCENT,     hddMonInfo->hdd_health_percent    );
	   if (hddMonInfo->power_on_time          != DEF_INVALID_VALUE) cJSON_AddNumberToObject(pNode, POWER_ON_TIME,          hddMonInfo->power_on_time         );
	   if (hddMonInfo->ecc_count              != DEF_INVALID_VALUE) cJSON_AddNumberToObject(pNode, ECC_COUNT,              hddMonInfo->ecc_count             );
	   if (hddMonInfo->endurance_check        != DEF_INVALID_VALUE) cJSON_AddNumberToObject(pNode, ENDURANCE_CHECK,        hddMonInfo->endurance_check       );
	   if (hddMonInfo->good_block_rate        != DEF_INVALID_VALUE) cJSON_AddNumberToObject(pNode, GOOD_BLOCK_RATE,        hddMonInfo->good_block_rate       );
	   if (hddMonInfo->max_reserved_block     != DEF_INVALID_VALUE) cJSON_AddNumberToObject(pNode, MAX_RESERVED_BLOCK,     hddMonInfo->max_reserved_block    );
	   if (hddMonInfo->current_reserved_block != DEF_INVALID_VALUE) cJSON_AddNumberToObject(pNode, CURRENT_RESERVED_BLOCK, hddMonInfo->current_reserved_block);
	   if (hddMonInfo->average_program        != DEF_INVALID_VALUE) cJSON_AddNumberToObject(pNode, AVERAGE_PROGRAM,        hddMonInfo->average_program       );
	   if (hddMonInfo->max_program            != DEF_INVALID_VALUE) cJSON_AddNumberToObject(pNode, MAX_PROGRAM,            hddMonInfo->max_program           );

	   
	   if(!pHDDList)
		   pHDDList = cJSON_CreateArray();
	   cJSON_AddItemToArray(pHDDList, pNode);

	   item = item->next;
   }
   
   root = cJSON_CreateObject();
   pAgentInfoHead = cJSON_CreateObject();
   cJSON_AddItemToObject(root, HDD_INFO, pAgentInfoHead);
   if(pHDDList)
	   cJSON_AddItemToObject(pAgentInfoHead, HDD_MON_INFO_LIST, pHDDList);

   return root;
}

cJSON * CreateSmartAttriInfo(smart_attri_info_t * pSmartAttriInfo)
{
	cJSON * pSmartAttriInfoItem = NULL;
	char vendorDataStr[16] = {0};
	if(!pSmartAttriInfo) return NULL;
	pSmartAttriInfoItem = cJSON_CreateObject();
	cJSON_AddStringToObject(pSmartAttriInfoItem, SMART_ATTRI_NAME, pSmartAttriInfo->attriName);
	cJSON_AddNumberToObject(pSmartAttriInfoItem, SMART_ATTRI_TYPE, pSmartAttriInfo->attriType);
	cJSON_AddNumberToObject(pSmartAttriInfoItem, SMART_ATTRI_FLAGS, pSmartAttriInfo->attriFlags);
	cJSON_AddNumberToObject(pSmartAttriInfoItem, SMART_ATTRI_WORST, pSmartAttriInfo->attriWorst);
	cJSON_AddNumberToObject(pSmartAttriInfoItem, SMART_ATTRI_THRESH, pSmartAttriInfo->attriThresh);
	cJSON_AddNumberToObject(pSmartAttriInfoItem, SMART_ATTRI_VALUE, pSmartAttriInfo->attriValue);
	sprintf_s(vendorDataStr, sizeof(vendorDataStr), "%02X%02X%02X%02X%02X%02X", (unsigned char)pSmartAttriInfo->attriVendorData[5],
		(unsigned char)pSmartAttriInfo->attriVendorData[4], (unsigned char)pSmartAttriInfo->attriVendorData[3],
		(unsigned char)pSmartAttriInfo->attriVendorData[2],(unsigned char)pSmartAttriInfo->attriVendorData[1],
		(unsigned char)pSmartAttriInfo->attriVendorData[0]);
	cJSON_AddStringToObject(pSmartAttriInfoItem, SMART_ATTRI_VENDOR, vendorDataStr);
	return pSmartAttriInfoItem;
}

cJSON * parser_CreateHDDSMART(hdd_info_t * pHDDInfo)
{
	/*
{"requestID":103,"commCmd":276,"hddSmartInfoList":[{"hddName":"Disk0-SQF-S25M8-128G-S5C","smartAttrList":[],"hddType":1,"hddIndex":0}]}
	*/
   cJSON* root = NULL;
   //cJSON *pAgentInfoHead = NULL;
   cJSON* pHDDList = NULL;

   hdd_mon_info_node_t *item = NULL;

   if(!pHDDInfo) return NULL;

   item = pHDDInfo->hddMonInfoList->next;

   //printf("Find Topic\r\n");
   while(item != NULL)
   {
	   cJSON *pSmartAttriInfoListItem = NULL;
	   hdd_mon_info_t * hddMonInfo = &item->hddMonInfo;
	   cJSON* pSmartInfoItem = cJSON_CreateObject();
	   char strName[32] = {0};
	   sprintf(strName,"Disk%d-%s",hddMonInfo->hdd_index,hddMonInfo->hdd_name);
	   cJSON_AddNumberToObject(pSmartInfoItem, HDD_TYPE, hddMonInfo->hdd_type);
	   cJSON_AddStringToObject(pSmartInfoItem, HDD_NAME, strName);
	   cJSON_AddNumberToObject(pSmartInfoItem, HDD_INDEX, hddMonInfo->hdd_index);
	   
	   pSmartAttriInfoListItem = cJSON_CreateArray();
	   cJSON_AddItemToObject(pSmartInfoItem, SMART_ATTRI_INFO_LIST, pSmartAttriInfoListItem);
	   if(hddMonInfo->smartAttriInfoList)
	   {
		   smart_attri_info_node_t * curSmartAttriInfoNode = hddMonInfo->smartAttriInfoList->next;
		   while(curSmartAttriInfoNode)
		   {
			   cJSON * pSmartAttriInfoItem = CreateSmartAttriInfo(&curSmartAttriInfoNode->attriInfo);
			   cJSON_AddItemToArray(pSmartAttriInfoListItem, pSmartAttriInfoItem);
			   curSmartAttriInfoNode = curSmartAttriInfoNode->next;
		   }
	   }

	   if(!pHDDList)
		   pHDDList = cJSON_CreateArray();
	   cJSON_AddItemToArray(pHDDList, pSmartInfoItem);

	   item = item->next;
   }
   
   root = cJSON_CreateObject(); 
   if(pHDDList)
	   cJSON_AddItemToObject(root, HDD_SMART_INFO_LIST, pHDDList);

   return root;
}

cJSON * CreateInfoSpecEventNode(char const * name, char const * unit, char const * tag, char const * type, int min, int max, int thresholdtype)
{
	cJSON* pNode = cJSON_CreateObject();
	char sVal[64] = {0};

	if(name)
		cJSON_AddStringToObject(pNode, HWM_SPEC_NAME,name);
	if(unit)
		cJSON_AddStringToObject(pNode, HWM_PFI_UNIT, unit);
	if(tag)
		cJSON_AddStringToObject(pNode, HWM_SPEC_BASENAME, tag);
	if(type)
	cJSON_AddStringToObject(pNode, HWM_SPEC_TYPE, type);

	cJSON_AddNumberToObject(pNode, HWM_SPEC_MIN, min);
	cJSON_AddNumberToObject(pNode, HWM_SPEC_MAX, max);
	
	switch (thresholdtype)
	{
	default:
	case DEF_THR_UNKNOW_TYPE:
		cJSON_AddStringToObject(pNode, HWM_SPEC_THRESHOLD_TYPE, TYPE_THRESHOLD_UNKNOW);
		break;
	case DEF_THR_MAX_TYPE:
		cJSON_AddStringToObject(pNode, HWM_SPEC_THRESHOLD_TYPE, TYPE_THRESHOLD_MAX);
		break;
	case DEF_THR_MIN_TYPE:
		cJSON_AddStringToObject(pNode, HWM_SPEC_THRESHOLD_TYPE, TYPE_THRESHOLD_MIN);
		break;
	case DEF_THR_MAXMIN_TYPE:
		cJSON_AddStringToObject(pNode, HWM_SPEC_THRESHOLD_TYPE, TYPE_THRESHOLDTHR_MAXMIN);
		break;
	case DEF_THR_BOOL_TYPE:
		cJSON_AddStringToObject(pNode, HWM_SPEC_THRESHOLD_TYPE, TYPE_THRESHOLDTHR_BOOL);
		break;
	}

	return pNode;
}

cJSON * CreateHDDInfoSpecEventNode(char const * name, char const * unit, char const * tag, char const * type, int min, int max, int thresholdtype, char const * diskname)
{
	cJSON* pNode = cJSON_CreateObject();
	char sVal[64] = {0};

	if(name)
		cJSON_AddStringToObject(pNode, HWM_SPEC_NAME,name);
	if(unit)
		cJSON_AddStringToObject(pNode, HWM_PFI_UNIT, unit);
	if(tag)
		cJSON_AddStringToObject(pNode, HWM_SPEC_BASENAME, tag);
	if(type)
		cJSON_AddStringToObject(pNode, HWM_SPEC_TYPE, type);

	cJSON_AddNumberToObject(pNode, HWM_SPEC_MIN, min);
	cJSON_AddNumberToObject(pNode, HWM_SPEC_MAX, max);

	switch (thresholdtype)
	{
	default:
	case DEF_THR_UNKNOW_TYPE:
		cJSON_AddStringToObject(pNode, HWM_SPEC_THRESHOLD_TYPE, TYPE_THRESHOLD_UNKNOW);
		break;
	case DEF_THR_MAX_TYPE:
		cJSON_AddStringToObject(pNode, HWM_SPEC_THRESHOLD_TYPE, TYPE_THRESHOLD_MAX);
		break;
	case DEF_THR_MIN_TYPE:
		cJSON_AddStringToObject(pNode, HWM_SPEC_THRESHOLD_TYPE, TYPE_THRESHOLD_MIN);
		break;
	case DEF_THR_MAXMIN_TYPE:
		cJSON_AddStringToObject(pNode, HWM_SPEC_THRESHOLD_TYPE, TYPE_THRESHOLDTHR_MAXMIN);
		break;
	case DEF_THR_BOOL_TYPE:
		cJSON_AddStringToObject(pNode, HWM_SPEC_THRESHOLD_TYPE, TYPE_THRESHOLDTHR_BOOL);
		break;
	}

	if(diskname)
		cJSON_AddStringToObject(pNode, HWM_SPEC_DISKID, diskname);

	return pNode;
}

cJSON * parser_CreateInfoSpec(hwm_info_t * pHWMInfo, hdd_info_t * pHDDInfo)
{
	/*
	"HWM":
	{
		"e":
		[
			{ "n": "temperature", "u": "Cel", "bn": "01", "max":100, "min":10, "type":"temp", "thd":"max"},
			{ "n": "pressure", "u": "kg", "bn": "03", "max":100, "min":10, "type":"temp", "thd":"max"},
			{ "n": "vibration", "u": "g", "bn": "04", "max":100, "min":10, "type":"temp", "thd":"max"},
			{ "n": "temperature ", "u": "kwh", "bn": "06", "max":100, "min":10, "type":"temp", "thd":"max"},
			{ "n": "temperature", "u": "Cel", "bn": "10", "max":100, "min":10, "type":"fan", "thd":"min"},
			{ "n": "pressure", "u": "kg", "bn": "11", "max":100, "min":10, "type":"fan", "thd":"min"},
			{ "n": "vibration", "u": "g", "bn": "13", "max":100, "min":10, "type":"fan", "thd":"min"},
			{ "n": "temperature ", "u": "kwh", "bn": "14", "max":100, "min":10, "type":"fan", "thd":"min"},

			{ "n": "maxProgram", "bn":"disk0-01", "max":0, "min":0, "type":"sqflash", "thd":"none", "diskid":"Disk0-SQF-S25M8-128G-S5C"},
			{ "n": "averageProgram", "bn":"disk0-02", "max":0, "min":0, "type":"sqflash", "thd":"none", "diskid":"Disk0-SQF-S25M8-128G-S5C"},
			{ "n": "powerOnTime", "u": "hour", "bn":"disk0-03", "max":0, "min":0, "type":"sqflash", "thd":"none", "diskid":"Disk0-SQF-S25M8-128G-S5C"},
			{ "n": "enduranceCheck", "bn":"disk0-04", "max":0, "min":0, "type":"sqflash", "thd":"none", "diskid":"Disk0-SQF-S25M8-128G-S5C"},
			{ "n": "goodBlockRate", "u": "%", "bn":"disk0-05", "max":0, "min":0, "type":"sqflash", "thd":"none", "diskid":"Disk0-SQF-S25M8-128G-S5C"},
			{ "n": "maxReservedBlock", "bn":"disk0-06", "max":0, "min":0, "type":"sqflash", "thd":"none", "diskid":"Disk0-SQF-S25M8-128G-S5C"},
			{ "n": "currentReservedBlock", "bn":"disk0-07", "max":0, "min":0, "type":"sqflash", "thd":"none", "diskid":"Disk0-SQF-S25M8-128G-S5C"},
			{ "n": "eccCount", "bn":"disk0-08", "max":0, "min":0, "type":"sqflash", "thd":"none", "diskid":"Disk0-SQF-S25M8-128G-S5C"},
			{ "n": "hddHealthPercent", "u": "%", "bn":"disk0-09", "max":0, "min":50, "type":"sqflash", "thd":"min", "diskid":"Disk0-SQF-S25M8-128G-S5C"},
			{ "n": "powerOnTime", "u": "hour", "bn":"disk1-03", "max":0, "min":0, "type":"hdd", "thd":"none", "diskid":"Disk1-ST320LT020-9YG142"},
			{ "n": "eccCount", "bn":"disk1-08", "max":0, "min":0, "type":"hdd", "thd":"none", "diskid":"Disk1-ST320LT020-9YG142"},
			{ "n": "hddHealthPercent", "u": "%", "bn":"disk1-09", "max":0, "min":50, "type":"hdd", "thd":"min", "diskid":"Disk1-ST320LT020-9YG142"},
			{ "n": "hddTemp", "u": "Cel", "bn": "disk1-10", "max":50, "min":0, "type":"hdd", "thd":"max", "diskid":"Disk1-ST320LT020-9YG142"}
		]
	},
	*/
   //cJSON* pGlobalInfoHead = NULL;
   cJSON* root = NULL;
   cJSON *pAgentInfoHead = NULL;
   cJSON* pEventList = NULL;
   //hwm_item_t *target = NULL;
   if(pHWMInfo)
   {
	   hwm_item_t *item = pHWMInfo->items;
	   //printf("Find Topic\r\n");
	   while(item != NULL)
	   {
		   if(!pEventList)
			   pEventList = cJSON_CreateArray();
		   cJSON_AddItemToArray(pEventList, CreateInfoSpecEventNode(item->name, item->unit, item->tag, item->type, item->minThreshold, item->maxThreshold, item->thresholdType));

		   item = item->next;
	   }
   }
  
   if(pHDDInfo && pHDDInfo->hddMonInfoList)
   {
	   char tag[128] = {0};
	   char hddtype[32] = {0};
	   hdd_mon_info_node_t * item = pHDDInfo->hddMonInfoList->next;
	   //printf("Find Topic\r\n");
	   while(item != NULL)
	   {
		   hdd_mon_info_t * hddMonInfo = &item->hddMonInfo;
		   char strName[32] = {0};
		   sprintf(strName,"Disk%d-%s",hddMonInfo->hdd_index,hddMonInfo->hdd_name);
		   
		   if(!pEventList)
			   pEventList = cJSON_CreateArray();

		   switch (hddMonInfo->hdd_type)
		   {
		   case SQFlash:
			   sprintf(hddtype, "%s", "SQFlash");
		   	break;
		   case StdDisk:
			   sprintf(hddtype, "%s", "STDDisk");
			   break;
		   default:
			   sprintf(hddtype, "%s", "Unknown");
			   break;
		   }

		   if(hddMonInfo->hdd_type == SQFlash)
		   {
			   memset(tag, 0, sizeof(tag));
			   sprintf(tag, "disk%d-%s", hddMonInfo->hdd_index, MAX_PROGRAM);
			   cJSON_AddItemToArray(pEventList, CreateHDDInfoSpecEventNode(MAX_PROGRAM, NULL, tag, hddtype, 0, 0, DEF_THR_UNKNOW_TYPE,strName));
			   memset(tag, 0, sizeof(tag));
			   sprintf(tag, "disk%d-%s", hddMonInfo->hdd_index, AVERAGE_PROGRAM);
			   cJSON_AddItemToArray(pEventList, CreateHDDInfoSpecEventNode(AVERAGE_PROGRAM, NULL, tag, hddtype, 0, 0, DEF_THR_UNKNOW_TYPE, strName));
			   memset(tag, 0, sizeof(tag));
			   sprintf(tag, "disk%d-%s", hddMonInfo->hdd_index, ENDURANCE_CHECK);
			   cJSON_AddItemToArray(pEventList, CreateHDDInfoSpecEventNode(ENDURANCE_CHECK, NULL, tag, hddtype, 0, 0, DEF_THR_UNKNOW_TYPE, strName));
			   memset(tag, 0, sizeof(tag));
			   sprintf(tag, "disk%d-%s", hddMonInfo->hdd_index, GOOD_BLOCK_RATE);
			   cJSON_AddItemToArray(pEventList, CreateHDDInfoSpecEventNode(GOOD_BLOCK_RATE, NULL, tag, hddtype, 0, 0, DEF_THR_UNKNOW_TYPE, strName));
			   memset(tag, 0, sizeof(tag));
			   sprintf(tag, "disk%d-%s", hddMonInfo->hdd_index, MAX_RESERVED_BLOCK);
			   cJSON_AddItemToArray(pEventList, CreateHDDInfoSpecEventNode(MAX_RESERVED_BLOCK, NULL, tag, hddtype, 0, 0, DEF_THR_UNKNOW_TYPE,strName));
			   memset(tag, 0, sizeof(tag));
			   sprintf(tag, "disk%d-%s", hddMonInfo->hdd_index, CURRENT_RESERVED_BLOCK);
			   cJSON_AddItemToArray(pEventList, CreateHDDInfoSpecEventNode(CURRENT_RESERVED_BLOCK, NULL, tag, hddtype, 0, 0, DEF_THR_UNKNOW_TYPE,strName));
		   }
		   else if(hddMonInfo->hdd_type == StdDisk)
		   {
			   memset(tag, 0, sizeof(tag));
			   sprintf(tag, "disk%d-%s", hddMonInfo->hdd_index, HDD_TEMP);
			   cJSON_AddItemToArray(pEventList, CreateHDDInfoSpecEventNode(HDD_TEMP, "Cel", tag, hddtype, 0, 100, DEF_THR_MAX_TYPE, strName));
		   }
		   memset(tag, 0, sizeof(tag));
		   sprintf(tag, "disk%d-%s", hddMonInfo->hdd_index, HDD_HEALTH_PERCENT);
		   cJSON_AddItemToArray(pEventList, CreateHDDInfoSpecEventNode(HDD_HEALTH_PERCENT, "%", tag, hddtype, 50, 0, DEF_THR_MIN_TYPE, strName));   
		   memset(tag, 0, sizeof(tag));
		   sprintf(tag, "disk%d-%s", hddMonInfo->hdd_index, POWER_ON_TIME);
		   cJSON_AddItemToArray(pEventList, CreateHDDInfoSpecEventNode(POWER_ON_TIME, "Hour", tag, hddtype, 0, 0, DEF_THR_UNKNOW_TYPE, strName));
		   memset(tag, 0, sizeof(tag));
		   sprintf(tag, "disk%d-%s", hddMonInfo->hdd_index, ECC_COUNT);
		   cJSON_AddItemToArray(pEventList, CreateHDDInfoSpecEventNode(ECC_COUNT, NULL, tag, hddtype, 0, 0, DEF_THR_UNKNOW_TYPE, strName));  

		   item = item->next;
	   }
   }
   root = cJSON_CreateObject();
   pAgentInfoHead = cJSON_CreateObject();
   cJSON_AddItemToObject(root, HWM_SPEC_TITLE, pAgentInfoHead);
   if(pEventList)
   {
		//cJSON* node = cJSON_CreateObject();
	    //cJSON_AddItemToObject(node, HWM_SPEC_EVENT, pTempList);
		cJSON_AddItemToObject(pAgentInfoHead, HWM_SPEC_EVENT, pEventList);
   }
   return root;
}

bool parser_ParseAutoUploadCmd(void* data, int datalen, int * interval, cJSON* reqItems)
{
	/*{"susiCommData":{"catalogID":4,"autoUploadIntervalSec":30,"requestID":1001,"requestItems":["HWM"],"commCmd":2053,"type":"WSN"}}*/

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

	target = cJSON_GetObjectItem(body, HWM_AUTOUPLOAD_INTERVAL_SEC);
	if(target)
	{
		*interval = target->valueint;
	}

	if(reqItems)
	{
		target = cJSON_GetObjectItem(body, HWM_AUTOUPLOAD_REQ_ITEMS);
		if(target)
		{
			int i=0;
			int size = cJSON_GetArraySize(target);
			
			if(size == 0)
			{
				cJSON_AddStringToObject(reqItems, "all", "all");
			}
			else
			{
				for(i=0; i<size; i++)
				{
					cJSON* item = cJSON_GetArrayItem(target, i);
					if(!item) continue;
					if(!item->string)
					{
						if(item->valuestring && !_stricmp(item->valuestring, "all"))
						{
							cJSON_AddStringToObject(reqItems, "all", "all");
						}
					}
					else if(!_stricmp(item->string, "hwm"))
					{
						cJSON_AddItemToObject(reqItems, item->string, cJSON_Duplicate(item->child, true));
					}
				}
			}
		}
	}

	cJSON_Delete(root);
	return true;
}

cJSON * CreateAutoReportEventNode(char const * name, char const * unit, char const * tag, char const * value)
{
	cJSON* pNode = cJSON_CreateObject();

	if(name)
		cJSON_AddStringToObject(pNode, HWM_SPEC_NAME, name);

	if(unit)
		cJSON_AddStringToObject(pNode, HWM_PFI_UNIT, unit);

	if(tag)
		cJSON_AddStringToObject(pNode, HWM_SPEC_BASENAME, tag);

	if(value)
		cJSON_AddStringToObject(pNode, HWM_SPEC_VALUE, value);

	return pNode;
}

cJSON * CreateAutoReportEventNode_Int(char const * name, char const * unit, char const * tag, unsigned int value)
{
	cJSON* pNode = cJSON_CreateObject();
	char sVal[64] = {0};

	if(name)
		cJSON_AddStringToObject(pNode, HWM_SPEC_NAME, name);

	if(unit)
		cJSON_AddStringToObject(pNode, HWM_PFI_UNIT, unit);

	/*if(tag)
		cJSON_AddStringToObject(pNode, HWM_SPEC_BASENAME, tag);*/

	if(tag)
		cJSON_AddStringToObject(pNode, HWM_SPEC_ID, tag);

	cJSON_AddNumberToObject(pNode, HWM_SPEC_VALUE, value);
	/*memset(sVal, 0, sizeof(sVal));
	sprintf(sVal, "%d",  value);
	cJSON_AddStringToObject(pNode, HWM_SPEC_VALUE, sVal);*/

	return pNode;
}

cJSON * CreateAutoReportEventNode_float(char const * name, char const * unit, char const * tag, float value)
{
	cJSON* pNode = cJSON_CreateObject();
	char sVal[64] = {0};

	if(name)
		cJSON_AddStringToObject(pNode, HWM_SPEC_NAME, name);

	if(unit)
		cJSON_AddStringToObject(pNode, HWM_PFI_UNIT, unit);

	if(tag)
		cJSON_AddStringToObject(pNode, HWM_SPEC_BASENAME, tag);

	cJSON_AddNumberToObject(pNode, HWM_SPEC_VALUE, value);
	/*memset(sVal, 0, sizeof(sVal));
	sprintf(sVal, "%f",  value);
	cJSON_AddStringToObject(pNode, HWM_SPEC_VALUE, sVal);*/

	return pNode;
}


bool parser_CreateHDDReport(hdd_info_t * pHDDInfo, cJSON * pENode)
{
	/*
	{
	"e":
	[
	{ "bn":"disk0-01", "v": "5000"},
	{ "bn":"disk0-02", "v": "1376"},
	{ "bn":"disk0-03", "v": "11074"},
	{ "bn":"disk0-04", "v": "73"},
	{ "bn":"disk0-05", "v": "100"},
	{ "bn":"disk0-06", "v": "164"},
	{ "bn":"disk0-07", "v": "104"},
	{ "bn":"disk0-08", "v": "0"},
	{ "bn":"disk0-09", "v": "73"},
	{ "bn":"disk1-03", "v": "11074"},
	{ "bn":"disk1-08", "v": "0"},
	{ "bn":"disk1-09", "v": "73"},
	{ "bn":"disk1-10", "v": "43"}
	]
	}
	*/
   char tag[128] = {0};
   hdd_mon_info_node_t *item = NULL;

   if(!pENode) return false;
   if(!pHDDInfo) return false;

   item = pHDDInfo->hddMonInfoList->next;

   //printf("Find Topic\r\n");
   while(item != NULL)
   {
	   hdd_mon_info_t * hddMonInfo = &item->hddMonInfo;
	
	   if(hddMonInfo->hdd_type == SQFlash)
	   {
		   memset(tag, 0, sizeof(tag));
		   sprintf(tag, "disk%d-%s", hddMonInfo->hdd_index, AVERAGE_PROGRAM);
		   cJSON_AddItemToArray(pENode, CreateAutoReportEventNode_Int(NULL, NULL, tag, hddMonInfo->average_program));
		   memset(tag, 0, sizeof(tag));
		   sprintf(tag, "disk%d-%s", hddMonInfo->hdd_index, ENDURANCE_CHECK);
		   cJSON_AddItemToArray(pENode, CreateAutoReportEventNode_Int(NULL, NULL, tag, hddMonInfo->endurance_check));
		   memset(tag, 0, sizeof(tag));
		   sprintf(tag, "disk%d-%s", hddMonInfo->hdd_index, GOOD_BLOCK_RATE);
		   cJSON_AddItemToArray(pENode, CreateAutoReportEventNode_Int(NULL, NULL, tag, hddMonInfo->good_block_rate));
		   memset(tag, 0, sizeof(tag));
		   sprintf(tag, "disk%d-%s", hddMonInfo->hdd_index, MAX_RESERVED_BLOCK);
		   cJSON_AddItemToArray(pENode, CreateAutoReportEventNode_Int(NULL, NULL, tag, hddMonInfo->max_reserved_block));
		   memset(tag, 0, sizeof(tag));
		   sprintf(tag, "disk%d-%s", hddMonInfo->hdd_index, CURRENT_RESERVED_BLOCK);
		   cJSON_AddItemToArray(pENode, CreateAutoReportEventNode_Int(NULL, NULL, tag, hddMonInfo->current_reserved_block));
	   }
	   else if(hddMonInfo->hdd_type == StdDisk)
	   {
		   memset(tag, 0, sizeof(tag));
		   sprintf(tag, "disk%d-%s", hddMonInfo->hdd_index, HDD_TEMP);
		   cJSON_AddItemToArray(pENode, CreateAutoReportEventNode_Int(NULL, NULL, tag, hddMonInfo->hdd_temp));
	   }
	   memset(tag, 0, sizeof(tag));
	   sprintf(tag, "disk%d-%s", hddMonInfo->hdd_index, HDD_HEALTH_PERCENT);
	   cJSON_AddItemToArray(pENode, CreateAutoReportEventNode_Int(NULL, NULL, tag, hddMonInfo->hdd_health_percent));
	   memset(tag, 0, sizeof(tag));
	   sprintf(tag, "disk%d-%s", hddMonInfo->hdd_index, POWER_ON_TIME);
	   cJSON_AddItemToArray(pENode, CreateAutoReportEventNode_Int(NULL, NULL, tag, hddMonInfo->power_on_time));
	   memset(tag, 0, sizeof(tag));
	   sprintf(tag, "disk%d-%s", hddMonInfo->hdd_index, ECC_COUNT);
	   cJSON_AddItemToArray(pENode, CreateAutoReportEventNode_Int(NULL, NULL, tag, hddMonInfo->ecc_count));  

	   item = item->next;
   }
   
   return true;
}

bool parser_CreateHWMReport(hwm_info_t * pHWMInfo, cJSON * pENode)
{
	/*
	{
	"e":
	[
	{ "v": "27.2", "bn": "01"},
	{ "v": "1300", "bn": "04"},
	{ "v": "0.5", "bn": "10"},
	]
	},
	*/
  
   hwm_item_t *item = NULL;
    if(!pENode) return false;
   if(!pHWMInfo) return false;
   item = pHWMInfo->items;

   //printf("Find Topic\r\n");
   while(item != NULL)
   {      
	   cJSON_AddItemToArray(pENode, CreateAutoReportEventNode_float(NULL, NULL, item->tag, item->value));	
	   item = item->next;
   }

   return true;
}