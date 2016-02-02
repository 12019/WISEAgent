#include "hwmhelper.h"
#include "HWM3PartyHelper.h"
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include "OsDeclarations.h"
#include "util_libloader.h"

//----------------------------------Susi lib data define---------------------------------------
typedef enum SUSILIBTYPE
{
	SA_LIB_UNKNOWN,
	SA_LIB_3,
	SA_LIB_4,
	SA_LIB_3PTY,
}SUSILIBTYPE;

#define SUSI_STATUS_NOT_INITIALIZED				((uint32_t)0xFFFFFFFF)
#define SUSI_STATUS_SUCCESS						((uint32_t)0)

#define SUSI_ID_BOARD_NAME_STR					((uint32_t)1)
#define SUSI_ID_BOARD_BIOS_REVISION_STR			((uint32_t)4)

#define SUSI_ID_BASE_GET_NAME_HWM						(0x00000000)
#define SUSI_ID_MAPPING_GET_NAME_HWM(Id)				((uint32_t)(Id | SUSI_ID_BASE_GET_NAME_HWM))

#define SUSI_ID_HWM_TEMP_MAX					10									/* Maximum temperature item number */
#define SUSI_ID_HWM_TEMP_BASE					0x00020000
#define SUSI_ID_HWM_VOLTAGE_MAX					23									/* Maximum voltage item number */
#define SUSI_ID_HWM_VOLTAGE_BASE				0x00021000
#define SUSI_ID_HWM_FAN_MAX						10									/* Maximum fan item number */
#define SUSI_ID_HWM_FAN_BASE					0x00022000
#define SUSI_ID_HWM_CURRENT_MAX					3									/* Maximum current item number */
#define SUSI_ID_HWM_CURRENT_BASE				0x00023000
#define SUSI_ID_HWM_CASEOPEN_MAX				3									/* Maximum caseopen item number */
#define SUSI_ID_HWM_CASEOPEN_BASE				0x00024000

SUSILIBTYPE   SusiLibType = SA_LIB_UNKNOWN;
#ifdef WIN32
#define DEF_SUSI4_LIB_NAME    "Susi4.dll"
#else
#define DEF_SUSI4_LIB_NAME    "libSUSI-4.00.so"
#endif

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#include <windows.h>
#ifndef SUSI_API
#define SUSI_API WINAPI
#endif
#else
#define SUSI_API
#endif

typedef uint32_t (SUSI_API *PSusiLibInitialize)();
typedef uint32_t (SUSI_API *PSusiLibUninitialize)();
typedef uint32_t (SUSI_API *PSusiBoardGetValue)(uint32_t Id, uint32_t *pValue);
typedef uint32_t (SUSI_API *PSusiBoardGetStringA)(uint32_t Id, char *pBuffer, uint32_t *pBufLen);
void * hSUSI4Dll = NULL;
PSusiLibInitialize pSusiLibInitialize = NULL;
PSusiLibUninitialize pSusiLibUninitialize = NULL;
PSusiBoardGetValue pSusiBoardGetValue = NULL;
PSusiBoardGetStringA pSusiBoardGetStringA = NULL;

#ifdef WIN32
#define DEF_SUSI3_LIB_NAME    "Susi.dll"
#else
#define DEF_SUSI3_LIB_NAME    "libSUSI-3.02.so"
#endif
typedef int (*PSusiDllInit)();
typedef int (*PSusiDllUnInit)();
typedef void (*PSusiDllGetVersion)(unsigned long *major, unsigned long *minor);
typedef int (*PSusiHWMAvailable)();
typedef int (*PSusiHWMGetFanSpeed)(unsigned short fanType, unsigned short *retval, unsigned short *typeSupport);
typedef int (*PSusiHWMGetTemperature)(unsigned short tempType, float *retval, unsigned short *typeSupport);
typedef int (*PSusiHWMGetVoltage)(unsigned long voltType, float *retval, unsigned long *typeSupport);
typedef int (*PSusiCoreGetBIOSVersion)(char* BIOSVersion, unsigned long* size);
typedef int (*PSusiCoreGetPlatformName)(char* PlatformName, unsigned long* size);

void * hSUSI3Dll = NULL;
PSusiDllInit pSusiDllInit = NULL;
PSusiDllUnInit pSusiDllUnInit = NULL;
PSusiDllGetVersion pSusiDllGetVersion = NULL;
PSusiHWMAvailable pSusiHWMAvailable = NULL;
PSusiHWMGetTemperature pSusiHWMGetTemperature = NULL;
PSusiHWMGetFanSpeed  pSusiHWMGetFanSpeed = NULL;
PSusiHWMGetVoltage pSusiHWMGetVoltage = NULL;
PSusiCoreGetBIOSVersion pSusiCoreGetBIOSVersion = NULL;
PSusiCoreGetPlatformName pSusiCoreGetPlatformName = NULL;
//----------------------------------------------------------------------------------------------
static char SUSI3TempDefTagArray[16][DEF_HWMNAME_LENGTH] = {
	"TCPU",
	"TSYS",
	"TAUX",
	"TCPU2",
	"OEM0",
	"OEM1",
	"OEM2",
	"OEM3",
	"OEM4",
	"OEM5",
	"OEM6",
	"OEM7",
	"OEM8",
	"OEM9",
	"OEM10",
	"OEM11"
};

