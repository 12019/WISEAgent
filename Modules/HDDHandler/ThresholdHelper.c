#include "ThresholdHelper.h"
#include "Parser.h"
#include <string.h>


//************************************
// Method:    CheckSourceValue
// Access:    private .Only be called in Threshold_ChkValue ()
// Returns:   bool
// Qualifier:
// Parameter: sa_thr_item_t * pThrItem
// Parameter: check_value_t * pCheckValue
// Parameter: value_type_t valueType
//************************************

hdd_thr_list_t Threshold_CreateHDDThrList(hdd_info_t * pHDDInfo)
{
	hdd_thr_list_t    hddThrList = NULL;
	hdd_mon_info_node_t * curHDD = NULL;
	hdd_thr_node_t     * curNode = NULL;
	hdd_thr_node_t    * prevNode = NULL;
	if (NULL == pHDDInfo)   return NULL;

	//== initial HDDThrList
	for (curHDD = pHDDInfo->hddMonInfoList->next; NULL != curHDD; curHDD = curHDD->next)
	{
		//== malloc memory for a HDD
		int size = sizeof(hdd_thr_node_t);
		curNode = (hdd_thr_node_t *)malloc(size);
		if (NULL == curNode) return NULL;
		memset(curNode, 0, size);

		//== set HDDBaseName
		sprintf(curNode->hddBaseName , "Disk%d-%s", curHDD->hddMonInfo.hdd_index, curHDD->hddMonInfo.hdd_name);

		//== link pointer and loop
		if (NULL == prevNode) hddThrList = prevNode = curNode;
		else prevNode->next = curNode;
	}
	return hddThrList;
}


cJSON * Threshold_ParseCmdToThrArray(void* data, int datalen)
{
	//get "hddThr":[***] from {"susiCommData":{"catalogID":4,"commCmd":257, "handlerName":"HDDMonitor","hddThr":[***] }}
	cJSON * root     = NULL;
	cJSON * body     = NULL;
	cJSON * temp     = NULL;
	cJSON * thrArray = NULL;
	if (!data || datalen <= 0) return false;

	root     = cJSON_Parse((const char *)data);                  if (! root)     goto FINISH;
	body     = cJSON_GetObjectItem(root, AGENTINFO_BODY_STRUCT); if (! body)     goto FINISH;
	temp     = cJSON_GetObjectItem(body, HDD_THR);               if (! temp)     goto FINISH;
	thrArray = cJSON_Duplicate(temp,true);                   

FINISH:
	if (root) cJSON_Delete(root);
	return thrArray;
}


cJSON * Threshold_ReadHDDThrCfg(char const * modulepath)
{
	cJSON * hddThrArray = NULL;
	FILE *fp;

	unsigned int Len = 0;
	char fullPath[MAX_PATH] = { 0 };

	if (modulepath == NULL) return NULL;
	path_combine(fullPath, modulepath, DEF_THRESHOLD_CONFIG);
	if ((fp = fopen(fullPath, "rb")) == NULL) {
		printf("It's unable to read the ThrCfg file (\"%s\"), it isn't exist at all.\r\n", fullPath);
		return NULL;
	}
	fseek(fp, 0, SEEK_END);
	Len = ftell(fp) + 1;
	if (Len > 1)
	{
		size_t readLen = 0;
		char *readData = NULL;

		readData = (char *)malloc(Len);
		memset(readData, 0, Len);
		fseek(fp, 0, SEEK_SET);
		readLen = fread(readData, sizeof(char), Len, fp);
		if (readLen > 0) {
			hddThrArray = Threshold_ParseCmdToThrArray(readData, readLen);
		}
		if (readData) free(readData);
	}
	fclose(fp);
	return hddThrArray;
}



