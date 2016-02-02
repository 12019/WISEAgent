
#ifdef _WIN32

#include "RecoveryHandler.h"
#include "FtpDownload.h"
#include "md5.h"
#include "parser_rcvy.h"
#include "status_rcvy.h"
#include "install_update_rcvy.h"
#include "public_rcvy.h"

//-----------------------------------------------------------------------------
// Local Types/Macros/Variables:
//-----------------------------------------------------------------------------
static BOOL AcrInstallThreadRunning;
static CAGENT_THREAD_TYPE AcrInstallThreadHandle;
static BOOL IsAcrInstallerDLMonThreadRunning = FALSE;
static char agentConfigFilePath[MAX_PATH] = {0};

//-----------------------------------------------------------------------------
// Local functions declare:
//-----------------------------------------------------------------------------
static BOOL GetFileMD5(char * filePath, char * retMD5Str);
static BOOL CheckOtherAcronis();
static cagent_callback_status_t cagent_get_server_ip(OUT char **server_ip, OUT size_t *size);
static void SetLastWarningMsg(char * warningMsg);
static void SetActionMsg(char * actionMsg);
static BOOL AcrToInstall(EINSTALLTYPE installType);


//-----------------------------------------------------------------------------
// Global functions define:
//-----------------------------------------------------------------------------
BOOL AcrInstallMsgSend(EINSTALLTYPE installType, char * msg, rcvy_reply_status_code statusCode)
{
	BOOL bRet = FALSE;
	if(msg == NULL) return bRet;
	switch(installType)
	{
	case ITT_INSTALL:
		{
			SendReplyMsg_Rcvy(msg, statusCode, rcvy_install_rep);
			bRet = TRUE;
			break;
		}
	case ITT_UPDATE:
		{
			SendReplyMsg_Rcvy(msg, statusCode, rcvy_update_rep);
			bRet = TRUE;
			break;
		}
	default: break;
	}
	return bRet;
}

static BOOL GetFileMD5(char * filePath, char * retMD5Str)
{
#define  DEF_PER_MD5_DATA_SIZE       512
	BOOL bRet = FALSE;
	if(NULL == filePath || NULL == retMD5Str) return bRet;
	{
		FILE *fptr = NULL;
		fptr = fopen(filePath, "rb");
		if(fptr)
		{
			MD5_CTX context;
			unsigned char retMD5[DEF_MD5_SIZE] = {0};
			char dataBuf[DEF_PER_MD5_DATA_SIZE] = {0};
			unsigned int readLen = 0;
			unsigned int realReadLen = 0;
			MD5Init(&context);
			readLen = sizeof(dataBuf);
			while ((realReadLen = fread(dataBuf, sizeof(char), readLen, fptr)) != 0)
			{
				MD5Update(&context, dataBuf, realReadLen);
				memset(dataBuf, 0, sizeof(dataBuf));
				realReadLen = 0;
				readLen = sizeof(dataBuf);
			}
			MD5Final(retMD5, &context);

			{
				char md5str0x[DEF_MD5_SIZE*2+1] = {0};
				int i = 0;
				for(i = 0; i<DEF_MD5_SIZE; i++)
				{
					sprintf(&md5str0x[i*2], "%.2x", retMD5[i]);
				}
				strcpy(retMD5Str, md5str0x);
				bRet = TRUE;
			}
			fclose(fptr);
		}
	}
	return bRet;
}

