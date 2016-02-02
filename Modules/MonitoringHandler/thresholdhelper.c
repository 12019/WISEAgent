#include  "common.h"
#include "thresholdhelper.h"
//static bool g_bTrace = true;
////#define LINUX_DEBUG_CHU
//#ifdef LINUX_DEBUG_CHU
//#define TRACE_LOG(enable,format,...) {if (enable) printf("=== METHOD[%s] LINE[%4d] ==="format"\n",__FUNCTION__, __LINE__, ##__VA_ARGS__);}
//#else
//#define TRACE_LOG(format,...)
//#endif

#define DEF_INVALID_TIME				(-1) 

void FreeSourceValueList(check_value_list_t target)
{
	check_value_node_t * cur = target.head;
	check_value_node_t * temp = NULL;
	while (cur)
	{
		temp = cur->next;
		free(cur);
		cur = temp;
	}
	return;
}


//************************************
// Method:    threshold_LoadRule
// Describe:  Fill the value to the rule pointer, used in initial hdd_thr_List, copy the head rule into every hdd_thr_node
// Access:    private
// Returns:   bool
// Parameter: sa_thr_item_t * rule === the destination pointer as the address to be write value
//************************************
bool threshold_LoadOneRule(sa_thr_item_t * rule, char * tag, float maxThreshold, float minThreshold, int thresholdType, int isEnable, int lasting, int interval)
{
	if (NULL == rule ) return false;
	if (NULL == tag) memset(rule->tagName, 0, sizeof(rule->tagName));
	else strcpy(rule->tagName, tag);
	rule->maxThreshold = maxThreshold;
	rule->minThreshold = minThreshold;
	rule->thresholdType = thresholdType;
	rule->isEnable = isEnable;
	rule->lastingTimeS = lasting;
	rule->intervalTimeS = interval;

	rule->checkSourceValueList.head = NULL;
	rule->checkSourceValueList.nodeCnt = 0;
	rule->checkType = ck_type_avg;
	rule->isNormal = true;
	rule->repThrTime = 0;
	//printf(">Load threshold rule: ( %s, %f, %f, %d, %d, %d, %d)\n", tag, maxThreshold, minThreshold, thresholdType, isEnable, lasting, interval);
	return true;
}

//************************************
// Method:    Threshold_CreateHDDThrDefaultNode( void )
// Describe:  Create Empty hdd_thr_item_node_t
// Access:    private
// Returns:   hdd_thr_item_node_t *
// Qualifier: tang.tao
//************************************
hdd_thr_item_node_t * threshold_CreateHDDThrDefaultNode()
{
	hdd_thr_item_node_t * head = NULL;
	head = (hdd_thr_item_node_t *)malloc(sizeof(hdd_thr_item_node_t));
	if (head)
	{
		int i = 0;
		head->next = NULL;
		memset(head->hddThrItem.hdd_name, 0, sizeof(head->hddThrItem.hdd_name));
		head->hddThrItem.hdd_index = INVALID_DEVMON_VALUE;
		for (i = 0; i<2; i++)
		{
			memset(head->hddThrItem.thrItem[i].tagName, 0, sizeof(head->hddThrItem.thrItem[i].tagName));
			head->hddThrItem.thrItem[i].isEnable = FALSE;
			head->hddThrItem.thrItem[i].maxThreshold = DEF_INVALID_VALUE;
			head->hddThrItem.thrItem[i].minThreshold = DEF_INVALID_VALUE;
			head->hddThrItem.thrItem[i].thresholdType = DEF_THR_UNKNOW_TYPE;
			head->hddThrItem.thrItem[i].lastingTimeS = DEF_INVALID_TIME;
			head->hddThrItem.thrItem[i].intervalTimeS = DEF_INVALID_TIME;
			head->hddThrItem.thrItem[i].checkResultValue.vi = DEF_INVALID_VALUE;
			head->hddThrItem.thrItem[i].checkResultValue.vf = DEF_INVALID_VALUE;
			head->hddThrItem.thrItem[i].checkSourceValueList.head = NULL;
			head->hddThrItem.thrItem[i].checkSourceValueList.nodeCnt = 0;
			head->hddThrItem.thrItem[i].checkType = ck_type_avg;
			head->hddThrItem.thrItem[i].repThrTime = DEF_INVALID_VALUE;
			head->hddThrItem.thrItem[i].isNormal = TRUE;
		}
	}
	return head;
}

