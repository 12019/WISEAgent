#include "RecoveryHandler.h"
#include "activate_rcvy.h"
#include "public_rcvy.h"

//-----------------------------------------------------------------------------
// Local functions declare:
//-----------------------------------------------------------------------------
#ifdef _is_linux
static BOOL IsLeapYear(int year);
static int DaysHave(int day, int month, int year);
static int CheckLicenses(void);
#else
static void WriteActivateReg();
static void DeleteRunReg();
static void DeleteNotifyKey();
#endif /*_is_linux*/

//-----------------------------------------------------------------------------
// Global functions define:
//-----------------------------------------------------------------------------
#ifndef _is_linux
void Activate()
{
	WriteActivateReg();
	DeleteRunReg();
	DeleteNotifyKey();
	GetRcvyCurStatus();
}
#endif /* _is_linux */

BOOL IsActive()
{
#ifndef _is_linux
   HKEY hk;
   char regName[] = "SOFTWARE\\AdvantechAcronis";
   char valueName[] = "EndDate";
   char valueStr[128] = {0};
   int  valueDataSize = sizeof(valueStr);
   if(ERROR_SUCCESS != app_os_RegOpenKeyExA(HKEY_LOCAL_MACHINE, regName, 0, KEY_ALL_ACCESS, &hk))
	   return FALSE;
   else
   {
      if(ERROR_SUCCESS != app_os_RegQueryValueExA(hk, valueName, 0, NULL, valueStr, &valueDataSize))
      {
         app_os_RegCloseKey(hk);
         return FALSE;
      }
      app_os_RegCloseKey(hk);
      if(strlen(valueStr) == 0)
      {
         return TRUE;
      }
      else
      {
         return FALSE;
      }
   }
#else
#if 1
	if (app_os_is_file_exist(AcroCmdExePath))
		return TRUE;
	else
	    return FALSE;


#else

	int days = CheckLicenses();
	if (days <= 30 )
		return FALSE;
	else
		return TRUE;
#endif /*1*/
#endif /* _is_linux */
}


BOOL IsExpired()
{
#ifndef _is_linux
   BOOL bRet = TRUE;
   HKEY hk;
   char regName[] = "SOFTWARE\\AdvantechAcronis";
   char valueName[] = "EndDate";
   char valueStr[128] = {0};
   int  valueDataSize = sizeof(valueStr);
   if(ERROR_SUCCESS != app_os_RegOpenKeyExA(HKEY_LOCAL_MACHINE, regName, 0, KEY_READ, &hk)) return bRet;
   else
   {
      if(ERROR_SUCCESS != app_os_RegQueryValueExA(hk, valueName, 0, NULL, valueStr, &valueDataSize))
      {
         app_os_RegCloseKey(hk);
         return bRet;
      }
      app_os_RegCloseKey(hk);
   }

   if(strlen(valueStr))
   {
      char timeStr[128] = {0};
      struct tm endTm;
      time_t nowTime = time(NULL);
      time_t endTime;
      memset(&endTm, 0, sizeof(struct tm));
      sprintf(timeStr, "%s 0:0:0", valueStr);

      Str2Tm(timeStr, "%d-%d-%d %d:%d:%d", &endTm);
      endTime = mktime(&endTm);
      bRet = ((long)endTime - (long)nowTime > 0 ? FALSE : TRUE);
      if(bRet == TRUE)
      {
         Str2Tm_MDY(timeStr, "%d/%d/%d %d:%d:%d", &endTm);
         endTime = mktime(&endTm);
         bRet = ((long)endTime - (long)nowTime > 0 ? FALSE : TRUE);
         if(bRet == TRUE)
         {
            Str2Tm_YMD(timeStr, "%d/%d/%d %d:%d:%d", &endTm);
            endTime = mktime(&endTm);
            bRet = ((long)endTime - (long)nowTime > 0 ? FALSE : TRUE);
         }
      }
   }

   return bRet;
#else
#if 1
	if (app_os_is_file_exist(AcroCmdExePath))
		return FALSE;
	else
		return TRUE;

#else

   if (CheckLicenses() > 0)
	   return FALSE;
   else
	   return TRUE;
#endif /*1*/

#endif /* _is_linux */
}


