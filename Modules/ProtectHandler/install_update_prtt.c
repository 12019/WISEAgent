#ifdef _WIN32

#include "ProtectHandler.h"
#include "parser_prtt.h"
#include "install_update_prtt.h"
#include "activate_prtt.h"
#include "public_prtt.h"
#include "status_prtt.h"

//-----------------------------------------------------------------------------
// Global Variables define:
//-----------------------------------------------------------------------------
BOOL McAfeeIsInstalling = FALSE;
BOOL IsUpdateAction = FALSE;
BOOL IsInstallAction = FALSE;
BOOL IsDownloadAction = FALSE;
BOOL IsInstallThenActive = FALSE;

//-----------------------------------------------------------------------------
// Local Types/Macros/Variables:
//-----------------------------------------------------------------------------
#define DEF_MCAFEE_INSTALL_PROCESS_NAME          "McAfeeInstaller.exe"

static CAGENT_THREAD_HANDLE McAfeeInstallThreadHandle;
static BOOL McAfeeInstallThreadRunning;
static BOOL IsMcAfeeInstallerDLMonThreadRunning = FALSE;

//-----------------------------------------------------------------------------
// Local functions declare:
//-----------------------------------------------------------------------------
static CAGENT_PTHREAD_ENTRY(McAfeeInstallThreadStart, args);
static BOOL McAfeeToInstall(EINSTALLTYPE installType);
static CAGENT_PTHREAD_ENTRY(McAfeeInstallerDLMonThreadStart, args);
static BOOL McAfeeInstallMsgSend(EINSTALLTYPE installType, char *repMsg, prtt_reply_status_code statusCode);
static void SetLastWarningMsg(char * warningMsg);
static void SetActionMsg(char * actionMsg);


//-----------------------------------------------------------------------------
// Global functions define:
//-----------------------------------------------------------------------------
void InstallAction(prtt_installer_dl_params_t * pDLParams)
{
	EINSTALLTYPE tmpInstallType = ITT_UNKNOWN;
	if(pDLParams == NULL) return;
	tmpInstallType = pDLParams->installType;
	if(tmpInstallType != ITT_UNKNOWN)
	{
		if(tmpInstallType == ITT_INSTALL && app_os_is_file_exist(InstallPath))
		{
			SetLastWarningMsg("McAfee already exist, don't need install!");
			McAfeeInstallMsgSend(tmpInstallType, "McAfee already exist, don't need install!", istl_exist);
		}
		else
		{
			if(!IsInstallAction && !IsUpdateAction)
			{
				McAfeeInstallThreadHandle = NULL;
				McAfeeInstallThreadRunning = TRUE;
				McAfeeInstallMsgSend(tmpInstallType, "Install action start", istl_updt_istlActnStart);
				if(app_os_thread_create(&McAfeeInstallThreadHandle, McAfeeInstallThreadStart, pDLParams) != 0)
				{
					McAfeeInstallThreadRunning = FALSE;
					SetLastWarningMsg("Install action start failed!");
					McAfeeInstallMsgSend(tmpInstallType, "Install action start failed!", istl_updt_startFail);
				}
				else app_os_sleep(10);
			}
			else
			{
				if(IsInstallAction)
				{
					SetLastWarningMsg("McAfee is installing, this operation is invalid!");
					McAfeeInstallMsgSend(tmpInstallType, "McAfee is installing, this operation is invalid!", istl_updt_istlBusy);
				}
				if(IsUpdateAction)
				{
					SetLastWarningMsg("McAfee is updating, this operation is invalid!");
					McAfeeInstallMsgSend(tmpInstallType, "McAfee is updating, this operation is invalid!", istl_updt_updtBusy);
				}
			}
		}
	}
}

void McAfeeInstallThreadStop()
{
	if(McAfeeInstallThreadRunning)
	{
		DWORD dwRet = 0;
		McAfeeInstallThreadRunning = FALSE;
		dwRet = app_os_WaitForSingleObject(McAfeeInstallThreadHandle, 3000);
		if(dwRet != WAIT_OBJECT_0)
		{
			app_os_TerminateThread(McAfeeInstallThreadHandle, 0);
			app_os_CloseHandle(McAfeeInstallThreadHandle);
			McAfeeInstallThreadHandle = NULL;
		}
	}
}

BOOL IsInstallerRunning()
{
	BOOL bRet = FALSE;
	bRet = app_os_process_check(DEF_MCAFEE_INSTALL_PROCESS_NAME);
	return bRet;
}



