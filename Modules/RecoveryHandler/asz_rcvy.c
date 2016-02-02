#include "RecoveryHandler.h"
#include "parser_rcvy.h"
#include "asz_rcvy.h"

//-----------------------------------------------------------------------------
// Local Types/Macros/Variables:
//-----------------------------------------------------------------------------
static BOOL IsHaveASZ = FALSE;
static char AcrDiskName[64] = {0};

//-----------------------------------------------------------------------------
// Local functions declare:
//-----------------------------------------------------------------------------
static BOOL AcrStartService();
static CAGENT_PTHREAD_ENTRY(CreateASZThreadStart, args);
static void WriteASZExistRecord(BOOL isExist);

//-----------------------------------------------------------------------------
// Global functions define:
//-----------------------------------------------------------------------------
BOOL GetVolume(char * volumeName)
{
#ifndef _is_linux
	BOOL bRet = FALSE;
	BOOL isFindCVolume = FALSE;
	//FILE *pOutput;

	char cmdLine[BUFSIZ] = "cmd.exe /c acrocmd list disks>c:\\GetVolumeOutput.txt";
	char outputPath[MAX_PATH] = "c:\\GetVolumeOutput.txt";

	if (NULL == volumeName)
		return bRet;

	/*pOutput = fopen(outputPath, "wb");
	if (NULL == pOutput)
	{
		RecoveryLog(g_loghandle, Normal, "%s()[%d]###Create %s failed!\n", __FUNCTION__, __LINE__, outputPath );
		return bRet;
	}
	fclose(pOutput);*/

	if(!app_os_CreateProcess(cmdLine))
	{
		RecoveryLog(g_loghandle, Normal, "%s()[%d]###Create acrocmd process failed!\n", __FUNCTION__, __LINE__ );
		return bRet;
	}

	{
		FILE *pOutputFile = NULL;
		char lineBuf[1024] = { 0 };
		IsHaveASZ = FALSE;
		pOutputFile = fopen(outputPath, "rb");
		if (pOutputFile)
		{
			int lineCnt = 0;
			while (!feof(pOutputFile))
			{
				memset(lineBuf, 0, sizeof(lineBuf));
				if (fgets(lineBuf, sizeof(lineBuf), pOutputFile))
				{
					if (!isFindCVolume)
					{
						if (strstr(lineBuf, "Disk"))
						{
							char * word[16] = { NULL };
							char *buf = lineBuf;
							int i = 0;
							while ((word[i] = strtok(buf, " ")) != NULL)
							{
								i++;
								buf = NULL;
							}
							if (word[1] != NULL)
							{
								strcpy(AcrDiskName, word[1]);
							}
							lineCnt = 0;
							continue;
						}

						if (strstr(lineBuf, "MB")) {
							lineCnt++;
						}

						if (strstr(lineBuf, "C:")) {
							sprintf(volumeName, "%s-%d", AcrDiskName, lineCnt);
							isFindCVolume = TRUE;
							bRet = TRUE;
						}
					}
					if (strstr(lineBuf, "ACRONIS SZ"))
						IsHaveASZ = TRUE;
				}
			}
			fclose(pOutputFile);
		}
	}
	app_os_file_remove(outputPath);
	return bRet;

#else
	BOOL bRet = FALSE;
	FILE *pOutput;
	char diskNumbers[12] = { 0 };
	char tmpBuf[BUFSIZ] = { 0 };
	char cmdLine[BUFSIZ] = "/usr/sbin/acrocmd list disks>/tmp/GetVolumeOutput.txt";
	char outputPath[MAX_PATH] = "/tmp/GetVolumeOutput.txt";

	if (NULL == volumeName)
		return bRet;

	pOutput = fopen(outputPath, "wb");
	if (NULL == pOutput)
	{
		RecoveryLog(g_loghandle, Normal, "%s()[%d]###Create %s failed!\n", __FUNCTION__, __LINE__, outputPath);
		return bRet;
	}
	fclose(pOutput);

	if (!app_os_CreateProcess(cmdLine))
	{
		RecoveryLog(g_loghandle, Normal, "%s()[%d]###Create acrocmd process failed!\n", __FUNCTION__, __LINE__);
		return bRet;
	}

	{
		FILE *pOutputFile = NULL;
		char lineBuf[1024] = { 0 };
		IsHaveASZ = FALSE;
		pOutputFile = fopen(outputPath, "rb");
		if (pOutputFile)
		{
			memset(volumeName, 0, sizeof(volumeName));
			char backupFolderDevName[256] = {0};
			{
				FILE *mtabFd = fopen("/etc/mtab", "r");
				while(fgets(lineBuf, sizeof(lineBuf), mtabFd))
				{
					if (strstr(lineBuf, backupFolder))
					{
						strcpy(backupFolderDevName, strrchr(strtok(lineBuf, " "), '/')+1);
						break;
					}
				}
				fclose(mtabFd);
			}
			Recovery_debug_print("backupFolderDevName: %s", backupFolderDevName);
			while (fgets(lineBuf, sizeof(lineBuf), pOutputFile))
			{
				if (strstr(lineBuf, "Ext") && !strstr(lineBuf, backupFolderDevName))// Get volume Numebers and disk Numebers
				{
					BOOL isExists = FALSE;
					int num, i;
					char *tp = strtok(lineBuf, " ");// Get volume numbers,like '1-2'
					strncat(volumeName, tp, strlen(tp));
					strncat(volumeName, ",", 1);
					num = *tp;
					if (num >= '1' && num <= '9')// Presume one computer has <= 9 harddisks
					{
						for (i = 0; diskNumbers[i] != 0 && i < 9; i++)
						{
							if (num == diskNumbers[i])
							{
								isExists = TRUE;
								break;
							}
						}
						if (!isExists)
							diskNumbers[i] = num;
					}
				}
				else if (strstr(lineBuf, "Linux Swap"))
				{
					strcpy(tmpBuf, strtok(lineBuf, " "));
				}
				else if (strstr(lineBuf, "ACRONIS SZ"))
				{
					IsHaveASZ = TRUE;
				}
			}
			fclose(pOutputFile);

			if (strlen(tmpBuf) > 0)	// add linux swap partition
				strncat(volumeName, tmpBuf, strlen(tmpBuf));
			else
			{
				char *tp = strrchr(AcrDiskName, ',');
				if (tp)
					*tp = '\0';		// delete last character ','
			}

			{	// write disk numebers into the global value 'AcrDiskName'
				int i;
				*AcrDiskName = '\0';
				for (i = 0; diskNumbers[i] != 0; i++)
				{
					strncat(AcrDiskName, diskNumbers + i, 1);
					strncat(AcrDiskName, ",", 1);
				}
				if (i > 0)
				{
					char *tp = strrchr(AcrDiskName, ',');
					if (tp)
						*tp = '\0';		// delete last character ','
					bRet = TRUE;
				}
			}
		}
	}

	Recovery_debug_print("IsHaveASZ=%d, AcrDiskName=%s,volumeName=%s", IsHaveASZ, AcrDiskName, volumeName);
	return bRet;
#endif /* _is_linux */
}

