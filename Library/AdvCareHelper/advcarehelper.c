#include "advcarehelper.h"
#include <stdlib.h>
#include <string.h>
#include "util_libloader.h"
#include "util_string.h"
//----------------------------------Susi lib data define---------------------------------------
#define DEF_ADVCARE_LIB_NAME    "AdvCare.dll"
typedef int (*PAdvCareDLLInit)();
typedef int (*PAdvCareDLLUnInit)();
typedef int (*PAdvCareCheckValid)();
typedef int (*PAdvCareGetBIOSVersion)(char *biosVersion, unsigned long *size);
typedef int (*PAdvCareGetPlatformName)(char *platformName, unsigned long *size);
void * hAdvCareDll = NULL;
PAdvCareDLLInit pAdvCareDLLInit = NULL;
PAdvCareDLLUnInit pAdvCareDLLUnInit = NULL;
PAdvCareCheckValid pAdvCareCheckValid = NULL;
PAdvCareGetBIOSVersion pAdvCareGetBIOSVersion = NULL;
PAdvCareGetPlatformName pAdvCareGetPlatformName = NULL;

//----------------------------------------------------------------------------------------------
bool advcare_IsExistAdvCareLib()
{
	bool bRet = false;
	void * hAdvCare = NULL;
	if(util_dlopen(DEF_ADVCARE_LIB_NAME, &hAdvCare))
	{
		bRet = true;
		util_dlclose(hAdvCare);
		hAdvCare = NULL;
	}
	return bRet;
}

void advcare_GetAdvCareFunction(void * hAdvCareDLL)
{
	if(hAdvCareDLL!=NULL)
	{
		pAdvCareDLLInit = (PAdvCareDLLInit)util_dlsym(hAdvCareDLL, "AdvCareDLLInit");
		pAdvCareDLLUnInit = (PAdvCareDLLUnInit)util_dlsym(hAdvCareDLL, "AdvCareDLLUnInit");
		pAdvCareCheckValid = (PAdvCareCheckValid)util_dlsym(hAdvCareDLL, "AdvCareCheckValid");
		pAdvCareGetBIOSVersion = (PAdvCareGetBIOSVersion)util_dlsym(hAdvCareDLL, "AdvCareGetBIOSVersion");
		pAdvCareGetPlatformName = (PAdvCareGetPlatformName)util_dlsym(hAdvCareDLL, "AdvCareGetPlatformName");
	}
}

bool advcare_StartupAdvCareLib()
{
	bool bRet = false;
	if(util_dlopen(DEF_ADVCARE_LIB_NAME, &hAdvCareDll))
	{
		advcare_GetAdvCareFunction(hAdvCareDll);
		if(pAdvCareDLLInit)
		{
			bRet = pAdvCareDLLInit();
		}
	}
	return bRet;
}

bool advcare_CleanupAdvCareLib()
{
	bool bRet = false;
	if(pAdvCareDLLUnInit)
	{
		bRet = pAdvCareDLLUnInit();
	}
	if(hAdvCareDll != NULL)
	{
		util_dlclose(hAdvCareDll);
		hAdvCareDll = NULL;
		pAdvCareDLLInit = NULL;
		pAdvCareDLLUnInit = NULL;
		pAdvCareCheckValid = NULL;
		pAdvCareGetBIOSVersion = NULL;
		pAdvCareGetPlatformName = NULL;
	}
	return bRet;
}

bool advcare_GetPlatformName(char* name, int length)
{
	bool bRet = false;
	if(pAdvCareGetPlatformName)
	{
		unsigned long  tmpLen = length;
		if(pAdvCareGetPlatformName(NULL, &tmpLen))
		{
			if(tmpLen > 0)
			{
				wchar_t * tmpPfiNameWcs = (wchar_t *)malloc(sizeof(wchar_t)*(tmpLen+1));
				memset(tmpPfiNameWcs, 0, sizeof(wchar_t)*(tmpLen+1));
				if(pAdvCareGetPlatformName((char *)tmpPfiNameWcs, &tmpLen))
				{
					char * tmpPfiNameBs = UnicodeToANSI(tmpPfiNameWcs);
					if(tmpPfiNameBs && strlen(tmpPfiNameBs))
					{
						strcpy(name, tmpPfiNameBs);
 					}
					free(tmpPfiNameBs);
				}
				free(tmpPfiNameWcs);
				bRet = true;
			}
		}
	}
	return bRet;
}

bool advcare_GetBIOSVersion(char* version, int length)
{
	bool bRet = false;
	if(pAdvCareGetBIOSVersion)
	{
		unsigned long  tmpLen = length;
		if(pAdvCareGetBIOSVersion(NULL, &tmpLen))
		{
			if(tmpLen > 0)
			{
				wchar_t * tmpBiosVerWcs = (wchar_t *)malloc(sizeof(wchar_t)*(tmpLen+1));
				memset(tmpBiosVerWcs, 0, sizeof(wchar_t)*(tmpLen+1));
				if(pAdvCareGetBIOSVersion((char *)tmpBiosVerWcs, &tmpLen))
				{
					char * tmpBiosVerBs = UnicodeToANSI(tmpBiosVerWcs);
					if(tmpBiosVerBs && strlen(tmpBiosVerBs))
					{
						strcpy(version, tmpBiosVerBs);
 					}
					free(tmpBiosVerBs);
				}
				free(tmpBiosVerWcs);
				bRet = true;
			}
		}
	}
	return bRet;
}