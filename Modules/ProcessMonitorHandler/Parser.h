#ifndef _PARSER_H_
#define _PARSER_H_

#include "platform.h"
#include "ProcessMonitorHandler.h"
#include "cJSON.h"

#define AGENTINFO_BODY_STRUCT			"susiCommData"
#define AGENTINFO_REQID					"requestID"
#define AGENTINFO_CMDTYPE				"commCmd"
#define CATALOG_ID            "catalogID"

//-----------------------Software----------------------------------
#define DEF_HANDLER_NAME         "ProcessMonitor"

#define SWM_SYS_MON_INFO              "System Monitor Info"
#define SWM_PRC_MON_INFO_LIST         "ProcessMonitor"
#define SWM_PRC_MON_INFO              "Process Monitor Info"
#define SWM_REQUEST_ITEMS             "requestItems"
#define SWM_SESSION_ID                "sessionID"
#define SWM_SENSOR_ID_LIST            "sensorIDList"
#define SWM_SENSOR_INFO_LIST          "sensorInfoList"

#define SWM_PRC_NAME                  "Name"
#define SWM_PRC_ID                    "prcID"
#define SWM_PID                       "PID"
#define SWM_OWNER                       "Owner"
#define SWM_PRC_CPU_USAGE             "CPU Usage"
#define SWM_PRC_MEM_USAGE             "Mem Usage"
#define SWM_PRC_IS_ACTIVE             "IsActive"
#define SWM_SYS_CPU_USAGE             "sysCPUUsage"
#define SWM_SYS_TOTAL_PHYS_MEM        "totalPhysMemKB"
#define SWM_SYS_AVAIL_PHYS_MEM        "availPhysMemKB"

#define SWM_THR_INFO                  "swmThr"
#define SWM_THR_CPU                   "cpu"
#define SWM_THR_MEM                   "mem"
#define SWM_THR_PRC_NAME              "prc"
#define SWM_THR_PRC_ID                "prcID"
#define SWM_THR_MAX                   "max"
#define SWM_THR_MIN                   "min"
#define SWM_THR_TYPE                  "type"
#define SWM_THR_LASTING_TIME_S        "lastingTimeS"
#define SWM_THR_INTERVAL_TIME_S       "intervalTimeS"
#define SWM_THR_ENABLE                "enable"
#define SWM_THR_ACTTYPE               "actType"
#define SWM_THR_ACTCMD                "actCmd"

#define SWM_SET_MON_OBJS_REP          "setThrRep"                   //"swmSetMonObjsRep"
#define SWM_DEL_ALL_MON_OBJS_REP      "delAllThrRep"                //"swmDelAllMonObjsRep"
#define SWM_NORMAL_STATUS             "thrCheckStatus"              //"normalStatus"
#define SWM_PRC_MON_EVENT_MSG         "thrCheckMsg"                 //"monEventMsg"

#define SWM_RESTART_PRC_REP           "swmRestartPrcRep"
#define SWM_KILL_PRC_REP              "swmKillPrcRep"

#define SWM_E_FLAG                   "e"
#define SWM_N_FLAG                   "n"
#define SWM_V_FLAG                   "v"
#define SWM_SV_FLAG                  "sv"
#define SWM_BV_FLAG                  "bv"
#define SWM_BN_FLAG                  "bn"
#define SWM_BASM_FLAG                "basm"
#define SWM_BTYPE_FLAG               "btype"
#define SWM_BST_FLAG                 "bst"
#define SWM_BEXTEND_FLAG             "bextend"
#define SWM_STATUSCODE_FLAG          "StatusCode"
#define SWM_AUTOUPDATE_ALL           "All"
#define SWM_PROC_NOTIFY_MSG          "Notification"
#define SWM_PROC_USER_NOT_LOGON        "Not logged on"

#define SWM_GETDATA_STATUS_SUCCESS   200
#define SWM_GETDATA_STATUS_NOTFOUND  404
#define PATH_DEPTH                   3
#define THR_TAG_NAME_COUNT           2
#define THR_SYSINFO_FLAG            -1
#define THR_PRC_SAME_NAME_ALL_FLAG            -1

#define SWM_ERROR_REP                     "errorRep"

#define SWM_AUTOREP_INTERVAL_SEC        "autoUploadIntervalSec"
#define SWM_AUTOREP_INTERVAL_MS        "autoUploadIntervalMs"
#define SWM_AUTOREP_TIMEOUT_MS        "autoUploadTimeoutMs"

int Parser_string(char *pCommData, char **outputStr,int repCommandID);
BOOL Parse_swm_kill_prc_req(char *inputstr, unsigned int * outputVal);
BOOL Parse_swm_restart_prc_req(char *inputstr, unsigned int* outputVal);
int Parser_swm_mon_prc_event_rep(char *pCommData, char **outputStr);
int Pack_swm_set_mon_objs_req(susi_comm_data_t * pCommData, char ** outputStr);
int Parser_get_spec_info_rep(char *pSysCommData ,char *pProcCommData, int gatherLevel, bool isUserLogon, char **outputStr);
int Parser_get_request_items_rep(char *pSysCommData ,char *pProcCommData, int gatherLevel, bool isUserLogon,sensor_info_list sensorInfoList, char **outputStr);
int Parse_swm_set_mon_objs_req(char * inputStr, char * monVal);
bool Parser_ParseAutoUploadCmd(char * cmdJsonStr, sensor_info_list siList, bool * isAutoUpdateAll,unsigned int * intervalTimeS);
bool Parser_ParseAutoReportStopCmd(char * cmdJsonStr);
int ParseReceivedData(void* data, int datalen, int * cmdID);
int Parser_PackGetSensorDataError(char * errorStr, char * pSessionID, char** outputStr);
int Parser_PackGetSensorDataRep(sensor_info_list sensorInfoList, char * pSessionID, char** outputStr);
bool Parser_ParseGetSensorDataReqEx(void * data, sensor_info_list siList, char * pSessionID);
bool Parser_GetSensorJsonStr(char *pSysCommData ,char *pProcCommData, sensor_info_t * pSensorInfo);
#endif
