#include "process_common.h"
#include "process_threshold.h"
#include "ProcessMonitorHandler.h"
#include "ProcessMonitorLog.h"

const char g_tagNameSet[THR_TAG_NAME_COUNT][64] = {{SWM_THR_CPU},{SWM_THR_MEM}};


void SendNormalMsg(bool isTotalNormal, char *pNormalMsg)
{
		//char normalMsg[10] = {0};
	swm_thr_rep_info_t thrRepInfo;
	thrRepInfo.repInfo = NULL;
	thrRepInfo.isTotalNormal = isTotalNormal;
	{
		if(pNormalMsg)
		{
			int normalMsgLen = strlen(pNormalMsg) + 1;
			thrRepInfo.repInfo = (char *)malloc(sizeof(char)* normalMsgLen);
			memset(thrRepInfo.repInfo, 0, sizeof(char)* normalMsgLen);
			sprintf_s(thrRepInfo.repInfo, sizeof(char)*normalMsgLen, "%s", pNormalMsg);
		}
		else
		{
			thrRepInfo.repInfo = (char *)malloc(sizeof(LONG));
			memset(thrRepInfo.repInfo, 0, sizeof(LONG));
		}
	}
	{
		char * uploadRepJsonStr = NULL;
		char * str = (char *)&thrRepInfo;
		int jsonStrlen = Parser_swm_mon_prc_event_rep(str, &uploadRepJsonStr);
		if(jsonStrlen > 0 && uploadRepJsonStr != NULL)
		{
			g_sendcbf(&g_PluginInfo, swm_mon_prc_event_rep, uploadRepJsonStr, strlen(uploadRepJsonStr)+1, NULL, NULL);
		}
		if(uploadRepJsonStr)free(uploadRepJsonStr);	
	}
	if(thrRepInfo.repInfo) free(thrRepInfo.repInfo);
}

bool ExistInTmpMonObjInfoList(mon_obj_info_list tmpMonObjInfoList, mon_obj_info_node_t * checkNode, bool isCopy)
{
	if(tmpMonObjInfoList == NULL ||  checkNode == NULL) return false;
	{
		mon_obj_info_node_t * curNode = NULL;
		curNode = tmpMonObjInfoList->next;
		while(curNode)
		{
			if(checkNode->monObjInfo.thrItem.isEnable && !strcmp(curNode->monObjInfo.path,checkNode->monObjInfo.path)) //!strcmp(delNode->monObjInfo.thrItem.tagName, tagName))
			{
				//Pass isNormal value used for threshold change.
				if(isCopy)
				{
					curNode->monObjInfo.thrItem.maxThreshold				= checkNode->monObjInfo.thrItem.maxThreshold;
					curNode->monObjInfo.thrItem.minThreshold				= checkNode->monObjInfo.thrItem.minThreshold;
					curNode->monObjInfo.thrItem.thresholdType				= checkNode->monObjInfo.thrItem.thresholdType;
					curNode->monObjInfo.thrItem.lastingTimeS				= checkNode->monObjInfo.thrItem.lastingTimeS;
					curNode->monObjInfo.thrItem.intervalTimeS				= checkNode->monObjInfo.thrItem.intervalTimeS;
					curNode->monObjInfo.thrItem.isEnable					= checkNode->monObjInfo.thrItem.isEnable;
					curNode->monObjInfo.thrItem.checkType					= checkNode->monObjInfo.thrItem.checkType;
					//curNode->monObjInfo.thrItem.isNormal					= checkNode->monObjInfo.thrItem.isNormal;
					//curNode->monObjInfo.thrItem.repThrTime				= checkNode->monObjInfo.thrItem.repThrTime;
					//curNode->monObjInfo.thrItem.checkSourceValueList.nodeCnt = checkNode->monObjInfo.thrItem.checkSourceValueList.nodeCnt;
					//curNode->monObjInfo.thrItem.checkSourceValueList.head	= checkNode->monObjInfo.thrItem.checkSourceValueList.head;
					//checkNode->monObjInfo.thrItem.checkSourceValueList.head = NULL;
					//curNode->monObjInfo.thrItem.checkResultValue.vi		= checkNode->monObjInfo.thrItem.checkResultValue.vi;
					curNode->monObjInfo.act									= checkNode->monObjInfo.act;
					if(curNode->monObjInfo.actCmd) 
						free(curNode->monObjInfo.actCmd);
					curNode->monObjInfo.actCmd								= checkNode->monObjInfo.actCmd?strdup(checkNode->monObjInfo.actCmd):NULL;
					curNode->monObjInfo.isHasPID							= checkNode->monObjInfo.isHasPID;
					curNode->monObjInfo.prcID								= checkNode->monObjInfo.prcID;
				}
				return true;
			}
			curNode = curNode->next;
		}
	}
	return false;
}

