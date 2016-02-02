#include "MsgGenerator.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "cJSON.h"
#include "AdvPlatform.h"

#define TAG_BASE_NAME		"bn"
#define TAG_ATTR_NAME		"n"
#define TAG_VERSION			"ver"
#define TAG_E_NODE			"e"
#define TAG_VALUE			"v"
#define TAG_BOOLEAN			"bv"
#define TAG_STRING			"sv"
#define TAG_MAX				"max"
#define TAG_MIN				"min"
#define TAG_ASM				"asm"
#define TAG_UNIT			"u"
#define TAG_STATUS_CODE		"StatusCode"

#pragma region Add_Resource
MSG_CLASSIFY_T* MSG_CreateRoot()
{
	MSG_CLASSIFY_T *pMsg = malloc(sizeof(MSG_CLASSIFY_T));
	if(pMsg)
	{
		memset(pMsg, 0, sizeof(MSG_CLASSIFY_T));
		pMsg->type = class_type_root;
	}
	return pMsg;
}

MSG_CLASSIFY_T* MSG_AddClassify(MSG_CLASSIFY_T *pNode, char const* name, char const* version, bool bArray, bool isIoT)
{
	MSG_CLASSIFY_T *pCurNode = NULL;
	if(!pNode || !name)
		return pNode;

	pCurNode = malloc(sizeof(MSG_CLASSIFY_T));
	if(pCurNode)
	{
		memset(pCurNode, 0, sizeof(MSG_CLASSIFY_T));
		pCurNode->type = bArray ? class_type_array : class_type_object;
		if(name)
		{
			strncpy(pCurNode->classname, name, strlen(name));
		}

		if(version)
		{
			strncpy(pCurNode->version, version, strlen(version));
		}

		pCurNode->bIoTFormat = isIoT;

		if(!pNode->sub_list)
		{
			pNode->sub_list = pCurNode;
		}
		else
		{
			MSG_CLASSIFY_T *pLastNode = pNode->sub_list;
			while(pLastNode->next)
			{
				pLastNode = pLastNode->next;
			}
			pLastNode->next = pCurNode;
		}
	}
	return pCurNode;
}

MSG_ATTRIBUTE_T* MSG_AddAttribute(MSG_CLASSIFY_T* pClass, char const* attrname, bool isSensorData)
{
	MSG_ATTRIBUTE_T* curAttr = NULL;
	MSG_ATTRIBUTE_T* lastAttr = NULL;
	if(!pClass)
		return curAttr;
	curAttr = malloc(sizeof(MSG_ATTRIBUTE_T));
	if(curAttr)
	{
		memset(curAttr, 0, sizeof(MSG_ATTRIBUTE_T));

		if(attrname)
		{
			strncpy(curAttr->name, attrname, strlen(attrname));
		}
		curAttr->bSensor = isSensorData;

		if(pClass->attr_list == NULL)
		{	
			
			pClass->attr_list = curAttr;
		
		}
		else
		{
			lastAttr = pClass->attr_list;
			while(lastAttr->next)
			{	
				lastAttr = lastAttr->next;
			}
			lastAttr->next = curAttr;
		}
	}
	return curAttr;
}
#pragma endregion Add_Resource

#pragma region Release_Resource
void ReleaseAttribute(MSG_ATTRIBUTE_T* attr)
{
	if(!attr)
		return;

	if(attr->sv)
	{
		free(attr->sv);
		attr->sv = NULL;
	}

	free(attr);
	attr = NULL;
}

void ReleaseClassify(MSG_CLASSIFY_T* classify)
{
	MSG_ATTRIBUTE_T* curAttr = NULL;
	MSG_ATTRIBUTE_T* nxtAttr = NULL;

	MSG_CLASSIFY_T* curSubtype = NULL;
	MSG_CLASSIFY_T* nxtSubtype = NULL;

	if(!classify)
		return;
	curAttr = classify->attr_list;
	while (curAttr)
	{
		nxtAttr = curAttr->next;
		ReleaseAttribute(curAttr);
		curAttr = nxtAttr;
	}

	curSubtype = classify->sub_list;
	while (curSubtype)
	{
		nxtSubtype = curSubtype->next;
		ReleaseClassify(curSubtype);
		curSubtype = nxtSubtype;
	}

	free(classify);
	classify = NULL;
}

