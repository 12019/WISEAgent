#include "ProtectHandler.h"
#include "public_prtt.h"


//-----------------------------------------------------------------------------
// Local Types/Macros/Variables:
//-----------------------------------------------------------------------------
#ifdef _WIN32
#define DEF_DES_KEY                              "McAfee21"
#else
#define DEF_DES_KEY                              "ia82dawd"
#define DEF_DES_IV                               "p320#1ds"
#endif /*_WIN32*/


//-----------------------------------------------------------------------------
// Global functions define:
//-----------------------------------------------------------------------------
BOOL DES_BASE64Decode(char * srcBuf,char *destBuf)
{
	BOOL bRet = FALSE;
	char plaintext[512] = {0};
	int iRet = 0;
	if(srcBuf == NULL || destBuf == NULL) return bRet;
	{
		char *base64Dec = NULL;
		int decLen = 0;
		int len = strlen(srcBuf);
		iRet = Base64Decode(srcBuf, len, &base64Dec, &decLen);
		if(iRet == 0)
		{
#ifdef _WIN32
			iRet = DESDecodeEx(DEF_DES_KEY, DEF_DES_KEY,  base64Dec, decLen, plaintext);
#else
			iRet = DESDecodeEx(DEF_DES_KEY, DEF_DES_IV,  base64Dec, decLen, plaintext);
#endif
			if(iRet == 0)
			{
				len = strlen(plaintext);
				memcpy(destBuf, plaintext, len + 1);
				bRet = TRUE;
			}
		}
		if(base64Dec) free(base64Dec);
	}

	return bRet;
}

BOOL DES_BASE64Encode(char * srcBuf,char *destBuf)
{
	BOOL bRet = FALSE;
	char ciphertext[128] = {0};
	int cipherLen = 0;
	int iRet = 0;
	if(srcBuf == NULL || destBuf == NULL) return bRet;

#ifdef _WIN32
	iRet = DESEncodeEx(DEF_DES_KEY, DEF_DES_KEY, srcBuf, ciphertext, &cipherLen);
#else
	iRet = DESEncodeEx(DEF_DES_KEY, DEF_DES_IV, srcBuf, ciphertext, &cipherLen);
#endif
	if(iRet == 0)
	{
		char *base64Enc = NULL;
		int encLen = 0;
		int len = cipherLen;
		iRet= Base64Encode(ciphertext, len, &base64Enc, &encLen);
		if(iRet == 0)
		{
			len = strlen(base64Enc);
			if(len > 0)
			{
				memcpy(destBuf, base64Enc, len+1);
				bRet = TRUE;
			}
		}
		if(base64Enc) free(base64Enc);
	}
	return bRet;
}

BOOL GetFileMD5(char * filePath, char * retMD5Str)
{
#define  DEF_PER_MD5_DATA_SIZE       512
	BOOL bRet = FALSE;
	if(NULL == filePath || NULL == retMD5Str) return bRet;
	{
		FILE *fptr = NULL;
		fptr = fopen(filePath, "rb");
		if(fptr)
		{
			MD5_CTX context;
			unsigned char retMD5[DEF_MD5_SIZE] = {0};
			char dataBuf[DEF_PER_MD5_DATA_SIZE] = {0};
			unsigned int readLen = 0, realReadLen = 0;
			MD5Init(&context);
			readLen = sizeof(dataBuf);
			while ((realReadLen = fread(dataBuf, sizeof(char), readLen, fptr)) != 0)
			{
				MD5Update(&context, dataBuf, realReadLen);
				memset(dataBuf, 0, sizeof(dataBuf));
				realReadLen = 0;
				readLen = sizeof(dataBuf);
			}
			MD5Final(retMD5, &context);

			{
				char md5str0x[DEF_MD5_SIZE*2+1] = {0};
				int i = 0;
				for(i = 0; i<DEF_MD5_SIZE; i++)
				{
					sprintf(&md5str0x[i*2], "%.2x", retMD5[i]);
				}
				strcpy(retMD5Str, md5str0x);
				bRet = TRUE;
			}
			fclose(fptr);
		}
	}
	return bRet;
}

BOOL GetMD5(char * buf, unsigned int bufSize, unsigned char retMD5[DEF_MD5_SIZE])
{
	MD5_CTX context;
	if(NULL == buf || NULL == retMD5) return FALSE;
	memset(&context, 0, sizeof(MD5_CTX));
	MD5Init(&context);
	MD5Update(&context, buf, bufSize);
	MD5Final(retMD5, &context);
	return TRUE;
}

