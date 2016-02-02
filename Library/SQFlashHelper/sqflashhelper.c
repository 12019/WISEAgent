#include "platform.h"
#include "sqflashhelper.h"
#define MAX_HDD_COUNT   32


bool GetHDDHealth(hdd_mon_info_t * hddMonInfo);
bool GetAttriNameFromType(smart_attri_type_t attriType, char * attriName);

smart_attri_info_node_t * FindSmartAttriInfoNode(smart_attri_info_list attriInfoList, char * attriName);
smart_attri_info_node_t * FindSmartAttriInfoNodeWithType(smart_attri_info_list attriInfoList, smart_attri_type_t attriType);
smart_attri_info_list CreateSmartAttriInfoList();
int InsertSmartAttriInfoNode(smart_attri_info_list attriInfoList, smart_attri_info_t * attriInfo);
int InsertSmartAttriInfoNodeEx(smart_attri_info_list attriInfoList, smart_attri_info_t * attriInfo);
int UpdateSmartAttriInfoNode(smart_attri_info_list attriInfoList, smart_attri_info_t * attriInfo);
int DeleteSmartAttriInfoNode(smart_attri_info_list attriInfoList, char * attriName);
int DeleteSmartAttriInfoNodeWithID(smart_attri_info_list attriInfoList, smart_attri_type_t attriType);
int DeleteAllSmartAttriInfoNode(smart_attri_info_list attriInfoList);
bool IsSmartAttriInfoListEmpty(smart_attri_info_list attriInfoList);

hdd_mon_info_node_t * FindHDDInfoNode(hdd_mon_info_list hddInfoList, char * hddName);
hdd_mon_info_node_t * FindHDDInfoNodeWithID(hdd_mon_info_list hddInfoList, int hddIndex);
hdd_mon_info_list hdd_CreateHDDInfoList();
int InsertHDDInfoNode(hdd_mon_info_list hddInfoList, hdd_mon_info_t * hddMonInfo);
int InsertHDDInfoNodeEx(hdd_mon_info_list hddInfoList, hdd_mon_info_t * hddMonInfo);
int DeleteHDDInfoNode(hdd_mon_info_list hddInfoList, char * hddName);
int DeleteHDDInfoNodeWithID(hdd_mon_info_list hddInfoList, int hddIndex);
int DeleteAllHDDInfoNode(hdd_mon_info_list hddInfoList);
void hdd_DestroyHDDInfoList(hdd_mon_info_list hddInfoList);
bool IsHddInfoListEmpty(hdd_mon_info_list hddInfoList);
#ifdef WIN32
//---------------------------------SQFlash lib data define--------------------------------------
#define DEF_SQFLASH_LIB_NAME       "SQFlash_Dll.dll"
#define SQFLASH_DEF_ACCESS_CODE    "ASAQ5-3ZTH3-Y37QS-ADYP9-FVHII-CT78N-85VAA"
//typedef BOOL (*PSQFlash_Initialize)(LPCTSTR pAccessCode);
//typedef void (*PSQFlash_UnInitialize)();
//typedef int (*PSQFlash_FindFirstDevice)();
//typedef int (*PSQFlash_FindNextDevice)();
typedef BOOL (*PSQFlash_SelectDevice)(int deveNum);
typedef BOOL (*PSQFlash_GetSmartAttribute)(PATA_SMART_ATTR_TABLE pASAT);
typedef BOOL (*PSQFlash_GetSmartAttributeStd)(BYTE *pBuf, DWORD len);
typedef BOOL (*PSQFlash_GetSmartThresholdsStd)(BYTE *pBuf, DWORD len);
typedef BOOL (*PSQFlash_GetDeviceModelName)(TCHAR *pModelNameBuf, DWORD len);
typedef BOOL (*PSQFlash_CheckAccessCode)(LPCTSTR pAccessCode);
void * hSQFlashDll = NULL;
//PSQFlash_Initialize pSQFlash_Initialize = NULL;
//PSQFlash_UnInitialize pSQFlash_UnInitialize = NULL;
//PSQFlash_FindFirstDevice pSQFlash_FindFirstDevice = NULL;
//PSQFlash_FindNextDevice pSQFlash_FindNextDevice = NULL;
PSQFlash_SelectDevice pSQFlash_SelectDevice = NULL;
PSQFlash_GetSmartAttribute pSQFlash_GetSmartAttribute = NULL;
PSQFlash_GetSmartAttributeStd pSQFlash_GetSmartAttributeStd = NULL;
PSQFlash_GetSmartThresholdsStd pSQFlash_GetSmartThresholdsStd = NULL;
PSQFlash_GetDeviceModelName pSQFlash_GetDeviceModelName = NULL;
PSQFlash_CheckAccessCode pSQFlash_ChechAccessCode = NULL;
//----------------------------------------------------------------------------------------------

bool hdd_IsExistSQFlashLib()
{
	bool bRet = false;
	void * hSQFlash = LoadLibrary(DEF_SQFLASH_LIB_NAME);
	if(hSQFlash != NULL)
	{
		bRet = true;
		FreeLibrary(hSQFlash);
		hSQFlash = NULL;
	}
	return bRet;
}

void GetSQFlashFunction(void *hSQFlashDll)
{
	if(hSQFlashDll != NULL)
	{
		//pSQFlash_Initialize = (PSQFlash_Initialize)GetProcAddress((HMODULE)hSQFlashDll, "SQFlash_Initialize");
		//pSQFlash_UnInitialize = (PSQFlash_UnInitialize)GetProcAddress((HMODULE)hSQFlashDll, "SQFlash_UnInitialize");
		//pSQFlash_FindFirstDevice = (PSQFlash_FindFirstDevice)GetProcAddress((HMODULE)hSQFlashDll, "SQFlash_FindFirstDevice");
		//pSQFlash_FindNextDevice = (PSQFlash_FindNextDevice)GetProcAddress((HMODULE)hSQFlashDll, "SQFlash_FindNextDevice");
		pSQFlash_SelectDevice          = (PSQFlash_SelectDevice)         GetProcAddress((HMODULE)hSQFlashDll, "SQFlash_SelectDevice");
		pSQFlash_GetSmartAttribute     = (PSQFlash_GetSmartAttribute)    GetProcAddress((HMODULE)hSQFlashDll, "SQFlash_GetSmartAttribute");
		pSQFlash_GetSmartAttributeStd  = (PSQFlash_GetSmartAttributeStd) GetProcAddress((HMODULE)hSQFlashDll, "SQFlash_GetSmartAttributeStd");
		pSQFlash_GetSmartThresholdsStd = (PSQFlash_GetSmartThresholdsStd)GetProcAddress((HMODULE)hSQFlashDll, "SQFlash_GetSmartThresholdsStd");
		pSQFlash_GetDeviceModelName    = (PSQFlash_GetDeviceModelName)   GetProcAddress((HMODULE)hSQFlashDll, "SQFlash_GetDeviceModelName");
		pSQFlash_ChechAccessCode       = (PSQFlash_CheckAccessCode)      GetProcAddress((HMODULE)hSQFlashDll,"SQFlash_CheckAccessCode");
	}
}

bool hdd_StartupSQFlashLib()
{
	bool bRet = false;
	hSQFlashDll = LoadLibrary(DEF_SQFLASH_LIB_NAME);
	if(hSQFlashDll != NULL)
	{
		GetSQFlashFunction(hSQFlashDll);
		bRet = true;
	}
	return bRet;
}

