#include "Parser.h"

bool ParseReceivedData(void* data, int datalen, int * cmdID)
{
	/*{"susiCommData":{"commCmd":251,"catalogID":4,"requestID":10}}*/

	cJSON* root = NULL;
	cJSON* body = NULL;
	cJSON* target = NULL;

	if(!data) return false;
	if(datalen<=0) return false;
	//MonitorLog(g_loghandle, Normal, " %s>Parser_ParseReceivedData [%s]\n", MyTopic, data );
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

int Parser_PackScrrenshotUploadRep(ScreenshotUploadRep * pScreenshotUploadRep, char** outJsonStr)
{
	int iRet = 0;
	char * out = NULL;
	int outLen = 0;
	cJSON *pSUSICommDataItem = NULL;
	if(pScreenshotUploadRep == NULL)return iRet;
	pSUSICommDataItem = cJSON_CreateObject();
	if(strlen(pScreenshotUploadRep->uplFileName))
	{
		cJSON_AddStringToObject(pSUSICommDataItem, SCREENSHOT_FILE_NAME, pScreenshotUploadRep->uplFileName);
	}
	else
	{
		cJSON_AddStringToObject(pSUSICommDataItem, SCREENSHOT_FILE_NAME, "");
	}
	if(pScreenshotUploadRep->base64Str && strlen(pScreenshotUploadRep->base64Str))
	{
		cJSON_AddStringToObject(pSUSICommDataItem, SCREENSHOT_BASE64_INFO, pScreenshotUploadRep->base64Str);
	}
	else
	{
		cJSON_AddStringToObject(pSUSICommDataItem, SCREENSHOT_BASE64_INFO, "");
	}
	if(strlen(pScreenshotUploadRep->status))
	{
		cJSON_AddStringToObject(pSUSICommDataItem, SCREENSHOT_STATUS, pScreenshotUploadRep->status);
	}
	else
	{
		cJSON_AddStringToObject(pSUSICommDataItem, SCREENSHOT_STATUS, "False");
	}
	if(strlen(pScreenshotUploadRep->uplMsg))
	{
		cJSON_AddStringToObject(pSUSICommDataItem, SCREENSHOT_MSG, pScreenshotUploadRep->uplMsg);
	}
	else
	{
		cJSON_AddStringToObject(pSUSICommDataItem, SCREENSHOT_MSG, "");
	}
	out = cJSON_PrintUnformatted(pSUSICommDataItem);
	outLen = strlen(out) + 1;
	*outJsonStr = (char *)(malloc(outLen));
	memset(*outJsonStr, 0, outLen);
	strcpy(*outJsonStr, out);
	//memcpy(*outputStr, out, outLen);
	cJSON_Delete(pSUSICommDataItem);	
	printf("%s\n",out);	
	free(out);
	return iRet = outLen;
}

int Parser_PackScrrenshotError(char * errorStr, char **outputStr)
{
	char * out = NULL;
	int outLen = 0;
	cJSON *pSUSICommDataItem = NULL;
	if(errorStr == NULL || outputStr == NULL) return outLen;

	pSUSICommDataItem = cJSON_CreateObject();
	if(pSUSICommDataItem)
	{
		cJSON_AddStringToObject(pSUSICommDataItem, SCREENSHOT_ERROR_REP, errorStr);
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


int Parser_CreateCapabilityRep(char flagCode, char ** outJsonStr)
{
	int len = 0;
	cJSON * root = NULL;
	cJSON * parent = NULL;
	cJSON * child = NULL;
	if (NULL == outJsonStr) return len;

	root = cJSON_CreateObject();
	parent = root;   child = cJSON_CreateObject();  cJSON_AddItemToObject(parent, MyTopic, child);
	parent = child;  child = cJSON_CreateObject();  cJSON_AddItemToObject(parent, SCREENSHOT_INFOMATION, child);
	parent = child;  child = cJSON_CreateArray();   cJSON_AddItemToObject(parent, "e", child);
	cJSON_AddStringToObject(parent, "bn", SCREENSHOT_INFOMATION);
	cJSON_AddBoolToObject(parent, "nonSensorData", 1);

	parent = child;  child = NULL;
	switch (flagCode)
	{
	case SCREENSHOT_FLAGCODE_NONE:
		child = cJSON_CreateObject();
		cJSON_AddStringToObject(child, "n", SCREENSHOT_FUNCTION_LIST);
		cJSON_AddStringToObject(child, "sv", SCREENSHOT_FLAG_NONE);
		cJSON_AddItemToArray(parent, child);

		child = cJSON_CreateObject();
		cJSON_AddStringToObject(child, "n", SCREENSHOT_FUNCTION_CODE);
		cJSON_AddNumberToObject(child, "v", SCREENSHOT_FLAGCODE_NONE);
		cJSON_AddItemToArray(parent, child);
		break;
	
	default:
	case SCREENSHOT_FLAGCODE_INTERNAL:
		child = cJSON_CreateObject();
		cJSON_AddStringToObject(child, "n", SCREENSHOT_FUNCTION_LIST);
		cJSON_AddStringToObject(child, "sv", SCREENSHOT_FLAG_INTERNAL);
		cJSON_AddItemToArray(parent, child);

		child = cJSON_CreateObject();
		cJSON_AddStringToObject(child, "n", SCREENSHOT_FUNCTION_CODE);
		cJSON_AddNumberToObject(child, "v", SCREENSHOT_FLAGCODE_INTERNAL);
		cJSON_AddItemToArray(parent, child);
		break;
	}

	*outJsonStr = cJSON_PrintUnformatted(root);
	if (*outJsonStr) len = strlen(*outJsonStr);
	else             len = 0;
	cJSON_Delete(root);
	return len;	
}



