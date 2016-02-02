#ifndef _PROCESSMONITOR_HANDLER_H_
#define _PROCESSMONITOR_HANDLER_H_
#include "common.h"

#define cagent_request_software_monitoring 25
#define cagent_reply_software_monitoring 118


#define DEF_INVALID_TIME                           (-1) 
#define DEF_INVALID_VALUE                          (-999)    
#define DIV                       (1024)

//-----------------------Software----------------------------------
#define DEF_CONFIG_FILE_NAME	"agent_config.xml"
#define SWM_NORMAL_STATUS             "normalStatus"
#define SWM_PRC_MON_EVENT_MSG         "monEventMsg"
#define CMD_HELPER_FILE_NAME          "ProcessInfoHelper.exe"

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

typedef struct msg_context_t{
	int procID;
	BOOL isActive;
	BOOL isShutdown;
	//char msg[1024];
}msg_context_t;

typedef struct swm_get_sys_event_log_param_t{
	char eventGroup[64];
	char startTimeStr[32];
	char endTimeStr[32];
}swm_get_sys_event_log_param_t;


typedef enum{
	unknown_cmd = 0,
	swm_set_mon_objs_req = 157,
	swm_set_mon_objs_rep,
	swm_del_all_mon_objs_req,
	swm_del_all_mon_objs_rep,
 
	swm_restart_prc_req = 167,
	swm_restart_prc_rep,
	swm_kill_prc_req,
	swm_kill_prc_rep,

	swm_mon_prc_event_rep = 173,

	swm_get_capability_req = 521,
	swm_get_capability_rep = 522,
	swm_get_sensors_data_req = 523,
	swm_get_sensors_data_rep = 524,
	swm_set_sensors_data_req = 525,
	swm_set_sensors_data_rep = 526,
	swm_span_auto_report_req = 533,
	swm_span_auto_report_rep = 534,
	swm_get_sensors_data_error_rep = 598,
	swm_error_rep = 600,
}susi_comm_cmd_t;

typedef enum{
	swm_send_cbf = 0,
	swm_cust_cbf,
	swm_subscribe_cust_cbf,
	swm_send_report_cbf,
	swm_send_infospec_cbf,
}swm_send_cbf_level_t;

typedef struct sensor_info_t{
	//int id;
	char * pathStr;
	char * jsonStr;
	//sensor_set_ret setRet;
}sensor_info_t;
typedef struct sensor_info_node_t{
	sensor_info_t sensorInfo;
	struct sensor_info_node_t * next;
}sensor_info_node_t;
typedef sensor_info_node_t * sensor_info_list;

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
	//prc_thr_type_cpu,
	//prc_thr_type_mem,
}prc_thr_type_t;

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
	char * path;
	char * prcName;
	int prcID;
	int prcResponse;
	swm_thr_item_t thrItem;
	prc_action_t  act;
	char * actCmd;
	bool isHasPID;
	//Wei.Gang mask
	//swm_thr_item_t cpuThrItem;
	//swm_thr_item_t memThrItem;
	//prc_action_t  cpuAct;
	//char * cpuActCmd;
	//prc_action_t memAct;
	//char * memActCmd;
	//Wei.Gang mask end
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

//#define CAGENT_THREAD_TYPE HANDLE

typedef struct{
	//susi_handler_context_t susiHandlerContext;
	sys_mon_info_t sysMonInfo;
	prc_mon_info_node_t * prcMonInfoList;
	mon_obj_info_node_t * monObjInfoList; 
	CRITICAL_SECTION swPrcMonCS;
	CRITICAL_SECTION swMonObjCS;
	CRITICAL_SECTION swSysMonCS;
	CAGENT_THREAD_HANDLE softwareMonThreadHandle;
	BOOL isSoftwareMonThreadRunning;
	BOOL isUserLogon;
/*
	CAGENT_THREAD_TYPE prcMonInfoUploadThreadHandle;
	BOOL isPrcMonInfoUploadThreadRunning;
	CAGENT_COND_TYPE  prcMonInfoUploadSyncCond; 
	CAGENT_MUTEX_TYPE prcMonInfoUploadSyncMutex;
	BOOL isAutoUpload;
	int uploadIntervalMs;
	int autoUploadTimeoutMs;
	int timeoutStart;
	CRITICAL_SECTION prcMonInfoAutoCS;
*/
	//For auto upload handler api
	CAGENT_THREAD_HANDLE autoUploadThreadHandle;
	BOOL isAutoUploadThreadRunning;
	CAGENT_COND_TYPE  autoUploadSyncCond; 
	CAGENT_MUTEX_TYPE autoUploadSyncMutex;
	BOOL isAutoUpload;
	int autoUploadIntervalMs;
	CRITICAL_SECTION autoUploadCS;
	sensor_info_list autoUploadItemsList;
	CRITICAL_SECTION autoUploadItemsListCS;
	bool isAutoUpdateAll;
	//For auto report command

	CAGENT_THREAD_HANDLE autoReportThreadHandle;
	BOOL isAutoReportThreadRunning;
	CAGENT_COND_TYPE  autoReportSyncCond; 
	CAGENT_MUTEX_TYPE autoReportSyncMutex;
	BOOL isAutoReport;
	int autoReportIntervalMs;
	int autoReportTimeoutMs;
	int autoReportTimeoutStart;
	CRITICAL_SECTION autoReportCS;
	sensor_info_list autoReportItemsList;
	CRITICAL_SECTION autoReportItemsListCS;
	bool isAutoReportAll;

	int gatherLevel;
	char sysUserName[32];
}sw_mon_handler_context_t;

#endif