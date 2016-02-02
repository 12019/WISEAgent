/****************************************************************************/
/* Copyright(C) : Advantech Technologies, Inc.														 */
/* Create Date  : 2014/08/05 by Eric Liang															     */
/* Modified Date: 2014/08/05 by Eric Liang															 */
/* Abstract       :  Bluetooth Client              													             */
/* Reference    : None																									 */
/****************************************************************************/

#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include "platform.h"

#include "inc/AdvApiMux/AdvAPIMuxServer.h"
#include "inc/AdvApiMux/AdvAPIMuxBase.h"

#include "BasicFun_Tool.h"
#include "BasicSocketFunc.h"
#include "HashTable.h"

#include "inc/AdvApiMux/LoadAPIs.h"


//-----------------------------------------------------------------------------
// Internal Prototypes:
//-----------------------------------------------------------------------------
#define INI_PATH "/usr/lib/Advantech/iotgw"

static int RecvAllApiMuxData( int *skt, char *recvbuf, const int bufsize, int *stop );
static int ParseApiMuxProtocol(const char *buf, FnStruct *pFnStruct);

// APIMux Server Param
typedef struct _API_MUX_SERVER_PARAM
{
	int            Stop;
	int            ServerSkt;
	int            sktalivecount;
	MUTEX    MutexServer;
	ApiMuxConfig    Config;
	HashTable           *pAPIHashTable;
}API_MUX_SERVER_PARAM;


typedef struct _ns_connection {
  sock_t sock;					// Socket
   struct sockaddr sa;    // Peer address
  int            *pStop;
}ns_connection;


typedef struct _API_MUX_CLIENT_PARAM
{
	int            Stop;
	int            connected;
	int            KeepAliveSkt;
	int            sktalivecount;
	MUTEX    MutexClient;
	ApiMuxConfig    Config;
}API_MUX_CLIENT_PARAM;

//
////-----------------------------------------------------------------------------
//// Global Variables:
////-----------------------------------------------------------------------------

int					g_runing = 0;


// Server Variables
API_MUX_SERVER_PARAM		g_ApiMuxServer_Param;
TaskHandle_t protect_thread = NULL;
TaskHandle_t server_thread = NULL;


// Client Variables
API_MUX_CLIENT_PARAM		g_ApiMuxClient_Param;
TaskHandle_t client_thread = NULL;


static int SaveDataInHashTable( HashTable *pHashTable, const char *pHashkey, void *pData )
{
	int rc = -1;
	printf("%d %s \n",pHashTable,pHashkey);
	if( pHashTable && pHashkey ) {  
			PRINTF("Save %s Fn in Hash Table\n", pHashkey);
			HashTablePut( pHashTable, (char *)pHashkey, pData  );
			rc = 0;
	}
	return rc;
}

int LoadAPILib( const char *pszFullpathLib, HashTable *pHashTable )
{
	int i = 0;
	int rc = 0;
	API_MUX_FUNCTION *pApiMuxFn= NULL;
	void *pHandler = NULL;

	pHandler    = OpenLib( pszFullpathLib );

	if( pHandler )  {
		for( i = 0; i < MAX_FNS; i ++ ) {
			pApiMuxFn = AllocateMemory(sizeof(API_MUX_FUNCTION));
			if( pApiMuxFn ) {
				pApiMuxFn->pFn	=  ( void* ) GetLibFnAddress( pHandler, g_szFnName[i] );
				pApiMuxFn->Index = i;
				//PRINTF("[Api Mux Server] Successfule to load Interface =%s add=%d\n", g_szFnName[i], pApiMuxFn->pFn );
				SaveDataInHashTable(pHashTable, g_szFnName[i], (void*) pApiMuxFn );
			}
		}
		rc = 1;
	}

	return rc;
}

int LoadConfig(ApiMuxConfig *pApiMuxConfig)
{
	int rc = 0;
	char iniFile[1024]={0};
	
	//snprintf(iniFile, sizeof(iniFile),"%s/%s",GetWorkingDir( ), APIMUX_CONFIG_FILE);
	snprintf(iniFile, sizeof(iniFile),"%s/%s",INI_PATH, APIMUX_CONFIG_FILE);

	// Mode
	pApiMuxConfig->Mode = GetPrivateProfileInt( SERVER_INFO, SERVER_MODE_NAME, -1, iniFile );

	// ServerName
	GetPrivateProfileString(SERVER_INFO,SERVER_NAME,"",pApiMuxConfig->ServerName,sizeof(pApiMuxConfig->ServerName), iniFile );

	// Library Path
	GetPrivateProfileString(LIB_INFO,LIB_PATH,"",pApiMuxConfig->LibPath,sizeof(pApiMuxConfig->LibPath), iniFile );

	if( pApiMuxConfig->Mode == -1 ||  strlen( pApiMuxConfig->ServerName ) == 0 ) 
		rc = -1;

	return rc;
}

