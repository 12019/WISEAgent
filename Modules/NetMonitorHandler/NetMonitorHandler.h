#ifndef _NET_MONITOR_HANDLER_H_
#define _NET_MONITOR_HANDLER_H_

#include "common.h"

#define  cagent_request_net_monitoring 19
#define  cagent_reply_net_monitoring 113

#define DEF_INVALID_VALUE                          (-999)                              

#define cagent_status_t AGENT_SEND_STATUS

typedef enum{
	cagent_callback_abort = -1,
	cagent_callback_continue   
}cagent_callback_status_t;

#define DEF_MAX_THR_EVENT_STR                   "#tk#maxThreshold#tk#"
#define DEF_MIN_THR_EVENT_STR                   "#tk#minThreshold#tk#"
#define DEF_AVG_EVENT_STR                       "#tk#average#tk#"
#define DEF_MAX_EVENT_STR                       "#tk#max#tk#"
#define DEF_MIN_EVENT_STR                       "#tk#min#tk#"
#define DEF_AND_EVENT_STR                       "#tk#and#tk#"
#define DEF_NOR_EVENT_STR                       "#tk#normal#tk#"
#define DEF_NOT_SUPT_EVENT_STR                  "#tk#not support#tk#"

//--------------------------Net monitor data define-----------------------------
#define DEF_ADP_NAME_LEN    256
#define DEF_ADP_DESP_LEN    256
#define DEF_TAG_NAME_LEN    128
#define DEF_N_LEN			256

typedef struct net_info_t{
	DWORD index; //windows & linux: 0,1,2...
	DWORD id; //windows: 11,25...
	char type[128];
	char adapterName[DEF_ADP_NAME_LEN];
	char alias[DEF_ADP_NAME_LEN];
	char adapterDescription[DEF_ADP_DESP_LEN];
	char mac[128];
	char netStatus[128];
	DWORD netSpeedMbps;
	double netUsage;
	DWORD sendDateByte;
	DWORD recvDateByte;
	double sendThroughput;
	double recvThroughput;

	DWORD oldInDataByte;
	DWORD oldOutDataByte;
	DWORD initInDataByte;
	DWORD initOutDataByte;

	BOOL isFoundFlag;
}net_info_t;

typedef struct net_info_node_t{
	net_info_t netInfo;
	struct net_info_node_t * next;
}net_info_node_t;

#define CAGENT_THREAD_TYPE HANDLE

typedef net_info_node_t * net_info_list;

//---------------thr------------------
#define DEF_THR_UNKNOW_TYPE                0
#define DEF_THR_MAX_TYPE                   1
#define DEF_THR_MIN_TYPE                   2
#define DEF_THR_MAXMIN_TYPE                3

#define DEF_INVALID_TIME                   (-1) 
#define DEF_INVALID_VALUE                  (-999) 

typedef enum{
	VT_Unknown,
	VT_S,
	//VT_I,
	VT_F,
}check_value_type_t;

//typedef union check_value_t{
//	float vf;
//	int vi;
//}check_value_t;

typedef enum{
	ck_type_unknow = 0,
	ck_type_max,
	ck_type_min,
	ck_type_avg,
}check_type_t;

typedef struct check_value_node_t{
	//check_value_t ckV;
	float ckV;
	long long checkValTime;
	struct check_value_node_t * next;
}check_value_node_t;

typedef struct check_value_list_t{
	check_value_node_t * head;
	int nodeCnt;
}check_value_list_t;

typedef struct nm_thr_item_t{
	int isValid;
	char n[DEF_N_LEN];
	int index;
	char bn[DEF_N_LEN];
	char adapterName[DEF_ADP_NAME_LEN];
	char tagName[DEF_TAG_NAME_LEN];
	float maxThr;
	float minThr;
	int thrType;
	int isEnable;
	int lastingTimeS;
	int intervalTimeS;
	check_type_t checkType;
	//check_value_t checkRetValue;
	float checkRetValue;
	check_value_list_t checkSrcValList;
	long long repThrTime;
	int isNormal;
}nm_thr_item_t;

typedef struct nm_thr_item_node_t{
	nm_thr_item_t thrItem;
	struct nm_thr_item_node_t * next;
}nm_thr_item_node_t;

typedef nm_thr_item_node_t * nm_thr_item_list;

