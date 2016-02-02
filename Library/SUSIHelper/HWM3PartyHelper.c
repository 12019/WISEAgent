#include "HWM3PartyHelper.h"
#include <libxml/parser.h>
#include <libxml/xpath.h>
#include <stdlib.h>
#include <string.h>
#include "util_libloader.h"
#include "util_string.h"
#include "AdvPlatform.h"

unsigned int iHWMCount = 0;

#ifdef WIN32
#define DEF_HWM3PARTY_LIB_NAME    "HWM3rdParty.dll"
#else
#define DEF_HWM3PARTY_LIB_NAME    "libhwm3rdparty.so"
#endif

typedef bool (*PHWM3PartyInitialize)();
typedef bool (*PHWM3PartyUninitialize)();
typedef int (*PHWM3PartyGetValue)(unsigned short type, float* retval);
typedef int (*PHWM3PartyGetPlatformName)(char *platformName, unsigned long *size);
typedef int (*PHWM3PartyHWMAvailable)(void);
typedef int (*PHWM3PartyGetHWMXmlThreshold)(wchar_t* HWMXmlThreshold, unsigned int* size);
typedef int (*PHWM3PartyGetHWMXmlPlatform)(wchar_t* HWMXmlPlatform, unsigned int* size);

void * hHWM3PARTYDll = NULL;
PHWM3PartyInitialize pHWM3PARTYInitialize = NULL;
PHWM3PartyUninitialize pHWM3PARTYUninitialize = NULL;
PHWM3PartyGetValue pHWM3PARTYGetValue = NULL;
PHWM3PartyGetPlatformName pHWM3PARTYGetPlatformName = NULL;
PHWM3PartyHWMAvailable pHWM3PartyHWMAvailable = NULL;
PHWM3PartyGetHWMXmlThreshold pHWM3PARTYGetHWMXmlThreshold = NULL;
PHWM3PartyGetHWMXmlPlatform pHWM3PARTYGetHWMXmlPlatform = NULL;

 void GetHWM3PartyFunction(void * hHWM3PTYDLL)
{
	if(hHWM3PTYDLL!=NULL)
	{
		pHWM3PARTYInitialize = (PHWM3PartyInitialize)util_dlsym(hHWM3PTYDLL, "DllInit");
		pHWM3PARTYUninitialize = (PHWM3PartyUninitialize)util_dlsym(hHWM3PTYDLL, "DllUnInit");
		pHWM3PARTYGetValue = (PHWM3PartyGetValue)util_dlsym(hHWM3PTYDLL, "HWMGetValue");
		pHWM3PARTYGetPlatformName = (PHWM3PartyGetPlatformName)util_dlsym(hHWM3PTYDLL, "DllGetPlatformName");
		pHWM3PartyHWMAvailable = (PHWM3PartyHWMAvailable)util_dlsym(hHWM3PTYDLL, "HWMAvailable");
		pHWM3PARTYGetHWMXmlThreshold = (PHWM3PartyGetHWMXmlThreshold)util_dlsym(hHWM3PTYDLL, "DllGetHWMXmlThreshold");
		pHWM3PARTYGetHWMXmlPlatform = (PHWM3PartyGetHWMXmlPlatform)util_dlsym(hHWM3PTYDLL, "DllGetHWMXmlPlatform");
	}
}

 bool IsExistHWM3PartyLib()
{
	bool bRet = false;
    void * hHWM3Pty = NULL;
	if(util_dlopen(DEF_HWM3PARTY_LIB_NAME, &hHWM3Pty))
	{
		bRet = true;
		util_dlclose(hHWM3Pty);
		hHWM3Pty = NULL;
	}
	return bRet;
}

 bool StartupHWM3PartyLib()
{
   bool bRet = false;
   if(util_dlopen(DEF_HWM3PARTY_LIB_NAME, &hHWM3PARTYDll))
   {
      GetHWM3PartyFunction(hHWM3PARTYDll);
      if(pHWM3PARTYInitialize)
      {
         bRet = pHWM3PARTYInitialize();
      }
   }
   return bRet;
}

 bool CleanupHWM3PartyLib()
{
	bool bRet = false;
	if(pHWM3PARTYUninitialize)
	{
		bRet = pHWM3PARTYUninitialize();
	}
	if(hHWM3PARTYDll != NULL)
	{
		util_dlclose(hHWM3PARTYDll);
		hHWM3PARTYDll = NULL;
		pHWM3PARTYInitialize = NULL;
		pHWM3PARTYUninitialize = NULL;
		pHWM3PARTYGetValue = NULL;
		pHWM3PARTYGetPlatformName = NULL;
		pHWM3PartyHWMAvailable = NULL;
		pHWM3PARTYGetHWMXmlThreshold = NULL;
		pHWM3PARTYGetHWMXmlPlatform = NULL;
	}
	return bRet;
}

 unsigned int HWM3PartyHWMAvailable()
{
	if(iHWMCount == 0 && pHWM3PartyHWMAvailable)
	{
		iHWMCount = pHWM3PartyHWMAvailable();
	}
	return iHWMCount;
}

 bool HWM3PartyGetPlatformName(char *platformName, unsigned long *size)
{
	int iRet = -1;
	if(pHWM3PARTYGetPlatformName)
	{
		iRet = pHWM3PARTYGetPlatformName(platformName, size);
	}
	return iRet < 0 ? false : true;
}

