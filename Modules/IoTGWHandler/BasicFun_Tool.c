/****************************************************************************/
/* Copyright(C) : Advantech Technologies, Inc.(研華科技股份有限公司)				 */
/* Create Date  : 2013/05/20 by Eric Liang															     */
/* Modified Date: 2013/07/26 by Eric Liang															 */
/* Abstract     : Basic Function Tool Programs                   										*/
/* Reference    : None																									 */
/****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <common.h>
#include <platform.h>
#include <ctype.h>
#include "BasicFun_Tool.h"

//-----------------------------------------------------------------------------
// Global Variable:
//-----------------------------------------------------------------------------
//


char  g_szWorkinPath[MAXPATH] = {0};
char  g_szLibaryPath[MAXPATH] = {0};
int     g_DebugType = 1; // 0: None output 1: standard output , 2: write to default log

//-----------------------------------------------------------------------------
// function:
//-----------------------------------------------------------------------------
//




void SetDebugType( int Type )
{
	g_DebugType = Type;
}

void PRINTF( char *format, ... )
{
//#ifdef RELEASE

//#else//DEBUG
	if( g_DebugType == 0 ) {

	} else {
		char szBuf[10240];
		va_list argptr;
		va_start( argptr, format );
		vsprintf( szBuf, format, argptr );	//print string to szBuf by format
		va_end( argptr );
		if( g_DebugType == 2 )
			printf( szBuf );
		else
			printf( szBuf ); // Default
	}
//#endif
}

static int _MATCH(char *source,char *target)
{
	if ( strncmp( source, target, strlen(target) ) == 0 )
	{
		int len = strlen(target);
		if( source[len] >= 'a' && source[len] <= 'z' ) {
			return 0;
		}
		if( source[len] >= 'A' && source[len] <= 'Z' ) {
			return 0;
		}
		if( source[len] >= '0' && source[len] <= '9' ) {
			return 0;
		}
		return 1;
	}
	else	return 0;
}

static int _getValue(char *line,char *s) {	//| = value
	char *sp=strchr(line,'=');		//| 分隔
	if (sp) {
		sp=sp+1;
		while( *sp == ' ' || *sp == '\t' ) {	//去除掉" "或"\t"
			sp++;
		}

		strcpy(s,sp);

		sp=strchr(s,0x0d);			//| 結尾
		if (sp) *sp='\0';
		sp=strchr(s,0x0a);			//| 結尾
		if (sp) *sp='\0';
		return 1;
	}
	return 0;
}

static int _getValue2(char *line,char *s) {	//| = value
	char *sp=strchr(line,'=');		//| 分隔

	char *end;
	int size = 0;
	if (sp) {
		sp=sp+1;
		while( *sp == ' ' || *sp == '\t' ) {	//去除掉" "或"\t"
			sp++;
		}
		sp=strchr(sp,'"');			//| begin

		if (sp) {
			end=strchr(sp+1,'"');			//| end
			if(end) {
				size = end - sp-1;
				if( size > 0 ) {
					memcpy(s, sp+1,size);
					return 1;
				}
			}
		}
	}
	return 0;
}

int ReadFromFile(char *filepath, char *buf, int bufsize)
{
	FILE	*file = fopen(filepath, "r");
	char *target = buf;
	int remain = bufsize;
	int readsize = 0;
	int totalsize = 0;
	if ( file == NULL )
		return -1;

	while ( !feof(file) )	//檢查檔案指標是否到了底部
	{
		if ( (readsize = (int) fgets(target, remain, file)) == 0 )	//Get 512 bytes data from file to source buffer
				break;

		totalsize += readsize;
		remain -= readsize;
		target = buf + totalsize;

	}
	fclose(file);
	return totalsize;
}

#if  defined(__linux)//linux
int GetPrivateProfileString(char* lpAppName, char* lpKeyName, char* lpDefault, char* lpReturnedString, int nSize, char* lpFileName)
{
	char	source[512];
	char	target[512];
	FILE	*file = fopen(lpFileName, "r");
	if ( file == NULL )
	{
		strcpy( lpReturnedString, lpDefault );
//		return strlen(lpReturnedString);
		return -1;
	}
	//fseek(file, 0, SEEK_SET);
	
	memset( lpReturnedString, 0, nSize );
	
	//先找出 AppName
	memset( target, 0, 512 );
	sprintf( target, "[%s]", lpAppName );
	while ( !feof(file) )	//檢查檔案指標是否到了底部
	{
		memset(source, 0, 512);
		if ( fgets(source, 512, file) == NULL )	//Get 512 bytes data from file to source buffer
		{
			strcpy( lpReturnedString, lpDefault );	// 沒有取到資料時回傳 default 值
			fclose(file);
			return strlen(lpReturnedString);
		}
		
		if ( _MATCH(source, target) )
			break;
	}
	
	//再找出 KeyName
	memset(target, 0, 512);
	sprintf(target, "%s", lpKeyName);
	while ( !feof(file) )
	{
search_again:
		memset(source, 0, 512);
		if ( fgets(source, 512, file) == NULL )
		{
			strcpy(lpReturnedString, lpDefault);
			fclose(file);
			return strlen(lpReturnedString);
		}
		
		if ( source[0] == '[' )
		{
			strcpy(lpReturnedString, lpDefault);
			fclose(file);
			return strlen(lpReturnedString);
		}else 
			if (source[0] == '\n') goto search_again;	//允許有多個換行

		if ( _MATCH(source, target) )
			break;
	}

	if ( _getValue(source, target) )
		strcpy(lpReturnedString, target);
	else
		strcpy(lpReturnedString, lpDefault);
	
	fclose(file);
	return strlen(lpReturnedString);
}

int GetPrivateProfileInt(char* lpAppName, char* lpKeyName, int nDefault, char* lpFileName)
{
	char lpReturnedString[256];
	char lpDefault[256];
	sprintf(lpDefault, "%d", nDefault);
	GetPrivateProfileString( lpAppName, lpKeyName, lpDefault, lpReturnedString, 256, lpFileName );
	
	return atoi(lpReturnedString);
}
#endif
int setargs(char *args, char **argv)
{
    int count = 0;

    while (isspace(*args)) ++args;
    while (*args) {
        if (argv) argv[count] = args;
        while (*args && !isspace(*args)) ++args;
        if (argv && *args) *args++ = '\0';
        while (isspace(*args)) ++args;
        count++;
    }
    return count;
}

char **parsedargs(char *args, int *argc)
{
    char **argv = NULL;
    int    argn = 0;

    if (args && *args
    && (args = strdup(args))
    && (argn = setargs(args,NULL))
    && (argv = (char **)malloc((argn+1) * sizeof(char *)))) {
        *argv++ = args;
        argn = setargs(args,argv);
    }

    if (args && !argv) free(args);

    *argc = argn;
    return argv;
}

void freeparsedargs(char **argv)
{
    if (argv) {
    free(argv[-1]);
    free(argv-1);
    } 
}

// 1: exist, 0: no
int file_exists(const char * filename)
{
	FILE * file = NULL;
	file = fopen(filename, "r");
    if ( file != NULL )
    {
        fclose(file);
        return 1;
    }
    return 0;
}

int RunScript( char *script, char *param)
{
	char command[2048]={0};

	if ( file_exists(script) == 0 ) return 0;

	if( strlen( param ) )
		sprintf(command,"%s %s",script, param );
	else
		sprintf(command,"%s", script );

    if (system(command) != 0) {
        PRINTF("Warnning: RunScript : %s Failed!\n", command);
        return 0;
    }
	return 1;
}

int GetExtenParam(char *cmd, char *tag,char *s)
{
	char *sp = NULL;
	if(strlen(cmd)) {
		sp=strstr(cmd,tag);
		if( sp ) {
			if(_getValue2(sp,s)==1) // get value untile " " or "\t"
				return strlen(s);
			else
				return 0;
		}
	}
	return 0;
}

int CleanExtenTag(char *cmd, char *tag )
{
	char *sp = NULL;
	int len = strlen(tag);
	if(strlen(cmd)) {
		sp=strstr(cmd,tag);
		if( sp ) {
			memset(sp,1,len);
			return len;
		} else
			return 0;
	} else
		return 0;
}

bool GetCurrTime(char *curren)
{
	char cTime[64]={0};
	time_t t;    
	struct tm* tm;

	if(curren == NULL)
		return false;

    time(&t);
    tm = localtime(&t);
     
	//2014-07-20T08:11:30+8:00
    strftime(cTime, sizeof(cTime), "%Y-%m-%dT%I:%M:%S", tm);
	//PRINTF("Current time=%s\n",cTime);
	sprintf(curren,"%s",cTime);
	return true;
}

char *GetWorkingDir( )
{
	if( strlen( g_szWorkinPath ) == 0) {
		app_os_get_module_path(g_szWorkinPath);
	    //getcwd( g_szWorkinPath, MAXPATH );
	}
	return g_szWorkinPath;
}

char *GetLibaryDir(const char* libname )
{
	if( strlen( g_szLibaryPath ) != 0 ) 
		return g_szLibaryPath;
	else {
		char cmd[512]={0};
		char result[1024] = {0};
		char *sp = NULL;
		int len = 0;
		sprintf(cmd, "cat /proc/%d/maps | grep %s | awk '{print $6}'",app_os_get_process_id(), libname);
		if ( GetSystemCall( cmd, result, 1024 ) > 0 ) {
			sp = strrchr(result,'//'); 
			if( sp != NULL ) {
				len = sp -result;
				memcpy(g_szLibaryPath,result,len);
				return g_szLibaryPath;
			}
		}
	}
	return NULL;
}

int GetSystemCall( const char *cmd, char *buffer, int size )
{
	//int rc = 0;
	int len = 0;

	FILE *fp = popen(cmd, "r");
	if( fp == NULL ) {
		return 0;
	}

	//rc = 
	fgets(buffer+len, size-len, fp);

	//while ( ( rc = fgets(buffer+len, size-len, fp) ) != 0  ) {
	//	/*...*/
	//	len += rc;
	//}
	pclose(fp);
	//PRINTF("Ret=%s\n",buffer);
	return strlen(buffer);
}


