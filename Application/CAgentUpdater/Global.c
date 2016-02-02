#ifdef WIN32
#include <WinSock2.h>
#include <Windows.h>
#include <direct.h>
#include <tlhelp32.h>
#include <IPHlpApi.h>
#include <psapi.h>
#pragma comment(lib,"Psapi.lib")
#pragma comment(lib,"Iphlpapi.lib")
#pragma comment(lib,"ws2_32.lib") 
#pragma comment(lib,"version.lib")
#endif
#include <memory.h>
#include <stdio.h>
#include <io.h>
#include <time.h>
#include "Global.h"

#ifdef WIN32
BOOL GetOSName(char * pOSNameBuf)
{
   BOOL bRet = FALSE;
   OSVERSIONINFO osvInfo;
   BOOL isUnknow = FALSE;
   if(NULL == pOSNameBuf) return bRet;

   memset(&osvInfo, 0, sizeof(OSVERSIONINFO));
   osvInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
   bRet = GetVersionEx(&osvInfo);
   if(!bRet) return bRet;
   switch(osvInfo.dwPlatformId)
   {
   case VER_PLATFORM_WIN32_WINDOWS:
      {
         if( 4 == osvInfo.dwMajorVersion )
         {
            switch(osvInfo.dwMinorVersion)
            {
            case 0:
               {
                  memcpy(pOSNameBuf, OS_WINDOWS_95, sizeof(OS_WINDOWS_95));
                  break;
               }
            case 10:
               {
                  memcpy(pOSNameBuf, OS_WINDOWS_98, sizeof(OS_WINDOWS_98));
                  break;
               }
            case 90:
               {
                  memcpy(pOSNameBuf, OS_WINDOWS_ME, sizeof(OS_WINDOWS_ME));
                  break;
               }
            default:
               {
                  isUnknow = TRUE;
                  break;
               }
            }
         }
         break;
      }
   case VER_PLATFORM_WIN32_NT:
      {
         switch(osvInfo.dwMajorVersion)
         {
         case 3:
            {
               memcpy(pOSNameBuf, OS_WINDOWS_NT_3_51, sizeof(OS_WINDOWS_NT_3_51));
               break;
            }
         case 4:
            {
               memcpy(pOSNameBuf, OS_WINDOWS_NT_4, sizeof(OS_WINDOWS_NT_4));
               break;
            }
         case 5:
            {
               switch(osvInfo.dwMinorVersion)
               {
               case 0:
                  {
                     memcpy(pOSNameBuf, OS_WINDOWS_2000, sizeof(OS_WINDOWS_2000));
                     break;
                  }
               case 1:
                  {
                     memcpy(pOSNameBuf, OS_WINDOWS_XP, sizeof(OS_WINDOWS_XP));
                     break;
                  }
               case 2:
                  {
                     memcpy(pOSNameBuf, OS_WINDOWS_SERVER_2003, sizeof(OS_WINDOWS_SERVER_2003));
                     break;
                  }
               default:
                  {
                     isUnknow = TRUE;
                     break;
                  }
               }
               break;
            }
         case 6:
            {
               switch(osvInfo.dwMinorVersion)
               {
               case 0:
                  {
                     memcpy(pOSNameBuf, OS_WINDOWS_VISTA, sizeof(OS_WINDOWS_VISTA));
                     break;
                  }
               case 1:
                  {
                     memcpy(pOSNameBuf, OS_WINDOWS_7, sizeof(OS_WINDOWS_7));
                     break;
                  }
               default:
                  {
                     isUnknow = TRUE;
                     break;
                  }
               }
               break;
            }
         default:
            {
               isUnknow = TRUE;
               break;
            }
         }
         break;
      }
   default: 
      {
         isUnknow = TRUE;
         break;
      }
   }
   if(isUnknow) memcpy(pOSNameBuf, OS_UNKNOW, sizeof(OS_UNKNOW));
   return bRet;
}

BOOL GetDefProgramFilesPath(char *pProgramFilesPath)
{
   BOOL bRet = FALSE;
   HKEY hk;
   char regName[] = "SOFTWARE\\Microsoft\\Windows\\CurrentVersion";
   if(NULL == pProgramFilesPath) return bRet;

   if(ERROR_SUCCESS != RegOpenKeyEx(HKEY_LOCAL_MACHINE, regName, 0, KEY_ALL_ACCESS, &hk)) return bRet;
   else
   {
      char valueName[] = "ProgramFilesDir";
      char valueData[MAX_PATH] = {0};
      int  valueDataSize = sizeof(valueData);
      if(ERROR_SUCCESS != RegQueryValueEx(hk, valueName, 0, NULL, valueData, &valueDataSize)) return bRet;
      else
      {
         strcpy(pProgramFilesPath, valueData);
         bRet = TRUE;
      }
      RegCloseKey(hk);
   }
   return bRet;
}

BOOL GetGUID(char * guidBuf, int bufSize)
{
   BOOL bRet = FALSE;
   GUID guid;
   if(NULL == guidBuf || bufSize <= 0) return bRet;
   if(S_OK == CoCreateGuid(&guid))
   {
      _snprintf(guidBuf, bufSize
         , "%08X-%04X-%04x-%02X%02X-%02X%02X%02X%02X%02X%02X"
         , guid.Data1, guid.Data2, guid.Data3, guid.Data4[0]
      , guid.Data4[1] , guid.Data4[2], guid.Data4[3], guid.Data4[4]
      , guid.Data4[5]	, guid.Data4[6], guid.Data4[7] );
      bRet = TRUE;
   }
   return bRet;
}

BOOL AdjustPrivileges() 
{
   HANDLE hToken;
   TOKEN_PRIVILEGES tp;
   TOKEN_PRIVILEGES oldtp;
   DWORD dwSize=sizeof(TOKEN_PRIVILEGES);
   LUID luid;

   if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken)) 
   {
      if (GetLastError()==ERROR_CALL_NOT_IMPLEMENTED) return TRUE;
      else return FALSE;
   }

   if (!LookupPrivilegeValue(NULL, SE_DEBUG_NAME, &luid)) 
   {
      CloseHandle(hToken);
      return FALSE;
   }

   memset(&tp, 0, sizeof(tp));
   tp.PrivilegeCount=1;
   tp.Privileges[0].Luid=luid;
   tp.Privileges[0].Attributes=SE_PRIVILEGE_ENABLED;

   if (!AdjustTokenPrivileges(hToken, FALSE, &tp, sizeof(TOKEN_PRIVILEGES), &oldtp, &dwSize)) 
   {
      CloseHandle(hToken);
      return FALSE;
   }

   CloseHandle(hToken);
   return TRUE;
}

