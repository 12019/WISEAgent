#ifndef _PARSER_H_
#define _PARSER_H_

#include "platform.h"
#include "SoftwareMonitorHandler.h"
#include "cJSON.h"

#define AGENTINFO_BODY_STRUCT			"susiCommData"
#define AGENTINFO_REQID					"requestID"
#define AGENTINFO_CMDTYPE				"commCmd"
#define CATALOG_ID            "catalogID"

//-----------------------Software----------------------------------
#define SWM_AUTO_UPLOAD_PARAMS        "swmAutoUploadParams"
#define SWM_AUTO_UPLOAD_INTERVAL_MS   "swmAutoUploadIntervalMs"
#define SWM_AUTO_UPLOAD_TIMEOUT_MS    "swmAutoUploadTimeoutMs"
#define SWM_SET_PMI_AUTO_UPLOAD_REP   "swmSetAutoUploadReply"
#define SWM_SET_PMI_REQP_REP          "swmSetReqpReply"
#define SWM_SET_SYSM_AUTO_UPLOAD_REP  "swmSetSysMonAutoUploadReply"
#define SWM_SET_SYSM_REQP_REP         "swmSetSysMonReqpReply"
#define SWM_SYS_MON_INFO              "swmSysMonInfo"
#define SWM_PRC_MON_INFO_LIST         "swmPrcMonInfoList"
#define SWM_PRC_MON_INFO              "swmPrcMonInfo"
#define SWM_PRC_NAME                  "prcName"
#define SWM_PRC_OWNER                      "Owner"
#define SWM_PRC_ID                    "prcID"
#define SWM_PRC_CPU_USAGE             "prcCPUUsage"
#define SWM_PRC_MEM_USAGE             "prcMemUsage"
#define SWM_PRC_IS_ACTIVE             "prcIsActive"
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

#define SWM_SET_MON_OBJS_REP          "swmSetMonObjsRep"
#define SWM_DEL_ALL_MON_OBJS_REP      "swmDelAllMonObjsRep"
#define SWM_RESTART_PRC_REP           "swmRestartPrcRep"
#define SWM_KILL_PRC_REP              "swmKillPrcRep"
#define SWM_NORMAL_STATUS             "normalStatus"
#define SWM_PRC_MON_EVENT_MSG         "monEventMsg"
#define SWM_GET_EVENT_LOG_PARAMS      "getEventLogParams"
#define SWM_START_TIME_PARAM          "startTimeStr"
#define SWM_END_TIME_PARAM            "endTimeStr"
#define SWM_EVENT_LOG_GROUP           "eventGroup"
#define SWM_GET_EVENT_LOG_REP         "swmGetEventLogRep"
#define SWM_EVENT_LOG_LIST            "eventLogList"
#define SWM_EVENT_LOG_INFO            "eventLogInfo"
#define SWM_EVENT_LOG_TIMESTAMP       "eventLogTimestamp"
#define SWM_EVENT_LOG_TYPE            "eventLogType"
#define SWM_EVENT_LOG_SOURCE          "eventLogSource"
#define SWM_EVENT_LOG_ID              "eventLogID"
#define SWM_EVENT_LOG_USER            "eventLogUser"
#define SWM_EVENT_LOG_COMPUTER        "eventComputer"
#define SWM_EVENT_LOG_DESCRIPTION     "eventDescription"

#define SWM_THR_SET_PARMAS                "params"
#define SWM_THR_SET_USERNAME              "userName"
#define SWM_THR_SET_PASSWORD              "pwd"
#define SWM_THR_SET_PORT                  "port"
#define SWM_THR_SET_PATH                  "path"
#define SWM_THR_SET_MD5                   "md5"

#define SWM_ERROR_REP                     "errorRep"

//static cJSON * cJSON_CreateMonObjCpuThr(mon_obj_info_t * pMonObjInfo);
//static cJSON * cJSON_CreateMonObjMemThr(mon_obj_info_t * pMonObjInfo);
//static cJSON * cJSON_CreateMonObjList(mon_obj_info_list monObjList);


int ParseReceivedData(void* data, int datalen, int * cmdID);
int Pack_swm_set_mon_objs_req(susi_comm_data_t * pCommData, char ** outputStr);
int Parse_swm_get_event_log_req(char *pCommData, swm_get_sys_event_log_param_t* OutParams);
BOOL Parse_swm_kill_prc_req(char *inputstr, unsigned int * outputVal);
BOOL Parse_swm_restart_prc_req(char *inputstr, unsigned int* outputVal);
int Parse_swm_set_mon_objs_req(char * inputStr, char * monVal);
BOOL Parse_swm_set_pmi_auto_upload_req(char *inputstr, swm_auto_upload_params_t *outputVal);
int Parser_string(char *pCommData, char **outputStr,int repCommandID);
int Parser_swm_event_log_list_rep(char *pCommData, char **outputStr);
int Parser_swm_get_pmi_list_rep(char *pCommData, char **outputStr);
int Parser_swm_get_smi_rep(char *pCommData, char **outputStr);
int Parser_swm_mon_prc_event_rep(char *pCommData, char **outputStr);


#endif
