#include <stdio.h>
#include <string.h>
#include "ScreenshotL.h"

#ifndef MAX_PATH
#define MAX_PATH   512
#endif
#ifndef F_OK
#  define F_OK 0
#endif
#define FILE_SEPARATOR      '/'

int GetModuleFileName( char* sModuleName, char* sFileName, int nSize)
{
	int ret = 0;
	if(sModuleName == NULL)
	{
		readlink("/proc/self/exe", sFileName, nSize);

		if( 0 == access( sFileName, F_OK ) )
		{
			ret = strlen(sFileName);
		}
	}
	else if(strchr( sModuleName,'/' ) != NULL )
	{
		strcpy( sFileName, sModuleName );
		if( 0 == access( sFileName, F_OK ) )
		{
			ret = strlen(sFileName);
		}
	}
	else
	{
		char* sPath = getenv("PATH");
		char* pHead = sPath;
		char* pTail = NULL;
		while( pHead != NULL && *pHead != '\0' )
		{
			pTail = strchr( pHead, ':' );
			if( pTail != NULL )
			{
				strncpy( sFileName, pHead, pTail-pHead );
				sFileName[pTail-pHead] = '\0';
				pHead = pTail+1;
			}
			else
			{
				strcpy( sFileName, pHead );
				pHead = NULL;
			}

			int nLen = strlen(sFileName);
			if( sFileName[nLen] != '/' )sFileName[nLen] = '/';
			strcpy( sFileName+nLen+1,sModuleName);
			if( 0 == access( sFileName, F_OK ) )
			{
				ret = strlen(sFileName);
				break;
			}
		}
	}
	return ret;
}

int GetMoudlePath(char * moudlePath)
{
	int iRet = 0;
	char * lastSlash = NULL;
	char tempPath[MAX_PATH] = {0};
	if(NULL == moudlePath) return iRet;
	if(0 != GetModuleFileName(NULL, tempPath, sizeof(tempPath)))
	{
		lastSlash = strrchr(tempPath, FILE_SEPARATOR);
		if(NULL != lastSlash)
		{
			strncpy(moudlePath, tempPath, lastSlash - tempPath + 1);
			iRet = lastSlash - tempPath + 1;
		}
	}
	return iRet;
}

int main(int argc, char * argv[])
{
	if(argc > 1)
	{
		char modulePath[MAX_PATH] = {0};
		char localFileName[MAX_PATH] = {0};
		char screenshotFileName[64] = {0};
		strcpy(screenshotFileName, argv[1]);
		GetMoudlePath(modulePath);
		sprintf(localFileName, "%s%s", modulePath, screenshotFileName);
		printf("localFileName:%s, %s, %s\n", localFileName, argv[1], screenshotFileName);
		ScreenshotFullWindow(localFileName);
	}
	return 0;
}