void SWMWhenDelThrCheckNormal(mon_obj_info_list monObjInfoList, mon_obj_info_list tmpMonObjInfoList) 
{
	if(NULL == monObjInfoList || NULL == tmpMonObjInfoList) return;
	{
		mon_obj_info_node_t * curNode = NULL;
		char * pRepMsg = NULL;
		int repBufLen = 0;
		int repMsgLen = 0;
		char tmpMsg[512] = {0};
		curNode = monObjInfoList->next;
		while(curNode)
		{
			if(!ExistInTmpMonObjInfoList(tmpMonObjInfoList, curNode, false))
			{
				mon_obj_info_node_t * tmpCurNode = NULL;
				if(curNode->monObjInfo.thrItem.isEnable && !curNode->monObjInfo.thrItem.isNormal)
				{
					int len = 0;
					memset(tmpMsg, 0, sizeof(tmpMsg));
					if(curNode->monObjInfo.prcID < 0)sprintf_s(tmpMsg, sizeof(tmpMsg), "System performance #tk#%s usage back to normal#tk#", //curNode->monObjInfo.prcName, curNode->monObjInfo.prcID, 
						curNode->monObjInfo.thrItem.tagName);
					else sprintf_s(tmpMsg, sizeof(tmpMsg), "Process(PrcName:%s, PrcID:%d) #tk#%s usage back to normal#tk#", curNode->monObjInfo.prcName, 
						curNode->monObjInfo.prcID, curNode->monObjInfo.thrItem.tagName);
					curNode->monObjInfo.thrItem.isNormal = TRUE;
			
					if(strlen(tmpMsg))
						pRepMsg = (char *)DynamicStrCat(pRepMsg, &repBufLen,";", tmpMsg);
					//Delete threshold node from old list which don't exist in new list. 
				}
				tmpCurNode = curNode;
				curNode = curNode->next;
				DeleteMonObjInfoNodeWithID(monObjInfoList, tmpCurNode->monObjInfo.prcID, tmpCurNode->monObjInfo.thrItem.tagName);
				continue;
			}
			curNode = curNode->next;
		}
		if(pRepMsg)
		{
			BOOL isSWMNormal = TRUE;
			if(IsSWMThrNormal(&isSWMNormal))
			{
				SendNormalMsg(isSWMNormal,pRepMsg);
				free(pRepMsg);
			}
		}
	}
}

static CAGENT_PTHREAD_ENTRY(SetSWMThrThreadStart, args)
{
	char repMsg[1024] = {0};
	BOOL bRet = FALSE;
	mon_obj_info_list tmpMonObjList = NULL;
	//Set Server total normal message as true.
	//SendNormalMsg(TRUE,NULL);
	tmpMonObjList = CreateMonObjInfoList();
	bRet = InitMonObjInfoListFromConfig(tmpMonObjList, MonObjTmpConfigPath);
	if(!bRet)
	{
		sprintf(repMsg, "%s", "SWM threshold apply failed!");
	}
	else
	{
		mon_obj_info_list monObjList = SWMonHandlerContext.monObjInfoList; 
		app_os_EnterCriticalSection(&SWMonHandlerContext.swMonObjCS);
		if(monObjList && tmpMonObjList) 
		{
			//SWMWhenDelThrSetToNormal(monObjList);
			/*Delete threshold node from old list which don't exist in new list.*/
			SWMWhenDelThrCheckNormal(monObjList,tmpMonObjList);
			//DeleteAllMonObjInfoNode(monObjList);
		}
		{
			mon_obj_info_node_t *curMonObjNode = tmpMonObjList->next;
			//if(!monObjList) monObjList = CreateMonObjInfoList();
			while(curMonObjNode)
			{
				if(!ExistInTmpMonObjInfoList(monObjList, curMonObjNode, true))
					InsertMonObjInfoList(monObjList, &curMonObjNode->monObjInfo);
				else
					
				curMonObjNode = curMonObjNode->next;
			}
			//MonObjUpdate();  //Don't update monObjList, else same name proc node will add in. Update it at analysing time.
		}
		SetThresholdConfigFile(monObjList, MonObjConfigPath);
		app_os_LeaveCriticalSection(&SWMonHandlerContext.swMonObjCS);
		sprintf(repMsg, "SWM threshold apply OK!");
	}
	if(tmpMonObjList) DestroyMonObjInfoList(tmpMonObjList);
	if(strlen(repMsg))
	{
			char * uploadRepJsonStr = NULL;
			char * str = repMsg;
			int jsonStrlen = Parser_string(str, &uploadRepJsonStr,swm_set_mon_objs_rep);
			if(jsonStrlen > 0 && uploadRepJsonStr != NULL)
			{
				g_sendcbf(&g_PluginInfo, swm_set_mon_objs_rep, uploadRepJsonStr, strlen(uploadRepJsonStr)+1, NULL, NULL);
			}
			if(uploadRepJsonStr)free(uploadRepJsonStr);
	}

	//SendNormalMsg(TRUE,NULL);//Complete in function SWMWhenDelThrCheckNormal().
	remove(MonObjTmpConfigPath);
#ifdef WIN32
	app_os_CloseHandle(SetSWMThrThreadHandle);
	SetSWMThrThreadHandle = NULL;
#else
	pthread_detach(pthread_self());
#endif
	SetSWMThrThreadRunning = FALSE;
	return 0;
}

void SWMSetMonObjs(sw_mon_handler_context_t * pSWMonHandlerContext, char * requestData)
{
	if(requestData == NULL || pSWMonHandlerContext == NULL) return;
	{
		char repMsg[256] = {0};
		if(!SetSWMThrThreadRunning)
		{
			FILE * fPtr = fopen(MonObjTmpConfigPath, "wb");
			if(fPtr)
			{
				fwrite(requestData, 1, strlen(requestData)+1, fPtr);
				fclose(fPtr);
			}
			SetSWMThrThreadRunning = TRUE;
			if (app_os_thread_create(&SetSWMThrThreadHandle, SetSWMThrThreadStart, NULL) != 0)
			{
				SetSWMThrThreadRunning = FALSE;
				sprintf(repMsg, "%s", "Set swm threshold thread start error!");
				remove(MonObjTmpConfigPath);
			}
		}
		else
		{
			sprintf(repMsg, "%s", "Set swm threshold thread running!");
		}

		if(strlen(repMsg))
		{
			{
				char * uploadRepJsonStr = NULL;
				char * str = repMsg;
				int jsonStrlen = Parser_string(str, &uploadRepJsonStr,swm_set_mon_objs_rep);
				if(jsonStrlen > 0 && uploadRepJsonStr != NULL)
				{
					g_sendcbf(&g_PluginInfo, swm_set_mon_objs_rep, uploadRepJsonStr, strlen(uploadRepJsonStr)+1, NULL, NULL);
				}
				if(uploadRepJsonStr)free(uploadRepJsonStr);	
			}
		}
	}
}