//************************************
// Method:    Threshold_LoadOneRule
// Describe:  Load a threshold (from JSON data to saRule)
// Access:    public 
// Returns:   bool
// Parameter:(IN) sa_thr_item_t * saRule
// Parameter:(IN) cJSON *         objRule
//************************************
bool Threshold_LoadOneRule(sa_thr_item_t * saRule, cJSON * objRule)
{
	/*
	{"n":"HDDMonitor/hddSmartInfoList/Disk1-WDC WD10EZEX-08M2NA0/Temperature/value",
	"max" : 45, "min" : 30, "type" : 3, "lastingTimeS" : 3, "intervalTimeS" : 20, "enable" : "true"},
	*/
	cJSON * tmp = NULL;
	if (NULL == saRule || NULL == objRule) return false;
	
	tmp = cJSON_GetObjectItem(objRule, THR_ENABLE);
	if ( 0 == strcmp("true", tmp->valuestring)) saRule->isEnable = true;
	if ( 0 == strcmp("false",tmp->valuestring)) saRule->isEnable = false;
	
	tmp = cJSON_GetObjectItem(objRule, THR_N);               if (tmp) saRule->tagPath       = strdup(tmp->valuestring);
	tmp = cJSON_GetObjectItem(objRule, THR_MAX);             if (tmp) saRule->maxThreshold  = tmp->valuedouble;
	tmp = cJSON_GetObjectItem(objRule, THR_MIN);             if (tmp) saRule->minThreshold  = tmp->valuedouble;
	tmp = cJSON_GetObjectItem(objRule, THR_TYPE);            if (tmp) saRule->thresholdType = tmp->valueint;
	tmp = cJSON_GetObjectItem(objRule, THR_LASTING_TIME_S);  if (tmp) saRule->lastingTimeS  = tmp->valueint;
	tmp = cJSON_GetObjectItem(objRule, THR_INTERVAL_TIME_S); if (tmp) saRule->intervalTimeS = tmp->valueint;

	saRule->checkSourceValueList.head = NULL;
	saRule->checkSourceValueList.nodeCnt = 0;
	saRule->checkType    = ck_type_avg;
	saRule->isNormal      = true;
	saRule->repThrTime  = 0;
	printf("[HDDMonitor] Success to load HDD Threshold :\"%s\".\r\n", saRule->tagPath);
	return true;
}



//************************************
// Method:    Threshold_LoadALLRules
// Describe:  Load rules one by one,  unless it match the correct HDD
// Access:    public 
// Returns:   void
// Parameter: hdd_thr_list_t hddThrList
// Parameter: cJSON * arrayRules
//************************************
void Threshold_LoadALLRules(hdd_thr_list_t hddThrList, cJSON * arrayRules)
{
	int i, j, ruleCnt, layerCnt;
	cJSON * curRule = NULL;
	hdd_thr_node_t * curHDDThrNode = NULL;
	sa_thr_node_t  * newSAThrNode = NULL;
	sa_thr_node_t  * curSAThrNode = NULL;
	char * path = NULL;
	char * pathLayer[MAX_PATH_LAYER];
	char * hddBN = NULL;
	bool bMatch = false;

	if (NULL == hddThrList || NULL == arrayRules) return;
	ruleCnt = cJSON_GetArraySize(arrayRules);
	for (i = 0; i < ruleCnt; ++i)
	{
		//== get hddBaseName of the rule[i]
		curRule = cJSON_GetArrayItem(arrayRules, i);
		path = cJSON_GetObjectItem(curRule, THR_N)->valuestring;
		memset(pathLayer, 0, sizeof(pathLayer));
		if (!Parser_ParsePathLayer((const char *)path, pathLayer, &layerCnt)){
			printf("[Error] in Threshold_LoadAllRule, can't parse rule path: \"%s\"\r\n", path);
			for (j = 0; j < MAX_PATH_LAYER && NULL != pathLayer[j]; ++j) free(pathLayer[j]);
			continue;
		}
		/* 
		path :"HDDMonitor/hddSmartInfoList/Disk1-WDC WD10EZEX-08M2NA0/Temperature/value")
		hddBaseName :"Disk1-WDC WD10EZEX-08M2NA0"
		*/
		hddBN = pathLayer[2];

		//==match hddBaseName && load the rule
		bMatch = false;
		for (curHDDThrNode = hddThrList; NULL != curHDDThrNode; curHDDThrNode = curHDDThrNode->next)
		{
			if (0 == strcmp(curHDDThrNode->hddBaseName, hddBN))
			{
				//== malloc and load the rule
				newSAThrNode = (sa_thr_node_t *)malloc(sizeof(sa_thr_node_t));
				memset(newSAThrNode, 0, sizeof(sa_thr_node_t));
				Threshold_LoadOneRule(&(newSAThrNode->item), curRule);

				//== add rule into thrList
				if (NULL == curHDDThrNode->thrList) curHDDThrNode->thrList = newSAThrNode;
				else {
					curSAThrNode = curHDDThrNode->thrList;
					while (curSAThrNode && curSAThrNode->next) curSAThrNode = curSAThrNode->next;
					curSAThrNode->next = newSAThrNode;
				}
				bMatch = true;
				break;
			}
		}
		if (!bMatch) printf("[WARNING] in Threshold_LoadAllRule: rule \"%s\" doesn't match any HardDisk's name.\r\n", hddBN);
		//==free pathLayer
		for (j = 0; j < MAX_PATH_LAYER && NULL != pathLayer[j]; ++j) free(pathLayer[j]);
	}
	return;
}