//************************************
// Method:    CreateHDDThrNode
// Describe:  Create a node save a hdd_thr_item_t Copy  for every HardDisk, Only be called in Threshold_InitHDDThrList()
// Returns:   hdd_thr_item_node_t *
// Parameter: const char * hddName
// Parameter: int hddIndex
// Parameter: sa_thr_item_t * pRuleArray, the rull be copied, usually be the head node of HDDThrList
//************************************
hdd_thr_item_node_t * threshold_CreateHDDThrNode(const char * hddName, int hddIndex, sa_thr_item_t * pRuleArray)
{
	hdd_thr_item_node_t * node = NULL;
	node = (hdd_thr_item_node_t *)malloc(sizeof(hdd_thr_item_node_t));
	if (node)
	{
		int i = 0;
		memset(node->hddThrItem.hdd_name, 0, sizeof(node->hddThrItem.hdd_name));
		sprintf(node->hddThrItem.hdd_name, "%s", hddName);
		node->hddThrItem.hdd_index = hddIndex;
		for (i = 0; i<2; i++)
		{
			memset(node->hddThrItem.thrItem[i].tagName, 0, sizeof(node->hddThrItem.thrItem[i].tagName));
			sprintf(node->hddThrItem.thrItem[i].tagName, "%s", pRuleArray[i].tagName);
			node->hddThrItem.thrItem[i].isEnable = pRuleArray[i].isEnable;
			node->hddThrItem.thrItem[i].isNormal = TRUE;
			node->hddThrItem.thrItem[i].maxThreshold = pRuleArray[i].maxThreshold;
			node->hddThrItem.thrItem[i].minThreshold = pRuleArray[i].minThreshold;
			node->hddThrItem.thrItem[i].checkType = pRuleArray[i].checkType;
			node->hddThrItem.thrItem[i].thresholdType = pRuleArray[i].thresholdType;
			node->hddThrItem.thrItem[i].lastingTimeS = pRuleArray[i].lastingTimeS;
			node->hddThrItem.thrItem[i].intervalTimeS = pRuleArray[i].intervalTimeS;
			node->hddThrItem.thrItem[i].checkResultValue.vi = pRuleArray[i].checkResultValue.vi;
			node->hddThrItem.thrItem[i].checkResultValue.vf = pRuleArray[i].checkResultValue.vf;
			node->hddThrItem.thrItem[i].checkSourceValueList.head = NULL;
			node->hddThrItem.thrItem[i].checkSourceValueList.nodeCnt = 0;
			node->hddThrItem.thrItem[i].repThrTime = pRuleArray[i].repThrTime;
		}
		node->next = NULL;
	}
	return node;
}

//************************************
// Method:    Threshold_CreatHDDThrList
// Describe:  the head node is reserved for rule which is read from config or cmd(set), if no rule,just set bEnable = false. 
//			  if the head node of pHDDThrlist is NULL then create HDDThrlist with Default value
//			  Only be called in Threshold_InitHDDThrList()
// Access:    public
// Returns:   bool
// Parameter: hdd_thr_item_list * pHDDThrList
// Parameter: size_t cnt:the Count of HDD, 
//************************************
bool threshold_InitHDDThrList(hdd_thr_item_list * pHDDThrList, hdd_info_t * pHDDInfo)
{
	int i = 0;
	int cnt = pHDDInfo->hddCount;
	hdd_mon_info_node_t * curHDDNode = NULL;
	hdd_thr_item_node_t * preThrNode = NULL;
	hdd_thr_item_node_t * curThrNode = NULL;
	if (0 == cnt) return false;
	curHDDNode = pHDDInfo->hddMonInfoList->next;
	if (NULL == *pHDDThrList) * pHDDThrList = threshold_CreateHDDThrDefaultNode();
	preThrNode = *pHDDThrList;
	for (i = 0; i < cnt; i++){
		curThrNode = threshold_CreateHDDThrNode(curHDDNode->hddMonInfo.hdd_name, curHDDNode->hddMonInfo.hdd_index, &(*pHDDThrList)->hddThrItem.thrItem[0]);
		preThrNode->next = curThrNode;
		preThrNode = curThrNode;
		curHDDNode = curHDDNode->next;
	}
	return true;
}

