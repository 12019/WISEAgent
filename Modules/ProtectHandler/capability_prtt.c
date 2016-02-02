#include "capability_prtt.h"


#define cJSON_AddPrttCapabilityInfoToObject(object, name, pR) cJSON_AddItemToObject(object, name, cJSON_CreatePrttCapabilityInfo(pR))


static cJSON * cJSON_CreatePrttCapabilityItem(const char *itemName, const char *valueType, const void *value)
{
	cJSON * pPrttCapabilityItem = NULL;
	if (itemName && valueType && value) {
		pPrttCapabilityItem = cJSON_CreateObject();
		cJSON_AddStringToObject(pPrttCapabilityItem, "n", itemName);
		if (!strncmp(valueType, "bv", 2)) {
			cJSON_AddBoolToObject(pPrttCapabilityItem, "bv", *(int *)value);
		} else if (!strncmp(valueType, "sv", 2)) {
			cJSON_AddStringToObject(pPrttCapabilityItem, "sv", (const char *)value);//must be careful
		} else {
			cJSON_AddNumberToObject(pPrttCapabilityItem, "v", *(int *)value);
		}
	}
	return pPrttCapabilityItem;
}

static cJSON * cJSON_CreatePrttCapabilityInfo(prtt_capability_t *pPrttCapability)
{	
	cJSON * pPrttCapabilityInfoItem = NULL;

	if(pPrttCapability) {
		cJSON * pPrttCapabilityArrayItem = NULL;
		//Array
		cJSON * pPrttCapabilityArray = cJSON_CreateArray();		
		cJSON_AddItemToArray(pPrttCapabilityArray, \
			cJSON_CreatePrttCapabilityItem(PRTT_STATUS_IS_INSTALLED, "bv", \
			&pPrttCapability->prtt_status.isInstalled));
		cJSON_AddItemToArray(pPrttCapabilityArray, \
			cJSON_CreatePrttCapabilityItem(PRTT_STATUS_IS_ACTIVATED, "bv", \
			&pPrttCapability->prtt_status.isActivated));
		cJSON_AddItemToArray(pPrttCapabilityArray, \
			cJSON_CreatePrttCapabilityItem(PRTT_STATUS_IS_EXPIRED, "bv", \
			&pPrttCapability->prtt_status.isExpired));
		cJSON_AddItemToArray(pPrttCapabilityArray, \
			cJSON_CreatePrttCapabilityItem(PRTT_STATUS_IS_PROTECTION, "bv", \
			&pPrttCapability->prtt_status.isProtection));
		cJSON_AddItemToArray(pPrttCapabilityArray, \
			cJSON_CreatePrttCapabilityItem(PRTT_STATUS_IS_NEWERVER, "bv", \
			&pPrttCapability->prtt_status.isExistNewerVer));
		cJSON_AddItemToArray(pPrttCapabilityArray, \
			cJSON_CreatePrttCapabilityItem(PRTT_STATUS_VERSION, "sv", \
			pPrttCapability->prtt_status.version));//prtt_status.version is an address of string array
		cJSON_AddItemToArray(pPrttCapabilityArray, \
			cJSON_CreatePrttCapabilityItem(PRTT_STATUS_ACTION_MSG, "sv", \
			pPrttCapability->prtt_status.actionMsg));//Same as above
		cJSON_AddItemToArray(pPrttCapabilityArray, \
			cJSON_CreatePrttCapabilityItem(PRTT_STATUS_LWARNING_MSG, "sv", \
			pPrttCapability->prtt_status.lastWarningMsg));//Same as above
		cJSON_AddItemToArray(pPrttCapabilityArray, \
			cJSON_CreatePrttCapabilityItem("functionList", "sv", \
			pPrttCapability->pPrttFuncList));//pPrttCapability->pPrttFuncList is a pointer and point to string
		cJSON_AddItemToArray(pPrttCapabilityArray, \
			cJSON_CreatePrttCapabilityItem("functionCode", "v", \
			&pPrttCapability->prttFuncCode));
		//add array to array item
		pPrttCapabilityArrayItem = cJSON_CreateObject();
		cJSON_AddItemToObject(pPrttCapabilityArrayItem, "e", pPrttCapabilityArray);

		//add 'non-sensor data flag'	
		cJSON_AddStringToObject(pPrttCapabilityArrayItem, "bn", "Information");
		cJSON_AddBoolToObject(pPrttCapabilityArrayItem, "nonSensorData", 1);

		//add array item to "Information" item
		pPrttCapabilityInfoItem = cJSON_CreateObject();
		cJSON_AddItemToObject(pPrttCapabilityInfoItem, "Information", pPrttCapabilityArrayItem);
	}
	return pPrttCapabilityInfoItem;
}

int GetPrttCapability(prtt_capability_get_way_t way, char ** pOutReply)
{
	int len = 0;

	if (pOutReply) {
		prtt_capability_t capability;
		cJSON *pPrttCapability = NULL; 

		GetPrttStatus(&capability.prtt_status);
		capability.prttFuncCode = PRTT_FUNC_CODE;
		capability.pPrttFuncList = PRTT_FUNC_LIST;	
		if (way == prtt_capability_api_way) {
			pPrttCapability = cJSON_CreateObject();	
			cJSON_AddPrttCapabilityInfoToObject(pPrttCapability, strPluginName, &capability);
		} else {
			pPrttCapability = cJSON_CreatePrttCapabilityInfo(&capability);
		}
		
		if (pPrttCapability) {
			len = PrttReplyMsgPack(pPrttCapability, pOutReply);
		}
	}
	return len;
}

