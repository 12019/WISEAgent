#ifndef _HDD_PARSER_H_
#define _HDD_PARSER_H_

#include "platform.h"
#include "common.h"
#include <cJSON.h>
#include <hwmhelper.h>
#include <sqflashhelper.h>
#include "ThresholdHelper.h"

#define MARK_TK                     "#tk#"

#define AGENTINFO_BODY_STRUCT		"susiCommData"
#define AGENTINFO_REQID				"requestID"
#define AGENTINFO_CMDTYPE			"commCmd"
#define AGENTINFO_SENSORIDLIST		"sensorIDList"
#define AGENTINFO_SENSORINFOLIST	"sensorInfoList"
#define AGENTINFO_SESSIONID			"sessionID"

#define STATUS_SUCCESS				"Success"
#define STATUS_SETTING				"Setting"
#define STATUS_REQUEST_ERROR		"Request Error"
#define STATUS_NOT_FOUND			"Not Found"
#define STATUS_WRITE				"Write Only"
#define STATUS_READ					"Read Only"
#define STATUS_REQUEST_TIMEOUT		"Request Timeout"
#define STATUS_RESOURCE_LOSE		"Resource Lose"
#define STATUS_FORMAT_ERROR			"Format Error"
#define STATUS_OUTOF_RANGE			"Out of Range"
#define STATUS_SYNTAX_ERROR			"Syntax Error"
#define STATUS_LOCKED				"Resource Locked"
#define STATUS_FAIL					"Fail"
#define STATUS_NOT_IMPLEMENT		"Not Implement"
#define STATUS_SYS_BUSY				"Sys Busy"

#define STATUSCODE					"StatusCode"
#define STATUSCODE_SUCCESS			200
#define STATUSCODE_SETTING			202
#define STATUSCODE_REQUEST_ERROR	400
#define STATUSCODE_NOT_FOUND		404
#define STATUSCODE_WRITE			405
#define STATUSCODE_READ				405
#define STATUSCODE_REQUEST_TIMEOUT	408
#define STATUSCODE_RESOURCE_LOSE	410
#define STATUSCODE_FORMAT_ERROR		415
#define STATUSCODE_OUTOF_RANGE		416
#define STATUSCODE_SYNTAX_ERROR		422
#define STATUSCODE_LOCKED			426
#define STATUSCODE_FAIL				500
#define STATUSCODE_NOT_IMPLEMENT	401
#define STATUSCODE_SYS_BUSY			503

#define HDD_AUTOUPLOAD_PARAM		"autoUploadParams"
#define HDD_AUTOUPLOAD_INTERVAL_MS	"autoUploadIntervalMs"
#define HDD_AUTOUPLOAD_TIMEOUT_MS	"autoUploadTimeoutMs"
#define HDD_AUTOUPLOAD_INTERVAL_SEC	"autoUploadIntervalSec"
#define HDD_AUTOUPLOAD_TIMEOUT_SEC	"autoUploadTimeoutSec"
#define HDD_AUTOUPLOAD_REQ_ITEMS	"requestItems"
#define MAX_HDD_COUNT               32
#define MAX_PATH_LAYER				32
#define MAX_PATH_LENGTH             1024

#define DEF_INVALID_TIME			(-1) 
#define DEF_INVALID_VALUE			(-999)   
#define HDD_THR      				"hddThr"
#define THR_SET_THR_REP             "setThrRep"
#define THR_DEL_ALL_THR_REP         "delAllThrRep"
#define THR_CHECK_STATUS            "thrCheckStatus"
#define THR_CHECK_MSG				"thrCheckMsg"
#define THR_NORMAL                  "normal"

#define THR_MAX_THRESHOLD           "maxThreshold"
#define THR_MIN_THRESHOLD           "minThreshold"
#define THR_LASTING_TIME_S			"lastingTimeS"
#define THR_INTERVAL_TIME_S			"intervalTimeS"
#define THR_N                       "n"
#define THR_MAX						"max"
#define THR_MIN						"min"
#define THR_ENABLE					"enable"
#define THR_TYPE					"type"

#define THR_TYPE_UNKNOW		        "none"
#define THR_TYPE_MAX			    "max"
#define THR_TYPE_MIN			    "min"
#define THR_TYPETHR_MAXMIN	        "max-min"
#define THR_TYPETHR_BOOL		    "bool"