//************************************
// Method:    Threshold_UninitHDDThrList
// Describe:  Destroy the hdd_thr_list
// Returns:   void
// Parameter: hdd_thr_item_list * pHDDThrList
//************************************
void threshold_UninitHDDThrList(hdd_thr_item_list * pHDDThrList)
{
	hdd_thr_item_node_t * curNode = *pHDDThrList;
	hdd_thr_item_node_t * tempNode = NULL;
	while (curNode) {
		tempNode = curNode->next;
		FreeSourceValueList(curNode->hddThrItem.thrItem[0].checkSourceValueList);
		FreeSourceValueList(curNode->hddThrItem.thrItem[1].checkSourceValueList);
		free(curNode);
		curNode = tempNode; 
	}
	*pHDDThrList = NULL;
	return;
}

//************************************
// Method:    Threshold_RenewHDDThrList
// FullName:  Threshold_RenewHDDThrList
// Describe:  the rules in HDDThrList should be update when the head node is changed.  this function just rebuild the list in simple
// Access:    public 
// Returns:   bool
// Qualifier:
// Parameter: hdd_thr_item_list * pHDDThrList
// Parameter: hdd_info_t * pHDDInfo
//************************************
bool threshold_RenewHDDThrList(hdd_thr_item_list * pHDDThrList, hdd_info_t * pHDDInfo)
{
	hdd_thr_item_node_t * pHead = *pHDDThrList;
	hdd_thr_item_node_t * pSecond = pHead->next;
	if (pSecond) threshold_UninitHDDThrList(&pSecond);
	return threshold_InitHDDThrList(&pHead, pHDDInfo);
}




sa_thr_item_t* threshold_FindItem(thr_items_t* itemlist, char const * tag)
{
	struct sa_thr_item_t *item = itemlist->hwmRule;
	struct sa_thr_item_t *target = NULL;
	if (!tag) return target;
	//printf("Find Last\n"); 
	while(item != NULL)
	{
		if(!_stricmp(item->tagName, tag))
		{
			target = item;
			break;
		}
		item = item->next;
	}
	return target;
}

sa_thr_item_t* threshold_AddThresholdItem(thr_items_t* itemlist, char * tag, float maxThreshold, float minThreshold, int thresholdType, 
	                                                            int isEnable, int lasting, int interval, char * retMsg)
{
	sa_thr_item_t* item = NULL;
	if(!tag) return item;

	item = threshold_FindItem(itemlist, tag);
	if (item) {
		//===Reset Normal
		if (false == item->isNormal && true == item->isEnable && false == isEnable){
			if (retMsg) {
				if (!retMsg[0])	sprintf(retMsg, "%s(avg:%.2f) normal!", item->tagName, item->checkResultValue.vf);
				else	 sprintf(retMsg, "%s; %s(avg:%.2f) normal!", retMsg, item->tagName, item->checkResultValue.vf);
			}
			else printf("have to send reset normal message! \r\n");
		}
	}
	if(item == NULL )
	{
		sa_thr_item_t* last = NULL;
		sa_thr_item_t* cur = itemlist->hwmRule;
		while (cur && cur->next) cur = cur->next;
		last = cur;
		item = (sa_thr_item_t*)malloc(sizeof(sa_thr_item_t));
		memset(item, 0, sizeof(sa_thr_item_t));
		if (last) last->next = item;
		else itemlist->hwmRule = item;
		itemlist->total++;
	}
	
	//============
	strcpy(item->tagName, tag);
	item->maxThreshold = maxThreshold;
	item->minThreshold = minThreshold;
	item->thresholdType = thresholdType;
	item->isEnable = isEnable;
	item->lastingTimeS = lasting;
	item->intervalTimeS = interval;

	item->checkSourceValueList.head = NULL;
	item->checkSourceValueList.nodeCnt = 0;
	item->checkType = ck_type_avg;
	item->isNormal = true;
	item->repThrTime = 0;

	printf(">Add Item: (%s, %f, %f, %d, %d, %d, %d)\n", tag, maxThreshold, minThreshold, thresholdType, isEnable, lasting, interval);

	return item;
}