BOOL MonObjUpdate()
{
	BOOL bRet = FALSE;
	sw_mon_handler_context_t *pSoftwareMonHandlerContext = (sw_mon_handler_context_t *)&SWMonHandlerContext;
	if(pSoftwareMonHandlerContext == NULL || pSoftwareMonHandlerContext->prcMonInfoList == NULL || 
		pSoftwareMonHandlerContext->monObjInfoList == NULL) return bRet;
	{
		mon_obj_info_node_t * curMonObjInfoNode = NULL;
		prc_mon_info_node_t * curPrcMonInfoNode = pSoftwareMonHandlerContext->prcMonInfoList->next;
		while(curPrcMonInfoNode)
		{
			int i = 0;
			app_os_EnterCriticalSection(&pSoftwareMonHandlerContext->swPrcMonCS);
			for(i = 0; i < THR_TAG_NAME_COUNT; i++)
			{
				curMonObjInfoNode = FindMonObjInfoNodeWithID(pSoftwareMonHandlerContext->monObjInfoList, curPrcMonInfoNode->prcMonInfo.prcID,curPrcMonInfoNode->prcMonInfo.prcName,g_tagNameSet[i]);
				if(curMonObjInfoNode)
				{
					if(curMonObjInfoNode->monObjInfo.thrItem.isEnable&& curMonObjInfoNode->monObjInfo.isValid == 0) 
						curMonObjInfoNode->monObjInfo.isValid = 1;
				}
				else
				{
					curMonObjInfoNode = FindMonObjInfoNode(pSoftwareMonHandlerContext->monObjInfoList, curPrcMonInfoNode->prcMonInfo.prcName,g_tagNameSet[i]);
					if(curMonObjInfoNode)
					{
						//if(curMonObjInfoNode->monObjInfo.isHasPID) break;
						if(!curMonObjInfoNode->monObjInfo.isHasPID  && curMonObjInfoNode->monObjInfo.thrItem.isEnable)
						{
							/*if(curMonObjInfoNode->monObjInfo.prcID == 0 || curMonObjInfoNode->monObjInfo.isValid == 0) 
							{
								curMonObjInfoNode->monObjInfo.prcID = curPrcMonInfoNode->prcMonInfo.prcID;
								curMonObjInfoNode->monObjInfo.isValid = 1;
								ClearMonObjThr(&curMonObjInfoNode->monObjInfo);
							}
							else
							{*/
								mon_obj_info_t monObjInfo;
								memset(&monObjInfo, 0, sizeof(mon_obj_info_t));
								monObjInfo.isValid = 1;
								if(curMonObjInfoNode->monObjInfo.path)
								{
									int pathLen = strlen(curMonObjInfoNode->monObjInfo.path)+1;
									monObjInfo.path = (char *)malloc(pathLen);
									memset(monObjInfo.path, 0, pathLen);
									strcpy(monObjInfo.path, curMonObjInfoNode->monObjInfo.path);
								}
								//if(curMonObjInfoNode->monObjInfo.prcName)
								{
									int prcNameLen = strlen(curMonObjInfoNode->monObjInfo.prcName)+1;
									monObjInfo.prcName = (char *)malloc(prcNameLen);
									memset(monObjInfo.prcName, 0, prcNameLen);
									strcpy(monObjInfo.prcName, curMonObjInfoNode->monObjInfo.prcName);
								}
								monObjInfo.prcID = curPrcMonInfoNode->prcMonInfo.prcID;
								monObjInfo.act = curMonObjInfoNode->monObjInfo.act;
								if(curMonObjInfoNode->monObjInfo.actCmd)
								{
									int actCmdLen = strlen(curMonObjInfoNode->monObjInfo.actCmd)+1;
									monObjInfo.actCmd = (char *)malloc(actCmdLen);
									memset(monObjInfo.actCmd, 0, actCmdLen);
									strcpy(monObjInfo.actCmd, curMonObjInfoNode->monObjInfo.actCmd);
								}

								//////set thritem
								memcpy(&(monObjInfo.thrItem), &(curMonObjInfoNode->monObjInfo.thrItem), sizeof(swm_thr_item_t));
								strcpy(monObjInfo.thrItem.tagName, g_tagNameSet[i]);
								monObjInfo.thrItem.isEnable = TRUE;
								monObjInfo.thrItem.checkResultValue.vi = DEF_INVALID_VALUE;
								monObjInfo.thrItem.checkSourceValueList.head = NULL;
								monObjInfo.thrItem.checkSourceValueList.nodeCnt = 0;
								monObjInfo.thrItem.repThrTime = 0;
								monObjInfo.thrItem.isNormal = TRUE;

								InsertMonObjInfoList(pSoftwareMonHandlerContext->monObjInfoList, &monObjInfo);
								if(monObjInfo.path) free(monObjInfo.path);
								if(monObjInfo.prcName) free(monObjInfo.prcName);
								if(monObjInfo.actCmd) free(monObjInfo.actCmd);
							//}
						}
					}
				}
			}
			app_os_LeaveCriticalSection(&pSoftwareMonHandlerContext->swPrcMonCS);
			curPrcMonInfoNode = curPrcMonInfoNode->next;
			app_os_sleep(10);
		}
	}

	return bRet = TRUE;
}