static char SUSI3VoltDefTagArray[32][DEF_HWMNAME_LENGTH] = {
	"VCORE",
	"TSYS",
	"TAUX",
	"TCPU2",
	"V25",
	"V33",
	"V50",
	"V120",
	"V5SB",
	"V3SB",
	"OEM6",
	"OEM7",
	"OEM8",
	"OEM9",
	"OEM10",
	"OEM11",
	"TCPU",
	"TSYS",
	"TAUX",
	"TCPU2",
	"VBAT",
	"VN50",
	"VN120",
	"VTT",
	"VCORE2",
	"V105",
	"V15",
	"V18",
	"V240",
	"OEM0",
	"OEM1",
	"OEM2"
};

static char SUSI3FanDefTagArray[16][DEF_HWMNAME_LENGTH] = {
	"FCPU",
	"FSYS",
	"F2ND",
	"FCPU2",
	"FAUX2",
	"OEM0",
	"OEM1",
	"OEM2",
	"OEM3",
	"OEM4",
	"OEM5",
	"OEM6",
	"OEM7",
	"OEM8",
	"OEM9",
	"OEM10"
};


static char SUSI4TempDefTagArray[10][DEF_HWMTAG_LENGTH] = {
	"cpuT",
	"chipsetT",
	"systemT",
	"cpu2T",
	"oem0T",
	"oem1T",
	"oem2T",
	"oem3T",
	"oem4T",
	"oem5T"
};


static char SUSI4VoltDefTagArray[23][DEF_HWMTAG_LENGTH] = {
	"vcoreV",
	"vcore2V",
	"v2_5V",
	"v3_3V",
	"v5V",
	"v12V",
	"vsb5V",
	"vsb3V",
	"vbatV",
	"nv5V",
	"nv12V",
	"vttV",
	"v24V",
	"dcV",
	"dcstbyV",
	"vbatliV",
	"oem0V",
	"oem1V",
	"oem2V",
	"oem3V",
	"v1_05V",
	"v1_5V",
	"v1_8V"
};

static char SUSI4FanDefTagArray[10][DEF_HWMTAG_LENGTH] = {
	"cpuF",
	"systemF",
	"cpu2F",
	"oem0F",
	"oem1F",
	"oem2F",
	"oem3F",
	"oem4F",
	"oem5F",
	"oem6F"
};

typedef enum{
	TCPU_FLAG = 0,
	TSYS_FLAG,
	TAUX_FLAG,
	TCPU2_FLAG,
}hwm_temp_item_3_flag;

typedef enum{
	CPU_TEMP_FLAG = 0,
	CHIPSET_TEMP_FLAG,
	SYSTEM_TEMP_FLAG,
	CPU2_TEMP_FLAG,
	OEM0_TEMP_FLAG,
	OEM1_TEMP_FLAG,
	OEM2_TEMP_FLAG,
	OEM3_TEMP_FLAG,
	OEM4_TEMP_FLAG,
	OEM5_TEMP_FLAG
}hwm_temp_item_4_flag;

static int HWMTempID3To4(int ID3)
{
	int ID4 = -1;
	switch (ID3){
	case TCPU_FLAG: ID4 = CPU_TEMP_FLAG;   break;
	case TSYS_FLAG: ID4 = SYSTEM_TEMP_FLAG;break;
	case TAUX_FLAG: ID4 = OEM0_TEMP_FLAG;  break;
	case TCPU2_FLAG:ID4 = CPU2_TEMP_FLAG;  break;
	}
	return ID4;
}

typedef enum{
	VCORE_FLAG = 0,
	V25_FLAG,
	V33_FLAG,
	V50_FLAG,
	V120_FLAG,
	V5SB_FLAG,
	V3SB_FLAG,
	VBAT_FLAG,
	VN50_FLAG,
	VN120_FLAG,
	VTT_FLAG,
	VCORE2_FLAG,
	V105_FLAG,
	V15_FLAG,
	V18_FLAG,
	V240_FLAG
}hwm_volt_item_3_flag;