bool threshold_RemoveThresholdItem(thr_items_t* thrList, char const * tag, char * retMsg)
{
	struct sa_thr_item_t *item = thrList->hwmRule;
	struct sa_thr_item_t *targetItem   = NULL;
	struct sa_thr_item_t *prevItem     = NULL;

	check_value_node_t * valueNode = NULL;
	check_value_node_t * tmpNode   = NULL;
	
	if(!tag) return false;
	
	while(item != NULL)
	{
		if(!_stricmp(item->tagName, tag))
		{
			targetItem = item;
			//=== Reset Normal==
			if (false == targetItem->isNormal && true == targetItem->isEnable ){
				if (retMsg) {
					if (!retMsg[0])	sprintf(retMsg, "%s(avg:%.2f) normal!", targetItem->tagName, targetItem->checkResultValue.vf);
					else	        sprintf(retMsg, "%s; %s(avg:%.2f) normal!", retMsg, targetItem->tagName, targetItem->checkResultValue.vf);
				}
				else printf("when delete threshold %s have to send reset normal message! \r\n", targetItem->tagName);
			}
			break;
		}
		prevItem = item;
		item = item->next;
	}
	if(! targetItem) return false;
	if(! prevItem) thrList->hwmRule = targetItem->next;
	else prevItem->next = targetItem->next;
	thrList->total--;
	printf(">Remove Item: (%s, %f, %f, %d, %d, %d, %d)\n", targetItem->tagName, targetItem->maxThreshold, targetItem->minThreshold, targetItem->thresholdType, targetItem->isEnable, targetItem->lastingTimeS, targetItem->intervalTimeS);

	valueNode = targetItem->checkSourceValueList.head;
	while(valueNode != NULL) {
		tmpNode = valueNode;
		free(tmpNode);
		valueNode = valueNode->next;
	}
	free(targetItem);
	return true;
}

bool threshold_ClearAllThresholdItem(thr_items_t* itemlist)
{
	struct sa_thr_item_t *cur = itemlist->hwmRule;
	struct sa_thr_item_t *temp = NULL;
	check_value_node_t * valueNode = NULL;

	while(cur != NULL)
	{
		temp = cur->next;
		FreeSourceValueList(cur->checkSourceValueList);
		free(cur);
		cur = temp;
	}
	itemlist->total = 0;
	itemlist->hwmRule = NULL;
	threshold_UninitHDDThrList(& itemlist->hddRule);
	return true;
}