BOOL IsSWMThrNormal(BOOL * isNormal)
{
	BOOL bRet = FALSE;
	sw_mon_handler_context_t *pSoftwareMonHandlerContext = (sw_mon_handler_context_t *)&SWMonHandlerContext;
	if(isNormal == NULL || pSoftwareMonHandlerContext == NULL || pSoftwareMonHandlerContext->monObjInfoList == NULL) return bRet;
	{
		mon_obj_info_node_t * curMonObjInfoNode = pSoftwareMonHandlerContext->monObjInfoList->next;
		*isNormal = TRUE;
		while(curMonObjInfoNode)
		{
			if(curMonObjInfoNode->monObjInfo.isValid)
			{
				if((curMonObjInfoNode->monObjInfo.thrItem.isEnable && !curMonObjInfoNode->monObjInfo.thrItem.isNormal))
					//|| (curMonObjInfoNode->monObjInfo.memThrItem.isEnable && !curMonObjInfoNode->monObjInfo.memThrItem.isNormal))
				{
					*isNormal = FALSE;
					break;
				}
				if(!curMonObjInfoNode->monObjInfo.prcResponse)
				{
					*isNormal = FALSE;
					break;
				}
			}
			curMonObjInfoNode = curMonObjInfoNode->next;
		}
	}
	return bRet = TRUE;
}

void SWMWhenDelThrSetToNormal(mon_obj_info_list monObjInfoList) 
{
	if(NULL == monObjInfoList) return;
	{
		mon_obj_info_node_t * curNode = NULL;
		//char tmpRepMsg[1024*2] = {0};
		char * pRepMsg = NULL;
		int repBufLen = 0;
		int repMsgLen = 0;
		char tmpMsg[512] = {0};
		curNode = monObjInfoList->next;
		while(curNode)
		{
			memset(tmpMsg, 0, sizeof(tmpMsg));
			if(curNode->monObjInfo.thrItem.isEnable && !curNode->monObjInfo.thrItem.isNormal)
			{
				int len = 0;
				//if(strlen(tmpMsg))sprintf_s(tmpMsg, sizeof(tmpMsg), "%s, #tk#%s usage back to normal#tk#", tmpMsg, curNode->monObjInfo.thrItem.tagName);
				//else 
				if(curNode->monObjInfo.prcID < 0)sprintf_s(tmpMsg, sizeof(tmpMsg), "System performance #tk#%s usage back to normal#tk#", //curNode->monObjInfo.prcName, curNode->monObjInfo.prcID, 
					curNode->monObjInfo.thrItem.tagName);
				else sprintf_s(tmpMsg, sizeof(tmpMsg), "Process(PrcName:%s, PrcID:%d) #tk#%s usage back to normal#tk#", curNode->monObjInfo.prcName, 
					curNode->monObjInfo.prcID, curNode->monObjInfo.thrItem.tagName);
				curNode->monObjInfo.thrItem.isNormal = TRUE;
			
				if(strlen(tmpMsg))
					pRepMsg = (char *)DynamicStrCat(pRepMsg, &repBufLen,";", tmpMsg);
			}
			curNode = curNode->next;
		}
		if(pRepMsg)
		{
			BOOL isSWMNormal = TRUE;
			if(IsSWMThrNormal(&isSWMNormal))
			{
				SendNormalMsg(isSWMNormal,pRepMsg);
				free(pRepMsg);
			}
		}
		/*if(tmpRepMsg && strlen(tmpRepMsg))
		{
			swm_thr_rep_info_t thrRepInfo;
			int len = strlen(tmpRepMsg) + 1;
			thrRepInfo.isTotalNormal = TRUE;
			thrRepInfo.repInfo = (char *)malloc(len);
			memset(thrRepInfo.repInfo, 0, sizeof(thrRepInfo.repInfo));
			strcpy(thrRepInfo.repInfo, tmpRepMsg);
			{
				char * uploadRepJsonStr = NULL;
				char * str =  (char *)&thrRepInfo;
				int jsonStrlen = Parser_swm_mon_prc_event_rep(str, &uploadRepJsonStr);
				if(jsonStrlen > 0 && uploadRepJsonStr != NULL)
				{
					g_sendcbf(&g_PluginInfo, swm_mon_prc_event_rep, uploadRepJsonStr, strlen(uploadRepJsonStr)+1, NULL, NULL);
				}
				if(uploadRepJsonStr)free(uploadRepJsonStr);	
			}
			if(thrRepInfo.repInfo) free(thrRepInfo.repInfo);
		}
		if(tmpRepMsg) free(tmpRepMsg);*/
	}
}


void SWMDelAllMonObjs()
{
	char repMsg[32] = {0};
	sw_mon_handler_context_t *pSWMonHandlerContext =  &SWMonHandlerContext;
	app_os_EnterCriticalSection(&pSWMonHandlerContext->swMonObjCS);
	if(pSWMonHandlerContext->monObjInfoList)
	{
		SWMWhenDelThrSetToNormal(pSWMonHandlerContext->monObjInfoList);
		DeleteAllMonObjInfoNode(pSWMonHandlerContext->monObjInfoList);
	}
	SetThresholdConfigFile(SWMonHandlerContext.monObjInfoList, MonObjConfigPath);
	app_os_LeaveCriticalSection(&pSWMonHandlerContext->swMonObjCS);
	sprintf_s(repMsg, sizeof(repMsg), "%s", "Success");
	{
		char * uploadRepJsonStr = NULL;
		char * str = repMsg;
		int jsonStrlen = Parser_string(str, &uploadRepJsonStr,swm_del_all_mon_objs_rep);
		if(jsonStrlen > 0 && uploadRepJsonStr != NULL)
		{
			g_sendcbf(&g_PluginInfo, swm_del_all_mon_objs_rep, uploadRepJsonStr, strlen(uploadRepJsonStr)+1, NULL, NULL);
		}
		if(uploadRepJsonStr)free(uploadRepJsonStr);	
	}
}