xmlXPathObjectPtr HWM3PTY_GetNodeSet(xmlDocPtr doc, const xmlChar *pXpath) 
{
	xmlXPathContextPtr context = NULL;	    
	xmlXPathObjectPtr xpRet = NULL;		
   if(doc == NULL || pXpath == NULL) return xpRet;
	{
		context = xmlXPathNewContext(doc);		
		if (context != NULL) 
		{	
			xpRet = xmlXPathEvalExpression(pXpath, context); 
			xmlXPathFreeContext(context);				
			if (xpRet != NULL) 
			{
				if (xmlXPathNodeSetIsEmpty(xpRet->nodesetval))   
				{
					xmlXPathFreeObject(xpRet);
					xpRet = NULL;
				}
			}
		}
	}

	return xpRet;	
}

 xmlDocPtr GetHWM3PartyPlatform()
{
	char *data = NULL, *tmp = NULL;
	wchar_t* pPlatform = NULL;
    unsigned int len = 0;
	xmlDocPtr doc = NULL;
	int length = 0;

	if(!pHWM3PARTYGetHWMXmlPlatform) return doc;

	if(pHWM3PARTYGetHWMXmlPlatform(pPlatform, &len)<0) return doc;

	pPlatform = (wchar_t *)malloc(sizeof(wchar_t)*len);
	memset(pPlatform, 0, sizeof(wchar_t)*len);
	if(pHWM3PARTYGetHWMXmlPlatform(pPlatform, &len)<0)  return doc;
	data = UnicodeToUTF8(pPlatform);
	length = strlen(data)+strlen("<?xml version='1.0'?>\n")+1;
	tmp = malloc(length);
	memset(tmp, 0, length);
	sprintf(tmp, "<?xml version='1.0'?>\n");
	strcat(tmp, data);
	free(data);
	doc = xmlReadMemory(tmp, strlen(tmp), NULL, "UTF-8", 0);
	free(tmp);
	return doc;
}

bool HWM3PartyGetHWMPlatformInfo(hwm_info_t * pHWMInfo)
{
	bool bRet = false;
	xmlXPathObjectPtr xpRet = NULL;

	xmlDocPtr doc = GetHWM3PartyPlatform();
	
	if(!doc)return bRet;

	xpRet = HWM3PTY_GetNodeSet(doc, BAD_CAST("/RemoteMonitoring/Platform/Category"));
	if(xpRet) 
	{
		int i = 0;
		xmlNodeSetPtr nodeset = xpRet->nodesetval;
		for (i = 0; i < nodeset->nodeNr; i++) 
		{
			xmlNodePtr catelog = nodeset->nodeTab[i];    
			if(catelog != NULL) 
			{
				xmlNodePtr curNode = NULL;
				xmlAttrPtr curAttr = catelog->properties;
				char type[DEF_HWMTYPE_LENGTH] = {0};
				while(curAttr)
				{
					if(xmlStrcmp(curAttr->name, BAD_CAST("name")) == 0)
					{
						strcpy(type, (char *)curAttr->children->content);
						break;
					}
					curAttr = curAttr->next;
				}

				curNode = catelog->children;
				while(curNode)
				{
					if(xmlStrcmp(curNode->name, BAD_CAST("Type")) == 0)
					{
						xmlAttrPtr curAttr = curNode->properties;
						hwm_item_t* item = NULL;
						char name[DEF_HWMNAME_LENGTH] = {0};
						char tag[DEF_HWMTAG_LENGTH] = {0};
						char unit[DEF_HWMUNIT_LENGTH] = {0};
						while(curAttr)
						{
							if(xmlStrcmp(curAttr->name, BAD_CAST("name")) == 0)
							{
								strcpy(name, (char *)curAttr->children->content);
							}
							else if(xmlStrcmp(curAttr->name, BAD_CAST("tag")) == 0)
							{
								strcpy(tag, (char *)curAttr->children->content);
							}
							else if(xmlStrcmp(curAttr->name, BAD_CAST("unit")) == 0)
							{
								if(strcasecmp(type, DEF_SENSORTYPE_TEMPERATURE)==0)
									strcpy(unit, DEF_UNIT_TEMPERATURE_CELSIUS);
								else
									strcpy(unit, (char *)curAttr->children->content);
							}
							//else if(xmlStrcmp(curAttr->name, BAD_CAST("rule")) == 0)
							//{
							//	strcpy(type, curAttr->children->content);
							//}
							curAttr = curAttr->next;
						}
						item = hwm_FindItem(pHWMInfo, tag);
						if(item == NULL)
						{
							hwm_AddItem(pHWMInfo, type, name, tag, unit, DEF_INVALID_VALUE);
						}
					}
					curNode = curNode->next;
				}
				
			}
		}
		xmlXPathFreeObject(xpRet);
		bRet = true;
	}
	return bRet;
}