typedef enum{
	VCORE_VOLT_FLAG = 0,
	VCORE2_VOLT_FLAG,
	V2_5_VOLT_FLAG,
	V3_3_VOLT_FLAG,
	V5_VOLT_FLAG,
	V12_VOLT_FLAG,
	VSB5_VOLT_FLAG,
	VSB3_VOLT_FLAG,
	VBAT_VOLT_FLAG,
	NV5_VOLT_FLAG,
	NV12_VOLT_FLAG,
	VTT_VOLT_FLAG,
	V24_VOLT_FLAG,
	DC_VOLT_FLAG,
	DCSTBY_VOLT_FLAG,
	VBATLI_VOLT_FLAG,
	OEM0_VOLT_FLAG,
	OEM1_VOLT_FLAG,
	OEM2_VOLT_FLAG,
	OEM3_VOLT_FLAG,
	V1_05_VOLT_FLAG,
	V1_5_VOLT_FLAG,
	V1_8_VOLT_FLAG,
}hwm_volt_item_4_flag;

static int HWMVoltID3To4(int ID3)
{
	int ID4 = -1;
	switch (ID3)
	{
	case VCORE_FLAG: ID4 = VCORE_VOLT_FLAG; break;
	case V25_FLAG:   ID4 = V2_5_VOLT_FLAG;  break;
	case V33_FLAG:   ID4 = V3_3_VOLT_FLAG;  break;
	case VBAT_FLAG:  ID4 = VBAT_VOLT_FLAG;  break;
	case V50_FLAG:   ID4 = V5_VOLT_FLAG;    break;
	case V5SB_FLAG:  ID4 = VSB5_VOLT_FLAG;  break;
	case V120_FLAG:  ID4 = V12_VOLT_FLAG;   break;
	case VCORE2_FLAG:ID4 = VCORE2_VOLT_FLAG;break;
	case VTT_FLAG:   ID4 = VTT_VOLT_FLAG;   break;
	case V3SB_FLAG:  ID4 = VSB3_VOLT_FLAG;  break;
	case V240_FLAG:  ID4 = V24_VOLT_FLAG;   break;
	case VN50_FLAG:  ID4 = NV5_VOLT_FLAG;   break;
	case VN120_FLAG: ID4 = NV12_VOLT_FLAG;  break;
	case V105_FLAG:  ID4 = OEM0_VOLT_FLAG;  break;
	case V15_FLAG:   ID4 = OEM1_VOLT_FLAG;  break;
	case V18_FLAG:   ID4 = OEM2_VOLT_FLAG;  break;
	}
	return ID4;
}

typedef enum{
	FCPU_FLAG = 0,
	FSYS_FLAG,
	F2ND_FLAG,
	FCPU2_FLAG,
	FAUX2_FLAG,
}hwm_fan_item_3_flag;

typedef enum{
	CPU_FAN_FLAG = 0,
	SYSTEM_FAN_FLAG,
	CPU2_FAN_FLAG,
	OEM0_FAN_FLAG,
	OEM1_FAN_FLAG,
	OEM2_FAN_FLAG,
	OEM3_FAN_FLAG,
	OEM4_FAN_FLAG,
	OEM5_FAN_FLAG,
	OEM6_FAN_FLAG
}hwm_fan_item_4_flag;

static int HWMFanID3To4(int ID3)
{
	int ID4 = -1;
	switch (ID3)
	{
	case FCPU_FLAG: ID4 = CPU_FAN_FLAG;   break;
	case FSYS_FLAG: ID4 = SYSTEM_FAN_FLAG;break;
	case FCPU2_FLAG:ID4 = CPU2_FAN_FLAG;  break;
	case F2ND_FLAG: ID4 = OEM0_FAN_FLAG;  break;
	case FAUX2_FLAG:ID4 = OEM1_FAN_FLAG;  break;
	}
	return ID4;
}


static void hwm_GetSUSI3Function(void * hSUSI3DLL)
{
	if(hSUSI3Dll!=NULL)
	{
		pSusiDllInit = (PSusiDllInit)util_dlsym(hSUSI3Dll, "SusiDllInit");
		pSusiDllUnInit = (PSusiDllUnInit)util_dlsym(hSUSI3Dll, "SusiDllUnInit");
		pSusiHWMAvailable = (PSusiHWMAvailable)util_dlsym(hSUSI3Dll, "SusiHWMAvailable");
		pSusiHWMGetTemperature = (PSusiHWMGetTemperature)util_dlsym(hSUSI3Dll, "SusiHWMGetTemperature");
		pSusiHWMGetFanSpeed = (PSusiHWMGetFanSpeed)util_dlsym(hSUSI3Dll, "SusiHWMGetFanSpeed");
		pSusiHWMGetVoltage = (PSusiHWMGetVoltage)util_dlsym(hSUSI3Dll, "SusiHWMGetVoltage");
		pSusiCoreGetBIOSVersion = (PSusiCoreGetBIOSVersion)util_dlsym(hSUSI3Dll, "SusiCoreGetBIOSVersion");
		pSusiCoreGetPlatformName = (PSusiCoreGetPlatformName)util_dlsym(hSUSI3Dll, "SusiCoreGetPlatformName");
	}
}

static bool hwm_IsExistSUSI3Lib()
{
	return util_dlexist(DEF_SUSI3_LIB_NAME);
}

