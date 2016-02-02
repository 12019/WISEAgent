#ifndef _HWM3PARTY_HELPER_H_
#define _HWM3PARTY_HELPER_H_ 
#include "hwmmanage.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

 void GetHWM3PartyFunction(void * hHWM3PTYDLL);
 bool IsExistHWM3PartyLib();
 bool StartupHWM3PartyLib();
 bool CleanupHWM3PartyLib();
 unsigned int HWM3PartyHWMAvailable();
 bool HWM3PartyGetPlatformName(char *platformName, unsigned long *size);
 bool HWM3PartyGetHWMPlatformInfo(hwm_info_t * pHWMInfo);
 bool HWM3PartyGetHWMTempInfo(hwm_info_t * pHWMInfo);
 bool HWM3PartyGetHWMVoltInfo(hwm_info_t * pHWMInfo);
 bool HWM3PartyGetHWMFanInfo(hwm_info_t * pHWMInfo);
 bool HWM3PartyGetHWMCurrentInfo(hwm_info_t * pHWMInfo);
 bool HWM3PartyGetHWMCaseOpenInfo(hwm_info_t * pHWMInfo);
 /*BOOL HWM3PartyGetPFITempList(dev_mon_platform_info_item_list tempPFIList, PInsertDevMonPFIList pInsertDevMonPFIList);
 BOOL HWM3PartyGetPFIVoltList(dev_mon_platform_info_item_list voltPFIList, PInsertDevMonPFIList pInsertDevMonPFIList);
 BOOL HWM3PartyGetPFIFanList(dev_mon_platform_info_item_list fanPFIList, PInsertDevMonPFIList pInsertDevMonPFIList);

 BOOL HWM3PartyGetHWMTempInfo(dev_mon_platform_info_item_list * tempPFIList);
 BOOL HWM3PartyGetHWMVoltInfo(dev_mon_platform_info_item_list * voltPFIList);
 BOOL HWM3PartyGetHWMFanInfo(dev_mon_platform_info_item_list * fanPFIList);

 cJSON * HWM3PartyCreateHWMInfo(dev_mon_platform_info_item_list pPFIList);

 BOOL HWM3PartyGetStdaTempHwmi(dev_mon_platform_info_item_list tempPFIList, char * tempStr);
 BOOL HWM3PartyGetStdaVoltHwmi(dev_mon_platform_info_item_list voltPFIList, char * voltStr);
 BOOL HWM3PartyGetStdaFanHwmi(dev_mon_platform_info_item_list fanPFIList, char * fanStr);*/
#ifdef __cplusplus
}
#endif
#endif