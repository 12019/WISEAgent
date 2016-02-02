#include "Global.h"
#include "AdvPlatform.h"
#include <stdio.h>
#include <string.h>

#ifdef WIN32
#include <Windows.h>
#include <TlHelp32.h>
#include <psapi.h>
#pragma comment(lib,"Psapi.lib")

bool AdjustPrivileges() 
{
   HANDLE hToken;
   TOKEN_PRIVILEGES tp;
   TOKEN_PRIVILEGES oldtp;
   DWORD dwSize=sizeof(TOKEN_PRIVILEGES);
   LUID luid;

   if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken)) 
   {
      if (GetLastError()==ERROR_CALL_NOT_IMPLEMENTED) return true;
      else return false;
   }

   if (!LookupPrivilegeValue(NULL, SE_DEBUG_NAME, &luid)) 
   {
      CloseHandle(hToken);
      return false;
   }

   memset(&tp, 0, sizeof(tp));
   tp.PrivilegeCount=1;
   tp.Privileges[0].Luid=luid;
   tp.Privileges[0].Attributes=SE_PRIVILEGE_ENABLED;

   if (!AdjustTokenPrivileges(hToken, false, &tp, sizeof(TOKEN_PRIVILEGES), &oldtp, &dwSize)) 
   {
      CloseHandle(hToken);
      return false;
   }

   CloseHandle(hToken);
   return true;
}

bool GetTokenByName(HANDLE * hToken, char * prcName)
{
	bool bRet = false;
	HANDLE hPrc = NULL;
	PROCESSENTRY32 pe;
	HANDLE hSnapshot=NULL;
	if(NULL == prcName || NULL == hToken) return bRet;
	hSnapshot=CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	pe.dwSize=sizeof(PROCESSENTRY32);
	if(!Process32First(hSnapshot,&pe))
		return bRet;
	while(true)
	{
		pe.dwSize=sizeof(PROCESSENTRY32);
		if(Process32Next(hSnapshot,&pe)==false)
			break;
		if(strcasecmp(pe.szExeFile, prcName)==0)
		{	
			hPrc = OpenProcess(PROCESS_QUERY_INFORMATION, false, pe.th32ProcessID);
			bRet = OpenProcessToken(hPrc,TOKEN_ALL_ACCESS,hToken);
			CloseHandle(hPrc);
			break;
		}
	}
	if(hSnapshot) CloseHandle(hSnapshot);
	return bRet;
}


bool RunProcessAsUser(char * cmdLine, bool isAppNameRun, bool isShowWindow, unsigned long * newPrcID)
{
	bool bRet = false;
	if(NULL == cmdLine) return bRet;
	{
		HANDLE hToken;
		if(!GetTokenByName(&hToken,"EXPLORER.EXE"))
		{
			return bRet;
		}
		else
		{
			STARTUPINFO si;
			PROCESS_INFORMATION pi;
			unsigned long dwCreateFlag = CREATE_NO_WINDOW;
			memset(&si, 0, sizeof(si));
			si.dwFlags = STARTF_USESHOWWINDOW; 
			si.wShowWindow = SW_HIDE;
			if(isShowWindow)
			{
				si.wShowWindow = SW_SHOW;
				dwCreateFlag = CREATE_NEW_CONSOLE;
			}
			si.cb = sizeof(si);
			memset(&pi, 0, sizeof(pi));
			if(isAppNameRun)
			{
				bRet = CreateProcessAsUserA(hToken, cmdLine, NULL,  NULL ,NULL,
					false, dwCreateFlag, NULL, NULL, &si, &pi);
			}
			else
			{
				bRet = CreateProcessAsUserA(hToken, NULL, cmdLine, NULL ,NULL,
					false, dwCreateFlag, NULL, NULL, &si, &pi);
			}

			if(!bRet)
			{
				printf("error code: %s  %d\n", cmdLine, GetLastError());
			}
			else
			{
				if(newPrcID != NULL) *newPrcID = pi.dwProcessId;
			}
			CloseHandle(hToken);
		}
	}
	return bRet;
}

