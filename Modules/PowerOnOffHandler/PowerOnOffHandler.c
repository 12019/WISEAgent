/****************************************************************************/
/* Copyright(C) : Advantech Technologies, Inc.								       */
/* Create Date  : 2014/11/12 by li.ge								          */
/* Modified Date: 2014/11/12 by li.ge								          */
/* Abstract     : PowerOnOff Handler API                                     			 */
/* Reference    : None														             */
/****************************************************************************/
#include "network.h"
#include "platform.h"
#include "susiaccess_handler_api.h"
#include "PowerOnOffHandler.h"
#include "PowerOnOffLog.h"
#include <configuration.h>
#include "Parser.h"
#include "common.h"
#include "cJSON.h"
#include "amtconfig.h"

//-----------------------------------------------------------------------------
// Types and defines:
//-----------------------------------------------------------------------------
const char strPluginName[MAX_TOPIC_LEN] = {"power_onoff"};
const int iRequestID = cagent_request_power_onoff;
const int iActionID = cagent_reply_power_onoff;


//-----------------------------------------------------------------------------
// Internal Prototypes:
//-----------------------------------------------------------------------------
//
typedef struct{
	void* threadHandler;
	bool isThreadRunning;
	bool threadSyncFlag;
}handler_context_t;

//-----------------------------------------------------------------------------
// Variables
//-----------------------------------------------------------------------------
static void* g_loghandle = NULL;
static bool g_bEnableLog = TRUE;
static char agentConfigFilePath[MAX_PATH] = {0};
char ConfigFilePath[MAX_PATH] = {0};//agent_config.xml

#define cagent_handle_t void *

typedef struct {
	cagent_handle_t   cagentHandle;
}susi_handler_context_t;

typedef struct{
	susi_handler_context_t susiHandlerContext;
}power_onoff_handler_context_t;

power_onoff_handler_context_t     PowerOnOffHandlerContext;

#define COMM_DATA_WITH_JSON

typedef struct{
#ifdef COMM_DATA_WITH_JSON
	int reqestID;
#endif
	susi_comm_cmd_t comm_Cmd;
	int message_length;
	char message[];
}susi_comm_data_t;

static Handler_info  g_PluginInfo;

static HandlerSendCbf  g_sendcbf = NULL;						// Client Send information (in JSON format) to Cloud Server	
static HandlerSendCustCbf  g_sendcustcbf = NULL;			    // Client Send information (in JSON format) to Cloud Server with custom topic	
static HandlerAutoReportCbf g_sendreportcbf = NULL;				// Client Send report (in JSON format) to Cloud Server with AutoReport topic	
static HandlerSendCapabilityCbf g_sendcapabilitycbf = NULL;		//not used
static HandlerSubscribeCustCbf g_subscribecustcbf = NULL;

power_capability_info_t cpbInfo;
power_amt_params amtParams;

static void GetCapability();
static void CollectCpbInfo(power_capability_info_t * pCpbInfo);
int Parser_PackCpbInfo(power_capability_info_t * cpbInfo, char **outputStr);

//-----------------------------------------------------------------------------
// Functions
//-----------------------------------------------------------------------------
static void InitPowerPath()
{
	char modulePath[MAX_PATH] = {0};
	char vncPath[MAX_PATH] = {0};
	if(strlen(g_PluginInfo.WorkDir)<=0) return;
	strcpy(modulePath, g_PluginInfo.WorkDir);
	path_combine(ConfigFilePath, modulePath, DEF_CONFIG_FILE_NAME);
}

BOOL GetMacList(char * macListStr)
{
	BOOL bRet = FALSE;
	if(NULL == macListStr) return bRet;
	{
		char macsStr[10][20] = {0};
		int macCnt = 0, i = 0;
		macCnt = network_mac_list_get(macsStr, sizeof(macsStr)/20);
		if(macCnt > 0)
		{
			for(i = 0; i < macCnt; i++)
			{
				strcat(macListStr, macsStr[i]);
				if(i < macCnt -1) strcat(macListStr, ",");
			}
			bRet = TRUE;
		}
	}
	return bRet;
}

static void PowerMacList()
{
	char powerMsg[BUFSIZ] = {0};
	if(!GetMacList(powerMsg))
	{
		//strcat_s(powerMsg,sizeof(powerMsg), "None");
		strcat(powerMsg, "None");
	}

	{
		char * uploadRepJsonStr = NULL;
		const char * str = powerMsg;
		int jsonStrlen = Parser_PackPowerOnOffStrRep(str, &uploadRepJsonStr);
		if(jsonStrlen > 0 && uploadRepJsonStr != NULL)
		{
			g_sendcbf(&g_PluginInfo, power_mac_list_rep, uploadRepJsonStr, strlen(uploadRepJsonStr)+1, NULL, NULL);
		}
		if(uploadRepJsonStr)free(uploadRepJsonStr);
	}
}

