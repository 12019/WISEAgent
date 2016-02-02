#include <stdio.h>
#include "Screenshot/screenshot.h"

BOOL GetMoudlePath(char * moudlePath)
{
	BOOL bRet = FALSE;
	char * lastSlash = NULL;
	char tempPath[MAX_PATH] = {0};
	if(NULL == moudlePath) return bRet;
	if(ERROR_SUCCESS != GetModuleFileName(NULL, tempPath, sizeof(tempPath)))
	{
		lastSlash = strrchr(tempPath, '\\');
		if(NULL != lastSlash)
		{
			strncpy(moudlePath, tempPath, lastSlash - tempPath + 1);
			bRet = TRUE;
		}
	}
	return bRet;
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
		sprintf_s(localFileName, sizeof(localFileName), "%s%s", modulePath, screenshotFileName);
		ScreenshotFullWindow(localFileName, NULL);
	}
	return 0;
}