#define HDD_INFO					"hddInfo"
#define HDD_INFO_LIST				"hddInfoList"
#define HDD_MON_INFO_LIST			"hddMonInfoList"
#define HDD_SMART_INFO_LIST			"hddSmartInfoList"

#define HDD_TYPE_HDD				"STDDisk"
#define HDD_TYPE_SQFLASH			"SQFlash"

#define HDD_TYPE					"hddType"
#define HDD_NAME					"hddName"
#define HDD_INDEX					"hddIndex"
#define HDD_POWER_ON_TIME			"powerOnTime"

#define HDD_HEALTH_PERCENT			"hddHealthPercent"
#define HDD_TEMP					"hddTemp"

#define HDD_MAX_PROGRAM				"maxProgram"
#define HDD_AVERAGE_PROGRAM			"averageProgram"
#define HDD_ECC_COUNT				"eccCount"
#define HDD_ENDURANCE_CHECK			"enduranceCheck"
#define HDD_MAX_RESERVED_BLOCK		"maxReservedBlock"
#define HDD_CURRENT_RESERVED_BLOCK	"currentReservedBlock"
#define HDD_GOOD_BLOCK_RATE			"goodBlockRate"
#define HDD_HEALTH					"health"

#define SMART_BASEINFO				"BaseInfo"
#define SMART_ATTRI_INFO_LIST		"smartAttrList"
#define SMART_ATTRI_TYPE			"type"
#define SMART_ATTRI_NAME			"name"
#define SMART_ATTRI_FLAGS			"flags"
#define SMART_ATTRI_WORST			"worst"
#define SMART_ATTRI_THRESH			"thresh"
#define SMART_ATTRI_VALUE			"value"
#define SMART_ATTRI_VENDOR			"vendorData"

#define HDD_SPEC_UNIT				"u"
#define HDD_SPEC_VALUE				"v"
#define HDD_SPEC_SVALUE				"sv"
#define HDD_SPEC_MAX				"max"
#define HDD_SPEC_MIN				"min"
#define HDD_SPEC_TYPE				"type"
#define HDD_SPEC_THRESHOLD_TYPE		"thd"
#define HDD_SPEC_ELEMENT			"e"
#define HDD_SPEC_DISKID				"diskid"

#define HDD_SPEC_NAME				"n"
#define HDD_SPEC_BASENAME			"bn"
#define HDD_SPEC_VERSION			"ver"
#define HDD_SPEC_VERSION_DEF		1
#define HDD_SPEC_ASM				"asm"
#define HDD_SPEC_BASM_DEF			"R"
#define HDD_SPEC_BTYPE				"btype"
#define HDD_SPEC_BTYPE_DEF			"d"
#define HDD_SPEC_BST				"bst"
#define HDD_SPEC_BST_DEF			""
#define HDD_SPEC_BEXTEND			"bextend"
#define HDD_SPEC_BEXTEND_DEF		""

#define SENSOR_UNIT_PERCENT			"percent"
#define SENSOR_UNIT_HOUR			"hour"
#define SENSOR_UNIT_CELSIUS			"celsius"

// typedef struct report_context_t{
// 	bool              bEnable;
// 	cJSON *           pRequestPathArray;
// 	int               iIntervalMs;
// 	int               iTimeoutMs;
// 	CAGENT_MUTEX_TYPE reportMutex;
// } report_context_t;

bool Parser_ParseAutoReportCmd(void * data, bool * bEnable, cJSON ** ppRequestPathArray, int * intervalMs, int * timeoutMs );
bool Parser_ParsePathLayer( const char * path, char *layer[], int *cnt );
void Parser_ConvertGroupRules(cJSON * inRules, cJSON ** pOutRules , struct hdd_thr_node_t * hddThrList);
void Parser_DropInvalidRules(cJSON ** pRuleArray,cJSON * pMonitor);
int  Parser_GetSensorValueByPath(const char * path, cJSON* pMonitor);
bool Parser_CreateCustomReportOverall(cJSON ** ppCopyRoot, const cJSON * pSourceRoot, const cJSON * pEnodeOfPathArray, char * errMsg);


cJSON * Parser_CreateHDDSMART( hdd_info_t * pHDDInfo );
cJSON * Parser_CreateInfoSpec( hdd_info_t * pHDDInfo );
cJSON * Parser_CreateThrChkRet( bool bResult, char* chkMsg );
cJSON * Parser_CreateSensorInfo(cJSON * reqRoot, cJSON * hddRoot );


#endif
