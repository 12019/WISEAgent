#ifndef _MONITOR_PARSER_H_
#define _MONITOR_PARSER_H_

#include "platform.h"
#include <cJSON.h>
#include <hwmhelper.h>
#include <sqflashhelper.h>
#include "thresholdhelper.h"

#define AGENTINFO_BODY_STRUCT			"susiCommData"
#define AGENTINFO_REQID					"requestID"
#define AGENTINFO_CMDTYPE				"commCmd"
#define HWM_PLATFORM_INFO				"pfInfo"
#define HWM_TEMP_PFI_LIST				"temp"
#define HWM_VOLT_PFI_LIST				"volt"
#define HWM_FAN_PFI_LIST				"fan"
#define HWM_CURRENT_PFI_LIST			"current"
#define HWM_CASEOP_PFI_LIST				"caseopen"
#define HWM_OTHERS_PFI_LIST				"others"
#define HWM_PFI_TAG_NAME				"tag"
#define HWM_PFI_DISPLAY_NAME			"display"
#define HWM_PFI_UNIT					"unit"
#define HWM_PFI_MAX_THSHD				"max"
#define HWM_PFI_MIN_THSHD				"min"
#define HWM_PFI_THSHD_TYPE				"type"
#define HWM_HWM_INFO					"hwmInfo"
#define HWM_AUTOUPLOAD_PARAM			"autoUploadParams"
#define HWM_AUTOUPLOAD_INTERVAL_MS		"autoUploadIntervalMs"
#define HWM_AUTOUPLOAD_TIMEOUT_MS		"autoUploadTimeoutMs"
#define HWM_AUTOUPLOAD_INTERVAL_SEC		"autoUploadIntervalSec"
//#define HWM_AUTOUPLOAD_TIMEOUT_SEC		"autoUploadTimeoutSec"
#define HWM_AUTOUPLOAD_REQ_ITEMS		"requestItems"
#define HWM_THR_INFO					"hwmThr"
#define HWM_HDD_THR_LIST				"hdd"
#define THR_HEALTH_TAG				"healthH"
#define THR_TEMPH_TAG				"tempH"
#define HWM_THR_LASTING_TIME_S			"lastingTimeS"
#define HWM_THR_INTERVAL_TIME_S			"intervalTimeS"
#define HWM_THR_ENABLE					"enable"
#define DEF_INVALID_TIME                           (-1) 
#define DEF_INVALID_VALUE                          (-999)   
#define HWM_NORMAL_STATUS				"normalStatus"
#define HWM_MON_EVENT_MSG				"monEventMsg"

#define HDD_INFO                  "hddInfo"
#define HDD_MON_INFO_LIST         "hddMonInfoList"
#define HDD_TYPE                  "hddType"
#define HDD_NAME                  "hddName"
#define HDD_INDEX                 "hddIndex"
#define MAX_PROGRAM               "maxProgram"
#define AVERAGE_PROGRAM           "averageProgram"
#define ENDURANCE_CHECK           "enduranceCheck"
#define POWER_ON_TIME             "powerOnTime"
#define ECC_COUNT                 "eccCount"
#define MAX_RESERVED_BLOCK        "maxReservedBlock"
#define CURRENT_RESERVED_BLOCK    "currentReservedBlock"
#define GOOD_BLOCK_RATE           "goodBlockRate"
#define HDD_HEALTH_PERCENT        "hddHealthPercent"
#define HDD_TEMP                  "hddTemp"

#define HDD_SMART_INFO_LIST       "hddSmartInfoList"
#define SMART_ATTRI_INFO_LIST     "smartAttrList"
#define SMART_ATTRI_TYPE          "type"
#define SMART_ATTRI_NAME          "name"
#define SMART_ATTRI_FLAGS         "flags"
#define SMART_ATTRI_WORST         "worst"
#define SMART_ATTRI_THRESH         "thresh"
#define SMART_ATTRI_VALUE         "value"
#define SMART_ATTRI_VENDOR        "vendorData"
#define HWM_SPEC_TITLE			"HWM"
#define HWM_SPEC_NAME			"n"
#define HWM_SPEC_BASENAME		"bn"
#define HWM_SPEC_ID				"id"
#define HWM_SPEC_UNIT			"u"
#define HWM_SPEC_VALUE			"v"
#define HWM_SPEC_MAX			"max"
#define HWM_SPEC_MIN			"min"
#define HWM_SPEC_TYPE			"type"
#define HWM_SPEC_THRESHOLD_TYPE	"thd"
#define HWM_SPEC_EVENT			"e"
#define HWM_SPEC_DISKID			"diskid"
#define HWM_HDD_PFI_LIST		"hdd"
#define TYPE_THRESHOLD_UNKNOW		"none"
#define TYPE_THRESHOLD_MAX			"max"
#define TYPE_THRESHOLD_MIN			"min"
#define TYPE_THRESHOLDTHR_MAXMIN	"max-min"
#define TYPE_THRESHOLDTHR_BOOL		"bool"

bool parser_ParseReceivedData(void* data, int datalen, int * cmdID);
bool parser_ParsePeriodReportCmd(void* data, int datalen, int * interval, int * timeout);
bool parser_ParseThresholdCmd(void* data, int datalen, thr_items_t* itemlist);

bool parser_ReadThresholdConfig(char const * modulepath, thr_items_t* rules);
bool parser_WriteThresholdConfig(char const * modulepath, char const * rules);
cJSON * parser_CreateThresholdCheckResult(bool bResult, char* msg);

cJSON * parser_CreatePlatformInfo(hwm_info_t * pHWMInfo);
cJSON * parser_CreateHWMInfo(hwm_info_t * pHWMInfo);
cJSON * parser_CreateHDDInfo(hdd_info_t * pHDDInfo);
cJSON * parser_CreateHDDSMART(hdd_info_t * pHDDInfo);
cJSON * parser_CreateInfoSpec(hwm_info_t * pHWMInfo, hdd_info_t * pHDDInfo);

bool parser_ParseAutoUploadCmd(void* data, int datalen, int * interval, cJSON* reqItems);
bool parser_CreateHWMReport(hwm_info_t * pHWMInfo, cJSON * pENode);
bool parser_CreateHDDReport(hdd_info_t * pHDDInfo, cJSON * pENode);

#endif