bool hdd_CleanupSQFlashLib()
{
	bool bRet = true;

	if(hSQFlashDll != NULL)
	{
		FreeLibrary(hSQFlashDll);
		hSQFlashDll = NULL;
		//pSQFlash_Initialize = NULL;
		//pSQFlash_UnInitialize = NULL;
		//pSQFlash_FindFirstDevice = NULL;
		//pSQFlash_FindNextDevice = NULL;
		pSQFlash_SelectDevice = NULL;
		pSQFlash_GetSmartAttribute = NULL;
		pSQFlash_GetSmartAttributeStd = NULL;
		pSQFlash_GetSmartThresholdsStd = NULL;
		pSQFlash_GetDeviceModelName = NULL;
		pSQFlash_ChechAccessCode = NULL;
	}
	return bRet;
}

bool SelectDevice(int deveNum)
{
	bool bRet = false;

	if(pSQFlash_SelectDevice)
	{
		bRet = pSQFlash_SelectDevice(deveNum) == TRUE;
	}
	return bRet;
}

bool GetDeviceModelName(char * pModelNameBuf, unsigned int len)
{
	bool bRet = false;

	if(pSQFlash_GetDeviceModelName)
	{
		bRet = pSQFlash_GetDeviceModelName(pModelNameBuf, len) == TRUE;
	}
	return bRet;
}

bool CheckAccessCode(char * pAccessCode)
{
	bool bRet = false;

	if(pSQFlash_ChechAccessCode)
	{
		bRet = pSQFlash_ChechAccessCode(pAccessCode) == TRUE;
	}
	return bRet;
}

bool GetSmartAttribute(PATA_SMART_ATTR_TABLE pASAT)
{
	bool bRet = false;

	if(pSQFlash_GetSmartAttribute)
	{
		bRet = pSQFlash_GetSmartAttribute(pASAT) == TRUE;
	}
	return bRet;
}

bool GetSmartAttributeStd(char * pBuf, unsigned int len)
{
	bool bRet = false;

	if(pSQFlash_GetSmartAttributeStd)
	{
		bRet = pSQFlash_GetSmartAttributeStd(pBuf, len) == TRUE;
	}
	return bRet;
}

bool GetHDDCount(int * pHDDCount)
{
	bool bRet = false;
	int hddCount = 0;
	if(!pSQFlash_SelectDevice || !pSQFlash_GetDeviceModelName) goto done;
	{
		int i = 0;
		char tmpDiskNameWcs[128] = {0};
		while(i < MAX_HDD_COUNT)
		{
			if(pSQFlash_SelectDevice(i))
			{
				memset(tmpDiskNameWcs, 0, sizeof(tmpDiskNameWcs));
				if(pSQFlash_GetDeviceModelName(tmpDiskNameWcs, sizeof(tmpDiskNameWcs)) == true && strlen(tmpDiskNameWcs) > 0)
				{
					hddCount++;
				}
			}
			i++;
		}
	}
	bRet = true;
done:
	*pHDDCount = hddCount;
	return bRet;
}

bool HDDSetAttriInfo(hdd_mon_info_t * hddMonInfo, char * attriBuf, int attriBufLen)
{
	bool bRet = false;
	if(hddMonInfo == NULL || attriBuf == NULL || attriBufLen<=0) return bRet;
	if(hddMonInfo->hdd_type == SQFlash)
	{
		PATA_SMART_ATTR_TABLE ataSmartAttrTable = (PATA_SMART_ATTR_TABLE)attriBuf;
		hddMonInfo->max_program            = ataSmartAttrTable->dwMaxProgram;
		hddMonInfo->average_program        = ataSmartAttrTable->dwAverageProgram;
		hddMonInfo->endurance_check        = ataSmartAttrTable->dwEnduranceCheck;
		hddMonInfo->power_on_time          = ataSmartAttrTable->dwPowerOnTime;
		hddMonInfo->ecc_count              = ataSmartAttrTable->dwEccCount;
		hddMonInfo->max_reserved_block     = ataSmartAttrTable->dwMaxReservedBlock;
		hddMonInfo->current_reserved_block = ataSmartAttrTable->dwCurrentReservedBlock;
		hddMonInfo->good_block_rate        = ataSmartAttrTable->dwGoodBlockRate;
	}
	else if(hddMonInfo->hdd_type == StdDisk)
	{
		int offset = 0;
		smart_attri_info_t smartAttriInfo;
		smart_attri_info_node_t * findNode = NULL;
		for (offset = 2; offset < attriBufLen; offset += 12)
		{
			if (offset + 12 >= attriBufLen)
			{
				break;
			}
			else
			{
				memset(&smartAttriInfo, 0, sizeof(smart_attri_info_t));
				smartAttriInfo.attriType = (unsigned char)attriBuf[offset];
				// Attribute type 0x00, 0xfe, 0xff are invalid
				if(smartAttriInfo.attriType > SmartAttriTypeUnknown && smartAttriInfo.attriType <= FreeFallProtection)
				{
					smartAttriInfo.attriFlags = ((unsigned char)attriBuf[offset+1]<<8)+(unsigned char)attriBuf[offset+2];
					smartAttriInfo.attriValue = (unsigned char)attriBuf[offset+3];
					smartAttriInfo.attriWorst = (unsigned char)attriBuf[offset+4];
					memcpy(smartAttriInfo.attriVendorData, &attriBuf[offset+5], 6);
					GetAttriNameFromType(smartAttriInfo.attriType, smartAttriInfo.attriName);
					if(smartAttriInfo.attriType == Temperature)
					{
						hddMonInfo->hdd_temp = ((unsigned char)smartAttriInfo.attriVendorData[1]*256) + (unsigned char)smartAttriInfo.attriVendorData[0];
					}
					else if(smartAttriInfo.attriType == PowerOnHoursPOH)
					{
						hddMonInfo->power_on_time = ((unsigned char)smartAttriInfo.attriVendorData[3]*256*3) + ((unsigned char)smartAttriInfo.attriVendorData[2]*256*2) +
							((unsigned char)smartAttriInfo.attriVendorData[1]*256) + (unsigned char)smartAttriInfo.attriVendorData[0];
					}
					else if(smartAttriInfo.attriType == ReportedUncorrectableErrors)
					{
						hddMonInfo->ecc_count = ((unsigned char)smartAttriInfo.attriVendorData[1]*256) + (unsigned char)smartAttriInfo.attriVendorData[0];
					}

					if(hddMonInfo->smartAttriInfoList == NULL)
					{
						hddMonInfo->smartAttriInfoList = CreateSmartAttriInfoList();
					}
					findNode = FindSmartAttriInfoNodeWithType(hddMonInfo->smartAttriInfoList, smartAttriInfo.attriType);
					if(findNode)
					{
						UpdateSmartAttriInfoNode(hddMonInfo->smartAttriInfoList, &smartAttriInfo);
					}
					else
					{
						InsertSmartAttriInfoNode(hddMonInfo->smartAttriInfoList, &smartAttriInfo);
					}
				}

				if(smartAttriInfo.attriType == FreeFallProtection) break;
			}
		}
	}
	bRet = true;
	return bRet;
}

bool RefreshHddInfoList(hdd_mon_info_list hddMonInfoList)
{
	bool bRet = false;
	if(hddMonInfoList == NULL) return bRet;
	{
		int i = 0;
		char tmpDiskNameWcs[128] = {0};
		hdd_mon_info_t hddMonInfo;
		DeleteAllHDDInfoNode(hddMonInfoList);
		while(i < MAX_HDD_COUNT)
		{
			if(SelectDevice(i))
			{
				memset(tmpDiskNameWcs, 0, sizeof(tmpDiskNameWcs));
				if(GetDeviceModelName(tmpDiskNameWcs, sizeof(tmpDiskNameWcs)) == true && strlen(tmpDiskNameWcs) > 0)
				{
					memset(&hddMonInfo, 0, sizeof(hdd_mon_info_t));
					hddMonInfo.hdd_index = i;
					hddMonInfo.hdd_type = HddTypeUnknown;
					hddMonInfo.smartAttriInfoList = NULL;
					InsertHDDInfoNode(hddMonInfoList, &hddMonInfo);
				}
			}
			i++;
		}
	}
	return bRet;
}

