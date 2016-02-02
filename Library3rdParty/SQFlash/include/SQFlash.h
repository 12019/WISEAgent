#pragma once

typedef struct _ATA_SMART_ATTR_TABLE
{
   DWORD dwMaxProgram;
   DWORD dwAverageProgram;
   DWORD dwEnduranceCheck;
   DWORD dwPowerOnTime;
   DWORD dwEccCount;
   DWORD dwMaxReservedBlock;
   DWORD dwCurrentReservedBlock;
   DWORD dwGoodBlockRate;
} ATA_SMART_ATTR_TABLE, *PATA_SMART_ATTR_TABLE;

#ifdef __cplusplus 
extern "C" { 
#endif

int SQFlash_FindFirstDevice();
int SQFlash_FindNextDevice();
BOOL SQFlash_SelectDevice(int nDeviceNum);

BOOL SQFlash_Initialize(LPCTSTR pszAccessCode);
void SQFlash_UnInitialize();

BOOL SQFlash_GetSmartAttribute(PATA_SMART_ATTR_TABLE pASAT);
BOOL SQFlash_GetSmartAttributeStd(BYTE *pbuff, DWORD len);
BOOL SQFlash_GetSmartThresholdsStd(BYTE *pbuff, DWORD len);
BOOL SQFlash_GetDeviceModelName(TCHAR * pszModelName, DWORD cchBuffer);
BOOL SQFlash_GetStorageSN(TCHAR * pszSerialNumber, DWORD cchBuffer);
BOOL SQFlash_GetFirmwareVersion(TCHAR * pszFW, DWORD cchBuffer);

#ifdef __cplusplus 
}
#endif

BOOL SQFlash_IsUnusedSectorExist();
BOOL SQFlash_SetSecurityID(LPCTSTR pszSecurityID);
BOOL SQFlash_GetSecurityID(TCHAR * pszSecurityID, DWORD cchBuffer);
BOOL SQFlash_ClearSecurityID();

int SQFlash_GetSecurityID2_MaxLen();
BOOL SQFlash_SetSecurityID2(LPCTSTR pszSecurityID);
BOOL SQFlash_GetSecurityID2(TCHAR * pszSecurityID, DWORD cchBuffer);
BOOL SQFlash_ClearSecurityID2();

BOOL SQFlash_IsSupportFlashLock(HWND hWnd, UINT msgID);
BOOL SQFlash_IsFlashLockEnable(HWND hWnd, UINT msgID);
BOOL SQFlash_EnableFlashLock(LPCTSTR pszPassword, HWND hWnd, UINT msgID);
BOOL SQFlash_DisableFlashLock(HWND hWnd, UINT msgID);

BOOL SQFlash_EraseAllBlock();
BOOL SQFlash_Trim();
BOOL SQFlash_IsTrimSupported();
