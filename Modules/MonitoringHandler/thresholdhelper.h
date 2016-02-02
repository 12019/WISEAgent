
#ifndef _THRESHOLD_HELPER_H_
#define _THRESHOLD_HELPER_H_

#include "platform.h"
#include "sqflashhelper.h"
#include <hwmhelper.h>

#define DEF_TAG_NAME_LEN                           32


typedef enum{
	ck_type_unknow = 0,
	ck_type_max,
	ck_type_min,
	ck_type_avg,
	ck_type_bool,
}check_type_t;


typedef enum{
	value_type_unknow = 0,
	value_type_float,
	value_type_int,
}value_type_t;


typedef struct check_value_t{
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
	struct sa_thr_item_t * next;
}sa_thr_item_t;


typedef struct hdd_thr_item_t{
	char hdd_name[128];
	int hdd_index;
	sa_thr_item_t thrItem[2];
}hdd_thr_item_t;


typedef struct hdd_thr_item_node_t{
	hdd_thr_item_t hddThrItem;
	struct hdd_thr_item_node_t * next;
}hdd_thr_item_node_t;


typedef hdd_thr_item_node_t * hdd_thr_item_list;


typedef struct{
	int total;
	sa_thr_item_t * hwmRule;
	hdd_thr_item_list hddRule;
}thr_items_t;


bool threshold_LoadOneRule(sa_thr_item_t * rule, char * tag, float maxThreshold, float minThreshold, int thresholdType, int isEnable, int lasting, int interval);
bool threshold_InitHDDThrList(hdd_thr_item_list * pHDDThrList, hdd_info_t * pHDDinfo);
void threshold_UninitHDDThrList(hdd_thr_item_list * pHDDThrList);
bool threshold_RenewHDDThrList(hdd_thr_item_list * pHDDThrList, hdd_info_t * pHDDInfo);
hdd_thr_item_node_t * threshold_CreateHDDThrDefaultNode();

sa_thr_item_t* threshold_FindItem(thr_items_t* itemlist, char const * tag);
sa_thr_item_t* threshold_AddThresholdItem(thr_items_t* itemlist, char * tag, float maxThreshold, float minThreshold, int thresholdType, int isEnable, int lasting, int interval, char * retMsg);
bool threshold_RemoveThresholdItem(thr_items_t* thrList, char const * tag, char * retMsg);
bool threshold_ClearAllThresholdItem(thr_items_t* itemlist);
bool threshold_ChkOneRule(sa_thr_item_t * rule, float value, char * checkMsg, bool *checkRet);
bool threshold_ChkHDD(hdd_thr_item_list HDDThrList, hdd_info_t * pHDDInfo, char * retMsg, bool *bHealth);
// bool Handler_ChkHWMThrRule(thr_items_t* pRules, hwm_info_t * pHWMInfo, char * msg, bool *bHealth)

bool threshold_SyncHWMRule(thr_items_t* source, thr_items_t* target, char * retMsg);
bool threshold_SyncHDDRule(hdd_thr_item_list * source, hdd_thr_item_list * target, hdd_info_t * pHDDInfo, char * retMsg);


#endif