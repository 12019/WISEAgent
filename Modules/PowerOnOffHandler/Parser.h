#ifndef _POWERONOFF_PARSER_H_
#define _POWERONOFF_PARSER_H_

#include "platform.h"
#include "cJSON.h"
#include "PowerOnOffHandler.h"


#define AGENTINFO_BODY_STRUCT			"susiCommData"
#define AGENTINFO_REQID					"requestID"
#define AGENTINFO_CMDTYPE				"commCmd"
#define HANDLER_NAME                    "power_onoff"

//------------------------power on off-----------------------------
#define POWER_REP_MSG             "powerOnOffReplyMessage"
#define POWER_WAKE_ON_LAN_MACS    "macs"
#define POWER_AMT_PARAMS          "amtInfo"
#define POWER_AMT_EN              "amtEn"
#define POWER_AMT_ID              "amtId"
#define POWER_AMT_PWD             "amtPwd"
#define POWER_ERROR_REP           "errorRep"

#define POWER_INFOMATION                            "Information"
#define POWER_E_FLAG                                "e"
#define POWER_N_FLAG                                "n"
#define POWER_V_FLAG                                "v"
#define POWER_SV_FLAG                               "sv"
#define POWER_BV_FLAG                               "bv"
#define POWER_BN_FLAG							    "bn"
#define POWER_NONSENSORDATA_FLAG					"nonSensorData"
#define POWER_FUNCTION_LIST                         "functionList"
#define POWER_FUNCTION_CODE                         "functionCode"
#define POWER_ERROR_REP                             "errorRep"
#define POWER_MAC_LIST								"MAC List"

bool ParseReceivedData(void* data, int datalen, int * cmdID);

int Parser_PackPowerOnOffStrRep(const char * pCommData, char** outputStr);
int Parser_PackPowerOnOffAMT(power_amt_params * pAmtParams, char** outputStr);
int Parser_PackPowerOnOffError(char * pCommData, char** outputStr);
bool ParsePowerRecvMacsCmd(void* data, char* outputStr);

#endif