BOOL GetLastBootupTimeStr(char * timeStr)
{
	BOOL bRet = FALSE;
	unsigned long long dwTickMs = 0;
	time_t nowTime;
	time_t lastBootupTime;
	struct tm * pLastBootupTm = NULL;
	char timeTempStr[64] = {0};
	if(NULL == timeStr) return bRet;
	dwTickMs = app_os_GetTickCount();
	nowTime = time(NULL);
	lastBootupTime = nowTime - (time_t)dwTickMs/1000;
	pLastBootupTm = localtime(&lastBootupTime); 
	strftime(timeTempStr, sizeof( timeTempStr), "%Y-%m-%d %H:%M:%S", pLastBootupTm); 
	strcpy(timeStr,timeTempStr);
	//strcpy_s(timeStr, sizeof(timeTempStr),timeTempStr);
	bRet = TRUE;
	return bRet;
}

static void PowerLastBootupTime()
{
	char powerMsg[BUFSIZ] = {0};
	char lastBootupTimeStr[32] = {0};
	BOOL bRet = GetLastBootupTimeStr(lastBootupTimeStr);
	if(bRet)
	{
		//strcat_s(powerMsg, sizeof(powerMsg),lastBootupTimeStr);
		strcat(powerMsg, lastBootupTimeStr);
	}
	else
	{
		//strcat_s(powerMsg,sizeof(powerMsg), "Failed");
		strcat(powerMsg, "Failed");
	}

	{
		char * uploadRepJsonStr = NULL;
		const char * str = powerMsg;
		int jsonStrlen = Parser_PackPowerOnOffStrRep(str, &uploadRepJsonStr);
		if(jsonStrlen > 0 && uploadRepJsonStr != NULL)
		{
			g_sendcbf(&g_PluginInfo, power_last_bootup_time_rep, uploadRepJsonStr, strlen(uploadRepJsonStr)+1, NULL, NULL);
		}
		if(uploadRepJsonStr)free(uploadRepJsonStr);
	}

}

BOOL GetBootupPeriodStr(char *periodStr)
{
	BOOL bRet = FALSE;
	unsigned long long tickMs = 0;
	unsigned long iDay = 0, iHour = 0, iMin = 0, iSec = 0;
	if(NULL == periodStr) return bRet;
	tickMs = app_os_GetTickCount();
	iDay = (unsigned long)tickMs/(3600000*24);
	iHour = (unsigned long)(tickMs-3600000*24*iDay)/3600000;
	iMin = (unsigned long)(tickMs-3600000*24*iDay - 3600000*iHour)/60000;
	iSec = (unsigned long)(tickMs-3600000*24*iDay - 3600000*iHour - iMin*60000)/1000;

	sprintf(periodStr,"%d %d:%d:%d", iDay, iHour, iMin, iSec);
	return TRUE;
}

static void PowerBootupPeriod()
{
	char powerMsg[BUFSIZ] = {0};
	char bootupTimeStr[32] = {0};
	BOOL bRet = GetBootupPeriodStr(bootupTimeStr);
	if(bRet)
	{
		//strcat_s(powerMsg, sizeof(powerMsg),bootupTimeStr);
		strcat(powerMsg, bootupTimeStr);
	}
	else
	{
		strcat(powerMsg, "Failed");;
	}

	{
		char * uploadRepJsonStr = NULL;
		const char *str = powerMsg;
		int jsonStrlen = Parser_PackPowerOnOffStrRep(str, &uploadRepJsonStr);
		if(jsonStrlen > 0 && uploadRepJsonStr != NULL)
		{
			g_sendcbf(&g_PluginInfo, power_bootup_period_rep, uploadRepJsonStr, strlen(uploadRepJsonStr)+1, NULL, NULL);
		}
		if(uploadRepJsonStr)free(uploadRepJsonStr);
	}
}


static void PowerOff()
{
	char powerMsg[BUFSIZ] = {0};  
	if(app_os_PowerOff() != 0)
	{
		PowerOnOffLog(g_loghandle, Normal, "%s()[%d]###Create shutdown process failed!\n", __FUNCTION__, __LINE__);
		//strcat_s(powerMsg,sizeof(powerMsg), "Failed");
		strcat(powerMsg, "Failed");
	}
	else
	{
		//strcat_s(powerMsg, sizeof(powerMsg),"Success");
		strcat(powerMsg, "Success");
	}
	{
		char * uploadRepJsonStr = NULL;
		const char *str = powerMsg;
		int jsonStrlen = Parser_PackPowerOnOffStrRep(str, &uploadRepJsonStr);
		if(jsonStrlen > 0 && uploadRepJsonStr != NULL)
		{
			g_sendcbf(&g_PluginInfo, power_off_rep, uploadRepJsonStr, strlen(uploadRepJsonStr)+1, NULL, NULL);
		}
		if(uploadRepJsonStr)free(uploadRepJsonStr);
	}

}