static bool hwm_StartupSUSI3Lib()
{
	bool bRet = false;
	if(util_dlopen(DEF_SUSI3_LIB_NAME, &hSUSI3Dll))
	{
		hwm_GetSUSI3Function(hSUSI3Dll);
		if(pSusiDllInit)
		{
			if(pSusiDllInit())
			{
				bRet = true;
			}
		}
	}
	return bRet;
}

static bool hwm_CleanupSUSI3Lib()
{
	bool bRet = false;
	if(pSusiDllUnInit)
	{
		if(pSusiDllUnInit())
		{
			bRet = true;
		}
	}
	if(hSUSI3Dll != NULL)
	{
		util_dlclose(hSUSI3Dll);
		hSUSI3Dll = NULL;
		pSusiDllInit = NULL;
		pSusiDllUnInit = NULL;
		pSusiHWMGetTemperature = NULL;
		pSusiHWMGetFanSpeed = NULL;
		pSusiHWMGetVoltage = NULL;
	}
	return bRet;
}

static void hwm_GetSUSI4Function(void * hSUSI4DLL)
{
	if(hSUSI4Dll!=NULL)
	{
		pSusiLibInitialize = (PSusiLibInitialize)util_dlsym(hSUSI4Dll, "SusiLibInitialize");
		pSusiLibUninitialize = (PSusiLibUninitialize)util_dlsym(hSUSI4Dll, "SusiLibUninitialize");
		pSusiBoardGetValue = (PSusiBoardGetValue)util_dlsym(hSUSI4Dll, "SusiBoardGetValue");
		pSusiBoardGetStringA = (PSusiBoardGetStringA)util_dlsym(hSUSI4Dll, "SusiBoardGetStringA");
	}
}

static bool hwm_IsExistSUSI4Lib()
{
	return util_dlexist(DEF_SUSI4_LIB_NAME);
}

static bool hwm_StartupSUSI4Lib()
{
	bool bRet = false;
	if(util_dlopen(DEF_SUSI4_LIB_NAME, &hSUSI4Dll))
	{
		hwm_GetSUSI4Function(hSUSI4Dll);
		if(pSusiLibInitialize)
		{
			uint32_t iRet = pSusiLibInitialize();
			if(iRet != SUSI_STATUS_NOT_INITIALIZED)
			{
				bRet = true;
			}
		}
	}
	return bRet;
}

static bool hwm_CleanupSUSI4Lib()
{
	bool bRet = false;
	if(pSusiLibUninitialize)
	{
		uint32_t iRet = pSusiLibUninitialize();
		if(iRet == SUSI_STATUS_SUCCESS)
		{
			bRet = true;
		}
	}
	if(hSUSI4Dll != NULL)
	{
		util_dlclose(hSUSI4Dll);
		hSUSI4Dll = NULL;
		pSusiLibInitialize = NULL;
		pSusiLibUninitialize = NULL;
		pSusiBoardGetValue = NULL;
		pSusiBoardGetStringA = NULL;
	}
	return bRet;
}

bool hwm_IsExistSUSILib()
{
	bool bRet = false;
	bRet = IsExistHWM3PartyLib();
	if(!bRet)
	{
		bRet = hwm_IsExistSUSI4Lib();
		if(!bRet)
		{
			bRet = hwm_IsExistSUSI3Lib();
		}
	}
	return bRet;
}

bool hwm_StartupSUSILib()
{
	bool bRet = false;
	bRet = StartupHWM3PartyLib();
	if(bRet)
	{
		bRet = HWM3PartyHWMAvailable() <= 0 ? false : true;
	}
	if(!bRet)
	{
		bRet = hwm_StartupSUSI4Lib();
		if(!bRet)
		{
			bRet = hwm_StartupSUSI3Lib();
			if(bRet)
			{
				SusiLibType = SA_LIB_3;
			}
		}
		else
		{
			SusiLibType = SA_LIB_4;
		}
	}
	else
	{
		
		SusiLibType = SA_LIB_3PTY;
	}
	return bRet;
}

bool hwm_CleanupSUSILib()
{
	CleanupHWM3PartyLib();
	hwm_CleanupSUSI4Lib();
	hwm_CleanupSUSI3Lib();
	return true;
}

bool hwm_GetPlatformName(char* name, int length)
{
	bool bRet = false;
	if(SusiLibType == SA_LIB_3PTY)
	{
		unsigned long pBufLen = length;
		bRet = HWM3PartyGetPlatformName(name, &pBufLen);
	}
	else if(SusiLibType == SA_LIB_4)
	{
		if(pSusiBoardGetStringA)
		{
			uint32_t pBufLen = length;
			int iRet = pSusiBoardGetStringA(SUSI_ID_BOARD_NAME_STR, name, &pBufLen);
			if(iRet == SUSI_STATUS_SUCCESS)
			{
				bRet = true;
			}
		}
	}
	else if(SusiLibType == SA_LIB_3)
	{
		if(pSusiCoreGetPlatformName)
		{
			unsigned long pBufLen = 0;
			if(pSusiCoreGetPlatformName(name, &pBufLen))
			{
				bRet = true;
			}
		}
	}
	
	return bRet;
}