//************************************
// Method:    IncreaseBufferMessage
// Describe:  Dynamic malloc memory for building multi message, initial buffer is 1024 byte, will double autoly
// Access:    public 
// Returns:   bool
// Parameter: (INOUT) char * * ppBuffer, pointer of buffer, presents the whole message we build
// Parameter: (INOUT) int * pBufferSize,
// Parameter: (IN)    char * msg       , the one message we append to buffer
//************************************
bool IncreaseBufferMessage(char ** ppBuffer, int * pBufferSize, char * msg)
{
	char * pBuffer = *ppBuffer;
	int bufSize = * pBufferSize;
	int bufUsed = 0;
	int msgLen  = 0;
	if (!msg) return false;
	if (!pBuffer) {
		pBuffer =(char *) malloc(1024);
		memset(pBuffer, 0 ,1024);
		bufSize =1024;
	}
	bufUsed = strlen(pBuffer);
	msgLen  = strlen(msg);

	while ( bufSize < bufUsed + msgLen + 3 ){
		char * old = pBuffer;
		pBuffer = (char *)realloc(old, bufSize * 2);
		if (!pBuffer) {
			free(old);
			return false;
		}
		bufSize = bufSize * 2;
	}

	if (bufUsed)  bufUsed += sprintf(pBuffer + bufUsed, "; %s",msg);
	else          bufUsed += sprintf(pBuffer + bufUsed, msg);
	*ppBuffer    = pBuffer; // buffer address saved back
	*pBufferSize = bufSize;
	return true;
}


