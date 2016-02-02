// ProcessInfoHelper.cpp : Defines the entry point for the application.
//

#include "stdafx.h"
#include "ProcessInfoHelper.h"
#include "stdio.h"

#define MAX_LOADSTRING 100

// Global Variables:
HINSTANCE hInst;								// current instance
TCHAR szTitle[MAX_LOADSTRING];					// The title bar text
TCHAR szWindowClass[MAX_LOADSTRING];			// the main window class name

// Forward declarations of functions included in this code module:
ATOM				MyRegisterClass(HINSTANCE hInstance);
BOOL				InitInstance(HINSTANCE, int);
LRESULT CALLBACK	WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK	About(HWND, UINT, WPARAM, LPARAM);

BOOL IsProcessActiveWithIDEx(DWORD prcID);

int APIENTRY _tWinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPTSTR    lpCmdLine,
                     int       nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

 	// TODO: Place code here.
	MSG msg;
	HACCEL hAccelTable;

	// Initialize global strings
	LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	LoadString(hInstance, IDC_PROCESSINFOHELPER, szWindowClass, MAX_LOADSTRING);
	MyRegisterClass(hInstance);

	// Perform application initialization:
	if (!InitInstance (hInstance, nCmdShow))
	{
		return FALSE;
	}

	hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_PROCESSINFOHELPER));

	// Main message loop:
	while (GetMessage(&msg, NULL, 0, 0))
	{
		if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	return (int) msg.wParam;
}



//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
//  COMMENTS:
//
//    This function and its usage are only necessary if you want this code
//    to be compatible with Win32 systems prior to the 'RegisterClassEx'
//    function that was added to Windows 95. It is important to call this function
//    so that the application will get 'well formed' small icons associated
//    with it.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
	WNDCLASSEX wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style			= CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc	= WndProc;
	wcex.cbClsExtra		= 0;
	wcex.cbWndExtra		= 0;
	wcex.hInstance		= hInstance;
	wcex.hIcon			= LoadIcon(hInstance, MAKEINTRESOURCE(IDI_PROCESSINFOHELPER));
	wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground	= (HBRUSH)(COLOR_WINDOW+1);
	wcex.lpszMenuName	= MAKEINTRESOURCE(IDC_PROCESSINFOHELPER);
	wcex.lpszClassName	= szWindowClass;
	wcex.hIconSm		= LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

	return RegisterClassEx(&wcex);
}

//
//   FUNCTION: InitInstance(HINSTANCE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   HWND hWnd;
	//Wei.Gang add
   	HANDLE hFileMapping   = NULL; 
	LPVOID lpShareMemory  = NULL; 
	msg_context_t *pShareBuf = NULL;
	//char strBuf[1024];
		//Add for debug
	//FILE *fp = NULL;
	//if(fp = fopen("debug_log.txt","a+"))
	//	fputs("Open ok!\n",fp);
	//For debug end
	//open share memory 
	/*if(AllocConsole())
	{
		freopen("CONOUT$","w",stdout);
	}*/
	//printf("Start \n");
   //Wei.Gang add end
   hInst = hInstance; // Store instance handle in our global variable

   hWnd = CreateWindow(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
      CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, NULL, NULL, hInstance, NULL);

   if (!hWnd)
   {
      return FALSE;
   }

	hFileMapping = OpenFileMappingA(FILE_MAP_ALL_ACCESS, 
								FALSE, 
								"Global\\Share-Mem-For-RMM-3-ID-0x0001"); 
	if (NULL == hFileMapping) 
	{ 
		//sprintf_s(strBuf,sizeof(strBuf),"Open file mapping failed, errno is %d\n",GetLastError());
		//fputs(strBuf,fp);
		//printf("Open file mapping failed\n");
		goto CLIENT_SHARE_MEMORY_END; 
	} 
  
	lpShareMemory = MapViewOfFile(hFileMapping, 
								FILE_MAP_ALL_ACCESS, 
								0, 
								0, 
								0); 
	if (NULL == lpShareMemory) 
	{ 
		//fputs("Open share memory failed\n",fp);
		//printf("Open file mapping failed\n");
		goto CLIENT_SHARE_MEMORY_END; 
	}

	pShareBuf = (msg_context_t *)lpShareMemory;
	pShareBuf->isActive = IsProcessActiveWithIDEx(pShareBuf->procID);

	//printf("Process id %d, active is %d\n",pShareBuf->procID,pShareBuf->isActive);
	//sprintf_s(strBuf,sizeof(strBuf),"Process id %d, active is %d\n",pShareBuf->procID,pShareBuf->isActive);
	//fputs(strBuf,fp);