bool hwm_GetBIOSVersion(char* version, int length)
{
	bool bRet = false;
	if(SusiLibType == SA_LIB_4)
	{
		if(pSusiBoardGetStringA)
		{
			uint32_t pBufLen = length;
			int iRet = pSusiBoardGetStringA(SUSI_ID_BOARD_BIOS_REVISION_STR, version, &pBufLen);
			if(iRet == SUSI_STATUS_SUCCESS)
			{
				bRet = true;
			}
		}
	}
	else if(SusiLibType == SA_LIB_3)
	{
		if(pSusiCoreGetBIOSVersion)
		{
			unsigned long pBufLen = 0;
			if(pSusiCoreGetBIOSVersion(version, &pBufLen))
			{
				bRet = true;
			}
		}
	}

	return bRet;
}

bool hwm_GetHWMTempInfo(hwm_info_t * pHWMInfo)
{
	bool bRet = false;

	char type[DEF_HWMTYPE_LENGTH] = DEF_SENSORTYPE_TEMPERATURE;
	char unit[DEF_HWMUNIT_LENGTH] = DEF_UNIT_TEMPERATURE_CELSIUS;
	if(!pHWMInfo) return bRet;
	if(SusiLibType == SA_LIB_3PTY)
	{
		if(pHWMInfo->total<=0)
			HWM3PartyGetHWMPlatformInfo(pHWMInfo);
		bRet = HWM3PartyGetHWMTempInfo(pHWMInfo);
	}
	else if(SusiLibType == SA_LIB_4)
	{
		if(pSusiBoardGetValue)
		{
			int tempUnitMax = SUSI_ID_HWM_TEMP_MAX;
			uint32_t iRet = SUSI_STATUS_SUCCESS;
			uint32_t tempValue = 0;
			int i = 0;
			int id = 0;
			for(i=0; i<tempUnitMax; i++)
			{
				tempValue = DEF_INVALID_VALUE;
				id = SUSI_ID_HWM_TEMP_BASE + i;
				iRet = pSusiBoardGetValue(id, &tempValue);
				if(iRet == SUSI_STATUS_SUCCESS)
				{
					hwm_item_t* item = NULL;
					char tag[DEF_HWMTAG_LENGTH] = {0};
					//float value =  (float)tempValue/10;										/*Kelvin*/
					float value =  (float)(tempValue - DEF_TEMP_KELVINS_OFFSET)/10;				/*Celsius*/
					//float value =  (float)(tempValue - DEF_TEMP_KELVINS_OFFSET)/10*1.8+32;	/*Fahrenheit*/
					sprintf(tag, "%s", SUSI4TempDefTagArray[i]);
					item = hwm_FindItem(pHWMInfo, tag);
					if(item == NULL)
					{
						char name[DEF_HWMNAME_LENGTH] = {0};
						if(pSusiBoardGetStringA)
						{
							uint32_t length = sizeof(name);
							if(pSusiBoardGetStringA(SUSI_ID_MAPPING_GET_NAME_HWM(id), name, &length) == SUSI_STATUS_SUCCESS)
							{
								hwm_AddItem(pHWMInfo, type, name, tag, unit, value);
								//printf(">Add Item: (%s, %s, %s, %s, %f)\n", type, name, tag, unit, value);
							}
						}
					}
					else
					{
						item->value = value;
						//printf(">Update Item: (%s, %s, %s, %s, %f)\n", type, item->name, tag, unit, value);
					}
				}
			}
			bRet = true;
		}
	}	
	else if(SusiLibType == SA_LIB_3)
	{
		int offset = 32;
		if(pSusiHWMAvailable)
		{
			if(pSusiHWMAvailable()<=0)
			{
				bRet = false; 
				return bRet;
			}
		}
		if(pSusiHWMGetTemperature)
		{
			unsigned short u16TempSupport=0;
			if(pSusiHWMGetTemperature(0,0,&u16TempSupport))
			{
				short i=0;
				for(i=0; i<16; i++)
				{
					unsigned short id = (unsigned short)(1<<i);
					if ((id & u16TempSupport) != 0)
					{
						float value;
						if (!pSusiHWMGetTemperature(id, &value, NULL))
						{
							hwm_item_t* item = NULL;
							char tag[DEF_HWMTAG_LENGTH];
							int j = HWMTempID3To4(i);
							if (j>=0) strcpy(tag, SUSI4TempDefTagArray[j]);
							else      strcpy(tag, SUSI3TempDefTagArray[i]);
							item = hwm_FindItem(pHWMInfo, tag);
							if(item == NULL)
							{
								char name[DEF_HWMNAME_LENGTH];
								strcpy(name, SUSI3TempDefTagArray[i]);
								hwm_AddItem(pHWMInfo, type, name, tag, unit, value);
								//printf(">Add Item: (%s, %s, %s, %s, %f)\n", type, name, tag, unit, value);
							}
							else
							{
								item->value = value;
								//printf(">Update Item: (%s, %s, %s, %s, %f)\n", type, item->name, tag, unit, value);
							}

						}
					}
				}
			}
			bRet = true;
		}
	}
	return bRet;
}