//-----------------------------------------------------------------------------
// Local functions define:
//-----------------------------------------------------------------------------
static CAGENT_PTHREAD_ENTRY(McAfeeInstallThreadStart, args)
{
	BOOL bFlag = TRUE, bDLOK= FALSE, isNeedDownload = TRUE;
	int iRet = 0;
	HFTPDL   hfdHandle = NULL;
	BOOL * actionFlag  = NULL;
	EINSTALLTYPE installType = ITT_UNKNOWN;
	prtt_installer_dl_params_t dlParams;
	if(args == NULL) goto done0;
	memset(&dlParams, 0, sizeof(prtt_installer_dl_params_t));
	memcpy(&dlParams, (prtt_installer_dl_params_t*)args, sizeof(prtt_installer_dl_params_t));
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

	if(app_os_is_file_exist(McAfeeInstallerPath))
	{
		char md5Str[64] = {0};
		//check md5
		if(GetFileMD5(McAfeeInstallerPath, md5Str))
		{
			if(!_stricmp(md5Str, dlParams.md5))
			{
				isNeedDownload = FALSE;
			}
		}
	}
	if(isNeedDownload)
	{
		CAGENT_THREAD_HANDLE McAfeeInstallerDLMonThreadHandle;
		char installerFileUrl[MAX_PATH*2] = {0};
		char serverIP[256] = {0};
		strcpy_s(serverIP, sizeof(serverIP), g_PluginInfo.ServerIP);
		if(!strlen(serverIP))
		{
			SetLastWarningMsg("Get server ip failed!");
			McAfeeInstallMsgSend(installType, "Get server ip failed!", istl_updt_ipFail);
			goto done1;
		}
		else
		{
			hfdHandle = FtpDownloadInit();
			if(hfdHandle == NULL)
			{
				SetLastWarningMsg("File downloader initialize failed!");
				McAfeeInstallMsgSend(installType, "File downloader initialize failed!", istl_updt_dwlodInitFail);
				goto done1;
			}
			else
			{
				prtt_dl_mon_params_t dlMonParams;
				memset(&dlMonParams, 0, sizeof(prtt_dl_mon_params_t));
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

				IsMcAfeeInstallerDLMonThreadRunning = TRUE;
				if(app_os_thread_create(&McAfeeInstallerDLMonThreadHandle, McAfeeInstallerDLMonThreadStart, &dlMonParams) != 0)
				{
					IsMcAfeeInstallerDLMonThreadRunning = FALSE;
					SetLastWarningMsg("McAfee installer download monitor start failed!");
					McAfeeInstallMsgSend(ITT_INSTALL, "McAfee installer download monitor start failed!", istl_updt_dwlodStartFail);
					goto done2;
				}
				iRet = FtpDownload(hfdHandle, installerFileUrl, McAfeeInstallerPath);
				while(McAfeeInstallThreadRunning)
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
						app_os_WaitForSingleObject(McAfeeInstallerDLMonThreadHandle, INFINITE);
						if(McAfeeInstallThreadRunning)
						{
							char md5Str[64] = {0};
							BOOL bRet = FALSE;
							//check md5
							if(GetFileMD5(McAfeeInstallerPath, md5Str))
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
										McAfeeInstallMsgSend(ITT_INSTALL, "Check md5 error!", istl_updt_md5Err);
									}
								}
							}
						}
						break;
					}
					app_os_sleep(10);
				}

				if(!McAfeeInstallThreadRunning)
				{
					IsMcAfeeInstallerDLMonThreadRunning = FALSE;
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
		McAfeeToInstall(installType);
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
	McAfeeInstallThreadRunning = FALSE;
	McAfeeInstallThreadHandle = NULL;
	return 0;
}