bool MSG_DelAttribute(MSG_CLASSIFY_T* pNode, char* name, bool isSensorData)
{
	MSG_ATTRIBUTE_T* curAttr = NULL;
	MSG_ATTRIBUTE_T* nxtAttr = NULL;
	if(!pNode || !name)
		return false;
	curAttr = pNode->attr_list;
	if(curAttr->bSensor == isSensorData)
	{
		if(!strcmp(curAttr->name, name))
		{
			pNode->attr_list = curAttr->next;
			ReleaseAttribute(curAttr);
			return true;
		}
	}

	nxtAttr = curAttr->next;
	while (nxtAttr)
	{
		if(nxtAttr->bSensor == isSensorData)
		{
			if(!strcmp(nxtAttr->name, name))
			{
				curAttr->next = nxtAttr->next;
				ReleaseAttribute(nxtAttr);
				return true;
			}
		}
		curAttr = nxtAttr;
		nxtAttr = curAttr->next;
	}
	return false;
}

bool MSG_DelClassify(MSG_CLASSIFY_T* pNode, char* name)
{
	MSG_CLASSIFY_T* curClass = NULL;
	MSG_CLASSIFY_T* nxtClass = NULL;
	if(!pNode || !name)
		return false;
	curClass = pNode->sub_list;
	if(!strcmp(curClass->classname, name))		
	{
		pNode->sub_list = curClass->next;
		ReleaseClassify(curClass);
		return true;
	}

	nxtClass = curClass->next;
	while (nxtClass)
	{
		if(!strcmp(nxtClass->classname, name))
		{
			curClass->next = nxtClass->next;
			ReleaseClassify(nxtClass);
			return true;
		}
		curClass = nxtClass;
		nxtClass = curClass->next;
	}
	return false;
}

void MSG_ReleaseRoot(MSG_CLASSIFY_T* classify)
{
	ReleaseClassify(classify);
}
#pragma endregion Release_Resource

#pragma region Find_Resource
MSG_CLASSIFY_T* MSG_FindClassify(MSG_CLASSIFY_T* pNode, char const* name)
{
	MSG_CLASSIFY_T* curClass = NULL;
	MSG_CLASSIFY_T* nxtClass = NULL;
	if(!pNode || !name)
		return curClass;
	curClass = pNode->sub_list;
	while (curClass)
	{
		nxtClass = curClass->next;
		if(!strcmp(curClass->classname, name))
			return curClass;
		curClass = nxtClass;
	}
	return NULL;
}

MSG_ATTRIBUTE_T* MSG_FindAttribute(MSG_CLASSIFY_T* root, char const* senname, bool isSensorData)
{
	MSG_ATTRIBUTE_T* curAttr = NULL;
	MSG_ATTRIBUTE_T* nxtAttr = NULL;
	if(!root || !senname)
		return curAttr;
	curAttr = root->attr_list;
	while (curAttr)
	{
		nxtAttr = curAttr->next;
		if(curAttr->bSensor == isSensorData)
		{
			if(!strcmp(curAttr->name, senname))
				return curAttr;
		}
		curAttr = nxtAttr;
	}
	return NULL;
}

bool MSG_SetFloatValue(MSG_ATTRIBUTE_T* attr, float value, char* readwritemode, char *unit)
{
	return MSG_SetDoubleValue(attr, value, readwritemode, unit);
}

bool MSG_SetFloatValueWithMaxMin(MSG_ATTRIBUTE_T* attr, float value, char* readwritemode, float max, float min, char *unit)
{
	return MSG_SetDoubleValueWithMaxMin(attr, value, readwritemode, max, min, unit);
}

bool MSG_SetDoubleValue(MSG_ATTRIBUTE_T* attr, double value, char* readwritemode, char *unit)
{
	if(!attr)
		return false;
	attr->v = value;
	attr->type = attr_type_numeric;
	attr->bRange = false;
	if(readwritemode)
		strncpy(attr->readwritemode, readwritemode, strlen(readwritemode));
	if(unit)
		strncpy(attr->unit, unit, strlen(unit));
	return true;
}

bool MSG_SetDoubleValueWithMaxMin(MSG_ATTRIBUTE_T* attr, double value, char* readwritemode, double max, double min, char *unit)
{
	if(!attr)
		return false;
	attr->v = value;
	attr->type = attr_type_numeric;
	attr->bRange = true;
	attr->max = max;
	attr->min = min;
	if(readwritemode)
		strncpy(attr->readwritemode, readwritemode, strlen(readwritemode));
	if(unit)
		strncpy(attr->unit, unit, strlen(unit));
	return true;
}

bool MSG_SetBoolValue(MSG_ATTRIBUTE_T* attr, bool bvalue, char* readwritemode)
{
	if(!attr)
		return false;
	attr->bv = bvalue;
	attr->type = attr_type_boolean;
	if(readwritemode)
		strncpy(attr->readwritemode, readwritemode, strlen(readwritemode));
	attr->bRange = false;
	return true;
}