bool CheckSourceValue(sa_thr_item_t * pThrItem, check_value_t * pCheckValue, value_type_t valueType)
{
	bool bRet = false;
	long long nowTime = time(NULL);
	if (pThrItem == NULL || pCheckValue == NULL) return bRet;
	switch (valueType) //modified by tang.tao @2015-7-9
	{
	case value_type_float: pThrItem->checkResultValue.vf = DEF_INVALID_VALUE; break;
	case value_type_int:   pThrItem->checkResultValue.vi = DEF_INVALID_VALUE; break;
	}

	if (pThrItem->checkSourceValueList.head == NULL)
	{
		pThrItem->checkSourceValueList.head = (check_value_node_t *)malloc(sizeof(check_value_node_t));
		pThrItem->checkSourceValueList.nodeCnt = 0;
		pThrItem->checkSourceValueList.head->checkValueTime = DEF_INVALID_TIME;
		switch (valueType) //modified by tang.tao @ 2015-07-09
		{
		case value_type_float: pThrItem->checkSourceValueList.head->ckV.vf = DEF_INVALID_VALUE; break;
		case value_type_int:   pThrItem->checkSourceValueList.head->ckV.vi = DEF_INVALID_VALUE; break;
		default:                                                                                break;
		}
		pThrItem->checkSourceValueList.head->next = NULL;
	}

	{
		check_value_node_t * head = pThrItem->checkSourceValueList.head;
		check_value_node_t * newNode = (check_value_node_t *)malloc(sizeof(check_value_node_t));
		//printf( ">Add Value Node\n");
		switch (valueType)
		{
		case value_type_float:
			{
				newNode->ckV.vf = pCheckValue->vf;
				break;
			}
		case value_type_int:
			{
				newNode->ckV.vi = pCheckValue->vi;
				break;
			}
		default: break;
		}
		newNode->checkValueTime = nowTime;
		newNode->next = head->next;
		head->next = newNode;
		pThrItem->checkSourceValueList.nodeCnt++;
	}

	if (pThrItem->checkSourceValueList.nodeCnt > 0)
	{
		long long minCkvTime = 0;
		check_value_node_t * curNode = pThrItem->checkSourceValueList.head->next;
		minCkvTime = curNode->checkValueTime;
		while (curNode)
		{
			if (curNode->checkValueTime < minCkvTime)  minCkvTime = curNode->checkValueTime;
			curNode = curNode->next;
		}

		if (nowTime - minCkvTime >= pThrItem->lastingTimeS || nowTime - minCkvTime == 0)
		{
			switch (pThrItem->checkType)
			{
			case ck_type_avg:
			{
				check_value_node_t * curNode = pThrItem->checkSourceValueList.head->next;
				float avgTmpF = 0;
				int avgTmpI = 0;
				while (curNode)
				{
					switch (valueType)
					{
					case value_type_float:
						if (curNode->ckV.vf != DEF_INVALID_VALUE)
						{
							avgTmpF += curNode->ckV.vf;
							break;
						}
					case value_type_int:
						if (curNode->ckV.vi != DEF_INVALID_VALUE)
						{
							avgTmpI += curNode->ckV.vi;
							break;
						}
					default: break;
					}
					curNode = curNode->next;
				}
				if (pThrItem->checkSourceValueList.nodeCnt > 0)
				{
					switch (valueType)
					{
					case value_type_float:
					{
						avgTmpF = avgTmpF / pThrItem->checkSourceValueList.nodeCnt;
						pThrItem->checkResultValue.vf = avgTmpF;
						bRet = true;
						break;
					}
					case value_type_int:
					{
						avgTmpI = avgTmpI / pThrItem->checkSourceValueList.nodeCnt;
						pThrItem->checkResultValue.vi = avgTmpI;
						bRet = true;
						break;
					}
					default: break;
					}
				}
				break;
			}
			case ck_type_max:
			{
				check_value_node_t * curNode = pThrItem->checkSourceValueList.head->next;
				float maxTmpF = DEF_INVALID_VALUE;
				int maxTmpI = DEF_INVALID_VALUE;
				while (curNode)
				{
					switch (valueType)
					{
					case value_type_float:
					{
						if (curNode->ckV.vf > maxTmpF) maxTmpF = curNode->ckV.vf;
						break;
					}
					case value_type_int:
					{
						if (curNode->ckV.vi > maxTmpI) maxTmpI = curNode->ckV.vi;
						break;
					}
					default: break;
					}
					curNode = curNode->next;
				}
				switch (valueType)
				{
				case value_type_float:
				{
					if (maxTmpF > DEF_INVALID_VALUE)
					{
						pThrItem->checkResultValue.vf = maxTmpF;
						bRet = true;
					}
					break;
				}
				case value_type_int:
				{
					if (maxTmpI > DEF_INVALID_VALUE)
					{
						pThrItem->checkResultValue.vi = maxTmpI;
						bRet = true;
					}
					break;
				}
				default: break;
				}
				break;
			}
			case ck_type_min:
			{
				check_value_node_t * curNode = pThrItem->checkSourceValueList.head->next;
				float minTmpF = 99999;
				int minTmpI = 99999;
				while (curNode)
				{
					switch (valueType)
					{
					case value_type_float:
					{
						if (curNode->ckV.vf < minTmpF) minTmpF = curNode->ckV.vf;
						break;
					}
					case value_type_int:
					{
						if (curNode->ckV.vi < minTmpI) minTmpI = curNode->ckV.vi;
						break;
					}
					default: break;
					}
					curNode = curNode->next;
				}
				switch (valueType)
				{
				case value_type_float:
				{
					if (minTmpF < 99999)
					{
						pThrItem->checkResultValue.vf = minTmpF;
						bRet = true;
					}
					break;
				}
				case value_type_int:
				{
					if (minTmpI < 99999)
					{
						pThrItem->checkResultValue.vi = minTmpI;
						bRet = true;
					}
					break;
				}
				default: break;
				}

				break;
			}
			default: break;
			}

			{
				check_value_node_t * frontNode = pThrItem->checkSourceValueList.head;
				check_value_node_t * curNode = frontNode->next;
				check_value_node_t * delNode = NULL;
				while (curNode)
				{
					if (nowTime - curNode->checkValueTime >= pThrItem->lastingTimeS)
					{
						//printf( ">Remove Value Node\n");
						delNode = curNode;
						frontNode->next = curNode->next;
						curNode = frontNode->next;
						free(delNode);
						pThrItem->checkSourceValueList.nodeCnt--;
						delNode = NULL;
					}
					else
					{
						frontNode = curNode;
						curNode = frontNode->next;
					}
				}
			}
		}
	}
	
	return bRet;
}