//Config
// 1. APIMux Mode: Socket / Local Socket
// 2. Domain Name:  Bind which interface or Any / Local Socket File

APIMUX_CODE SNCALL InitAdvAPIMux_Server()
{
	APIMUX_CODE rc = APIMUX_ER_FAILED;
	
	// LoadConifg 
	if( LoadConfig( &g_ApiMuxServer_Param.Config ) == -1 ) 
		return rc;

	// Aollacte
	g_ApiMuxServer_Param.pAPIHashTable = HashTableNew( MAX_FNS<<4 );

	if( g_ApiMuxServer_Param.pAPIHashTable == NULL )
		return rc;

	// Load DLL & Function Point
	if( LoadAPILib( g_ApiMuxServer_Param.Config.LibPath, g_ApiMuxServer_Param.pAPIHashTable ) == 0 ) 
		return rc;

	// Start Run
	StartAdvMuxServer(&g_ApiMuxServer_Param, NULL );

	rc = APIMUX_OK;

	return rc;
}


void SNCALL UnInitAdvAPIMux_Server()
{
	StopAdvMuxServer( &g_ApiMuxServer_Param );

	MUTEX_DESTROY(&g_ApiMuxServer_Param.MutexServer);
}

static int FillApiMuxHeader(ApiMuxHeader *pApiMuxHeader, const int size, const unsigned int ver, const unsigned int type, const unsigned int param, const unsigned int id, const unsigned int len )
{
	int rc = 0;
	if ( pApiMuxHeader && size == sizeof(ApiMuxHeader) ) {
		rc = 1;
		pApiMuxHeader->ver = ver;
		pApiMuxHeader->type =type;
		pApiMuxHeader->param = param;
		pApiMuxHeader->id = id;
		pApiMuxHeader->len = len;
	}
	return rc;
}

static int ProceFn5( char *buf, const int *bufsize, FnStruct *pfn )
{
	int len =0;
	int i = 0;
	char asPa[Fn5Ps][256]={0};
	Fn5P1 pP1;
	Fn5P2 pP2;
	ApiMuxHeader *pApiMuxHeader = ( ApiMuxHeader *) buf;
	Fn5 pFn5 =(Fn5)pfn->pFn;
	char *result = (char*) ( buf + API_MUX_HEADER_SIZE );

	for( i = 0; i< Fn5Ps ; i ++ ) {
		memset(asPa[i],0,sizeof(asPa[i]));
		memcpy(asPa[i], pfn->Paras[i], pfn->ParaLen[i]);
	}

	pP1 = (Fn5P1)asPa[0];
	pP2 = CACHE_MODE;//(Fn5P2)asPa[1];

	snprintf(result,bufsize -API_MUX_HEADER_SIZE ,"%s",pFn5(pP1,pP2));
	len = strlen(result);
	FillApiMuxHeader( pApiMuxHeader, sizeof(ApiMuxHeader), API_MUX_VER, 8, 1, pfn->callid, len );
	return len + API_MUX_HEADER_SIZE;
}



