// AgentEncrypt.cpp : Defines the entry point for the console application.
//

#include "platform.h"
#include "md5.h"

#define DEF_MD5_SIZE                16
#define DEF_PER_MD5_DATA_SIZE       512

bool GetFileMD5(char * filePath, char * retMD5Str)
{
	bool bRet = false;
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
				bRet = true;
			}
			fclose(fptr);
		}
	}
	return bRet;
}

int main(int argc, char *argv[])
{
	char md5Str[64] = {0};
	if(argc !=2) 
	{
		return -1;
	}
	if(GetFileMD5(argv[1], md5Str))
		printf("%s", md5Str);
	return 0;
}