mon_obj_info_node_t * CreateMonObjInfoList()
{
	mon_obj_info_node_t * head = NULL;
	head = (mon_obj_info_node_t *)malloc(sizeof(mon_obj_info_node_t));
	if(head) 
	{
		head->next = NULL;
		head->monObjInfo.isValid = TRUE;
		head->monObjInfo.prcName = NULL;
		head->monObjInfo.prcID = 0;
		head->monObjInfo.prcResponse = TRUE;
		memset(head->monObjInfo.thrItem.tagName, 0, sizeof(head->monObjInfo.thrItem.tagName));
		head->monObjInfo.thrItem.maxThreshold = DEF_INVALID_VALUE;
		head->monObjInfo.thrItem.minThreshold = DEF_INVALID_VALUE;
		head->monObjInfo.thrItem.thresholdType = DEF_THR_UNKNOW_TYPE;
		head->monObjInfo.thrItem.lastingTimeS = DEF_INVALID_TIME;
		head->monObjInfo.thrItem.intervalTimeS = DEF_INVALID_TIME;
		head->monObjInfo.thrItem.isEnable = FALSE;
		head->monObjInfo.thrItem.checkType = ck_type_unknow;
		head->monObjInfo.thrItem.checkSourceValueList.head = NULL;
		head->monObjInfo.thrItem.checkSourceValueList.nodeCnt = 0;
		head->monObjInfo.thrItem.checkResultValue.vi = DEF_INVALID_VALUE;
		head->monObjInfo.thrItem.isNormal = TRUE;
		head->monObjInfo.thrItem.repThrTime = 0;
		head->monObjInfo.act = prc_act_unknown;
		head->monObjInfo.actCmd = NULL;
	}
	return head;
}

BOOL InitMonObjInfoListFromConfig(mon_obj_info_list monObjList, char *monObjConfigPath)
{
	BOOL bRet = FALSE;
	FILE *fptr = NULL;
	char * pTmpThrInfoStr = NULL;
	if(monObjList == NULL || monObjConfigPath == NULL) return bRet;
	if ((fptr = fopen(monObjConfigPath, "rb")) == NULL) return bRet;
	//{
	//	ProcessMonitorLog(g_loghandle, Normal, "%s()[%d]###Error: open %s failed!\n", __FUNCTION__, __LINE__, monObjConfigPath);
	//	return bRet;
	//}
	{
		unsigned int fileLen = 0;
		fseek(fptr, 0, SEEK_END);
		fileLen = ftell(fptr);
		if(fileLen > 0)
		{
			unsigned int readLen = fileLen + 1, realReadLen = 0;
			pTmpThrInfoStr = (char *) malloc(readLen);
			memset(pTmpThrInfoStr, 0, readLen);
			fseek(fptr, 0, SEEK_SET);
			realReadLen =  fread(pTmpThrInfoStr, sizeof(char), readLen, fptr);
			if(realReadLen > 0)
			{
				char *monStr = (char*)malloc(sizeof(mon_obj_info_list));
				memcpy(monStr, (char *)&monObjList, sizeof(mon_obj_info_list));
				bRet = Parse_swm_set_mon_objs_req(pTmpThrInfoStr, monStr);
				free(monStr);//Wei.Gang add to fix  mem leak.
			}
			if(pTmpThrInfoStr) free(pTmpThrInfoStr);
		}
	}
	fclose(fptr);
	return bRet;
}

void ClearMonObjThr(mon_obj_info_t * curMonObj)
{
	if(curMonObj != NULL && curMonObj->thrItem.checkSourceValueList.head)
	{
		check_value_node_t * frontValueNode = curMonObj->thrItem.checkSourceValueList.head;
		check_value_node_t * delValueNode = frontValueNode->next;
		while(delValueNode)
		{
			frontValueNode->next = delValueNode->next;
			free(delValueNode);
			delValueNode = frontValueNode->next;
		}
		curMonObj->thrItem.checkSourceValueList.nodeCnt = 0;
		curMonObj->thrItem.checkResultValue.vi = DEF_INVALID_VALUE;
		curMonObj->thrItem.repThrTime = 0;
		curMonObj->thrItem.isNormal = TRUE;
	}
}

int DeleteAllMonObjInfoNode(mon_obj_info_node_t * head)
{
	int iRet = -1;
	mon_obj_info_node_t * delNode = NULL;
	if(head == NULL) return iRet;

	delNode = head->next;
	while(delNode)
	{
		head->next = delNode->next;
		if(delNode->monObjInfo.path)
		{
			free(delNode->monObjInfo.path);
			delNode->monObjInfo.path = NULL;
		}
		if(delNode->monObjInfo.prcName)
		{
			free(delNode->monObjInfo.prcName);
			delNode->monObjInfo.prcName = NULL;
		}
		if(delNode->monObjInfo.actCmd)
		{
			free(delNode->monObjInfo.actCmd);
			delNode->monObjInfo.actCmd = NULL;
		}

		ClearMonObjThr(&delNode->monObjInfo);
		if(delNode->monObjInfo.thrItem.checkSourceValueList.head)
		{
			free(delNode->monObjInfo.thrItem.checkSourceValueList.head);
			delNode->monObjInfo.thrItem.checkSourceValueList.head = NULL;
		}

		free(delNode);
		delNode = head->next;
	}
	head->next = NULL;
	iRet = 0;
	return iRet;
}

