#ifndef _BASICSOCKETFUNC_H
#define _BASICSOCKETFUNC_H


#if defined(__linux) //linux
	#include <stdlib.h>
	#include <stdio.h>
	#include <string.h>
	#include <sys/socket.h>
	#include <sys/ioctl.h> 
	#include <netdb.h>
	#include <netinet/in.h>
	#include <arpa/inet.h>
	#include <pthread.h>
	#include <semaphore.h>
	#include <unistd.h>
	#include <ctype.h>
	#include <errno.h>
	#include <sys/ioctl.h>
	#include <net/if.h>
	#include <arpa/inet.h>
	#include <sys/un.h>
	#define LINUX
#else
	#include <AFXSOCK.H>
	#define _WINDOWS

#endif // linux

typedef int							BOOL;      // int


#define TRUE                    (1)
#define FALSE					(0)

#define LOCAL_SOCKET_PORT 9000
#define  WL_MAX_RECVSIZE   20000
#define  WL_RECV_TIMEOUT   30  // sec


#define INVALID_SOCKET -1
#define SOCKET_ERROR -1
#define SOCKADDR_IN struct sockaddr_in
#define SOCKADDR struct sockaddr

//Error or Success Number
typedef enum _tagERRORNO
{
	//error
	WL_Buffer_Full	= -100,
	WL_Connect_Err	= -101,
	WL_Send_Err		= -102,
	WL_Recv_Err		= -103,
	WL_AUTH_Err		= -104,
	WL_Stop			= -105,
	WL_Recv_Timeout = -106,
	//success
	WL_Connect_OK	=  101,
	WL_Send_OK		=  102,
	WL_Recv_OK		=  103,

	// Server error
	Server_Skt_Err     =  -200,
	Server_Bind_Err   = -201,
	Server_Listen_Err = -202,
	Server_Accept_Err = -203,

	// Server Success
	Server_Listen_OK     = 200,
	Server_Accept_OK = 201,
	
} ERRORNO;

typedef struct _WL_URL_COMMAND
{
	char ipaddress[256];
	int  port;
	char HttpMethod[128];	//GET/POST
	char Command[1024];		// /admin/login.cgi
	char HttpData[1024];	// POST
	char AuthBasic[256];	//Base64
	char Cookie[64];		//Cookie: %s\r\n
} WL_Url_Command;


// TCP Socket Function
// 建立nonblocking TCP Socket
int   WL_NblkTCPConnect(int *skt, char *ip, unsigned  port, int wait_time, int *stop);
// 建立 nonblocking TCP 連線 (可以設定 time-out 時間
int   WL_NblkTCPTimeoutConnect(int *skt, char *ip, unsigned port, int *stop,int timeout/*sec*/);
//建立連線並回傳本機端連線的IP
int WL_NblkTCPConnect_GetLocalIP(int *skt, char *ip, unsigned port, int wait_time,char *localip , int *stop);
// UDP Connect, 嚙踝蕭嚙瞌嚙踝蕭WLNblkTCPConnect嚙踝蕭嚙踝蕭嚙褒入嚙諍立好嚙踝蕭Socket
int WL_NblkUDPConnect(int *skt, char *ip, unsigned port, int *stop);
// 先Bind一個port，然後才在去做TCP Connection動作
int   WL_BindPort_TCPConnect(int *skt, char *ip, SOCKADDR_IN *Cli_Addr, unsigned int Cli_Port, unsigned int Ser_Port, int *stop);
// 關閉Socket
void  WL_CloseSktHandle(int *skt);
// 以nonblocking方式收http header，一次收一Byte資料
int   WL_NblkRecvHeaderByte(int *skt, char *result, int *size, int bufsize, int *stop);
int   WL_NblkRecvHeaderByteTimeout(int *skt, char *result, int *size, int bufsize, int *stop, int iTime);
// 以nonblocking方式收http header，一次收一個定值的資料
int   WL_NblkRecvHeaderValueTimeout(int *skt, char *result, int recvsize, int bufsize, char** Headerptr, int *stop, int time);
int   WL_NblkRecvHeaderValue(int *skt, char *result, int recvsize, int bufsize, char** Headerptr, int *stop);
// 以nonblocking方式收HTML File
int   WL_NblkRecvHTMLFile(int *skt, char *result, int *size, int bufsize, int *stop); 
// 可傳入 time-out 值，使用者決定多久沒收到資料即 time-out 離開
int   WL_NblkRecvHTMLFileTimeout(int *skt, char *result, int *size, int bufsize, int *stop,int TimeOut/*sec*/);