bool hdd_GetHDDInfo(hdd_info_t * pHDDInfo)
{
	bool bRet = false;
	char tmpDiskNameWcs[128] = {0};
	char tmpDiskNameBs[128] = {0};
	int curHddCnt = 0;
	int curHddIndex = 0;
	hdd_mon_info_node_t * curHddMonInfoNode = NULL;
	hdd_mon_info_t * curHddMonInfo = NULL;
	ATA_SMART_ATTR_TABLE ataSmartAttrTable = {0};
	if(!pHDDInfo) return bRet;
	GetHDDCount(&curHddCnt);
	if ( (curHddCnt == 0) || (curHddCnt != pHDDInfo->hddCount) )
	{
		if(pHDDInfo->hddMonInfoList == NULL)
		{
			pHDDInfo->hddMonInfoList = hdd_CreateHDDInfoList();
		}
		RefreshHddInfoList(pHDDInfo->hddMonInfoList);
		pHDDInfo->hddCount = curHddCnt;
	}

	curHddMonInfoNode = pHDDInfo->hddMonInfoList->next;
	while(curHddMonInfoNode)
	{
		curHddMonInfo = (hdd_mon_info_t *)&curHddMonInfoNode->hddMonInfo;
		curHddIndex = curHddMonInfo->hdd_index;
		if(SelectDevice(curHddIndex))
		{
			memset(tmpDiskNameWcs, 0, sizeof(tmpDiskNameWcs));
			if(GetDeviceModelName(tmpDiskNameWcs, sizeof(tmpDiskNameWcs)) == true || strlen(tmpDiskNameWcs) > 0)
			{
				wcstombs(tmpDiskNameBs, (wchar_t *)tmpDiskNameWcs, sizeof(tmpDiskNameBs));
				sprintf(curHddMonInfo->hdd_name, "%s", tmpDiskNameBs);
			}
			TrimStr(curHddMonInfo->hdd_name);
			CheckAccessCode("");
			curHddMonInfo->max_program             = INVALID_DEVMON_VALUE;
			curHddMonInfo->average_program         = INVALID_DEVMON_VALUE;
			curHddMonInfo->endurance_check         = INVALID_DEVMON_VALUE;
			curHddMonInfo->power_on_time           = INVALID_DEVMON_VALUE;
			curHddMonInfo->ecc_count               = INVALID_DEVMON_VALUE;
			curHddMonInfo->max_reserved_block      = INVALID_DEVMON_VALUE;
			curHddMonInfo->current_reserved_block  = INVALID_DEVMON_VALUE;
			curHddMonInfo->good_block_rate         = INVALID_DEVMON_VALUE;
			curHddMonInfo->hdd_temp = 0;
			if(strstr(curHddMonInfo->hdd_name,"SQF"))
			{
				curHddMonInfo->hdd_type = SQFlash;
				if (GetSmartAttribute(&ataSmartAttrTable))
				{        
					HDDSetAttriInfo(curHddMonInfo, (char*)&ataSmartAttrTable, sizeof(ATA_SMART_ATTR_TABLE));
				}
			}
			else
			{
				char smartAttriBuf[4096] = {0};
				curHddMonInfo->hdd_type = StdDisk;
				if(GetSmartAttributeStd(smartAttriBuf, sizeof(smartAttriBuf)))
				{
					HDDSetAttriInfo(curHddMonInfo, smartAttriBuf, sizeof(smartAttriBuf));
				}
			}
			GetHDDHealth(curHddMonInfo);
		}
		curHddMonInfoNode = curHddMonInfoNode->next;
	}
	bRet = true;
	return bRet;
}

#else
#include "common.h"

#define  SMART_TOOL_NAME             "smartctl"
#define  SDA_DEV_TYPE                "sda"
#define  SDB_DEV_TYPE                "sdb"
#define  HDA_DEV_TYPE                "hda"
static char SmartctlPath[MAX_PATH] = {0};
bool hdd_IsExistSQFlashLib()
{
	bool bRet = false;
	if(strlen(SmartctlPath) == 0)
	{
		char moudlePath[MAX_PATH] = {0};
		memset(moudlePath, 0 , sizeof(moudlePath));
		app_os_get_module_path(moudlePath);
		sprintf_s(SmartctlPath, sizeof(SmartctlPath), "%s/%s", moudlePath, SMART_TOOL_NAME);
	}
	if(access(SmartctlPath, F_OK) == F_OK)
	{
		bRet = true;
	}
	return bRet;
}

bool hdd_StartupSQFlashLib()
{
	bool bRet = false;
   bRet = true;
	return bRet;
}

bool hdd_CleanupSQFlashLib()
{
	bool bRet = false;
   bRet = true;
	return bRet;
}

static int HddIndex = 0;

