#include "ProtectHandler.h"
#include "parser_prtt.h"
#include "public_prtt.h"
#include "activate_prtt.h"

//-----------------------------------------------------------------------------
// Local Types/Macros/Variables:
//-----------------------------------------------------------------------------
#ifndef _WIN32
#define DEF_MCAFEE_SOLIDCORE_CONF_FILE			 "/etc/mcafee/solidcore/solidcore.conf"
#endif /*_WIN32*/

//-----------------------------------------------------------------------------
// Local functions declare:
//-----------------------------------------------------------------------------
#ifndef _WIN32
static BOOL RestartMcAfeeService();
static BOOL HideLicense();
static BOOL AddLicense();
static BOOL SadminRecover();
static BOOL IsHaveLicenses();
#endif /*_WIN32*/

//-----------------------------------------------------------------------------
// Global functions define:
//-----------------------------------------------------------------------------
BOOL IsActive()
{
	BOOL bRet = FALSE;
	char valueStr[128] = {0};
#ifdef _WIN32
	char regName[] = "SOFTWARE\\McAfee";
	char valueName[] = "EndDate";
	if(!app_os_get_regLocalMachine_value(regName, valueName, valueStr, sizeof(valueStr)))return bRet;
	if(strlen(valueStr) == 0) bRet = TRUE;
	else
	{
		if(!strcmp(valueStr, "None")) bRet = TRUE;
		else
		{
			char endDateBuf[128] = {0};
			DES_BASE64Decode(valueStr, endDateBuf);
			if(!strcmp(endDateBuf, "AlreadyActive")) bRet = TRUE;
		}
	}
#else
#if 0	
	FILE *fd = fopen(DEF_LINUX_SA_INSTALL_TIME_RECORD, "rb");
	if (fd)
	{
		fgets(valueStr, sizeof(valueStr), fd);
		fclose(fd);
		if (strlen(valueStr) > 0)
		{
			char decodeDateBuf[128] = {0};
			DES_BASE64Decode(valueStr, decodeDateBuf);
			if(!strcmp(decodeDateBuf, "2100-1-1")) bRet = TRUE;
		}
	}
#else
	if (app_os_is_file_exist(InstallPath))
		return TRUE;
	else
		return FALSE;
#endif /*0*/
#endif /* _WIN32 */
	return bRet;
}

BOOL IsExpired()
{
#ifdef _WIN32 
	BOOL bRet = TRUE;
	char valueStr[128] = {0};
#ifdef _WIN32
	char regName[] = "SOFTWARE\\McAfee";
	char valueName[] = "EndDate";
	app_os_get_regLocalMachine_value(regName, valueName, valueStr, sizeof(valueStr));
#else
	FILE *fd = fopen(DEF_LINUX_SA_INSTALL_TIME_RECORD, "rb");
	if (fd)
	{
		fgets(valueStr, sizeof(valueStr), fd);
		fclose(fd);
	}
#endif /* _WIN32 */
	if(strlen(valueStr))
	{
		char timeStr[128] = {0};
		DES_BASE64Decode(valueStr, timeStr);
#ifdef _WIN32
		if(strcmp(timeStr, "AlreadyActive"))
#else
		if(strcmp(timeStr, "2100-1-1"))// Means already active
#endif /* _WIN32 */
		{
			struct tm endTm;
			time_t nowTime = time(NULL);
			time_t endTime;
			memset(&endTm, 0, sizeof(struct tm));
			sprintf(timeStr, "%s 0:0:0", timeStr);

			app_os_Str2Tm(timeStr, "%d-%d-%d %d:%d:%d", &endTm);
			endTime = mktime(&endTm);
			bRet = ((long)endTime - (long)nowTime > 0 ? FALSE : TRUE);
			if(bRet == TRUE)
			{
				app_os_Str2Tm_MDY(timeStr, "%d/%d/%d %d:%d:%d", &endTm);
				endTime = mktime(&endTm);
				bRet = ((long)endTime - (long)nowTime > 0 ? FALSE : TRUE);
				if(bRet == TRUE)
				{
					app_os_Str2Tm_YMD(timeStr, "%d/%d/%d %d:%d:%d", &endTm);
					endTime = mktime(&endTm);
					bRet = ((long)endTime - (long)nowTime > 0 ? FALSE : TRUE);
				}
			}
		}
		else
		{
			bRet = FALSE;
		}
	}
	return bRet;
#else 
	if (app_os_is_file_exist(InstallPath))
		return FALSE;
	else
	    return TRUE;
#endif /*_WIN32*/
}