mon_obj_info_node_t * FindMonObjInfoNode(mon_obj_info_node_t * head, char * prcName , char * tagName)
{
	mon_obj_info_node_t * findNode = NULL;
	if(head == NULL || prcName == NULL) return findNode;
	findNode = head->next;
	while(findNode)
	{
		if(!strcmp(findNode->monObjInfo.prcName, prcName) 
			&& !strcmp(findNode->monObjInfo.thrItem.tagName, tagName) 
			&& !findNode->monObjInfo.isHasPID) 
			//&& findNode->monObjInfo.prcID == THR_PRC_SAME_NAME_ALL_FLAG) 
			break;
		else
		{
			findNode = findNode->next;
		}
	}

	return findNode;
}


mon_obj_info_node_t * FindMonObjInfoNodeWithID(mon_obj_info_node_t * head, int prcID , char * prcName,char * tagName)
{
	mon_obj_info_node_t * findNode = NULL;
	if(head == NULL || prcID == 0 || tagName == NULL) return findNode;
	findNode = head->next;
	while(findNode)
	{
		if(findNode->monObjInfo.prcID == prcID && !strcmp(findNode->monObjInfo.prcName, prcName) 
			&& !strcmp(findNode->monObjInfo.thrItem.tagName, tagName)) break;
		else
		{
			findNode = findNode->next;
		}
	}

	return findNode;
}

int InsertMonObjInfoList(mon_obj_info_node_t * head, mon_obj_info_t * monObjInfo)
{
	int iRet = -1;
	mon_obj_info_node_t * newNode = NULL, * findNode = NULL;
	if(monObjInfo == NULL || monObjInfo->prcName == NULL || head == NULL) return iRet;
	findNode = FindMonObjInfoNodeWithID(head, monObjInfo->prcID, monObjInfo->prcName, monObjInfo->thrItem.tagName);
	if(findNode == NULL)
	{
		newNode = (mon_obj_info_node_t *)malloc(sizeof(mon_obj_info_node_t));
		memset(newNode, 0, sizeof(mon_obj_info_node_t));
		newNode->monObjInfo.isValid = 1;
		if(monObjInfo->path)
		{
			int tmpLen = strlen(monObjInfo->path);
			newNode->monObjInfo.path = (char *)malloc(tmpLen + 1);
			memset(newNode->monObjInfo.path, 0, tmpLen + 1);
			memcpy(newNode->monObjInfo.path, monObjInfo->path, tmpLen);
		}

		if(monObjInfo->prcName)
		{
			int tmpLen = strlen(monObjInfo->prcName);
			newNode->monObjInfo.prcName = (char *)malloc(tmpLen + 1);
			memset(newNode->monObjInfo.prcName, 0, tmpLen + 1);
			memcpy(newNode->monObjInfo.prcName, monObjInfo->prcName, tmpLen);
		}

		newNode->monObjInfo.prcID = monObjInfo->prcID;
		newNode->monObjInfo.prcResponse = TRUE;

		memcpy(&(newNode->monObjInfo.thrItem), &(monObjInfo->thrItem), sizeof(swm_thr_item_t));

		newNode->monObjInfo.act = monObjInfo->act;
		if(monObjInfo->actCmd)
		{
			int tmpLen = strlen(monObjInfo->actCmd);
			newNode->monObjInfo.actCmd = (char *)malloc(tmpLen + 1);
			memset(newNode->monObjInfo.actCmd, 0, tmpLen + 1);
			memcpy(newNode->monObjInfo.actCmd, monObjInfo->actCmd, tmpLen);
		}
		newNode->monObjInfo.isHasPID = monObjInfo->isHasPID;
		newNode->next = head->next;
		head->next = newNode;
		iRet = 0;
	}
	else
	{
		iRet = 1;
	}
	return iRet;
}

void SetThresholdConfigFile(mon_obj_info_list monObjList, char *monObjConfigPath)
{
	if(monObjList == NULL || monObjConfigPath == NULL) return;
	{
		char * pJsonStr = NULL;
		int jsonLen = 0;
		int dataLen = sizeof(mon_obj_info_list);
		int sendLen = sizeof(susi_comm_data_t) + dataLen;
		char * sendData = (char *)malloc(sendLen);
		susi_comm_data_t *pSusiCommData = (susi_comm_data_t *) sendData;
		pSusiCommData->comm_Cmd = swm_set_mon_objs_req;
#ifdef COMM_DATA_WITH_JSON
		pSusiCommData->reqestID = cagent_request_software_monitoring;
#endif
		pSusiCommData->message_length = dataLen;
		memcpy(pSusiCommData->message, &monObjList, dataLen);

		jsonLen = Pack_swm_set_mon_objs_req(pSusiCommData, &pJsonStr);
		if(jsonLen > 0 && pJsonStr != NULL)
		{
			FILE * fPtr = fopen(monObjConfigPath, "wb");
			if(fPtr)
			{
				fwrite(pJsonStr, 1, jsonLen, fPtr);
				fclose(fPtr);
			}
			free(pJsonStr);
		}
		if(sendData) free(sendData);
	}
}

void DestroyMonObjInfoList(mon_obj_info_node_t * head)
{
	if(NULL == head) return;
	DeleteAllMonObjInfoNode(head);
	free(head); 
	head = NULL;
}