static CAGENT_PTHREAD_ENTRY(AcrInstallerDLMonThreadStart, args)
{
	if(args != NULL)
	{
		rcvy_dl_mon_params_t rcvyDLMonParams;
		HFTPDL hfdHandle = NULL;
		FTPDLSTATUS ftpDlStatus = FTP_DSC_UNKNOWN;
		EINSTALLTYPE installType = ITT_UNKNOWN;
		BOOL isBreak = FALSE;
		int iRet = 0, i = 0;
		memset(&rcvyDLMonParams, 0, sizeof(rcvy_dl_mon_params_t));
		memcpy(&rcvyDLMonParams, args, sizeof(rcvy_dl_mon_params_t));
		hfdHandle = rcvyDLMonParams.hfdHandle;
		installType = rcvyDLMonParams.installType;
		app_os_sleep(10);
		while(IsAcrInstallerDLMonThreadRunning)
		{
			iRet = FTPDownloadGetStatus(hfdHandle, &ftpDlStatus);
			if(iRet == 0)
			{
				switch(ftpDlStatus)
				{
				case FTP_DSC_START:
					{
						SetActionMsg("File downloader start!");
						AcrInstallMsgSend(installType, "File downloader start!", istl_updt_dwlodStart);
						for(i = 0; i<5 && IsAcrInstallerDLMonThreadRunning; i++) app_os_sleep(100);
						break;
					}
				case FTP_DSC_DOWNLOADING:
					{
						char downloadDetial[128] = {0};
						DWORD dPercent = 0;
						float dSpeed = 0;
						FTPDownloadGetPersent(hfdHandle, &dPercent);
						FtpDownloadGetSpeedKBS(hfdHandle, &dSpeed);
						sprintf(downloadDetial,"Downloading,download Persent:%d%%, Speed: %4.2fKB/S\n", dPercent, dSpeed);
						SetActionMsg(downloadDetial);
						AcrInstallMsgSend(installType, downloadDetial, istl_updt_dwlodProgress);
						for(i = 0; i<10 && IsAcrInstallerDLMonThreadRunning; i++) app_os_sleep(100);
						break;
					}
				case FTP_DSC_FINISHED:
					{
						char downloadDetial[128] = {0};
						DWORD dPercent = 0;
						float dSpeed = 0;
						FTPDownloadGetPersent(hfdHandle, &dPercent);
						FtpDownloadGetSpeedKBS(hfdHandle, &dSpeed);
						sprintf(downloadDetial,"Download Finished,Download Persent:%d%%, Speed: %4.2fKB/S\n", dPercent, dSpeed);
						SetActionMsg(downloadDetial);
						AcrInstallMsgSend(installType, downloadDetial, istl_updt_dwlodEnd);
						isBreak = TRUE;
						break;
					}
				case FTP_DSC_ERROR:
					{
						char lastMsgTmp[1024] = {0};
						char lastDownloaderErrorMsg[512] = {0};
						FTPDownLoadGetLastError(hfdHandle, lastDownloaderErrorMsg, sizeof(lastDownloaderErrorMsg));
						sprintf(lastMsgTmp,"File downloader status error!Error msg:%s\n", lastDownloaderErrorMsg);
						SetLastWarningMsg(lastMsgTmp);
						AcrInstallMsgSend(installType, lastMsgTmp, istl_updt_dwlodErr);
						IsDownloadAction = FALSE;
						isBreak = TRUE;
						break;
					}
					/* Recovery add */
				default :
					isBreak = FALSE;
					break;
					/* Recovery add */
				}
			}
			if(isBreak) break;
			app_os_sleep(10);
		}
	}
	IsAcrInstallerDLMonThreadRunning = FALSE;
	return 0;
}