bool Threshold_ChkValue(sa_thr_node_t * pRuleNode, unsigned int value, char * cRetMsg, bool * bRetNormal)
{
	bool bReport = false;
	sa_thr_item_t * rule = &(pRuleNode->item);
	char checkTypeStr[32] = { 0 };
	check_value_t checkValue;
	bool isTrigger   = false;
	bool triggerMax  = false;
	bool triggerMin  = false;
	bool triggerBool = false;

	if (!pRuleNode || !pRuleNode->item.isEnable)	return bReport;

	switch (rule->checkType)
	{
	case ck_type_max:
		sprintf(checkTypeStr, "max");     break;
	case ck_type_min:
		sprintf(checkTypeStr, "min");     break;
	case ck_type_avg:
		sprintf(checkTypeStr, "average"); break;
	case ck_type_bool:
		sprintf(checkTypeStr, "bool");    break;
	default:                              break;
	}

	checkValue.vi = value;
	if (CheckSourceValue(rule, &checkValue, value_type_int) && rule->checkResultValue.vi != DEF_INVALID_VALUE)
	{
		int usageV = rule->checkResultValue.vi;

		if (rule->thresholdType & DEF_THR_MAX_TYPE){
			if (rule->maxThreshold != DEF_INVALID_VALUE && (usageV > rule->maxThreshold)){
				sprintf(cRetMsg, "%s (%s%s%s:%d) > %s%s%s(%f)", rule->tagPath, MARK_TK, checkTypeStr, MARK_TK, usageV, MARK_TK, THR_MAX_THRESHOLD, MARK_TK, rule->maxThreshold);
				triggerMax = true;
			}
		}
		if (rule->thresholdType & DEF_THR_MIN_TYPE) {
			if (rule->minThreshold != DEF_INVALID_VALUE && (usageV < rule->minThreshold)) {
				sprintf(cRetMsg, "%s (%s%s%s:%d) < %s%s%s(%f)", rule->tagPath, MARK_TK, checkTypeStr, MARK_TK, usageV, MARK_TK, THR_MIN_THRESHOLD, MARK_TK, rule->minThreshold);
				triggerMin = true;
			}
		}
		// 		if (rule->thresholdType & DEF_THR_BOOL_TYPE) {
		// 			if (rule->minThreshold != DEF_INVALID_VALUE && (usageV != rule->maxThreshold))
		// 			{
		// 				if (strlen(tmpRetMsg)) sprintf(tmpRetMsg, "%s and HDD(%s) %s(%s:%.2f)!=maxThreshold(%.2f)", tmpRetMsg, hddName, rule->tagPath, checkTypeStr, usageV, rule->maxThreshold);
		// 				else sprintf(tmpRetMsg, "HDD(%s) %s(%s:%.2f)!=maxThreshold(%.2f)", hddName, rule->tagPath, checkTypeStr, usageV, rule->maxThreshold);
		// 				triggerBool = true;
		// 			}
		// 		}
	}

	switch (rule->thresholdType)
	{
	case DEF_THR_MAX_TYPE:
		isTrigger = triggerMax;	break;
	case DEF_THR_MIN_TYPE:
		isTrigger = triggerMin; break;
	case DEF_THR_MAXMIN_TYPE:
		isTrigger = triggerMin || triggerMax; break;
	case DEF_THR_BOOL_TYPE:
		isTrigger = triggerBool; break;
	}

	if (isTrigger) {
		long long nowTime = time(NULL);
		if (rule->repThrTime == 0 || rule->intervalTimeS == DEF_INVALID_TIME || rule->intervalTimeS == 0 || nowTime - rule->repThrTime > rule->intervalTimeS) {
			rule->repThrTime = nowTime;
			rule->isNormal = false;
			*bRetNormal = false;
			bReport = true;
		}
	}
	else if (!rule->isNormal && rule->checkResultValue.vi != DEF_INVALID_VALUE) {
		memset(cRetMsg, 0, sizeof(cRetMsg));
		sprintf(cRetMsg, "%s %s%s%s", rule->tagPath, MARK_TK, THR_NORMAL, MARK_TK);
		rule->isNormal = true;
		*bRetNormal = true;
		bReport = true;
	}
	return bReport;
}


