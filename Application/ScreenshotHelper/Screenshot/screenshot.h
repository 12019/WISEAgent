#ifndef _SCREEN_SHOT_H_
#define _SCREEN_SHOT_H_

#include <Windows.h>

#ifdef __cplusplus
extern "C"
{
#endif

int ScreenshotFullWindow(char * fileName, RECT * pRect);

int ScreenshotSubWindow(char * fileName, HWND hWnd, RECT * pRect);

int ScreenshotWithWndName(char * fileName, char * wndName, RECT * pRect);

int ScreenshotWithWndwcsName(char * fileName, WCHAR * wcsWndName, RECT * pRect);

#ifdef __cplusplus
}
#endif

#endif