static CAGENT_PTHREAD_ENTRY(AcrInstallThreadStart, args)
{
	BOOL bFlag = TRUE, bDLOK= FALSE, isNeedDownload = TRUE;
	int iRet = 0;
	HFTPDL   hfdHandle = NULL;
	BOOL * actionFlag  = NULL;
	EINSTALLTYPE installType = ITT_UNKNOWN;
	recovery_install_params dlParams;
	if(args == NULL) goto done0;
	memset(&dlParams, 0, sizeof(recovery_install_params));
	memcpy(&dlParams, (recovery_install_params*)args, sizeof(recovery_install_params));
	installType = dlParams.installType;
	switch(installType)
	{
	case ITT_INSTALL:
		{
			actionFlag = &IsInstallAction;
			break;
		}
	case ITT_UPDATE:
		{
			actionFlag = &IsUpdateAction;
			break;
		}
	default:
		{
			break;
		}
	}
	if(actionFlag == NULL) goto done0;
	*actionFlag = TRUE;

	if(app_os_is_file_exist(AcrInstallerPath))
	{
		char md5Str[64] = {0};
		//check md5
		if(GetFileMD5(AcrInstallerPath, md5Str))
		{
			if(!_stricmp(md5Str, dlParams.md5))
			{
				isNeedDownload = FALSE;
			}
		}
	}
	if(isNeedDownload)
	{
		CAGENT_THREAD_TYPE AcrInstallerDLMonThreadHandle;
		char installerFileUrl[MAX_PATH*2] = {0};
		char * serverIP = NULL;
		int serverIPLen = 0;
		cagent_get_server_ip(&serverIP, &serverIPLen);
		if(serverIP == NULL || serverIPLen <= 0)
		{
			SetLastWarningMsg("Get server ip failed!");
			AcrInstallMsgSend(installType, "Get server ip failed!", istl_updt_ipFail);
			goto done1;
		}
		else
		{
			hfdHandle = FtpDownloadInit();
			if(hfdHandle == NULL)
			{
				SetLastWarningMsg("File downloader initialize failed!");
				AcrInstallMsgSend(installType, "File downloader initialize failed!", istl_updt_dwlodInitFail);
				goto done1;
			}
			else
			{
				rcvy_dl_mon_params_t dlMonParams;
				memset(&dlMonParams, 0, sizeof(rcvy_dl_mon_params_t));
				dlMonParams.hfdHandle = hfdHandle;
				dlMonParams.installType = installType;
				if(strlen(dlParams.ftpuserName) && strlen(dlParams.ftpPassword))
				{
					sprintf_s(installerFileUrl, sizeof(installerFileUrl), "ftp://%s:%s@%s:%d%s", dlParams.ftpuserName, dlParams.ftpPassword,
						serverIP, dlParams.port, dlParams.installerPath);
				}
				else
				{
					sprintf_s(installerFileUrl, sizeof(installerFileUrl), "ftp://%s:%d%s", serverIP, dlParams.port, dlParams.installerPath);
				}

				IsAcrInstallerDLMonThreadRunning = TRUE;
				if(app_os_thread_create(&AcrInstallerDLMonThreadHandle, AcrInstallerDLMonThreadStart, &dlMonParams) != 0)
				{
					IsAcrInstallerDLMonThreadRunning = FALSE;
					SetLastWarningMsg("Acronis installer download monitor start failed!");
					AcrInstallMsgSend(ITT_INSTALL, "Acronis installer download monitor start failed!", istl_updt_dwlodStartFail);
					goto done2;
				}
				iRet = FtpDownload(hfdHandle, installerFileUrl, AcrInstallerPath);
				app_os_sleep(1000);
				while(AcrInstallThreadRunning)
				{
					FTPDLSTATUS dlStatus = FTP_DSC_UNKNOWN;
					FTPDownloadGetStatus(hfdHandle, &dlStatus);
					if(dlStatus == FTP_DSC_ERROR)
					{
						bDLOK = FALSE;
						break;
					}
					if(dlStatus == FTP_DSC_FINISHED)
					{
						app_os_WaitForSingleObject(AcrInstallerDLMonThreadHandle, INFINITE);
						if(AcrInstallThreadRunning)
						{
							char md5Str[64] = {0};
							BOOL bRet = FALSE;
							//check md5
							if(GetFileMD5(AcrInstallerPath, md5Str))
							{
								if(!_stricmp(md5Str, dlParams.md5))
								{
									bDLOK = TRUE;
								}
								else
								{
									if(!bRet)
									{
										SetLastWarningMsg("Check md5 error!");
										AcrInstallMsgSend(ITT_INSTALL, "Check md5 error!", istl_updt_md5Err);
									}
								}
							}
						}
						break;
					}
					app_os_sleep(10);
				}

				if(!AcrInstallThreadRunning)
				{
					IsAcrInstallerDLMonThreadRunning = FALSE;
					goto done2;
				}
			}
		}
	}
	else bDLOK = TRUE;

	if(!bDLOK) goto done2;
	else
	{
		IsDownloadAction = FALSE;
		AcrToInstall(installType);
	}
done2:
	if(hfdHandle)
	{
		FtpDownloadCleanup(hfdHandle);
		hfdHandle = NULL;
	}
done1:
	*actionFlag = FALSE;
done0:
	AcrInstallThreadRunning = FALSE;
	AcrInstallThreadHandle = NULL;
	return 0;
}

void InstallAction(recovery_install_params * pRcvyInstallParams)
{
	if(pRcvyInstallParams == NULL) return;
	{
		EINSTALLTYPE curInstallType = pRcvyInstallParams->installType;
		if(curInstallType == ITT_INSTALL && app_os_is_file_exist(AcroCmdExePath))
		{
			SetLastWarningMsg("Acronis already exist, don't need install!");
			AcrInstallMsgSend(curInstallType, "Acronis already exist, don't need install!", istl_exist);
		}
		else
		{
			if(!IsInstallAction && !IsUpdateAction)
			{
				AcrInstallThreadHandle = NULL;
				AcrInstallThreadRunning = TRUE;
				AcrInstallMsgSend(curInstallType, "Install action start", istl_updt_istlActnStart);
				if(app_os_thread_create(&AcrInstallThreadHandle, AcrInstallThreadStart, pRcvyInstallParams) != 0)
				{
					AcrInstallThreadRunning = FALSE;
					SetLastWarningMsg("Install action start failed!");
					AcrInstallMsgSend(curInstallType, "Install action start failed!", istl_updt_startFail);
				}
				app_os_sleep(10);
			}
			else
			{
				if(IsInstallAction)
				{
					SetLastWarningMsg("Acronis is installing, this operation is invalid!");
					AcrInstallMsgSend(curInstallType, "Acronis is installing, this operation is invalid!", istl_updt_istlBusy);
				}
				if(IsUpdateAction)
				{
					SetLastWarningMsg("Acronis is updating, this operation is invalid!");
					AcrInstallMsgSend(curInstallType, "Acronis is updating, this operation is invalid!", istl_updt_updtBusy);
				}
			}
		}
	}
}