#ifndef _is_linux
void CreateASZ(char * aszParams)
{
	recovery_handler_context_t *pRecoveryHandlerContext = (recovery_handler_context_t *)&RecoveryHandlerContext;
	char createASZMsg[BUFSIZ] = {0};
	char aszParamsStr[BUFSIZ] = {0};
	char * word[16] = {NULL};
	if(NULL == aszParams) return;
	strcpy(aszParamsStr, aszParams);
	if(strstr(aszParamsStr, ","))
	{
		char *buf = aszParamsStr;
		int i = 0, j = 0;
		while((word[i] = strtok( buf, ",")) != NULL)
		{
			i++;
			buf=NULL;
		}
		for(j = 0; j <= i; j++)
		{
			TrimStr(word[i]);
		}
	}
	if(!pRecoveryHandlerContext->isCreateASZRunning)
	{
		pRecoveryHandlerContext->isCreateASZRunning = TRUE;
		if (app_os_thread_create(&pRecoveryHandlerContext->acrCreateASZThreadHandle, CreateASZThreadStart, word) != 0)
		{
			RecoveryLog(g_loghandle, Normal, "%s()[%d]###Start create ASZ thread failed!\n",__FUNCTION__, __LINE__ );
			sprintf(createASZMsg, "%s,%s",  "ASZInfo", "Create ASZ thread failed!");			
			SendReplyMsg_Rcvy(createASZMsg, asz_threadFail, rcvy_create_asz_rep);
			pRecoveryHandlerContext->isCreateASZRunning = FALSE;
		}
		else
		{
			sprintf(createASZMsg, "%s,%s",  "ASZInfo", "Creating ASZ...");
			SendReplyMsg_Rcvy(createASZMsg, asz_creating, rcvy_create_asz_rep);
		}
	}
}

