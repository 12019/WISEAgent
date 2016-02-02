#ifndef _SUSI_CONTROL_PARSER_H_
#define _SUSI_CONTROL_PARSER_H_

#include "platform.h"
#include "cJSON.h"
#include "SUSIControlHandler.h"

#define DEF_HANDLER_NAME            "SUSIControl"
#define AGENTINFO_BODY_STRUCT			"susiCommData"
#define AGENTINFO_REQID					"requestID"
#define AGENTINFO_CMDTYPE				"commCmd"

//--------------------------SUSI Ctrl monitor----------------------
#define SUSICTRL_CAPABILITY                  "Platform Information"
#define SUSICTRL_HARDWARE_MONITOR            "Hardware Monitor"
#define SUSICTRL_E_FLAG                      "e"
#define SUSICTRL_N_FLAG                      "n"
#define SUSICTRL_V_FLAG                      "v"
#define SUSICTRL_SV_FLAG                     "sv"
#define SUSICTRL_BV_FLAG                     "bv"
#define SUSICTRL_ID_FLAG                     "id"
#define SUSICTRL_DESC_FLAG                   "desc"
#define SUSICTRL_ASM_FLAG                    "asm"
#define SUSICTRL_STATUS_CODE_FLAG            "StatusCode"
#define SUSICTRL_HWM_TEMP                    "Temperature"
#define SUSICTRL_HWM_VOLT                    "Voltage"
#define SUSICTRL_HWM_FAN                     "Fan Speed"
#define SUSICTRL_SENSORS_ID                  "sensorsID"
#define SUSICTRL_SESSION_ID                  "sessionID"
#define SUSICTRL_SENSOR_ID_LIST              "sensorIDList"
#define SUSICTRL_SENSOR_INFO_LIST            "sensorInfoList"
#define SUSICTRL_SENSOR_SET_RET              "result"
#define SUSICTRL_ERROR_REP                   "errorRep"
#define SUSICTRL_SENSOR_SET_PARAMS           "sensorSetParams"
#define SUSICTRL_SENSOR_ID                   "id"
#define SUSICTRL_SENSOR_SET_RET              "result"
#define SUSICTRL_AUTOREP_REQ_ITEMS           "requestItems"
#define SUSICTRL_AUTOREP_ALL                 "All"
#define SUSICTRL_AUTOREP_INTERVAL_SEC        "autoUploadIntervalSec"
#define SUSICTRL_AUTOUPLOAD_INTERVAL_MS      "autoUploadIntervalMs"
#define SUSICTRL_AUTOUPLOAD_CONTINUE_MS      "autoUploadTimeoutMs"
#define SUSICTRL_SET_THR_REP                 "setThrRep"
#define SUSICTRL_DEL_ALL_THR_REP             "delAllThrRep"
#define SUSICTRL_THR_CHECK_STATUS            "thrCheckStatus"
#define SUSICTRL_THR_CHECK_MSG               "thrCheckMsg"

#define SUSICTRL_JSON_ROOT_NAME              "susiCommData"
#define SUSICTRL_THR                         "susictrlThr"
#define SUSICTRL_THR_DESC                    "desc"
#define SUSICTRL_THR_ID                      "id"
#define SUSICTRL_THR_MAX                     "max"
#define SUSICTRL_THR_MIN                     "min"
#define SUSICTRL_THR_TYPE                    "type"
#define SUSICTRL_THR_LTIME                   "lastingTimeS"
#define SUSICTRL_THR_ITIME                   "intervalTimeS"
#define SUSICTRL_THR_ENABLE                  "enable"
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
	PFT_ASM    =3,
	PFT_DESC   =4,
}path_find_t;

bool ParseReceivedData(void* data, int datalen, int * cmdID);
int Parser_PackCapabilityStrRep(char * cpbStr, char** outputStr);
int Parser_PackSUSICtrlError(char * errorStr, char** outputStr);
int Parser_PackGetSensorDataRep(sensor_info_list sensorInfoList, char * pSessionID, char** outputStr);
int Parser_PackGetSensorDataError(char * errorStr, char * pSessionID, char** outputStr);
int Parser_PackSetSensorDataRep(char * repStr, char * sessionID, char** outputStr);
int Parser_PackSetSensorDataRepEx(sensor_info_list sensorInfoList, char * sessionID, char** outputStr);
int Parser_PackSpecInfoRep(char * cpbStr, char * handlerName, char ** outputStr);
cJSON * Parser_PackFilterRepData(char * iotDataJsonStr, cJSON * repFilterItem);
int Parser_PackReportIotData(char * iotDataJsonStr, char * repFilter, char * handlerName,char ** outputStr);
int Parser_PackAutoUploadIotData(char * iotDataJsonStr, char * repFilter, char * handlerName,char ** outputStr);
cJSON * Parser_PackThrItemInfo(susictrl_thr_item_info_t * pThrItemInfo);
cJSON * Parser_PackThrItemList(susictrl_thr_item_list thrItemList);
int Parser_PackThrInfo(susictrl_thr_item_list thrList, char ** outputStr);
int Parser_PackSetThrRep(char * repStr, char ** outputStr);
int Parser_PackDelAllThrRep(char * repStr, char ** outputStr);
int Parser_PackThrCheckRep(susictrl_thr_rep_t * pThrCheckRep, char ** outputStr);

bool Parser_ParseGetSensorDataReq(void* data, char*outputStr, char * pSessionID);
bool Parser_ParseGetSensorDataReqEx(void * data, sensor_info_list siList, char * pSessionID);
bool Parser_ParseSetSensorDataReq(void* data, sensor_info_list sensorInfoList, char * sessionID);
bool Parser_ParseSetSensorDataReqEx(void* data, sensor_info_list sensorInfoList, char * sessionID);
bool Parser_ParseAutoReportCmd(char * cmdJsonStr, unsigned int * intervalTimeS, char * repFilter);
bool Parser_ParseAutoUploadCmd(char * cmdJsonStr, unsigned int * intervalTimeMs, unsigned int * continueTimeMs, char * repFilter);
bool Parser_ParseAutoReportStopCmd(char * cmdJsonStr);
bool Parser_ParseIotHWMInfo(char * iotDatajsonStr, iot_hwm_info_t * pIotHWMInfo);
bool Parser_ParseIotDataInfo(char * iotDataJsonStr, iot_data_info_t * pIotDataInfo);
bool Parser_ParseDataInfo(cJSON * jsonObj, iot_data_info_t * pDataInfo);
bool Parser_ParseThrItemInfo(cJSON * jsonObj, susictrl_thr_item_info_t * pThrItemInfo);
bool Parser_ParseThrItemList(cJSON * jsonObj, susictrl_thr_item_list thrItemList);
bool Parser_ParseThrInfo(char * thrJsonStr, susictrl_thr_item_list thrList);
bool Parser_GetItemDescWithPath(char * path, char * cpbStr, char * descStr);

bool Parser_GetSensorJsonStr(char * curIotJsonStr, char * iotPFIJsonStr, sensor_info_t * pSensorInfo);
bool Parser_GetSensorSetJsonStr(char * curIotJsonStr, char * iotPFIJsonStr, sensor_info_t * pSensorInfo);
#ifdef __cplusplus
}
#endif

#endif