bool hwm_GetHWMVoltInfo(hwm_info_t * pHWMInfo)
{
	bool bRet = false;
	char type[DEF_HWMTYPE_LENGTH] = DEF_SENSORTYPE_VOLTAGE;
	char unit[DEF_HWMUNIT_LENGTH] = DEF_UNIT_VOLTAGE;
	if(!pHWMInfo) return bRet;
	if(SusiLibType == SA_LIB_3PTY)
	{
		if(pHWMInfo->total<=0)
			HWM3PartyGetHWMPlatformInfo(pHWMInfo);
		bRet = HWM3PartyGetHWMVoltInfo(pHWMInfo);
	}
	else if(SusiLibType == SA_LIB_4)
	{
		if(pSusiBoardGetValue)
		{
			int tempUnitMax = SUSI_ID_HWM_VOLTAGE_MAX;
			uint32_t iRet = SUSI_STATUS_SUCCESS;
			uint32_t tempValue = 0;
			int i = 0;
			int id = 0;
			for(i=0; i<tempUnitMax; i++)
			{
				tempValue = DEF_INVALID_VALUE;
				id = SUSI_ID_HWM_VOLTAGE_BASE + i;
				iRet = pSusiBoardGetValue(id, &tempValue);
				if(iRet == SUSI_STATUS_SUCCESS)
				{
					hwm_item_t* item = NULL;
					char tag[DEF_HWMTAG_LENGTH];
					float value =  (float)tempValue/1000;
					sprintf(tag, "%s", SUSI4VoltDefTagArray[i]);
					item = hwm_FindItem(pHWMInfo, tag);
					if(item == NULL)
					{
						
						char name[DEF_HWMNAME_LENGTH];
						if(pSusiBoardGetStringA)
						{
							uint32_t length = sizeof(name);
							if(pSusiBoardGetStringA(SUSI_ID_MAPPING_GET_NAME_HWM(id), name, &length) == SUSI_STATUS_SUCCESS)
							{
								hwm_AddItem(pHWMInfo, type, name, tag, unit, value);
								//printf(">Add Item: (%s, %s, %s, %s, %f)\n", type, name, tag, unit, value);
							}
						}
					}
					else
					{
						item->value = value;
						//printf(">Update Item: (%s, %s, %s, %s, %f)\n", type, item->name, tag, unit, value);
					}
				}
			}
			bRet = true;
		}
	}
	else if(SusiLibType == SA_LIB_3)
	{
		int offset = 0;
		if(pSusiHWMAvailable)
		{
			if(pSusiHWMAvailable()<=0)
			{
				bRet = false; 
				return bRet;
			}
		}
		if(pSusiHWMGetVoltage)
		{
			unsigned long u32VoltSupport=0;
			if(pSusiHWMGetVoltage(0,0,&u32VoltSupport))
			{
				short i=0;
				for(i=0; i<32; i++)
				{
					unsigned short id = (unsigned short)(1<<i);
					if ((id & u32VoltSupport) != 0)
					{
						float value;
						if (!pSusiHWMGetVoltage(id, &value, NULL))
						{
							hwm_item_t* item = NULL;
							char tag[DEF_HWMTAG_LENGTH];
							int j = HWMVoltID3To4(i);
							if (j>=0) strcpy(tag, SUSI4VoltDefTagArray[j]);
							else      strcpy(tag, SUSI3VoltDefTagArray[i]);
							item = hwm_FindItem(pHWMInfo, tag);
							if(item == NULL)
							{
								char name[DEF_HWMNAME_LENGTH];
								strcpy(name, SUSI3VoltDefTagArray[i]);
								hwm_AddItem(pHWMInfo, type, name, tag, unit, value);
								//printf(">Add Item: (%s, %s, %s, %s, %f)\n", type, name, tag, unit, value);
							}
							else
							{
								item->value = value;
								//printf(">Update Item: (%s, %s, %s, %s, %f)\n", type, item->name, tag, unit, value);
							}

						}
					}
				}
			}
			bRet = true;
		}
	}
	return bRet;
}

