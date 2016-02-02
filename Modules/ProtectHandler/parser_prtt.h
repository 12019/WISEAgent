#ifndef __PARSER_PRTT_H__
#define __PARSER_PRTT_H__

#include "ProtectHandler.h"
#include "cJSON.h"

#define AGENTINFO_BODY_STRUCT			"susiCommData"
#define AGENTINFO_REQID					"requestID"
#define AGENTINFO_CMDTYPE				"commCmd"

//------------------------protection-------------------------------
#define PRTT_REP_MSG					"protectReplyMessage"
#define PRTT_STATUS_INFO				"protectionStatusInfo"
#define PRTT_STATUS_IS_INSTALLED		"prttIsInstalled"
#define PRTT_STATUS_IS_ACTIVATED		"prttIsActivated"
#define PRTT_STATUS_IS_PROTECTION		"prttIsProtection"
#define PRTT_STATUS_IS_EXPIRED			"prttIsExpired"
#define PRTT_STATUS_IS_NEWERVER			"prttIsNewerVer"
#define PRTT_STATUS_VERSION				"prttVersion"
#define PRTT_STATUS_ACTION_MSG			"prttActionMsg"
#define PRTT_STATUS_LWARNING_MSG		"prttWarningMsg"
#define PRTT_IS_INSTALL_THEN_ACTIVE		"prttInstallThenActive" 
//#define PRTT_ERROR_REP                "errorRep"

#define PRTT_FTP_DL_USERNAME			"userName"
#define PRTT_FTP_DL_PASSWORD			"pwd"
#define PRTT_FTP_DL_PORT				"port"
#define PRTT_FTP_DL_PATH				"path"
#define PRTT_FTP_DL_MD5					"md5"
//-----------------------------------------------------------------

//----------------protection reply msg status code-------------------------
//Refer to "Handler_reply_msg_status_code.xlsx"
typedef enum {
#define OPRATION_STATUS_CODE			"statusCode"
	oprt_success = 200,
	oprt_fail = 500,
	oprt_unkown = 700,

	//------------------------------fail msg status code--------------------------------
	actv_notSupport = 501,
	istl_sizeLack = 511,
	istl_argErr,
	istl_parseErr,
	istl_exist,
	istl_updt_startFail,
	istl_updt_istlBusy,
	istl_updt_updtBusy,
	istl_updt_ipFail,
	istl_updt_dwlodInitFail,
	istl_updt_dwlodStartFail,
	istl_updt_dwlodErr,
	istl_updt_md5Err,
	istl_updt_noInstaller,
	istl_updt_noType,
	istl_notSupport = 525,
	updt_AcrocmdBusy = 531,
	updt_argErr,
	updt_parseErr,
	updt_unistlFail = 534,
	prtt_expire = 651,
	prtt_busy,
	prtt_soStartFail,
	prtt_soFail,
	prtt_addUpdtWLFail = 655,
	log_err = 661,

	//------------------------------info msg status code--------------------------------
	istl_updt_istlActnStart = 701,
	istl_updt_dwlodStart,
	istl_updt_dwlodProgress,
	istl_updt_dwlodEnd,
	istl_updt_istlStart,
	istl_updt_istling,
	istl_updt_istlMsg,
	istl_updt_istlEnd = 708,
	updt_unistlStart = 711,
	updt_unistling,
	updt_unistlEnd = 713,
	prtt_start = 851,
	prtt_soStart,
	prtt_soInfo,
	prtt_soEnd,
	prtt_addUpdtWLOK,
	prtt_end = 856,
	log_warning = 861,
	log_sys = 862,
}prtt_reply_status_code;
//-----------------------------------------------------------------

int PrttReplyMsgPack(cJSON *pInReply, char ** pOutReply);
void SendReplyMsg_Prtt(char *repMsg, prtt_reply_status_code statusCode, susi_comm_cmd_t prtt_rep_id);
void SendReplySuccessMsg_Prtt(char *repMsg, susi_comm_cmd_t prtt_rep_id);
void SendReplyFailMsg_Prtt(char *repMsg, susi_comm_cmd_t prtt_rep_id);
bool ParseReceivedData(void* data, int datalen, int * cmdID);
bool Parser_ParseInstallerDlParams(void * data, int dataLen, prtt_installer_dl_params_t * dlParams);
int Parser_PackPrttStatusRep(prtt_status_t * pPrttStatus, char** outJsonStr);

#endif /* __PARSER_PRTT_H__ */
 