BOOL WriteASZPercentRegistry(int aszPersent)
{
	BOOL bRet = FALSE;
	HKEY hk;
	char regName[] = "SOFTWARE\\AdvantechAcronis";
	if(ERROR_SUCCESS == app_os_RegOpenKeyExA(HKEY_LOCAL_MACHINE, regName, 0, KEY_ALL_ACCESS, &hk))
	{
		char aszPersentStr[5] = {0};
		app_os_itoa(aszPersent, aszPersentStr, 10);
		app_os_RegSetValueExA(hk, "ASZPercent", 0, REG_SZ, aszPersentStr, strlen(aszPersentStr)+1);
		app_os_RegCloseKey(hk);
		bRet = TRUE;
	}
	return bRet;
}

BOOL ASZCreateParamCheck(char * aszParams)
{
	BOOL bRet = FALSE;
	if(aszParams == NULL || strlen(aszParams)<=0) return bRet;
	{
		char tmpaszParams[128] = {0};
		char * word[16] = {NULL};
		strcpy(tmpaszParams, aszParams);
		if(strstr(tmpaszParams, ","))
		{
			char *buf = tmpaszParams;
			int i = 0, j = 0;
			while((word[i] = strtok(buf, ",")) != NULL)
			{
				i++;
				buf=NULL;
			}
			if(i==2)
			{
				for(j = 0; j <= i; j++)
				{
					TrimStr(word[i]);
				}
				if(!_stricmp(word[0], "FALSE") || !_stricmp(word[0], "TRUE"))
				{
					char * pPercentStr = word[1];
					int tmpPercent = 0;
					BOOL isDigit = TRUE;
					char *p = pPercentStr;
					while(*p != '\0')
					{
						if(*p < '0' || *p++ > '9')
						{
							isDigit = FALSE;
							break;
						}
					}
					if(isDigit)
					{
						tmpPercent = atoi(pPercentStr);
						if(tmpPercent>=0 && tmpPercent<=100)
						{
							bRet = TRUE;
						}
					}
				}
			}
		}
	}
	return bRet;
}

BOOL GetASZExistRecord(char* existStatus)
{
#ifndef _is_linux
	BOOL bRet = FALSE;
	HKEY hk;
	char regName[] = "SOFTWARE\\AdvantechAcronis";
	char valueName[] = "ASZExist";
	char valueStr[128] = {0};
	int  valueDataSize = sizeof(valueStr);
	if(NULL == existStatus) return bRet;
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
	strcpy(existStatus, valueStr);
	bRet = TRUE;
	return bRet;
#else
	BOOL bRet = FALSE;
	char valueStr[128] = {0};
	if(existStatus) // is not NULL
	{
		FILE *fd = fopen("/usr/lib/Acronis/ASZStatus.txt", "r");
		if(fd)
		{
			fgets(valueStr, sizeof(valueStr), fd);
			strtok(valueStr, ":");
			strcpy(existStatus, strtok(NULL, ":"));
			bRet = TRUE;
			fclose(fd);
			Recovery_debug_print("Get ASZ status success!");
		}
	}
	return bRet;
#endif /* _is_linux */
}

void CheckASZStatus()
{
	char volumeName[4096] = {0};
	if(GetVolume(volumeName))
	{
		WriteASZExistRecord(IsHaveASZ);
	}
}



#else

LONGLONG GetFolderAvailSpace(char *path)
{
	/*
	* return:  0 - LONGLONG_MAX : Is available space(unit:Mb).
	* 			-1 : Error.
	*/
	if (path)
	{
		char buf[BUFSIZ] = {0};
		FILE * fd = NULL;

		sprintf(buf, "df -m \'%s\'  | awk \'{ print $4 }\' ", path);
		fd = popen(buf, "r");
		if (fd)
		{
			fgets(buf, sizeof(buf), fd);//throw first line
			fgets(buf, sizeof(buf), fd);
			pclose(fd);
			return atoi(buf);
		}
	}
	return -1;
}

