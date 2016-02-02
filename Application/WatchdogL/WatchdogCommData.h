#ifndef _WATCHDOG_COMM_DATA_H_
#define _WATCHDOG_COMM_DATA_H_

#define DEF_KEEPALIVE_TIME_S         3
#define DEF_KEEPALIVE_TRY_TIMES      3

typedef enum WatchObjType
{
   FORM_PROCESS,
   NO_FORM_PROCESS,
   WIN_SERVICE,
}WATCHOBJTYPE;

typedef enum WatchCmdKey{
   START_WATCH,
   KEEPALIVE,
   BUSY_WAIT,
   STOP_WATCH,
}WATCHCMDKEY;

typedef union WatchParams{
   unsigned long busyWaitTimeS;
   struct {
      WATCHOBJTYPE  objType;
      unsigned long watchPID;
   }starWatchInfo;
}WATCHPARAMS;

typedef struct WatchMessage{
   unsigned long  commID;
   WATCHCMDKEY  commCmd;
   WATCHPARAMS  commParams;
}WATCHMSG, *PWATCHMSG;

#endif