int GetHddMonInfoWithSmartctl(hdd_mon_info_list hddMonInfoList, char * devType)
{
	int hddCnt = 0;
	if(hddMonInfoList == NULL || devType == NULL) return hddCnt;
	{
		char cmdStr[256] = {0};
		FILE * pF = NULL;
		sprintf_s(cmdStr, sizeof(cmdStr), "%s -a /dev/%s", SmartctlPath, devType);
		pF = popen(cmdStr, "r");
		if(pF)
		{
			char tmp[1024] = {0};
			smart_attri_info_t curSmartAttriInfo;
			hdd_mon_info_t * pCurHddMonInfo = NULL;

			#pragma region while fgets
			while(fgets(tmp, sizeof(tmp), pF)!=NULL)
			{
				
				#pragma region Device Mode
				if(strstr(tmp, "Device Model:"))
				{
					if (pCurHddMonInfo)
					{
						hddCnt++;
						GetHDDHealth(pCurHddMonInfo);
						InsertHDDInfoNodeEx(hddMonInfoList, pCurHddMonInfo);
						DestroySmartAttriInfoList(pCurHddMonInfo->smartAttriInfoList); // add by tang.tao @2015-8-13 11:19:25
						free(pCurHddMonInfo);
						pCurHddMonInfo = NULL;
					}
					pCurHddMonInfo = (hdd_mon_info_t *)malloc(sizeof(hdd_mon_info_t));
					memset((char*)pCurHddMonInfo, 0, sizeof(hdd_mon_info_t));
					{
						int i = 0;
						char * infoUnit[4] = {NULL};
						infoUnit[i] = strtok(tmp, ":");
						i++;
						while((infoUnit[i] = strtok(NULL, ":\n")) !=  NULL)
						{
							i++;
						}
						if(i>=2)
						{
							TrimStr(infoUnit[1]);
							pCurHddMonInfo->hdd_index = HddIndex;
							HddIndex++;
							//sprintf(pCurHddMonInfo->hdd_name, "Disk%d-%s", pCurHddMonInfo->hdd_index, infoUnit[1]);
							sprintf(pCurHddMonInfo->hdd_name, "%s", infoUnit[1]);
						}

					}
					if(strstr(pCurHddMonInfo->hdd_name, "SQF"))
					{
						pCurHddMonInfo->hdd_type = SQFlash;
						if(pCurHddMonInfo->hdd_name[4] == 'S' || pCurHddMonInfo->hdd_name[4] == 's')
						{
							if(pCurHddMonInfo->hdd_name[7] == 'S' || pCurHddMonInfo->hdd_name[7] == 's')
							{
								pCurHddMonInfo->max_program = 10000;
							}
							else if(pCurHddMonInfo->hdd_name[7] == 'U' || pCurHddMonInfo->hdd_name[7] == 'u')
							{
								pCurHddMonInfo->max_program = 30000;
							}
							else if(pCurHddMonInfo->hdd_name[7] == 'M' || pCurHddMonInfo->hdd_name[7] == 'm')
							{
								pCurHddMonInfo->max_program = 30000;
							}
							else
							{
								pCurHddMonInfo->max_program = 10000;
							}
						}
					}
					else
					{
						pCurHddMonInfo->hdd_type = StdDisk;
					}
				}
#pragma endregion Device Mode
				else if(strstr(tmp, "ATTR"))
				{
					int i = 0;
					char * attrUnit[7] = {NULL};
					memset((char *)&curSmartAttriInfo, 0, sizeof(smart_attri_info_t));
					attrUnit[i] = strtok(tmp, ",");
					i++;
					while((attrUnit[i] = strtok(NULL, ",\n"))!=NULL)
					{
						i++;
					}
					if(i>=5)
					{
						unsigned int tmpAttriType = 0;
						TrimStr(attrUnit[1]);
						tmpAttriType =  atoi(attrUnit[1]);
						if(pCurHddMonInfo->hdd_type == SQFlash)   //SQFlash 
						{
							switch (tmpAttriType)
							{
							case ReadErrorRate: 
								{
									int tmpData = 0;
									TrimStr(attrUnit[4]);
									tmpData = atoi(attrUnit[4]);
									pCurHddMonInfo->ecc_count = (tmpData>>16);
									break;
								}
							case PowerOnHoursPOH: 
								{
									int tmpData = 0;
									TrimStr(attrUnit[4]);
									tmpData = atoi(attrUnit[4]);
									pCurHddMonInfo->power_on_time = tmpData; 
									break;
								}
							case SQAverageProgram: 
								{
									int tmpData = 0;
									TrimStr(attrUnit[4]);
									tmpData = atoi(attrUnit[4]);
									pCurHddMonInfo->average_program = (tmpData>>16);
									break;
								}
							case SQEnduranceRemainLife: 
								{
									int tmpData = 0;
									TrimStr(attrUnit[4]);
									tmpData = atoi(attrUnit[4]);
									pCurHddMonInfo->endurance_check = tmpData;
									break;
								}
							case SQPowerOnTime:
								{
									int tmpData = 0;
									TrimStr(attrUnit[4]);
									tmpData = atoi(attrUnit[4]);
									pCurHddMonInfo->power_on_time = tmpData/(60*60);
									break;
								}
							case SQECCLog:
								{
									int tmpData = 0;
									TrimStr(attrUnit[4]);
									tmpData = atoi(attrUnit[4]);
									pCurHddMonInfo->ecc_count = tmpData;
									break;
								}
							case SQGoodBlockRate:
								{
									int tmpData = 0;
									TrimStr(attrUnit[4]);
									tmpData = atoi(attrUnit[4]);
									pCurHddMonInfo->good_block_rate = tmpData;
									break;
								}
							}
						}
						else  //General disk
						{
							if(tmpAttriType > SmartAttriTypeUnknown && tmpAttriType <= FreeFallProtection)
							{
								smart_attri_info_node_t * findNode = NULL;
								char tmpAttriName[64] = {0};
								GetAttriNameFromType(tmpAttriType, tmpAttriName);
								if(strlen(tmpAttriName) > 0)
								{
									strcpy(curSmartAttriInfo.attriName, tmpAttriName);
								}
								curSmartAttriInfo.attriType = tmpAttriType;
								TrimStr(attrUnit[2]);
								curSmartAttriInfo.attriValue = atoi(attrUnit[2]);
								TrimStr(attrUnit[3]);
								curSmartAttriInfo.attriThresh = atoi(attrUnit[3]);
								TrimStr(attrUnit[4]);
								{//add VendorData process
									unsigned int tmpVendorData = atoi(attrUnit[4]);
									memcpy(curSmartAttriInfo.attriVendorData, (char *)&tmpVendorData, sizeof(unsigned int));
								}
								curSmartAttriInfo.attriFlags = 0;
								curSmartAttriInfo.attriWorst = 0;
								if(curSmartAttriInfo.attriType == Temperature)
								{
									pCurHddMonInfo->hdd_temp = ((unsigned char)curSmartAttriInfo.attriVendorData[1]*256) + (unsigned char)curSmartAttriInfo.attriVendorData[0];
								}
								else if(curSmartAttriInfo.attriType == PowerOnHoursPOH)
								{
									pCurHddMonInfo->power_on_time = ((unsigned char)curSmartAttriInfo.attriVendorData[3]*(256*256*256)) + ((unsigned char)curSmartAttriInfo.attriVendorData[2]*(256*256)) +
										((unsigned char)curSmartAttriInfo.attriVendorData[1]*256) + (unsigned char)curSmartAttriInfo.attriVendorData[0];
								}
								else if(curSmartAttriInfo.attriType == ReportedUncorrectableErrors)
								{
									pCurHddMonInfo->ecc_count = ((unsigned char)curSmartAttriInfo.attriVendorData[1]*256) + (unsigned char)curSmartAttriInfo.attriVendorData[0];
								}

								if(pCurHddMonInfo->smartAttriInfoList == NULL)
								{
									pCurHddMonInfo->smartAttriInfoList = CreateSmartAttriInfoList();
								}
								findNode = FindSmartAttriInfoNodeWithType(pCurHddMonInfo->smartAttriInfoList, curSmartAttriInfo.attriType);
								if(findNode)
								{
									UpdateSmartAttriInfoNode(pCurHddMonInfo->smartAttriInfoList, &curSmartAttriInfo);
								}
								else
								{
									InsertSmartAttriInfoNodeEx(pCurHddMonInfo->smartAttriInfoList, &curSmartAttriInfo);
								}
							}
						}
					}
				}
				memset(tmp, 0, sizeof(tmp));
			}
			#pragma endregion while fgets
			if(pCurHddMonInfo)
			{
				hddCnt++;
				GetHDDHealth(pCurHddMonInfo);
				InsertHDDInfoNodeEx(hddMonInfoList, pCurHddMonInfo);
				DestroySmartAttriInfoList(pCurHddMonInfo->smartAttriInfoList); // add by tang.tao @2015-8-13 11:19:25
				free(pCurHddMonInfo);
				pCurHddMonInfo = NULL;
			}
		}
		pclose(pF);
	}
	return hddCnt;
}

bool hdd_GetHDDInfo(hdd_info_t * pHDDInfo)
{
	bool bRet = false;
	if(pHDDInfo == NULL) return bRet;

	{
		int tmpHddCnt = 0;
		pHDDInfo->hddCount = 0;
		if(pHDDInfo->hddMonInfoList == NULL)
		{
			pHDDInfo->hddMonInfoList = hdd_CreateHDDInfoList();
		}
		else
		{
			DeleteAllHDDInfoNode(pHDDInfo->hddMonInfoList);
		}
		HddIndex = 0;
		tmpHddCnt = GetHddMonInfoWithSmartctl(pHDDInfo->hddMonInfoList, SDA_DEV_TYPE);
		pHDDInfo->hddCount += tmpHddCnt;
		tmpHddCnt = GetHddMonInfoWithSmartctl(pHDDInfo->hddMonInfoList, SDB_DEV_TYPE);
		pHDDInfo->hddCount += tmpHddCnt;
		tmpHddCnt = GetHddMonInfoWithSmartctl(pHDDInfo->hddMonInfoList, HDA_DEV_TYPE);
		pHDDInfo->hddCount += tmpHddCnt;
		bRet = true;
	}
	return bRet ;
}
#endif

//---------------------device monitor hdd smart attribute list function ------------------------
smart_attri_info_node_t * FindSmartAttriInfoNode(smart_attri_info_list attriInfoList, char * attriName)
{
	smart_attri_info_node_t * findNode = NULL, * head = NULL;
	if(attriInfoList == NULL || attriName == NULL) return findNode;
	head = attriInfoList;
	findNode = head->next;
	while(findNode)
	{
		if(!strcmp(findNode->attriInfo.attriName, attriName)) break;
		findNode = findNode->next;
	}

	return findNode;
}

