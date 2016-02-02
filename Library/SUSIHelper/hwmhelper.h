#ifndef _HWMONITOR_HELPER_H_
#define _HWMONITOR_HELPER_H_
#include "hwmmanage.h"
#include <stdbool.h>
#ifdef __cplusplus
extern "C" {
#endif

bool hwm_IsExistSUSILib();

bool hwm_StartupSUSILib();

bool hwm_CleanupSUSILib();

bool hwm_GetPlatformName(char* name, int length);

bool hwm_GetBIOSVersion(char* version, int length);

bool hwm_GetHWMInfo(hwm_info_t * pHWMInfo);

bool hwm_ReleaseHWMInfo(hwm_info_t * pHWMInfo);

hwm_item_t * hwm_FindItem(hwm_info_t * pHWMInfo, char const * tag);

#ifdef __cplusplus
}
#endif

#endif /* _HWMONITOR_HELPER_H_ */