#ifndef _RECOVERY_HANDLER_H_
#define _RECOVERY_HANDLER_H_

//-----------------------------------------------------------------------------
// Include:
//-----------------------------------------------------------------------------
#include "platform.h"
#include "common.h"
#include "susiaccess_handler_api.h"

#include "cJSON.h"
#include "sqlite3.h"
#include "FtpDownload.h"
#include "Base64.h"
#include "DES.h"

#include "RecoveryLog.h"


//-----------------------------------------------------------------------------
// Global types:
//-----------------------------------------------------------------------------
#ifndef _WIN32
#define _is_linux
#endif /* _WIN32 */

#undef BOOL
#ifndef BOOL_is_defined
#define BOOL_is_defined
typedef int   BOOL;
#define  FALSE  0
#define  TRUE   1
#endif

#define CAGENT_THREAD_TYPE HANDLE

typedef enum{
	unknown_cmd = 0,
	//--------------------------Recovery command define(41--70)--------------------------------
	rcvy_backup_req = 41,
	rcvy_backup_rep,
	rcvy_restore_req,
	rcvy_restore_rep,
	rcvy_return_status_req,
	rcvy_return_status_rep,
	rcvy_return_status_without_call_hotkey_req,
	rcvy_return_status_without_call_hotkey_rep,
	rcvy_install_req,
	rcvy_install_rep,
	rcvy_active_req,//51
	rcvy_active_rep,
	rcvy_cancel_install_req,
	rcvy_cancel_install_rep,
	rcvy_reinstall_req,
	rcvy_reinstall_rep,
	rcvy_update_req,
	rcvy_update_rep,
	rcvy_create_asz_req,
	rcvy_create_asz_rep,
	rcvy_status_req,//61
	rcvy_status_rep,
	rcvy_log_rep,
	rcvy_error_rep = 70,
	//-----------------------------------------------------------------------------------------
	rcvy_capability_req = 521,
	rcvy_capability_rep,
}susi_comm_cmd_t;

#define COMM_DATA_WITH_JSON

typedef struct{
#ifdef COMM_DATA_WITH_JSON
	int reqestID;
#endif
	susi_comm_cmd_t comm_Cmd;
	int message_length;
	char message[];
}susi_comm_data_t;

#define cagent_handle_t void *
typedef struct {
	cagent_handle_t   cagentHandle;
}susi_handler_context_t;

typedef struct{
	susi_handler_context_t susiHandlerContext;

	BOOL isAcrBackupRunning;
	CAGENT_THREAD_TYPE acrBackupThreadHandle;
	int acrBackupPercent;
	int acrPercentCheckIntervalMs;
	BOOL isAcrPercentCheckRunning;
	BOOL isAcrPercentCheck;
	CAGENT_THREAD_TYPE acrBackupPercentCheckThreadHandle;

	BOOL isAcrRestoreRunning;
	CAGENT_THREAD_TYPE acrRestoreThreadHandle;

	BOOL isCreateASZRunning;
	CAGENT_THREAD_TYPE acrCreateASZThreadHandle;
}recovery_handler_context_t;

typedef enum{
	ITT_UNKNOWN,
	ITT_INSTALL,
	ITT_UPDATE,
	ITT_REINSTALL,
}EINSTALLTYPE;

typedef struct{
	char ftpuserName[64];
	char ftpPassword[64];
	int port;
	char installerPath[260];
	char md5[128];
	int isThenActive;
	int ASZPersent;
	EINSTALLTYPE  installType;
}recovery_install_params;

typedef struct{
	int isInstalled;
	int isActivated;
	int isExpired;
	int isExistNewerVer;
	int isExistASZ;
	int isAcrReady;
	char lastBackupTime[32];
	char version[32];
	char actionMsg[256];
	char lastWarningMsg[256];
}recovery_status_t;

typedef struct rcvy_dl_mon_params_t{
	EINSTALLTYPE installType;
	HFTPDL hfdHandle;
}rcvy_dl_mon_params_t;

recovery_handler_context_t     RecoveryHandlerContext;

typedef enum{
	cagent_callback_abort = -1,
	cagent_callback_continue   
}cagent_callback_status_t;

typedef enum
{
	create_asz_service_fail,
	create_asz_get_voluem_fail,
	create_asz_exist,
	create_asz_no_install,
	create_asz_success,
	create_asz_other_error,
} create_asz_status_t;

