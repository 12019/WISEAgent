#ifndef __ASZ_RCVY_H__
#define __ASZ_RCVY_H__

//-----------------------------------------------------------------------------
// Global functions declare:
//-----------------------------------------------------------------------------
BOOL GetVolume(char * volumeName);

#ifndef _is_linux
void CreateASZ(char * aszParams);
void CheckASZStatus();
BOOL GetASZExistRecord(char* existStatus);
BOOL ASZCreateParamCheck(char * aszParams);
BOOL WriteASZPercentRegistry(int aszPersent);
#else
LONGLONG GetFolderAvailSpace(char *path);
LONGLONG GetBackupNeedSpace(void);
#endif /*_is_linux*/


#endif /*__ASZ_RCVY_H__*/