//************************************
// Method:    Threshold_CheckHDD
// Describe:  Handler call this function to do ThresholdCheck
// Access:    public 
// Returns:   bool
// Parameter: hdd_info_t * pHDDInfo === contain curHDDInfo such as health_percent or temp
// Parameter: hdd_thr_item_list * pThrList === a list co-work with HDD list, each node contain 2 sa_thr_item_t, which save the each HDD`s a  streamhistory value
// Parameter: char * retMsg	===  Output report Message, caller shoulld provid enough char * buffer to contain
// Parameter: bool * bHealth === Output ,quick indicator of totoally normal.
//************************************
bool threshold_ChkHDD(hdd_thr_item_list HDDThrList, hdd_info_t * pHDDInfo, char * retMsg, bool *bHealth)
{
	bool bReport = false;	// Return,whether have event to be report, Default no report to be sent
	bool bNormal = true;	// temporary normal flag, the last status will pass to bHealth as Output
	char tmpMsg[20480] = { 0 };
	//int  ruleHealthIndex	= 0;	// the index of HealthH rule in sa_thr_item_t[2], the sequence is fixed when read Thr.config
	//int  ruleTempIndex	= 1;	// the index of TempH	rule in sa_thr_item_t[2], the sequence is fixed when read Thr.config

	hdd_thr_item_node_t * headThrNode = HDDThrList;
	hdd_thr_item_node_t * curThrNode = headThrNode->next;
	hdd_mon_info_node_t * curHDDNode = pHDDInfo->hddMonInfoList->next;

	if (!pHDDInfo || !HDDThrList || !curThrNode || !curHDDNode) return bReport;
	//==========================================
	while (curHDDNode && curThrNode)
	{ 
		char chkHealthMsg[1024] = { 0 };
		char chkTempMsg[1024] = { 0 };
		char hddNameStr[128] = { 0 };

		bool bHealthNormal = true;
		bool   bTempNormal = true;
		bool bHealthReport = false;
		bool   bTempReport = false;
		sa_thr_item_t * pRule = NULL;
		
		sprintf(hddNameStr, "HDD(Disk%d-%s)", curThrNode->hddThrItem.hdd_index, curThrNode->hddThrItem.hdd_name);

		if (!& curHDDNode->hddMonInfo) {  //  this may be not to happen, temp undelete
			curHDDNode = curHDDNode->next;
			curThrNode = curThrNode->next;
			continue;
		}

		if (headThrNode->hddThrItem.thrItem[0].isEnable) {
			pRule = &curThrNode->hddThrItem.thrItem[0];
			bHealthReport = threshold_ChkOneRule(pRule, curHDDNode->hddMonInfo.hdd_health_percent, chkHealthMsg, &bHealthNormal);
		}
		if (headThrNode->hddThrItem.thrItem[1].isEnable) {
			pRule = &curThrNode->hddThrItem.thrItem[1];
			bTempReport = threshold_ChkOneRule(pRule, curHDDNode->hddMonInfo.hdd_temp, chkTempMsg, &bTempNormal);
		}
		bNormal &= bHealthNormal && bTempNormal;
		if (bHealthReport || bTempReport)  {
			bReport = true;
			if (bHealthReport) {
				if (tmpMsg[0]) sprintf(tmpMsg, "%s; %s%s", tmpMsg, hddNameStr, chkHealthMsg);
				else sprintf(tmpMsg, "%s%s", hddNameStr, chkHealthMsg);
			}
			if (bTempReport) {
				if (tmpMsg[0]) sprintf(tmpMsg, "%s; %s%s", tmpMsg, hddNameStr, chkTempMsg);
				else sprintf(tmpMsg, "%s%s", hddNameStr, chkTempMsg);
			}
		}
		curHDDNode = curHDDNode->next;
		curThrNode = curThrNode->next;
	}
	*bHealth = bNormal;
	strncpy(retMsg, tmpMsg, strlen(tmpMsg) + 1);
	return bReport;
}


bool RemoveNotExistRule(thr_items_t* source, thr_items_t* target, char * retMsg)
{
	struct sa_thr_item_t *sourceItem = NULL;
	struct sa_thr_item_t *targetItem = NULL;
	struct sa_thr_item_t *       tmp = NULL;
	if (target == NULL) return false;
	targetItem = target->hwmRule;

	while (targetItem != NULL)
	{
		tmp = targetItem->next;
		if ( source->hwmRule) sourceItem = threshold_FindItem(source, targetItem->tagName);
		else sourceItem = NULL; 
		if  (sourceItem == NULL)  threshold_RemoveThresholdItem(target, targetItem->tagName, retMsg);
		targetItem = tmp;
	}
	return true;
}

bool threshold_SyncHWMRule(thr_items_t* source, thr_items_t* target, char * retMsg)
{
	struct sa_thr_item_t *item = source->hwmRule;
	struct sa_thr_item_t *current = NULL;
	RemoveNotExistRule(source, target, retMsg);
	while (item != NULL)
	{
		current = item;
		item = item->next;
		threshold_AddThresholdItem(target, current->tagName, current->maxThreshold, current->minThreshold, current->thresholdType, current->isEnable, current->lastingTimeS, current->intervalTimeS, retMsg);
	}

	return true;
}


