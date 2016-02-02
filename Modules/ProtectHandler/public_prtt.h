#ifndef __PUBLIC_PRTT_H__
#define __PUBLIC_PRTT_H__

//-----------------------------------------------------------------------------
// Global functions declare:
//-----------------------------------------------------------------------------
BOOL DES_BASE64Decode(char * srcBuf,char *destBuf);
BOOL DES_BASE64Encode(char * srcBuf,char *destBuf);
BOOL GetFileMD5(char * filePath, char * retMD5Str);
BOOL GetMD5(char * buf, unsigned int bufSize, unsigned char retMD5[DEF_MD5_SIZE]);

#endif /*__PUBLIC_PRTT_H__*/