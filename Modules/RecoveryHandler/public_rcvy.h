#ifndef __PUBLIC_RCVY_H__
#define __PUBLIC_RCVY_H__

//-----------------------------------------------------------------------------
// Global functions declare:
//-----------------------------------------------------------------------------
int GetFileLineCount(const char * pFileName);
BOOL FileCopy(const char * srcFilePath, const char * destFilePath);
BOOL FileAppend(const char *srcFilePath, const char * destFilePath);
BOOL Str2Tm_YMD(const char *buf, const char *format, struct tm *tm);
BOOL Str2Tm(const char *buf, const char *format, struct tm *tm);
BOOL Str2Tm_MDY(const char *buf, const char *format, struct tm *tm);
#ifndef _is_linux
BOOL WriteDefaultReg();
#endif 

#endif /*__PUBLIC_RCVY_H__*/