static int ProceFn6( char *buf, const int *bufsize, FnStruct *pfn )
{
	int len =0;
	int i = 0;
	int counter = 0;
	char asPa[Fn6Ps][256]={0};
	Fn6P1 pP1;
	Fn6P2 pP2;
	API_MUX_ASYNC_DATA *pAsyncData = NULL;
	ApiMuxHeader *pApiMuxHeader = NULL;
	Fn6 pFn6 =NULL; 
	char *result = (char*) ( buf + API_MUX_HEADER_SIZE );

	pApiMuxHeader = ( ApiMuxHeader *) buf;
	pFn6 = (Fn6)pfn->pFn;
	for( i = 0; i< Fn6Ps ; i ++ ) {
		memset(asPa[i],0,sizeof(asPa[i]));
		memcpy(asPa[i], pfn->Paras[i], pfn->ParaLen[i]);
	}

	pP1 = (Fn6P1)asPa[0];
	pP2 = (Fn6P2)asPa[1];

	pAsyncData = AllocateMemory(sizeof(API_MUX_ASYNC_DATA));

	if( pAsyncData ) {
		pAsyncData->callid = pfn->callid;
		snprintf(pAsyncData->magickey,sizeof(pAsyncData->magickey),"%s",ASYNC_MAGIC_KEY);
		pAsyncData->nDone = 0;
		pAsyncData->bufsize = MAX_SEND_BUF;
	}
	snprintf(result,bufsize-API_MUX_HEADER_SIZE,"%s",pFn6(pP1,pP2, pAsyncData ));

	if( strstr( result, "202" ) ) { // Accepted
		memset(result,0,bufsize-API_MUX_HEADER_SIZE);
		while( !pAsyncData->nDone ) {
			counter++;
			if( counter > ASYNC_TIMEOUT ) {
					PRINTF("Timeout=%d\n",counter);
				break;
			}
			TaskSleep(1000); // Sleep 1 sec
		}

		if( pAsyncData->nDone == 1 ) {
			// get from async => free 
			snprintf( result,bufsize-API_MUX_HEADER_SIZE,"%s",pAsyncData->buf );
			FreeMemory( pAsyncData );
		} else {
			// fill error not free			
			snprintf(result,bufsize-API_MUX_HEADER_SIZE,"%s",REPLY_FAIL);
			pAsyncData->nDone = -1; // ap need release it when recv callback event			
		}
	}
	len = strlen(result);
	FillApiMuxHeader( pApiMuxHeader, sizeof(ApiMuxHeader), API_MUX_VER, 8, 1, pfn->callid, len );
	return len + API_MUX_HEADER_SIZE;
}

static int ProceError( char *buf, const int *bufsize, FnStruct *pfn )
{
	int len = 0;
	snprintf(buf+API_MUX_HEADER_SIZE,bufsize,"%s","123");
	len = strlen(buf) + API_MUX_HEADER_SIZE;
	return len;
}

static int ProcRemoteCall( char *buf, const int *bufsize, FnStruct *pfn )
{
	int len = 0;
	switch( pfn->nIndex )
	{
	case 5:
		len = ProceFn5(buf, bufsize, pfn);
		break;
	case 6:
		len = ProceFn6(buf, bufsize, pfn);
		break;
	default:
		len = ProceError(buf, bufsize, pfn);
		break;
	}
	return len;
}

static void process_new_connection(void *pParam ) 
{
	char recvbuf[MAX_RECV_BUF] = {0};
	char sendbuf[MAX_SEND_BUF]={0};
	int len = 0;
	FnStruct  fn;
	ns_connection *pConn = ( ns_connection*) pParam;

	while( ! (*pConn->pStop) ) {
		// Recv All ApiMux Data
		memset(recvbuf, 0, MAX_RECV_BUF);
		len = RecvAllApiMuxData(&pConn->sock, recvbuf, MAX_RECV_BUF, pConn->pStop );
		PRINTF("Server Recv Data len=%d",len);
		if( len <= 0 ) 
			goto ExitClient;

		if( ParseApiMuxProtocol( recvbuf, &fn ) )  {
			// call api and return the result
			PRINTF("Fn index=%d num=%d\n",fn.nIndex, fn.num );
			memset(sendbuf, 0, MAX_SEND_BUF);
			len = ProcRemoteCall(sendbuf, sizeof(sendbuf), &fn );
			PRINTF("Server Send ske=%d stop=%d Result len = %d Result=%s\n",pConn->sock, *pConn->pStop, len, sendbuf+API_MUX_HEADER_SIZE);
			if( WL_SendAllData( &pConn->sock, sendbuf, len, pConn->pStop ) != WL_Send_OK )  {
				PRINTF("Send Failed\n");
				goto ExitClient;
			}
		}
	}

ExitClient:
	WL_CloseSktHandle(&pConn->sock);
	PRINTF("Exit\n");
	FreeMemory( pConn );
}