bool Threshold_Compare(sa_thr_item_t * rule1, sa_thr_item_t * rule2)
{
	bool bRet = true;
	if ( rule1 == rule2) return bRet;
	if (!rule1 || !rule2) return false;
	if ( rule1->isEnable      != rule2->isEnable      ) bRet = false;
	if ( rule1->thresholdType != rule2->thresholdType ) bRet = false;
	if ( rule1->intervalTimeS != rule2->intervalTimeS ) bRet = false;
	if ( rule1->lastingTimeS  != rule2->lastingTimeS  ) bRet = false;
	if ( rule1->maxThreshold  != rule2->maxThreshold  ) bRet = false;
	if ( rule1->minThreshold  != rule2->minThreshold  ) bRet = false;
	return bRet;
}


//************************************
// Method:    Threshold_MoveCache
// Describe:  move date-stream of thr-rule from oldList to newList 
//            ...unless the threshold rule is changed or is not exist anymore 
// Returns:   bool
// Parameter INOUT:  hdd_thr_list_t oldList (as source)
// Parameter INOUT:  hdd_thr_list_t newList (as target)
//************************************
bool Threshold_MoveCache(hdd_thr_list_t oldList, hdd_thr_list_t newList)
{
	bool bRet = false;
	hdd_thr_node_t * curOldHDDNode = NULL, *curNewHDDNode = NULL;
	sa_thr_node_t  * curOldSANode = NULL, *curNewSANode = NULL;
	if ( !oldList || !newList ) return bRet;

	//== compare the old with the new , loop by HDD
	for (curOldHDDNode = oldList, curNewHDDNode = newList;
		curOldHDDNode && curNewHDDNode;
		curOldHDDNode = curOldHDDNode->next, curNewHDDNode = curNewHDDNode->next)
	{
		//== loop by old rule
		for (curOldSANode = curOldHDDNode->thrList; curOldSANode; curOldSANode = curOldSANode->next)
		{
			//== loop by new rule / try to match rule
			for (curNewSANode = curNewHDDNode->thrList; curNewSANode; curNewSANode = curNewSANode->next)
			{
				//== move cache date from oldThr to newThr, if they are match and remain unchanged
				if (0 == strcmp(curOldSANode->item.tagPath, curNewSANode->item.tagPath)) // match
				{
					sa_thr_item_t * oldRule = & curOldSANode->item;
					sa_thr_item_t * newRule = & curNewSANode->item;			
					if (!Threshold_Compare(oldRule, newRule)) break;
					newRule->repThrTime                   = oldRule->repThrTime;
					newRule->isNormal                     = oldRule->isNormal;
					newRule->checkResultValue             = oldRule->checkResultValue;
					newRule->checkSourceValueList.head    = oldRule->checkSourceValueList.head;
					newRule->checkSourceValueList.nodeCnt = oldRule->checkSourceValueList.nodeCnt;
					oldRule->checkSourceValueList.head    = NULL;
					oldRule->checkSourceValueList.nodeCnt = 0;
					//printf("The cache date of threshold[%s] is remained.\n",oldRule->tagPath);
				}

			}//END loop new rule		
		}//END loop old rule
	}//END loop HDD
	return bRet;

}


