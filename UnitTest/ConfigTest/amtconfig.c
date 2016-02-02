#include <XMLBase.h>
#include "amtconfig.h"

#define XML_CONFIG_ROOT "XMLConfigSettings"
#define XML_CONFIG_BASIC "Customize"

#define AMT_EN_KEY				"AmtEn"
#define AMT_ID_KEY				"AmtID"
#define AMT_PWD_KEY				"AmtPwd"

int amt_load(char const * configFile, susiaccess_amt_conf_body_t * conf)
{
	int iRet = false;
	xml_doc_info * doc = NULL;
	bool bModify = false;

	if(configFile == NULL) 
		return iRet;
	if(conf == NULL)
		return iRet;
	
	doc = xml_Loadfile(configFile, XML_CONFIG_ROOT, XML_CONFIG_BASIC);
	if(doc == NULL)
		return iRet;

	memset(conf, 0, sizeof(susiaccess_amt_conf_body_t));

	if(!xml_GetItemValue(doc, AMT_EN_KEY, conf->amtEn, sizeof(conf->amtEn)))
		memset(conf->amtEn, 0, sizeof(conf->amtEn));
	if(strlen(conf->amtEn)<=0)
	{
		strncpy(conf->amtEn, "False", strlen("False")+1);
		xml_SetItemValue(doc, AMT_EN_KEY, conf->amtEn);
		bModify = true;
	}

	if(!xml_GetItemValue(doc, AMT_ID_KEY, conf->amtID, sizeof(conf->amtID)))
		memset(conf->amtID, 0, sizeof(conf->amtID));
	if(strlen(conf->amtID)<=0)
	{
		strncpy(conf->amtID, "", strlen("")+1);
		xml_SetItemValue(doc, AMT_ID_KEY, conf->amtID);
		bModify = true;
	}

	if(!xml_GetItemValue(doc, AMT_PWD_KEY, conf->amtPwd, sizeof(conf->amtPwd)))
		memset(conf->amtPwd, 0, sizeof(conf->amtPwd));
	if(strlen(conf->amtPwd)<=0)
	{
		strncpy(conf->amtPwd, "", strlen("")+1);
		xml_SetItemValue(doc, AMT_PWD_KEY, conf->amtPwd);
		bModify = true;
	}

	if(bModify)
	{
		xml_SaveFile(configFile, doc);
	}
	iRet = true;
	xml_FreeDoc(doc);
	return iRet;
}

int amt_save(char const * configFile, susiaccess_amt_conf_body_t const * const conf)
{
	int iRet = false;
	xml_doc_info * doc = NULL;
	
	if(configFile == NULL) 
		return iRet;
	if(conf == NULL)
		return iRet;
	
	doc = xml_Loadfile(configFile, XML_CONFIG_ROOT, XML_CONFIG_BASIC);
	if(doc == NULL)
	{
		doc = xml_CreateDoc( XML_CONFIG_ROOT, XML_CONFIG_BASIC);
	}

	if(xml_SetItemValue(doc,AMT_EN_KEY, conf->amtEn)<0)
		goto SAVE_EXIT;

	if(xml_SetItemValue(doc,AMT_ID_KEY, conf->amtID)<0)
		goto SAVE_EXIT;

	if(xml_SetItemValue(doc,AMT_PWD_KEY, conf->amtPwd)<0)
		goto SAVE_EXIT;

	xml_SaveFile(configFile, doc);
	iRet = true;
SAVE_EXIT:
	xml_FreeDoc(doc);
	return iRet;
}

int amt_create(char const * configFile, susiaccess_amt_conf_body_t * conf)
{
	int iRet = false;
	xml_doc_info * doc = NULL;

	if(configFile == NULL) 
		return iRet;
	if(conf == NULL)
		return iRet;

	memset(conf, 0, sizeof(susiaccess_amt_conf_body_t));

	doc = xml_Loadfile(configFile, XML_CONFIG_ROOT, XML_CONFIG_BASIC);
	if(doc == NULL)
	{
		doc = xml_CreateDoc( XML_CONFIG_ROOT, XML_CONFIG_BASIC);

		strncpy(conf->amtEn, "False", strlen("False")+1);
		if(xml_SetItemValue(doc,AMT_EN_KEY, conf->amtEn)<0)
			goto CREATE_EXIT;

		memset(conf->amtID, 0, sizeof(conf->amtID));
		if(xml_SetItemValue(doc,AMT_ID_KEY, conf->amtID)<0)
			goto CREATE_EXIT;

		memset(conf->amtPwd, 0, sizeof(conf->amtPwd));
		if(xml_SetItemValue(doc,AMT_PWD_KEY, conf->amtPwd)<0)
			goto CREATE_EXIT;

		xml_SaveFile(configFile, doc);
		iRet = true;
CREATE_EXIT:
		xml_FreeDoc(doc);
		return iRet;
	}
	else
	{
		xml_FreeDoc(doc);
		return amt_load(configFile, conf);
	}
}

int amt_get(char const * const configFile, char const * const itemName, char * itemValue, int valueLen)
{
	int iRet = false;
	if(NULL == configFile || NULL == itemName || NULL == itemValue) return iRet;
	{
		xml_doc_info * doc = NULL;
		doc = xml_Loadfile(configFile, XML_CONFIG_ROOT, XML_CONFIG_BASIC);
		if(doc)
		{
			iRet = xml_GetItemValue(doc, itemName, itemValue, valueLen);
			xml_FreeDoc(doc);
		}
	}
	return iRet;
}

int amt_set(char const * const configFile, char const * const itemName, char const * const itemValue)
{
	int iRet = false;
   if(NULL == configFile || NULL == itemName || NULL == itemValue) return iRet;
	{
		xml_doc_info * doc = NULL;
		doc = xml_Loadfile(configFile, XML_CONFIG_ROOT, XML_CONFIG_BASIC);
		if(doc)
		{
			if(xml_SetItemValue(doc, itemName, itemValue)<0)
				return iRet;

			xml_SaveFile(configFile, doc);
			xml_FreeDoc(doc);
			iRet = true;
		}
	}
	return iRet;
}