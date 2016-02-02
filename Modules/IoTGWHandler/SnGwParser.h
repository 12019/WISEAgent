#ifndef _SUSI_CONTROL_PARSER_H_
#define _SUSI_CONTROL_PARSER_H_

#include "platform.h"
#include "cJSON.h"
#include "BasicFun_Tool.h"
#include "IoTGWFunction.h"
#include "IoTGWHandler.h"


#define MAX_SIZE_SESSIONID              64
#define MAX_HANDL_NAME                 32

#define DEF_HANDLER_NAME               "SUSIControl"
#define AGENTINFO_BODY_STRUCT	"susiCommData"
#define AGENTINFO_REQID					"requestID"
#define AGENTINFO_CMDTYPE				"commCmd"
#define AGENTINFO_SESSION_ID         "sessionID"
#define SN_ERROR_REP                           "result"

#define SN_STATUS_CODE                     "StatusCode"
#define SN_RESUTL                                 "Result"


#define SEN_HANDLER_NAME				 "handlerName"
#define SEN_IDLIST									 "sensorIDList"
// ------------------------- SenML -----------------------------
#define SENML_E_FLAG								"e"
#define SENML_N_FLAG								"n"
#define SENML_V_FLAG								"v"
#define SENML_SV_FLAG							"sv"
#define SENML_BV_FLAG							"bv"
#define SENML_ASM_FLAG                     "asm"
#define SENML_RW                                     "rw"
#define SENML_R                                          "r"
#define SENML_W                                        "w"

//--------------------------Reply Format----------------------
// { "sessionID":"XXX", "sensorInfoList":{"e":[{"n":"SenData/dout","sv":"Setting","StatusCode":202}]} }

static const char *g_szErrorCapability = "{\"IoTGW\":{\"ver\":1}}";
static const char *g_szSenHubErrorCapability = "{\"SenHub\":{\"ver\":1}}";

#define REPLY_FORMAT "{\"sessionID\":\"%s\", \"sensorInfoList\":{\"e\":[%s]} }"
// {"n":"SenData/door temp","v":26, "StatusCode": 200}
#define REPLY_SENSOR_FORMAT "{ \"n\":\"%s\", %s, \"StatusCode\": %d}"  

#define REPLY_SENSOR_400ERROR "{\"n\":\"%s\", \"StatusCode\": 400 }"

#define SENHUB_SET_REPLY_FORMAT "{ \"sessionID\":\"%s\", \"sensorInfoList\":{\"e\":[ {\"n\":\"%s\", %%s, \"StatusCode\": %%d } ] }}"

#define IOTGW_ERROR_FORMAT "{\"%s\":\"%s\"}"

#define IOTGW_INFTERFACE_INDEX -100

//-----------------------------------------------------------------
#ifdef __cplusplus
extern "C" {
#endif


//-----------------------------------------------------
//findType   0      unknow
//           1      value
//           2      id
//-----------------------------------------------------
typedef enum{
	PFT_UNKONW = 0,
	PFT_VALUE  =1,
	PFT_ID     =2,
}path_find_t;

int ParseReceivedData(void* data, int datalen, int * cmdID, char *sessionId, int nLenSessionId );


// Sensor Hub
int GetSenHubUIDfromTopic(const char *topic, char *uid , const int size );

int ProcGetSenHubCapability(const char *uid, const char *data, char *buffer, const int bufsize);
int ProcGetSenHubValue(const char *uid, const char *sessionId, const char *cmd, char *buffer, const int bufsize);
int ProcSetSenHubValue(const char *uid, const char *sessionId, const char *cmd, const int index, const char *topic, char *buffer, const int bufsize);

int EnableSenHubAutoReport(const char *uid ,int enable );

// IoTGW Interface
int ProcGetInterfaceValue(const char *sessionId, const char *data, char *buffer, const int buffersize);
int ProcSetInterfaceValue(const char *sessionId, const char *data, char *buffer, const int buffersize);
int ProcGetTotalInterface(char *buffer, const int bufsize);

int ProcAsynSetSenHubValueEvent(const char *data, const int datalen, const AsynSetParam *pAsynSetParam, char *buffer, const int bufsize);



#ifdef __cplusplus
}
#endif

#endif