int DeleteMonObjInfoNodeWithID(mon_obj_info_node_t * head, int prcID ,char * tagName)
{
	int iRet = -1;
	mon_obj_info_node_t * delNode = NULL;
	mon_obj_info_node_t * p = NULL;
	if(head == NULL) return iRet;
	iRet = 1;
	p = head;
	delNode = head->next;
	while(delNode)
	{
		if(delNode->monObjInfo.prcID == prcID && !strcmp(delNode->monObjInfo.thrItem.tagName, tagName))
		{
			p->next = delNode->next;
			if(delNode->monObjInfo.path)
			{
				free(delNode->monObjInfo.path);
				delNode->monObjInfo.path = NULL;
			}
			if(delNode->monObjInfo.prcName)
			{
				free(delNode->monObjInfo.prcName);
				delNode->monObjInfo.prcName = NULL;
			}
			if(delNode->monObjInfo.actCmd)
			{
				free(delNode->monObjInfo.actCmd);
				delNode->monObjInfo.actCmd = NULL;
			}
			/*if(delNode->monObjInfo.memActCmd)
			{
				free(delNode->monObjInfo.memActCmd);
				delNode->monObjInfo.memActCmd = NULL;
			}*/
			ClearMonObjThr(&delNode->monObjInfo);
			if(delNode->monObjInfo.thrItem.checkSourceValueList.head)
			{
				free(delNode->monObjInfo.thrItem.checkSourceValueList.head);
				delNode->monObjInfo.thrItem.checkSourceValueList.head = NULL;
			}
			/*if(delNode->monObjInfo.memThrItem.checkSourceValueList.head)
			{
				free(delNode->monObjInfo.memThrItem.checkSourceValueList.head);
				delNode->monObjInfo.memThrItem.checkSourceValueList.head = NULL;
			}*/
			free(delNode);
			delNode = p->next;
			iRet = 0;
			continue;
		}
		else
		{
			p = delNode;
			delNode = delNode->next;
		}
	}
	return iRet;
}