//--------------------------------------------------------
// Function Name : _System_Different
// Purpose       : _System_Different
// Input         : timeold
// Output        : None
// return value  : None
// Modified Date : 2013/07/25 by Eric Liang
//--------------------------------------------------------
unsigned int _System_Different(time_t timeold)
{
	time_t timenew;
	long m;
	timenew = time(NULL);

	m = difftime(timenew, timeold);

	return m;
}

//--------------------------------------------------------
// Function Name : toLOWER
// Purpose       : to lower char
// Input         : None
// Output        : None
// return value  : None
// Modified Date : 2013/07/24 by Eric Liang
//--------------------------------------------------------
void toLOWER( char *str, int length )
{
	int	i;
	for ( i = 0 ; i < length ; i++)
	{
		str[i] = tolower(str[i]);
	}	
}

//--------------------------------------------------------
// Function Name : StrPos
// Purpose       : 找尋字串位置
// Input         : str1: 原本字串   str2:欲尋找字串
// Output        : 有找到就回傳位置, 找不到回傳-1
// return value  : None
// Modified Date : 2013/07/24 by Eric Liang
//--------------------------------------------------------
int	StrPos(char *str1, char *str2){
	char	*p_str;
	
	p_str = strstr( str1, str2);
	if (p_str == 0) return -1;
	return ((int)p_str) - ((int)str1);
}

