#ifndef _HDD_THRESHOLDHELPER_
#define _HDD_THRESHOLDHELPER_

#include "platform.h"
#include "common.h"
#include <hwmhelper.h>
#include <cJSON.h>
#include "Parser.h"
#include "sqflashhelper.h"

#define DEF_THRESHOLD_CONFIG	"HDDThresholdCfg"



typedef enum{
	ck_type_unknow = 0,
	ck_type_max,
	ck_type_min,
	ck_type_avg,
	ck_type_bool,
} check_type_t;


typedef enum{
	value_type_unknow = 0,
	value_type_float,
	value_type_int,
} value_type_t;


typedef union check_value_t{
	float vf;
	int vi;
} check_value_t;


typedef struct check_value_node_t{
	check_value_t ckV;
	long long checkValueTime;
	struct check_value_node_t * next;
} check_value_node_t;


typedef struct check_value_list_t{
	check_value_node_t * head;
	int nodeCnt;
} check_value_list_t;


typedef struct sa_thr_item_t{
	char * tagPath;
	bool isEnable;
	bool isNormal;
	double maxThreshold;
	double minThreshold;
	int thresholdType;
	int lastingTimeS;
	int intervalTimeS;
	check_type_t checkType;
	check_value_t checkResultValue;
	check_value_list_t checkSourceValueList;
	long long repThrTime;
} sa_thr_item_t;


typedef struct sa_thr_node_t{
	sa_thr_item_t item;
	struct sa_thr_node_t *next;
} sa_thr_node_t;
typedef sa_thr_node_t *sa_thr_list_t;


typedef struct hdd_thr_node_t{
	char hddBaseName[256]; 
	sa_thr_list_t thrList;
	struct hdd_thr_node_t *next;
} hdd_thr_node_t;
typedef hdd_thr_node_t *hdd_thr_list_t;


hdd_thr_list_t Threshold_CreateHDDThrList(hdd_info_t * pHDDInfo);
cJSON * Threshold_ParseCmdToThrArray(void* data, int datalen);
cJSON * Threshold_ReadHDDThrCfg(char const * modulepath);
bool    Threshold_LoadOneRule(sa_thr_item_t * saRule, cJSON * objRule);
void    Threshold_LoadALLRules(hdd_thr_list_t hddThrList, cJSON * arrayRules);
bool    Threshold_MoveCache(hdd_thr_list_t oldList, hdd_thr_list_t newList);
bool    Threshold_ChkValue(sa_thr_node_t * pRuleNode, unsigned int value, char * checkMsg, bool *checkRet);
bool    Threshold_ChkHDDThrList(hdd_thr_list_t hddThrList, cJSON * pMonitor, bool * bNormal,char ** ppRepMsg);
bool    Threshold_ResetNormal(hdd_thr_list_t oldList, hdd_thr_list_t newList, char ** ppRepMsg);
bool    Threshold_WriteHDDThrCfg(char const * modulepath, char const * rules);
bool    Threshold_DestroyHDDThrList(hdd_thr_list_t hddThrList);


#endif