//-----------------------------------------------------------------------------
// Local functions define:
//-----------------------------------------------------------------------------
#ifdef _is_linux
static BOOL IsLeapYear(int year)
{
	if (year % 4 == 0)
	{
		if (!(year % 100 == 0) || (year % 400 == 0))
			return TRUE;
	}
	return FALSE;
}

static int DaysHave(int day, int month, int year)
{
	int days = 0, nowDaysSinceEpoch = 0, daysSinceEpoch = 0;
	int leapDays = 0, daysOfThisYear = 0;
	int daysOfMonth[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
	int i;

	nowDaysSinceEpoch = time(NULL) / (60*60*24) + 1;

	for (i = 1970; i < year-1; i++)
	{
		if (IsLeapYear(i))
			leapDays += 1;
	}
	for (i = 1; i < month; i++)
	{
		daysOfThisYear += daysOfMonth[i-1];
	}
	daysOfThisYear += day;
	if (month > 2 && IsLeapYear(year))
	{
		daysOfThisYear += 1;
	}
	daysSinceEpoch = (year - 1970)*365 + leapDays + daysOfThisYear;

	days = daysSinceEpoch - nowDaysSinceEpoch;
	return days;
}

static int CheckLicenses(void)
{
	char buf[BUFSIZ] = {0};
	int days = 0;
	FILE *fd = popen("acrocmd list licenses | sed '1,2d' | awk \'{ print $4 }\' ", "r");
	if (fd)
	{
		while (fgets(buf, sizeof(buf), fd))
		{
			if (!strstr(buf, "successfully.") && strstr(buf, "."))
			{
				int tmp = 0;
				char *tp = strtok(buf, ".");
				if (tp)
				{
					int d = 0, m = 0, y = 0;
					d = atoi(tp);
					tp = strtok(NULL, ".");
					m = atoi(tp);
					while (*tp++ != '\0');
					y = atoi(tp);
					tmp = DaysHave(d, m, y);
				}

				if (tmp > days)
				{
					days = tmp;
				}
			}
		}
		pclose(fd);
		Recovery_debug_print("haveDays = %d", days);
		return days;
	}
	return -1;
}

#else

static void WriteActivateReg()
{
	char cmdLine[BUFSIZ] = {0};

	memset(cmdLine, 0, sizeof(cmdLine));
	sprintf(cmdLine, "%s", "cmd.exe /c eventcreate /ID 1 /L application /T information /SO AcronisActivateCommand /D \"Activate\"");
	if(!app_os_CreateProcess(cmdLine))
	{
		RecoveryLog(g_loghandle, Normal, "%s()[%d]###Create write activate process failed!\n",__FUNCTION__, __LINE__ );
		return;
	}

}
static void DeleteRunReg()
{
	{
		HKEY hk;
		char regName[] = "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run";
		if(ERROR_SUCCESS == app_os_RegOpenKeyExA(HKEY_LOCAL_MACHINE, regName, 0, KEY_ALL_ACCESS, &hk))
		{
			app_os_RegDeleteValueA(hk, "Acronis");
			app_os_RegCloseKey(hk);
		}
	}

	{
		HKEY hk;
		char regName[] = "SOFTWARE\\AdvantechAcronis";
		if(ERROR_SUCCESS == app_os_RegOpenKeyExA(HKEY_LOCAL_MACHINE, regName, 0, KEY_ALL_ACCESS, &hk))
		{
			app_os_RegSetValueExA(hk, "EndDate", 0, REG_SZ, "", 0);
			app_os_RegCloseKey(hk);
		}
	}
}

static void DeleteNotifyKey()
{
	char acrNotifyKeyExePath[MAX_PATH] = {0};
	if(app_os_GetDefProgramFilesPath(acrNotifyKeyExePath))
	{
		strcat(acrNotifyKeyExePath, "\\Acronis\\CommandLineTool\\AcronisNotifyKey.exe");
		if(app_os_is_file_exist(acrNotifyKeyExePath))
		{
			app_os_file_remove(acrNotifyKeyExePath);
		}
	}
}
#endif /*_is_linux*/
