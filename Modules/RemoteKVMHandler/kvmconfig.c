#include <XMLBase.h>
#include "kvmconfig.h"

#define XML_CONFIG_ROOT "XMLConfigSettings"
#define XML_CONFIG_BASIC "Customize"

#define KVM_MODE_KEY			"KVMMode"
#define KVM_CUSTPWD_KEY			"CustVNCPwd"
#define KVM_CUSTPORT_KEY		"CustVNCPort"

int kvm_load(char const * configFile, susiaccess_kvm_conf_body_t * conf)
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

	memset(conf, 0, sizeof(susiaccess_kvm_conf_body_t));

	if(!xml_GetItemValue(doc, KVM_MODE_KEY, conf->kvmMode, sizeof(conf->kvmMode)))
		memset(conf->kvmMode, 0, sizeof(conf->kvmMode));
	if(strlen(conf->kvmMode)<=0)
	{
		strncpy(conf->kvmMode, "default", strlen("default")+1);
		xml_SetItemValue(doc, KVM_MODE_KEY, conf->kvmMode);
		bModify = true;
	}

	if(!xml_GetItemValue(doc, KVM_CUSTPWD_KEY, conf->custVNCPwd, sizeof(conf->custVNCPwd)))
		memset(conf->custVNCPwd, 0, sizeof(conf->custVNCPwd));
	if(strlen(conf->custVNCPwd)<=0)
	{
		strncpy(conf->custVNCPwd, "na", strlen("na")+1);
		xml_SetItemValue(doc, KVM_CUSTPWD_KEY, conf->custVNCPwd);
		bModify = true;
	}

	if(!xml_GetItemValue(doc, KVM_CUSTPORT_KEY, conf->custVNCPort, sizeof(conf->custVNCPort)))
		memset(conf->custVNCPort, 0, sizeof(conf->custVNCPort));
	if(strlen(conf->custVNCPort)<=0)
	{
		strncpy(conf->custVNCPort, "5900", strlen("5900")+1);
		xml_SetItemValue(doc, KVM_CUSTPORT_KEY, conf->custVNCPort);
		bModify = true;
	}
	xml_FreeDoc(doc);
	if(bModify)
	{
		//xml_SaveFile(configFile, conf);
	}
	iRet = true;
	return iRet;
}

int kvm_save(char const * configFile, susiaccess_kvm_conf_body_t const * const conf)
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

	if(xml_SetItemValue(doc,KVM_MODE_KEY, conf->kvmMode)<0)
		goto SAVE_EXIT;

	if(xml_SetItemValue(doc,KVM_CUSTPWD_KEY, conf->custVNCPwd)<0)
		goto SAVE_EXIT;

	if(xml_SetItemValue(doc,KVM_CUSTPORT_KEY, conf->custVNCPort)<0)
		goto SAVE_EXIT;

	xml_SaveFile(configFile, doc);
	iRet = true;
SAVE_EXIT:
	xml_FreeDoc(doc);
	return iRet;
}

int kvm_create(char const * configFile, susiaccess_kvm_conf_body_t * conf)
{
	int iRet = false;
	xml_doc_info * doc = NULL;

	if(configFile == NULL) 
		return iRet;
	if(conf == NULL)
		return iRet;

	memset(conf, 0, sizeof(susiaccess_kvm_conf_body_t));

	doc = xml_Loadfile(configFile, XML_CONFIG_ROOT, XML_CONFIG_BASIC);
	if(doc == NULL)
	{
		doc = xml_CreateDoc( XML_CONFIG_ROOT, XML_CONFIG_BASIC);

		sprintf_s(conf->kvmMode, sizeof(conf->kvmMode), "%d", DEF_NORMAL_KVM_MODE);
		if(xml_SetItemValue(doc,KVM_MODE_KEY, conf->kvmMode)<0)
			goto CREATE_EXIT;

		memset(conf->custVNCPwd, 0, sizeof(conf->custVNCPwd));
		if(xml_SetItemValue(doc,KVM_CUSTPWD_KEY, conf->custVNCPwd)<0)
			goto CREATE_EXIT;

		sprintf_s(conf->custVNCPort, sizeof(conf->custVNCPort), "%d", 5900);
		if(xml_SetItemValue(doc,KVM_CUSTPORT_KEY, conf->custVNCPort)<0)
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
		return kvm_load(configFile, conf);
	}
}

int kvm_get(char const * const configFile, char const * const itemName, char * itemValue, int valueLen)
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

int kvm_set(char const * const configFile, char const * const itemName, char const * const itemValue)
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