//************************************
// Method:    Threshold_ChkHDDThrList
// FullName:  Threshold_ChkHDDThrList
// Access:    public 
// Returns:   bool (not used)
// Qualifier:
// Parameter: (IN) hdd_thr_list_t hddThrList
// Parameter: (IN) cJSON * pMonitor (json root of InfoSpec)
// Parameter: (OUT) bool * bNormal
// Parameter: (OUT) char * * ppRepMsg
//************************************
bool Threshold_ChkHDDThrList(hdd_thr_list_t hddThrList, cJSON * pMonitor, bool * bNormal, char ** ppRepMsg)
{
	bool bRetReport = false;
	int  msgBufferSize = 0;
	hdd_thr_node_t  * curThrNode = NULL;
	sa_thr_node_t   * curRuleNode = NULL;
	if (! hddThrList || ! pMonitor) return bRetReport;
	if (*ppRepMsg) *ppRepMsg = NULL;
	*bNormal = true;

	//== loop to check HDDThrlist ========================================
	for (curThrNode = hddThrList; curThrNode; curThrNode = curThrNode->next)
	{
		//== loop to check all rules of current HDD
		for (curRuleNode = curThrNode->thrList; curRuleNode; curRuleNode = curRuleNode->next)
		{
			bool bOneReport   = false;
			bool bOneNormal   = true;
			char oneMsg[256]  = { 0 };
			int  chkValue     = -999;

			//== Get sensor value && check rule
			chkValue = Parser_GetSensorValueByPath(curRuleNode->item.tagPath, pMonitor);
			if (chkValue < 0) {
				printf("[Error] in function -- ChkHDDThrList -- Failed to get sensor value by the path \"%s\".\r\n", curRuleNode->item.tagPath);
				continue;
			}
			if (curRuleNode->item.isEnable) {
				bOneReport = Threshold_ChkValue(curRuleNode, chkValue, oneMsg, &bOneNormal);
				//== summary chkThrMsg 
				if (bOneReport) {
					bRetReport = true;
					*bNormal &= bOneNormal;
					if (!IncreaseBufferMessage(ppRepMsg, &msgBufferSize, oneMsg)) {
						printf("[Error]:Out of memory when build buffer threshold message.\n");
						return false;
					}
				}
			}
			continue;
		}// END rules loop
	}// END hddThr loop

	return bRetReport;
}