HANDLE GetProcessHandle(char * processName)
{
   HANDLE hPrc = NULL;
   PROCESSENTRY32 pe;
   HANDLE hSnapshot=NULL;
   if(NULL == processName) return hPrc;
   hSnapshot=CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS,0);
   pe.dwSize=sizeof(PROCESSENTRY32);
   if(!Process32First(hSnapshot,&pe))
      return hPrc;
   while(TRUE)
   {
      pe.dwSize=sizeof(PROCESSENTRY32);
      if(Process32Next(hSnapshot,&pe)==FALSE)
         break;
      if(strcmp(pe.szExeFile, processName)==0)
      {
         hPrc = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pe.th32ProcessID);

         if(hPrc == NULL) 
         {
            DWORD dwRet = GetLastError();          
            if(dwRet == 5)
            {
               if(AdjustPrivileges())
               {
                  hPrc = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pe.th32ProcessID);
               }
            }
         }
         break;
      }
   }
   if(hSnapshot) CloseHandle(hSnapshot);
   return hPrc;
}

HANDLE GetProcessHandleWithID(DWORD prcID)
{
   HANDLE hPrc = NULL;
   hPrc = OpenProcess(PROCESS_ALL_ACCESS, FALSE, prcID);
   if(hPrc == NULL) 
   {
      DWORD dwRet = GetLastError();          
      if(dwRet == 5)
      {
         if(AdjustPrivileges())
         {
            hPrc = OpenProcess(PROCESS_ALL_ACCESS, FALSE, prcID);
         }
      }
   }
   return hPrc;
}

BOOL GetProcessUsername(HANDLE hProcess, char * userNameBuf, int bufLen) 
{
   BOOL bRet = FALSE;
   if(hProcess == NULL || userNameBuf == NULL || bufLen <= 0) return bRet;
   {
      HANDLE hToken;
      TOKEN_USER * pTokenUser = NULL;
      if (OpenProcessToken(hProcess,TOKEN_QUERY,&hToken))
      {
         DWORD dwNeedLen = 0;
         GetTokenInformation(hToken, TokenUser, NULL, 0, &dwNeedLen);
         if(dwNeedLen > 0)
         {
            pTokenUser = (TOKEN_USER *)malloc(dwNeedLen);
            if(GetTokenInformation(hToken, TokenUser, pTokenUser, dwNeedLen, &dwNeedLen))
            {
               SID_NAME_USE sn;
               char szDomainName[MAX_PATH] = {0};
               DWORD dwDmLen = MAX_PATH;
               if(LookupAccountSid(NULL, pTokenUser->User.Sid, userNameBuf, &bufLen, szDomainName, &dwDmLen, &sn))
               {
                  bRet = TRUE;
               }
            }
            if(pTokenUser)
            {
               free(pTokenUser);
               pTokenUser = NULL;
            }
         }
         CloseHandle(hToken);
      }
   }

   return bRet;
}

BOOL IsProcessActive(char * processName)
{
   HANDLE hPrc = NULL;
   BOOL bRet = FALSE;
   DWORD dwRet = 0;
   if(NULL == processName) return bRet;
   hPrc = GetProcessHandle(processName);
   if(hPrc == NULL ) return bRet;
   if(GetExitCodeProcess(hPrc, &dwRet))
   {
      if(dwRet == STILL_ACTIVE) bRet = TRUE;
   }

   CloseHandle(hPrc);
   return bRet;
}

BOOL IsProcessActiveWithID(DWORD prcID)
{
   HANDLE hPrc = NULL;
   BOOL bRet = FALSE;
   DWORD dwRet = 0;
   hPrc = GetProcessHandleWithID(prcID);
   if(hPrc == NULL ) return bRet;
   if(GetExitCodeProcess(hPrc, &dwRet))
   {
      if(dwRet == STILL_ACTIVE) bRet = TRUE;
   }

   CloseHandle(hPrc);
   return bRet;
}

BOOL ProcessCheck(char * processName)
{
   BOOL bRet = FALSE;
   PROCESSENTRY32 pe;
   DWORD id=0;
   HANDLE hSnapshot=NULL;
   if(NULL == processName) return bRet;
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
      if(strcmp(pe.szExeFile,processName)==0)
      {
         id=pe.th32ProcessID;
         bRet = TRUE;
         break;
      }
   }
   CloseHandle(hSnapshot);
   return bRet;
}

int ProcessCheckWithID(DWORD prcID)
{
   int iRet = -1;
   PROCESSENTRY32 pe;
   HANDLE hSnapshot=NULL;
   hSnapshot=CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS,0);
   pe.dwSize=sizeof(PROCESSENTRY32);
   if(!Process32First(hSnapshot,&pe))
   {
      CloseHandle(hSnapshot);
      return iRet;
   }
   iRet = 1;
   while(TRUE)
   {
      pe.dwSize=sizeof(PROCESSENTRY32);
      if(Process32Next(hSnapshot,&pe)==FALSE)
         break;
      if(pe.th32ProcessID == prcID)
      {
         iRet = 0;
         break;
      }
   }
   CloseHandle(hSnapshot);
   return iRet;
}

BOOL KillProcessWithName(char * processName)
{
   BOOL bRet = FALSE;
   PROCESSENTRY32 pe;
   HANDLE hSnapshot=NULL;
   if(NULL == processName) return bRet;
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
      if(strcmp(pe.szExeFile,processName)==0)
      {
         HANDLE hPrc = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pe.th32ProcessID);
         if(hPrc == NULL) 
         {
            DWORD dwRet = GetLastError();          
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
            bRet = TRUE;
            CloseHandle(hPrc);
         }

         break;
      }
   }
   CloseHandle(hSnapshot);
   return bRet;
}

BOOL KillProcessWithID(DWORD prcID)
{
	BOOL bRet = FALSE;
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
				DWORD dwRet = GetLastError();          
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
				bRet = TRUE;
				CloseHandle(hPrc);
			}

			break;
		}
	}
	CloseHandle(hSnapshot);
	return bRet;
}

