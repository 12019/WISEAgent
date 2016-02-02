#ifndef _SOFTWAREMONITOR_HANDLER_H_
#define _SOFTWAREMONITOR_HANDLER_H_

//#include "susiaccess_handler_api.h "
//#include <WinBase.h>
#include "common.h"

#define cagent_request_software_monitoring 15
#define cagent_reply_software_monitoring 108


#define DEF_INVALID_TIME                           (-1) 
#define DEF_INVALID_VALUE                          (-999)    
#define DIV                       (1024)


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

#define DEF_TAG_NAME_LEN                           32

#define  cagent_status_t AGENT_SEND_STATUS

#define MAX_RECORD_BUFFER_SIZE       (6*1024)
#define MAX_TIMESTAMP_LEN            (19 + 1)     // YYYY-MM-DD hh:mm:ss
#define MAX_EVENT_TYPE_LEN           (32)
#define MAX_EVENT_USER_LEN           (128)
#define MAX_EVENT_DOMAIN_LEN         (128)
#define MAX_EVENT_DESCRIPTION_LEN    (1024)
#define MAX_EVENT_DESP_SSTR_CNT      (32)
#define DEF_PER_QUERY_CNT            (20)

#define MAX_LOGON_USER_CNT           32
#define MAX_LOGON_USER_NAME_LEN      32

#define CFG_FLAG_SEND_PRCINFO_ALL       0
#define CFG_FLAG_SEND_PRCINFO_BY_ALLUSERS  1
#define CFG_FLAG_SEND_PRCINFO_BY_USER  2

typedef struct swm_get_sys_event_log_param_t{
	char eventGroup[64];
	char startTimeStr[32];
	char endTimeStr[32];
}swm_get_sys_event_log_param_t;


typedef enum{
	unknown_cmd = 0,

	swm_set_pmi_auto_upload_req = 151,    //pmi---process monitor info
	swm_set_pmi_auto_upload_rep,
	swm_set_pmi_reqp_req,
	swm_set_pmi_reqp_rep,

	swm_set_mon_objs_req = 157,
	swm_set_mon_objs_rep,
	swm_del_all_mon_objs_req,
	swm_del_all_mon_objs_rep,
 
	swm_restart_prc_req = 167,
	swm_restart_prc_rep,
	swm_kill_prc_req,
	swm_kill_prc_rep,

	swm_mon_prc_event_rep = 173,

	swm_get_event_log_req = 174,
	swm_get_event_log_rep,
	swm_event_log_list_rep,

	swm_get_smi_req = 177,     //smi---system monitor info
	swm_get_smi_rep,

	swm_get_pmi_list_req = 179,    //pmi---process monitor info
	swm_get_pmi_list_rep,

	swm_set_smi_auto_upload_req = 181, //smi---system monitor info
	swm_set_smi_auto_upload_rep,
	swm_set_smi_reqp_req,
	swm_set_smi_reqp_rep,

	swm_error_rep = 250,
}susi_comm_cmd_t;

typedef struct{
	__int64 lastIdleTime;
	__int64 lastKernelTime;
	__int64 lastUserTime;
}sys_cpu_usage_time_t;

typedef struct{
	int cpuUsage;
	long totalPhysMemoryKB;
	long availPhysMemoryKB;
	sys_cpu_usage_time_t sysCpuUsageLastTimes;
}sys_mon_info_t;

typedef struct{
	__int64 lastKernelTime;
	__int64 lastUserTime;
	__int64 lastTime;
}prc_cpu_usage_time_t;

typedef struct{
	int isValid;
	char * prcName;
	char * ownerName;
	unsigned int prcID;
	int cpuUsage;
	long memUsage;
	int isActive;
	prc_cpu_usage_time_t prcCpuUsageLastTimes;
}prc_mon_info_t;

typedef struct prc_mon_info_node_t{
	prc_mon_info_t prcMonInfo;
	struct prc_mon_info_node_t * next;
}prc_mon_info_node_t;

typedef enum{
	prc_thr_type_unknown,
	prc_thr_type_active,
	prc_thr_type_cpu,
	prc_thr_type_mem,
}prc_thr_type_t;

typedef struct{
	unsigned long auto_upload_interval_ms;
	unsigned long auto_upload_timeout_ms;
}swm_auto_upload_params_t;

typedef struct EventLogInfo{
	char * eventGroup;
	char * eventTimestamp;
	char * eventType;
	char * eventSource;
	unsigned long eventID;
	char * eventUser;
	char * eventComputer;
	char * eventDescription;
}EVENTLOGINFO, *PEVENTLOGINFO;

typedef struct EventLogInfoNode * PEVENTLOGINFONODE;
typedef struct EventLogInfoNode{
	EVENTLOGINFO eventLogInfo;
	PEVENTLOGINFONODE next;
}EVENTLOGINFONODE;
typedef PEVENTLOGINFONODE EVENTLOGINFOLIST;

//------------------------------------Threshold define--------------------------
#define DEF_THR_UNKNOW_TYPE                0
#define DEF_THR_MAX_TYPE                   1
#define DEF_THR_MIN_TYPE                   2
#define DEF_THR_MAXMIN_TYPE                3