//************************************
// Method:    threshold_SyncHDDRule
// Describe:  opreation1 : swap the pointer of source.hddHead  and target.hddHead (Notice : Body nodes are remain)
// Describe:  opreation2 : build string of Reset Normal  which will be sent to server
// Describe:  opreation3 : renew the hddThrList (discard the date stream)
// Access:    public 
// Returns:   bool
// Qualifier:
// Parameter: thr_items_t * source
// Parameter: thr_items_t * target
//************************************
bool threshold_SyncHDDRule(hdd_thr_item_list * source, hdd_thr_item_list * target, hdd_info_t * pHDDInfo, char * retMsg)
{
	//==opreation1 : swap the head pointer ( body Nodes are remained!)
	hdd_thr_item_node_t * pTargetHeadNode = *target;
	hdd_thr_item_node_t * pTargetBodyNode = (*target)->next;
	*target = *source;
	(*target)->next = pTargetBodyNode;
	*source = pTargetHeadNode;
	(*source)->next = NULL;

	//==opreation2 : build string of Reset Normal  which will be sent to server
	if (retMsg)
	{
		char nameStr[64] = { 0 };
		hdd_thr_item_node_t * headNode = *target;
		hdd_thr_item_node_t * curNode = (*target)->next;
		hdd_mon_info_node_t * curHDDNode = pHDDInfo->hddMonInfoList->next;
		while (curNode && curHDDNode)
		{
			int i = 0;
			memset(nameStr, 0, sizeof(nameStr));
			sprintf(nameStr, "HDD(Disk%d-%s)", curNode->hddThrItem.hdd_index, curNode->hddThrItem.hdd_name);
			for (i = 0; i < 2; i++) {
				sa_thr_item_t * newRule = & headNode->hddThrItem.thrItem[i];
				sa_thr_item_t * oldRule = & curNode ->hddThrItem.thrItem[i];
				if ((true  == oldRule->isEnable && false == oldRule->isNormal) &&
					(false == newRule->isEnable || NULL  == newRule->tagName ) )
				{
					if (!retMsg[0]) sprintf(retMsg, "%s %s(avg:%.2f) normal!", nameStr,  oldRule->tagName, oldRule->checkResultValue.vf);
					else sprintf(retMsg, "%s; %s %s(avg:%.2f) normal!", retMsg, nameStr, oldRule->tagName, oldRule->checkResultValue.vf);
				}
			}
			curNode    = curNode->next;
			curHDDNode = curHDDNode->next;
		}
	}
	//=====================

	//renew hddThrList
	threshold_RenewHDDThrList( target, pHDDInfo);
	return true;
}