bool MSG_SetStringValue(MSG_ATTRIBUTE_T* attr, char *svalue, char* readwritemode)
{
	int length = 0;
	if(!attr)
		return false;
	length = strlen(svalue);
	if(attr->sv)
		free(attr->sv);
	attr->sv = malloc(length+1);
	memset(attr->sv, 0, length+1);
	strncpy(attr->sv, svalue, strlen(svalue));
	attr->type = attr_type_string;
	if(readwritemode)
		strncpy(attr->readwritemode, readwritemode, strlen(readwritemode));
	attr->bRange = false;
	return true;
}
#pragma endregion Find_Resource

#pragma region Generate_JSON 
bool MatchFilterString(char* target, char** filter, int filterlength)
{
	int i=0;
	if(!filter)
		return true;

	if(!target)
		return false;

	for(i=0; i<filterlength; i++)
	{
		if(!strcasecmp(target, filter[i]))
			return true;
	}
	return false;
}

bool AddJSONAttribute(cJSON *pClass, MSG_ATTRIBUTE_T *attr_list, char** filter, int length)
{
	cJSON* pAttr = NULL;
	cJSON* pENode = NULL;
	MSG_ATTRIBUTE_T* curAttr = NULL;
	MSG_ATTRIBUTE_T* nxtAttr = NULL;

	if(!pClass || !attr_list)
		return false;

	curAttr = attr_list;
	while (curAttr)
	{
		nxtAttr = curAttr->next;

		if(curAttr->bSensor)
		{	
			if(!pENode)
			{
				pENode = cJSON_CreateArray();
				cJSON_AddItemToObject(pClass, TAG_E_NODE, pENode);
			}
			pAttr = cJSON_CreateObject();
			cJSON_AddItemToArray(pENode, pAttr);
			if(MatchFilterString(TAG_ATTR_NAME, filter, length))
				cJSON_AddStringToObject(pAttr, TAG_ATTR_NAME, curAttr->name);
			switch (curAttr->type)
			{
			case attr_type_numeric:
				{
					if(MatchFilterString(TAG_VALUE, filter, length))
						cJSON_AddNumberToObject(pAttr, TAG_VALUE, curAttr->v);
					if(curAttr->bRange)
					{
						if(MatchFilterString(TAG_MAX, filter, length))
							cJSON_AddNumberToObject(pAttr, TAG_MAX, curAttr->max);
						if(MatchFilterString(TAG_MIN, filter, length))
							cJSON_AddNumberToObject(pAttr, TAG_MIN, curAttr->min);
					}
					if(MatchFilterString(TAG_ASM, filter, length))
						cJSON_AddStringToObject(pAttr, TAG_ASM, curAttr->readwritemode);
	

					if(strlen(curAttr->unit)>0)
					{
						if(MatchFilterString(TAG_UNIT, filter, length))
							cJSON_AddStringToObject(pAttr, TAG_UNIT, curAttr->unit);
					}
				}
				break;
			case attr_type_boolean:
				{
					if(MatchFilterString(TAG_BOOLEAN, filter, length))
					{
						if(curAttr->bv)
							cJSON_AddTrueToObject(pAttr, TAG_BOOLEAN);
						else
							cJSON_AddFalseToObject(pAttr, TAG_BOOLEAN);
					}

					if(MatchFilterString(TAG_ASM, filter, length))
						cJSON_AddStringToObject(pAttr, TAG_ASM, curAttr->readwritemode);


				}
				break;
			case attr_type_string:
				{
					if(strlen(curAttr->sv)>0)
					{
						if(MatchFilterString(TAG_STRING, filter, length))
							cJSON_AddStringToObject(pAttr, TAG_STRING, curAttr->sv);
					}
					if(MatchFilterString(TAG_ASM, filter, length))
						cJSON_AddStringToObject(pAttr, TAG_ASM, curAttr->readwritemode);

				}
				break;
			default:
				{
				}
				break;
			}
		}
		else
		{	
			switch (curAttr->type)
			{
			case attr_type_numeric:
				{
					if(MatchFilterString(curAttr->name, filter, length))
						cJSON_AddNumberToObject(pClass, curAttr->name, curAttr->v);
				}
				break;
			case attr_type_boolean:
				{
					if(MatchFilterString(curAttr->name, filter, length))
					{
						if(curAttr->bv)
							cJSON_AddTrueToObject(pClass, curAttr->name);
						else
							cJSON_AddFalseToObject(pClass, curAttr->name);
					}
				}
				break;
			case attr_type_string:
				{
					if(strlen(curAttr->sv)>0)
					{
						if(MatchFilterString(curAttr->name, filter, length))
							cJSON_AddStringToObject(pClass, curAttr->name, curAttr->sv);
					}
				}
				break;
			default:
				{
				}
				break;
			}
		}

		curAttr = nxtAttr;
	};

	return true;
}