static void *adv_api_mux_server( void *arg)
{

	API_MUX_SERVER_PARAM *pPara = (API_MUX_SERVER_PARAM*) arg;
	char recvbuf[32]={0};
	char cmd[16]={0};
	char arg1[16]={0};
	ns_connection *pConnection = NULL;
	 struct sockaddr sa;
	socklen_t len = sizeof(sa);
	int i = 0;

	int client_skt = INVALID_SOCKET;

	if( pPara->Config.Mode == MODE_LocalSocket ) {	
		PRINTF("Local Socket = %s\n",pPara->Config.ServerName);
		if( LS_TCPListenServer(&pPara->ServerSkt, pPara->Config.ServerName ) != Server_Listen_OK ) 
		{
				PRINTF("Create failed Local Socket\n");
				goto exit_server;
		}
	} else {
		//
		PRINTF("Socket\n");
	}
	while( !pPara->Stop )
	{
			client_skt = Accept(&pPara->ServerSkt, (struct sockaddr*) &sa, &len );
			if( client_skt != INVALID_SOCKET )
			{
				pConnection =  AllocateMemory(sizeof(ns_connection));
				if( pConnection != NULL ) {
					pConnection->sock = client_skt;
					memcpy(&pConnection->sa, &sa, sizeof(sa));
					pConnection->pStop = &pPara->Stop;
					SUSIThreadCreate("AdvApiMux_Server", 8192, 61, process_new_connection, pConnection );
				}
			}
	}

exit_server:
	WL_CloseSktHandle(&pPara->ServerSkt);
	server_thread = NULL;
	g_runing = 0;
}

static void *protect_blocking(void *arg)
{
	API_MUX_SERVER_PARAM *pPara = (API_MUX_SERVER_PARAM*) arg;

	while( !pPara->Stop )
	{
		if( pPara->sktalivecount > -1 )
			pPara->sktalivecount++;

		if( pPara->sktalivecount > MAX_ALIVE_TIMEOUT ) {
			PRINTF("Blocking in Socket & Reset Connection\n");
		}
		TaskSleep(1000);
	}
	protect_thread = NULL;
}



int StartAdvMuxServer(API_MUX_SERVER_PARAM *pApiMuxServerParam, void *pParam1 )
{
	int rc = 0;

	pApiMuxServerParam->Stop = 0;
	// Create Thread
	if( !g_runing )
	{
		server_thread = SUSIThreadCreate("AdvApiMux_Server", 8192, 61, adv_api_mux_server, pApiMuxServerParam );
	}

	return rc;
}

int StopAdvMuxServer( void *arg )
{
	int rc = 0;
	API_MUX_SERVER_PARAM *pPara = (API_MUX_SERVER_PARAM*) arg;
	pPara->Stop = 1;

	WL_CloseSktHandle( &pPara->ServerSkt );

	while( g_runing )
		TaskSleep(100);

	return rc;
}


// ApiMuxProtocol

static int RecvAllApiMuxData( int *skt, char *recvbuf, const int bufsize, int *stop )
{
	int rc=0;
	ApiMuxHeader *pApiMuxHeader = NULL;
	int len = 0;

	rc = WL_NblkRecvFullData( skt, recvbuf, API_MUX_HEADER_SIZE, bufsize, stop );
	if( rc != WL_Recv_OK )  return -1;

	pApiMuxHeader = ( ApiMuxHeader *) recvbuf;

	rc = WL_NblkRecvFullData( skt, recvbuf+API_MUX_HEADER_SIZE, pApiMuxHeader->len, bufsize-API_MUX_HEADER_SIZE, stop );
	if( rc != WL_Recv_OK )  return -1;

	return pApiMuxHeader->len;
}