// 以nonblocking方式收一定值的資料
int   WL_NblkRecvFullDataTimeout(int *skt, char* result, int datasize, int bufsize, int *stop, int time);
int   WL_NblkRecvFullData(int *skt, char* result, int datasize, int bufsize, int *stop);
// 只下要收的資料，但不保證一定可以收滿
int   WL_NblkRecvData(int *skt,char *result,int datasize, int* recvsize,int *stop);
// 接收到所想要的string出現，即停止。
int   WL_NblkRecvUntilStr(int *skt, char *result, int *size, int bufsize, char* StopStr, int *stop);
// 以nonblocking方式傳送command
int   WL_SendAllData(int *skt, char* sendcmd, int datasize, int *stop);

// UDP Socket Function
// 建立 UDP Socket
int   WL_CreatUDPSkt(int *skt);
// 建立Server UDP Socket
int   WL_CreateSerUDPSkt(int *skt, int port);
//建立server 端 特定IP的UDP Skt
int WL_CreateUDPSktForSpeificIP(int *skt, int port , char* pszIP);
// 建立client UDP Socket
int   WL_CreateCliUDPSkt(int *skt, char* ser_ip, int ser_port, SOCKADDR_IN *ser_addr);
// 取得UDP Port是否可以使用(startport: 1025~65535)
int   WL_GetUDPPort(int startport, int index, int *port);
// 嚙瘡嚙踝蕭嚙踝蕭嚙緻嚙瑾嚙踝蕭嚙緩嚙範嚙踩內可嚙瘡嚙誕用迎蕭嚙編嚙踝蕭port,嚙瞎嚙踝蕭嚙範嚙踩不荔蕭嚙磕嚙盤10000
int   WLGetRandomRangePort(int iStartPort, int iEndPort, int iRange, int *piFindPort, BOOL bIsUDP);
// 以nonblocking方式傳送UDP的資料
int   WL_SendUDPData(int *skt, SOCKADDR_IN *ser_addr, char *sendcmd, int datasize, int *stop);
// 以nonblocking方式收UDP的資料
int   WL_NblkRecvUDPData(int *skt, char *result, int size, SOCKADDR_IN *cli_addr, int wait_time, int *stop);
// blocking socket with RecvTimeout timeout UDP
int   WL_NblkRecvUDPDataTimeout(int *skt, char *result, int size, SOCKADDR_IN *cli_addr, int *stop);
// Other Function
// 一次收1460 Byte資料，然後再一Byte一Byte傳回
//int   WL_RecvOneByte(int *skt, char *RecvBuff, int *RemainLen, int *RecvLen, char *RtnBuff, int *stop);
int WL_RecvOneByte(int *skt, char *RecvBuff, int BuffLen, int *RecvLen, int *stop);
// 依據傳入要search的起始字串與結束字串，切好一塊區域回傳。
int   WL_GetSearchRegion(int *skt, char **beginptr, char *recvbuf, int bufsize, int *stop, int *framesize, int *remainsize, int* tmpbufsize
		,char* StrBeg, int StrBegLen, char* StrEnd, int StrEndLen, int SearchType);
// 收MJPEG Stream資料，並切一張MJPEG傳回
int   WL_GetOneMJPEGFrame(int *skt, int *beginptr, char *recvbuf, int bufsize, int *stop, int *framesize, int *remainsize, int* tmpbufsize);
// 收MPEG4 Stream資料，並切回一張Frame傳回
int   WL_GetOneMPEG4Frame(int *skt, char **beginptr, char *recvbuf, int bufsize, int *stop, int *framesize, int *remainsize, int* tmpbufsize);
// 在某Buffer內搜尋一Token，並回傳此Token開始的位址
char* WL_FindFlageHeader(char* pSrc,int size,char *Token,int TonkeLen);
// 處理一次socket()->connect->send()->recv()的動作，用於Send一次URL Command
int	  WL_SendHttpCommand( int *sockfd, WL_Url_Command *url, char *netbuf, int bufsize, int *stop );


// Local Socket Server API
int LS_TCPListenServer(int *sockfd, const char *LocalSockPath /*INADDR_ANY */);
int Accept( int *skt, struct sockaddr *addr,socklen_t *addrlen);

// Local Socket Client API
int   LS_NblkTCPConnect(int *skt, const char *localSockPath, int wait_time, int *stop);
#endif