smart_attri_info_node_t * FindSmartAttriInfoNodeWithType(smart_attri_info_list attriInfoList, smart_attri_type_t attriType)
{
	smart_attri_info_node_t * findNode = NULL, * head = NULL;
	if(attriInfoList == NULL || attriType == SmartAttriTypeUnknown) return findNode;
	head = attriInfoList;
	findNode = head->next;
	while(findNode)
	{
		if(findNode->attriInfo.attriType == attriType) break;
		findNode = findNode->next;
	}

	return findNode;
}

smart_attri_info_list CreateSmartAttriInfoList()
{
	smart_attri_info_node_t * head = NULL;
	head = (smart_attri_info_node_t *)malloc(sizeof(smart_attri_info_node_t));
	if(head)
	{
		head->next = NULL;
		memset(head->attriInfo.attriName, 0, sizeof(head->attriInfo.attriName));
		head->attriInfo.attriFlags = INVALID_DEVMON_VALUE;
		head->attriInfo.attriType = SmartAttriTypeUnknown;
		head->attriInfo.attriValue = INVALID_DEVMON_VALUE;
		head->attriInfo.attriWorst = INVALID_DEVMON_VALUE;
		memset(head->attriInfo.attriVendorData, 0, sizeof(head->attriInfo.attriVendorData));
	}
	return head;
}