LONGLONG GetBackupNeedSpace(void)
{
	/*
	* return:  0 - LONGLONG_MAX : Is backup need space(unit:Mb).
	* 			-1 : Error.
	*/
	char buf[BUFSIZ] = {0};
	FILE * fd = NULL;
	LONGLONG existFileSizeMax = 0;

	sprintf(buf, "du -m %s/*.TIB 2>/dev/null | awk \'{ print $1 }\' ", backupFolder);
	fd = popen(buf, "r");
	if (fd)//find the biggest of the '*.TIB' file size
	{
		int lastNumLen = 0, tmp = 0;
		while (fgets(buf, sizeof(buf), fd))
		{
			tmp = strlen(buf);
			if (lastNumLen < tmp)
			{
				existFileSizeMax = atoi(buf);
				lastNumLen = tmp;
			}
			else if (lastNumLen == tmp)
			{
				tmp = atoi(buf);
				if (existFileSizeMax < tmp)
				{
					existFileSizeMax = tmp;
				}
			}
		}
		pclose(fd);
	}
	if (existFileSizeMax > 0)
	{
		return existFileSizeMax;
	}
	else
	{
		LONGLONG spaceMin, spaceMax;
		if (app_os_GetMinAndMaxSpaceMB(&spaceMin, &spaceMax))
			return spaceMin;
		else
			return -1;
	}
}

#endif /*_is_linux*/


//-----------------------------------------------------------------------------
// Local functions define:
//-----------------------------------------------------------------------------

static void WriteASZExistRecord(BOOL isExist)
{
#ifndef _is_linux
	HKEY hk;
	char regName[] = "SOFTWARE\\AdvantechAcronis";
	if(ERROR_SUCCESS == app_os_RegOpenKeyExA(HKEY_LOCAL_MACHINE, regName, 0, KEY_ALL_ACCESS, &hk))
	{
		if(isExist)
		{
			app_os_RegSetValueExA(hk, "ASZExist", 0, REG_SZ, "True", sizeof("True"));
		}
		else
		{
			app_os_RegSetValueExA(hk, "ASZExist", 0, REG_SZ, "False", sizeof("False"));
		}

		app_os_RegCloseKey(hk);
	}
#else
	Recovery_debug_print("isExist=%d", isExist);
	FILE *fd = fopen("/usr/lib/Acronis/ASZStatus.txt", "w");
	if(fd) // is not NULL
	{
		if(isExist)
		{
			fprintf(fd, "ASZExist:True");
		}
		else
		{
			fprintf(fd, "ASZExist:False");
		}
		fclose(fd);
		Recovery_debug_print("write success");
	}
	else
	{
		Recovery_debug_print("write failed");
	}
#endif /* _is_linux */
}

static BOOL AcrStartService()
{
#ifndef _is_linux
	BOOL bRet = FALSE;
	DWORD dwRet = 0;
	int i = 0;
	for(i=0; i<5; i++)
	{
		dwRet = app_os_StartSrv(DEF_ACRONIS_SERVICE_NAME);
		if(dwRet == SERVICE_RUNNING)
		{
			bRet = TRUE;
			break;
		}
	}
	return bRet;
#else
	return TRUE;
#endif  _is_linux
}