void Activate()
{
#ifdef _WIN32
	handler_context_t *pProtectHandlerContext = (handler_context_t *)&g_HandlerContex;
	{
		char regName[] = "SOFTWARE\\McAfee";
		char valueName[] = "EndDate";
		char desStr[128] = {0};
		char srcBuf[32] = {0};
		strcpy(srcBuf, "AlreadyActive");
		DES_BASE64Encode(srcBuf,desStr);
		if(app_os_set_regLocalMachine_value(regName, valueName, desStr, strlen(desStr) + 1))
		{
			SendReplySuccessMsg_Prtt("ActiveSuccess", prtt_active_rep);
		}
		else
		{
			SendReplyFailMsg_Prtt("ActiveFail", prtt_active_rep);			
		}
	}
	{
		char regName[] = "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run";
		char valueName[] = "McAfee";
		app_os_set_regLocalMachine_value(regName, valueName, "", 0);
	}

#else
#if 0
	FILE *fd = NULL;
	if (AddLicense())
	{
		printf("Add license success!\n");
		if (RestartMcAfeeService())
		{
			printf("Restart McAfee service success!\n");
		}
	}

	fd = fopen(DEF_LINUX_SA_INSTALL_TIME_RECORD, "wb");
	if (fd)
	{
		protect_debug_print();
		char dateBuf[128] = "2100-1-1";
		char encodeDateBuf[128] = {0};
		int len = 0;

		DES_BASE64Encode(dateBuf, encodeDateBuf);
		len = strlen(encodeDateBuf);
		protect_debug_print("len = %d", len);
		if ((len > 0) && (len == fprintf(fd, "%s", encodeDateBuf)))
		{
			protect_debug_print();
			SendReplySuccessMsg_Prtt("ActiveSuccess", prtt_active_rep);
			goto done;
		}
	}

	protect_debug_print();
	//Activate failed
	{
		SendReplyFailMsg_Prtt("ActiveFail", prtt_active_rep);
	}
done:
	if (fd)	fclose(fd);
#endif /*0*/
#endif /* _WIN32 */
	GetProtectionCurStatus();
}


#ifdef _WIN32
void IniPasswd()
{
	char PassWord[9] = {0};
	char regName[] = "SOFTWARE\\McAfee";
	char valueName[] = "Passwd";
	char valueData[BUFSIZ] = {0};
	if(app_os_get_regLocalMachine_value(regName, valueName, valueData, sizeof(valueData)))
	{
		if(!strlen(valueData) || !strcmp(valueData, "None"))
		{
			if(app_os_GetGUID(PassWord, sizeof(PassWord) - 1))
			{
				app_os_set_regLocalMachine_value(regName, valueName, PassWord, sizeof(PassWord));
			}
		}
		else
		{
			strcpy(PassWord, valueData);
		}

		if (strlen(PassWord) > 0)
		{
			char srcBuf[32] = {0};
			unsigned char retMD5[DEF_MD5_SIZE] = {0};
			char md5str0x[DEF_MD5_SIZE*2+1] = {0};
			strcat(srcBuf, PassWord);
			strcat(srcBuf, "AdvSUSIAccess");
			GetMD5(srcBuf, strlen(srcBuf), retMD5);
			{
				int i = 0;
				for(i = 0; i<DEF_MD5_SIZE; i++)
				{
					sprintf(&md5str0x[i*2], "%.2x", retMD5[i]);
					//_itoa(retMD5[i], &md5str0x[i*2], 16);
				}
			}
			memcpy(EncodePassWord, md5str0x, 8);
		}
	}
}

#else

void InitLicense()
{
	BOOL restartSrv = FALSE;
	if (IsInstalled())
	{
		if (IsExpired())
		{
			ProtectLog(g_loghandle, Warning, "%s","McAfee license is expired!");
			if (DeleteLicense())
			{
				restartSrv = TRUE;
			}
		}
		else
		{
			SadminRecover();//make sure 'sadmin' is not lock down
			if (!IsHaveLicenses() && AddLicense())
			{
				restartSrv = TRUE;
			}
		}
		if (restartSrv)
			RestartMcAfeeService();
	}
}

