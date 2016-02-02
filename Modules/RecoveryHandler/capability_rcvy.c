#include "capability_rcvy.h"


#define cJSON_AddRcvyCapabilityInfoToObject(object, name, pR) cJSON_AddItemToObject(object, name, cJSON_CreateRcvyCapabilityInfo(pR))


static cJSON * cJSON_CreateRcvyCapabilityItem(const char *itemName, const char *valueType, const void *value)
{
	cJSON * pRcvyCapabilityItem = NULL;
	if (itemName && valueType && value) {
		pRcvyCapabilityItem = cJSON_CreateObject();
		cJSON_AddStringToObject(pRcvyCapabilityItem, "n", itemName);
		if (!strncmp(valueType, "bv", 2)) {
			cJSON_AddBoolToObject(pRcvyCapabilityItem, "bv", *(int *)value);
		} else if (!strncmp(valueType, "sv", 2)) {
			cJSON_AddStringToObject(pRcvyCapabilityItem, "sv", (const char *)value);//must be careful
		} else {
			cJSON_AddNumberToObject(pRcvyCapabilityItem, "v", *(int *)value);
		}
	}
	return pRcvyCapabilityItem;
}

static cJSON * cJSON_CreateRcvyCapabilityInfo(rcvy_capability_t *pRcvyCapability)
{	
	cJSON * pRcvyCapabilityInfoItem = NULL;

	if(pRcvyCapability) {
		cJSON * pRcvyCapabilityArrayItem = NULL;
		//Array
		cJSON * pRcvyCapabilityArray = cJSON_CreateArray();		
		cJSON_AddItemToArray(pRcvyCapabilityArray, \
			cJSON_CreateRcvyCapabilityItem(RCVY_STATUS_IS_INSTALLED, "bv", \
			&pRcvyCapability->rcvy_status.isInstalled));
		cJSON_AddItemToArray(pRcvyCapabilityArray, \
			cJSON_CreateRcvyCapabilityItem(RCVY_STATUS_IS_ACTIVATED, "bv", \
			&pRcvyCapability->rcvy_status.isActivated));
		cJSON_AddItemToArray(pRcvyCapabilityArray, \
			cJSON_CreateRcvyCapabilityItem(RCVY_STATUS_IS_EXPIRED, "bv", \
			&pRcvyCapability->rcvy_status.isExpired));
		cJSON_AddItemToArray(pRcvyCapabilityArray, \
			cJSON_CreateRcvyCapabilityItem(RCVY_STATUS_IS_EXIST_ASZ, "bv", \
			&pRcvyCapability->rcvy_status.isExistASZ));
		cJSON_AddItemToArray(pRcvyCapabilityArray, \
			cJSON_CreateRcvyCapabilityItem(RCVY_STATUS_IS_NEWERVER, "bv", \
			&pRcvyCapability->rcvy_status.isExistNewerVer));
		cJSON_AddItemToArray(pRcvyCapabilityArray, \
			cJSON_CreateRcvyCapabilityItem(RCVY_STATUS_IS_ACRREADY, "bv", \
			&pRcvyCapability->rcvy_status.isAcrReady));
		cJSON_AddItemToArray(pRcvyCapabilityArray, \
			cJSON_CreateRcvyCapabilityItem(RCVY_STATUS_VERSION, "sv", \
			pRcvyCapability->rcvy_status.version));//rcvy_status.version is an address of string array
		cJSON_AddItemToArray(pRcvyCapabilityArray, \
			cJSON_CreateRcvyCapabilityItem(RCVY_STATUS_LATEST_BK_TIME, "sv", \
			pRcvyCapability->rcvy_status.lastBackupTime));//Same as above
		cJSON_AddItemToArray(pRcvyCapabilityArray, \
			cJSON_CreateRcvyCapabilityItem(RCVY_STATUS_ACTION_MSG, "sv", \
			pRcvyCapability->rcvy_status.actionMsg));//Same as above
		cJSON_AddItemToArray(pRcvyCapabilityArray, \
			cJSON_CreateRcvyCapabilityItem(RCVY_STATUS_LWARNING_MSG, "sv", \
			pRcvyCapability->rcvy_status.lastWarningMsg));//Same as above
		cJSON_AddItemToArray(pRcvyCapabilityArray, \
			cJSON_CreateRcvyCapabilityItem("functionList", "sv", \
			pRcvyCapability->pRcvyFuncList));//pRcvyCapability->pRcvyFuncList is a pointer and point to string
		cJSON_AddItemToArray(pRcvyCapabilityArray, \
			cJSON_CreateRcvyCapabilityItem("functionCode", "v", \
			&pRcvyCapability->rcvyFuncCode));
		//add array to array item
		pRcvyCapabilityArrayItem = cJSON_CreateObject();
		cJSON_AddItemToObject(pRcvyCapabilityArrayItem, "e", pRcvyCapabilityArray);

		//add 'non-sensor data flag'	
		cJSON_AddStringToObject(pRcvyCapabilityArrayItem, "bn", "Information");
		cJSON_AddBoolToObject(pRcvyCapabilityArrayItem, "nonSensorData", 1);

		//add array item to "Information" item
		pRcvyCapabilityInfoItem = cJSON_CreateObject();
		cJSON_AddItemToObject(pRcvyCapabilityInfoItem, "Information", pRcvyCapabilityArrayItem);
	}
	return pRcvyCapabilityInfoItem;
}

int GetRcvyCapability(rcvy_capability_get_way_t way, char ** pOutReply)
{
	int len = 0;

	if (pOutReply) {
		rcvy_capability_t capability;
		cJSON *pRcvyCapability = NULL; 

		GetRcvyStatus(&capability.rcvy_status);
		capability.rcvyFuncCode = RCVY_FUNC_CODE;
		capability.pRcvyFuncList = RCVY_FUNC_LIST;	
		if (way == rcvy_capability_api_way) {
			pRcvyCapability = cJSON_CreateObject();	
			cJSON_AddRcvyCapabilityInfoToObject(pRcvyCapability, MyTopic, &capability);
		} else {
			pRcvyCapability = cJSON_CreateRcvyCapabilityInfo(&capability);
		}
		
		if (pRcvyCapability) {
			len = RcvyReplyMsgPack(pRcvyCapability, pOutReply);
		}
	}
	return len;
}