static void PowerRestart()
{
	char powerMsg[BUFSIZ] = {0};

	if(app_os_PowerRestart() != 0)
	{
		PowerOnOffLog(g_loghandle, Normal, "%s()[%d]###Create restart process failed!\n", __FUNCTION__, __LINE__);
		//strcat_s(powerMsg,sizeof(powerMsg), "Failed");
		strcat(powerMsg, "Failed");
	}
	else
	{
		//strcat_s(powerMsg, sizeof(powerMsg),"Success");
		strcat(powerMsg, "Success");
	}
	{
		char * uploadRepJsonStr = NULL;
		const char *str = powerMsg;
		int jsonStrlen = Parser_PackPowerOnOffStrRep(str, &uploadRepJsonStr);
		if(jsonStrlen > 0 && uploadRepJsonStr != NULL)
		{
			g_sendcbf(&g_PluginInfo, power_restart_rep, uploadRepJsonStr, strlen(uploadRepJsonStr)+1, NULL, NULL);
		}
		if(uploadRepJsonStr)free(uploadRepJsonStr);
	}
}

static void PowerSuspend()
{
	char powerMsg[BUFSIZ] = {0};
	if(app_os_IsPwrSuspendAllowed())
	{
		app_os_disableResumePassword();
		//strcat_s(powerMsg, sizeof(powerMsg),"Suspending");
		strcat(powerMsg, "Suspending");
		{
			char * uploadRepJsonStr = NULL;
			const char *str = powerMsg;
			int jsonStrlen = Parser_PackPowerOnOffStrRep(str, &uploadRepJsonStr);
			if(jsonStrlen > 0 && uploadRepJsonStr != NULL)
			{
				g_sendcbf(&g_PluginInfo, power_suspend_rep, uploadRepJsonStr, strlen(uploadRepJsonStr)+1, NULL, NULL);
			}
			if(uploadRepJsonStr)free(uploadRepJsonStr);
		}

		memset(powerMsg, 0, sizeof(powerMsg));
		if(app_os_PowerSuspend())
		{
			//strcat_s(powerMsg, sizeof(powerMsg),"Success");
			strcat(powerMsg,"Success");
		}
		else
		{
			strcat(powerMsg, "Failed");
			//strcat_s(powerMsg,sizeof(powerMsg), "Failed");
		}
	}
	else
	{
		//strcat_s(powerMsg, sizeof(powerMsg),"NotSupport");
		strcat(powerMsg, "NotSupport");
	}

	{
		char * uploadRepJsonStr = NULL;
		const char *str = powerMsg;
		int jsonStrlen = Parser_PackPowerOnOffStrRep(str, &uploadRepJsonStr);
		if(jsonStrlen > 0 && uploadRepJsonStr != NULL)
		{
			g_sendcbf(&g_PluginInfo, power_suspend_rep, uploadRepJsonStr, strlen(uploadRepJsonStr)+1, NULL, NULL);
		}
		if(uploadRepJsonStr)free(uploadRepJsonStr);
	}

}

static void PowerHibernate()
{
	char powerMsg[BUFSIZ] = {0};
	if(app_os_IsPwrHibernateAllowed())
	{
		app_os_disableResumePassword();
		//strcat_s(powerMsg,sizeof(powerMsg), "Hibernating");
		strcat(powerMsg, "Hibernating");

		{
			char * uploadRepJsonStr = NULL;
			const char *str = powerMsg;
			int jsonStrlen = Parser_PackPowerOnOffStrRep(str, &uploadRepJsonStr);
			if(jsonStrlen > 0 && uploadRepJsonStr != NULL)
			{
				g_sendcbf(&g_PluginInfo, power_hibernate_rep, uploadRepJsonStr, strlen(uploadRepJsonStr)+1, NULL, NULL);
			}
			if(uploadRepJsonStr)free(uploadRepJsonStr);
		}

		if(app_os_PowerHibernate())
		{
			//strcat_s(powerMsg, sizeof(powerMsg),"Success");
			strcat(powerMsg, "Success");
		}
		else
		{
			//strcat_s(powerMsg, sizeof(powerMsg),"Failed");
			strcat(powerMsg, "Failed");
		}
	}
	else
	{
		//strcat_s(powerMsg,sizeof(powerMsg), "NotSupport");
		strcat(powerMsg, "NotSupport");
	}

	{
		char * uploadRepJsonStr = NULL;
		const char *str = powerMsg;
		int jsonStrlen = Parser_PackPowerOnOffStrRep(str, &uploadRepJsonStr);
		if(jsonStrlen > 0 && uploadRepJsonStr != NULL)
		{
			g_sendcbf(&g_PluginInfo, power_hibernate_rep, uploadRepJsonStr, strlen(uploadRepJsonStr)+1, NULL, NULL);
		}
		if(uploadRepJsonStr)
			free(uploadRepJsonStr);
	}
}


static BOOL TransMac(char * macStr, char * newMac)
{
	BOOL bRet = FALSE;
	if( NULL == macStr || NULL == newMac) return bRet;
	{
		char tmpMacStr[32] = {0};
		char * itemToken[8] = {NULL};
		char * buf = NULL;
		int i = 0 , j = 0;
		//strcpy_s(tmpMacStr, sizeof(tmpMacStr),macStr);
		strcpy(tmpMacStr,macStr);
		buf = tmpMacStr;
		while(itemToken[i] = strtok(buf, "-"))
		{
			i++;
			if(i > 6) break;
			buf = NULL;
		}
		if(i==6)
		{
			for(j = 0; j<6; j++)
			{
				newMac[j] = (char)strtol(itemToken[j], NULL, 16);
			}
			bRet = TRUE;
		}
	}
	return bRet;
}

