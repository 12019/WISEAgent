
#ifndef _CAGENT_GATHERPLATFORMINFO_H_
#define _CAGENT_GATHERPLATFORMINFO_H_
#include "susiaccess_def.h"

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#include <Windows.h>
#ifndef SAGATHERINFO_API
#define SAGATHERINFO_API WINAPI
#endif
#else
#define SAGATHERINFO_API
#endif

#ifdef __cplusplus
extern "C" {
#endif

int SAGATHERINFO_API sagetInfo_gatherplatforminfo(susiaccess_agent_profile_body_t * profile);

#ifdef __cplusplus
}
#endif

#endif