bool hwm_GetHWMFanInfo(hwm_info_t * pHWMInfo)
{
	bool bRet = false;
	char type[DEF_HWMTYPE_LENGTH] = DEF_SENSORTYPE_FANSPEED;
	char unit[DEF_HWMUNIT_LENGTH] = DEF_UNIT_FANSPEED;
	if(!pHWMInfo) return bRet;
	if(SusiLibType == SA_LIB_3PTY)
	{
		if(pHWMInfo->total<=0)
			HWM3PartyGetHWMPlatformInfo(pHWMInfo);
		bRet = HWM3PartyGetHWMFanInfo(pHWMInfo);
	}
	else if(SusiLibType == SA_LIB_4)
	{
		if(pSusiBoardGetValue)
		{
			int tempUnitMax = SUSI_ID_HWM_FAN_MAX;
			uint32_t iRet = SUSI_STATUS_SUCCESS;
			uint32_t tempValue = 0;
			int i = 0;
			int id = 0;
			for(i=0; i<tempUnitMax; i++)
			{
				tempValue = DEF_INVALID_VALUE;
				id = SUSI_ID_HWM_FAN_BASE + i;
				iRet = pSusiBoardGetValue(id, &tempValue);
				if(iRet == SUSI_STATUS_SUCCESS)
				{
					hwm_item_t* item = NULL;
					char tag[DEF_HWMTAG_LENGTH];
					float value =  (float)tempValue;
					sprintf(tag, "%s", SUSI4FanDefTagArray[i]);
					item = hwm_FindItem(pHWMInfo, tag);
					if(item == NULL)
					{

						char name[DEF_HWMNAME_LENGTH];
						if(pSusiBoardGetStringA)
						{
							uint32_t length = sizeof(name);
							if(pSusiBoardGetStringA(SUSI_ID_MAPPING_GET_NAME_HWM(id), name, &length) == SUSI_STATUS_SUCCESS)
							{
								hwm_AddItem(pHWMInfo, type, name, tag, unit, value);
								//printf(">Add Item: (%s, %s, %s, %s, %f)\n", type, name, tag, unit, value);
							}
						}
					}
					else
					{
						item->value = value;
						//printf(">Update Item: (%s, %s, %s, %s, %f)\n", type, item->name, tag, unit, value);
					}
				}
			}
			bRet = true;
		}
	}
	else if(SusiLibType == SA_LIB_3)
	{
		int offset = 48;
		if(pSusiHWMAvailable)
		{
			if(pSusiHWMAvailable()<=0)
			{
				bRet = false; 
				return bRet;
			}
		}
		if(pSusiHWMGetFanSpeed)
		{
			unsigned short u16FanSupport=0;
			if(pSusiHWMGetFanSpeed(0,0,&u16FanSupport))
			{
				short i=0;
				for(i=0; i<16; i++)
				{
					unsigned short id = (unsigned short)(1<<i);
					if ((id & u16FanSupport) != 0)
					{
						unsigned short value;
						if (!pSusiHWMGetFanSpeed(id, &value, NULL))
						{
							hwm_item_t* item = NULL;
							char tag[DEF_HWMTAG_LENGTH];
							int j = HWMFanID3To4(i);
							if (j >= 0) strcpy(tag, SUSI4FanDefTagArray[j]);
							else        strcpy(tag, SUSI3FanDefTagArray[i]);
							item = hwm_FindItem(pHWMInfo, tag);
							if(item == NULL)
							{
								char name[DEF_HWMNAME_LENGTH];
								strcpy(name, SUSI3FanDefTagArray[i]);
								hwm_AddItem(pHWMInfo, type, name, tag, unit, value);
								//printf(">Add Item: (%s, %s, %s, %s, %d)\n", type, name, tag, unit, value);
							}
							else
							{
								item->value = value;
								//printf(">Update Item: (%s, %s, %s, %s, %d)\n", type, item->name, tag, unit, value);
							}

						}
					}
				}
			}
			bRet = true;
		}
	}
	return bRet;
}

