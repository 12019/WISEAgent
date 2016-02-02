#ifndef __PARSER_RCVY_H__
#define __PARSER_RCVY_H__

#include "RecoveryHandler.h"

//------------------------recovery---------------------------------
#define AGENTINFO_BODY_STRUCT			"susiCommData"
#define AGENTINFO_REQID					"requestID"
#define AGENTINFO_CMDTYPE				"commCmd"
#define CATALOG_ID						"catalogID"

#define RCVY_CREATE_ASZ_PARAMS        "recoveryCreateASZParams"
#define RCVY_IS_INSTALL_THEN_ACTIVE   "rcvyInstallThenActive" 
#define RCVY_INSTALL_ASZ_PERSENT      "rcvyInstallASZPersent"
#define RCVY_REP_MSG                  "recoveryReplyMessage"

#define RCVY_STATUS_INFO              "recoveryStatusInfo"
#define RCVY_STATUS_IS_INSTALLED      "rcvyIsInstalled"
#define RCVY_STATUS_IS_ACTIVATED      "rcvyIsActivated"
#define RCVY_STATUS_IS_EXIST_ASZ      "rcvyIsExistASZ"
#define RCVY_STATUS_IS_EXPIRED        "rcvyIsExpired"
#define RCVY_STATUS_IS_NEWERVER       "rcvyIsNewerVer"
#define RCVY_STATUS_IS_ACRREADY       "rcvyIsAcrReady"
#define RCVY_STATUS_VERSION           "rcvyVersion"
#define RCVY_STATUS_LATEST_BK_TIME    "rcvyLatestBKTime"
#define RCVY_STATUS_ACTION_MSG        "rcvyActionMsg"
#define RCVY_STATUS_LWARNING_MSG      "rcvyWarningMsg"
//#define RCVY_ERROR_REP                "errorRep"

#define RCVY_FTP_DL_USERNAME          "userName"
#define RCVY_FTP_DL_PASSWORD          "pwd"
#define RCVY_FTP_DL_PORT              "port"
#define RCVY_FTP_DL_PATH              "path"
#define RCVY_FTP_DL_MD5               "md5"
//-----------------------------------------------------------------

//----------------recovery reply msg status code-------------------------
//Refer to "Handler_reply_msg_status_code.xlsx"
typedef enum {
#define OPRATION_STATUS_CODE			"statusCode"
	oprt_success = 200,
	oprt_fail = 500,
	oprt_unkown = 700,

	//------------------------------fail msg status code--------------------------------
	actv_notSupport = 501,
	istl_sizeLack = 511,//start
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
	istl_notSupport = 525,//end
	updt_AcrocmdBusy = 531,
	updt_argErr,
	updt_parseErr,
	updt_unistlFail = 534,
	bkp_rstr_expire = 601,
	bkp_busy,
	bkp_noASZ,
	bkp_sizeLack,
	bkp_noResult = 605,
	rstr_busy = 611,
	asz_argErr = 621,
	asz_parseErr,
	asz_threadFail,
	asz_srvFail,
	asz_volumeFail,
	asz_exist,
	asz_noAcronis = 627,


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
	bkp_start = 801,
	bkp_progress,
	bkp_end = 803,
	rstr_start = 811,
	rstr_end = 812,
	asz_creating = 821,

}rcvy_reply_status_code;
//-----------------------------------------------------------------

//int Pack_string(char *pCommData, char **outputStr);
int RcvyReplyMsgPack(cJSON *pInReply, char ** pOutReply);
int Pack_replyMsg(char *pReplyMsg, rcvy_reply_status_code statusCode, char **outputStr);
void SendReplyMsg_Rcvy(char *rcvyMsg, rcvy_reply_status_code statusCode, susi_comm_cmd_t rcvy_rep_id);
void SendReplySuccessMsg_Rcvy(char *rcvyMsg, susi_comm_cmd_t rcvy_rep_id);
void SendReplyFailMsg_Rcvy(char *rcvyMsg, susi_comm_cmd_t rcvy_rep_id);
int Pack_rcvy_status_rep(char * inputstr, char ** outputStr);

int parse_rcvy_install_req(char *inputstr, recovery_install_params * OutParams);
int parse_rcvy_update_req(char *inputstr, recovery_install_params * OutParams);
int parse_rcvy_create_asz_req(char *inputstr, char * outputStr);
bool ParseReceivedData(void* data, int datalen, int * cmdID);

#endif /*__PARSER_RCVY_H__*/
