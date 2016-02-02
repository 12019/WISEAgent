#ifndef _PARSER_H_
#define _PARSER_H_

#include "platform.h"
#include "NetMonitorHandler.h"
#include "cJSON.h"

#define DEF_HANDLER_NAME         "NetMonitor"

#define DEF_IPSO_VER                1

#define AGENTINFO_BODY_STRUCT			"susiCommData"
#define AGENTINFO_REQID					"requestID"
#define AGENTINFO_CMDTYPE				"commCmd"

#define SUSICTRL_AUTOREP_INTERVAL_SEC        "autoUploadIntervalSec"

#define IPSO_BN                     "bn"
#define IPSO_BT                     "bt"
#define IPSO_BU                     "bu"
#define IPSO_VER                    "ver"
#define IPSO_E                      "e"
#define IPSO_N                      "n"
#define IPSO_V                      "v"
#define IPSO_SV                     "sv"
#define IPSO_ALIAS					"alias"

#define NETMON_E_FLAG                   "e"
#define NETMON_N_FLAG                   "n"
#define NETMON_V_FLAG                   "v"
#define NETMON_SV_FLAG                  "sv"
#define NETMON_BV_FLAG                  "bv"
#define NETMON_BN_FLAG                  "bn"
#define	NETMON_STATUS_CODE              "StatusCode"
#define NETMON_Group_THR_ALL			"All"
#define NETMON_GROUP_NETINDEX           "Index"

#define AUTOREP_INTERVAL_SEC                 "autoUploadIntervalSec"
#define NETMON_INFO_LIST                     "netMonInfoList"
#define NET_MON_INFO_LIST                    "netMonInfoList"
#define NET_MON_INFO                         "netMonInfo"
#define NET_INFO_ADAPTER_NAME                "adapterName"
#define NET_ALIAS			                 "alias"
#define NET_INFO_ADAPTER_DESCRI              "adapterDescription"
#define NET_INFO_INDEX                       "index"
#define NET_INFO_TYPE                        "type"
#define NET_INFO_USAGE                       "netUsage"
#define NET_INFO_SEND_BYTE                   "sendDateByte"
#define NET_INFO_SEND_THROUGHPUT             "sendThroughput"
#define NET_INFO_RECV_BYTE                   "recvDateByte"
#define NET_INFO_RECV_THROUGHPUT             "recvThroughput"

//SA3.1
#define NET_INFO_STATUS                      "netStatus"
//#define NET_INFO_SPEED_MBPS                  "netSpeedMbps"
#define NET_INFO_SPEED_MBPS                  "Link SpeedMbps"
//SA3.0
#define NET_INFO_STATUS_30                    "status"
#define NET_INFO_SPEED_MBPS_30                "speedMbps"

#define NETMON_SET_AUTO_UPLOAD_REP           "setAutoUploadRep"
#define NETMON_SET_REQP_REP                  "setReqpRep"
#define NETMON_ERROR_REP                     "errorRep"
#define NETMON_SESSION_ID                    "sessionID"

#define AUTO_UPLOAD_PARAMS        "autoUploadParams"
#define AUTO_UPLOAD_INTERVAL_MS   "autoUploadIntervalMs"
#define AUTO_UPLOAD_TIMEOUT_MS    "autoUploadTimeoutMs"
#define NETMON_REQUEST_ITEMS      "requestItems"
#define NETMON_AUTOUPDATE_ALL      "All"

#define NETMON_JSON_ROOT_NAME              "susiCommData"
#define NETMON_THR                         "nmThr"
#define NETMON_THR_TAG_NAME                "tagName"
#define NETMON_THR_N				       "n"
#define NETMON_THR_MAX                     "max"
#define NETMON_THR_MIN                     "min"
#define NETMON_THR_TYPE                    "type"
#define NETMON_THR_LTIME                   "lastingTimeS"
#define NETMON_THR_ITIME                   "intervalTimeS"
#define NETMON_THR_ENABLE                  "enable"

#define NETMON_SET_THR_REP                 "setThrRep"
#define NETMON_DEL_ALL_THR_REP             "delAllThrRep"
#define NETMON_THR_CHECK_STATUS            "thrCheckStatus"
#define NETMON_THR_CHECK_MSG               "thrCheckMsg"

#define NETMON_SENSOR_ID_LIST            "sensorIDList"
#define NETMON_SENSOR_INFO_LIST            "sensorInfoList"

int Parse_netmon_set_netinfo_auto_upload_req(char *inputstr,  auto_upload_params_t* outVal);
bool ParseReceivedData(void* data, int datalen, int * cmdID);
int Pack_netmon_netinfo_rep(char * pCommData, char** outputStr);
int Pack_netmon_set_netinfo_auto_upload_rep(char *pCommData, char **outputStr);
int Pack_netmon_error_rep(char *pCommData, char **outputStr);
int Pack_netmon_set_netinfo_reqp_rep(char *pCommData, char **outputStr);

int Parser_PackCapabilityStrRep(net_info_list netInfoList, char ** outputStr);
int Parser_PackCapabilityStrRepNew(net_info_list netInfoList, char *adapterName, char ** outputStr);
int Parser_PackNMData(net_info_list netInfoList, char ** outputStr);
int Parser_PackReportNMData(char * nmJsonStr, char * handlerName,char ** outputStr);
int Parser_PackGetSensorDataRep(sensor_info_list sensorDataStr, char * pSessionID, char** outputStr);
int Parser_PackGetSensorDataError(char * errorStr, char * pSessionID, char** outputStr);
int Parser_PackThrInfo(nm_thr_item_list thrList, char ** outputStr);
cJSON * Parser_PackThrItemList(nm_thr_item_list thrList);
cJSON * Parser_PackThrItemInfo(nm_thr_item_t * pThrItemInfo);
int Parser_PackThrCheckRep(nm_thr_rep_info_t * pThrCheckRep, char ** outputStr);
int Parser_PackSetThrRep(char * repStr, char ** outputStr);
int Parser_PackDelAllThrRep(char * repStr, char ** outputStr);

bool Parser_ParseAutoUploadCmd(char * cmdJsonStr, sensor_info_list siList, bool * isAutoUpdateAll,unsigned int * intervalTimeS);
bool Parser_ParseAutoReportStopCmd(char * cmdJsonStr);
bool Parser_ParseGetSensorDataReq(void* data, char * pSessionID);
bool Parser_ParseThrItemInfo(cJSON * jsonObj, nm_thr_item_t * pThrItemInfo, bool *nIsValidPnt);
bool Parser_ParseThrInfo(char * thrJsonStr, net_info_list curNetInfoList, nm_thr_item_list thrList);
bool Parser_GetSensorJsonStr(char *pNetInfoList, sensor_info_t * pSensorInfo);
bool Parser_get_request_items_rep(net_info_list pNetInfoList, sensor_info_list sensorInfoList, char *isAutoUpdateAllNet, char **outputStr);
bool Parser_ParseGetSensorDataReqEx(void * data, sensor_info_list siList, char * pSessionID);

//---------------------------------------------------net---------------------------------------------------------------------
#define cJSON_AddNetMonInfoToObject(object, name, pN) cJSON_AddItemToObject(object, name, cJSON_CreateNetMonInfo(pN))
#define cJSON_AddNetMonInfoListToObject(object, name, pN) cJSON_AddItemToObject(object, name, cJSON_CreateNetMonInfoList(pN))
#define cJSON_AddSensorInfoListToObject(object, name, sN) cJSON_AddItemToObject(object, name, cJSON_CreateSensorInfoList(sN))

#endif