BOOL RestartProcess(char * prcName)
{
   BOOL bRet = FALSE;
   HANDLE hPrc = NULL;
   PROCESSENTRY32 pe;
   HANDLE hSnapshot=NULL;
   if(NULL == prcName) return bRet;
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
      if(strcmp(pe.szExeFile, prcName)==0)
      {
         hPrc = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pe.th32ProcessID);

         if(hPrc == NULL) 
         {
            DWORD dwRet = GetLastError();          
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
            DWORD  needed;
            HMODULE hModule = NULL;
            char path[MAX_PATH] = {0};
            if(EnumProcessModules(hPrc, &hModule, sizeof(hModule), &needed))
            {
               if(GetModuleFileNameEx(hPrc, hModule, path, sizeof(path)))
               {               
                  TerminateProcess(hPrc, 0);    
                  {
                     char cmdLine[BUFSIZ] = {0};
                     STARTUPINFO si;
                     PROCESS_INFORMATION pi;

                     memset(&si, 0, sizeof(si));
                     si.dwFlags = STARTF_USESHOWWINDOW;  
                     si.wShowWindow = SW_HIDE;
                     si.cb = sizeof(si);
                     memset(&pi, 0, sizeof(pi));

                     sprintf(cmdLine, "%s \"%s\"", "cmd.exe /c ", path);

                     if(CreateProcess(NULL, cmdLine, NULL, NULL, FALSE, CREATE_NO_WINDOW , NULL, NULL, &si, &pi))
                     {
                        bRet = TRUE;
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
   return bRet;
}

DWORD RestartProcessWithID(DWORD prcID)
{
   DWORD dwPrcID = 0;
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
         hPrc = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pe.th32ProcessID);

         if(hPrc == NULL) 
         {
            DWORD dwRet = GetLastError();          
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
            DWORD  needed;
            HMODULE hModule = NULL;
            char path[MAX_PATH] = {0};
            if(EnumProcessModules(hPrc, &hModule, sizeof(hModule), &needed))
            {
               if(GetModuleFileNameEx(hPrc, hModule, path, sizeof(path)))
               {               
                  TerminateProcess(hPrc, 0);    
                  {
                     char cmdLine[BUFSIZ] = {0};
                     STARTUPINFO si;
                     PROCESS_INFORMATION pi;

                     memset(&si, 0, sizeof(si));
                     si.dwFlags = STARTF_USESHOWWINDOW;  
                     //si.wShowWindow = SW_HIDE;
                     si.wShowWindow = SW_SHOW;
                     si.cb = sizeof(si);
                     memset(&pi, 0, sizeof(pi));

                     sprintf(cmdLine, "%s", path);
                     //sprintf(cmdLine, "%s \"%s\"", "cmd.exe /c ", path);

                     if(CreateProcess(cmdLine, NULL, NULL, NULL, FALSE, CREATE_NO_WINDOW , NULL, NULL, &si, &pi))
                     //if(CreateProcess(NULL, cmdLine, NULL, NULL, FALSE, CREATE_NO_WINDOW , NULL, NULL, &si, &pi))
                     {
                        dwPrcID = pi.dwProcessId;
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

BOOL GetOSVersion(char * osVersionBuf,  int bufLen)
{
   BOOL bRet = FALSE;
   if(osVersionBuf == NULL || bufLen <= 0) return bRet;
   {
      OSVERSIONINFO osvInfo;
      BOOL isUnknow = FALSE;

      memset(&osvInfo, 0, sizeof(OSVERSIONINFO));
      osvInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
      bRet = GetVersionEx(&osvInfo);
      if(!bRet) return bRet;
      switch(osvInfo.dwPlatformId)
      {
      case VER_PLATFORM_WIN32_WINDOWS:
         {
            if( 4 == osvInfo.dwMajorVersion )
            {
               switch(osvInfo.dwMinorVersion)
               {
               case 0:
                  {
                     memcpy(osVersionBuf, OS_WINDOWS_95, sizeof(OS_WINDOWS_95));
                     break;
                  }
               case 10:
                  {
                     memcpy(osVersionBuf, OS_WINDOWS_98, sizeof(OS_WINDOWS_98));
                     break;
                  }
               case 90:
                  {
                     memcpy(osVersionBuf, OS_WINDOWS_ME, sizeof(OS_WINDOWS_ME));
                     break;
                  }
               default:
                  {
                     isUnknow = TRUE;
                     break;
                  }
               }
            }
            break;
         }
      case VER_PLATFORM_WIN32_NT:
         {
            switch(osvInfo.dwMajorVersion)
            {
            case 3:
               {
                  memcpy(osVersionBuf, OS_WINDOWS_NT_3_51, sizeof(OS_WINDOWS_NT_3_51));
                  break;
               }
            case 4:
               {
                  memcpy(osVersionBuf, OS_WINDOWS_NT_4, sizeof(OS_WINDOWS_NT_4));
                  break;
               }
            case 5:
               {
                  switch(osvInfo.dwMinorVersion)
                  {
                  case 0:
                     {
                        memcpy(osVersionBuf, OS_WINDOWS_2000, sizeof(OS_WINDOWS_2000));
                        break;
                     }
                  case 1:
                     {
                        memcpy(osVersionBuf, OS_WINDOWS_XP, sizeof(OS_WINDOWS_XP));
                        break;
                     }
                  case 2:
                     {
                        memcpy(osVersionBuf, OS_WINDOWS_SERVER_2003, sizeof(OS_WINDOWS_SERVER_2003));
                        break;
                     }
                  default:
                     {
                        isUnknow = TRUE;
                        break;
                     }
                  }
                  break;
               }
            case 6:
               {
                  switch(osvInfo.dwMinorVersion)
                  {
                  case 0:
                     {
                        memcpy(osVersionBuf, OS_WINDOWS_VISTA, sizeof(OS_WINDOWS_VISTA));
                        break;
                     }
                  case 1:
                     {
                        memcpy(osVersionBuf, OS_WINDOWS_7, sizeof(OS_WINDOWS_7));
                        break;
                     }
                  default:
                     {
                        isUnknow = TRUE;
                        break;
                     }
                  }
                  break;
               }
            default:
               {
                  isUnknow = TRUE;
                  break;
               }
            }
            break;
         }
      default: 
         {
            isUnknow = TRUE;
            break;
         }
      }
      if(isUnknow) memcpy(osVersionBuf, OS_UNKNOW, sizeof(OS_UNKNOW));
      else
      {
         sprintf_s(osVersionBuf, bufLen, "%s %s", osVersionBuf, osvInfo.szCSDVersion);
      }
   }

   return bRet;
}

BOOL GetProcessorName(char * processorNameBuf, DWORD * bufLen)
{
   BOOL bRet = FALSE;
   HKEY hk;
   char regName[] = "HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0";
   char valueName[] = "ProcessorNameString";
   char valueData[256] = {0};    
   DWORD  valueDataSize = sizeof(valueData);
   if(processorNameBuf == NULL || bufLen == NULL || *bufLen <= 0) return bRet;
   if(ERROR_SUCCESS != RegOpenKeyEx(HKEY_LOCAL_MACHINE, regName, 0, KEY_ALL_ACCESS, &hk)) return bRet;
   else
   {
      if(ERROR_SUCCESS != RegQueryValueEx(hk, valueName, 0, NULL, (LPBYTE)valueData, &valueDataSize)) 
      {
         RegCloseKey(hk);
         return bRet;
      }
      RegCloseKey(hk);
      bRet = valueDataSize == 0 ? FALSE : TRUE;
   }
   if(bRet)
   {
      unsigned int cpyLen = valueDataSize < *bufLen ? valueDataSize : *bufLen; 
      memcpy(processorNameBuf, valueData, cpyLen);
      *bufLen = cpyLen;
   }
   return bRet;
}

BOOL GetSysBIOSVersion(char * biosVersionBuf, DWORD * bufLen)
{
   BOOL bRet = FALSE;
   HKEY hk;
   char regName[] = "HARDWARE\\DESCRIPTION\\System";
   char valueName[] = "SystemBiosVersion";
   char valueData[1024] = {0};    
   DWORD  valueDataSize = sizeof(valueData);
   if(biosVersionBuf == NULL || bufLen == NULL || *bufLen <= 0) return bRet;
   if(ERROR_SUCCESS != RegOpenKeyEx(HKEY_LOCAL_MACHINE, regName, 0, KEY_ALL_ACCESS, &hk)) return bRet;
   else
   {
      if(ERROR_SUCCESS != RegQueryValueEx(hk, valueName, 0, NULL, (LPBYTE)valueData, &valueDataSize)) 
      {
         RegCloseKey(hk);
         return bRet;
      }
      RegCloseKey(hk);
      bRet = valueDataSize == 0 ? FALSE : TRUE;
   }
   if(bRet)
   {
      unsigned int cpyLen = valueDataSize < *bufLen ? valueDataSize : *bufLen; 
      memcpy(biosVersionBuf, valueData, cpyLen);
      *bufLen = cpyLen;
   }
   return bRet;
}

BOOL GetOSArch(char * osArchBuf, int bufLen)
{
   BOOL bRet = FALSE;
   if(osArchBuf == NULL || bufLen <= 0) return bRet;
   {
      SYSTEM_INFO siSysInfo;

      GetSystemInfo(&siSysInfo); 

      switch(siSysInfo.wProcessorArchitecture)
      {
      case PROCESSOR_ARCHITECTURE_AMD64:
         {
            memcpy(osArchBuf, ARCH_X64, sizeof(ARCH_X64));
            break;
         }
      case PROCESSOR_ARCHITECTURE_ARM:
         {
            memcpy(osArchBuf, ARCH_ARM, sizeof(ARCH_ARM));
            break;
         }
      case PROCESSOR_ARCHITECTURE_IA64:
         {
            memcpy(osArchBuf, ARCH_IA64, sizeof(ARCH_IA64));
            break;
         }
      case PROCESSOR_ARCHITECTURE_INTEL:
         {
            memcpy(osArchBuf, ARCH_X86, sizeof(ARCH_X86));
            break;
         }
      case PROCESSOR_ARCHITECTURE_UNKNOWN:
         {
            memcpy(osArchBuf, ARCH_UNKNOW, sizeof(ARCH_UNKNOW));
            break;
         }
      }
      bRet = TRUE;
   }
   return bRet;
}

BOOL GetSysLogonUserName(char * userNameBuf, unsigned int bufLen)
{
   BOOL bRet = FALSE;
   if(userNameBuf == NULL || bufLen <= 0) return bRet;
   {
      HANDLE eplHandle = NULL;
      eplHandle = GetProcessHandle("explorer.exe");
      if(eplHandle)
      {
         if(GetProcessUsername(eplHandle, userNameBuf, bufLen))
         {
            bRet = TRUE;
         }
      }
      if(eplHandle) CloseHandle(eplHandle);
   }
   return bRet;
}

BOOL GetSysMemoryUsageKB(long * totalPhysMemKB, long * availPhysMemKB) 
{ 
   MEMORYSTATUSEX memStatex;
   if(NULL == totalPhysMemKB || NULL == availPhysMemKB) return FALSE;
   memStatex.dwLength = sizeof (memStatex);
   GlobalMemoryStatusEx (&memStatex);

   *totalPhysMemKB = (long)(memStatex.ullTotalPhys/DIV);
   *availPhysMemKB = (long)(memStatex.ullAvailPhys/DIV);

   return TRUE;
}

BOOL GetProcessMemoryUsageKB(char * processName, long * memUsageKB) 
{ 
   BOOL bRet = FALSE;
   HANDLE hPrc = NULL;
   PROCESS_MEMORY_COUNTERS pmc;
   if(NULL == processName || NULL == memUsageKB) return FALSE;
   hPrc =GetProcessHandle(processName);

   memset(&pmc, 0, sizeof(pmc));

   if (GetProcessMemoryInfo(hPrc, &pmc, sizeof(pmc)))
   {
      *memUsageKB = (long)(pmc.WorkingSetSize/DIV);
      bRet = TRUE;
   }

   CloseHandle(hPrc);

   return bRet;
}

BOOL GetProcessMemoryUsageKBWithID(DWORD prcID, long * memUsageKB)
{
   BOOL bRet = FALSE;
   HANDLE hPrc = NULL;
   PROCESS_MEMORY_COUNTERS pmc;
   if(NULL == memUsageKB) return FALSE;
   hPrc =GetProcessHandleWithID(prcID);

   memset(&pmc, 0, sizeof(pmc));

   if (GetProcessMemoryInfo(hPrc, &pmc, sizeof(pmc)))
   {
      *memUsageKB = (long)(pmc.WorkingSetSize/DIV);
      bRet = TRUE;
   }

   CloseHandle(hPrc);

   return bRet;
}

BOOL GetSysPrcListStr(char * sysPrcListStr)
{
   PROCESSENTRY32 pe;
   HANDLE hSnapshot=NULL;
   if(NULL == sysPrcListStr) return FALSE;
   hSnapshot=CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS,0);
   pe.dwSize=sizeof(PROCESSENTRY32);
   if(!Process32First(hSnapshot,&pe))
   {
      CloseHandle(hSnapshot);
      return FALSE;
   }
   while(TRUE)
   {
      pe.dwSize=sizeof(PROCESSENTRY32);
      if(Process32Next(hSnapshot,&pe)==FALSE)
         break;
      if(strlen(sysPrcListStr) == 0) sprintf(sysPrcListStr, "%s", pe.szExeFile);
      else sprintf(sysPrcListStr, "%s,%s", sysPrcListStr, pe.szExeFile);
   }
   if(hSnapshot) CloseHandle(hSnapshot);
   return TRUE;
}

BOOL GetLogonUserPrcListStr(char * prcListStr)
{
   BOOL bRet = FALSE;
   if(NULL == prcListStr) return bRet;
   {
      PROCESSENTRY32 pe;
      HANDLE hSnapshot = NULL;
      char logonUserName[32] = {0};
      char procUserName[32] = {0};
      GetSysLogonUserName(logonUserName, sizeof(logonUserName));
      hSnapshot=CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS,0);
      pe.dwSize=sizeof(PROCESSENTRY32);
      if(Process32First(hSnapshot,&pe))
      {
         while(TRUE)
         {
            pe.dwSize=sizeof(PROCESSENTRY32);
            if(Process32Next(hSnapshot,&pe)==FALSE)
               break;
            {
               HANDLE procHandle = NULL;
               memset(procUserName, 0, sizeof(procUserName));
               procHandle = GetProcessHandleWithID(pe.th32ProcessID);
               if(procHandle)
               {
                  GetProcessUsername(procHandle, procUserName, sizeof(procUserName));
               }
               if(!strcmp(logonUserName, procUserName))
               {
                  if(strlen(prcListStr) == 0) sprintf(prcListStr, "%s,%d", pe.szExeFile, pe.th32ProcessID);
                  else sprintf(prcListStr, "%s;%s,%d", prcListStr, pe.szExeFile, pe.th32ProcessID);
               }
               if(procHandle) CloseHandle(procHandle);
            }
         }
      }
      if(hSnapshot) CloseHandle(hSnapshot);
      bRet = TRUE;
   }
   return bRet;
}

BOOL GetDiskSizeBytes(unsigned int diskNum, LONGLONG *outSize)
{
   BOOL bRet = FALSE;
   char strDiskName[32] = "\\\\.\\PHYSICALDRIVE";
   char strIndex[3] = {'\0'};
   HANDLE hDisk;
   _itoa_s(diskNum, strIndex, sizeof(strIndex), 10);
   strcat(strDiskName, strIndex);

   hDisk = CreateFile(strDiskName, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
   if (INVALID_HANDLE_VALUE == hDisk)
   {
      return bRet;
   }

   {
      DWORD dwReturnBytes = 0;
      GET_LENGTH_INFORMATION lenInfo;

      bRet = DeviceIoControl(hDisk, IOCTL_DISK_GET_LENGTH_INFO, NULL, 0, &lenInfo, sizeof(lenInfo), &dwReturnBytes, NULL);
      if (!bRet)
      {
         if (INVALID_HANDLE_VALUE != hDisk)
         {
            CloseHandle(hDisk);
         }
         return bRet;
      }
      *outSize = lenInfo.Length.QuadPart;
      bRet = TRUE;
   }

   if (INVALID_HANDLE_VALUE != hDisk)
   {
      CloseHandle(hDisk);
   }
   return bRet;
}

int GetDiskPartitionCnt()
{
   int cnt = 0;
   int index = 1;
   int i = 0;
   DWORD dwDrvNum = GetLogicalDrives();
   for(i = 0; i < sizeof(DWORD) * 8; i++)
   {
      if(dwDrvNum & (index << i))
      {
         cnt ++;
      }
   }
   
   return cnt;
}

BOOL GetDiskPatitionSizeBytes(char *partName, LONGLONG *freeSpaceSizeToCaller, LONGLONG *totalSpaceSize)
{
   BOOL bRet = FALSE;
   LONGLONG freeSpaceSize = 0;
   if(NULL == partName || NULL == freeSpaceSizeToCaller || NULL == totalSpaceSize) return bRet;

   bRet = GetDiskFreeSpaceEx(partName, (PULARGE_INTEGER)freeSpaceSizeToCaller, (PULARGE_INTEGER)totalSpaceSize, (PULARGE_INTEGER)&freeSpaceSize);

   return bRet;
}

BOOL GetMinAndMaxSpaceMB( LONGLONG *minSpace, LONGLONG *maxSpace)
{
   BOOL bRet = FALSE;
   LONGLONG totalDiskSpace = 0;
   LONGLONG totalAvailableSpace = 0;
   int diskDrvCnt = 0;
   int drvNamesStrLen = 0;
   char * pDrvNamesStr = NULL;
   if(NULL == minSpace || NULL == maxSpace) goto done;
   diskDrvCnt = GetDiskPartitionCnt();
   drvNamesStrLen = diskDrvCnt*4 + 1;
   pDrvNamesStr = (char *)malloc(drvNamesStrLen);
   memset(pDrvNamesStr, 0, drvNamesStrLen);
   GetDiskSizeBytes(0, &totalDiskSpace);

   drvNamesStrLen = GetLogicalDriveStrings(drvNamesStrLen, pDrvNamesStr);
   if(!drvNamesStrLen)
   {
      goto done;
   }

   {
      int drvIndex = 0;
      LONGLONG freeSpaceSizeToCaller = 0, totalSpaceSize = 0;
      char * pDrvName = NULL;
      int drvType = DRIVE_UNKNOWN;
      while(drvIndex < diskDrvCnt)
      {
         pDrvName = pDrvNamesStr + drvIndex * 4;
         drvType = GetDriveType(pDrvName);
         if(drvType == DRIVE_FIXED)
         {
            GetDiskPatitionSizeBytes(pDrvName, &freeSpaceSizeToCaller, &totalSpaceSize);
            if(!strcmp(pDrvName, "C:\\"))
            {
               *minSpace = (totalSpaceSize - freeSpaceSizeToCaller)/1024/1024;
            }
            totalAvailableSpace += (totalSpaceSize - freeSpaceSizeToCaller);
            totalSpaceSize = 0;
            freeSpaceSizeToCaller = 0;
         }
         drvIndex++;
      }
   }

   *maxSpace = (totalDiskSpace - totalAvailableSpace)/1024/1024;
   bRet = TRUE;
done:
   if(pDrvNamesStr) free(pDrvNamesStr);
   return bRet;
}

BOOL GetFirstMac(char * macStr)
{
   // Use IPHlpApi	
   BOOL bRet = FALSE;
   PIP_ADAPTER_INFO pAdapterInfo;
   PIP_ADAPTER_INFO pAdapter = NULL;
   PIP_ADAPTER_INFO pAdInfo = NULL;
   ULONG            ulSizeAdapterInfo = 0;  
   DWORD            dwStatus;  

   MIB_IFROW MibRow = {0};  
   if(NULL == macStr) return bRet;
   ulSizeAdapterInfo = sizeof(IP_ADAPTER_INFO); 
   pAdapterInfo = (PIP_ADAPTER_INFO)malloc(ulSizeAdapterInfo);


   if (GetAdaptersInfo( pAdapterInfo, &ulSizeAdapterInfo) != ERROR_SUCCESS) 
   {
      free (pAdapterInfo);
      pAdapterInfo = (PIP_ADAPTER_INFO)malloc(ulSizeAdapterInfo);
   }

   dwStatus = GetAdaptersInfo(pAdapterInfo,   &ulSizeAdapterInfo);  

   if(dwStatus != ERROR_SUCCESS)  
   {  
      free(pAdapterInfo);  
      return  bRet;  
   }  

   pAdInfo = pAdapterInfo; 
   while(pAdInfo)  
   {  	
      memset(&MibRow, 0, sizeof(MIB_IFROW));
      MibRow.dwIndex = pAdInfo->Index;  
      MibRow.dwType = pAdInfo->Type; 

      if(GetIfEntry(&MibRow) == NO_ERROR)  
      {  
         if (MibRow.dwOperStatus == MIB_IF_OPER_STATUS_OPERATIONAL)
         {

            sprintf(macStr, "%02X:%02X:%02X:%02X:%02X:%02X",
               MibRow.bPhysAddr[0],
               MibRow.bPhysAddr[1],
               MibRow.bPhysAddr[2],
               MibRow.bPhysAddr[3],
               MibRow.bPhysAddr[4],
               MibRow.bPhysAddr[5]);

            bRet = TRUE;
            break;
         }
      }
      pAdInfo = pAdInfo->Next; 
   }
   free(pAdapterInfo); 

   return bRet;
}

int GetAllMacs(char macsStr[][20], int n)
{
   int iRet = -1;
   PIP_ADAPTER_INFO pAdapterInfo;
   PIP_ADAPTER_INFO pAdapter = NULL;
   PIP_ADAPTER_INFO pAdInfo = NULL;
   ULONG            ulSizeAdapterInfo = 0;  
   DWORD            dwStatus;  

   MIB_IFROW MibRow = {0}; 
   int macIndex = 0;
   if(n <= 0) return iRet;
   ulSizeAdapterInfo = sizeof(IP_ADAPTER_INFO); 
   pAdapterInfo = (PIP_ADAPTER_INFO)malloc(ulSizeAdapterInfo);


   if (GetAdaptersInfo( pAdapterInfo, &ulSizeAdapterInfo) != ERROR_SUCCESS) 
   {
      free (pAdapterInfo);
      pAdapterInfo = (PIP_ADAPTER_INFO)malloc(ulSizeAdapterInfo);
   }

   dwStatus = GetAdaptersInfo(pAdapterInfo,   &ulSizeAdapterInfo);  

   if(dwStatus != ERROR_SUCCESS)  
   {  
      free(pAdapterInfo);  
      return  iRet;  
   }  

   pAdInfo = pAdapterInfo; 
   while(pAdInfo)  
   {  	
      if(pAdInfo->Type != MIB_IF_TYPE_ETHERNET && pAdInfo->Type != IF_TYPE_IEEE80211) 
      {
         pAdInfo = pAdInfo->Next; 
         continue;
      }

      memset(&MibRow, 0, sizeof(MIB_IFROW));
      MibRow.dwIndex = pAdInfo->Index;  
      MibRow.dwType = pAdInfo->Type;  

      if(GetIfEntry(&MibRow) == NO_ERROR)  
      {  
         //if (MibRow.dwOperStatus == MIB_IF_OPER_STATUS_OPERATIONAL)
         {

            sprintf(macsStr[macIndex], "%02X:%02X:%02X:%02X:%02X:%02X",
               MibRow.bPhysAddr[0],
               MibRow.bPhysAddr[1],
               MibRow.bPhysAddr[2],
               MibRow.bPhysAddr[3],
               MibRow.bPhysAddr[4],
               MibRow.bPhysAddr[5]);
            macIndex++;
            if(macIndex >= n) break;
         }
      }
      pAdInfo = pAdInfo->Next; 
   }
   free(pAdapterInfo); 
   iRet = macIndex;
   return iRet;
}

BOOL GetLastBootupTimeStr(char * timeStr)
{
	BOOL bRet = FALSE;
	DWORD dwTickMs = 0;
	time_t nowTime;
	time_t lastBootupTime;
	struct tm * pLastBootupTm = NULL;
	char timeTempStr[64] = {0};
	if(NULL == timeStr) return bRet;
	dwTickMs = GetTickCount();
   nowTime = time(NULL);
	lastBootupTime = nowTime - (time_t)dwTickMs/1000;
   pLastBootupTm = gmtime(&lastBootupTime);
	strftime(timeTempStr, sizeof( timeTempStr), "%Y-%m-%d %H:%M:%S", pLastBootupTm); 
	strcpy(timeStr, timeTempStr);
	bRet = TRUE;
	return bRet;
}

BOOL GetBootupPeriodStr(char *periodStr)
{
	BOOL bRet = FALSE;
	DWORD tickMs = 0;
	DWORD iDay = 0, iHour = 0, iMin = 0, iSec = 0;
	if(NULL == periodStr) return bRet;
	tickMs = GetTickCount();
	iDay = tickMs/(3600000*24);
	iHour = (tickMs-3600000*24*iDay)/3600000;
	iMin = (tickMs-3600000*24*iDay - 3600000*iHour)/60000;
   iSec = (tickMs-3600000*24*iDay - 3600000*iHour - iMin*60000)/1000;

	sprintf(periodStr,"%d %d:%d:%d", iDay, iHour, iMin, iSec);
	return TRUE;
}

DWORD StartSrv(char * srvName)
{
   DWORD dwRet = 0;
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
               memset(&srvStat, 0, sizeof(SERVICE_STATUS));
               QueryServiceStatus(hSrv, &srvStat);
               dwRet = srvStat.dwCurrentState;
            }
         }
      }
   }
   if(hSrv) CloseServiceHandle(hSrv);
   if(hSCM) CloseServiceHandle(hSCM);
   return dwRet;
}

DWORD GetSrvStatus(char * srvName)
{
   DWORD dwRet = 0;
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

BOOL IsSrvExist(char * srvName)
{
   DWORD dwRet = 0;
   SC_HANDLE hSCM = NULL;
   SC_HANDLE hSrv = NULL;
   if(NULL == srvName) return dwRet;
   hSCM = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
   if(!hSCM) return FALSE;
   hSrv = OpenService(hSCM, srvName, SERVICE_ALL_ACCESS);
   if(NULL == hSrv)
   {
      DWORD dwErrorCode = GetLastError();
      if(ERROR_SERVICE_DOES_NOT_EXIST == dwErrorCode) return FALSE;
   }
   if(hSrv) CloseServiceHandle(hSrv);
   if(hSCM) CloseServiceHandle(hSCM);
   return TRUE;
}

DWORD StopSrv(char * srvName)
{
   DWORD dwRet = 0;
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
            Sleep(250);
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

BOOL GetModuleFile(char * moudleFile)
{
   BOOL bRet = FALSE;
   char tempPath[MAX_PATH] = {0};
   if(NULL == moudleFile) return bRet;
   if(ERROR_SUCCESS != GetModuleFileName(NULL, tempPath, sizeof(tempPath)))
   {
      strcpy(moudleFile, tempPath);
      bRet = TRUE;
   }
   return bRet;
}

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

BOOL GetCurrentDir(char * curDir)
{
   char curDirBuf[MAX_PATH] = {0};
   if(NULL == curDir) return FALSE;
   _getcwd(curDirBuf,MAX_PATH);
   strcpy(curDir, curDirBuf);
   return TRUE;
}

BOOL FileIsUnicode(char * fileName)
{
   BOOL bRet = FALSE;
   FILE *fp = NULL;
   unsigned char headBuf[2] = {0};
   if(NULL == fileName) return bRet;
   fp = fopen(fileName, "r");
   if(NULL == fp) return bRet;
   fread(headBuf, sizeof(unsigned char), 2, fp);
   fclose(fp);
   if(headBuf[0] == 0xFF && headBuf[1] == 0xFE) bRet = TRUE;
   return bRet;
}

int GetRandomInt(unsigned int startNum, unsigned int endNum)
{
   int randNum = -1;
   if(endNum < startNum) return randNum;
   {
      LARGE_INTEGER perfCnt = {0};
      QueryPerformanceCounter(&perfCnt);
      srand((unsigned int)perfCnt.QuadPart); //us

      //srand((unsigned int)GetTickCount());  //10ms
      //Sleep(11);  //10ms deviation

      //srand((unsigned int)time(NULL)); //sec

      randNum = startNum + rand()%(endNum - startNum + 1);
   }
   return randNum;
}

BOOL GetRandomStr(char *randomStrBuf, int bufSize)
{
   int flag = 0, strLen = 0, i = 0;
   LARGE_INTEGER perfCnt = {0};
   if(NULL == randomStrBuf || bufSize <= 0) return FALSE;

   QueryPerformanceCounter(&perfCnt);
   srand((unsigned int)perfCnt.QuadPart); //us

   //srand((unsigned int)GetTickCount());  //10ms
   //Sleep(11);  //10ms deviation

   //srand((unsigned int)time(NULL)); //sec

   strLen = bufSize - 1;

   while(i < strLen)
   {
      flag = rand()%3;
      switch(flag)
      {
      case 0:
         {
            randomStrBuf[i] = 'a' + rand()%26;
            break;
         }
      case 1:
         {
            randomStrBuf[i] = '0' + rand()%10;
            break;
         }
      case 2:
         {
            randomStrBuf[i] = 'A' + rand()%26;
            break;
         }
      default: break;
      }
      i++;
   }
   randomStrBuf[strLen] = '\0';
   return TRUE; 
}


BOOL GetHostIP(char * ipStr)
{
   
   BOOL bRet = FALSE;
   WSADATA wsaData;
   char hostName[255] = {0};
   int i = 0;
   struct in_addr curAddr;
   struct hostent *phostent = NULL;
   if(NULL == ipStr) return bRet;

   if (WSAStartup(MAKEWORD(1, 1), &wsaData) != 0) return bRet;

   if (gethostname(hostName, sizeof(hostName)) == SOCKET_ERROR) return bRet;

   phostent = gethostbyname(hostName);
   if (phostent == NULL) return bRet;

   for (i = 0; phostent->h_addr_list[i] != 0; ++i) 
   {
      memset(&curAddr, 0, sizeof(struct in_addr));
      memcpy(&curAddr, phostent->h_addr_list[i], sizeof(struct in_addr));
      strcpy(ipStr, inet_ntoa(curAddr));
      if(strlen(ipStr) >0)
      {
         bRet = TRUE;
         break;
      }
   }

   WSACleanup();

   return bRet;
}

BOOL GetFileVersion(char * pFindFilePath, char * pVersion, DWORD nSize)
{
   DWORD dwCount = 0, dwHandle = 0, dwValueLen = 0;
   BOOL  bRet = FALSE;
   char  *pVerValue = NULL, *pc = NULL, *pVerInfoBuffer = NULL, szQuery[100] = {0};

   if (nSize == 0 || pFindFilePath == NULL || pVersion == NULL) return bRet;

   if ((dwCount = GetFileVersionInfoSize(pFindFilePath, &dwHandle)) != 0)
   {
      pVerInfoBuffer = (char *)malloc(dwCount);
      if (!pVerInfoBuffer) return bRet;

      if (GetFileVersionInfo(pFindFilePath, dwHandle, dwCount, pVerInfoBuffer) != 0)
      {
         BOOL bVer = VerQueryValue(pVerInfoBuffer, "\\VarFileInfo\\Translation", 
            (void **) &pVerValue, (unsigned int *) &dwValueLen);

         if (bVer && dwValueLen != 0)
         {   
            wsprintf(szQuery, "\\StringFileInfo\\%04X%04X\\FileVersion", 
               *(WORD *)pVerValue, *(WORD *)(pVerValue+2));    
            bRet = VerQueryValue(pVerInfoBuffer, szQuery, (void **) &pVerValue,(unsigned int *) &dwValueLen);
            if (bRet)
            {
               while ((pc = strchr(pVerValue, '(')) != NULL)
                  *pc = '{';
               while ((pc = strchr(pVerValue, ')')) != NULL)
                  *pc = '}';

               strncpy(pVersion, pVerValue, nSize);
               pVersion[nSize - 1] = '\0';
            }
         }
      }
      free(pVerInfoBuffer);
   }
   return bRet;
}

#endif



int GetFileLineCount(const char * pFileName)
{
   FILE * fp = NULL;
   char lineBuf[BUFSIZ] = {0};
   char * p = NULL;
   int lineLen = 0;
   int lineCount = 0;
   int isCountAdd = 0;
   if(NULL == pFileName) return -1;

   fp = fopen(pFileName, "rb");
   if(fp)
   {
      while ((lineLen = fread(lineBuf, 1, BUFSIZ, fp)) != 0) 
      {   
         isCountAdd = 1;
         p = lineBuf;
         while ((p = (char*)memchr((void*)p, '\n', (lineBuf + lineLen) - p)))
         {
            ++p;
            ++lineCount;
            isCountAdd = 0;
         }
         memset(lineBuf, 0, BUFSIZ);
      }
      if(isCountAdd) ++lineCount;
      fclose(fp);
   }
   return lineCount;
}

BOOL IsFileExist(const char * pFilePath)
{
   if(NULL == pFilePath) return FALSE;
   return _access(pFilePath, 0) == 0 ? TRUE : FALSE;
}

BOOL FileCopy(const char * srcFilePath, const char * destFilePath)
{
   BOOL bRet = FALSE;
   FILE *fpSrc = NULL, *fpDest = NULL;
   if(NULL == srcFilePath || NULL == destFilePath) goto done;
   fpSrc = fopen(srcFilePath, "rb");
   if(NULL == fpSrc) goto done;
   fpDest = fopen(destFilePath, "wb");
   if(NULL == fpDest) goto done;
   {
      char buf[BUFSIZ] = {0};
      int size = 0;
      while ((size = fread(buf, 1, BUFSIZ, fpSrc)) != 0) 
      { 
         fwrite(buf, 1, size, fpDest);
      }
      bRet = TRUE;
   }
done:
   if(fpSrc) fclose(fpSrc);
   if(fpDest) fclose(fpDest);
   return bRet;
}

BOOL FileAppend(const char *srcFilePath, const char * destFilePath)
{
   BOOL bRet = FALSE;
   FILE *fpSrc = NULL, *fpDest = NULL;
   if(NULL == srcFilePath || NULL == destFilePath) goto done;
   fpSrc = fopen(srcFilePath, "rb");
   if(NULL == fpSrc) goto done;
   fpDest = fopen(destFilePath, "ab");
   if(NULL == fpDest) goto done;
   {
      char lineBuf[BUFSIZ*2] = {0};
      while (!feof(fpSrc)) 
      {
         memset(lineBuf, 0, sizeof(lineBuf));
         if(fgets(lineBuf, sizeof(lineBuf), fpSrc))
         {
            fputs(lineBuf, fpDest);
         }
      }
      bRet = TRUE;
   }
done:
   if(fpSrc) fclose(fpSrc);
   if(fpDest) fclose(fpDest);
   return bRet;
}

void TrimStr(char * str)
{
   char *p, *stPos, *edPos;
   if(NULL == str) return;

   for (p = str; (*p == ' ' || *p == '\t') && *p != '\0'; p++);
   edPos = stPos = p;
   for (; *p != '\0'; p++)
   {
      if(*p != ' ' && *p != '\t') edPos = p;   
   }
   memmove(str, stPos, edPos - stPos + 1);
   *(str + (edPos - stPos + 1)) = '\0' ;
}

BOOL Str2Tm(const char *buf, const char *format, struct tm *tm)
{
	BOOL bRet = FALSE;
   int iYear = 0, iMon = 0, iDay = 0, iHour = 0, iMin = 0, iSec = 0;
	if(NULL == buf || NULL == format || NULL == tm) return bRet;
	if(sscanf(buf, format, &iYear, &iMon, &iDay, &iHour, &iMin, &iSec) != 0)
	{
      tm->tm_year = iYear - 1900;
      tm->tm_mon = iMon - 1;
      tm->tm_mday = iDay;
      tm->tm_hour = iHour;
      tm->tm_min = iMin;
      tm->tm_sec = iSec;
		tm->tm_isdst = 0;
		bRet = TRUE;
	}
   return bRet;
}

BOOL Str2Tm_MDY(const char *buf, const char *format, struct tm *tm)
{
   BOOL bRet = FALSE;
   int iYear = 0, iMon = 0, iDay = 0, iHour = 0, iMin = 0, iSec = 0;
   if(NULL == buf || NULL == format || NULL == tm) return bRet;
   if(sscanf(buf, format, &iMon, &iDay, &iYear, &iHour, &iMin, &iSec) != 0)
   {
      tm->tm_year = iYear - 1900;
      tm->tm_mon = iMon - 1;
      tm->tm_mday = iDay;
      tm->tm_hour = iHour;
      tm->tm_min = iMin;
      tm->tm_sec = iSec;
      tm->tm_isdst = 0;
      bRet = TRUE;
   }
   return bRet;
}

BOOL Str2Tm_YMD(const char *buf, const char *format, struct tm *tm)
{
   BOOL bRet = FALSE;
   int iYear = 0, iMon = 0, iDay = 0, iHour = 0, iMin = 0, iSec = 0;
   if(NULL == buf || NULL == format || NULL == tm) return bRet;
   if(sscanf(buf, format, &iYear, &iMon, &iDay,  &iHour, &iMin, &iSec) != 0)
   {
      tm->tm_year = iYear - 1900;
      tm->tm_mon = iMon - 1;
      tm->tm_mday = iDay;
      tm->tm_hour = iHour;
      tm->tm_min = iMin;
      tm->tm_sec = iSec;
      tm->tm_isdst = 0;
      bRet = TRUE;
   }
   return bRet;
}

