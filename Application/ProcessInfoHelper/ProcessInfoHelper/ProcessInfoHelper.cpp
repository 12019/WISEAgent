#include "stdio.h"
#include "processInfoHelper.h"

using namespace std;

BOOL IsProcessActiveWithIDEx(DWORD prcID);

int main()
{
	HANDLE hFileMapping   = NULL; 
	LPVOID lpShareMemory  = NULL; 
	msg_context_t *pShareBuf = NULL;
	char strBuf[1024];
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
	hFileMapping = OpenFileMapping(FILE_MAP_ALL_ACCESS, 
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
	//system("pause");
	return 0;
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