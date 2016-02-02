#include "common.h"
#include "SSHClient.h"
#include "launcher.h"
#include "service.h"
#include "session.h"
#include "check.h"
#include "config.h"

#include <fcntl.h>
#include <getopt.h>
#include <limits.h>
#include <locale.h>
#include <poll.h>
#include <setjmp.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/resource.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>
#include <errno.h>

#ifdef HAVE_SYS_PRCTL_H
#include <sys/prctl.h>
#endif

#ifdef HAVE_UNUSED
#defined ATTR_UNUSED __attribute__((unused))
#defined UNUSED(x)   do { } while (0)
#else
#define ATTR_UNUSED
#define UNUSED(x)    do { (void)(x); } while (0)
#endif

#define MAX_RESPONSE      2048

//-------------------------------global val define-----------------------
DataSendHandle SendDataHandle = NULL;

static SesDataQueue SesInputDataQueue;
static CAGENT_MUTEX_TYPE SesInputDataMutex;

static CAGENT_THREAD_HANDLE  DataInputThreadHandle;
static int IsDataInputThreadRunning = 1;

static CAGENT_THREAD_HANDLE DataOutputThreadHandle;
static int IsDataOutputThreadRunning = 1;
//-----------------------------------------------------------------------

//-------------------------------static func define----------------------
static void InitSesDataQueue(SesDataQueue * pSesDataQueue);
static void DestroySesDataQueue(SesDataQueue * pSesDataQueue);
static void Enqueue(SesDataQueue * pSesDataQueue, PSesDataQNode pQNode);
static PSesDataQNode Dequeue(SesDataQueue * pSesDataQueue);
static PSesDataQNode AllocSesDataQNode(PSesData pSesData);
static void FreeSesDataQNode(PSesDataQNode pQNode);

static int ProcSesInputData(PSesData pSesData);
static CAGENT_PTHREAD_ENTRY(DataInputThreadStart, args);
static char *jsonEscape(const char *buf, int len);
static int RealOutputData(struct Session *session, const char *buf, int len, int maxLength);
static int ProcOutputData(void *arg, const char *key, char **value);
static CAGENT_PTHREAD_ENTRY(DataOutputThreadStart, args);
static int SSHClientInit();
static void removeLimits();
//-----------------------------------------------------------------------

static void InitSesDataQueue(SesDataQueue * pSesDataQueue)
{
	pSesDataQueue->head = NULL;
	pSesDataQueue->trail = NULL;
	pSesDataQueue->nodeCnt = 0;
}

static void DestroySesDataQueue(SesDataQueue * pSesDataQueue)
{
	if(pSesDataQueue)
	{
		PSesDataQNode pQNode = Dequeue(pSesDataQueue);
		while(pQNode)
		{
			FreeSesDataQNode(pQNode);
			pQNode = Dequeue(pSesDataQueue);  
		}
		pSesDataQueue->head = NULL;
		pSesDataQueue->trail = NULL;
		pSesDataQueue->nodeCnt = 0;
	}
}

static void Enqueue(SesDataQueue * pSesDataQueue, PSesDataQNode pQNode)
{
	if((pSesDataQueue) && (pQNode))
	{
		if(pSesDataQueue->trail != NULL)
		{
			pSesDataQueue->trail->next = pQNode;
		}

		pSesDataQueue->trail = pQNode;
		pSesDataQueue->nodeCnt++;
		pSesDataQueue->trail->next = NULL;

		if(pSesDataQueue->head == NULL)
		{
			pSesDataQueue->head = pSesDataQueue->trail;
		}
	}
}

static PSesDataQNode Dequeue(SesDataQueue * pSesDataQueue)
{
	PSesDataQNode pQNode = NULL;
	if(!pSesDataQueue || !pSesDataQueue->head) return NULL;
	pQNode = pSesDataQueue->head;
	pSesDataQueue->head = pQNode->next;
	pSesDataQueue->nodeCnt--;
	if(pQNode == pSesDataQueue->trail)
	{
		pSesDataQueue->trail = NULL;
	}
	return pQNode;
}

