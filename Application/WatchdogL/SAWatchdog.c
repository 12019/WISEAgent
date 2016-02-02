#include "SAWatchdog.h"
#include "NamedPipeServer.h"
#include "Global.h"
#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include "util_path.h"

#define DEF_PIPE_INST_COUNT                 (10)
#ifdef WIN32
#define DEF_PIPE_NAME                       "\\\\.\\pipe\\SAWatchdogCommPipe"
#else
#define DEF_PIPE_NAME			    "/tmp/SAWatchdogFifo"
#endif

#define DEF_WATCH_OBJ_CONFIG_FILE_NAME      "SAWatchdog_Config"
#define DEF_WATCHDOG_LOG_FILE_NAME          "SAWatchdogLog.txt"

WATCHOBJECTLIST WatchObjList = NULL;
char ModulePath[MAX_PATH] = {0};
char WatchObjConfigFilePath[MAX_PATH] = {0};
char DefWatchdogLogFilePath[MAX_PATH] = {0};
LOGHANDLE DefLogHandle = NULL;
#define DEF_SRVC_LOG_MODE    (LOG_MODE_CONSOLE_OUT|LOG_MODE_FILE_OUT)
#define wdLog(objLoghandle, level, fmt, ...)  do { if (objLoghandle != NULL)   \
   WriteIndividualLog(objLoghandle, "watchdog", DEF_SRVC_LOG_MODE, level, fmt, ##__VA_ARGS__); } while(0)

//----------------------------Watch object list function define--------------------------
static WATCHOBJECTLIST CreateWatchObjList();
static void DestroyWatchObjList(WATCHOBJECTLIST watchObjList);
static int InsertWatchObjNode(WATCHOBJECTLIST watchObjList, PWATCHOBJCONFINFO pWatchObjConfInfo);
static PWATCHOBJECTNODE FindWatchObjNode(WATCHOBJECTLIST watchObjList, unsigned long commID);
static bool DeleteWatchObjNode(WATCHOBJECTLIST watchObjList, unsigned long commID);
static bool DeleteAllWatchObjNode(WATCHOBJECTLIST watchObjList);
//---------------------------------------------------------------------------------------

//----------------------------IPC message queue function define--------------------------
static PWATCHMSGQUEUE InitWatchMsgQueue();
static void DestroyWatchMsgQueue(PWATCHMSGQUEUE pWatchMsgQueue);
static bool WatchMsgEnqueue(PWATCHMSGQUEUE pWatchMsgQueue, PWATCHMSG pWatchMsg);
static PWATCHMSGNODE WatchMsgDequeue(PWATCHMSGQUEUE pWatchMsgQueue);
//---------------------------------------------------------------------------------------
//----------------------------Other function define--------------------------------------
static bool WatchMsgDispatchCB(char* recvData, int recvLen);
static bool ParseConfigFile(WATCHOBJECTLIST watchObjList, char * configFilePath);
static bool LaunchWatch(WATCHOBJECTLIST watchObjList);
static bool UnlaunchWatch(WATCHOBJECTLIST watchObjList);
static bool InitSAWatchdog();
static bool UninitSAWatchdog();
//---------------------------------------------------------------------------------------

static WATCHOBJECTLIST CreateWatchObjList()
{
   PWATCHOBJECTNODE head = NULL;
   head = (PWATCHOBJECTNODE)malloc(sizeof(WATCHOBJECTNODE));
   if(head) 
   {
	   memset(head, 0, sizeof(WATCHOBJECTNODE));
      //head->next = NULL;
      //head->watchObj.configInfo.commID = 0;
      //head->watchObj.watchMsgQueue = NULL;
      //head->watchObj.msgProcessThreadHandle = NULL;
      head->watchObj.msgProcessThreadRun = false;
      head->watchObj.processID = 0;
      head->watchObj.objType = FORM_PROCESS;
      //head->watchObj.configInfo.processName = NULL;
      //head->watchObj.configInfo.startupCmdLine = NULL;
      //head->watchObj.logHandle = NULL;
      //head->watchObj.watchMsgCS = NULL;
   }
   return head;
}

static void DestroyWatchObjList(WATCHOBJECTLIST watchObjList)
{
   PWATCHOBJECTNODE head = watchObjList;
   if(NULL == head) return;
   DeleteAllWatchObjNode(head);
   free(head); 
   head = NULL;
}

static int InsertWatchObjNode(WATCHOBJECTLIST watchObjList, PWATCHOBJCONFINFO pWatchObjConfInfo)
{
   int iRet = -1;
   PWATCHOBJECTNODE newNode = NULL;
   PWATCHOBJECTNODE findNode = NULL;
   PWATCHOBJECTNODE head = watchObjList;
   if(head == NULL || pWatchObjConfInfo == NULL) return iRet;
   findNode = FindWatchObjNode(head, pWatchObjConfInfo->commID);
   if(findNode == NULL)
   {
      newNode = (PWATCHOBJECTNODE)malloc(sizeof(WATCHOBJECTNODE));
      memset(newNode, 0, sizeof(WATCHOBJECTNODE));
      if(pWatchObjConfInfo->processName)
      {
         int strLen = strlen(pWatchObjConfInfo->processName);
         newNode->watchObj.configInfo.processName = (char *)malloc(strLen + 1);
         memset(newNode->watchObj.configInfo.processName, 0, strLen + 1);
         memcpy(newNode->watchObj.configInfo.processName, pWatchObjConfInfo->processName, strLen);
      }
      /*else
      {
         newNode->watchObj.configInfo.processName = NULL;
      }*/

      if(pWatchObjConfInfo->startupCmdLine)
      {
         int strLen = strlen(pWatchObjConfInfo->startupCmdLine);
         newNode->watchObj.configInfo.startupCmdLine = (char *)malloc(strLen + 1);
         memset(newNode->watchObj.configInfo.startupCmdLine, 0, strLen + 1);
         memcpy(newNode->watchObj.configInfo.startupCmdLine, pWatchObjConfInfo->startupCmdLine, strLen);
      }
     /* else
      {
         newNode->watchObj.configInfo.startupCmdLine = NULL;
      }*/

      newNode->watchObj.configInfo.commID = pWatchObjConfInfo->commID;
      //newNode->watchObj.watchMsgQueue = NULL;
      newNode->watchObj.objType = FORM_PROCESS;
      //newNode->watchObj.msgProcessThreadHandle = NULL;
      //newNode->watchObj.msgProcessThreadRun = false;
      //newNode->watchObj.processID = 0;
      //newNode->watchObj.logHandle = NULL;
      //newNode->watchObj.watchMsgCS = NULL;

      newNode->next = head->next;
      head->next = newNode;
      iRet = 0;
   }
   else
   {
      iRet = 1;
   }
   return iRet;
}

static PWATCHOBJECTNODE FindWatchObjNode(WATCHOBJECTLIST watchObjList, unsigned long commID)
{
   PWATCHOBJECTNODE head = watchObjList;
   PWATCHOBJECTNODE findNode = NULL;
   if(head == NULL) return findNode;
   findNode = head->next;
   while(findNode)
   {
      if(findNode->watchObj.configInfo.commID == commID) break;
      else
      {
         findNode = findNode->next;
      }
   }

   return findNode;
}

static bool DeleteWatchObjNode(WATCHOBJECTLIST watchObjList, unsigned long commID)
{
   bool bRet = false;
   PWATCHOBJECTNODE delNode = NULL;
   PWATCHOBJECTNODE p = NULL;
   PWATCHOBJECTNODE head = watchObjList;
   if(head == NULL) return bRet;
   p = head;
   delNode = head->next;
   while(delNode)
   {
      if(delNode->watchObj.configInfo.commID == commID) 
      {
         p->next = delNode->next;

         if(delNode->watchObj.msgProcessThreadHandle)
         {
            delNode->watchObj.msgProcessThreadRun = false;
			pthread_join(delNode->watchObj.msgProcessThreadHandle, NULL);
            //WaitForSingleObject(delNode->watchObj.msgProcessThreadHandle, INFINITE);
            //CloseHandle(delNode->watchObj.msgProcessThreadHandle);
            delNode->watchObj.msgProcessThreadHandle = 0;
         }

         if(delNode->watchObj.watchMsgQueue)
         {
            DestroyWatchMsgQueue(delNode->watchObj.watchMsgQueue);
            free(delNode->watchObj.watchMsgQueue);
            delNode->watchObj.watchMsgQueue = NULL;
         }

         if(delNode->watchObj.logHandle)
         {
            UninitLog(delNode->watchObj.logHandle);
            delNode->watchObj.logHandle = NULL;
         }

         if(delNode->watchObj.configInfo.processName)
         {
            free(delNode->watchObj.configInfo.processName);
            delNode->watchObj.configInfo.processName = NULL;
         }

         if(delNode->watchObj.configInfo.startupCmdLine)
         {
            free(delNode->watchObj.configInfo.startupCmdLine);
            delNode->watchObj.configInfo.startupCmdLine = NULL;
         }

         free(delNode);
         delNode = NULL;
         bRet = true;
         break;
      }
      else
      {
         p = delNode;
         delNode = delNode->next;
      }
   }
   return bRet;
}

static bool DeleteAllWatchObjNode(WATCHOBJECTLIST watchObjList)
{
   bool bRet = false;
   PWATCHOBJECTNODE delNode=NULL;
   PWATCHOBJECTNODE head = watchObjList;
   if(NULL == head) return bRet;

   delNode = head->next;
   while(delNode)
   {
      head->next = delNode->next;

      if(delNode->watchObj.msgProcessThreadHandle)
      {
         delNode->watchObj.msgProcessThreadRun = false;
		 pthread_join(delNode->watchObj.msgProcessThreadHandle, NULL);
        /* WaitForSingleObject(delNode->watchObj.msgProcessThreadHandle, INFINITE);
         CloseHandle(delNode->watchObj.msgProcessThreadHandle);*/
         delNode->watchObj.msgProcessThreadHandle = 0;
      }
      if(delNode->watchObj.watchMsgQueue)
      {
         DestroyWatchMsgQueue(delNode->watchObj.watchMsgQueue);
         free(delNode->watchObj.watchMsgQueue);
         delNode->watchObj.watchMsgQueue = NULL;
      }
      if(delNode->watchObj.logHandle)
      {
         UninitLog(delNode->watchObj.logHandle);
         delNode->watchObj.logHandle = NULL;
      }
      if(delNode->watchObj.configInfo.processName)
      {
         free(delNode->watchObj.configInfo.processName);
         delNode->watchObj.configInfo.processName = NULL;
      }
      if(delNode->watchObj.configInfo.startupCmdLine)
      {
         free(delNode->watchObj.configInfo.startupCmdLine);
         delNode->watchObj.configInfo.startupCmdLine = NULL;
      }
      free(delNode);
      delNode = head->next;
   }

   bRet = true;
   return bRet;
}

static PWATCHMSGQUEUE InitWatchMsgQueue()
{
   PWATCHMSGQUEUE pWatchMsgQueue = NULL;
   pWatchMsgQueue = (PWATCHMSGQUEUE)malloc(sizeof(WATCHMSGQUEUE));
   if(pWatchMsgQueue)
   {
      pWatchMsgQueue->head = NULL;
      pWatchMsgQueue->trail = NULL;
      pWatchMsgQueue->nodeCnt = 0;
   }
   return pWatchMsgQueue;
}

static void DestroyWatchMsgQueue(PWATCHMSGQUEUE pWatchMsgQueue)
{
   if(pWatchMsgQueue)
   {
      PWATCHMSGNODE pWatchMsgNode = WatchMsgDequeue(pWatchMsgQueue);
      while(pWatchMsgNode)
      {
         free(pWatchMsgNode);
         pWatchMsgNode = WatchMsgDequeue(pWatchMsgQueue);;   
      }
      pWatchMsgQueue->head = NULL;
      pWatchMsgQueue->trail = NULL;
      pWatchMsgQueue->nodeCnt = 0;
   }
}

static bool WatchMsgEnqueue(PWATCHMSGQUEUE pWatchMsgQueue, PWATCHMSG pWatchMsg)
{
   int bRet = false;
   if((pWatchMsgQueue) && (pWatchMsg))
   {
      PWATCHMSGNODE pNewNode = (PWATCHMSGNODE)malloc(sizeof(WATCHMSGNODE));
      if(pNewNode)
      {
	 memset(pNewNode, 0, sizeof(WATCHMSGNODE));
         pNewNode->watchMsg.commCmd = pWatchMsg->commCmd;
         pNewNode->watchMsg.commID = pWatchMsg->commID;
         memcpy(&(pNewNode->watchMsg.commParams), &(pWatchMsg->commParams), sizeof(WATCHPARAMS));

         if(pWatchMsgQueue->trail != NULL)
         {
            pWatchMsgQueue->trail->next = pNewNode;
         }

         pWatchMsgQueue->trail = pNewNode;
         pWatchMsgQueue->nodeCnt++;
         pWatchMsgQueue->trail->next = NULL;

         if(pWatchMsgQueue->head == NULL)
         {
            pWatchMsgQueue->head = pWatchMsgQueue->trail;
         }
         bRet = true;
      }
   }
   return bRet;
}

static PWATCHMSGNODE WatchMsgDequeue(PWATCHMSGQUEUE pWatchMsgQueue)
{
   PWATCHMSGNODE pQNode = NULL;
   if(!pWatchMsgQueue || !pWatchMsgQueue->head) return NULL;
   pQNode = pWatchMsgQueue->head;
   pWatchMsgQueue->head = pQNode->next;
   pWatchMsgQueue->nodeCnt--;
   if(pQNode == pWatchMsgQueue->trail)
   {
      pWatchMsgQueue->trail = NULL;
   }
   return pQNode;
}


static bool WatchMsgDispatchCB(char* recvData, int recvLen)
{
   bool bRet = false;
   if(recvData == NULL || recvLen != sizeof(WATCHMSG)) return bRet;
   if(WatchObjList == NULL) return bRet;
   {
      PWATCHMSG pWatchMsg = (PWATCHMSG) recvData;
      PWATCHOBJECTNODE pWatchObjNode = FindWatchObjNode(WatchObjList, pWatchMsg->commID);
      if(pWatchObjNode)
      {
         //if(pWatchObjNode->watchObj.watchMsgCS)
         //{
            //EnterCriticalSection(pWatchObjNode->watchObj.watchMsgCS);
	    pthread_mutex_lock(&pWatchObjNode->watchObj.watchMsgCS);
         //}
         if(pWatchObjNode->watchObj.watchMsgQueue)
         {
            bRet = WatchMsgEnqueue(pWatchObjNode->watchObj.watchMsgQueue, pWatchMsg);
         }
         //if(pWatchObjNode->watchObj.watchMsgCS)
         //{
            //LeaveCriticalSection(pWatchObjNode->watchObj.watchMsgCS);
	    pthread_mutex_unlock(&pWatchObjNode->watchObj.watchMsgCS);
         //}
      }
   }
   return bRet;
}

void* threat_watchobjecte(void* pThreadParam)
{
   PWATCHOBJECT pCurWatchObj = (PWATCHOBJECT)pThreadParam;
   PWATCHMSGNODE pWatchMsgNode = NULL;
   bool isWatch = false;
   int notContinueKeepaliveCnt = 0;
   int keepaliveTime = DEF_KEEPALIVE_TIME_S;
   int keepaliveTryTimes = DEF_KEEPALIVE_TRY_TIMES;
   time_t lastKeepaliveTime = 0;
   time_t curKeepaliveTime = 0;
   bool isKeepaliveTryTimesOut = false;
   while(pCurWatchObj->msgProcessThreadRun)
   {
      if(pCurWatchObj->watchMsgQueue)
      {
         //EnterCriticalSection(pCurWatchObj->watchMsgCS);
	pthread_mutex_lock(&pCurWatchObj->watchMsgCS);
         pWatchMsgNode = WatchMsgDequeue(pCurWatchObj->watchMsgQueue);
         //LeaveCriticalSection(pCurWatchObj->watchMsgCS);
	pthread_mutex_unlock(&pCurWatchObj->watchMsgCS);
         if(pWatchMsgNode)
         {
            switch(pWatchMsgNode->watchMsg.commCmd)
            {
            case START_WATCH:
               {
                  isWatch = true;
                  notContinueKeepaliveCnt = 0;
                  isKeepaliveTryTimesOut = false;
                  pCurWatchObj->processID = pWatchMsgNode->watchMsg.commParams.starWatchInfo.watchPID;
                  pCurWatchObj->objType = pWatchMsgNode->watchMsg.commParams.starWatchInfo.objType;
                  wdLog(pCurWatchObj->logHandle, Normal, "---%s:%d--- start monitor...", pCurWatchObj->configInfo.processName, pCurWatchObj->processID);
                  lastKeepaliveTime = time(NULL);
                  break;
               }
            case KEEPALIVE:
               {
                  //wdLog(pCurWatchObj->logHandle, Debug, "---%s:%d--- feed the dog!", pCurWatchObj->configInfo.processName, pCurWatchObj->processID);
                  lastKeepaliveTime = time(NULL);
                  break;
               }
            case BUSY_WAIT:
               {
                  wdLog(pCurWatchObj->logHandle, Normal, "---%s:%d--- busy wait %d second!", pCurWatchObj->configInfo.processName, pCurWatchObj->processID, pWatchMsgNode->watchMsg.commParams.busyWaitTimeS);
                  usleep(pWatchMsgNode->watchMsg.commParams.busyWaitTimeS * 1000*1000);
                  wdLog(pCurWatchObj->logHandle, Normal, "---%s:%d--- busy wait out, monitoring...", pCurWatchObj->configInfo.processName, pCurWatchObj->processID);
                  lastKeepaliveTime = time(NULL);
                  break;
               }
            case STOP_WATCH:
               {
                  wdLog(pCurWatchObj->logHandle, Normal, "---%s:%d--- stop monitor!", pCurWatchObj->configInfo.processName, pCurWatchObj->processID);
                  isWatch = false;
                  break;
               }
            default: break;
            }
            free(pWatchMsgNode);
            pWatchMsgNode = NULL;
         }
      }

      if(isWatch)
      {
         curKeepaliveTime = time(NULL);
         if(curKeepaliveTime - lastKeepaliveTime >= keepaliveTime)
         {
            notContinueKeepaliveCnt++;
            if(notContinueKeepaliveCnt >= keepaliveTryTimes)
            {
               isWatch = false;
               lastKeepaliveTime = 0;
               curKeepaliveTime = 0;
               isKeepaliveTryTimesOut = true;
               notContinueKeepaliveCnt = 0;
            }
         }
         else
         {
            notContinueKeepaliveCnt = 0;
         }
      }

      if(isKeepaliveTryTimesOut)
      {
         wdLog(pCurWatchObj->logHandle, Warning, "---%s:%d--- Can't recive keepalive %d times, will restart the process!", pCurWatchObj->configInfo.processName, pCurWatchObj->processID, keepaliveTryTimes);

         if(pCurWatchObj->objType == FORM_PROCESS || pCurWatchObj->objType ==  NO_FORM_PROCESS)
         {
			 if(RestartProcessWithID(pCurWatchObj->processID)!=0)
				 isKeepaliveTryTimesOut = false;
         }
         else if(pCurWatchObj->objType == WIN_SERVICE)
         {
            if(pCurWatchObj->configInfo.startupCmdLine)
            {
               if(IsSrvExist(pCurWatchObj->configInfo.startupCmdLine))
               {
                  unsigned long dwRet = GetSrvStatus(pCurWatchObj->configInfo.startupCmdLine);
                  if(dwRet == SERVICE_RUNNING)
                  {
                     dwRet = StopSrv(pCurWatchObj->configInfo.startupCmdLine);
                     if(dwRet != SERVICE_STOPPED)
                     {
                        wdLog(pCurWatchObj->logHandle, Warning, "---%s:%d--- Stop the win service:%s failed!", pCurWatchObj->configInfo.processName, pCurWatchObj->processID, pCurWatchObj->configInfo.startupCmdLine);
                     }
                  }
                  if(dwRet != SERVICE_RUNNING)
                  {
                     dwRet = StartSrv(pCurWatchObj->configInfo.startupCmdLine);
                     if(dwRet == SERVICE_RUNNING)
                     {
                        isKeepaliveTryTimesOut = false;
                     }
                  }
               }
               else
               {
                  wdLog(pCurWatchObj->logHandle, Warning, "---%s:%d--- Not found the win service:%s!", pCurWatchObj->configInfo.processName, pCurWatchObj->processID, pCurWatchObj->configInfo.startupCmdLine);
               }
            }
         }

         if(!isKeepaliveTryTimesOut)
         {
            wdLog(pCurWatchObj->logHandle, Warning, "---ProcName:%s-CommID:%d--- Restart the watch object successfully!", pCurWatchObj->configInfo.processName, pCurWatchObj->configInfo.commID);
         }
         else
         {
            wdLog(pCurWatchObj->logHandle, Warning, "---ProcName:%s-CommID:%d--- Restart the the watch object failed!", pCurWatchObj->configInfo.processName, pCurWatchObj->configInfo.commID);
            usleep(2000*1000);
         }
      }

      usleep(100*1000);
   }

   wdLog(pCurWatchObj->logHandle, Normal, "---ProcName:%s-CommID:%d--- Watch thread stop!", pCurWatchObj->configInfo.processName, pCurWatchObj->configInfo.commID);
   return 0;
}

static bool ParseConfigFile(WATCHOBJECTLIST watchObjList, char * configFilePath)
{
   bool bRet = false;
   if(watchObjList == NULL || configFilePath == NULL) return bRet;
   {
      FILE * fConfig = NULL;
      char lineBuf[2048] = {0};
      char * cfgItems[8] = {NULL};
      char * buf = NULL;
      int i = 0, k = 0;
      WATCHOBJCONFINFO watchObjInfo;
      watchObjInfo.processName = NULL;
      watchObjInfo.startupCmdLine = NULL;
      watchObjInfo.commID = 0;
      if((fConfig = fopen(configFilePath, "rb")) == NULL) return bRet;
      while(fgets(lineBuf, sizeof(lineBuf), fConfig) != NULL)
      { 
         i = 0;
         k = 0;
         memset(cfgItems, 0, sizeof(cfgItems));
         buf = lineBuf;
         while((cfgItems[i] = strtok( buf, ";\r\n" )) != NULL)
         {
            i++;
            buf=NULL; 
         }
         for(k = 0; k<i; k++)
         {
            if(cfgItems[k] != NULL)
            {
               int j = 0;
               char * itemValue[4] = {NULL};
               char * itemBuf = cfgItems[k];
               while ((itemValue[j] = strtok(itemBuf, "=\t\v\f\n\r")) != NULL)
               {
                  j++;
                  itemBuf = NULL;
               }
               if(itemValue[0] != NULL && !strcmp(itemValue[0], "ProcName"))
               {
                  if(itemValue[1] != NULL)
                  {
                     int tmpLen = strlen(itemValue[1]);
                     watchObjInfo.processName = (char *)malloc(tmpLen + 1);
                     memset(watchObjInfo.processName, 0, tmpLen + 1);
                     memcpy(watchObjInfo.processName, itemValue[1], tmpLen);
                  }
               }
               if(itemValue[0] != NULL && !strcmp(itemValue[0], "CommID"))
               {
                  if(itemValue[1] != NULL)
                  {
                     watchObjInfo.commID = atoi(itemValue[1]);
                  }
               }
               if(itemValue[0] != NULL && !strcmp(itemValue[0], "StartupCmdLine"))
               {
                  if(itemValue[1] != NULL)
                  {
                     int tmpLen = strlen(itemValue[1]);
                     watchObjInfo.startupCmdLine = (char *)malloc(tmpLen + 1);
                     memset(watchObjInfo.startupCmdLine, 0, tmpLen + 1);
                     memcpy(watchObjInfo.startupCmdLine, itemValue[1], tmpLen);
                  }
               }
            }
         }

         if(watchObjInfo.processName && watchObjInfo.startupCmdLine && watchObjInfo.commID > 0 && WatchObjList) 
         {
				InsertWatchObjNode(WatchObjList, &watchObjInfo);
         }
         if(watchObjInfo.processName)
         {
            free(watchObjInfo.processName);
            watchObjInfo.processName = NULL;
         }
         if(watchObjInfo.startupCmdLine)
         {
            free(watchObjInfo.startupCmdLine);
            watchObjInfo.startupCmdLine = NULL;
         }
         watchObjInfo.commID = 0;
         memset(lineBuf, 0, sizeof(lineBuf));
      }
      fclose(fConfig);
      bRet = true;
   }
   return bRet;
}

static bool LaunchWatch(WATCHOBJECTLIST watchObjList)
{
   bool bRet = false;
   if(watchObjList == NULL) return bRet;
   {
      PWATCHOBJECTNODE  pCurWatchObjNode = watchObjList->next;
      while(pCurWatchObjNode)
      {
         //pCurWatchObjNode->watchObj.watchMsgCS = (PCRITICAL_SECTION)malloc(sizeof(CRITICAL_SECTION));
         //InitializeCriticalSection(pCurWatchObjNode->watchObj.watchMsgCS);
         pthread_mutex_init(&pCurWatchObjNode->watchObj.watchMsgCS, NULL);
         
         if(!pCurWatchObjNode->watchObj.logHandle) 
         {
            pCurWatchObjNode->watchObj.logHandle = DefLogHandle;
         }
         pCurWatchObjNode->watchObj.watchMsgQueue = InitWatchMsgQueue();
         pCurWatchObjNode->watchObj.msgProcessThreadRun = true;
         if(pthread_create(&pCurWatchObjNode->watchObj.msgProcessThreadHandle, NULL, threat_watchobjecte, &pCurWatchObjNode->watchObj) != 0)
         {
            pCurWatchObjNode->watchObj.msgProcessThreadRun = false;
         }
         pCurWatchObjNode = pCurWatchObjNode->next;
      }
   }
   bRet = true;
   return bRet;
}

static bool UnlaunchWatch(WATCHOBJECTLIST watchObjList)
{
   bool bRet = false;
   if(watchObjList == NULL) return bRet;
   {
      PWATCHOBJECTNODE  pCurWatchObjNode = watchObjList->next;
      while(pCurWatchObjNode)
      {
         if(pCurWatchObjNode->watchObj.msgProcessThreadHandle)
         {
            pCurWatchObjNode->watchObj.msgProcessThreadRun = false;
			pthread_join(pCurWatchObjNode->watchObj.msgProcessThreadHandle, NULL);
            pCurWatchObjNode->watchObj.msgProcessThreadHandle = 0;
         }

         if(pCurWatchObjNode->watchObj.logHandle && pCurWatchObjNode->watchObj.logHandle != DefLogHandle) UninitLog(pCurWatchObjNode->watchObj.logHandle);

         if(pCurWatchObjNode->watchObj.watchMsgQueue)
         {
            DestroyWatchMsgQueue(pCurWatchObjNode->watchObj.watchMsgQueue);
         }

         //if(pCurWatchObjNode->watchObj.watchMsgCS)
         //{
            //DeleteCriticalSection(pCurWatchObjNode->watchObj.watchMsgCS);
            //free(pCurWatchObjNode->watchObj.watchMsgCS);
            //pCurWatchObjNode->watchObj.watchMsgCS = NULL;
			 pthread_mutex_destroy(&pCurWatchObjNode->watchObj.watchMsgCS);
         //}
         pCurWatchObjNode = pCurWatchObjNode->next;
      }
   }
   bRet = true;
   return bRet;
}

static bool InitSAWatchdog()
{
   bool bRet = true;
   util_module_path_get(ModulePath);
   sprintf(WatchObjConfigFilePath, "%s%s", ModulePath, DEF_WATCH_OBJ_CONFIG_FILE_NAME);
   //sprintf_s(DefWatchdogLogFilePath, sizeof(DefWatchdogLogFilePath), "%s", ModulePath);
   //DefLogHandle = InitLog(DefWatchdogLogFilePath);
   WatchObjList = CreateWatchObjList();

   bRet = ParseConfigFile(WatchObjList, WatchObjConfigFilePath);
   return bRet;
}

static bool UninitSAWatchdog()
{
   bool bRet = true;
   if(WatchObjList)
   {
      DestroyWatchObjList(WatchObjList);
   }
   //UninitLog(DefLogHandle);
   return bRet;
}

bool StartSAWatchdog(void* loghandle)
{
   bool bRet = true;
   DefLogHandle = loghandle;
   bRet = InitSAWatchdog();
   if(bRet)
   {
      bRet = LaunchWatch(WatchObjList);
      if(bRet)
      {
         bRet = NamedPipeServerInit(DEF_PIPE_NAME, DEF_PIPE_INST_COUNT)==true;
         if(bRet)
         {
            bRet = NamedPipeServerRegRecvCB(0, WatchMsgDispatchCB)==true;
         }
      }
   }
   return bRet;
}

bool StopSAWatchdog()
{
   bool bRet = true;
   UnlaunchWatch(WatchObjList);
   UninitSAWatchdog();
   NamedPipeServerUninit();
   return bRet;
}