BOOL IsInstalled()
{
	if (app_os_is_file_exist(DEF_MCAFEE_SADMIN_PATH))
	{
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}

BOOL DeleteLicense()
{
	BOOL bRet = FALSE;
	char cmdLine[1024] = {0};
	if (app_os_is_file_exist(DEF_MCAFEE_SOLIDCORE_CONF_FILE))
	{
		sprintf(cmdLine, "sed -i '/%s/d' %s", \
			DEF_LINUX_MCAFEE_LICENSE, \
			DEF_MCAFEE_SOLIDCORE_CONF_FILE);
		pid_t pid = fork();// avoid zombie process
		if (0 == pid)
		{
			app_os_CreateProcessWithCmdLineEx(cmdLine);
			exit(0);
		}
		else if (pid > 0)
		{
			waitpid(pid, NULL, 0);
			bRet = TRUE;
		}
	}
	return bRet;
}

//-----------------------------------------------------------------------------
// Local functions define:
//-----------------------------------------------------------------------------
static BOOL RestartMcAfeeService()
{
	if (app_os_CreateProcess("/etc/init.d/scsrvc restart"))
		return TRUE;
	else
		return FALSE;
}

static BOOL HideLicense()
{
	BOOL bRet = FALSE;
	char cmdLine[1024] = {0};
	if (app_os_is_file_exist(DEF_LINUX_MCAFEE_LOG_FILE))
	{
		sprintf(cmdLine, "sed -i 's/%s/0000-0000-0000-0000-0000/' %s", \
			DEF_LINUX_MCAFEE_LICENSE, \
			DEF_LINUX_MCAFEE_LOG_FILE);
		pid_t pid = fork();// avoid zombie process
		if (0 == pid)
		{
			app_os_CreateProcessWithCmdLineEx(cmdLine);
			exit(0);
		}
		else if (pid > 0)
		{
			waitpid(pid, NULL, 0);
			bRet = TRUE;
		}
	}
	if (app_os_is_file_exist(DEF_LINUX_MCAFEE_LOG_FILE1))
	{
		memset(cmdLine, 0, sizeof(cmdLine));
		sprintf(cmdLine, "sed -i 's/%s/0000-0000-0000-0000-0000/' %s", \
			DEF_LINUX_MCAFEE_LICENSE, \
			DEF_LINUX_MCAFEE_LOG_FILE1);
		pid_t pid = fork();// avoid zombie process
		if (0 == pid)
		{
			app_os_CreateProcessWithCmdLineEx(cmdLine);
			exit(0);
		}
		else if (pid > 0)
		{
			waitpid(pid, NULL, 0);
			bRet = TRUE;
		}
		else
		{
			bRet = FALSE;
		}
	}
	return bRet;
}

static BOOL SadminRecover()
{
	char cmdLine[1024] = {0};
	FILE *fd = NULL;

	sprintf(cmdLine, "sadmin recover");
	fd = popen(cmdLine, "r");
	if (fd)
	{
		char buf[1024] = {0};
		fgets(buf, sizeof(buf)-1, fd);
		pclose(fd);
		if (strlen(buf) > 0 && ( \
			strstr(buf, "Local CLI is already recovered.") || \
			strstr(buf, "Recovering local CLI")))
			return TRUE;
	}
	return FALSE;
}

static BOOL AddLicense()
{
	char cmdLine[1024] = {0};
	FILE *fd = NULL;

	sprintf(cmdLine, "sadmin license add %s", DEF_LINUX_MCAFEE_LICENSE);
	fd = popen(cmdLine, "r");
	if (fd)
	{
		char buf[1024] = {0};
		int len = 0;
		fgets(buf, sizeof(buf)-1, fd);
		pclose(fd);
		len = strlen(buf);
		if (len == 0 || (len > 0 && strstr(buf, "\"Application Control\" license added.")))
			//if (len > 0 && strstr(buf, "\"Application Control\" license added."))
		{
			if (!HideLicense())
				printf("License hide failed!\n");
			return TRUE;
		}
	}
	return FALSE;
}

static BOOL IsHaveLicenses()
{
	char cmdLine[1024] = {0};
	FILE *fd = NULL;

	sprintf(cmdLine, "sadmin license list");
	fd = popen(cmdLine, "r");
	if (fd)
	{
		char buf[1024] = {0};
		fgets(buf, sizeof(buf)-1, fd);
		pclose(fd);
		if (strlen(buf) > 0 && strstr(buf, DEF_LINUX_MCAFEE_LICENSE))
			return TRUE;
	}
	return FALSE;
}

#endif /*_WIN32*/