bool ConverNativePathToWin32(char * nativePath, char * win32Path)
{
	bool bRet = false;
	if(NULL == nativePath || NULL == win32Path) return bRet;
	{
		char drv = 'A';
		char devName[3] = {drv, ':', '\0'};
		char tmpDosPath[MAX_PATH] = {0};
		while( drv <= 'Z')
		{
			devName[0] = drv;
			memset(tmpDosPath, 0, sizeof(tmpDosPath));
			if(QueryDosDeviceA(devName, tmpDosPath, sizeof(tmpDosPath) - 1)!=0)
			{
				if(strstr(nativePath, tmpDosPath))
				{
					strcat(win32Path, devName);
					strcat(win32Path, nativePath + strlen(tmpDosPath));
					bRet = true;
					break;
				}
			}
			drv++;
		}
	}
	return bRet;
}

bool KillProcessWithID(unsigned long prcID)
{
	bool bRet = false;
	PROCESSENTRY32 pe;
	HANDLE hSnapshot=NULL;
	hSnapshot=CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS,0);
	pe.dwSize=sizeof(PROCESSENTRY32);
	if(!Process32First(hSnapshot,&pe))
	{
		CloseHandle(hSnapshot);
		return bRet;
	}
	while(TRUE)
	{
		pe.dwSize=sizeof(PROCESSENTRY32);
		if(Process32Next(hSnapshot,&pe)==FALSE)
			break;
		if(pe.th32ProcessID == prcID)
		{
			HANDLE hPrc = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pe.th32ProcessID);
			if(hPrc == NULL) 
			{
				unsigned long dwRet = GetLastError();          
				if(dwRet == 5)
				{
					if(AdjustPrivileges())
					{
						hPrc = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pe.th32ProcessID);
					}
				}
			}

			if(hPrc)
			{
				TerminateProcess(hPrc, 0);    //asynchronous
				bRet = true;
				CloseHandle(hPrc);
			}

			break;
		}
	}
	CloseHandle(hSnapshot);
	return bRet;
}

unsigned long RestartProcessWithID(unsigned long prcID)
{
	unsigned long dwPrcID = 0;
	HANDLE hPrc = NULL;
	PROCESSENTRY32 pe;
	HANDLE hSnapshot=NULL;
	hSnapshot=CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS,0);
	pe.dwSize=sizeof(PROCESSENTRY32);
	if(!Process32First(hSnapshot,&pe))
	{
		CloseHandle(hSnapshot);
		return dwPrcID;
	}
	while(TRUE)
	{
		pe.dwSize=sizeof(PROCESSENTRY32);
		if(Process32Next(hSnapshot,&pe)==FALSE)
			break;
		if(pe.th32ProcessID == prcID)
		{
			hPrc = OpenProcess(PROCESS_ALL_ACCESS, false, pe.th32ProcessID);

			if(hPrc == NULL) 
			{
				unsigned long dwRet = GetLastError();          
				if(dwRet == 5)
				{
					if(AdjustPrivileges())
					{
						hPrc = OpenProcess(PROCESS_ALL_ACCESS, false, pe.th32ProcessID);
					}
				}
			}
			if(hPrc)
			{
				char nativePath[MAX_PATH] = {0};
				char win32Path[MAX_PATH] = {0};
				if(GetProcessImageFileNameA(hPrc, nativePath, sizeof(nativePath)))
				{
					if(ConverNativePathToWin32(nativePath, win32Path))
					{              
						TerminateProcess(hPrc, 0);    
						{
							char cmdLine[BUFSIZ] = {0};
							DWORD tmpPrcID = 0;
							sprintf(cmdLine, "%s", win32Path);
							//sprintf(cmdLine, "%s \"%s\"", "cmd.exe /c ", path);
							if(RunProcessAsUser(cmdLine, true, true, &tmpPrcID))
								//if(CreateProcess(cmdLine, NULL, NULL, NULL, FALSE, CREATE_NO_WINDOW , NULL, NULL, &si, &pi))
								//if(CreateProcess(NULL, cmdLine, NULL, NULL, FALSE, CREATE_NO_WINDOW , NULL, NULL, &si, &pi))
							{
								dwPrcID = tmpPrcID;
							}
						}
					}
				}
				CloseHandle(hPrc);            
			}
			break;
		}
	}
	if(hSnapshot) CloseHandle(hSnapshot);
	return dwPrcID;
}