static void PowerWakeOnLan(char * pMacsStr)
{
#define DEF_MAC_TOKEN_CNT      50
	if(NULL == pMacsStr) return;
	{
		char tmpMacsStr[1024] = {0};
		char repMsg[1024] = {0};
		char * macToken[DEF_MAC_TOKEN_CNT] = {NULL};
		char * buf = NULL;
		int realCnt = 0, i = 0, j = 0;
		//strcpy_s(tmpMacsStr, sizeof(tmpMacsStr),pMacsStr);
		strcpy(tmpMacsStr, pMacsStr);
		buf = tmpMacsStr;
		while(macToken[i] = strtok(buf, ";"))
		{
			i++;
			if(i >= DEF_MAC_TOKEN_CNT) break;
			buf = NULL;
		}
		realCnt = i;
		if(realCnt > 0)
		{
			char mac[6] = {0};
			for( j = 0; j<realCnt; j++)
			{
				memset(mac, 0, sizeof(mac));
				if(TransMac(macToken[j], mac))
				{
					if(network_magic_packet_send(mac, sizeof(mac)))
					{
						if(strlen(repMsg))sprintf_s(repMsg, sizeof(repMsg),"%s,%s SUCCESS", repMsg, macToken[j]);
						else sprintf_s(repMsg, sizeof(repMsg),"%s SUCCESS", macToken[j]);
					}
					else
					{
						if(strlen(repMsg))sprintf_s(repMsg,sizeof(repMsg), "%s,%s FAILED", repMsg, macToken[j]);
						else sprintf_s(repMsg,sizeof(repMsg), "%s FAILED", macToken[j]);
					}
				}
				else
				{
					if(strlen(repMsg))sprintf_s(repMsg, sizeof(repMsg),"%s,%s format error", repMsg, macToken[j]);
					else sprintf_s(repMsg,sizeof(repMsg), "%s format error", macToken[j]);
				}
			}
		}
		else
		{
			sprintf_s(repMsg, sizeof(repMsg),"%s", "Mac params error!");
		}

		if(strlen(repMsg))
		{
			char * uploadRepJsonStr = NULL;
			const char *str = repMsg;
			int jsonStrlen = Parser_PackPowerOnOffStrRep(str, &uploadRepJsonStr);
			if(jsonStrlen > 0 && uploadRepJsonStr != NULL)
			{
				g_sendcbf(&g_PluginInfo, power_wake_on_lan_rep, uploadRepJsonStr, strlen(uploadRepJsonStr)+1, NULL, NULL);
			}
			if(uploadRepJsonStr)free(uploadRepJsonStr);
		}
	}
}

cagent_callback_status_t cagent_get_amt_en(OUT char **amtEn, OUT size_t *size)
{
	cagent_callback_status_t status = cagent_callback_continue;
	if(strlen(AMT_EN) == 0)
	{
		if(cfg_get(agentConfigFilePath, AMT_EN_KEY, AMT_EN, sizeof(AMT_EN)) <= 0)
		{
			return cagent_callback_abort;
		}
	}
	*amtEn = AMT_EN;
	*size = strlen(AMT_EN);
	return status;
}

cagent_callback_status_t cagent_get_amt_id(OUT char **amtID, OUT size_t *size)
{
	cagent_callback_status_t status = cagent_callback_continue;
	if(strlen(AMT_ID) == 0)
	{
		if(cfg_get(agentConfigFilePath, AMT_ID_KEY, AMT_ID, sizeof(AMT_ID)) <= 0)
		{
			return cagent_callback_abort;
		}
	}
	*amtID = AMT_ID;
	*size = strlen(AMT_ID);
	return status;
}

cagent_callback_status_t cagent_get_amt_pwd(OUT char **amtPwd, OUT size_t *size)
{
	cagent_callback_status_t status = cagent_callback_continue;
	if(strlen(AMT_PWD) == 0)
	{
		if(cfg_get(agentConfigFilePath, AMT_PWD_KEY, AMT_PWD, sizeof(AMT_PWD)) <= 0)
		{
			return cagent_callback_abort;
		}
	}
	*amtPwd = AMT_PWD;
	*size = strlen(AMT_PWD);
	return status;
}


