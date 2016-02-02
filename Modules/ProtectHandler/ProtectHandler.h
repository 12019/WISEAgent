#ifndef _PROTECT_HANDLER_H_
#define _PROTECT_HANDLER_H_

//-----------------------------------------------------------------------------
// Include:
//-----------------------------------------------------------------------------
#include "platform.h"
#include "common.h"
#include "susiaccess_handler_api.h"

#include "md5.h"
#include "Base64.h"
#include "FtpDownload.h"
#include "DES.h"

#include "ProtectHandlerLog.h"


//-----------------------------------------------------------------------------
// Global types:
//-----------------------------------------------------------------------------
typedef enum{
	unknown_cmd = 0,
	//--------------------------Protection command define(11--40)------------------------------
	prtt_protect_req = 11,
	prtt_protect_rep,
	prtt_unprotect_req,
	prtt_unprotect_rep,
	prtt_return_status_req,
	prtt_uninstall_req,
	prtt_uninstall_rep,
	prtt_install_req,
	prtt_install_rep,
	prtt_active_req,//20
	prtt_active_rep,
	prtt_solidify_req,
	prtt_solidify_rep,
	prtt_update_req,
	prtt_update_rep,
	prtt_status_req,
	prtt_status_rep,
	prtt_msg_rep,//28
	prtt_error_rep = 40,
	prtt_capability_req = 521,	
	prtt_capability_rep,
}susi_comm_cmd_t;

typedef enum{
	ITT_UNKNOWN,
	ITT_INSTALL,
	ITT_UPDATE,
	ITT_REINSTALL,
}EINSTALLTYPE;

typedef struct prtt_installer_dl_params_t{
	char ftpuserName[64];
	char ftpPassword[64];
	int port;
	char installerPath[260];
	char md5[128];
	int isInstallThenActive;
	EINSTALLTYPE  installType;
}prtt_installer_dl_params_t;

typedef struct{
	int isInstalled;
	int isActivated;
	int isExpired;
	int isProtection;
	int isExistNewerVer;
	char version[32];
	char actionMsg[256];
	char lastWarningMsg[256];
}prtt_status_t;

typedef struct{
	CAGENT_THREAD_HANDLE sysLogHandle;
	CAGENT_THREAD_HANDLE sysLogCheckThreadHandle;
	CAGENT_THREAD_HANDLE logCheckEventHandle;
	bool isSysLogCheckThreadRunning;

	CAGENT_THREAD_HANDLE sadminOutputCheckThreadHandle;
	bool sadminOutputCheckEnable;
	int sadminOutputCheckIntervalMs;
	bool isSadminOutputCheckThreadRunning;
	bool isProtectActionRunning;
	bool isUnProtectActionRunning;

	CAGENT_THREAD_HANDLE mcafeeLogCheckThreadHandle;
	CAGENT_THREAD_HANDLE mcafeeLogChangeHandle;
	bool isMcAfeeLogCheckThreadRunning;

	CAGENT_MUTEX_TYPE warningMsgMutex;
	CAGENT_MUTEX_TYPE actionMsgMutex;
}handler_context_t;

typedef struct prtt_dl_mon_params_t{
	EINSTALLTYPE installType;
	HFTPDL hfdHandle;
}prtt_dl_mon_params_t;


//-----------------------------------------------------------------------------
// Macros define:
//-----------------------------------------------------------------------------
#define cagent_request_protection  13
#define cagent_reply_protection    106
#define DEF_SADMIN_OUTPUT_CHECK_INTERVAL_MS     100
#define DEF_MD5_SIZE                            16

#ifdef _WIN32
#define DEF_SADMIN_PROCESS_NAME					"sadmin.exe"
#define DEF_SADMIN_OUTPUT_FILE_NAME             "\\McAfee.txt"
#define DEF_MCAFEE_SADMIN_PATH                  "\\McAfee\\Solidcore\\sadmin.exe"

#else

#define DEF_SADMIN_PROCESS_NAME                 "sadmin"
#define DEF_MCAFEE_SADMIN_PATH                   "/usr/sbin/sadmin"
#define DEF_SADMIN_OUTPUT_FILE_NAME             "/McAfee.txt"
#define DEF_LINUX_MCAFEE_LOG_FILE               "/var/log/mcafee/solidcore/solidcore.log"
#define DEF_LINUX_MCAFEE_LOG_FILE1              "/var/log/mcafee/solidcore/solidcore.log.1"
#define DEF_LINUX_SA_INSTALL_TIME_RECORD		 "/etc/solidcore_it"
#define DEF_LINUX_MCAFEE_LICENSE				"0710-2208-1402-0108-2708"
#endif /* _WIN32 */

//-----------------------------------------------------------------------------
// Global variables declare:
//-----------------------------------------------------------------------------
const char	strPluginName[MAX_TOPIC_LEN];
const int	iRequestID;
const int	iActionID;
Handler_info		g_PluginInfo;
handler_context_t	g_HandlerContex;
void*	g_loghandle;
bool	g_bEnableLog;
HandlerSendCbf			g_sendcbf;			// Client Send information (in JSON format) to Cloud Server	
HandlerSendCustCbf		g_sendcustcbf;		// Client Send information (in JSON format) to Cloud Server with custom topic	
HandlerAutoReportCbf	g_sendreportcbf;	// Client Send report (in JSON format) to Cloud Server with AutoReport topic	
HandlerSendCapabilityCbf	g_sendcapabilitycbf;	
HandlerSubscribeCustCbf	g_subscribecustcbf;

char InstallPath[MAX_PATH];
char EncodePassWord[DEF_MD5_SIZE];
char McAfeeInstallerPath[MAX_PATH];
char LastWarningMsg[256];
char ActionMsg[256];

#ifndef _WIN32
BOOL McAfeeStatus;
#endif /*_WIN32*/

//-----------------------------------------------------------------------------
// Global function declare:
//-----------------------------------------------------------------------------
#ifndef _WIN32
char * strcpy_s(char * dest, unsigned int dest_size, const char * src);
#endif /* _WIN32 */
int GetResultFromProcess(void* prcHandle);

#endif /*_PROTECT_HANDLER_H_*/
