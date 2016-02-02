#ifndef _GLOBAL_H_
#define _GLOBAL_H_

#include <stdbool.h>

#ifndef WIN32
#define SERVICE_STOPPED 0x00000001
#define SERVICE_START_PENDING 0x00000002
#define SERVICE_STOP_PENDING 0x00000003
#define SERVICE_RUNNING 0x00000004
#define SERVICE_CONTINUE_PENDING 0x00000005
#define SERVICE_PAUSE_PENDING 0x00000006
#define SERVICE_PAUSED 0x00000007
#endif

bool KillProcessWithID(unsigned long processID);

unsigned long RestartProcessWithID(unsigned long prcID);

bool IsSrvExist(char * srvName);

unsigned long StartSrv(char * srvName);

unsigned long GetSrvStatus(char * srvName);

unsigned long StopSrv(char * srvName);

#endif