#ifndef _SSH_CLIENT_H_
#define _SSH_CLIENT_H_

//-------------------------------data type define------------------------
typedef struct SesData{
	char * key;
	char * data;
	unsigned int width;
	unsigned int height;
}SesData, *PSesData;

typedef struct SesDataQNode{
	PSesData pSesData;
	struct SesDataQNode *  next;
}SesDataQNode, *PSesDataQNode;

typedef struct SesDataQueue{
	PSesDataQNode head;
	PSesDataQNode trail;
	int nodeCnt;
}SesDataQueue;
//-----------------------------------------------------------------------

typedef int (*DataSendHandle)(char * sessionKey, char * buf);

#ifdef __cplusplus
extern "C" {
#endif

	int SSHClientStart();
   int RegisterDataSendHandle(DataSendHandle sendHandle);
	int RecvData(char * sessionKey, char * buf, unsigned int width, unsigned int height);
	int CloseSession(char * sessionKey);
	int SSHClientStop();

#ifdef __cplusplus
}
#endif

#endif
