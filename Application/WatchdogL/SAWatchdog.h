#ifndef _SA_WATCHDOG_H_
#define _SA_WATCHDOG_H_

#include "WatchdogCommData.h"
#include "Log.h"
#include <stdbool.h>
#include <pthread.h>

//-----------------------------------------SAWatchdog date define-------------------------------
typedef struct WatchMessageNode * PWATCHMSGNODE;
typedef struct WatchMessageNode
{
   WATCHMSG watchMsg;
   PWATCHMSGNODE next;
}WATCHMSGNODE;

typedef struct WatchMessageQueue{
   PWATCHMSGNODE head;
   PWATCHMSGNODE trail;
   int nodeCnt;
}WATCHMSGQUEUE, *PWATCHMSGQUEUE;

typedef struct WatchObjectConfigInfo
{
   unsigned long commID;
   char * processName;
   char * startupCmdLine;
}WATCHOBJCONFINFO, *PWATCHOBJCONFINFO;

typedef struct WatchObject{
   WATCHOBJCONFINFO  configInfo;
   WATCHOBJTYPE objType;
    unsigned long processID;
   PWATCHMSGQUEUE watchMsgQueue;
   pthread_t msgProcessThreadHandle;
   LOGHANDLE logHandle;
   bool   msgProcessThreadRun;
   pthread_mutex_t  watchMsgCS;
}WATCHOBJECT, *PWATCHOBJECT;

typedef struct WatchObjectNode *  PWATCHOBJECTNODE;
typedef struct WatchObjectNode{
   WATCHOBJECT watchObj;
   PWATCHOBJECTNODE next;
}WATCHOBJECTNODE;

typedef PWATCHOBJECTNODE WATCHOBJECTLIST;
//----------------------------------------------------------------------------------------------

bool StartSAWatchdog(void* loghandle);

bool StopSAWatchdog();

#endif