/*BOOL GetAMTParams(power_amt_params * amtParams)
{
	BOOL bRet = FALSE;
	if(NULL == amtParams) return bRet;
	{
		char * pItemValue = NULL;
		unsigned int valueSize = 0;
		susiaccess_amt_conf_body_t amt;
		if(amt_load(agentConfigFilePath, &amt))
		{
			strcpy(amtParams->amtEn, amt.amtEn);
			strcpy(amtParams->amtID, amt.amtID);
			strcpy(amtParams->amtPwd, amt.amtPwd);
		}

		//cagent_get_amt_en(&pItemValue, &valueSize);
		//if(pItemValue && strlen(pItemValue))
		//{
		//	//strcpy_s(amtParams->amtEn, sizeof(amtParams->amtEn),pItemValue);
		//	strcpy(amtParams->amtEn,pItemValue);
		//}
		//else
		//{
		//	//strcpy_s(amtParams->amtEn,sizeof(amtParams->amtEn), "False");
		//	strcpy(amtParams->amtEn, "False");
		//}
		//pItemValue = NULL;
		//cagent_get_amt_id(&pItemValue, &valueSize);
		//if(pItemValue && strlen(pItemValue))
		//{
		//	//strcpy_s(amtParams->amtID, sizeof(amtParams->amtID),pItemValue);
		//	strcpy(amtParams->amtID, pItemValue);
		//}
		//pItemValue = NULL;
		//cagent_get_amt_pwd(&pItemValue, &valueSize);
		//if(pItemValue && strlen(pItemValue))
		//{
		//	//strcpy_s(amtParams->amtPwd, sizeof(amtParams->amtPwd),pItemValue);
		//	strcpy(amtParams->amtPwd,pItemValue);
		//}
	}
	return bRet = TRUE;
}*/

static void PowerGetAMTParams()
{
	//power_amt_params amtParams;
	//memset(&amtParams, 0, sizeof(amtParams));
	//GetAMTParams(&amtParams);

	{
		char * uploadRepJsonStr = NULL;
		power_amt_params amtvalue;
		int jsonStrlen = 0;
		memcpy(&amtvalue, &amtParams, sizeof(amtParams));
		jsonStrlen = Parser_PackPowerOnOffAMT(&amtvalue, &uploadRepJsonStr);
		if(jsonStrlen > 0 && uploadRepJsonStr != NULL)
		{
			g_sendcbf(&g_PluginInfo, power_get_amt_params_rep, uploadRepJsonStr, strlen(uploadRepJsonStr)+1, NULL, NULL);
		}
		if(uploadRepJsonStr)free(uploadRepJsonStr);
	}

}

int Parser_PackCpbInfo(power_capability_info_t * cpbInfo, char **outputStr)
{
	char * out = NULL;
	int outLen = 0;
	cJSON *root = NULL, *pSUSICommDataItem = NULL, *handlerName = NULL, *infoItem = NULL;
	cJSON *subItem = NULL, *eItem = NULL, *bnItem = NULL, *nonSensorDataItem = NULL;
	if(cpbInfo == NULL || outputStr == NULL) return outLen;

	root = cJSON_CreateObject();

	pSUSICommDataItem = cJSON_CreateObject();
	cJSON_AddItemToObject(root, AGENTINFO_BODY_STRUCT, pSUSICommDataItem);
	if(pSUSICommDataItem)
	{
		handlerName = cJSON_CreateObject();
		if(handlerName)
		{
			cJSON_AddItemToObject(pSUSICommDataItem, HANDLER_NAME, handlerName);
			infoItem = cJSON_CreateObject();
			if(infoItem)
			{
				cJSON_AddItemToObject(handlerName, POWER_INFOMATION, infoItem);
				eItem = cJSON_CreateArray();
				if(eItem)
				{
					cJSON_AddItemToObject(infoItem, POWER_E_FLAG, eItem);
					if(strlen(cpbInfo->MacList))
					{
						subItem = cJSON_CreateObject();
						cJSON_AddStringToObject(subItem, POWER_N_FLAG, POWER_MAC_LIST);
						cJSON_AddStringToObject(subItem, POWER_SV_FLAG, cpbInfo->MacList);
						cJSON_AddItemToArray(eItem, subItem);
					}
					if(strlen(cpbInfo->AmtParam.amtEn))
					{
						subItem = cJSON_CreateObject();
						cJSON_AddStringToObject(subItem, POWER_N_FLAG, POWER_AMT_EN);
						cJSON_AddStringToObject(subItem, POWER_SV_FLAG, cpbInfo->AmtParam.amtEn);//cJSON_AddNumberToObject
						cJSON_AddItemToArray(eItem, subItem);
					}
					if(strlen(cpbInfo->AmtParam.amtID))
					{
						subItem = cJSON_CreateObject();
						cJSON_AddStringToObject(subItem, POWER_N_FLAG, POWER_AMT_ID);
						cJSON_AddStringToObject(subItem, POWER_SV_FLAG, cpbInfo->AmtParam.amtID);
						cJSON_AddItemToArray(eItem, subItem);
					}
					if(strlen(cpbInfo->AmtParam.amtPwd))
					{
						subItem = cJSON_CreateObject();
						cJSON_AddStringToObject(subItem, POWER_N_FLAG, POWER_AMT_PWD);
						cJSON_AddStringToObject(subItem, POWER_SV_FLAG, cpbInfo->AmtParam.amtPwd);
						cJSON_AddItemToArray(eItem, subItem);
					}
					if(strlen(cpbInfo->funcsStr))
					{
						subItem = cJSON_CreateObject();
						cJSON_AddStringToObject(subItem, POWER_N_FLAG, POWER_FUNCTION_LIST);
						cJSON_AddStringToObject(subItem, POWER_SV_FLAG, cpbInfo->funcsStr);
						cJSON_AddItemToArray(eItem, subItem);
					}
					{
						subItem = cJSON_CreateObject();
						cJSON_AddStringToObject(subItem, POWER_N_FLAG, POWER_FUNCTION_CODE);
						cJSON_AddNumberToObject(subItem, POWER_V_FLAG, cpbInfo->funcsCode);
						cJSON_AddItemToArray(eItem, subItem);
					}
				}

				cJSON_AddStringToObject(infoItem, POWER_BN_FLAG, POWER_INFOMATION);
				cJSON_AddBoolToObject(infoItem, POWER_NONSENSORDATA_FLAG, 1);
			}
		}
	}

	out = cJSON_PrintUnformatted(pSUSICommDataItem);
	outLen = strlen(out) + 1;
	*outputStr = (char *)(malloc(outLen));
	memset(*outputStr, 0, outLen);
	strcpy(*outputStr, out);
	cJSON_Delete(root);
	printf("%s\n",out);	
	free(out);
	return outLen;
}