CLIENT_SHARE_MEMORY_END: 
  //release share memory 
	//if(fp) fclose(fp);
	if (NULL != lpShareMemory)   UnmapViewOfFile(lpShareMemory); 
	if (NULL != hFileMapping)    CloseHandle(hFileMapping); 

   //ShowWindow(hWnd, nCmdShow);
   //UpdateWindow(hWnd);
   DestroyWindow(hWnd);
   return TRUE;
}

BOOL CALLBACK SAEnumProc(HWND hWnd,LPARAM lParam)
{
	DWORD dwProcessId;
	LPWNDINFO pInfo = NULL;
	GetWindowThreadProcessId(hWnd, &dwProcessId);
	pInfo = (LPWNDINFO)lParam;

	if(dwProcessId == pInfo->dwProcessId)
	{
		BOOL isWindowVisible = IsWindowVisible(hWnd);
		if(isWindowVisible == TRUE)
		{
			pInfo->hWnd = hWnd;
			return FALSE;
		}
	}
	return TRUE;
}

HWND GetProcessMainWnd(DWORD dwProcessId)
{
	WNDINFO wi;
	wi.dwProcessId = dwProcessId;
	wi.hWnd = NULL;
	EnumWindows(SAEnumProc,(LPARAM)&wi);
	return wi.hWnd;
}

BOOL IsProcessActiveWithIDEx(DWORD prcID)
{
	BOOL bRet = TRUE;

	if(prcID > 0)
	{
		HWND hWnd = NULL;
		bRet = TRUE;
		hWnd = GetProcessMainWnd(prcID);
		if(hWnd && IsWindow(hWnd))
		{
			if(IsHungAppWindow(hWnd))
			{
				bRet = FALSE;
			}
		}
	}

	return bRet;
}

//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE:  Processes messages for the main window.
//
//  WM_COMMAND	- process the application menu
//  WM_PAINT	- Paint the main window
//  WM_DESTROY	- post a quit message and return
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;
	PAINTSTRUCT ps;
	HDC hdc;

	switch (message)
	{
	case WM_COMMAND:
		wmId    = LOWORD(wParam);
		wmEvent = HIWORD(wParam);
		// Parse the menu selections:
		switch (wmId)
		{
		case IDM_ABOUT:
			DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
			break;
		case IDM_EXIT:
			DestroyWindow(hWnd);
			break;
		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
		}
		break;
	case WM_PAINT:
		hdc = BeginPaint(hWnd, &ps);
		// TODO: Add any drawing code here...
		EndPaint(hWnd, &ps);
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	case WM_QUERYENDSESSION:
		{
			HANDLE hFileMapping   = NULL; 
			LPVOID lpShareMemory  = NULL; 
			msg_context_t *pShareBuf = NULL;
			char strBuf[1024];

			hFileMapping = OpenFileMappingA(FILE_MAP_ALL_ACCESS, 
								FALSE, 
								"Global\\Share-Mem-For-RMM-3-ID-0x0001"); 
			if (NULL == hFileMapping) 
				goto CLIENT_SHARE_MEMORY_END; 
  
			lpShareMemory = MapViewOfFile(hFileMapping, 
								FILE_MAP_ALL_ACCESS, 
								0, 
								0, 
								0); 
			if (NULL == lpShareMemory) 
				goto CLIENT_SHARE_MEMORY_END; 

			pShareBuf = (msg_context_t *)lpShareMemory;
			pShareBuf->isShutDown = true;

CLIENT_SHARE_MEMORY_END: 
		if (NULL != lpShareMemory)   UnmapViewOfFile(lpShareMemory); 
		if (NULL != hFileMapping)    CloseHandle(hFileMapping); 
			//MessageBox(NULL,L"Will shutdown!",L"Will shutdown!",MB_OK);
			break;
		}
	case WM_ENDSESSION:
		//MessageBox(NULL,L"Shutting down!",L"Shutting down!",MB_OK);
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return DefWindowProc(hWnd, message, wParam, lParam);
	return 0;
}

// Message handler for about box.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);
	switch (message)
	{
	case WM_INITDIALOG:
		return (INT_PTR)TRUE;

	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
		{
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}
		break;
	}
	return (INT_PTR)FALSE;
}