bool AddJSONClassify(cJSON *pRoot, MSG_CLASSIFY_T* msg, char** filter, int length)
{
	cJSON* pClass = NULL;

	MSG_CLASSIFY_T* curClass = NULL;
	MSG_CLASSIFY_T* nxtClass = NULL;

	if(!pRoot || !msg)
		return false;

	if(msg->type == class_type_root)
		pClass = pRoot;
	else
	{
		if(msg->type == class_type_array)
			pClass = cJSON_CreateArray();
		else
		{
			pClass = cJSON_CreateObject();
			if(msg->bIoTFormat)
			{
				if(MatchFilterString(TAG_BASE_NAME, filter, length))
					cJSON_AddStringToObject(pClass, TAG_BASE_NAME, msg->classname);
			}
		}

		if(pRoot->type == cJSON_Array)
			cJSON_AddItemToArray(pRoot, pClass);
		else
			cJSON_AddItemToObject(pRoot, msg->classname, pClass);
	}

	//if(msg->type != class_type_array)
	//{
		if(msg->attr_list)
		{
			AddJSONAttribute(pClass, msg->attr_list, filter, length);
		}
	//}

	if(msg->sub_list)
	{
		curClass = msg->sub_list;
		while (curClass)
		{
			nxtClass = curClass->next;

			AddJSONClassify(pClass, curClass, filter, length);

			curClass = nxtClass;
		};
	}
/*	
	if(msg->next)
	{
		curClass = msg->next;
		while (curClass)
		{
			nxtClass = curClass->next;

			AddJSONClassify(pClass, curClass);

			curClass = nxtClass;
		};
	}
*/	
	return true;
}

char *MSG_PrintUnformatted(MSG_CLASSIFY_T* msg)
{
	char* buffer = NULL;
	cJSON *pRoot = NULL;

	pRoot = cJSON_CreateObject();

	AddJSONClassify(pRoot, msg, NULL, 0);
	
	buffer = cJSON_PrintUnformatted(pRoot);
	cJSON_Delete(pRoot);
	pRoot = NULL;
	return buffer;
}

char *MSG_PrintWithFiltered(MSG_CLASSIFY_T* msg, char** filter, int length)
{
	char* buffer = NULL;
	cJSON *pRoot = NULL;

	pRoot = cJSON_CreateObject();

	AddJSONClassify(pRoot, msg, filter, length);
	
	buffer = cJSON_PrintUnformatted(pRoot);
	cJSON_Delete(pRoot);
	pRoot = NULL;
	return buffer;
}


//---------------------------------------------------Others
bool MSG_Find_Sensor(MSG_CLASSIFY_T *msg,char *path)
{
	MSG_CLASSIFY_T  *classify=NULL;
	MSG_ATTRIBUTE_T* attr;

	char *delim = "/";
	char *str=NULL;
	char temp[200];
	char handler[30];
	char type[30];
	char name[30];
	int i=0;
    
	strcpy(temp,path);
	str = strtok(temp,delim);

	while (str != NULL)
	{	
		if(i==0)
			strcpy(handler,str);
		else if(i==1)
			strcpy(type,str);
		else	
			strcpy(name,str);
		str = strtok (NULL, delim);
		i++;
	}

	if(msg)
	{		
		classify=IoT_FindGroup(msg,type);
		if(classify)
		{	attr=IoT_FindSensorNode(classify,name);
			if(attr)
				return true;
			else 
				return false;
		}
		else
			return false;
	}
	else
		return false;
}


/*int  MSG_Find_Type_Sensor(MSG_CLASSIFY_T *msg,char *path)
{
	MSG_CLASSIFY_T  *classify=NULL;
	MSG_ATTRIBUTE_T* attr;

	char *delim = "/";
	char *str=NULL;
	char temp[200];
	char handler[30];
	char type[30];
	char name[30];
	int  Check_Level=0;
	int i=0;
    
	strcpy(temp,path);
	str = strtok(temp,delim);

	while (str != NULL)
	{	
		if(i==0)
			strcpy(handler,str);
		else if(i==1)
			strcpy(type,str);
		else	
			strcpy(name,str);
		str = strtok (NULL, delim);
		i++;
	}

	if(msg)
	{		
		classify=IoT_FindGroup(msg,type);
		if(classify)
		{	Check_Level++;
			attr=IoT_FindSensorNode(classify,name);
			if(attr)
				Check_Level++;			
		}

	}
	else
		return Check_Level;
}*/

#pragma endregion Generate_JSON 


