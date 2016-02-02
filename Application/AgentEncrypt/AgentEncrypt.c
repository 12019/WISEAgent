// AgentEncrypt.cpp : Defines the entry point for the console application.
//

#include "platform.h"
#include "DES.h"
#include "Base64.h"


#define DEF_DES_KEY                              "29B4B9C5"
#define DEF_DES_IV                               "42b19631"

static bool DES_BASE64Encode(char * srcBuf,char **destBuf)
{
   bool bRet = false;
   unsigned char ciphertext[128] = {0};
   int cipherLen = 0;
   int iRet = 0;
   if(srcBuf == NULL) return bRet;
   iRet = DESEncodeEx(DEF_DES_KEY, DEF_DES_IV, srcBuf, ciphertext, &cipherLen);
   if(iRet == 0)
   {
      char *base64Enc = NULL;
	  int encLen = 0;
      int len = strlen(ciphertext);
      iRet= Base64Encode(ciphertext, len, &base64Enc, &encLen);
      if(iRet == 0)
      {
         len = strlen(base64Enc);
         if(len > 0)
         {
			 *destBuf = (char *)malloc(len + 1);
			 memset(*destBuf, 0, len + 1);
			 strcpy(*destBuf, base64Enc);
			 bRet = true;
		 }
	  }
      if(base64Enc) free(base64Enc);
   }
   return bRet;
}

static bool DES_BASE64Decode(char * srcBuf,char **destBuf)
{
   bool bRet = false;
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
			iRet = DESDecodeEx(DEF_DES_KEY, DEF_DES_IV,  base64Dec, decLen, plaintext);
         if(iRet == 0)
         {
            *destBuf = (char *)malloc(len + 1);
			 memset(*destBuf, 0, len + 1);
			 strcpy(*destBuf, plaintext);
			 bRet = true;
         }
      }
      if(base64Dec) free(base64Dec);
   }

   return bRet;
}

int main(int argc, char *argv[])
{
	char* targetStr = NULL;
	if(argc !=2) 
	{
		return -1;
	}
#ifdef ENC_ONLY
	if(DES_BASE64Encode(argv[1], &targetStr))
		printf("%s", targetStr);
#else

	if(DES_BASE64Decode(argv[1], &targetStr))
		printf("%s", targetStr);
#endif
	if(targetStr)
		free(targetStr);
	return 0;
}