static BOOL CheckSourceValue(hwm_thr_item_t * pThrItem, check_value_t * pCheckValue, value_type_t valueType)
{
	BOOL bRet = FALSE;
	if(pThrItem == NULL || pCheckValue == NULL) return bRet;
	{
		long long nowTime = time(NULL);
		pThrItem->checkResultValue.vi = DEF_INVALID_VALUE;
		if(pThrItem->checkSourceValueList.head == NULL)
		{
			pThrItem->checkSourceValueList.head = (check_value_node_t *)malloc(sizeof(check_value_node_t));
			pThrItem->checkSourceValueList.nodeCnt = 0;
			pThrItem->checkSourceValueList.head->checkValueTime = DEF_INVALID_TIME;
			pThrItem->checkSourceValueList.head->ckV.vi = DEF_INVALID_VALUE;
			pThrItem->checkSourceValueList.head->next = NULL;
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
							if(curNode->ckV.vi != DEF_INVALID_VALUE) 
							{
								switch(valueType)
								{
								case value_type_float:
									{
										avgTmpF += curNode->ckV.vf;
										break;
									}
								case value_type_int:
									{
										avgTmpI += curNode->ckV.vi;
										break;
									}
								default: break;
								}
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
									bRet = TRUE;
									break;
								}
							case value_type_int:
								{
									avgTmpI = avgTmpI/pThrItem->checkSourceValueList.nodeCnt;
									pThrItem->checkResultValue.vi = avgTmpI;
									bRet = TRUE;
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
						float maxTmpF = -999;
						int maxTmpI = -999;
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
								if(maxTmpF > -999)
								{
									pThrItem->checkResultValue.vf = maxTmpF;
									bRet = TRUE;
								}
								break;
							}
						case value_type_int:
							{
								if(maxTmpI > -999)
								{
									pThrItem->checkResultValue.vi = maxTmpI;
									bRet = TRUE;
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
									bRet = TRUE;
								}
								break;
							}
						case value_type_int:
							{
								if(minTmpI < 99999)
								{
									pThrItem->checkResultValue.vi = minTmpI;
									bRet = TRUE;
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
					while(curNode)
					{
						if(nowTime - curNode->checkValueTime >= pThrItem->lastingTimeS)
						{
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
		{
			check_value_node_t * head = pThrItem->checkSourceValueList.head;
			check_value_node_t * newNode = (check_value_node_t *)malloc(sizeof(check_value_node_t));
			newNode->checkValueTime = nowTime;
			newNode->ckV.vi = DEF_INVALID_VALUE;
			switch(valueType)
			{
			case value_type_float:
				{
					newNode->ckV.vf = (*pCheckValue).vf;
					break;
				}
			case value_type_int:
				{
					newNode->ckV.vi = (*pCheckValue).vi;
					break;
				}
			default: break;
			}
			newNode->next = head->next;
			head->next = newNode;
			pThrItem->checkSourceValueList.nodeCnt++;
		}
	}
	return bRet;
}

static BOOL ThrItemCheck(swm_thr_item_t * pThrItem, int valueI, char * checkMsg, BOOL * checkRet)
{
	BOOL bRet = FALSE;
	if(pThrItem == NULL || checkRet == NULL || checkMsg == NULL) return bRet;
	if(pThrItem->isEnable)
	{
		BOOL isTrigger = FALSE;
		BOOL triggerMax = FALSE;
		BOOL triggerMin = FALSE;
		char tmpRetMsg[1024] = {0};
		char checkTypeStr[32] = {0};
		switch(pThrItem->checkType)
		{
		case ck_type_avg:
			{
				//sprintf(checkTypeStr, "avg");
				sprintf(checkTypeStr, "average");
				break;
			}
		case ck_type_max:
			{
				sprintf(checkTypeStr, "max");
				break;
			}
		case ck_type_min:
			{
				sprintf(checkTypeStr, "min");
				break;
			}
		default: break;
		}
		{
			check_value_t checkValue;
			checkValue.vi = valueI;
			if(CheckSourceValue(pThrItem, &checkValue, value_type_int) && pThrItem->checkResultValue.vi != DEF_INVALID_VALUE)
			{
				int usageV = pThrItem->checkResultValue.vi;
				if(pThrItem->thresholdType & DEF_THR_MAX_TYPE)
				{
					if(pThrItem->maxThreshold != DEF_INVALID_VALUE && (usageV > pThrItem->maxThreshold))
					{
						sprintf(tmpRetMsg, "(#tk#%s#tk#:%d)>#tk#maxThreshold#tk#(%.0f)", checkTypeStr, usageV, pThrItem->maxThreshold);
						triggerMax = TRUE;
					}
				}
				if(pThrItem->thresholdType & DEF_THR_MIN_TYPE)
				{
					if(pThrItem->minThreshold != DEF_INVALID_VALUE && (usageV < pThrItem->minThreshold))
					{
						if(strlen(tmpRetMsg)) sprintf(tmpRetMsg, "%s and (#tk#%s#tk#:%d)<#tk#minThreshold#tk#(%.0f)", tmpRetMsg, checkTypeStr, usageV, pThrItem->minThreshold);
						else sprintf(tmpRetMsg, "(#tk#%s#tk#:%d)<#tk#minThreshold#tk#(%.0f)", checkTypeStr, usageV, pThrItem->minThreshold);
						triggerMin = TRUE;
					}
				}
			}
		}

		switch(pThrItem->thresholdType)
		{
		case DEF_THR_MAX_TYPE:
			{
				isTrigger = triggerMax;
				break;
			}
		case DEF_THR_MIN_TYPE:
			{
				isTrigger = triggerMin;
				break;
			}
		case DEF_THR_MAXMIN_TYPE:
			{
				isTrigger = triggerMin || triggerMax;
				break;
			}
		}

		if(isTrigger)
		{
			long long nowTime = time(NULL);
			if(pThrItem->repThrTime == 0 || pThrItem->intervalTimeS == DEF_INVALID_TIME || pThrItem->intervalTimeS == 0 || nowTime - pThrItem->repThrTime >= pThrItem->intervalTimeS)
			{
				pThrItem->repThrTime = nowTime;
				pThrItem->isNormal = FALSE;
				*checkRet = TRUE;
				bRet = TRUE;
			}
		}
		else
		{
			if(!pThrItem->isNormal && pThrItem->checkResultValue.vi != DEF_INVALID_VALUE)
			{
				memset(tmpRetMsg, 0, sizeof(tmpRetMsg));
				sprintf(tmpRetMsg, "(%s) #tk#back to normal#tk#!", checkTypeStr);
				pThrItem->isNormal = TRUE;
				*checkRet = FALSE;
				bRet = TRUE;
			}
		}
		if(!bRet) sprintf(checkMsg,"");
		else sprintf(checkMsg, "%s", tmpRetMsg);
	}
	return bRet;
}

prc_thr_type_t MonObjThrCheck(sw_thr_check_params_t * pThrCheckParams, BOOL *checkRet)
{
	prc_thr_type_t tRet = prc_thr_type_unknown;
	BOOL bRet = FALSE;
	mon_obj_info_t * pMonObjInfo = NULL;
	prc_mon_info_t * pPrcMonInfo = NULL;
	if(pThrCheckParams == NULL) return bRet;
	pMonObjInfo = pThrCheckParams->pMonObjInfo;
	pPrcMonInfo = pThrCheckParams->pPrcMonInfo;
	if(pMonObjInfo == NULL || pPrcMonInfo == NULL) return bRet;
	{
		memset(pThrCheckParams->checkMsg, 0, sizeof(pThrCheckParams->checkMsg));
		if(!bRet)
		{
			if(!pPrcMonInfo->isActive && pMonObjInfo->prcResponse)
			{
				sprintf(pThrCheckParams->checkMsg, "%s", "#tk#Process no response#tk#!");
				pThrCheckParams->pMonObjInfo->prcResponse = FALSE;
				bRet = TRUE;
				tRet = prc_thr_type_active;
			}
			else if(pPrcMonInfo->isActive && !pMonObjInfo->prcResponse)
			{
				sprintf(pThrCheckParams->checkMsg,"%s", "#tk#Process recovery response#tk#!");
				pThrCheckParams->pMonObjInfo->prcResponse = TRUE;
				bRet = TRUE;
				tRet = prc_thr_type_active;
			}
		}

		if(!bRet && pMonObjInfo->thrItem.isEnable)
		{
			*checkRet = FALSE;
			if(!strcmp(pMonObjInfo->thrItem.tagName, SWM_THR_CPU))
			{
				bRet = ThrItemCheck(&pMonObjInfo->thrItem, pPrcMonInfo->cpuUsage, pThrCheckParams->checkMsg, checkRet);
				if(bRet){
					tRet = prc_thr_type_active;
					//tRet = prc_thr_type_cpu;
				}
			}
			else if(!strcmp(pMonObjInfo->thrItem.tagName, SWM_THR_MEM))
			{
				bRet = ThrItemCheck(&pMonObjInfo->thrItem, pPrcMonInfo->memUsage, pThrCheckParams->checkMsg, checkRet);
				if(bRet)
				{
					tRet = prc_thr_type_active;
					//tRet = prc_thr_type_mem;
				}
			}
		} 
	}
	return tRet;
}