typedef Handler_info Plugin_info, PLUGIN_INFO; //Changed in SA3.1



//-----------------------------------------------------------------------------
// Macros define:
//-----------------------------------------------------------------------------
#define DEF_MD5_SIZE							16
#define DEF_TAG_NAME_LEN                        32
#define cagent_request_recovery 				14
#define cagent_reply_recovery 					107

#ifndef _is_linux
#define DEF_UNINSTALL_OLD_ACRONIS_NAME            "UninstallOldAcronis.exe"
#define DEF_ACRONIS_INSTALLER_NAME                "AcronisInstaller.exe"
#define DEF_CLOSE_ACROCMD_NAME                    "CloseAcrocmd.exe"
#define DEF_ACRONIS_CMD_NAME                      "acrocmd.exe"
#endif /* _is_linux */

#define DEF_ACRONIS_SERVICE_NAME                  "mms"
#define DEF_BACKUP_PERCENT_CHECK_INTERVAL_MS      1000
#define DEF_RESTORING_EVENT_ID                    9
#define DEF_BACKUPING_EVENT_ID                    10
#define DEFBACKUP_SUCCESS_EVENT_ID                100
#define DEF_BACKUP_START           "BackupStart"
#define DEF_BACKUP_START_ID        10
#define DEF_BACKUP_SUCCESS         "BackupSuccess"
#define DEF_BACKUP_SUCCESS_ID      4
#define DEF_BACKUP_ERROR           "BackupError"
#define DEF_BACKUP_ERROR_ID        3
#define DEF_RESTORE_START          "RestoreStart"
#define DEF_RESTORE_START_ID       9
#define DEF_RESTORE_SUCCESS        "RecoverySuccess"
#define DEF_RESTORE_REEOR          "RecoveryError"
#define DEF_RESTORE_SUCCESS_ID     6
#define DEF_RESTORE_ERROR_ID       5
#define DEF_ACR_CHECK              "AcrCheck"
#define DEF_ACR_CHECK_ID           51

//-----------------------------------------------------------------------------
// Global variables declare:
//-----------------------------------------------------------------------------
Plugin_info  g_PluginInfo;
void* g_loghandle;
BOOL g_bEnableLog;
char SERVER_IP[512];
const char MyTopic[MAX_TOPIC_LEN];
const int RequestID;
const int ActionID;

HandlerSendCbf 			g_sendcbf;		// Client Send information (in JSON format) to Cloud Server
HandlerSendCustCbf 		g_sendcustcbf;	// Client Send information (in JSON format) to Cloud Server with custom topic
HandlerAutoReportCbf	g_sendreportcbf;// Client Send report (in JSON format) to Cloud Server with AutoReport topic
recovery_handler_context_t  RecoveryHandlerContext;

char rcvyDBFilePath[MAX_PATH];
CAGENT_MUTEX_TYPE  CSWMsg;
CAGENT_MUTEX_TYPE  CSAMsg;
CAGENT_MUTEX_TYPE  Mutex_AcrReady;
BOOL IsAcrReady;
BOOL IsBackuping;
BOOL IsRestoring;
char LastWarningMsg[1024];
char ActionMsg[1024];
char AcrCmdLineToolPath[MAX_PATH];
char AcroCmdExePath[MAX_PATH];
char BackupInfoFilePath[MAX_PATH];
char BackupInfoCopyFilePath[MAX_PATH];
char AcrHistoryFilePath[MAX_PATH];
char BackupBatPath[MAX_PATH];
char RestoreBatPath[MAX_PATH];
//char AcrLogPath[MAX_PATH];
//char AcrTempLogPath[MAX_PATH];

#ifdef _is_linux
char BackupVolume[512];
char backupFolder[MAX_PATH];
char BkpVolumeArgFilePath[MAX_PATH];
#else
BOOL IsAcrInstall;
BOOL IsAcrInstallThenActive;
BOOL IsUpdateAction;
BOOL IsInstallAction;
BOOL IsDownloadAction;
EINSTALLTYPE CurInstallType;
char AcrInstallerPath[MAX_PATH];
char OtherAcronisPath[MAX_PATH];
char DeactivateHotkeyProPath[MAX_PATH];
char ActivateHotkeyProPath[MAX_PATH];
#endif /*_is_linux*/



#endif /* _RECOVERY_HANDLER_H_ */