static int ParseApiMuxProtocol(const char *buf, FnStruct *pFnStruct)
{
	ApiMuxHeader *pApiMuxHeader = NULL;
	ApiFnHeader *pApiFnHeader = NULL;
	API_MUX_FUNCTION *pApiMuxFn = NULL;
	ApiParamHeader *pApiParamHeader = NULL;
	char pFnName[MAX_FN_NAME];
	unsigned short len = 0;
	int tsize = 0;
	int i = 0;
	int rc = 0;

	pApiMuxHeader = ( ApiMuxHeader *) buf;
	pApiFnHeader = ( ApiFnHeader*) (buf+API_MUX_HEADER_SIZE) ;

	//PRINTF("APIMUXHeader ver=%d type=%d param=%d id=%d len=%d\n",pApiMuxHeader->ver,pApiMuxHeader->type,pApiMuxHeader->param,pApiMuxHeader->id,pApiMuxHeader->len);
	
	memset(pFnName,0,sizeof(pFnName));

	len = pApiFnHeader->len;
	//PRINTF("len=%d len=%d ret=%d\n",pApiFnHeader->len, len, pApiFnHeader->ret);
	tsize = len;


	memcpy(pFnName, ( buf + API_MUX_HEADER_SIZE + API_MUX_FN_HEADER_SIZE) , len);
	
	pApiMuxFn = (API_MUX_FUNCTION*)HashTableGet( g_ApiMuxServer_Param.pAPIHashTable, (char *)pFnName );

	PRINTF("Name=%s find=%d\n", pFnName, pApiMuxFn );

	if( pApiMuxFn ) {
		pFnStruct->num       = pApiMuxHeader->param;
		pFnStruct->callid		 = pApiMuxHeader->id;
		pFnStruct->nIndex   = pApiMuxFn->Index;
		pFnStruct->pFn         = pApiMuxFn->pFn;
		
		for( i = 0 ; i < pFnStruct->num; i ++ ) {
			pApiParamHeader = ( ApiParamHeader * ) ( buf + API_MUX_HEADER_SIZE + API_MUX_FN_HEADER_SIZE + ( API_MUX_PARAM_HEADER_SIZE * i) + tsize );
			len = pApiParamHeader->len;
			//PRINTF("Param len=%d type=%d\n",pApiParamHeader->len, pApiParamHeader->type );
			pFnStruct->Paras[i] = ( void *) buf + API_MUX_HEADER_SIZE + API_MUX_FN_HEADER_SIZE + ( API_MUX_PARAM_HEADER_SIZE * (i+1)) + tsize;
			pFnStruct->ParaLen[i] = len;		
			tsize += len;
			//PRINTF("param=%s len=%d\n",pFnStruct->Paras[i], pFnStruct->ParaLen[i] );
		}
		rc = 1;
	}

	return rc;
}


// Client Function

static void *adv_api_mux_client( void *arg)
{
	API_MUX_CLIENT_PARAM *pPara = (API_MUX_CLIENT_PARAM*) arg;
	char recvbuf[32]={0};
	 struct sockaddr sa;
	 int rc = 0;
	socklen_t len = sizeof(sa);
	int i = 0;
	
	while( !pPara->Stop )
	{
			if( pPara->connected == 0 ) {
				if( LS_NblkTCPConnect( &pPara->KeepAliveSkt, pPara->Config.ServerName, 20, &pPara->Stop ) == WL_Connect_OK )
					pPara->connected = 1;
			}
			
			rc = WL_NblkRecvFullData( &pPara->KeepAliveSkt, recvbuf, API_MUX_HEADER_SIZE, sizeof(recvbuf), &pPara->Stop );
			if( rc != WL_Recv_OK )  
				pPara->connected = 0;

			TaskSleep(100);
	}

	pPara->connected = 1;
exit_client:
	WL_CloseSktHandle(&pPara->KeepAliveSkt);
	client_thread = NULL;
	pPara->connected = 0; 
	g_runing = 0;
}

int StartAdvMuxClient(API_MUX_CLIENT_PARAM *pApiMuxClientParam, void *pParam1 )
{
	int rc = 0;

	// Create Thread
	if( !g_runing )
	{
		pApiMuxClientParam->Stop = 0;
		pApiMuxClientParam->connected = 0;
		client_thread = SUSIThreadCreate("ApiMuxClientThread", 8192, 61, adv_api_mux_client, pApiMuxClientParam );
	}

	return rc;
}

int StopApiMuxClient( void *arg )
{
	int rc = 0;
	API_MUX_CLIENT_PARAM *pPara = (API_MUX_CLIENT_PARAM*) arg;
	pPara->Stop = 1;

	WL_CloseSktHandle( &pPara->KeepAliveSkt );

	while( g_runing )
		TaskSleep(100);

	return rc;
}

APIMUX_CODE SNCALL InitAdvAPIMux_Client()
{
	APIMUX_CODE rc = APIMUX_ER_FAILED;

	memset(&g_ApiMuxClient_Param, 0, sizeof(g_ApiMuxClient_Param));
	// LoadConifg 
	if( LoadConfig( &g_ApiMuxClient_Param.Config ) == -1 ) 
		return rc;


	//StartAdvMuxClient( &g_ApiMuxClient_Param, NULL );

	rc = APIMUX_OK;
	return rc;
}

