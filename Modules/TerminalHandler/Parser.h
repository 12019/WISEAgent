#ifndef _TERMINAL_PARSER_H_
#define _TERMINAL_PARSER_H_

#include "platform.h"
#include "cJSON.h"
#include "TerminalHandler.h"

#define AGENTINFO_BODY_STRUCT			          "susiCommData"
#define AGENTINFO_REQID					          "requestID"
#define AGENTINFO_CMDTYPE				          "commCmd"

//--------------------------SUSI Ctrl monitor----------------------
#define TERMINAL_CMDSTR                       "terminalCmdStr"
#define TERMINAL_CMDID                        "terminalCmdId"
#define TERMINAL_WIDTH                        "terminalWidth"
#define TERMINAL_HEIGHT                       "terminalHeight"
#define TERMINAL_AUTO_UPLOAD_PARAMS           "terminalAutoUploadParams"
#define TERMINAL_SET_TI_AUTO_UPLOAD_REP       "terminalSetTiAutoUploadReply"
#define TERMINAL_AUTO_UPLOAD_INTERVAL_MS      "terminalAutoUploadIntervalMs"
#define TERMINAL_AUTO_UPLOAD_TIMEOUT_MS       "terminalAutoUploadTimeoutMs"
#define TERMINAL_TI_INFO                      "terminalTiInfo"
#define TERMINAL_CONTENT                      "terminalContent"
#define TERMINAL_STOP_CONTENT                 "terminalStopContent"
#define TERMINAL_PID					             "terminalPid"
#define TERMINAL_SET_REQP_REP                 "terminalSetReqpReply"
#define TERMINAL_ERROR_REP                    "errorRep"

#define TERMINAL_INFOMATION                   "Information"
#define TERMINAL_E_FLAG                       "e"
#define TERMINAL_N_FLAG                       "n"
#define TERMINAL_BN_FLAG                      "bn"
#define TERMINAL_V_FLAG                       "v"
#define TERMINAL_SV_FLAG                      "sv"
#define TERMINAL_SSH_ID                       "sshId"
#define TERMINAL_SSH_PWD                      "sshPwd"
#define TERMINAL_FUNCTION_LIST                "functionList"
#define TERMINAL_FUNCTION_CODE                "functionCode"
#define TERMINAL_NS_DATA                      "nonSensorData"
//-----------------------------------------------------------------

#ifdef __cplusplus
extern "C" {
#endif
bool ParseReceivedData(void* data, int datalen, int * cmdID);
bool Parser_ParseSessionCmd(char * jsonStr, int *sesID, char ** cmdStr);
bool Parser_ParseSessionCmdEx(char * jsonStr, int *sesID, int *width, int *height, char ** cmdStr);
bool Parser_ParseSessionStopParams(char * jsonStr, int *sesID);

int Parser_PackSessionStopRep(int sesID, char * repMsg, char **outputStr);
int Parser_PackSessionStartRep(int sesID, char * repMsg, char **outputStr);
int Parser_PackTerminalError(char * errorStr, char **outputStr);
int Parser_PackSesRet(int sesID, char * retStr, char **outputStr);

int Parser_PackCpbInfo(tmn_capability_info_t * cpbInfo, char **outputStr);
int Parser_PackSpecInfoRep(char * cpbStr, char * handlerName, char ** outputStr);
#ifdef __cplusplus
}
#endif

#endif