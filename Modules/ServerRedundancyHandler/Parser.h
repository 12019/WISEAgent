#ifndef _SERVERREDUNDANCY_PARSER_H_
#define _SERVERREDUNDANCY_PARSER_H_

#include "platform.h"
#include "cJSON.h"
#include "ServerRedundancyHandler.h"


#define AGENTINFO_BODY_STRUCT			"susiCommData"
#define AGENTINFO_REQID					"requestID"
#define AGENTINFO_CMDTYPE				"commCmd"

#define SERVERREDUNDANCY_ERROR_REP           "errorRep"

#define SERVERREDUNDANCY_RESPONSE				 "response"
#define SERVERREDUNDANCY_STATUSCODE			 "statuscode"
#define SERVERREDUNDANCY_RESPONSEMESSAGE		 "msg"
#define SERVERREDUNDANCY_SERVER_NODE			 "server"
#define SERVERREDUNDANCY_SERVER_ADDRESS		 "address"
#define SERVERREDUNDANCY_SERVER_PORT			 "port"
#define SERVERREDUNDANCY_SERVER_AUTH			 "auth"
#define SERVERREDUNDANCY_SERVER_IP_LIST		 "serverIPList"
//#define SERVERREDUNDANCY_N_FLAG				 "n"
#define SERVERREDUNDANCY_TLSTYPE_FLAG		 "TLSType"
#define SERVERREDUNDANCY_PSK_FLAG			 "PSK"

typedef struct SERVER_INFO
{	
	char sServerIP[DEF_MAX_STRING_LENGTH];
	int iServerPort;
	char sServerAuth[DEF_USER_PASS_LENGTH];
	tls_type eTLSType;
	char sPSK[DEF_USER_PASS_LENGTH];
	bool bMaster;
	struct SERVER_INFO *next;
};

typedef struct SERVER_LIST
{	
	int serverCount;
	struct SERVER_INFO *server;
};

typedef struct RESPONSE_MESSAGE
{
	int iStatusCode;
	char* strMsg;
	struct SERVER_INFO *pServer;
	struct SERVER_LIST* pServerList;
}RESPONSE_MESSAGE, Response_Msg;

bool ParseReceivedData(void* data, int datalen, int * cmdID);
void ParseServerList(cJSON * pServerIPList, struct SERVER_LIST* pServerList);
void FreeServerList(struct SERVER_INFO* pServerList);
int ParseServerCtrl(void* data, int datalen, RESPONSE_MESSAGE *pMessage, char *workDir);

#endif