static CAGENT_PTHREAD_ENTRY(CreateASZThreadStart, args)
{
	char ** aszParams = (char **)args;
	char * pIsDefSetting = aszParams[0];
	char * pASZPercentStr = aszParams[1];
	unsigned int ASZPercent = atoi(pASZPercentStr);
	create_asz_status_t createASZStatus = create_asz_success;
	char cmdLine[BUFSIZ] = {0};

	if(app_os_is_file_exist(AcroCmdExePath))
	{
		if(!AcrStartService())
		{
			createASZStatus = create_asz_service_fail;
			goto done;
		}

		{
			char aszVolumeName[4096] = {0};
			int i=0;

			for (i = 0; i < 10; i++)
			{
				memset(aszVolumeName, 0, sizeof(aszVolumeName));
				GetVolume(aszVolumeName);
				if(strlen(aszVolumeName))
				{
					break;
				}
				app_os_sleep(10000);
			}

			if (strlen(aszVolumeName) == 0)
			{
				createASZStatus = create_asz_get_voluem_fail;
				goto done;
			}

			if (IsHaveASZ)
			{
				createASZStatus = create_asz_exist;
				WriteASZExistRecord(TRUE);
				goto done;
			}

#ifdef _is_linux
			{
				FILE *fd = NULL;
				char buf[BUFSIZ] = {0};
				fd = popen("swapon -s", "r");
				while (fgets(buf, sizeof(buf), fd))
				{
					if (strstr(buf, "partition"))// delete swap partition
					{
						char *tp = aszVolumeName;
						while (*tp++ != '\0');
						while (*--tp != ',');
						*tp = '\0';
						break;
					}
				}
				pclose(fd);
			}
			if (!_stricmp(pIsDefSetting, "TRUE"))
			{
				sprintf(cmdLine, "acrocmd create asz --disk=%s --volume=%s --reboot", AcrDiskName, aszVolumeName);
			}
			else
			{
				LONGLONG ASZSize = 0;
				LONGLONG MinSize = 0, MaxSize = 0;
				app_os_GetMinAndMaxSpaceMB(&MinSize, &MaxSize);
				ASZSize = MaxSize * ASZPercent / 100;
				if (ASZSize <= MinSize)	ASZSize = MinSize;
				sprintf(cmdLine, "acrocmd create asz --disk=%s --volume=%s --asz_size=%d --reboot", AcrDiskName, aszVolumeName, ASZSize);
			}
#else

			if(!_stricmp(pIsDefSetting, "TRUE"))
			{
				sprintf(cmdLine, "cmd.exe /c acrocmd.exe create asz --disk=%s --volume=%s --reboot", AcrDiskName, aszVolumeName);
			}
			else
			{
				LONGLONG ASZSize = 0;
				LONGLONG MinSize = 0, MaxSize = 0;
				app_os_GetMinAndMaxSpaceMB(&MinSize, &MaxSize);
				ASZSize = MaxSize * ASZPercent / 100;
				if (ASZSize <= MinSize) ASZSize = MinSize;
				sprintf(cmdLine, "cmd.exe /c acrocmd.exe create asz --disk=%s --volume=%s --asz_size=%d --reboot", AcrDiskName, aszVolumeName, ASZSize);
			}
#endif /* _is_linux */

			if(!app_os_CreateProcess(cmdLine))
			{
				RecoveryLog(g_loghandle, Normal, "%s()[%d]###Create asz process failed!\n",__FUNCTION__, __LINE__);
				createASZStatus = create_asz_other_error;
				goto done;
			}
			else
			{
				WriteASZExistRecord(TRUE);
#ifdef _is_linux
				createASZStatus = create_asz_success;
#else
				{
					char modulePath[MAX_PATH] = {0};
					char acrCloseCmdPath[MAX_PATH] = {0};
					app_os_get_module_path(modulePath);
					sprintf(acrCloseCmdPath, "%s%s", modulePath, DEF_CLOSE_ACROCMD_NAME);

					memset(cmdLine, 0, sizeof(cmdLine));
					sprintf(cmdLine, "%s", acrCloseCmdPath);
					if(!app_os_CreateProcess(cmdLine))
					{
						RecoveryLog(g_loghandle, Normal, "%s()[%d]###Create asz close process failed!\n",__FUNCTION__, __LINE__ );
						createASZStatus = create_asz_other_error;
						goto done;
					}
					else
					{
						createASZStatus = create_asz_success;
					}
				}
#endif /* _is_linux */
			}
		}
	}
	else
	{
		createASZStatus = create_asz_no_install;
	}
done:
	{
		char createASZMsg[BUFSIZ] = {0};
		switch(createASZStatus)
		{
		case create_asz_service_fail:
			{
				sprintf(createASZMsg, "%s", "create_asz_service_fail");
				SendReplyMsg_Rcvy(createASZMsg, asz_srvFail, rcvy_create_asz_rep);
				break;
			}
		case create_asz_get_voluem_fail:
			{
				sprintf(createASZMsg, "%s", "create_asz_get_voluem_fail");
				SendReplyMsg_Rcvy(createASZMsg, asz_volumeFail, rcvy_create_asz_rep);
				break;
			}
		case create_asz_exist:
			{
				sprintf(createASZMsg, "%s", "create_asz_exist");
				SendReplyMsg_Rcvy(createASZMsg, asz_exist, rcvy_create_asz_rep);
				break;
			}
		case create_asz_no_install:
			{
				sprintf(createASZMsg, "%s", "create_asz_no_install");
				SendReplyMsg_Rcvy(createASZMsg, asz_noAcronis, rcvy_create_asz_rep);
				break;
			}
		case create_asz_success:
			{
				sprintf(createASZMsg, "%s", "create_asz_success");
				SendReplyMsg_Rcvy(createASZMsg, oprt_success, rcvy_create_asz_rep);
				break;
			}
		case create_asz_other_error:
			{
				sprintf(createASZMsg, "%s", "create_asz_other_error");
				SendReplyMsg_Rcvy(createASZMsg, oprt_fail, rcvy_create_asz_rep);
				break;
			}
		}
	}
	GetRcvyCurStatus();

	if(createASZStatus == create_asz_success)
	{
		memset(cmdLine, 0, sizeof(cmdLine));
#ifndef _is_linux
		sprintf(cmdLine, "%s", "cmd.exe /c shutdown -r -f -t 0");
#else
		sprintf(cmdLine, "%s", "reboot");
#endif /* _is_linux */
		if(!app_os_CreateProcess(cmdLine))
		{
			RecoveryLog(g_loghandle, Normal, "%s()[%d]###Create shut down process failed!\n",__FUNCTION__, __LINE__ );
		}

	}
	return 0;
}