static BOOL McAfeeToInstall(EINSTALLTYPE installType)
{
	BOOL bRet = FALSE;
	McAfeeIsInstalling = TRUE;
	if(!app_os_is_file_exist(McAfeeInstallerPath))
	{
		SetLastWarningMsg("The installer not found!");
		McAfeeInstallMsgSend(installType, "The installer not found!", istl_updt_noInstaller);
	}
	else
	{
		if(app_os_is_file_exist(InstallPath))
		{
			CAGENT_HANDLE uninstallPrcHandle = NULL;
			McAfeeInstallMsgSend(installType, "Uninstall start!", updt_unistlStart);
			uninstallPrcHandle = app_os_CreateProcessWithAppNameEx(McAfeeInstallerPath, " uninstall");
			if(!uninstallPrcHandle)
			{
				SetLastWarningMsg("Installer to uninstall failed!");
				McAfeeInstallMsgSend(installType, "Installer to uninstall failed!", updt_unistlFail);
				goto done1;
			}
			else
			{
				McAfeeInstallMsgSend(installType, "Uninstalling", updt_unistling);
				app_os_WaitForSingleObject(uninstallPrcHandle, INFINITE);
				McAfeeInstallMsgSend(installType, "Uninstall ok!", updt_unistlEnd);
				app_os_CloseHandle(uninstallPrcHandle);
			}
		}
		app_os_sleep(1000);
		{
			CAGENT_HANDLE installPrcHandle = NULL;
			McAfeeInstallMsgSend(installType, "Installer install start!", istl_updt_istlStart);
			installPrcHandle = app_os_CreateProcessWithAppNameEx(McAfeeInstallerPath, " silent");
			if(!installPrcHandle)
			{
				SetLastWarningMsg("Installer to silent install failed!");
				McAfeeInstallMsgSend(installType, "Installer to silent install failed!", oprt_fail);
				goto done1;
			}
			else
			{
				McAfeeInstallMsgSend(installType, "Installer installing!", istl_updt_istling);
				app_os_WaitForSingleObject(installPrcHandle, INFINITE);
				app_os_CloseHandle(installPrcHandle);

				if(!app_os_is_file_exist(InstallPath))
				{
					SetLastWarningMsg("Installer install failed, please try again!");
					McAfeeInstallMsgSend(installType, "Installer install failed, please try again!", oprt_fail);
				}
				else
				{
					SetLastWarningMsg("Installer Install OK!");
					McAfeeInstallMsgSend(installType, "Installer Install OK!", istl_updt_istlEnd);
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
				McAfeeInstallMsgSend(installType, "Install action successfully!", oprt_success);
				break;
			}
		case ITT_UPDATE:
			{
				McAfeeInstallMsgSend(installType, "Update action successfully!", oprt_success);
				break;
			}
		default: break;
		}
		if(IsInstallThenActive)
		{
			Activate();
		}
	}
	McAfeeIsInstalling = FALSE;
	GetProtectionCurStatus();
	return bRet;
}


static CAGENT_PTHREAD_ENTRY(McAfeeInstallerDLMonThreadStart, args)
{
	if(args != NULL)
	{
		prtt_dl_mon_params_t prttDLMonParams;
		HFTPDL hfdHandle = NULL;
		FTPDLSTATUS ftpDlStatus = FTP_DSC_UNKNOWN;
		EINSTALLTYPE installType = ITT_UNKNOWN;
		BOOL isBreak = FALSE;
		int iRet = 0, i = 0;
		memset(&prttDLMonParams, 0, sizeof(prtt_dl_mon_params_t));
		memcpy(&prttDLMonParams, args, sizeof(prtt_dl_mon_params_t));
		hfdHandle = prttDLMonParams.hfdHandle;
		installType = prttDLMonParams.installType;
		app_os_sleep(10);
		while(IsMcAfeeInstallerDLMonThreadRunning)
		{
			iRet = FTPDownloadGetStatus(hfdHandle, &ftpDlStatus);
			if(iRet == 0)
			{
				switch(ftpDlStatus)
				{
				case FTP_DSC_START:
					{
						SetActionMsg("File downloader start!");
						McAfeeInstallMsgSend(installType, "File downloader start!", istl_updt_dwlodStart);
						for(i = 0; i<5 && IsMcAfeeInstallerDLMonThreadRunning; i++) app_os_sleep(100);
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
						McAfeeInstallMsgSend(installType, downloadDetial, istl_updt_dwlodProgress);
						for(i = 0; i<10 && IsMcAfeeInstallerDLMonThreadRunning; i++) app_os_sleep(100);
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
						McAfeeInstallMsgSend(installType, downloadDetial, istl_updt_dwlodEnd);
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
						McAfeeInstallMsgSend(installType, lastMsgTmp, istl_updt_dwlodErr);
						IsDownloadAction = FALSE;
						isBreak = TRUE;
						break;
					}
				}
			}
			if(isBreak) break;
			app_os_sleep(10);
		}
	}
	IsMcAfeeInstallerDLMonThreadRunning = FALSE;
	return 0;
}

static BOOL McAfeeInstallMsgSend(EINSTALLTYPE installType, char *repMsg, prtt_reply_status_code statusCode)
{
	BOOL bRet = FALSE;
	handler_context_t * pPrtContext = (handler_context_t *)&g_HandlerContex;
	if(repMsg == NULL) return bRet;
	switch(installType)
	{
	case ITT_INSTALL:
		{
			SendReplyMsg_Prtt(repMsg, statusCode, prtt_install_rep);
			bRet = TRUE;
			break;
		}
	case ITT_UPDATE:
		{
			SendReplyMsg_Prtt(repMsg, statusCode, prtt_update_rep);
			bRet = TRUE;
			break;
		}
	default: break;
	}
	return bRet;
}

static void SetLastWarningMsg(char * warningMsg)
{
	if(warningMsg == NULL) return;
	{
		app_os_mutex_lock(&g_HandlerContex.warningMsgMutex);
		memset(LastWarningMsg, 0, sizeof(LastWarningMsg));
		sprintf(LastWarningMsg, "%s", warningMsg);
		app_os_mutex_unlock(&g_HandlerContex.warningMsgMutex);
	}
}

static void SetActionMsg(char * actionMsg)
{
	if(actionMsg == NULL) return;
	{
		app_os_mutex_lock(&g_HandlerContex.actionMsgMutex);
		memset(ActionMsg, 0, sizeof(LastWarningMsg));	
		sprintf(ActionMsg, "%s", actionMsg);
		app_os_mutex_unlock(&g_HandlerContex.actionMsgMutex);
	}
}

#endif /* _WIN32 */