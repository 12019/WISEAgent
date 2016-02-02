#ifndef _GLOBAL_H_
#define _GLOBAL_H_

#ifdef WIN32
#include <Windows.h>

#define DEF_OS_NAME_LEN           34
#define DEF_OS_VERSION_LEN        64
#define DEF_BIOS_VERSION_LEN      256
#define DEF_PLATFORM_NAME_LEN     128
#define DEF_PROCESSOR_NAME_LEN    64
#define OS_UNKNOW                 "Unknow"
#define OS_WINDOWS_95             "Windows 95"
#define OS_WINDOWS_98             "Windows 98"
#define OS_WINDOWS_ME             "Windows ME"
#define OS_WINDOWS_NT_3_51        "Windows NT 3.51"
#define OS_WINDOWS_NT_4           "Windows NT 4"
#define OS_WINDOWS_2000           "Windows 2000"
#define OS_WINDOWS_XP             "Windows XP"
#define OS_WINDOWS_SERVER_2003    "Windows Server 2003"
#define OS_WINDOWS_VISTA          "Windows Vista"
#define OS_WINDOWS_7              "Windows 7"

BOOL GetOSName(char * pOSNameBuf);

BOOL GetDefProgramFilesPath(char *pProgramFilesPath);

#include <objbase.h>
#pragma comment(lib, "ole32.lib")
#define DEF_GUID_BUF_LEN          37
#define DIV                       (1024)

BOOL GetGUID(char * guidBuf, int bufSize);

BOOL AdjustPrivileges();

HANDLE GetProcessHandle(char * processName);

HANDLE GetProcessHandleWithID(DWORD prcID);

BOOL GetProcessUsername(HANDLE hProcess, char * userNameBuf, int bufLen);

BOOL IsProcessActive(char * processName);

BOOL IsProcessActiveWithID(DWORD prcID);

BOOL ProcessCheck(char * processName);

int ProcessCheckWithID(DWORD prcID);

BOOL KillProcessWithName(char * processName);

BOOL KillProcessWithID(DWORD prcID);

BOOL RestartProcess(char * prcName);

DWORD RestartProcessWithID(DWORD prcID);

#define DEF_OS_ARCH_LEN           16
#define ARCH_UNKNOW               "Unknow"
#define ARCH_X64                  "X64"
#define ARCH_ARM                  "ARM"
#define ARCH_IA64                 "IA64"
#define ARCH_X86                  "X86"

BOOL GetOSArch(char * osArchBuf, int bufLen);

BOOL GetProcessorName(char * processorNameBuf, DWORD * bufLen);

BOOL GetSysBIOSVersion(char * biosVersionBuf, DWORD * bufLen);

BOOL GetOSVersion(char * osVersionBuf,  int bufLen);

BOOL GetSysLogonUserName(char * userNameBuf, unsigned int bufLen);

BOOL GetSysMemoryUsageKB(long * totalPhysMemKB, long * availPhysMemKB);

BOOL GetProcessMemoryUsageKB(char * processName, long * memUsageKB);

BOOL GetProcessMemoryUsageKBWithID(DWORD prcID, long * memUsageKB);

BOOL GetSysPrcListStr(char * sysPrcListStr);

BOOL GetLogonUserPrcListStr(char * sysPrcListStr);

BOOL GetDiskSizeBytes(unsigned int diskNum, LONGLONG *outSize);

int GetDiskPartitionCnt();

BOOL GetDiskPatitionSizeBytes(char *partName, LONGLONG *freeSpaceSizeToCaller, LONGLONG *totalSpaceSize);

BOOL GetMinAndMaxSpaceMB( LONGLONG *minSpace, LONGLONG *maxSpace);

BOOL GetFirstMac(char * macStr);

int GetAllMacs(char macsStr[][20], int n);

BOOL GetLastBootupTimeStr(char * timeStr);

BOOL GetBootupPeriodStr(char *periodStr);

DWORD StartSrv(char * srvName);

DWORD GetSrvStatus(char * srvName);

BOOL IsSrvExist(char * srvName);

DWORD StopSrv(char * srvName);

BOOL GetModuleFile(char * moudleFile);

BOOL GetMoudlePath(char * moudlePath);

BOOL GetCurrentDir(char * curDir);

BOOL FileIsUnicode(char * fileName);

int GetRandomInt(unsigned int startNum, unsigned int endNum);

BOOL GetRandomStr(char *randomStrBuf, int bufSize);

BOOL GetHostIP(char * ipStr);

BOOL GetFileVersion(char * pFindFilePath, char * pVersion, DWORD nSize);

#endif

#ifndef BOOL
#define BOOL int
#endif

#ifndef FALSE
#define FALSE 0
#endif

#ifndef TRUE
#define TRUE 1
#endif

int GetFileLineCount(const char * pFileName);

BOOL IsFileExist(const char * pFilePath);

BOOL FileCopy(const char * srcFilePath, const char * destFilePath);

BOOL FileAppend(const char *srcFilePath, const char * destFilePath);

void TrimStr(char * str);

BOOL Str2Tm(const char *buf, const char *format, struct tm *tm);

BOOL Str2Tm_MDY(const char *buf, const char *format, struct tm *tm);

BOOL Str2Tm_YMD(const char *buf, const char *format, struct tm *tm);

#endif