bool HWM3PartyGetHWMTempInfo(hwm_info_t * pHWMInfo)
{
	hwm_item_t* curItem = NULL;
	if(!pHWMInfo) return false;
	if(!pHWM3PARTYGetValue) return false;
	curItem = pHWMInfo->items;
	while(curItem)
	{
		if(strcasecmp(curItem->type, DEF_SENSORTYPE_TEMPERATURE)==0)
		{
			float value = 0;
			int id = atoi(curItem->tag+1);
			if(pHWM3PARTYGetValue(id, &value)<0)
				value = DEF_INVALID_VALUE;
			curItem->value = value;
		}
		curItem = curItem->next;
	}
	return true;
}

bool HWM3PartyGetHWMVoltInfo(hwm_info_t * pHWMInfo)
{
	hwm_item_t* curItem = NULL;
	if(!pHWMInfo) return false;
	if(!pHWM3PARTYGetValue) return false;
	curItem = pHWMInfo->items;
	while(curItem)
	{
		if(strcasecmp(curItem->type, DEF_SENSORTYPE_VOLTAGE)==0)
		{
			float value = 0;
			int id = atoi(curItem->tag+1);
			if(pHWM3PARTYGetValue(id, &value)<0)
				value = DEF_INVALID_VALUE;
			curItem->value = value;
		}
		curItem = curItem->next;
	}
	return true;
}

bool HWM3PartyGetHWMFanInfo(hwm_info_t * pHWMInfo)
{
	hwm_item_t* curItem = NULL;
	if(!pHWMInfo) return false;
	if(!pHWM3PARTYGetValue) return false;
	curItem = pHWMInfo->items;
	while(curItem)
	{
		if(strcasecmp(curItem->type, DEF_SENSORTYPE_FANSPEED)==0)
		{
			float value = 0;
			int id = atoi(curItem->tag+1);
			if(pHWM3PARTYGetValue(id, &value)<0)
				value = DEF_INVALID_VALUE;
			curItem->value = value;
		}
		curItem = curItem->next;
	}
	return true;
}

bool HWM3PartyGetHWMCurrentInfo(hwm_info_t * pHWMInfo)
{
	hwm_item_t* curItem = NULL;
	if(!pHWMInfo) return false;
	if(!pHWM3PARTYGetValue) return false;
	curItem = pHWMInfo->items;
	while(curItem)
	{
		if(strcasecmp(curItem->type, DEF_SENSORTYPE_CURRENT)==0)
		{
			float value = 0;
			int id = atoi(curItem->tag+1);
			if(pHWM3PARTYGetValue(id, &value)<0)
				value = DEF_INVALID_VALUE;
			curItem->value = value;
		}
		curItem = curItem->next;
	}
	return true;
}

bool HWM3PartyGetHWMCaseOpenInfo(hwm_info_t * pHWMInfo){
	hwm_item_t* curItem = NULL;
	if(!pHWMInfo) return false;
	if(!pHWM3PARTYGetValue) return false;
	curItem = pHWMInfo->items;
	while(curItem)
	{
		if(strcasecmp(curItem->type, DEF_SENSORTYPE_CASEOPEN)==0)
		{
			float value = 0;
			int id = atoi(curItem->tag+1);
			if(pHWM3PARTYGetValue(id, &value)<0)
				value = DEF_INVALID_VALUE;
			curItem->value = value;
		}
		curItem = curItem->next;
	}
	return true;
}