bool CheckSourceValue(sa_thr_item_t * pThrItem, check_value_t * pCheckValue, value_type_t valueType)
{
	bool bRet = false;
	long long nowTime = time(NULL);
	//printf(">CheckSourceValue: %s\n", pThrItem->tagName);
	if(pThrItem == NULL || pCheckValue == NULL) return bRet;
	
	pThrItem->checkResultValue.vi = DEF_INVALID_VALUE;
	pThrItem->checkResultValue.vf = DEF_INVALID_VALUE;
	if(pThrItem->checkSourceValueList.head == NULL)
	{
		pThrItem->checkSourceValueList.head = (check_value_node_t *)malloc(sizeof(check_value_node_t));
		pThrItem->checkSourceValueList.nodeCnt = 0;
		pThrItem->checkSourceValueList.head->checkValueTime = DEF_INVALID_TIME;
		pThrItem->checkSourceValueList.head->ckV.vi = DEF_INVALID_VALUE;
		pThrItem->checkSourceValueList.head->ckV.vf = DEF_INVALID_VALUE;
		pThrItem->checkSourceValueList.head->next = NULL;
	}

	//== create new node
	{
		check_value_node_t * head = pThrItem->checkSourceValueList.head;
		check_value_node_t * newNode = (check_value_node_t *)malloc(sizeof(check_value_node_t));
		//printf(">Add Value Node\n");
		newNode->checkValueTime = nowTime;
		newNode->ckV.vi = DEF_INVALID_VALUE;
		newNode->ckV.vf = DEF_INVALID_VALUE;
		switch (valueType)
		{
		case value_type_float:
		{
			newNode->ckV.vf = pCheckValue->vf;
			pThrItem->checkResultValue.vf = pCheckValue->vf;
			break;
		}
		case value_type_int:
		{
			newNode->ckV.vi = pCheckValue->vi;
			pThrItem->checkResultValue.vi = pCheckValue->vi;
			break;
		}
		default: break;
		}
		newNode->next = head->next;
		head->next = newNode;
		pThrItem->checkSourceValueList.nodeCnt++;
		bRet = true;
	}

	if(pThrItem->checkSourceValueList.nodeCnt > 0)
	{
		long long minCkvTime = 0;
		check_value_node_t * curNode = pThrItem->checkSourceValueList.head->next;
		minCkvTime = curNode->checkValueTime;
		while(curNode)
		{
			if(curNode->checkValueTime < minCkvTime)  minCkvTime = curNode->checkValueTime;
			curNode = curNode->next; 
		}

		if(nowTime - minCkvTime >= pThrItem->lastingTimeS)
		{
			switch(pThrItem->checkType)
			{
			case ck_type_avg:
				{
					check_value_node_t * curNode = pThrItem->checkSourceValueList.head->next;
					float avgTmpF = 0;
					int avgTmpI = 0;
					while(curNode)
					{
						switch(valueType)
						{
						case value_type_float:
							if(curNode->ckV.vf != DEF_INVALID_VALUE) 
							{
								avgTmpF += curNode->ckV.vf;
								break;
							}
						case value_type_int:
							if(curNode->ckV.vi != DEF_INVALID_VALUE) 
							{
								avgTmpI += curNode->ckV.vi;
								break;
							}
						default: break;
						}
						curNode = curNode->next; 
					}
					if(pThrItem->checkSourceValueList.nodeCnt > 0)
					{
						switch(valueType)
						{
						case value_type_float:
							{
								avgTmpF = avgTmpF/pThrItem->checkSourceValueList.nodeCnt;
								pThrItem->checkResultValue.vf = avgTmpF;
								pThrItem->checkResultValue.vi = (int)avgTmpF;
								bRet = true;
								break;
							}
						case value_type_int:
							{
								avgTmpI = avgTmpI/pThrItem->checkSourceValueList.nodeCnt;
								pThrItem->checkResultValue.vi = avgTmpI;
								pThrItem->checkResultValue.vf = (float)avgTmpI;
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
					while(curNode)
					{
						switch(valueType)
						{
						case value_type_float:
							{
								if(curNode->ckV.vf > maxTmpF) maxTmpF = curNode->ckV.vf;
								break;
							}
						case value_type_int:
							{
								if(curNode->ckV.vi > maxTmpI) maxTmpI = curNode->ckV.vi;
								break;
							}
						default: break;
						}
						curNode = curNode->next; 
					}
					switch(valueType)
					{
					case value_type_float:
						{
							if(maxTmpF > DEF_INVALID_VALUE)
							{
								pThrItem->checkResultValue.vf = maxTmpF;
								bRet = true;
							}
							break;
						}
					case value_type_int:
						{
							if(maxTmpI > DEF_INVALID_VALUE)
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
					while(curNode)
					{
						switch(valueType)
						{
						case value_type_float:
							{
								if(curNode->ckV.vf < minTmpF) minTmpF = curNode->ckV.vf;
								break;
							}
						case value_type_int:
							{
								if(curNode->ckV.vi < minTmpI) minTmpI = curNode->ckV.vi;
								break;
							}
						default: break;
						}
						curNode = curNode->next; 
					}
					switch(valueType)
					{
					case value_type_float:
						{
							if(minTmpF < 99999)
							{
								pThrItem->checkResultValue.vf = minTmpF;
								bRet = true;
							}
							break;
						}
					case value_type_int:
						{
							if(minTmpI < 99999)
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
			}//End switch case

			{
				check_value_node_t * frontNode = pThrItem->checkSourceValueList.head;
				check_value_node_t * curNode = frontNode->next;
				check_value_node_t * delNode = NULL;
				while(curNode)
				{
					if(nowTime - curNode->checkValueTime >= pThrItem->lastingTimeS)
					{
						//printf(">Remove Value Node\n");
						delNode = curNode;
						frontNode->next  = curNode->next;
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

bool threshold_ChkOneRule(sa_thr_item_t * rule, float value, char * checkMsg, bool *checkRet)
{
	bool bReport     = false;
	bool isTrigger   = false;
	bool triggerMax  = false;
	bool triggerMin  = false;
	bool triggerBool = false;
	check_value_t checkValue;
	char tmpRetMsg[1024] = { 0 };
	char checkTypeStr[32] = { 0 };

	if (!rule)	return bReport;
	if (!rule->isEnable) return bReport;
	switch (rule->checkType)
	{
	case ck_type_avg:
		sprintf(checkTypeStr, "avg"); break;
	case ck_type_max:
		sprintf(checkTypeStr, "max"); break;
	case ck_type_min:
		sprintf(checkTypeStr, "min"); break;
	case ck_type_bool:
		sprintf(checkTypeStr, "bool");break;
	default: break;
	}
	checkValue.vi = (int)value;
	checkValue.vf = value;
	if (CheckSourceValue(rule, &checkValue, value_type_float) && rule->checkResultValue.vf != DEF_INVALID_VALUE)
	{
		float usageV = rule->checkResultValue.vf;
		if (rule->thresholdType & DEF_THR_MAX_TYPE){
			if (rule->maxThreshold != DEF_INVALID_VALUE && (usageV > rule->maxThreshold)){
				sprintf(tmpRetMsg, "%s(%s:%.2f)>maxThreshold(%.2f)", rule->tagName, checkTypeStr, usageV, rule->maxThreshold);
				triggerMax = true;
				*checkRet = false;
			}
		}
		if (rule->thresholdType & DEF_THR_MIN_TYPE) {
			if (rule->minThreshold != DEF_INVALID_VALUE && (usageV < rule->minThreshold)) {
				sprintf(tmpRetMsg, "%s(%s:%.2f)<minThreshold(%.2f)", rule->tagName, checkTypeStr, usageV, rule->minThreshold);
				triggerMin = true;
				*checkRet = false;
			}
		}
		//if(rule->thresholdType & DEF_THR_BOOL_TYPE)
		//{
		//	if(rule->minThreshold != DEF_INVALID_VALUE && (usageV != rule->maxThreshold))
		//	{
		//		if(strlen(tmpRetMsg)) sprintf(tmpRetMsg, "%s and %s(%s:%.2f)!=maxThreshold(%.2f)", tmpRetMsg, rule->tagName, checkTypeStr, usageV, rule->maxThreshold);
		//		else sprintf(tmpRetMsg, "%s(%s:%.2f)!=maxThreshold(%.2f)", rule->tagName, checkTypeStr, usageV, rule->maxThreshold);
		//		triggerBool = true;
		//	}
		//}
	}
//=========================================


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
			*checkRet = false;
			bReport = true;
		}
	}
	else if (!rule->isNormal && rule->checkResultValue.vf != DEF_INVALID_VALUE) {
		memset(tmpRetMsg, 0, sizeof(tmpRetMsg));
		sprintf(tmpRetMsg, "%s normal!", rule->tagName);
		rule->isNormal = true;
		*checkRet = true;
		bReport = true;

	}
	if (!bReport) memset(checkMsg, 0, sizeof(checkMsg));
	else strncpy(checkMsg, tmpRetMsg, strlen(tmpRetMsg) + 1);
	
	return bReport;
}