#ifndef _SCREENSHOT_PARSER_H_
#define _SCREENSHOT_PARSER_H_

#include "platform.h"
#include "cJSON.h"
#include "ScreenshotHandler.h"


#define MyTopic                        "screenshot"
#define AGENTINFO_BODY_STRUCT          "susiCommData"
#define AGENTINFO_REQID                "requestID"
#define AGENTINFO_CMDTYPE              "commCmd"

//-------------------------Screenshot------------------------------
#define SCREENSHOT_FTP_USER_NAME       "userName"
#define SCREENSHOT_FTP_PASSWORD        "pwd"
#define SCREENSHOT_FTP_PORT            "port"
#define SCREENSHOT_MSG                 "msg"
#define SCREENSHOT_FILE_NAME           "fileName"
#define SCREENSHOT_STATUS              "status"
#define SCREENSHOT_BASE64_INFO         "ScreenshotBase64"
#define SCREENSHOT_ERROR_REP           "errorRep"
#define SCREENSHOT_INFOMATION          "Information"

#define SCREENSHOT_FUNCTION_LIST       "functionList"
#define SCREENSHOT_FUNCTION_CODE       "functionCode"
#define SCREENSHOT_FLAG_NONE           "none"
#define SCREENSHOT_FLAG_INTERNAL       "internal"
#define SCREENSHOT_FLAGCODE_NONE       0
#define SCREENSHOT_FLAGCODE_INTERNAL   1



//-----------------------------------------------------------------
bool ParseReceivedData(void* data, int datalen, int * cmdID);

int Parser_PackScrrenshotUploadRep(ScreenshotUploadRep * pScreenshotUploadRep, char** outJsonStr);

int Parser_PackScrrenshotError(char * errorStr, char **outputStr);

int Parser_CreateCapabilityRep(char flagCode, char ** outJsonStr);

#endif