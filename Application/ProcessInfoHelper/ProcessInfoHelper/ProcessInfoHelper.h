#ifndef _PROCESSINFOHELPER_HANDLER_H_
#define _PROCESSINFOHELPER_HANDLER_H_
#include <Windows.h>
#include <iostream>
#include <process.h>
#include <tlhelp32.h>

typedef unsigned long DWORD;

typedef struct msg_context_t{
	int procID;
	bool isActive;
	char msg[1024];
}msg_context_t;

typedef struct tagWNDINFO
{
	DWORD dwProcessId;
	HWND hWnd;
} WNDINFO, *LPWNDINFO;

#endif