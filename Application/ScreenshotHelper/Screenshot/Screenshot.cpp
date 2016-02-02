#include <windows.h>
#include <gdiplus.h>
#include <string.h>  
#include <stdio.h> 
#include "Screenshot.h"

#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "gdiplus.lib")

using namespace Gdiplus;

int GetEncoderClsid(const WCHAR* format, CLSID* pClsid)
{
	int iRet = -1;
	UINT  num = 0;          
	UINT  size = 0;      
	ImageCodecInfo* pImageCodecInfo = NULL;

	GetImageEncodersSize(&num, &size);
	if(size == 0) return iRet;

	pImageCodecInfo = (ImageCodecInfo*)(malloc(size));
	if(pImageCodecInfo == NULL) return iRet;

	GetImageEncoders(num, size, pImageCodecInfo);

	for(UINT j = 0; j < num; ++j)
	{
		if(wcscmp(pImageCodecInfo[j].MimeType, format) == 0)
		{
			*pClsid = pImageCodecInfo[j].Clsid;
			iRet = j;
			break;
		}    
	}

	free(pImageCodecInfo);
	return iRet;
}

int ScreenshotFullWindow(char * fileName, RECT * pRect)
{
	int iRet = -1;
	RECT rect = {0};
	WCHAR wcsFileName[MAX_PATH] = {0};
   if(NULL == fileName) return iRet;
	mbstowcs(wcsFileName, fileName, strlen(fileName));
	if(pRect != NULL) 
	{
		rect.top = pRect->top;
		rect.left = pRect->left;
		rect.right = pRect->right;
		rect.bottom = pRect->bottom;
	}
	else
	{
		rect.left = GetSystemMetrics(SM_XVIRTUALSCREEN);
		rect.top = GetSystemMetrics(SM_YVIRTUALSCREEN);
		rect.right = rect.left + GetSystemMetrics(SM_CXVIRTUALSCREEN);
		rect.bottom = rect.top + GetSystemMetrics(SM_CYVIRTUALSCREEN);
		//printf("%d  %d  %d  %d\n",rect.left, rect.top, rect.right, rect.bottom);
	}
	{
		GdiplusStartupInput gdiplusStartupInput;
		ULONG_PTR gdiplusToken;
		CLSID  encoderClsid;
		Status stat = GenericError;
		Bitmap * b = NULL;
		HWND desktop = GetDesktopWindow();
		HDC desktopdc = GetDC(desktop);
		HDC mydc = CreateCompatibleDC(desktopdc);

		//int width  = (rect.right-rect.left==0) ? GetSystemMetrics(SM_CXSCREEN) : rect.right-rect.left;
		//int height = (rect.bottom-rect.top==0) ? GetSystemMetrics(SM_CYSCREEN) : rect.bottom-rect.top;
		int width  = rect.right-rect.left;
		int height = rect.bottom-rect.top;

		HBITMAP mybmp = CreateCompatibleBitmap(desktopdc, width, height);
		HBITMAP oldbmp = (HBITMAP)SelectObject(mydc, mybmp);

		GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);
		BitBlt(mydc,0,0,width,height,desktopdc,rect.left,rect.top, SRCCOPY|CAPTUREBLT);
		SelectObject(mydc, oldbmp);

		b = Bitmap::FromHBITMAP(mybmp, NULL);
		if (b && GetEncoderClsid(L"image/jpeg", &encoderClsid) != -1) 
		{
			stat = b->Save(wcsFileName, &encoderClsid, NULL);
		}
		if (b) delete b;

		GdiplusShutdown(gdiplusToken);
		ReleaseDC(desktop, desktopdc);
		DeleteObject(mybmp);
		DeleteDC(mydc);
      if(stat == Ok) iRet = 0;
	}
	return iRet;
}

int ScreenshotSubWindow(char * fileName, HWND hWnd, RECT * pRect)
{
	int iRet = -1;
	WCHAR wcsFileName[MAX_PATH]	= {0};
	RECT rect;
	bool rectProvided = false;
	if(!IsWindow(hWnd) || fileName == NULL) return iRet;
	mbstowcs(wcsFileName, fileName, strlen(fileName));

	SetWindowPos(hWnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW ); 
	Sleep(200);
	GetWindowRect(hWnd, &rect);

	if(pRect != NULL) 
	{
		rect.top = pRect->top;
		rect.left = pRect->left;
		rect.right = pRect->right;
		rect.bottom = pRect->bottom;
		rectProvided = true;
	}

	{
		GdiplusStartupInput gdiplusStartupInput;
		ULONG_PTR gdiplusToken;
		GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);
		if(rectProvided)
		{
			RECT wrect;
			GetWindowRect(hWnd, &wrect);
			OffsetRect(&rect, wrect.left, wrect.top );
		}
		HWND desktop = GetDesktopWindow();
		HDC desktopdc = GetDC(desktop);
		HDC mydc = CreateCompatibleDC(desktopdc);

		int width  = (rect.right-rect.left==0) ? GetSystemMetrics(SM_CXSCREEN) : rect.right-rect.left;
		int height = (rect.bottom-rect.top==0) ? GetSystemMetrics(SM_CYSCREEN) : rect.bottom-rect.top;

		HBITMAP mybmp = CreateCompatibleBitmap(desktopdc, width, height);
		HBITMAP oldbmp = (HBITMAP)SelectObject(mydc, mybmp);
		BitBlt(mydc,0,0,width,height,desktopdc,rect.left,rect.top, SRCCOPY|CAPTUREBLT);
		SelectObject(mydc, oldbmp);

		SetWindowPos(hWnd, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW ); 

		Bitmap* b = Bitmap::FromHBITMAP(mybmp, NULL);
		CLSID  encoderClsid;
		Status stat = GenericError;
		if (b && GetEncoderClsid(L"image/jpeg", &encoderClsid) != -1) 
		{
			stat = b->Save(wcsFileName, &encoderClsid, NULL);
		}
		if (b) delete b;

		GdiplusShutdown(gdiplusToken);
		ReleaseDC(desktop, desktopdc);
		DeleteObject(mybmp);
		DeleteDC(mydc);
      if(stat == Ok) iRet = 0;
	}
	return iRet;
}

int ScreenshotWithWndName(char * fileName, char * wndName, RECT * pRect)
{
	int iRet = -1;
	if(fileName == NULL || wndName == NULL) return iRet;
	{
		HWND hWnd = FindWindow(NULL, wndName);
		iRet = ScreenshotSubWindow(fileName, hWnd, pRect);
	}
	return iRet;
}

int ScreenshotWithWndwcsName(char * fileName, WCHAR * wcsWndName, RECT * pRect)
{
	int iRet = -1;
	if(fileName == NULL || wcsWndName == NULL) return iRet;
	{
		HWND hWnd = FindWindowW(NULL, wcsWndName);
		iRet = ScreenshotSubWindow(fileName, hWnd, pRect);
	}
	return iRet;
}
