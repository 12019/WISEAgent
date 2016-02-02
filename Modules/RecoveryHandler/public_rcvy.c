#include "RecoveryHandler.h"
#include "public_rcvy.h"

//-----------------------------------------------------------------------------
// Global functions define:
//-----------------------------------------------------------------------------
int GetFileLineCount(const char * pFileName)
{
	FILE * fp = NULL;
	char lineBuf[BUFSIZ] = {0};
	char * p = NULL;
	int lineLen = 0;
	int lineCount = 0;
	int isCountAdd = 0;
	if(NULL == pFileName) return -1;

	fp = fopen (pFileName, "rb");
	if(fp)
	{
		while ((lineLen = fread(lineBuf, 1, BUFSIZ, fp)) != 0)
		{
			isCountAdd = 1;
			p = lineBuf;
			while ((p = (char*)memchr((void*)p, '\n', (lineBuf + lineLen) - p)))
			{
				++p;
				++lineCount;
				isCountAdd = 0;
			}
			memset(lineBuf, 0, BUFSIZ);
		}
		if(isCountAdd) ++lineCount;
		fclose(fp);
	}
	return lineCount;
}

BOOL FileCopy(const char * srcFilePath, const char * destFilePath)
{
	BOOL bRet = FALSE;
	FILE *fpSrc = NULL, *fpDest = NULL;
	if(NULL == srcFilePath || NULL == destFilePath) goto done;
	fpSrc = fopen(srcFilePath, "rb");
	if(NULL == fpSrc) goto done;
	fpDest = fopen(destFilePath, "wb");
	if(NULL == fpDest) goto done;

	{
		char buf[BUFSIZ] = {0};
		int size = 0;
		while ((size = fread(buf, 1, BUFSIZ, fpSrc)) != 0)
		{
			fwrite(buf, 1, size, fpDest);
		}
		bRet = TRUE;
	}
done:
	if(fpSrc) fclose(fpSrc);
	if(fpDest) fclose(fpDest);
	return bRet;
}

BOOL FileAppend(const char *srcFilePath, const char * destFilePath)
{
	BOOL bRet = FALSE;
	FILE *fpSrc = NULL, *fpDest = NULL;
	if(NULL == srcFilePath || NULL == destFilePath) goto done;
	fpSrc = fopen(srcFilePath, "rb");
	if(NULL == fpSrc) goto done;
	fpDest = fopen(destFilePath, "ab");
	if(NULL == fpDest) goto done;
	{
		char lineBuf[BUFSIZ*2] = {0};
		while (!feof(fpSrc))
		{
			memset(lineBuf, 0, sizeof(lineBuf));
			if(fgets(lineBuf, sizeof(lineBuf), fpSrc))
			{
				fputs(lineBuf, fpDest);
			}
		}
		bRet = TRUE;
	}
done:
	if(fpSrc) fclose(fpSrc);
	if(fpDest) fclose(fpDest);
	return bRet;
}



BOOL Str2Tm_MDY(const char *buf, const char *format, struct tm *tm)
{
	BOOL bRet = FALSE;
	int iYear = 0, iMon = 0, iDay = 0, iHour = 0, iMin = 0, iSec = 0;
	if(NULL == buf || NULL == format || NULL == tm) return bRet;
	if(sscanf(buf, format, &iMon, &iDay, &iYear, &iHour, &iMin, &iSec) != 0)
	{
		tm->tm_year = iYear - 1900;
		tm->tm_mon = iMon - 1;
		tm->tm_mday = iDay;
		tm->tm_hour = iHour;
		tm->tm_min = iMin;
		tm->tm_sec = iSec;
		tm->tm_isdst = 0;
		bRet = TRUE;
	}
	return bRet;
}

BOOL Str2Tm(const char *buf, const char *format, struct tm *tm)
{
	BOOL bRet = FALSE;
	int iYear = 0, iMon = 0, iDay = 0, iHour = 0, iMin = 0, iSec = 0;
	if(NULL == buf || NULL == format || NULL == tm) return bRet;
	if(sscanf(buf, format, &iYear, &iMon, &iDay, &iHour, &iMin, &iSec) != 0)
	{
		tm->tm_year = iYear - 1900;
		tm->tm_mon = iMon - 1;
		tm->tm_mday = iDay;
		tm->tm_hour = iHour;
		tm->tm_min = iMin;
		tm->tm_sec = iSec;
		tm->tm_isdst = 0;
		bRet = TRUE;
	}
	return bRet;
}

BOOL Str2Tm_YMD(const char *buf, const char *format, struct tm *tm)
{
	BOOL bRet = FALSE;
	int iYear = 0, iMon = 0, iDay = 0, iHour = 0, iMin = 0, iSec = 0;
	if(NULL == buf || NULL == format || NULL == tm) return bRet;
	if(sscanf(buf, format, &iYear, &iMon, &iDay,  &iHour, &iMin, &iSec) != 0)
	{
		tm->tm_year = iYear - 1900;
		tm->tm_mon = iMon - 1;
		tm->tm_mday = iDay;
		tm->tm_hour = iHour;
		tm->tm_min = iMin;
		tm->tm_sec = iSec;
		tm->tm_isdst = 0;
		bRet = TRUE;
	}
	return bRet;
}

#ifndef _is_linux
BOOL WriteDefaultReg()
{
	BOOL bRet = FALSE;
	HKEY hk, hkSF;
	char regSF[] = "SOFTWARE";
	char regName[] = "SOFTWARE\\AdvantechAcronis";
	if(ERROR_SUCCESS == app_os_RegOpenKeyExA(HKEY_LOCAL_MACHINE, regSF, 0, KEY_ALL_ACCESS, &hkSF))
	{
		if(ERROR_SUCCESS == app_os_RegOpenKeyExA(HKEY_LOCAL_MACHINE, regName, 0, KEY_ALL_ACCESS, &hk))
		{
			app_os_RegCloseKey(hk);
		}
		else
		{
			HKEY hkAdv;
			DWORD dwDisp;
			if(ERROR_SUCCESS == RegCreateKeyEx(hkSF, "AdvantechAcronis", 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hkAdv, &dwDisp))
			{
				bRet = TRUE;
			}
		}
		app_os_RegCloseKey(hkSF);
	}
	return bRet;
}
#endif /*_is_linux*/