typedef struct nm_thr_rep_info_t{
	int isTotalNormal;
	//char repInfo[1024*2];
	char *repInfo;
}nm_thr_rep_info_t;
//------------------------------------
//------------------------------------------------------------------------------
#define cagent_handle_t void *

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

typedef struct {
	cagent_handle_t   cagentHandle;
}susi_handler_context_t;

typedef struct{
	susi_handler_context_t susiHandlerContext;
	net_info_list netInfoList;
	CAGENT_MUTEX_TYPE netInfoListMutex;
	CAGENT_THREAD_TYPE getNetInfoThreadHandle;
	BOOL isGetNetInfoThreadRunning;

	//Handler_AutoReportStart
	CAGENT_COND_TYPE  netAutoUploadSyncObj; 
	CAGENT_MUTEX_TYPE netMonUploadSyncMutex;
	CAGENT_MUTEX_TYPE netAutoUploadSyncMutex;
	CAGENT_MUTEX_TYPE NMAutoUploadParamsMutex;
	BOOL isAutoUploadAllNet;
	bool isAutoUploadAll;
	BOOL isNetMonAutoUpload;
	sensor_info_list autoUploadRequestItemsList;
	CRITICAL_SECTION autoUploadRequestItemsListCS;
	BOOL IsNMAutoUploadThreadRunning;
	int autoUploadTimeoutStart;


	//Handler_Recv netmon_auto_report_req(533)
	CAGENT_COND_TYPE  netAutoReportSyncObj; 
	CAGENT_MUTEX_TYPE netMonReportSyncMutex;
	CAGENT_MUTEX_TYPE netAutoReportSyncMutex;
	CAGENT_MUTEX_TYPE NMAutoReportParamsMutex;
	BOOL isAutoReportAllNet;
	bool isAutoReportAll;
	BOOL isNetMonAutoReport;
	sensor_info_list autoReportRequestItemsList;
	CRITICAL_SECTION autoReportRequestItemsListCS;
	BOOL IsNMAutoReportThreadRunning;
	int autoReportTimeoutStart;

	
	//SA3.0
	CAGENT_MUTEX_TYPE autoParamsMutex30;
	CAGENT_THREAD_TYPE netMonUploadThreadHandle30;
	CAGENT_COND_TYPE  netMonUploadSyncObj30; 
	CAGENT_MUTEX_TYPE netMonUploadSyncMutex30;
	BOOL isNetMonUploadThreadRunning30;
	BOOL isNetMonAutoUpload30;
	int dataUploadIntervalMs30;
	int dataUploadTimeoutMs30;
	int timeoutStart30;

}netmon_handler_context_t;

typedef enum{
	unknown_cmd = 0,
	netmon_get_capability_req = 521,
	netmon_get_capability_rep = 522,
	netmon_get_sensors_data_req = 523,
	netmon_get_sensors_data_rep = 524,
	netmon_set_sensors_data_req = 525,
	netmon_set_sensors_data_rep = 526,
	netmon_get_sensors_data_error_rep = 598,
	//-----------------------------Net monitor command define(451--500)------------------------
	netmon_set_netinfo_auto_upload_req = 451,
	netmon_set_netinfo_auto_upload_rep = 452,
	netmon_set_netinfo_reqp_req = 453,
	netmon_set_netinfo_reqp_rep = 454,
	netmon_netinfo_req = 455,
	netmon_netinfo_rep = 456,
	netmon_set_thr_req = 457,
	netmon_set_thr_rep = 458,
	netmon_del_thr_req = 459,
	netmon_del_thr_rep = 460,
	netmon_thr_check_rep = 462,
	netmon_error_rep = 500,

	netmon_auto_report_req = 533,
	netmon_auto_report_rep = 534,
	//-----------------------------------------------------------------------------------------
}susi_comm_cmd_t;

typedef struct{
	int auto_upload_interval_ms;
	int auto_upload_timeout_ms;
}auto_upload_params_t;


#define COMM_DATA_WITH_JSON

typedef struct{
#ifdef COMM_DATA_WITH_JSON
	int reqestID;
#endif
	susi_comm_cmd_t comm_Cmd;
	int message_length;
	char message[];
}susi_comm_data_t;

typedef enum{
	netmon_send_cbf = 0,
	netmon_cust_cbf,
	netmon_subscribe_cust_cbf,
	netmon_send_report_cbf,
	netmon_send_capability_infospec_cbf,
}netmon_send_cbf_level_t;

#endif