bool IsSrvExist(char * srvName)
{
   DWORD dwRet = 0;
   SC_HANDLE hSCM = NULL;
   SC_HANDLE hSrv = NULL;
   if(NULL == srvName) return false;
   hSCM = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
   if(!hSCM) return FALSE;
   hSrv = OpenService(hSCM, srvName, SERVICE_ALL_ACCESS);
   if(NULL == hSrv)
   {
      DWORD dwErrorCode = GetLastError();
      if(ERROR_SERVICE_DOES_NOT_EXIST == dwErrorCode) return false;
   }
   return true;
}

unsigned long StartSrv(char * srvName)
{
   unsigned long dwRet = 0;
   SC_HANDLE hSCM = NULL;
   SC_HANDLE hSrv = NULL;
   if(NULL == srvName) return dwRet;
   hSCM = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
   if(!hSCM) return dwRet;
   hSrv = OpenService(hSCM, srvName, SERVICE_ALL_ACCESS);
   if(hSrv)
   {
      SERVICE_STATUS srvStat;
      memset(&srvStat, 0, sizeof(SERVICE_STATUS));
      QueryServiceStatus(hSrv, &srvStat);
      dwRet = srvStat.dwCurrentState;
      if(dwRet != SERVICE_RUNNING)
      {
         if(dwRet == SERVICE_STOPPED)
         {
            if(StartService(hSrv, 0, NULL))
            {
               do 
               {
                  Sleep(100);
                  memset(&srvStat, 0, sizeof(SERVICE_STATUS));
                  QueryServiceStatus(hSrv, &srvStat);
                  dwRet = srvStat.dwCurrentState;
               } while (dwRet == SERVICE_START_PENDING);
            }
         }
      }
   }
   if(hSrv) CloseServiceHandle(hSrv);
   if(hSCM) CloseServiceHandle(hSCM);
   return dwRet;
}

unsigned long GetSrvStatus(char * srvName)
{
   unsigned long dwRet = 0;
   SC_HANDLE hSCM = NULL;
   SC_HANDLE hSrv = NULL;
   if(NULL == srvName) return dwRet;
   hSCM = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
   if(!hSCM) return dwRet;
   hSrv = OpenService(hSCM, srvName, SERVICE_ALL_ACCESS);
   if(hSrv)
   {
      SERVICE_STATUS srvStat;
      memset(&srvStat, 0, sizeof(SERVICE_STATUS));
      QueryServiceStatus(hSrv, &srvStat);
      dwRet = srvStat.dwCurrentState;
   }
   if(hSrv) CloseServiceHandle(hSrv);
   if(hSCM) CloseServiceHandle(hSCM);
   return dwRet;
}

unsigned long StopSrv(char * srvName)
{
   unsigned long dwRet = 0;
   SC_HANDLE hSCM = NULL;
   SC_HANDLE hSrv = NULL;
   if(NULL == srvName) return dwRet;
   hSCM = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
   if(!hSCM) return dwRet;
   hSrv = OpenService(hSCM, srvName, SERVICE_ALL_ACCESS);
   if(hSrv)
   {
      SERVICE_STATUS srvStat;
      memset(&srvStat, 0, sizeof(SERVICE_STATUS));
      QueryServiceStatus(hSrv, &srvStat);

      if(srvStat.dwCurrentState == SERVICE_RUNNING)
      {
         memset(&srvStat, 0, sizeof(SERVICE_STATUS));
         ControlService(hSrv, SERVICE_CONTROL_STOP, &srvStat);
         dwRet = srvStat.dwCurrentState;
         while(dwRet == SERVICE_STOP_PENDING)
         {
            Sleep(100);
            memset(&srvStat, 0, sizeof(SERVICE_STATUS));
            QueryServiceStatus(hSrv, &srvStat);   
            dwRet = srvStat.dwCurrentState;
         }
      }
   }
   if(hSrv) CloseServiceHandle(hSrv);
   if(hSCM) CloseServiceHandle(hSCM);
   return dwRet;
}
#else
#include <sys/types.h>
#include <signal.h>
#include <unistd.h>
#include <ctype.h>

