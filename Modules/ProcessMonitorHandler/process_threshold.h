#ifndef _PROCESS_THRESHOLD_H_
#define _PROCESS_THRESHOLD_H_
#include "process_common.h"

char MonObjConfigPath[MAX_PATH];
char MonObjTmpConfigPath[MAX_PATH];

mon_obj_info_node_t * CreateMonObjInfoList();
BOOL InitMonObjInfoListFromConfig(mon_obj_info_list monObjList, char *monObjConfigPath);
void ClearMonObjThr(mon_obj_info_t * curMonObj);
int DeleteAllMonObjInfoNode(mon_obj_info_node_t * head);
mon_obj_info_node_t * FindMonObjInfoNode(mon_obj_info_node_t * head, char * prcName , char * tagName);
mon_obj_info_node_t * FindMonObjInfoNodeWithID(mon_obj_info_node_t * head, int prcID , char * prcName ,char * tagName);
int InsertMonObjInfoList(mon_obj_info_node_t * head, mon_obj_info_t * monObjInfo);
void DestroyMonObjInfoList(mon_obj_info_node_t * head);
void SetThresholdConfigFile(mon_obj_info_list monObjList, char *monObjConfigPath);
int DeleteMonObjInfoNodeWithID(mon_obj_info_node_t * head, int prcID ,char * tagName);
prc_thr_type_t MonObjThrCheck(sw_thr_check_params_t * pThrCheckParams, BOOL *checkRet);
BOOL MonObjUpdate();
void SWMWhenDelThrSetToNormal(mon_obj_info_list monObjInfoList);
void SWMWhenDelThrCheckNormal(mon_obj_info_list monObjInfoList,mon_obj_info_list tmpMonObjInfoList);
void SWMDelAllMonObjs();
void SWMSetMonObjs(sw_mon_handler_context_t * pSWMonHandlerContext, char * requestData);
BOOL IsSWMThrNormal(BOOL * isNormal);
void SendNormalMsg(bool isTotalNormal, char *pNormalMsg);
#endif