void SNCALL UnInitAdvAPIMux_Client()
{
	StopApiMuxClient( &g_ApiMuxClient_Param );
}

static int PackageSendRequest( char *buf, const int bufsize, CallApiMuxParams *pApiMuxParams )
{
	int rc = 0;
	ApiMuxHeader *pApiMuxHeader;
	ApiFnHeader *pApiFnHeader = NULL;
	API_MUX_FUNCTION *pApiMuxFn = NULL;
	ApiParamHeader *pApiParamHeader = NULL;
	char *pFnName = NULL;
	int len = 0;
	int cPosit = 0;
	int i = 0;
	char *pData = NULL;

	if( pApiMuxParams == NULL ) 
		return rc;

	for( i = 0; i<pApiMuxParams->num; i ++) {
		len += pApiMuxParams->ParamLen[i];
	}
	
	// Not include ApiMuxHeader (8 Bytes)
	len += API_MUX_FN_HEADER_SIZE + ( API_MUX_PARAM_HEADER_SIZE * pApiMuxParams->num ) + strlen( pApiMuxParams->FnName ); 

	if( len > bufsize ) return rc;

	// APIMuxHeader
	pApiMuxHeader = (ApiMuxHeader *) ( buf +cPosit );
	pApiMuxHeader->ver	  = API_MUX_VER;
	pApiMuxHeader->type = 0;
	pApiMuxHeader->param = pApiMuxParams->num;
	pApiMuxHeader->id = 5768;
	pApiMuxHeader->len = len;

	cPosit += API_MUX_HEADER_SIZE;
	// ApiFnHeader ( 3 Bytes )
	pApiFnHeader = (ApiFnHeader*) ( buf + cPosit );
	pApiFnHeader->len = strlen( pApiMuxParams->FnName );
	pApiFnHeader->ret = 1;
	pData = ( char *) ( buf + cPosit + API_MUX_FN_HEADER_SIZE );

	memcpy( pData, pApiMuxParams->FnName, strlen( pApiMuxParams->FnName ) );

	cPosit += API_MUX_FN_HEADER_SIZE + strlen( pApiMuxParams->FnName );

	for( i=0; i< pApiMuxParams->num; i++ ) {
		pApiParamHeader = (ApiParamHeader*)  ( buf +cPosit );
		pApiParamHeader->len = pApiMuxParams->ParamLen[i];
		pApiParamHeader->type = 1;
		PRINTF("Param%d len=%d type=%d value=%s\n",i, pApiParamHeader->len,pApiParamHeader->type, pApiMuxParams->pParams[i] );
		pData = ( char *) ( buf + cPosit + API_MUX_PARAM_HEADER_SIZE );
		memcpy( pData, pApiMuxParams->pParams[i], pApiMuxParams->ParamLen[i] );
		cPosit += API_MUX_PARAM_HEADER_SIZE + pApiMuxParams->ParamLen[i];
	}
	//FnStruct fn;
	//ParseApiMuxProtocol(buf, &fn);
	return len+API_MUX_HEADER_SIZE;
}

int SNCALL SendApiMuxRPCOnce( int *skt, char *sendbuf, const int bufsize, char *recvbuf, const int recvbufsize, CallApiMuxParams *pParams, int timeout, int *Stop )
{
	int rc = 0;
	int len = 0;

	// 1. prepare send packeg
	if( ( len = PackageSendRequest( sendbuf, bufsize, pParams ) ) == 0 ) 
		return rc;

	PRINTF("Fn=%s num=%d param1=%s p1l=%d\n",pParams->FnName, pParams->num, (char*)pParams->pParams[0],pParams->ParamLen[0]);

	// 2. Connect
	if( LS_NblkTCPConnect( skt, g_ApiMuxClient_Param.Config.ServerName, timeout, Stop ) != WL_Connect_OK )
		return rc;

	// 2. Send Remote Process Call
	//int   WL_SendAllData(int *skt, char* sendcmd, int datasize, int *stop);
	if( WL_SendAllData( skt, sendbuf, len, Stop ) != WL_Send_OK ) 
		return rc;

	// 3. Result
	PRINTF("SendApiMuxRPCOnce Leave len=%d\n",len);
	len = RecvAllApiMuxData(skt, recvbuf, recvbufsize, Stop );
	PRINTF("Client Recv Data len=%d data\n",len);
	if( len > 0 )
		rc = 1;

	return rc;
}