#define DEF_PRC_MON_CONFIG_NAME                  "ProcessMonitorConfig"
#define DEF_PRC_MON_ITEM_PROC_NAME               "ProcName"
#define DEF_PRC_MON_ITEM_PROC_ID                 "ProcID"
#define DEF_PRC_MON_ITEM_PROC_THRESHOLD          "ProcThresholdStr"
#define DEF_PRC_MON_ITEM_PROC_CUPUSAGE_CTMS      "ProcCpuUsageTHSHDContinuTimeS"
#define DEF_PRC_MON_ITEM_PROC_MEMUSAGE_CTMS      "ProcMemUsageTHSHDContinuTimeS"
#define DEF_PRC_MON_ITEM_PROC_ACT                "ProcAct"
#define DEF_PRC_MON_ITEM_PROC_ACT_CMD            "ProcActCmd"
#define CONFIG_LINE_LENGTH        1024

//-------------------------software monitor data define-------------------------
#define DEF_PRC_MON_INFO_UPLOAD_INTERVAL_MS   (1000)
#define DEF_PRC_MON_INFO_UPLOAD_TIMEOUT_MS    (10*1000)    

typedef enum{
	cagent_callback_abort = -1,
	cagent_callback_continue   
}cagent_callback_status_t;



typedef enum{
	ck_type_unknow = 0,
	ck_type_max,
	ck_type_min,
	ck_type_avg,
}check_type_t;

typedef union check_value_t{
	float vf;
	int vi;
}check_value_t;

typedef struct check_value_node_t{
	check_value_t ckV;
	long long checkValueTime;
	struct check_value_node_t * next;
}check_value_node_t;

typedef struct check_value_list_t{
	check_value_node_t * head;
	int nodeCnt;
}check_value_list_t;

typedef struct sa_thr_item_t{
	char tagName[DEF_TAG_NAME_LEN];
	float maxThreshold;
	float minThreshold;
	int thresholdType;
	int isEnable;
	int lastingTimeS;
	int intervalTimeS;
	check_type_t checkType;
	check_value_t checkResultValue;
	check_value_list_t checkSourceValueList;
	long long repThrTime;
	int isNormal;
}sa_thr_item_t;

typedef sa_thr_item_t hwm_thr_item_t;
typedef sa_thr_item_t swm_thr_item_t;

typedef enum{
	prc_act_unknown,
	prc_act_stop,
	prc_act_restart,
	prc_act_with_cmd,
}prc_action_t;

typedef struct{
	int isValid;
	char * prcName;
	int prcID;
	int prcResponse;
	swm_thr_item_t cpuThrItem;
	swm_thr_item_t memThrItem;
	prc_action_t  cpuAct;
	char * cpuActCmd;
	prc_action_t memAct;
	char * memActCmd;
}mon_obj_info_t;

typedef struct mon_obj_info_node_t{
	mon_obj_info_t monObjInfo;
	struct mon_obj_info_node_t * next;
}mon_obj_info_node_t;

typedef mon_obj_info_node_t * mon_obj_info_list;

#define COMM_DATA_WITH_JSON

typedef struct{
#ifdef COMM_DATA_WITH_JSON
	int reqestID;
#endif
	susi_comm_cmd_t comm_Cmd;
	int message_length;
	char message[];
}susi_comm_data_t;

typedef struct sa_thr_rep_info_t{
	int isTotalNormal;
	char *repInfo;
	//char repInfo[1024*2];
}sa_thr_rep_info_t;
typedef sa_thr_rep_info_t swm_thr_rep_info_t;
#ifdef WIN32
typedef struct tagWNDINFO
{
	DWORD dwProcessId;
	HWND hWnd;
} WNDINFO, *LPWNDINFO;
#endif
typedef enum{
	value_type_unknow = 0,
	value_type_float,
	value_type_int,
}value_type_t;

typedef struct sw_thr_check_params_t {
	prc_mon_info_t * pPrcMonInfo;
	mon_obj_info_t * pMonObjInfo;
	char checkMsg[512];
}sw_thr_check_params_t;

#define cagent_handle_t void *

typedef struct {
	cagent_handle_t   cagentHandle;
}susi_handler_context_t;

typedef struct{
	susi_handler_context_t susiHandlerContext;
	sys_mon_info_t sysMonInfo;
	prc_mon_info_node_t * prcMonInfoList;
	mon_obj_info_node_t * monObjInfoList; 
	CRITICAL_SECTION swPrcMonCS;
	CRITICAL_SECTION swMonObjCS;
	CRITICAL_SECTION swSysMonCS;
	CAGENT_THREAD_HANDLE softwareMonThreadHandle;
	BOOL isSoftwareMonThreadRunning;

	CAGENT_THREAD_HANDLE prcMonInfoUploadThreadHandle;
	BOOL isPrcMonInfoUploadThreadRunning;
	CAGENT_COND_TYPE  prcMonInfoUploadSyncCond; 
	CAGENT_MUTEX_TYPE prcMonInfoUploadSyncMutex;
	BOOL isAutoUpload;
	BOOL isUserLogon;
	int uploadIntervalMs;
	int autoUploadTimeoutMs;
	int timeoutStart;
	CRITICAL_SECTION prcMonInfoAutoCS;

	CAGENT_THREAD_HANDLE sysMonInfoUploadThreadHandle;
	BOOL isSysMonInfoUploadThreadRunning;
	CAGENT_COND_TYPE  sysMonInfoUploadSyncCond; 
	CAGENT_MUTEX_TYPE sysMonInfoUploadSyncMutex;
	BOOL isSysAutoUpload;
	int sysUploadIntervalMs;
	int sysAutoUploadTimeoutMs;
	int sysTimeoutStart;
	CRITICAL_SECTION sysMonInfoAutoCS;

	int gatherLevel;
	char sysUserName[32];
}sw_mon_handler_context_t;

#endif