void GetCapability()
{
	char * cpbStr = NULL;
	int jsonStrlen = 0;
	/*power_capability_info_t cpbInfo;
	memset((char*)&cpbInfo, 0, sizeof(power_capability_info_t));
	CollectCpbInfo(&cpbInfo);*/
	
	jsonStrlen = Parser_PackCpbInfo(&cpbInfo, &cpbStr);
	if(jsonStrlen > 0 && cpbStr != NULL)
	{
		g_sendcbf(&g_PluginInfo, power_get_capability_rep, cpbStr, strlen(cpbStr)+1, NULL, NULL);
		if(cpbStr)free(cpbStr);
	}
	else
	{
		char * errorRepJsonStr = NULL;
		char errorStr[128];
		int jsonStrlen = 0;
		sprintf(errorStr, "Command(%d), Get capability error!", power_get_capability_req);
		jsonStrlen = Parser_PackPowerErrorRep(errorStr, &errorRepJsonStr);
		if(jsonStrlen > 0 && errorRepJsonStr != NULL)
		{
			g_sendcbf(&g_PluginInfo, power_error_rep, errorRepJsonStr, strlen(errorRepJsonStr)+1, NULL, NULL);
		}
		if(errorRepJsonStr)free(errorRepJsonStr);
	}
}


void CollectCpbInfo(power_capability_info_t * pCpbInfo)
{
	if(NULL == pCpbInfo) return;
	{
		//add code about ssh
		int funcFlag = 0;
		char powerMsg[BUFSIZ] = {0};
		char funcsStr[256] = {0};
		unsigned int funcsCode = 0;
		susiaccess_amt_conf_body_t amt_conf;
		susiaccess_power_func_conf_body_t power_func_conf;

		memset(&amt_conf, 0, sizeof(susiaccess_amt_conf_body_t));
		amt_load(ConfigFilePath, &amt_conf);
		strcpy(pCpbInfo->AmtParam.amtEn, amt_conf.amtEn);
		strcpy(pCpbInfo->AmtParam.amtID, amt_conf.amtID);
		strcpy(pCpbInfo->AmtParam.amtPwd, amt_conf.amtPwd);

		//MacList
		if(!GetMacList(powerMsg))
		{
			strcat(powerMsg, "None");
		}
		strcpy(pCpbInfo->MacList, powerMsg);
		memset(&power_func_conf, 0, sizeof(susiaccess_power_func_conf_body_t));
		powerFunc_load(ConfigFilePath, &power_func_conf);

		if(app_os_IsPwrHibernateAllowed()) 
		{
			strcpy(power_func_conf.hibernateFlag, "true");
		}

		if(app_os_IsPwrSuspendAllowed())
		{
			strcpy(power_func_conf.suspendFlag, "true");
		}

		//none,wol,shutdown,restart,suspend
		if(!stricmp(power_func_conf.wolFlag, "true"))
		{
			strcat(funcsStr, WOLStr);
			strcat(funcsStr, ",");
			funcsCode |= WOL;
		}
		if(!stricmp(power_func_conf.shutdownFlag, "true"))
		{
			strcat(funcsStr, ShutdownStr);
			strcat(funcsStr, ",");
			funcsCode |= Shutdown;
		}
		if(!stricmp(power_func_conf.restartFlag, "true"))
		{
			strcat(funcsStr, RestartStr);
			strcat(funcsStr, ",");
			funcsCode |= Restart;
		}
		if(!stricmp(power_func_conf.hibernateFlag, "true"))
		{
			strcat(funcsStr, HibernateStr);
			strcat(funcsStr, ",");
			funcsCode |= Hibernate;
		}
		if(!stricmp(power_func_conf.suspendFlag, "true"))
		{
			strcat(funcsStr, SuspendStr);
			strcat(funcsStr, ",");
			funcsCode |= Suspend;
		}

		if(strlen(funcsStr) > 0)
		{
			funcsStr[strlen(funcsStr) -1] = '\0';
			strcpy(pCpbInfo->funcsStr, funcsStr);
		}
		else
			strcpy(pCpbInfo->funcsStr, NoneStr);

		pCpbInfo->funcsCode = funcsCode;
	}
}