bool hwm_GetHWMCurrentInfo(hwm_info_t * pHWMInfo)
{
	bool bRet = false;
	char type[DEF_HWMTYPE_LENGTH] = DEF_SENSORTYPE_CURRENT;
	char unit[DEF_HWMUNIT_LENGTH] = DEF_UNIT_CURRENT;
	if(!pHWMInfo) return bRet;
	if(SusiLibType == SA_LIB_3PTY)
	{
		if(pHWMInfo->total<=0)
			HWM3PartyGetHWMPlatformInfo(pHWMInfo);
		bRet = HWM3PartyGetHWMCurrentInfo(pHWMInfo);
	}
	else if(SusiLibType == SA_LIB_4)
	{
		if(pSusiBoardGetValue)
		{
			int tempUnitMax = SUSI_ID_HWM_CURRENT_MAX;
			uint32_t iRet = SUSI_STATUS_SUCCESS;
			uint32_t tempValue = 0;
			int i = 0;
			int id = 0;
			for(i=0; i<tempUnitMax; i++)
			{
				tempValue = DEF_INVALID_VALUE;
				id = SUSI_ID_HWM_CURRENT_BASE + i;
				iRet = pSusiBoardGetValue(id, &tempValue);
				if(iRet == SUSI_STATUS_SUCCESS)
				{
					hwm_item_t* item = NULL;
					char tag[DEF_HWMTAG_LENGTH];
					float value =  (float)tempValue/1000;
					sprintf(tag, "V%d", SUSI_ID_MAPPING_GET_NAME_HWM(id));
					item = hwm_FindItem(pHWMInfo, tag);
					if(item == NULL)
					{

						char name[DEF_HWMNAME_LENGTH];
						if(pSusiBoardGetStringA)
						{
							uint32_t length = sizeof(name);
							if(pSusiBoardGetStringA(SUSI_ID_MAPPING_GET_NAME_HWM(id), name, &length) == SUSI_STATUS_SUCCESS)
							{
								hwm_AddItem(pHWMInfo, type, name, tag, unit, value);
								//printf(">Add Item: (%s, %s, %s, %s, %f)\n", type, name, tag, unit, value);
							}
						}
					}
					else
					{
						item->value = value;
						//printf(">Update Item: (%s, %s, %s, %s, %f)\n", type, item->name, tag, unit, value);
					}
				}
			}
			bRet = true;
		}
	}
	else if(SusiLibType == SA_LIB_3)
	{
		bRet = true;
	}
	return bRet;
}

bool hwm_GetHWMCaseOpenInfo(hwm_info_t * pHWMInfo)
{
	bool bRet = false;
	char type[DEF_HWMTYPE_LENGTH] = DEF_SENSORTYPE_CASEOPEN;
	char unit[DEF_HWMUNIT_LENGTH] = DEF_UNIT_CASEOPEN;
	if(!pHWMInfo) return bRet;
	if(SusiLibType == SA_LIB_3PTY)
	{
		if(pHWMInfo->total<=0)
			HWM3PartyGetHWMPlatformInfo(pHWMInfo);
		bRet = HWM3PartyGetHWMCaseOpenInfo(pHWMInfo);
	}
	else if(SusiLibType == SA_LIB_4)
	{
		if(pSusiBoardGetValue)
		{
			int tempUnitMax = SUSI_ID_HWM_CASEOPEN_MAX;
			uint32_t iRet = SUSI_STATUS_SUCCESS;
			uint32_t tempValue = 0;
			int i = 0;
			int id = 0;
			for(i=0; i<tempUnitMax; i++)
			{
				tempValue = DEF_INVALID_VALUE;
				id = SUSI_ID_HWM_CASEOPEN_BASE + i;
				iRet = pSusiBoardGetValue(id, &tempValue);
				if(iRet == SUSI_STATUS_SUCCESS)
				{
					hwm_item_t* item = NULL;
					char tag[DEF_HWMTAG_LENGTH];
					float value =  (float)tempValue/1000;
					sprintf(tag, "V%d", SUSI_ID_MAPPING_GET_NAME_HWM(id));
					item = hwm_FindItem(pHWMInfo, tag);
					if(item == NULL)
					{

						char name[DEF_HWMNAME_LENGTH];
						if(pSusiBoardGetStringA)
						{
							uint32_t length = sizeof(name);
							if(pSusiBoardGetStringA(SUSI_ID_MAPPING_GET_NAME_HWM(id), name, &length) == SUSI_STATUS_SUCCESS)
							{
								hwm_AddItem(pHWMInfo, type, name, tag, unit, value);
								//printf(">Add Item: (%s, %s, %s, %s, %f)\n", type, name, tag, unit, value);
							}
						}
					}
					else
					{
						item->value = value;
						//printf(">Update Item: (%s, %s, %s, %s, %f)\n", type, item->name, tag, unit, value);
					}
				}
			}
			bRet = true;
		}
	}	
	else if(SusiLibType == SA_LIB_3)
	{
		bRet = true;
	}
	return bRet;
}

bool hwm_GetHWMInfo(hwm_info_t * pHWMInfo)
{
	bool bRet = false;
	if(!pHWMInfo) return bRet;
	bRet = true;
	bRet &= hwm_GetHWMTempInfo(pHWMInfo);
	bRet &= hwm_GetHWMVoltInfo(pHWMInfo);
	bRet &= hwm_GetHWMFanInfo(pHWMInfo);
	bRet &= hwm_GetHWMCurrentInfo(pHWMInfo);
	bRet &= hwm_GetHWMCaseOpenInfo(pHWMInfo);

	return bRet;
}

bool hwm_ReleaseHWMInfo(hwm_info_t * pHWMInfo)
{
	bool bRet = false;
	hwm_item_t *item = NULL;
	hwm_item_t *target = NULL;
	if(!pHWMInfo) return bRet;
	item = hwm_LastItem(pHWMInfo);
	while(item != NULL)
	{
		target = item;
		item = item->prev;
		if(target!=NULL)
		{
			pHWMInfo->total--;
			free(target);
			target = NULL;
		}
	}
	return bRet;
}