void UpdateAction(recovery_install_params * pRcvyInstallParams)
{
	if(pRcvyInstallParams == NULL) return;
	{
		EINSTALLTYPE curInstallType = pRcvyInstallParams->installType;
		if(!IsInstallAction && !IsUpdateAction)
		{
			if(IsAcrocmdRunning())
			{
				SetLastWarningMsg("Acrocmd running, Please wait for some time and then update!");
				AcrInstallMsgSend(curInstallType, "Acrocmd running, Please wait for some time and then update!", updt_AcrocmdBusy);
			}
			else
			{
				InstallAction(pRcvyInstallParams);
			}
		}
		else
		{
			if(IsInstallAction)
			{
				SetLastWarningMsg("Acronis is installing, this operation is invalid!");
				AcrInstallMsgSend(curInstallType, "Acronis is installing, this operation is invalid!", istl_updt_istlBusy);
			}
			if(IsUpdateAction)
			{
				SetLastWarningMsg("Acronis is updating, this operation is invalid!");
				AcrInstallMsgSend(curInstallType, "Acronis is updating, this operation is invalid!", istl_updt_updtBusy);
			}
		}
	}
}

BOOL GetAcrLatestActionFromReg(char * action)
{
	BOOL bRet = FALSE;
	if(action == NULL) return bRet;
	{
		HKEY hk;
		char regName[] = "SOFTWARE\\AdvantechAcronis";
		if(ERROR_SUCCESS == app_os_RegOpenKeyExA(HKEY_LOCAL_MACHINE, regName, 0, KEY_READ, &hk))
		{
			char valueName[] = "AcrLatestAction";
			char actionTmp[32] = {0};
			int  valueDataSize = sizeof(actionTmp);
			if(ERROR_SUCCESS == app_os_RegQueryValueExA(hk, valueName, 0, NULL, actionTmp, &valueDataSize))
			{
				strcpy(action, actionTmp);
				bRet = TRUE;
			}

			app_os_RegCloseKey(hk);
		}
	}
	return bRet;
}

BOOL SetAcrLatestActionToReg(char * action)
{
	BOOL bRet = FALSE;
	if(action == NULL) return bRet;
	{
		HKEY hk;
		char regName[] = "SOFTWARE\\AdvantechAcronis";
		if(ERROR_SUCCESS == app_os_RegOpenKeyExA(HKEY_LOCAL_MACHINE, regName, 0, KEY_ALL_ACCESS, &hk))
		{
			char valueName[] = "AcrLatestAction";
			app_os_RegSetValueExA(hk, valueName, 0, REG_SZ, action, strlen(action)+1);
			app_os_RegCloseKey(hk);
			bRet = TRUE;
		}
	}
	return bRet;
}

BOOL IsInstallerRunning()
{
	return app_os_ProcessCheck(DEF_ACRONIS_INSTALLER_NAME);
}



//-----------------------------------------------------------------------------
// Local functions define:
//-----------------------------------------------------------------------------
static BOOL CheckOtherAcronis()
{
	return app_os_is_file_exist(OtherAcronisPath);
}

static cagent_callback_status_t cagent_get_server_ip(OUT char **server_ip, OUT size_t *size)
{
	cagent_callback_status_t status = cagent_callback_continue;
#if 0
	/** fill in server ip */
	if(strlen(SERVER_IP) == 0)
	{
		if(cfg_get(agentConfigFilePath, SERVER_IP_KEY, SERVER_IP, sizeof(SERVER_IP)) <= 0)
		{
			printf("%s() [%d] Get server ip error!\n", __FUNCTION__, __LINE__);
			return cagent_callback_abort;
		}
	}
#endif
	*server_ip = SERVER_IP;
	*size = strlen(SERVER_IP);
	return status;
}

