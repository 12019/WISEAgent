#ifndef _ADVCARE_HELPER_H_
#define _ADVCARE_HELPER_H_
#include <stdbool.h>
#ifdef __cplusplus
extern "C" {
#endif

bool advcare_IsExistAdvCareLib();

bool advcare_StartupAdvCareLib();

bool advcare_CleanupAdvCareLib();

bool advcare_GetPlatformName(char* name, int length);

bool advcare_GetBIOSVersion(char* version, int length);

#ifdef __cplusplus
}
#endif

#endif /* _ADVCARE_HELPER_H_ */