unsigned long getPIDByName(char * prcName)
{
	FILE *fopen = NULL;
	unsigned int pid = 0;	
	char buf[16];
	char cmdLine[256];
	sprintf(cmdLine,"pidof -s %s",prcName);
	if((fopen = popen(cmdLine,"r"))==NULL)
		return 0;
	if(!fgets(buf,sizeof(buf),fopen))
	{
		pclose(fopen);
		return 0;
	}
	sscanf(buf,"%d",&pid);
	pclose(fopen);
	return pid;
}

bool getSysLogonUserName(char * userNameBuf, unsigned int bufLen)
{
	int i = 0;
	if (userNameBuf == NULL || bufLen == 0) return false;

	FILE * fp = NULL;
	char cmdline[128];
	const int bufLength = 512;
	char buf[bufLength];
	char cmdbuf[12][32]={0};

	sprintf(cmdline,"last|grep still");//for opensusi kde desktop
	fp = popen(cmdline,"r");
	if(NULL != fp){
		if (fgets(buf, sizeof(buf), fp))
    	{
        	sscanf(buf,"%s",cmdbuf[0]);
		}
	}
	/*sprintf(cmdline,"ps -aux|grep startkde");//for opensusi kde desktop
	fp = popen(cmdline,"r");
	if(NULL != fp)
	{
		if (fgets(buf, sizeof(buf), fp))
    	{
        	sscanf(buf,"%s %s %s %s %s %s %s %s %s %s %s %s",cmdbuf[0],cmdbuf[1],cmdbuf[2],cmdbuf[3],
				cmdbuf[4],cmdbuf[5],cmdbuf[6],cmdbuf[7],cmdbuf[8],cmdbuf[9],cmdbuf[10],cmdbuf[11]);
		    //printf("[app_os_GetSysLogonUserName] kde name:%s, utime:%s, NTime:%s, ktime:%s.\n",cmdbuf[0],cmdbuf[1],cmdbuf[10],cmdbuf[11]);
    	}
		else
		{
			pclose(fp);
			sprintf(cmdline,"ps -aux|grep gnome-keyring-daemon");//for ubuntu gnome desktop gnome
			fp = popen(cmdline,"r");
			if (fgets(buf, sizeof(buf), fp))
    		{
        	sscanf(buf,"%s %s %s %s %s %s %s %s %s %s %s %s",cmdbuf[0],cmdbuf[1],cmdbuf[2],cmdbuf[3],
				cmdbuf[4],cmdbuf[5],cmdbuf[6],cmdbuf[7],cmdbuf[8],cmdbuf[9],cmdbuf[10],cmdbuf[11]);
		    //printf("[app_os_GetSysLogonUserName] gnome name:%s, utime:%s, NTime:%s, ktime:%s.\n",cmdbuf[0],cmdbuf[1],cmdbuf[10],cmdbuf[11]);
    		}
		}

	}*/
    	pclose(fp);

	i = strlen(cmdbuf[0]);
	if(i>0 && i< bufLen)
		strcpy(userNameBuf, cmdbuf[0]);
	else 
		return false;
	return true;
}

bool KillProcessWithID(unsigned long prcID)
{
	if(!prcID) return false;
	if(kill(prcID, SIGKILL)!=0) 
		return false;
	return true;
}

