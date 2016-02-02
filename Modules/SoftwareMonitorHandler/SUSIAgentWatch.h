#ifndef _SUSI_AGENT_WATCH_H_
#define _SUSI_AGENT_WATCH_H_

#include <Windows.h>

#define DEF_SELF_NAME              "CAgent.exe"            //Self name
#define DEF_PIPE_NAME              "\\\\.\\pipe\\SAWatchdogCommPipe"
#define DEF_COMM_ID                (1)
#define DEF_KEEPALIVE_INTERVAL_S   (2)

#define DEF_MAX_SELF_CPU_USAGE     70
#define DEF_MAX_SELF_MEM_USAGE     80000   //KB

#define DEVMON_THREAD1_FLAG        0x01
#define DEVMON_THREAD2_FLAG        0x02
#define SWM_THREAD1_FLAG           0x04

BOOL SUSIAgentStartWatch();

void SetWatchThreadFlag(int threadFlag);

BOOL SUSIAgentStopWatch();

#endif