static PSesDataQNode AllocSesDataQNode(PSesData pSesData)
{
	PSesDataQNode pQNode = NULL;
	BOOL allocFlag = FALSE;
	if(!pSesData) return NULL;

	pQNode = (PSesDataQNode)malloc(sizeof(SesDataQNode));
	if(!pQNode) return NULL;
	pQNode->next = NULL;
	pQNode->pSesData = (PSesData)malloc(sizeof(SesData));
	if(!pQNode->pSesData) goto done;
	if(pSesData->data)
	{
		int len = strlen(pSesData->data) + 1;
		pQNode->pSesData->data = (char*)malloc(len);
		if(!pQNode->pSesData->data) goto done;
		memset(pQNode->pSesData->data, 0, len);
		strcpy(pQNode->pSesData->data, pSesData->data);
	}
	if(pSesData->key)
	{
		int len = strlen(pSesData->key) + 1;
		pQNode->pSesData->key = (char*)malloc(len);
		if(!pQNode->pSesData->key) goto done;
		memset(pQNode->pSesData->key, 0, len);
		strcpy(pQNode->pSesData->key, pSesData->key);
	}
   pQNode->pSesData->width = pSesData->width;
	pQNode->pSesData->height = pSesData->height;
	allocFlag = TRUE;
done:
	if(!allocFlag)
	{
		FreeSesDataQNode(pQNode);
		pQNode = NULL;
	}
	return pQNode;
}

static void FreeSesDataQNode(PSesDataQNode pQNode)
{
	if(pQNode)
	{
		if(pQNode->pSesData)
		{
			if(pQNode->pSesData->data)
			{
				free(pQNode->pSesData->data);
				pQNode->pSesData->data = NULL;
			}
			if(pQNode->pSesData->key)
			{
				free(pQNode->pSesData->key);
				pQNode->pSesData->key = NULL;
			}
			free(pQNode->pSesData);
			pQNode->pSesData = NULL;
		}
		free(pQNode);
		pQNode = NULL;
	}
}

static int ProcSesInputData(PSesData pSesData)
{
	int iRet = -1;
	if(pSesData == NULL || pSesData->key == NULL) return iRet;
	{
		// Find an existing session, or create the record for a new one
		int isNew;
		struct Session *session = findSession(&isNew, pSesData->key);
		if (session == NULL) 
		{
			printf("---------------ProcSesData, Not found session---------------------\n");
			return iRet;
		}
		
		// Create a new session, if the client did not provide an existing one
		if (isNew) 
		{
			if(pSesData->width > 0)
			{
				session->width = pSesData->width;
			}
			else session->width = DEF_SES_WIDTH;
			if(pSesData->height > 0)
			{
				session->height = pSesData->height;
			}
			else session->height = DEF_SES_HEIGHT;
			printf("ProcSesInputData-launchChild, isNew:%d,%d,%d\n", isNew, session->width, session->height);
			if (launchChild(0, session) < 0)
			{
				abandonSession(session);
				printf("---------------ProcSesData, Internal Error launchChild---------------------\n");
				return iRet;
			}
		}
		// Reset window dimensions of the pseudo TTY, if changed since last time set.
		if (session->width != pSesData->width || session->height != pSesData->height) 
		{
			if(pSesData->width > 0) session->width = pSesData->width;
			else session->width = DEF_SES_WIDTH;
			if(pSesData->height > 0) session->height = pSesData->height;
			else session->height = DEF_SES_HEIGHT;
			printf("---------------ProcSesData, Window size changed to %dx%d---------------------\n", 
				session->width, session->height);
			setWindowSize(session->pty, session->width, session->height);
		}

		// Process keypresses, if any. Then send a synchronous reply.
		if (pSesData->data && strcmp(pSesData->data, "00")) 
		{
			char * keys = pSesData->data;
			char * keyCodes;
			check(keyCodes = malloc(strlen(keys)/2));
			int len = 0;
			for (const unsigned char *ptr = (const unsigned char *)keys; ;) 
			{
				unsigned c0 = *ptr++;
				if (c0 < '0' || (c0 > '9' && c0 < 'A') || (c0 > 'F' && c0 < 'a') || c0 > 'f') break;
				unsigned c1 = *ptr++;
				if (c1 < '0' || (c1 > '9' && c1 < 'A') || (c1 > 'F' && c1 < 'a') || c1 > 'f') break;
				keyCodes[len++] = 16*((c0 & 0xF) + 9*(c0 > '9')) + (c1 & 0xF) + 9*(c1 > '9');
			}
			//printf("ProcSesInputData, pty:%d,key:%s, data:%s, keyCodes:%s, len:%d\n", session->pty, pSesData->key, pSesData->data, keyCodes, len);
			if (write(session->pty, keyCodes, len) < 0 && errno == EAGAIN) 
			{
				//completePendingRequest(session, "\007", 1, MAX_RESPONSE);
			}
			free(keyCodes);
		}

		iRet = 0;
	}
	return iRet;
}