//--------------------------------------------------------
// Function Name : SplitUrl
// Purpose       : 把輸入的網址抽出ip port
// Input         : url , ip point, port point, path point
// Output        : None
// return value  : None
// Modified Date : 2013/07/24 by Eric Liang
//--------------------------------------------------------
int	SplitUrl(char *url, char *ip, int *port, char* path )
{
	int pos1,pos2;
	
	toLOWER( url, 4 );
	//有http://，新增rtsp判斷，kidd 2010/10/20
	if (strncmp(url, "http://", 7) == 0 || strncmp(url, "rtsp://", 7) == 0){
		url += 7;
	}else
		return 0;

	pos1 = StrPos(url, "/");
	pos2 = StrPos(url, ":");
	
	
	
	if (pos1 >= 0){					//有 /
		if (pos2 >= 0){				//有 : , 表示有指定port
			strncpy(ip, url, pos2 ); 
			ip[pos2] = 0x00;
			*port = atoi( url + pos2 + 1);
		}else{					//沒有:
			strncpy(ip, url , pos1 );
			ip[pos1] = 0x00;
			*port = 80;
		}
		strcpy(path, url + pos1);
	}else{						//沒有 /
		if (pos2 >= 0){				//有 :
			strncpy(ip, url , pos2 ); 
			ip[pos2] = 0x00;
			*port = atoi( url + pos2 + 1);
		}else{					//沒有:
			strcpy(ip, url );
			*port = 80;
		}
		sprintf(path,"/");
	}
	return 1;
}