static void SetLastWarningMsg(char * warningMsg)
{
	if(warningMsg == NULL) return;
	{
		app_os_mutex_lock(&CSWMsg);
		memset(LastWarningMsg, 0, sizeof(LastWarningMsg));
		sprintf(LastWarningMsg, "%s", warningMsg);
		app_os_mutex_unlock(&CSWMsg);
	}
}

static void SetActionMsg(char * actionMsg)
{
	if(actionMsg == NULL) return;
	{
		app_os_mutex_lock(&CSAMsg);
		memset(ActionMsg, 0, sizeof(LastWarningMsg));
		sprintf(ActionMsg, "%s", actionMsg);
		app_os_mutex_unlock(&CSAMsg);
	}
}

static BOOL AcrToInstall(EINSTALLTYPE installType)
{
	BOOL bRet = FALSE;
	if(!app_os_is_file_exist(AcrInstallerPath))
	{
		SetLastWarningMsg("The installer not found!");
		AcrInstallMsgSend(installType, "The installer not found!", istl_updt_noInstaller);
	}
	else
	{
		char cmdLine[BUFSIZ] = {0};
		CurInstallType = installType;
		if(CheckOtherAcronis())
		{
			char modulePath[MAX_PATH] = {0};
			char untOldAcrPath[MAX_PATH] = {0};
			app_os_get_module_path(modulePath);
			sprintf(untOldAcrPath, "%s%s", modulePath, DEF_UNINSTALL_OLD_ACRONIS_NAME);
			if(app_os_is_file_exist(untOldAcrPath))
			{

				memset(cmdLine, 0, sizeof(cmdLine));
				sprintf(cmdLine, "cmd.exe /c \"%s\"", untOldAcrPath);
				AcrInstallMsgSend(installType, "Uninstall old acronis start!", updt_unistlStart);
				if(!app_os_CreateProcess(cmdLine))
				{
					SetLastWarningMsg("Uninstall old acronis failed!");
					AcrInstallMsgSend(installType, "Uninstall old acronis failed!", updt_unistlFail);
					goto done1;
				}
				else
				{
					AcrInstallMsgSend(installType, "Uninstalling old acronis", updt_unistling);
					AcrInstallMsgSend(installType, "Uninstall old acronis ok!", updt_unistlEnd);

				}
			}
		}
		else
		{
			memset(cmdLine, 0, sizeof(cmdLine));
			switch(installType)
			{
			case ITT_INSTALL:
				{
					if(IsAcrInstallThenActive)
					{
						sprintf(cmdLine, "cmd.exe /c \"%s\" %s", AcrInstallerPath, "silent activate");
					}
					else
					{
						sprintf(cmdLine, "cmd.exe /c \"%s\" %s", AcrInstallerPath, "silent notactivate");
					}
					break;
				}
			case ITT_UPDATE:
				{
					sprintf(cmdLine, "cmd.exe /c \"%s\" %s", AcrInstallerPath, "reinstall");
					break;
				}
			default: break;
			}

			if(strlen(cmdLine) <= 0)
			{
				SetLastWarningMsg("Installer type not found!");
				AcrInstallMsgSend(installType, "Installer type not found!", istl_updt_noType);
				goto done1;
			}
			AcrInstallMsgSend(installType, "Installer install start!", istl_updt_istlStart);
			if(!app_os_CreateProcess(cmdLine))
			{
				SetLastWarningMsg("Installer to silent install failed!");
				AcrInstallMsgSend(installType, "Installer to silent install failed!", oprt_fail);
				goto done1;
			}
			else
			{
				AcrInstallMsgSend(installType, "Installer installing!", istl_updt_istling);
				WriteDefaultReg();
				SetAcrLatestActionToReg("Installing");

				if(!app_os_is_file_exist(AcroCmdExePath))
				{
					SetLastWarningMsg("Installer install failed, please try again!");
					AcrInstallMsgSend(installType, "Installer install failed, please try again!", oprt_fail);
				}
				else
				{
					SetLastWarningMsg("Installer Install OK!");
					AcrInstallMsgSend(installType, "Installer Install OK!", istl_updt_istlEnd);
					bRet = TRUE;
				}
			}
		}
	}
done1:
	if(bRet)
	{
		switch(installType)
		{
		case ITT_INSTALL:
			{
				AcrInstallMsgSend(installType, "Install action successfully!", oprt_success);
				break;
			}
		case ITT_UPDATE:
			{
				AcrInstallMsgSend(installType, "Update action successfully!", oprt_success);
				break;
			}
		default: break;
		}
		GetRcvyCurStatus();
	}
	return bRet;
}


#endif /* _WIN32 */
