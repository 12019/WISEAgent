/****************************************************************************/
/* Copyright(C) : Advantech Technologies, Inc.(研華科技股份有限公司)				 */
/* Create Date  : 2013/05/20 by Eric Liang															     */
/* Modified Date: 2013/05/20 by Eric Liang															 */
/* Abstract       :  Basic Function Tool definition													    */
/* Reference    : None																									 */
/****************************************************************************/
#ifndef  __TOOL_H__
#define __TOOL_H__


#if defined(__linux)//linux


#define LINUX

#ifndef CHAR
	#define	CHAR	char
#endif
#ifndef WORD
	#define	WORD	unsigned short
#endif
#ifndef DWORD
	#define	DWORD	unsigned int
#endif
#ifndef INT32
	#define	INT32	int
#endif
#ifndef BYTE
	#define	BYTE	unsigned char
#endif
#ifndef UINT
	#define	UINT	unsigned int
#endif

#define MAXPATH  260



#include <pthread.h>
#include <semaphore.h>

	#define TaskHandle_t		pthread_t
	#define TaskSleep(x)		usleep(x*1000)
	#define TaskExit()		pthread_exit("")
	#define TaskCreate(a,b,c,d,e,f)	{ pthread_create(a,b,(void *)c,(void *)d); pthread_detach(*a); } 
	#define TaskDelete(x)		pthread_cancel(*x) // 改到avosal_Thread.c
	

	#define MUTEX_INIT(x,y)		if(pthread_mutex_init(x,y) ) perror("Mutex initial failed");
	#define MUTEX_DESTROY(x)	if(pthread_mutex_destroy(x)) perror("Mutex destroy failed");
	#define MUTEX_UNLOCK(x)		if(pthread_mutex_unlock(x) ) perror("Mutex unlock failed" );
	#define MUTEX_LOCK(x)		if(pthread_mutex_lock(x)   ) perror("Mutex lock failed"   );
	#define MUTEX			pthread_mutex_t
	#define Sleep(x) usleep(x*1000)
	//#define	PRINTF			PRINTF
#elif NEXPERIA
	#define TaskHandle_t		TASK
	#define TaskSleep(x)		tmosalTaskSleep(x)
	#define TaskExit()		taskDelete(x)
	#define TaskCreate(a,b,c,d,e,f)	taskCreate(a,b,c,d,e,f)
	#define TaskDelete(x)		taskDelete(x) // 改到avosal_Thread.c
	

	#define MUTEX_INIT(x,y)		if(pthread_mutex_init(x,y) ) perror("Mutex initial failed");
	#define MUTEX_DESTROY(x)	if(pthread_mutex_destroy(x)) perror("Mutex destroy failed");
	#define MUTEX_UNLOCK(x)		if(pthread_mutex_unlock(x) ) perror("Mutex unlock failed" );
	#define MUTEX_LOCK(x)		if(pthread_mutex_lock(x)   ) perror("Mutex lock failed"   );
	#define MUTEX			pthread_mutex_t
	//#define	PRINTF			PRINTF
#elif _WIN32
#include "common.h"
	#define MAXPATH  260

	#define TaskHandle_t		CAGENT_THREAD_HANDLE  // X
	#define TaskSleep(x)		Sleep(x)
	#define TaskExit()			app_os_thread_exit(0)
	#define TaskCreate(a,b,c,d,e,f)		app_os_thread_create(a,c,d)
	#define TaskDelete(x)		pthread_cancel(*x) // 改到avosal_Thread.c // X
	

	#define MUTEX_INIT(x,y)		if(pthread_mutex_init(x,y) ) perror("Mutex initial failed"); // X
	#define MUTEX_DESTROY(x)	if(pthread_mutex_destroy(x)) perror("Mutex destroy failed"); // X
	#define MUTEX_UNLOCK(x)		if(pthread_mutex_unlock(x) ) perror("Mutex unlock failed" ); // X
	#define MUTEX_LOCK(x)		if(pthread_mutex_lock(x)   ) perror("Mutex lock failed"   ); // X
	#define MUTEX			pthread_mutex_t // X
#endif



#if defined(__linux)//linux
int GetPrivateProfileInt(char* lpAppName, char* lpKeyName, int nDefault, char* lpFileName);
int GetPrivateProfileString(char* lpAppName, char* lpKeyName, char* lpDefault, char* lpReturnedString, int nSize, char* lpFileName);
#endif

void identify_function_ptr( void *func);
int ReadFromFile(char *filepath, char *buf, int bufsize);
int GetExtenParam(char *cmd, char *tag,char *s);

int CleanExtenTag(char *cmd, char *tag );

char *GetLibaryDir(const char* libname );
int GetSystemCall( const char *cmd, char *buffer, int size );
bool GetCurrTime(char *curren);
char *GetWorkingDir( );
unsigned int _System_Different(time_t timeold);
void toLOWER( char *str, int length );
void SetDebugType( int Type ); // 0: None output 1: standard output , 2: write to default log
void	PRINTF( char *fmt, ... );
int setargs(char *args, char **argv);
char **parsedargs(char *args, int *argc);
int	StrPos(char *str1, char *str2);
int	SplitUrl(char *url, char *ip, int *port, char *path);
int file_exists(const char * filename);
int RunScript( char *script, char *param);
TaskHandle_t	SUSIThreadCreate(char	*taskname, int	stacksize, int piority, void *function, void* parameter);
void WL_WriteStreamToDisk( char* FilePath, int IncludeLen, int DataLen, char *SrcBuf, void *Param);
int  WaitFlagTimout(int *flag, const int stop, const unsigned int timeout /* ms */);

void* OpenLib(const char* path );
void* GetLibFnAddress( void * handle, const char *name );

void *AllocateMemory(int size);
void FreeMemory( void *pObj );

int random_number(int min_num, int max_num);
#endif // __TOOL_H__