unsigned long RestartProcessWithID(unsigned long prcID)
{
	if(!prcID) return 0;
	unsigned long dwPrcID = 0;
    	FILE* fp = NULL;
    	char cmdLine[256];
	char cmdBuf[300];
    	char file[128] = {0};
    	char buf[1024] = {0};
	char logonUserName[32] = {0};
	
    	sprintf(file,"/proc/%d/cmdline",prcID);
    	if (!(fp = fopen(file,"r")))
    	{
        	printf("read %s file fail!\n",file);
        	return dwPrcID;
    	}
    	//if (read_line(fp,line_buff,BUFF_LEN,1))
	if(fgets(buf,sizeof(buf),fp))
    	{
        	sscanf(buf,"%s",cmdLine);
			fclose(fp);
			printf("cmd line is %s\n",cmdLine);
    	}
	else 
	{
		fclose(fp);
		return dwPrcID;
	}

	if(kill(prcID,SIGKILL)!=0) 
		return dwPrcID;
	//waitpid();
	if(getSysLogonUserName(logonUserName, sizeof(logonUserName)))
	{
		usleep(10*1000);
		fp = NULL;
		//sprintf(cmdBuf,"su - %s -c %s &",logonUserName,cmdLine);
		//sprintf(cmdBuf,"DISPLAY=:0 su -c %s %s &",cmdLine,logonUserName);
		sprintf(cmdBuf,"DISPLAY=:0 su -c 'xterm -e /bin/bash -c '%s'' %s &",cmdLine,logonUserName);
		if((fp=popen(cmdBuf,"r"))==NULL)
		{
			printf("restart process failed,%s",cmdBuf);
			pclose(fp);
			return dwPrcID;	
		}
		pclose(fp);
	}
	else
		printf("restart process failed,%s",cmdBuf);
	dwPrcID = getPIDByName(cmdLine);
	return dwPrcID;
}

#define SCRIPT_FILE "./servicectl.sh"

bool IsSrvExist(char * srvName)
{
	bool bRet = false;
	if(GetSrvStatus(srvName) == 0)
		bRet = false;
	else
		bRet = true;
	return bRet;
}

unsigned long StartSrv(char * srvName)
{
    unsigned long dwRet = 0;
	FILE* fp = NULL;
	char cmdBuf[300];
	char buffer[128] = {0};

	sprintf(cmdBuf,"%s %s start", SCRIPT_FILE, srvName);
	if((fp=popen(cmdBuf,"r"))==NULL)
	{
		printf("Service not exist: %s",cmdBuf);
	}
	else
	{
		fgets(buffer, sizeof(buffer), fp);
		printf("BUFFER: %s", buffer);
		if(strcmp(buffer, " OK ")==0)
			dwRet = 1;
		else
			dwRet = 0;
	}
	pclose(fp);
	return dwRet;
}

unsigned long GetSrvStatus(char * srvName)
{
    unsigned long dwRet = 0;
    FILE* fp = NULL;
    char cmdBuf[300];
    char buffer[128] = {0};

    sprintf(cmdBuf,"%s %s status", SCRIPT_FILE, srvName);
    if((fp=popen(cmdBuf,"r"))==NULL)
    {
        printf("Service not exist: %s",cmdBuf);
    }
    else
    {
        fgets(buffer, sizeof(buffer), fp);
        printf("BUFFER: %s", buffer);
        if(strstr(buffer, "running...")>=0)
            dwRet = SERVICE_RUNNING;
        else if(strstr(buffer, "stopped")>=0)
            dwRet = SERVICE_STOPPED;
        else
        dwRet = 0;
    }
    pclose(fp);
    return dwRet;
}

unsigned long StopSrv(char * srvName)
{
   unsigned long dwRet = 0;
	FILE* fp = NULL;
	char cmdBuf[300];
	char buffer[128] = {0};

	sprintf(cmdBuf,"%s %s stop", SCRIPT_FILE, srvName);
	if((fp=popen(cmdBuf,"r"))==NULL)
	{
		printf("Service not exist: %s",cmdBuf);
	}
	else
	{
		fgets(buffer, sizeof(buffer), fp);
		printf("BUFFER: %s", buffer);
		if(strcmp(buffer, " OK ")==0)
			dwRet = 1;
		else
			dwRet = 0;
	}
	pclose(fp);
	return dwRet;
}
#endif