/* **************************************************************************************
*  Function Name: Handler_Initialize
*  Description: Init any objects or variables of this handler
*  Input :  PLUGIN_INFO *pluginfo
*  Output: None
*  Return:  handler_success  : Success Init Handler
*           handler_fail : Fail Init Handler
* ***************************************************************************************/
int HANDLER_API Handler_Initialize( HANDLER_INFO *pluginfo )
{
	if(pluginfo == NULL)
	{
		return handler_fail;
	}

	if(g_bEnableLog)
	{
		g_loghandle = pluginfo->loghandle;
	}

	// 1. Topic of this handler
	snprintf( pluginfo->Name, sizeof(pluginfo->Name), "%s", strPluginName );
	pluginfo->RequestID = iRequestID;
	pluginfo->ActionID = iActionID;
	printf(" >Name: %s\n", strPluginName);
	// 2. Copy agent info 
	memcpy(&g_PluginInfo, pluginfo, sizeof(HANDLER_INFO));
	g_PluginInfo.agentInfo = pluginfo->agentInfo;

	{
		char mdPath[MAX_PATH] = {0};
		//GetMoudlePath(mdPath);
		app_os_get_module_path(mdPath);
		path_combine(agentConfigFilePath, mdPath, DEF_CONFIG_FILE_NAME);
		//sprintf_s(agentConfigFilePath, sizeof(agentConfigFilePath), "%s\\%s", mdPath, DEF_CONFIG_FILE_NAME);

		if(g_bEnableLog)
		{
			g_loghandle = pluginfo->loghandle;
		}
	}

	// 3. Callback function -> Send JSON Data by this callback function
	g_sendcbf = g_PluginInfo.sendcbf = pluginfo->sendcbf;
	g_sendcustcbf = g_PluginInfo.sendcustcbf = pluginfo->sendcustcbf;
	g_subscribecustcbf = g_PluginInfo.subscribecustcbf = pluginfo->subscribecustcbf;
	g_sendreportcbf = g_PluginInfo.sendreportcbf = pluginfo->sendreportcbf;
	g_sendcapabilitycbf =g_PluginInfo.sendcapabilitycbf = pluginfo->sendcapabilitycbf; 

	InitPowerPath();

	memset((char*)&cpbInfo, 0, sizeof(power_capability_info_t));
	CollectCpbInfo(&cpbInfo);

	memset(&amtParams, 0, sizeof(amtParams));

	strcpy(amtParams.amtEn,cpbInfo.AmtParam.amtEn);
	strcpy(amtParams.amtID,cpbInfo.AmtParam.amtID);
	strcpy(amtParams.amtPwd,cpbInfo.AmtParam.amtPwd);


	return handler_success;
}


/* **************************************************************************************
*  Function Name: Handler_Get_Status
*  Description: Get Handler Threads Status. CAgent will restart current Handler or restart CAgent self if busy.
*  Input :  None
*  Output: char * : pOutStatus       // cagent handler status
*  Return:  handler_success  : Success Init Handler
*			 handler_fail : Fail Init Handler
* **************************************************************************************/
int HANDLER_API Handler_Get_Status( HANDLER_THREAD_STATUS * pOutStatus )
{
	return handler_success;
}


/* **************************************************************************************
*  Function Name: Handler_OnStatusChange
*  Description: Agent can notify handler the status is changed.
*  Input :  PLUGIN_INFO *pluginfo
*  Output: None
*  Return:  None
* ***************************************************************************************/
void HANDLER_API Handler_OnStatusChange( HANDLER_INFO *pluginfo )
{
	printf(" %s> Update Status", strPluginName);
	if(pluginfo)
	{
		memcpy(&g_PluginInfo, pluginfo, sizeof(HANDLER_INFO));
		/*if(g_PluginInfo.agentInfo->status == AGENT_STATUS_ONLINE)
		{
		PowerGetAMTParams();
		}*/
	}
	else
	{
		memset(&g_PluginInfo, 0, sizeof(HANDLER_INFO));
		snprintf( g_PluginInfo.Name, sizeof( g_PluginInfo.Name), "%s", strPluginName );
		g_PluginInfo.RequestID = iRequestID;
		g_PluginInfo.ActionID = iActionID;
	}
	if(pluginfo->agentInfo->status == AGENT_STATUS_ONLINE)
	{
		PowerGetAMTParams();
	}
}


/* **************************************************************************************
*  Function Name: Handler_Start
*  Description: Start Running
*  Input :  None
*  Output: None
*  Return:  handler_success : Success to Start Handler
*           handler_fail : Fail to Start Handler
* ***************************************************************************************/
int HANDLER_API Handler_Start( void )
{
	if(g_PluginInfo.agentInfo->status == AGENT_STATUS_ONLINE)
	{
		PowerGetAMTParams();
	}
	return handler_success;
}