int InsertSmartAttriInfoNode(smart_attri_info_list attriInfoList, smart_attri_info_t * attriInfo)
{
	int iRet = -1;
	smart_attri_info_node_t * newNode = NULL, * findNode = NULL, * head = NULL;
	if(attriInfoList == NULL || attriInfo == NULL) return iRet;
	head = attriInfoList;
	findNode = FindSmartAttriInfoNode(head, attriInfo->attriName);
	if(findNode == NULL)
	{
		newNode = (smart_attri_info_node_t *)malloc(sizeof(smart_attri_info_node_t));
		memset(newNode, 0, sizeof(smart_attri_info_node_t));
		strcpy(newNode->attriInfo.attriName, attriInfo->attriName);
		newNode->attriInfo.attriFlags = attriInfo->attriFlags;
		newNode->attriInfo.attriType = attriInfo->attriType;
		newNode->attriInfo.attriThresh = attriInfo->attriThresh;
		newNode->attriInfo.attriValue = attriInfo->attriValue;
		newNode->attriInfo.attriWorst = attriInfo->attriWorst;
		memcpy(newNode->attriInfo.attriVendorData, attriInfo->attriVendorData, sizeof(attriInfo->attriVendorData));
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

int InsertSmartAttriInfoNodeEx(smart_attri_info_list attriInfoList, smart_attri_info_t * attriInfo)
{
	int iRet = -1;
	smart_attri_info_node_t * newNode = NULL, * findNode = NULL, * head = NULL;
	if(attriInfoList == NULL || attriInfo == NULL) return iRet;
	head = attriInfoList;
	newNode = (smart_attri_info_node_t *)malloc(sizeof(smart_attri_info_node_t));
	memset(newNode, 0, sizeof(smart_attri_info_node_t));
	strcpy(newNode->attriInfo.attriName, attriInfo->attriName);
	newNode->attriInfo.attriFlags = attriInfo->attriFlags;
	newNode->attriInfo.attriType = attriInfo->attriType;
	newNode->attriInfo.attriThresh = attriInfo->attriThresh;
	newNode->attriInfo.attriValue = attriInfo->attriValue;
	newNode->attriInfo.attriWorst = attriInfo->attriWorst;
	memcpy(newNode->attriInfo.attriVendorData, attriInfo->attriVendorData, sizeof(attriInfo->attriVendorData));
	newNode->next = head->next;
	head->next = newNode;
	iRet = 0;
	return iRet;
}

int UpdateSmartAttriInfoNode(smart_attri_info_list attriInfoList, smart_attri_info_t * attriInfo)
{
	int iRet = -1;
	smart_attri_info_node_t * findNode = NULL, * head = NULL;
	if(attriInfoList == NULL || attriInfo == NULL) return iRet;
	head = attriInfoList;
	findNode = FindSmartAttriInfoNodeWithType(head, attriInfo->attriType);
	if(findNode)
	{
		memset(findNode->attriInfo.attriName, 0, sizeof(findNode->attriInfo.attriName));
		strcpy(findNode->attriInfo.attriName, attriInfo->attriName);
		findNode->attriInfo.attriFlags = attriInfo->attriFlags;
		findNode->attriInfo.attriThresh = attriInfo->attriThresh;
		findNode->attriInfo.attriValue = attriInfo->attriValue;
		findNode->attriInfo.attriWorst = attriInfo->attriWorst;
		memset(findNode->attriInfo.attriVendorData, 0, sizeof(findNode->attriInfo.attriVendorData));
		memcpy(findNode->attriInfo.attriVendorData, attriInfo->attriVendorData, sizeof(attriInfo->attriVendorData));
		iRet = 0;
	}
	else
	{
		iRet = 1;
	}
	return iRet;
}

int DeleteSmartAttriInfoNode(smart_attri_info_list attriInfoList, char * attriName)
{
	int iRet = -1;
	smart_attri_info_node_t * delNode = NULL, * head = NULL;
	smart_attri_info_node_t * p = NULL;
	if(attriInfoList == NULL || attriName == NULL) return iRet;
	head = attriInfoList;
	p = head;
	delNode = head->next;
	while(delNode)
	{
		if(!strcmp(delNode->attriInfo.attriName, attriName))
		{
			p->next = delNode->next;
			free(delNode);
			delNode = NULL;
			iRet = 0;
			break;
		}
		p = delNode;
		delNode = delNode->next;
	}
	if(iRet == -1) iRet = 1;
	return iRet;
}

int DeleteSmartAttriInfoNodeWithID(smart_attri_info_list attriInfoList, smart_attri_type_t attriType)
{
	int iRet = -1;
	smart_attri_info_node_t * delNode = NULL, * head = NULL;
	smart_attri_info_node_t * p = NULL;
	if(attriInfoList == NULL || attriType == SmartAttriTypeUnknown) return iRet;
	head = attriInfoList;
	p = head;
	delNode = head->next;
	while(delNode)
	{
		if(delNode->attriInfo.attriType == attriType)
		{
			p->next = delNode->next;
			free(delNode);
			delNode = NULL;
			iRet = 0;
			break;
		}
		p = delNode;
		delNode = delNode->next;
	}
	if(iRet == -1) iRet = 1;
	return iRet;
}

int DeleteAllSmartAttriInfoNode(smart_attri_info_list attriInfoList)
{
	int iRet = -1;
	smart_attri_info_node_t * delNode = NULL, *head = NULL;
	if(attriInfoList == NULL) return iRet;
	head = attriInfoList;
	delNode = head->next;
	while(delNode)
	{
		head->next = delNode->next;
		free(delNode);
		delNode = head->next;
	}
	iRet = 0;
	return iRet;
}

void DestroySmartAttriInfoList(smart_attri_info_list attriInfoList)
{
	if(NULL == attriInfoList) return;
	DeleteAllSmartAttriInfoNode(attriInfoList);
	free(attriInfoList); 
	attriInfoList = NULL;
}

bool IsSmartAttriInfoListEmpty(smart_attri_info_list attriInfoList)
{
	bool bRet = true;
	smart_attri_info_node_t * curNode = NULL, *head = NULL;
	if(attriInfoList == NULL) return bRet;
	head = attriInfoList;
	curNode = head->next;
	if(curNode != NULL) bRet = false;
	return bRet;
}
//----------------------------------------------------------------------------------------------

//---------------------device monitor hdd info list fuction ------------------------------------
hdd_mon_info_node_t * FindHDDInfoNode(hdd_mon_info_list hddInfoList, char * hddName)
{
	hdd_mon_info_node_t * findNode = NULL, * head = NULL;
	if(hddInfoList == NULL || hddName == NULL) return findNode;
	head = hddInfoList;
	findNode = head->next;
	while(findNode)
	{
		if(!strcmp(findNode->hddMonInfo.hdd_name, hddName)) break;
		findNode = findNode->next;
	}

	return findNode;
}

hdd_mon_info_node_t * FindHDDInfoNodeWithID(hdd_mon_info_list hddInfoList, int hddIndex)
{
	hdd_mon_info_node_t * findNode = NULL, * head = NULL;
	if(hddInfoList == NULL) return findNode;
	head = hddInfoList;
	findNode = head->next;
	while(findNode)
	{
		if(findNode->hddMonInfo.hdd_index == hddIndex) break;
		findNode = findNode->next;
	}
	return findNode;
}

hdd_mon_info_list hdd_CreateHDDInfoList()
{
	hdd_mon_info_node_t * head = NULL;
	head = (hdd_mon_info_node_t *)malloc(sizeof(hdd_mon_info_node_t));
	memset(head, 0, sizeof(hdd_mon_info_node_t));
	if(head)
	{
		head->next = NULL;
		memset(head->hddMonInfo.hdd_name, 0, sizeof(head->hddMonInfo.hdd_name));
		head->hddMonInfo.hdd_index              = INVALID_DEVMON_VALUE;
		head->hddMonInfo.hdd_type               = SmartAttriTypeUnknown;
		head->hddMonInfo.average_program        = INVALID_DEVMON_VALUE;
		head->hddMonInfo.current_reserved_block = INVALID_DEVMON_VALUE;
		head->hddMonInfo.ecc_count              = INVALID_DEVMON_VALUE;
		head->hddMonInfo.endurance_check        = INVALID_DEVMON_VALUE;
		head->hddMonInfo.good_block_rate        = INVALID_DEVMON_VALUE;
		head->hddMonInfo.max_program            = INVALID_DEVMON_VALUE;
		head->hddMonInfo.max_reserved_block     = INVALID_DEVMON_VALUE;
		head->hddMonInfo.power_on_time          = INVALID_DEVMON_VALUE;
		head->hddMonInfo.hdd_health_percent     = INVALID_DEVMON_VALUE;
		head->hddMonInfo.hdd_temp               = INVALID_DEVMON_VALUE;
		head->hddMonInfo.smartAttriInfoList     = NULL;	
	}
	return head;
}

int InsertHDDInfoNode(hdd_mon_info_list hddInfoList, hdd_mon_info_t * hddMonInfo)
{
	int iRet = -1;
	hdd_mon_info_node_t * newNode = NULL, * findNode = NULL, * head = NULL;
	if(hddInfoList == NULL || hddMonInfo == NULL) return iRet;
	head = hddInfoList;
	findNode = FindHDDInfoNodeWithID(head, hddMonInfo->hdd_index);
	if(findNode == NULL)
	{
		newNode = (hdd_mon_info_node_t *)malloc(sizeof(hdd_mon_info_node_t));
		memset(newNode, 0, sizeof(hdd_mon_info_node_t));
		strcpy(newNode->hddMonInfo.hdd_name, hddMonInfo->hdd_name);
		newNode->hddMonInfo.hdd_index = hddMonInfo->hdd_index;
		newNode->hddMonInfo.hdd_type = hddMonInfo->hdd_type;
		newNode->hddMonInfo.average_program = hddMonInfo->average_program;
		newNode->hddMonInfo.current_reserved_block = hddMonInfo->current_reserved_block;
		newNode->hddMonInfo.ecc_count = hddMonInfo->ecc_count;
		newNode->hddMonInfo.endurance_check = hddMonInfo->endurance_check;
		newNode->hddMonInfo.good_block_rate = hddMonInfo->good_block_rate;
		newNode->hddMonInfo.hdd_health_percent = hddMonInfo->hdd_health_percent;
		newNode->hddMonInfo.hdd_temp = hddMonInfo->hdd_temp;
		newNode->hddMonInfo.max_program = hddMonInfo->max_program;
		newNode->hddMonInfo.max_reserved_block = hddMonInfo->max_reserved_block;
		newNode->hddMonInfo.power_on_time = hddMonInfo->power_on_time;
		if(hddMonInfo->smartAttriInfoList)
		{
			smart_attri_info_node_t * curAtrriInfoNode = NULL;
			newNode->hddMonInfo.smartAttriInfoList = CreateSmartAttriInfoList();
			curAtrriInfoNode = hddMonInfo->smartAttriInfoList->next;
			while(curAtrriInfoNode)
			{
				InsertSmartAttriInfoNode(newNode->hddMonInfo.smartAttriInfoList, &curAtrriInfoNode->attriInfo);
				curAtrriInfoNode = curAtrriInfoNode->next;
			}
		}
		else
		{
			newNode->hddMonInfo.smartAttriInfoList = NULL;
		}
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

int InsertHDDInfoNodeEx(hdd_mon_info_list hddInfoList, hdd_mon_info_t * hddMonInfo)
{
	int iRet = -1;
	hdd_mon_info_node_t * newNode = NULL, * findNode = NULL, * head = NULL;
	if(hddInfoList == NULL || hddMonInfo == NULL) return iRet;
	head = hddInfoList;
	newNode = (hdd_mon_info_node_t *)malloc(sizeof(hdd_mon_info_node_t));
	memset(newNode, 0, sizeof(hdd_mon_info_node_t));
	strcpy(newNode->hddMonInfo.hdd_name, hddMonInfo->hdd_name);
	newNode->hddMonInfo.hdd_index              = hddMonInfo->hdd_index;
	newNode->hddMonInfo.hdd_type               = hddMonInfo->hdd_type;
	newNode->hddMonInfo.average_program        = hddMonInfo->average_program;
	newNode->hddMonInfo.current_reserved_block = hddMonInfo->current_reserved_block;
	newNode->hddMonInfo.ecc_count              = hddMonInfo->ecc_count;
	newNode->hddMonInfo.endurance_check        = hddMonInfo->endurance_check;
	newNode->hddMonInfo.good_block_rate        = hddMonInfo->good_block_rate;
	newNode->hddMonInfo.hdd_health_percent     = hddMonInfo->hdd_health_percent;
	newNode->hddMonInfo.hdd_temp               = hddMonInfo->hdd_temp;
	newNode->hddMonInfo.max_program            = hddMonInfo->max_program;
	newNode->hddMonInfo.max_reserved_block     = hddMonInfo->max_reserved_block;
	newNode->hddMonInfo.power_on_time          = hddMonInfo->power_on_time;
	
	if(hddMonInfo->smartAttriInfoList)
	{
		smart_attri_info_node_t * curAtrriInfoNode = NULL;
		newNode->hddMonInfo.smartAttriInfoList = CreateSmartAttriInfoList();
		curAtrriInfoNode = hddMonInfo->smartAttriInfoList->next;
		while(curAtrriInfoNode) {
			InsertSmartAttriInfoNode(newNode->hddMonInfo.smartAttriInfoList, &curAtrriInfoNode->attriInfo);
			curAtrriInfoNode = curAtrriInfoNode->next;
		}
	}
	else
	{
		newNode->hddMonInfo.smartAttriInfoList = NULL;
	}
	newNode->next = head->next;
	head->next = newNode;
	iRet = 0;
	return iRet;
}

int DeleteHDDInfoNode(hdd_mon_info_list hddInfoList, char * hddName)
{
	int iRet = -1;
	hdd_mon_info_node_t * delNode = NULL, * head = NULL;
	hdd_mon_info_node_t * p = NULL;
	if(hddInfoList == NULL || hddName == NULL) return iRet;
	head = hddInfoList;
	p = head;
	delNode = head->next;
	while(delNode)
	{
		if(!strcmp(delNode->hddMonInfo.hdd_name, hddName))
		{
			p->next = delNode->next;
			if(delNode->hddMonInfo.smartAttriInfoList)
			{
				DestroySmartAttriInfoList(delNode->hddMonInfo.smartAttriInfoList);
				delNode->hddMonInfo.smartAttriInfoList = NULL;
			}
			free(delNode);
			delNode = NULL;
			iRet = 0;
			break;
		}
		p = delNode;
		delNode = delNode->next;
	}
	if(iRet == -1) iRet = 1;
	return iRet;
}

int DeleteHDDInfoNodeWithID(hdd_mon_info_list hddInfoList, int hddIndex)
{
	int iRet = -1;
	hdd_mon_info_node_t * delNode = NULL, * head = NULL;
	hdd_mon_info_node_t * p = NULL;
	if(hddInfoList == NULL) return iRet;
	head = hddInfoList;
	p = head;
	delNode = head->next;
	while(delNode)
	{
		if(delNode->hddMonInfo.hdd_index == hddIndex)
		{
			p->next = delNode->next;
			if(delNode->hddMonInfo.smartAttriInfoList)
			{
				DestroySmartAttriInfoList(delNode->hddMonInfo.smartAttriInfoList);
				delNode->hddMonInfo.smartAttriInfoList = NULL;
			}
			free(delNode);
			delNode = NULL;
			iRet = 0;
			break;
		}
		p = delNode;
		delNode = delNode->next;
	}
	if(iRet == -1) iRet = 1;
	return iRet;
}

int DeleteAllHDDInfoNode(hdd_mon_info_list hddInfoList)
{
	int iRet = -1;
	hdd_mon_info_node_t * delNode = NULL, *head = NULL;
	if(hddInfoList == NULL) return iRet;
	head = hddInfoList;
	delNode = head->next;
	while(delNode)
	{
		head->next = delNode->next;
		if(delNode->hddMonInfo.smartAttriInfoList)
		{
			DestroySmartAttriInfoList(delNode->hddMonInfo.smartAttriInfoList);
			delNode->hddMonInfo.smartAttriInfoList = NULL;
		}
		free(delNode);
		delNode = head->next;
	}
	iRet = 0;
	return iRet;
}

void hdd_DestroyHDDInfoList(hdd_mon_info_list hddInfoList)
{
	if(NULL == hddInfoList) return;
	DeleteAllHDDInfoNode(hddInfoList);
	/*free(hddInfoList); 
	hddInfoList = NULL;*/
}

bool IsHddInfoListEmpty(hdd_mon_info_list hddInfoList)
{
	bool bRet = true;
	hdd_mon_info_node_t * curNode = NULL, *head = NULL;
	if(hddInfoList == NULL) return bRet;
	head = hddInfoList;
	curNode = head->next;
	if(curNode != NULL) bRet = false;
	return bRet;
}
//----------------------------------------------------------------------------------------------

bool GetHDDHealth(hdd_mon_info_t * hddMonInfo)
{
	bool bRet = false;
	if(hddMonInfo == NULL) return bRet;
    if(hddMonInfo->hdd_type == SQFlash)
	{
		hddMonInfo->hdd_health_percent = INVALID_DEVMON_VALUE;
		if(hddMonInfo->hdd_name[4] == 'S' || hddMonInfo->hdd_name[4] == 's')
		{
			if(hddMonInfo->max_program != 0)
			{
				hddMonInfo->hdd_health_percent = 100 - hddMonInfo->average_program*100/hddMonInfo->max_program;
			}
		}
		else if(hddMonInfo->hdd_name[4] == 'P' || hddMonInfo->hdd_name[4] == 'p')
		{
			hddMonInfo->hdd_health_percent = hddMonInfo->endurance_check;
		}
		else
		{
			hddMonInfo->hdd_health_percent = 100;
		}
	}
	else if(hddMonInfo->hdd_type == StdDisk)
	{
		if(hddMonInfo->smartAttriInfoList)
		{
         /*
		   05	Reallocated Sectors Count	     2	70
		   10	Spin Retry Count	             2	50
		   184	End-to-End Error	             1	50
		   196	Reallocation Event Count	     1	40
		   197	Current Pending Sectors Count	 1	40
		   198	Offline uncorrectable Sectors Count	2	70
		   201	Soft Read Error Rate	         1	20
		   */
			int tmpHealth = INVALID_DEVMON_VALUE;
			smart_attri_info_node_t * curAttriInfoNode = hddMonInfo->smartAttriInfoList->next;
			smart_attri_info_t * curAttriInfo = NULL;
			while(curAttriInfoNode)
			{
				curAttriInfo = (smart_attri_info_t *)&curAttriInfoNode->attriInfo;
				switch(curAttriInfo->attriType)
				{
				case ReallocatedSectorsCount:
					{
						if(tmpHealth == INVALID_DEVMON_VALUE) tmpHealth = 100;
						{
							unsigned int tmpData = (curAttriInfo->attriVendorData[1]<<8) + curAttriInfo->attriVendorData[0];
							int tmpValue = tmpData*2 < 70 ? tmpData*2 : 70;
							tmpHealth = (tmpHealth*(100 - tmpValue))/100;
						}
						break;
					}
				case SpinRetryCount:
					{
						if(tmpHealth == INVALID_DEVMON_VALUE) tmpHealth = 100;
						{
							unsigned int tmpData = (curAttriInfo->attriVendorData[1]<<8) + curAttriInfo->attriVendorData[0];
							int tmpValue = tmpData*2 < 50 ? tmpData*2 : 50;
							tmpHealth = (tmpHealth*(100 - tmpValue))/100;
						}
						break;
					}
				case EndtoEnderror:
					{
						if(tmpHealth == INVALID_DEVMON_VALUE) tmpHealth = 100;
						{
							unsigned int tmpData = (curAttriInfo->attriVendorData[1]<<8) + curAttriInfo->attriVendorData[0];
							int tmpValue = tmpData < 50 ? tmpData : 50;
							tmpHealth = (tmpHealth*(100 - tmpValue))/100;
						}
						break;
					}
				case ReallocationEventCount:
					{
						if(tmpHealth == INVALID_DEVMON_VALUE) tmpHealth = 100;
						{
							unsigned int tmpData = (curAttriInfo->attriVendorData[1]<<8) + curAttriInfo->attriVendorData[0];
							int tmpValue = tmpData < 40 ? tmpData : 40;
							tmpHealth = (tmpHealth*(100 - tmpValue))/100;
						}
						break;
					}
				case CurrentPendingSectorCount:
					{
						if(tmpHealth == INVALID_DEVMON_VALUE) tmpHealth = 100;
						{
							unsigned int tmpData = (curAttriInfo->attriVendorData[1]<<8) + curAttriInfo->attriVendorData[0];
							int tmpValue = tmpData < 40 ? tmpData : 40;
							tmpHealth = (tmpHealth*(100 - tmpValue))/100;
						}
						break;
					}
				case UncorrectableSectorCount:
					{
						if(tmpHealth == INVALID_DEVMON_VALUE) tmpHealth = 100;
						{
							unsigned int tmpData = (curAttriInfo->attriVendorData[1]<<8) + curAttriInfo->attriVendorData[0];
							int tmpValue = tmpData*2 < 70 ? tmpData*2 : 70;
							tmpHealth = (tmpHealth*(100 - tmpValue))/100;
						}
						break;
					}
				case OffTrackSoftReadErrorRate:
					{
						if(tmpHealth == INVALID_DEVMON_VALUE) tmpHealth = 100;
						{
							unsigned int tmpData = (curAttriInfo->attriVendorData[1]<<8) + curAttriInfo->attriVendorData[0];
							int tmpValue = tmpData < 20 ? tmpData : 20;
							tmpHealth = (tmpHealth*(100 - tmpValue))/100;
						}
						break;
					}
				default:
					{
						break;
					}
				}
				curAttriInfoNode = curAttriInfoNode->next;
			}
			hddMonInfo->hdd_health_percent = tmpHealth;
		}
	}
	bRet = true;
	return bRet;
}

bool GetAttriNameFromType(smart_attri_type_t attriType, char * attriName)
{
	bool bRet = false;
    if(NULL == attriName) return bRet;
	switch(attriType)
	{
	case ReadErrorRate:
		{
			strcpy(attriName, "ReadErrorRate");
			break;
		}
	case ThroughputPerformance:
		{
			strcpy(attriName, "ThroughputPerformance");
			break;
		}
	case SpinUpTime:
		{
			strcpy(attriName, "SpinUpTime");
			break;
		}
	case StartStopCount:
		{
			strcpy(attriName, "StartStopCount");
			break;
		}
	case ReallocatedSectorsCount:
		{
			strcpy(attriName, "ReallocatedSectorsCount");
			break;
		}
	case ReadChannelMargin:
		{
			strcpy(attriName, "ReadChannelMargin");
			break;
		}
	case SeekErrorRate:
		{
			strcpy(attriName, "SeekErrorRate");
			break;
		}
	case SeekTimePerformance:
		{
			strcpy(attriName, "SeekTimePerformance");
			break;
		}
	case PowerOnHoursPOH:
		{
			strcpy(attriName, "PowerOnHoursPOH");
			break;
		}
	case SpinRetryCount:
		{
			strcpy(attriName, "SpinRetryCount");
			break;
		}
	case CalibrationRetryCount:
		{
			strcpy(attriName, "CalibrationRetryCount");
			break;
		}
	case PowerCycleCount:
		{
			strcpy(attriName, "PowerCycleCount");
			break;
		}
	case SoftReadErrorRate:
		{
			strcpy(attriName, "SoftReadErrorRate");
			break;
		}
	case SATADownshiftErrorCount:
		{
			strcpy(attriName, "SATADownshiftErrorCount");
			break;
		}
	case EndtoEnderror:
		{
			strcpy(attriName, "EndtoEnderror");
			break;
		}
	case HeadStability:
		{
			strcpy(attriName, "HeadStability");
			break;
		}
	case InducedOpVibrationDetection:
		{
			strcpy(attriName, "InducedOpVibrationDetection");
			break;
		}
	case ReportedUncorrectableErrors:
		{
			strcpy(attriName, "ReportedUncorrectableErrors");
			break;
		}
	case CommandTimeout:
		{
			strcpy(attriName, "CommandTimeout");
			break;
		}
	case HighFlyWrites:
		{
			strcpy(attriName, "HighFlyWrites");
			break;
		}
	case AirflowTemperatureWDC:
		{
			strcpy(attriName, "AirflowTemperatureWDC");
			break;
		}
// 	case TemperatureDifferencefrom100:
// 		{
// 			strcpy(attriName, "TemperatureDifferencefrom100");
// 			break;
// 		}
	case GSenseErrorRate:
		{
			strcpy(attriName, "GSenseErrorRate");
			break;
		}
	case PoweroffRetractCount:
		{
			strcpy(attriName, "PoweroffRetractCount");
			break;
		}
	case LoadCycleCount:
		{
			strcpy(attriName, "LoadCycleCount");
			break;
		}
	case Temperature:
		{
			strcpy(attriName, "Temperature");
			break;
		}
	case HardwareECCRecovered:
		{
			strcpy(attriName, "HardwareECCRecovered");
			break;
		}
	case ReallocationEventCount:
		{
			strcpy(attriName, "ReallocationEventCount");
			break;
		}
	case CurrentPendingSectorCount:
		{
			strcpy(attriName, "CurrentPendingSectorCount");
			break;
		}
	case UncorrectableSectorCount:
		{
			strcpy(attriName, "UncorrectableSectorCount");
			break;
		}
	case UltraDMACRCErrorCount:
		{
			strcpy(attriName, "UltraDMACRCErrorCount");
			break;
		}
	case MultiZoneErrorRate:
		{
			strcpy(attriName, "MultiZoneErrorRate");
			break;
		}
// 	case WriteErrorRateFujitsu:
// 		{
// 			strcpy(attriName, "WriteErrorRateFujitsu");
// 			break;
// 		}
	case OffTrackSoftReadErrorRate:
		{
			strcpy(attriName, "OffTrackSoftReadErrorRate");
			break;
		}
	case DataAddressMarkerrors:
		{
			strcpy(attriName, "DataAddressMarkerrors");
			break;
		}
	case RunOutCancel:
		{
			strcpy(attriName, "RunOutCancel");
			break;
		}
	case SoftECCCorrection:
		{
			strcpy(attriName, "SoftECCCorrection");
			break;
		}
	case ThermalAsperityRateTAR:
		{
			strcpy(attriName, "ThermalAsperityRateTAR");
			break;
		}
	case FlyingHeight:
		{
			strcpy(attriName, "FlyingHeight");
			break;
		}
	case SpinHighCurrent:
		{
			strcpy(attriName, "SpinHighCurrent");
			break;
		}
	case SpinBuzz:
		{
			strcpy(attriName, "SpinBuzz");
			break;
		}
	case OfflineSeekPerformance:
		{
			strcpy(attriName, "OfflineSeekPerformance");
			break;
		}
	case VibrationDuringWrite:
		{
			strcpy(attriName, "VibrationDuringWrite");
			break;
		}
	case ShockDuringWrite:
		{
			strcpy(attriName, "ShockDuringWrite");
			break;
		}
	case DiskShift:
		{
			strcpy(attriName, "DiskShift");
			break;
		}
	case GSenseErrorRateAlt:
		{
			strcpy(attriName, "GSenseErrorRateAlt");
			break;
		}
	case LoadedHours:
		{
			strcpy(attriName, "LoadedHours");
			break;
		}
	case LoadUnloadRetryCount:
		{
			strcpy(attriName, "LoadUnloadRetryCount");
			break;
		}
	case LoadInTime:
		{
			strcpy(attriName, "LoadInTime");
			break;
		}
	case TorqueAmplificationCount:
		{
			strcpy(attriName, "TorqueAmplificationCount");
			break;
		}
	case PowerOffRetractCycle:
		{
			strcpy(attriName, "PowerOffRetractCycle");
			break;
		}
	case GMRHeadAmplitude:
		{
			strcpy(attriName, "GMRHeadAmplitude");
			break;
		}
	case DriveTemperature:
		{
			strcpy(attriName, "DriveTemperature");
			break;
		}
	case SQEnduranceRemainLife:
		{
			strcpy(attriName, "SQEnduranceRemainLife");
			break;
		}
	case SQPowerOnTime:
		{
			strcpy(attriName, "SQPowerOnTime");
			break;
		}
	case SQECCLog:
		{
			strcpy(attriName, "SQECCLog");
			break;
		}
	case SQGoodBlockRate:
		{
			strcpy(attriName, "SQGoodBlockRate");
			break;
		}
	case HeadFlyingHours:
		{
			strcpy(attriName, "HeadFlyingHours");
			break;
		}
// 	case TransferErrorRateFujitsu:
// 		{
// 			strcpy(attriName, "TransferErrorRateFujitsu");
// 			break;
// 		}
	case TotalLBAsWritten:
		{
			strcpy(attriName, "TotalLBAsWritten");
			break;
		}
	case TotalLBAsRead:
		{
			strcpy(attriName, "TotalLBAsRead");
			break;
		}
	case ReadErrorRetryRate:
		{
			strcpy(attriName, "ReadErrorRetryRate");
			break;
		}
	case FreeFallProtection:
		{
			strcpy(attriName, "FreeFallProtection");
			break;
		}
	default:
		{
			strcpy(attriName, "Unknown");
			break;
		}
	}
	bRet = true;
	return bRet;
}