bool Threshold_ResetNormal(hdd_thr_list_t oldList, hdd_thr_list_t newList, char ** ppRepMsg)
{
	bool bRetReport = false;
	int msgBufferSize = 0;
	hdd_thr_node_t * curOldHDDNode = NULL, * curNewHDDNode = NULL;
	sa_thr_node_t  * curOldSANode = NULL,  * curNewSANode = NULL;
	if (!oldList || !newList) return bRetReport;
	if (*ppRepMsg) *ppRepMsg = NULL;

	//== compare the old with the new , loop by HDD
	for (curOldHDDNode = oldList, curNewHDDNode = newList;
		 curOldHDDNode && curNewHDDNode;
		 curOldHDDNode = curOldHDDNode->next, curNewHDDNode = curNewHDDNode->next)
	{
		//== loop by old rule
		for (curOldSANode = curOldHDDNode->thrList; curOldSANode; curOldSANode = curOldSANode->next)
		{
			bool bReset = true;

			if (false == curOldSANode->item.isEnable || 
				true  == curOldSANode->item.isEnable && true == curOldSANode->item.isNormal)  {
				continue;
			}

			//== loop by new rule / try to match rule
			for (curNewSANode = curNewHDDNode->thrList; curNewSANode; curNewSANode = curNewSANode->next)
			{
				if ( 0 == strcmp(curOldSANode->item.tagPath,curNewSANode->item.tagPath) ) // match
				{
					bReset = false;
					if (false == curNewSANode->item.isEnable || 
						true  == curNewSANode->item.isEnable && true == curNewSANode->item.isNormal) {
						bReset = true;
					}
					break;
				}
			}//END loop new rule

			if (bReset){
				char oneMsg[256] = { 0 };
				sprintf(oneMsg, "%s %s%s%s", curOldSANode->item.tagPath, MARK_TK, THR_NORMAL, MARK_TK);		
				if (!IncreaseBufferMessage(ppRepMsg, & msgBufferSize, (char *)oneMsg)) {
					printf("[Error]:Out of memory when build buffer threshold reset normal message.\n");
					return false;
				}
				bRetReport = true;
			}
		}//END loop old rule
	}//END loop HDD
	return bRetReport;
}


bool Threshold_WriteHDDThrCfg(char const * modulepath, char const * rules)
{
	FILE * fp;
	char thrPath[MAX_PATH] = { 0 };
	if (NULL == modulepath) return false;
	path_combine(thrPath, modulepath, DEF_THRESHOLD_CONFIG);
	fp = fopen(thrPath, "w");
	if (NULL == fp) { printf("> Failed to open \"%s\" for writing.\r\n", thrPath); return false; }
	if (rules) fputs(rules, fp);
	fclose(fp);
	return true;
}


bool Threshold_DestroyHDDThrList(hdd_thr_list_t hddThrList)
{
	hdd_thr_node_t     * curHDDThrNode = NULL, *nextHDDThrNode = NULL;
	sa_thr_node_t      * curSAThrNode  = NULL, *nextSAThrNode  = NULL;
	check_value_node_t * curChkValNode = NULL, *nextChkValNode = NULL;

	if (NULL == hddThrList) return false;

	//== level 1 == pre-work: loop freeing(hdd_thr_node_t *)
	for (curHDDThrNode = hddThrList; NULL != curHDDThrNode; curHDDThrNode = nextHDDThrNode)
	{
		//== level 2 == pre-work: loop freeing(sa_thr_item_t *)
		for (curSAThrNode = curHDDThrNode->thrList; NULL != curSAThrNode; curSAThrNode = nextSAThrNode)
		{
			//== level 3 == pre-work: loop freeing(check_value_node_t *)
			for (curChkValNode = curSAThrNode->item.checkSourceValueList.head; NULL != curChkValNode; curChkValNode = nextChkValNode)  {
				nextChkValNode = curChkValNode->next;
				free(curChkValNode);
				curChkValNode = NULL;
			}
			if ( curSAThrNode->item.tagPath ) free(curSAThrNode->item.tagPath);
			nextSAThrNode = curSAThrNode->next;
			free(curSAThrNode);
		}
		nextHDDThrNode = curHDDThrNode->next;
		free(curHDDThrNode);
	}
	return true;
}


