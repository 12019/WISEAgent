#ifndef _HDD_HANDLER_H_
#define _HDD_HANDLER_H_


#include "common.h"
#include <cJSON.h>
#include <hwmhelper.h>
#include <sqflashhelper.h>
#include "ThresholdHelper.h"
#include "Parser.h"



#define cagent_request_hdd_monitoring 19
#define cagent_action_hdd_monitoring 113
//-----------------------------------------------------------------------------
// Types and defines:
//-----------------------------------------------------------------------------
#define MyTopic	 "HDDMonitor"
#define HDD_CHECK_PERIOD 2500

//-----------------------------------------------------------------------------
// Internal Prototypes:
//-----------------------------------------------------------------------------
typedef enum{
	unknown_cmd = 0,
   //------------------------Device monitor command define(1--10,251--300)---------------------
	devmon_set_hddinfo_auto_upload_req = 5,
	devmon_set_hddinfo_reqp_req = 6,
	devmon_hddinfo_req = 7,
	devmon_hddinfo_rep = 8,

	devmon_report_req = 253,
	devmon_report_rep = 254,
	devmon_set_thr_req = 257,
	devmon_set_thr_rep = 258,
	devmon_del_all_thr_req = 259,
	devmon_del_all_thr_rep = 260,

	devmon_set_hddinfo_auto_upload_rep = 273,
	devmon_set_hddinfo_reqp_rep = 274,
	devmon_get_hdd_smart_info_req = 275,
	devmon_get_hdd_smart_info_rep = 276,

	devmon_error_rep = 300,

	devmon_get_capability_req = 521,
	devmon_get_capability_rep = 522,
	devmon_get_sensors_data_req = 523,
	devmon_get_sensors_data_rep = 524,

	devmon_auto_upload_req = 533,
	devmon_auto_upload_rep = 534,

	devmon_get_sensors_data_error_rep = 598, 
}susi_comm_cmd_t;


typedef struct hdd_context_t{
	hdd_info_t        hddInfo;
	cJSON *           pMonitor;
	CAGENT_MUTEX_TYPE hddMutex;
}  hdd_context_t;


typedef struct thr_context_t{
	bool              bNormal;
	hdd_thr_list_t    thrList;
	CAGENT_MUTEX_TYPE thrMutex;
} thr_context_t;


typedef struct report_context_t{
	bool              bEnable;
	cJSON *           pRequestPathArray;
	int               iIntervalMs;
	int               iTimeoutMs;
	CAGENT_MUTEX_TYPE reportMutex;
} report_context_t;


typedef struct handler_context_t{
	void *                  threadHandler;
	bool                    bThreadRunning;
	bool                    bHasSQFlash;
	struct hdd_context_t    hddCtx;
	struct thr_context_t    thrCtx;
	struct report_context_t reportCtx;
	struct report_context_t uploadCtx;
} handler_context_t;


int Handler_Uninitialize();

#endif /* _HDD_HANDLER_H_ */
