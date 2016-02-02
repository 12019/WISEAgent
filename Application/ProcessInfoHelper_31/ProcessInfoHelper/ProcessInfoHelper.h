#pragma once

#include "resource.h"
typedef unsigned long DWORD;

typedef struct msg_context_t{
	int procID;
	bool isActive;
	bool isShutDown;
	//char msg[1024];
}msg_context_t;

typedef struct tagWNDINFO
{
	DWORD dwProcessId;
	HWND hWnd;
} WNDINFO, *LPWNDINFO;