static CAGENT_PTHREAD_ENTRY(DataInputThreadStart, args)
{
	SesDataQNode * curNode = NULL;
	while(IsDataInputThreadRunning)
	{
		app_os_mutex_lock(&SesInputDataMutex);
		curNode = Dequeue(&SesInputDataQueue);
		if(curNode)
		{
			ProcSesInputData(curNode->pSesData);
			FreeSesDataQNode(curNode);
			curNode = NULL;
		}
		app_os_mutex_unlock(&SesInputDataMutex);
		app_os_sleep(10);
	}
	IsDataInputThreadRunning = 0;
	return 0;
}

static char *jsonEscape(const char *buf, int len) 
{
	static const char *hexDigit = "0123456789ABCDEF";
	// Determine the space that is needed to encode the buffer
	int count = 0;
	const char *ptr = buf;
	for (int i = 0; i < len; i++) 
	{
		unsigned char ch = *(unsigned char *)ptr++;
		if (ch < ' ') 
		{
			switch (ch) 
			{
			case '\b': case '\f': case '\n': case '\r': case '\t':
				{
					count += 2;
					break;
				}
			default:
				{
					count += 6;
					break;
				}
			}
		} 
		else if (ch == '"' || ch == '\\' || ch == '/') 
		{
			count += 2;
		} 
		else if (ch > '\x7F') 
		{
			count += 6;
		} 
		else 
		{
			count++;
		}
	}

	// Encode the buffer using JSON string escaping
	char *result;
	check(result = malloc(count + 1));
	char *dst = result;
	ptr = buf;
	for (int i = 0; i < len; i++) 
	{
		unsigned char ch = *(unsigned char *)ptr++;
		if (ch < ' ') 
		{
			*dst++ = '\\';
			switch (ch) 
			{
			case '\b': *dst++ = 'b'; break;
			case '\f': *dst++ = 'f'; break;
			case '\n': *dst++ = 'n'; break;
			case '\r': *dst++ = 'r'; break;
			case '\t': *dst++ = 't'; break;
			default:
				{
unicode:
					*dst++ = 'u';
					*dst++ = '0';
					*dst++ = '0';
					*dst++ = hexDigit[ch >> 4];
					*dst++ = hexDigit[ch & 0xF];
					break;
				}
			}
		} 
		else if (ch == '"' || ch == '\\' || ch == '/') 
		{
			*dst++ = '\\';
			*dst++ = ch;
		} 
		else if (ch > '\x7F') 
		{
			*dst++ = '\\';
			goto unicode;
		} 
		else 
		{
			*dst++ = ch;
		}
	}
	*dst++ = '\000';
	return result;
}

static int RealOutputData(struct Session *session, const char *buf, int len, int maxLength) 
{
	int iRet = 0;
	char *data = NULL;
	if (session->buffered) 
	{
		check(session->buffered = realloc(session->buffered, session->len + len));
		memcpy(session->buffered + session->len, buf, len);
		session->len += len;
		if (maxLength > 0 && session->len > maxLength) 
		{
			data = jsonEscape(session->buffered, maxLength);
			session->len -= maxLength;
			memmove(session->buffered, session->buffered + maxLength, session->len);
		} 
		else 
		{
			data = jsonEscape(session->buffered, session->len);
			free(session->buffered);
			session->buffered = NULL;
			session->len = 0;
		}
	} 
	else 
	{
		if (maxLength > 0 && len > maxLength) 
		{
			session->len = len - maxLength;
			check(session->buffered = malloc(session->len));
			memcpy(session->buffered, buf + maxLength, session->len);
			data = jsonEscape(buf, maxLength);
		} 
		else 
		{
			data = jsonEscape(buf, len);
		}
	}

	if(SendDataHandle)
	{
		SendDataHandle((char *)session->sessionKey, data);
		iRet = 1;
	}
	free(data);

	if (session->done && !session->buffered) 
	{
		finishSession(session);
	}
	return iRet;
}

static int ProcOutputData(void *arg, const char *key, char **value) 
{
	int iRet = -1;
	struct Session *session = *(struct Session **)value;
	int len = MAX_RESPONSE - session->len;
	if (len <= 0) 
	{
		len = 1;
	}
	char buf[MAX_RESPONSE];
	int bytes = 0;
	bytes = NOINTR(read(session->pty, buf, len));
	if(bytes) 
	{
		check(RealOutputData(session, buf, bytes, MAX_RESPONSE));
		iRet = 0;
	} 
	return 1;
}

static CAGENT_PTHREAD_ENTRY(DataOutputThreadStart, args)
{
	while(IsDataOutputThreadRunning)
	{
		iterateOverSessions(ProcOutputData, NULL);
		app_os_sleep(10);
	}
	IsDataOutputThreadRunning = 0;
	return 0;
}