/* **************************************************************************************
*  Function Name: Handler_Stop
*  Description: Stop the handler
*  Input :  None
*  Output: None
*  Return:  handler_success : Success to Stop
*           handler_fail: Fail to Stop handler
* ***************************************************************************************/
int HANDLER_API Handler_Stop( void )
{
	BOOL bRet = TRUE;
	memset(&PowerOnOffHandlerContext, 0, sizeof(void *));
	return bRet;
}

/* **************************************************************************************
*  Function Name: Handler_Recv
*  Description: Receive Packet from MQTT Server
*  Input : char * const topic, 
*			void* const data, 
*			const size_t datalen
*  Output: void *pRev1, 
*			void* pRev2
*  Return: None
* ***************************************************************************************/
void HANDLER_API Handler_Recv( char * const topic, void* const data, const size_t datalen, void *pRev1, void* pRev2  )
{
	int commCmd = unknown_cmd;
	susi_comm_data_t *pSusiCommData = NULL;
	char errorStr[128] = {0};

	PowerOnOffLog(g_loghandle, Normal, " %s>Recv Topic [%s] Data %s", strPluginName, topic, (char*) data );
	printf(" >Recv Topic [%s] Data %s", topic, (char*) data );

	if(!ParseReceivedData(data, datalen, &commCmd))
		return;

	switch(commCmd)
	{
	case power_get_capability_req:
		{
			GetCapability();
			break;
		}
	case power_mac_list_req:
		{
			PowerMacList();
			break;
		}
	case power_last_bootup_time_req:
		{
			PowerLastBootupTime();
			break;
		}
	case power_bootup_period_req:
		{
			PowerBootupPeriod();
			break;
		}
	case power_off_req:
		{
			PowerOff();
			break;
		}
	case power_restart_req:
		{
			PowerRestart();
			break;
		}
	case power_suspend_req:
		{
			PowerSuspend();
			break;
		}
	case power_hibernate_req:
		{
			PowerHibernate();
			break;
		}
	case power_wake_on_lan_req:
		{
			char macsStr[512] = {0};
			if(ParsePowerRecvMacsCmd((char*)data, macsStr))
			{
				PowerWakeOnLan(macsStr);
			}
			else
			{
				memset(errorStr, 0, sizeof(errorStr));
				sprintf_s(errorStr,sizeof(errorStr), "Command(%d) parse error!", power_wake_on_lan_req);

				g_sendcbf(&g_PluginInfo, power_error_rep, errorStr, strlen(errorStr)+1, NULL, NULL);

			}
			break;
		}
	case power_get_amt_params_req:
		{
			PowerGetAMTParams();
			break;
		}
	default: 
		{
			g_sendcbf(&g_PluginInfo, power_error_rep, "Unknown cmd!", strlen("Unknown cmd!")+1, NULL, NULL);
			break;
		}
	}
}

/* **************************************************************************************
*  Function Name: Handler_Get_Capability
*  Description: Get Handler Information specification. 
*  Input :  None
*  Output: char * : pOutReply       // JSON Format
*  Return:  int  : Length of the status information in JSON format
*                :  0 : no data need to trans
* **************************************************************************************/
int HANDLER_API Handler_Get_Capability( char ** pOutReply )
{
	int len = 0; // Data length of the pOutReply 
	char * cpbStr = NULL;
	int jsonStrlen = 0;
	/*power_capability_info_t cpbInfo;
	memset((char*)&cpbInfo, 0, sizeof(power_capability_info_t));
	CollectCpbInfo(&cpbInfo);*/
	jsonStrlen = Parser_PackCpbInfo(&cpbInfo, &cpbStr);
	if(jsonStrlen > 0 && cpbStr != NULL)
	{
		len = strlen(cpbStr);
		*pOutReply = (char *)malloc(len + 1);
		memset(*pOutReply, 0, len + 1);
		strcpy(*pOutReply, cpbStr);
	}
	if(cpbStr)free(cpbStr);

	return len;
}



/* **************************************************************************************
*  Function Name: Handler_AutoReportStart
*  Description: Start Auto Report
*  Input : char *pInQuery
*  Output: None
*  Return: None
* ***************************************************************************************/
void HANDLER_API Handler_AutoReportStart(char *pInQuery)
{
	return ;
}

/* **************************************************************************************
*  Function Name: Handler_AutoReportStop
*  Description: Stop Auto Report
*  Input : char *pInQuery
*  Output: None
*  Return: None
* ***************************************************************************************/
void HANDLER_API Handler_AutoReportStop(char *pInQuery)
{
	return ;
}

/* **************************************************************************************
*  Function Name: Handler_MemoryFree
*  Description: free the mamory allocated for Handler_Get_Capability
*  Input : char *pInData.
*  Output: None
*  Return: None
* ***************************************************************************************/
void HANDLER_API Handler_MemoryFree(char *pInData)
{
	if(pInData)
	{
		free(pInData);
		pInData = NULL;
	}
}