TaskHandle_t	SUSIThreadCreate(char	*taskname, int	stacksize, int piority, void *function, void* parameter){
	TaskHandle_t handle;	
#ifdef	_NEXPERIA
	handle = taskCreate(taskname, stacksize, piority, function, parameter, 0);
#endif

#ifdef	LINUX

	pthread_create( &handle, NULL, function, (void*)parameter);
	pthread_detach(handle);
#endif
	return handle;
	
}

/*
*=============================================================================
*  Function name: WL_WriteStreamToDisk
*  Parameter:
*	(INPUT)
*	int		- IncludeLen: Choose data include length info or not.
*	int		- DataLen	: Data length
*	char*	- FilePath	: The path that user want to store.
*	char*	- SrcBuf	: The buffer that storing data.
*
*  Return value:	NA
*
*  History:
*	Add			09/05/2011		Ian
*=============================================================================
*/
void WL_WriteStreamToDisk( char* FilePath, int IncludeLen, int DataLen, char *SrcBuf, void *Param)
{
//#ifdef _DEBUG
	FILE *StoreFile = NULL;
//Chaeck input data
	if( SrcBuf == NULL )
		return;
	if( FilePath == NULL )
	{
#ifdef _WINDOWS
	sprintf( FilePath, "Stream");
#else
	sprintf( FilePath, "/mnt/RamDisk/Stream");
#endif	
	}

//Writing file
	StoreFile = fopen( FilePath, "ab" );
	if( StoreFile == NULL ) 
		return;
	if( IncludeLen )
		fwrite( &DataLen, 1, 4, StoreFile);

	fwrite( SrcBuf, 1, DataLen, StoreFile );
	fclose(StoreFile);

//#endif

	return;
}

void* OpenLib(const char* path )
{
#ifdef _WINDOWS
	return LoadLibrary(path); // HMODULE  WINAPI LoadLibrary( LPCTSTR lpFileName );
#else
	return dlopen( path, RTLD_LAZY );
#endif
}

void* GetLibFnAddress( void * handle, const char *name )
{
#ifdef _WINDOWS
	return (void*) GetProcAddress( handle, name ); // FARPROC WINAPI GetProcAddress( HMODULE hModule, LPCSTR lpProcName ); 
#else
	return dlsym( handle, name );
#endif
}

void *AllocateMemory(int size)
{
	return (void*) malloc ( size );
}

void FreeMemory( void *pObj )
{
	if( pObj ) {
		free( pObj );
		pObj = NULL;
	}
}

 int random_number(int min_num, int max_num)
{
        int result=0,low_num=0,hi_num=0;
        if(min_num<max_num)
        {
            low_num=min_num;
            hi_num=max_num+1; // this is done to include max_num in output.
        }else{
            low_num=max_num+1;// this is done to include max_num in output.
            hi_num=min_num;
        }
        srand(time(NULL));
        result = (rand()%(hi_num-low_num))+low_num;
        return result;
}

 int WaitFlagTimout(int *flag, const int stop, const unsigned int timeout /* ms */)
 {
	 unsigned int count = 0;
	 int rc = 0;
	 while( 1 ) {
		 TaskSleep(1);
		 count;
		 if( *flag == stop ) {
			 rc = 1;
			 break;
		 }
		 count++;
		 if( count >= timeout ) break;		
	 }
	 return rc;
 }

 int rep_str(const char *s, char *outBuf, int outBufSize, const char *old, const char *new1)
{
    //char *ret;
    int i, count = 0;
    int newlen = strlen(new1);
    int oldlen = strlen(old);
 
    for (i = 0; s[i] != '\0'; i++)    
    {
        if (strstr(&s[i], old) == &s[i]) 
        {
            count++;
            i += oldlen - 1;
        }
    }
    //ret = (char *)malloc(i + count * (newlen - oldlen));
    if (outBuf == NULL) return -1;
    i = 0;
    while (*s)
    {
        if (strstr(s, old) == s) //compare the substring with the newstring
        {
            strcpy(&outBuf[i], new1);
            i += newlen; //adding newlength to the new string
            s += oldlen;//adding the same old length the old string
        }
        else
        outBuf[i++] = *s++;
    }
    outBuf[i] = '\0';
    return strlen(outBuf);
}