//static LONGLONG GetUnallocatedSpace()
//{
//	LONGLONG lRet = -1;
//	char cmdLine[BUFSIZ] = "cmd.exe /c acrocmd list disks";
//	char outputPath[MAX_PATH] = "c:\\GetVolumeOutput.txt";
//	FILE *pOutput;
//	BOOL bRet = FALSE;
//
//	pOutput = fopen(outputPath, "wb");
//	if (NULL == pOutput)
//	{
//		RecoveryLog(g_loghandle, Normal, "%s()[%d]###Create %s failed!\n",__FUNCTION__, __LINE__, outputPath );
//		return bRet;
//	}
//	fclose(pOutput);
//
//	if(!app_os_CreateProcess(cmdLine))
//	{
//		RecoveryLog(g_loghandle, Normal, "%s()[%d]###Create acrocmd process failed!\n",__FUNCTION__, __LINE__ );
//		return lRet;
//	}
//
//	{
//		FILE *pOutputFile = NULL;
//		char lineBuf[1024] = {0};
//		pOutputFile = fopen (outputPath, "rb");
//		if(pOutputFile)
//		{
//			int count = 0;
//			while (!feof(pOutputFile))
//			{
//				memset(lineBuf, 0, sizeof(lineBuf));
//				if(fgets(lineBuf, sizeof(lineBuf), pOutputFile))
//				{
//					if(strstr(lineBuf, "Unallocated-1-1"))
//					{
//						char * word[16] = {NULL};
//						char *buf = lineBuf;
//						int i = 0, j = 0;
//						while((word[i] = strtok( buf, " ")) != NULL)
//						{
//							i++;
//							buf=NULL;
//						}
//						for(j = 0; j <= i; j++)
//						{
//							if(!strcmp(word[j], "MB"))
//							{
//								if(1 == count)
//								{
//									if(strstr(word[j-1], ","))
//									{
//										{
//											char unSpaceStr[128] = {0};
//											char * spaceWord[16] = {NULL};
//											char * spaceBuf = NULL;
//											int k = 0;
//											strcpy(unSpaceStr, word[j-1]);
//											spaceBuf = unSpaceStr;
//											while((spaceWord[k] = strtok( spaceBuf, ",")) != NULL)
//											{
//												k++;
//												spaceBuf=NULL;
//											}
//											if(spaceWord[0] != NULL)
//											{
//												lRet = atoi(spaceWord[0]);
//											}
//										}
//										break;
//									}
//								}
//								count ++;
//							}
//						}
//					}
//				}
//			}
//			fclose(pOutputFile);
//		}
//	}
//
//	app_os_file_remove(outputPath);
//
//	return lRet;
//}