static int SSHClientInit()
{
	int iRet = 0;
	struct HashMap *serviceTable    = newHashMap(destroyServiceHashEntry, NULL);
	// If the user did not register any services, provide the default service
	if (!getHashmapSize(serviceTable)) 
	{
		addToHashMap(serviceTable, "/",
		(char *)newService(
#ifdef HAVE_BIN_LOGIN
			geteuid() ? ":SSH" : ":LOGIN"
#else
			":SSH"
#endif
		));
	}
	enumerateServices(serviceTable);
	deleteHashMap(serviceTable);
	return iRet;
}

static void removeLimits() 
{
	static int res[] = { RLIMIT_CPU, RLIMIT_DATA, RLIMIT_FSIZE, RLIMIT_NPROC };
	for (unsigned i = 0; i < sizeof(res)/sizeof(int); i++) 
	{
		struct rlimit rl;
		getrlimit(res[i], &rl);
		if (rl.rlim_max < RLIM_INFINITY) 
		{
			rl.rlim_max  = RLIM_INFINITY;
			setrlimit(res[i], &rl);
			getrlimit(res[i], &rl);
		}
		if (rl.rlim_cur < rl.rlim_max) 
		{
			rl.rlim_cur  = rl.rlim_max;
			setrlimit(res[i], &rl);
		}
	}
}

int SSHClientStart()
{
	int iRet = -1;
#ifdef HAVE_SYS_PRCTL_H
	// Disable core files
	prctl(PR_SET_DUMPABLE, 0, 0, 0, 0);
#endif
	struct rlimit rl = { 0 };
	setrlimit(RLIMIT_CORE, &rl);
	removeLimits();
	SSHClientInit();

	app_os_mutex_setup(&SesInputDataMutex);
	InitSesDataQueue(&SesInputDataQueue);

	// Fork the launcher process, allowing us to drop privileges in the main
	// process.
	int launcherFd  = forkLauncher();

	printf("launcherFd:%d\n", launcherFd);

	// Make sure that our timestamps will print in the standard format
	setlocale(LC_TIME, "POSIX");

	IsDataInputThreadRunning = 1;
	if (app_os_thread_create(&DataInputThreadHandle, DataInputThreadStart, NULL) != 0)
	{
		IsDataInputThreadRunning = 0;		
		goto done1;
	}

	IsDataOutputThreadRunning = 1;
	if(app_os_thread_create(&DataOutputThreadHandle, DataOutputThreadStart, NULL) != 0)
	{
		IsDataOutputThreadRunning = 0;
		goto done1;
	}
	iRet = 0;
done1:
   if(iRet != 0)
	{
		SSHClientStop();
	}
	return iRet;
}

int RegisterDataSendHandle(DataSendHandle sendHandle)
{
	int iRet = 0;
	SendDataHandle = sendHandle;
	return iRet;
}

int RecvData(char * sessionKey, char * buf, unsigned int width, unsigned int height)
{
	int iRet = -1;
	if(sessionKey == NULL || buf == NULL) return iRet;
	{
		PSesDataQNode pQNode = NULL;
		SesData sesData;
		app_os_mutex_lock(&SesInputDataMutex);
		sesData.data = buf;
		sesData.key = sessionKey;
		sesData.width = width;
		sesData.height = height;
		pQNode = AllocSesDataQNode(&sesData);
		Enqueue(&SesInputDataQueue, pQNode);
		app_os_mutex_unlock(&SesInputDataMutex);
		iRet = 0;
	}
	return iRet;
}

int CloseSession(char * sessionKey)
{
	int iRet = 0;
   struct Session * findSes = findSessionEx(sessionKey);
	if(findSes)
	{
		//printf("CloseSession key:%s ,pid:%d\n",findSes->sessionKey, findSes->pid);
		if(findSes->pid>0)
		{
			kill(findSes->pid, SIGKILL);
			//printf("CloseSession kill pid:%d\n", findSes->pid);
		}
		finishSession(findSes);
	};
	return iRet;
}

int SSHClientStop()
{
	int iRet = 0;
	if(IsDataInputThreadRunning)
	{
		IsDataInputThreadRunning = 0;
		app_os_thread_join(DataInputThreadHandle);
	}
	if(IsDataOutputThreadRunning)
	{
		IsDataOutputThreadRunning = 0;
		app_os_thread_join(DataOutputThreadHandle);
	}

	DestroySesDataQueue(&SesInputDataQueue);
	app_os_mutex_cleanup(&SesInputDataMutex);

	terminateLauncher();
	finishAllSessions();
	//printf("SSHClientStop E\n");
	return iRet;
}