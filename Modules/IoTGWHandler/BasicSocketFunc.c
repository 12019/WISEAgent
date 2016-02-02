#include "BasicSocketFunc.h"

static unsigned long WL_GetAddrFromIP(char *ip)
{
#ifdef _WINDOWS
	struct hostent * hp;
	unsigned int addr=0;
	if (ip) 
	{
		hp = gethostbyname(ip);
		if (hp != NULL)
			return ((LPIN_ADDR)hp->h_addr)->s_addr; 
		else
		{
			addr = inet_addr(ip);
			if(addr == INADDR_NONE)
				return htonl(INADDR_ANY);
			else
				return addr;
		}
	}
	return htonl(INADDR_ANY);
#else
#ifdef LINUX
	struct		hostent	hostbuf, *hp;
	size_t		hstbuflen;
	char		*tmphstbuf;
	char		*tmphstbuf2;
	int			res;
	int			herr;
	in_addr_t	ipaddress;
	
	ipaddress = inet_addr(ip);
	if (ipaddress == INADDR_NONE)
	{
	   hstbuflen = 1024;
	   tmphstbuf = malloc (hstbuflen);
	   while ( (res = gethostbyname_r( ip, &hostbuf, tmphstbuf, hstbuflen, &hp, &herr)) == ERANGE )
	   {
	      if (hstbuflen >= 512*1024) break;
	      hstbuflen *= 2;
	      tmphstbuf2 = realloc (tmphstbuf, hstbuflen);
	      if (tmphstbuf2 != NULL) {
	         tmphstbuf = tmphstbuf2;
	      } else {
	         free(tmphstbuf);
	         tmphstbuf = NULL;
	         break;
	      }
	    }
	    //check error
	    if ( res || hp == NULL )
	    {
	      ipaddress = 0;
	    }
	    else
	    {
	      memcpy(&ipaddress, hostbuf.h_addr, sizeof(hostbuf.h_length));
	    }
	    if( tmphstbuf != NULL )
	    {
	      free(tmphstbuf);
	      tmphstbuf = NULL;
	    }
	}
	return ipaddress;
#endif//LINUX
#endif//_WINDOWS
}

/*
 * =============================================================================
 *  Function name: WLNblkTCPConnect
 *  : nonblocking�覡���@TCP Connect
 *
 *  Parameter:
 *      (INPUT)
 *       int*           - skt       : TCP's socket handle
 *       char*          - ip        : Server ip address
 *       unsigned       - port      : Server port
 *       int            - wait_time : ���ݳs�u���ɶ��A�H������
 *       int*           - stop      : �O�_�n���flage
 *
 *  Return value:
 *       int            - WL_Connect_OK   : �s�u���\
 *                      - WL_Connect_Err  : �����s�u���~
 *                      - WL_Stop         : �����s�u
 *
 *  History:
 *       Kidd Hsu      10/29/2008          Modified
 * =============================================================================
 */
int WL_NblkTCPConnect(int *skt, char *ip, unsigned port, int wait_time, int *stop)
{
	if (*skt != INVALID_SOCKET)
		WL_CloseSktHandle(skt);

#ifdef _WINDOWS
	int i;
	struct sockaddr_in server_addr;
	
	if (ip==NULL) 
		return WL_Connect_Err;
	else if (*stop)
		return WL_Stop;
	*skt=socket(AF_INET,SOCK_STREAM,0);
	if (*skt!=INVALID_SOCKET) {
		server_addr.sin_family=AF_INET;
		server_addr.sin_port=htons((unsigned short)port);
		server_addr.sin_addr.s_addr=WL_GetAddrFromIP(ip);
		i=connect(*skt,(sockaddr *)&server_addr,sizeof(sockaddr_in));
		if (i==0) 
			return WL_Connect_OK; // connect OK
		else 
		{
			TRACE("Error:%d\r\n",GetLastError());
			WL_CloseSktHandle(skt); // connect fail, release it
		}
	}
	return WL_Connect_Err;
#else
#ifdef LINUX
	struct sockaddr_in   server_addr;
	int 	flag = 1;
	int		ipaddress;
	int		retrytimes;
	struct	timeval TimeOut;
	TimeOut.tv_sec  = 20;
	TimeOut.tv_usec = 0;


	if( wait_time == 0 ) {
		retrytimes = WL_RECV_TIMEOUT * 10;	//�]���O�H100ms������
	} else {
		retrytimes = wait_time * 10;
	}

	if( ip == NULL ) {
		return WL_Connect_Err;
	} else if( *stop ) {
		return WL_Stop;
	}

	//domain name�ѪR
	ipaddress = WL_GetAddrFromIP(ip);
	if( ipaddress == 0 ) {
		return WL_Connect_Err;
	}
	
	*skt = socket(AF_INET,SOCK_STREAM,0);
	if( *skt != INVALID_SOCKET )
	{
		server_addr.sin_family      = AF_INET;	
		server_addr.sin_addr.s_addr = ipaddress;
		server_addr.sin_port        = htons(port);		
	
		flag = 1;
		ioctl(*skt, FIONBIO, &flag);	//�]�wnon-block mode
		do
		{
			if( *stop ) {
				WL_CloseSktHandle(skt);
				return WL_Stop;
			}
			if( connect(*skt, (struct sockaddr *)&server_addr, sizeof(server_addr)) >= 0 )
			{
				flag = 0;
				ioctl(*skt, FIONBIO, &flag);	//���^block mode
				
				if( setsockopt( *skt, SOL_SOCKET, SO_RCVTIMEO, (char*)&TimeOut, sizeof(TimeOut) ) < 0)
				{
					//PRINTF("\n<**IPCAM**> %s setsockopt failed\n\n", __FUNCTION__);
					WL_CloseSktHandle(skt);
					return WL_Connect_Err;
				}

				return WL_Connect_OK;
			}
			usleep( 100 * 1000 );
		} while ( retrytimes-- > 1 );
	}

	WL_CloseSktHandle(skt);
	return WL_Connect_Err;

#endif//LINUX
#endif//_WINDOWS
}


/*
 * =============================================================================
 *  Function name: WL_NblkTCPTimeoutConnect
 *  : nonblocking TCP �s�u �i�]�w time-out �ɶ�
 *
 *  Parameter:
 *      (INPUT)
 *       int*           - skt       : TCP's socket handle
 *       char*          - ip        : Server ip address
 *       unsigned       - port      : Server port
 *       int*           - stop      : �O�_�n���flage
 *       int            - time_out  : �s�u time-out �ɶ�
 *
 *  Return value:
 *       int            - WL_Connect_OK   : �s�u���\
 *                      - WL_Connect_Err  : �����s�u���~
 *                      - WL_Stop         : �����s�u
 *
 *  History:
 *       Eric Liang      11/20/2008          Modified
 * =============================================================================
 */
int   WL_NblkTCPTimeoutConnect(int *skt, char *ip, unsigned port, int *stop,int time_out /* 50 sec*/)
{
	if (*skt != INVALID_SOCKET)
		WL_CloseSktHandle(skt);
	
#ifdef _WINDOWS
	int i;
	struct sockaddr_in server_addr;
	
	if (ip==NULL) return WL_Connect_Err;
	
	if (*stop) return WL_Stop;

	if(time_out> 50 ) 
		time_out = 50;
	else if(time_out<1) 
		time_out = 1;

	*skt=socket(AF_INET,SOCK_STREAM,0);

	unsigned long ulargp = 1;

	int rc = ioctlsocket(*skt,FIONBIO,&ulargp);

	if (*skt!=INVALID_SOCKET) 
	{
		server_addr.sin_family=AF_INET;
		server_addr.sin_port=htons((unsigned short)port);
		server_addr.sin_addr.s_addr=WL_GetAddrFromIP(ip);
		i=connect(*skt,(sockaddr *)&server_addr,sizeof(sockaddr_in));
		if (i==SOCKET_ERROR)
		{
		   if(GetLastError() == WSAEWOULDBLOCK)
		   {
			  struct fd_set    fdwrite;
			  struct timeval   timeout;
		   
			  FD_ZERO(&fdwrite);
 			  FD_SET(*skt, &fdwrite);

			  timeout.tv_sec = time_out;
			  timeout.tv_usec = 0;
			  rc = select(0, NULL, &fdwrite, NULL, &timeout);
			  if( rc > 0)
			  {
				ulargp = 0;
				rc = ioctlsocket(*skt,FIONBIO,&ulargp); // �]�w�� blocking
				return WL_Connect_OK;
			  }
		   }
		}
		else
		{
			ulargp = 0;
			rc = ioctlsocket(*skt,FIONBIO,&ulargp); // �]�w�� blocking
			return WL_Connect_OK;				
		}
		WL_CloseSktHandle(skt); 
	}

	return WL_Connect_Err;
#else
#ifdef LINUX

	if(time_out> 50 ) 
		time_out = 50;
	else if(time_out<1) 
		time_out = 1;

	struct sockaddr_in   server_addr;
	int 	flag = 1;
	int		ipaddress;
	int		retrytimes = time_out*10;	//try 50��

	if( ip == NULL ) {
		return WL_Connect_Err;
	} else if( *stop ) {
		return WL_Stop;
	}

	//domain name�ѪR
	ipaddress = WL_GetAddrFromIP(ip);
	if( ipaddress == 0 ) {
		return WL_Connect_Err;
	}
	
	*skt = socket(AF_INET,SOCK_STREAM,0);
	if( *skt != INVALID_SOCKET )
	{
		server_addr.sin_family      = AF_INET;	
		server_addr.sin_addr.s_addr = ipaddress;
		server_addr.sin_port        = htons(port);		
	
		flag = 1;
		ioctl(*skt, FIONBIO, &flag);	//�]�wnon-block mode
		do
		{
			if( *stop ) {
				return WL_Stop;
			}
			if( connect(*skt, (struct sockaddr *)&server_addr, sizeof(server_addr)) >= 0 )
			{
				flag = 0;
				ioctl(*skt, FIONBIO, &flag);	//���^block mode
				return WL_Connect_OK;
			}
			usleep( 100 * 1000 );
		} while ( retrytimes-- > 1 );
	}
	WL_CloseSktHandle(skt);
	return WL_Connect_Err;
#endif//LINUX
#endif//_WINDOWS
	
}
/*
 * =============================================================================
 *  Function name: WLNblkTCPConnect_GetLocalIP
 *  : nonblocking覡@TCP Connect
 *
 *  Parameter:
 *      (INPUT)
 *       int*           - skt       : TCP's socket handle
 *       char*          - ip        : Server ip address
 *       unsigned       - port      : Server port
 *       int            - wait_time : ݳsuɶAH��? *		 char*          - localip   : Client ip address buffer
 *       int*           - stop      : O_nflage
 *
 *  Return value:
 *       int            - WL_Connect_OK   : su\
 *                      - WL_Connect_Err  : su~
 *                      - WL_Stop         : su
 *
 *  History:
 *        Janet Yao  09/08/2010         Modified
 * =============================================================================
 */
int WL_NblkTCPConnect_GetLocalIP(int *skt, char *ip, unsigned port, int wait_time,char *localip , int *stop)
{
	if (*skt != INVALID_SOCKET)
		WL_CloseSktHandle(skt);

#ifdef _WINDOWS
	int i,len;	
	struct sockaddr_in server_addr ,cli_addr;
	len = sizeof(cli_addr);
	if (ip==NULL) 
		return WL_Connect_Err;
	else if (*stop)
		return WL_Stop;
	*skt=socket(AF_INET,SOCK_STREAM,0);
	if (*skt!=INVALID_SOCKET) {
		server_addr.sin_family=AF_INET;
		server_addr.sin_port=htons((unsigned short)port);
		server_addr.sin_addr.s_addr=WL_GetAddrFromIP(ip);
		i=connect(*skt,(sockaddr *)&server_addr,sizeof(sockaddr_in));
		if (i==0) 
		{
			getsockname(*skt , (sockaddr*)&cli_addr , &len);
			sprintf(localip,"%d.%d.%d.%d",cli_addr.sin_addr.S_un.S_un_b.s_b1,cli_addr.sin_addr.S_un.S_un_b.s_b2,cli_addr.sin_addr.S_un.S_un_b.s_b3,cli_addr.sin_addr.S_un.S_un_b.s_b4);
			
			return WL_Connect_OK; // connect OK
		}
		else 
		{
			TRACE("Error:%d\r\n",GetLastError());
			WL_CloseSktHandle(skt); // connect fail, release it
		}
	}
	return WL_Connect_Err;
#else
#ifdef LINUX
	struct sockaddr_in   server_addr ,cli_addr;
	socklen_t len ;
	int 	flag = 1;
	int		ipaddress;
	int		retrytimes;

	len = sizeof(cli_addr);

	if( wait_time == 0 ) {
		retrytimes = WL_RECV_TIMEOUT * 10;	//]OH100ms
	} else {
		retrytimes = wait_time * 10;
	}

	if( ip == NULL ) {
		return WL_Connect_Err;
	} else if( *stop ) {
		return WL_Stop;
	}

	//domain nameѪR
	ipaddress = WL_GetAddrFromIP(ip);
	if( ipaddress == 0 ) {
		return WL_Connect_Err;
	}
	
	*skt = socket(AF_INET,SOCK_STREAM,0);
	if( *skt != INVALID_SOCKET )
	{
		server_addr.sin_family      = AF_INET;	
		server_addr.sin_addr.s_addr = ipaddress;
		server_addr.sin_port        = htons(port);		
	
		flag = 1;
		ioctl(*skt, FIONBIO, &flag);	//]wnon-block mode
		do
		{
			if( *stop ) {
				WL_CloseSktHandle(skt);
				return WL_Stop;
			}
			if( connect(*skt, (struct sockaddr *)&server_addr, sizeof(server_addr)) >= 0 )
			{
				flag = 0;
				ioctl(*skt, FIONBIO, &flag);	//^block mode
				getsockname(*skt , (struct sockaddr*)&cli_addr , &len);
				sprintf(localip,"%d.%d.%d.%d",*(((char*)&cli_addr)+4)&0xff, *(((char*)&cli_addr)+5)&0xff ,*(((char*)&cli_addr)+6)&0xff , *(((char*)&cli_addr)+7)&0xff);
		
				return WL_Connect_OK;
			}
			usleep( 100 * 1000 );
		} while ( retrytimes-- > 1 );
	}
	WL_CloseSktHandle(skt);
	return WL_Connect_Err;
#endif//LINUX
#endif//_WINDOWS
}

/*
 * =============================================================================
 *  Function name: WLNblkUDPConnect
 *  : nonblocking�覡���@UDP Connect, ���O��WLNblkTCPConnect�����ǤJ�إߦn��Socket
 *
 *  Parameter:
 *      (INPUT)
 *       int*           - skt       : UDP's socket handle, must had been created 
 *       char*          - ip        : Server ip address
 *       unsigned       - port      : Server port
 *       int            - wait_time : ���ݳs�u���ɶ��A�H������
 *       int*           - stop      : �O�_�n���flage
 *
 *  Return value:
 *       int            - WL_Connect_OK   : �s�u���\
 *                      - WL_Connect_Err  : ����s�u���~
 *                      - WL_Stop         : �����s�u
 *
 *  History:
 *       Jay Wang      06/29/2010          Add
 * =============================================================================
 */
int WL_NblkUDPConnect(int *skt, char *ip, unsigned port, int *stop)
{
	if (*skt == INVALID_SOCKET)
		return WL_Connect_Err;

	if (ip==NULL)
	{
		WL_CloseSktHandle(skt); // connect fail, release it
		return WL_Connect_Err;
	}

	if (*stop)
	{
		WL_CloseSktHandle(skt); // connect fail, release it
		return WL_Stop;
	}


	int iSocketResult;
	struct sockaddr_in server_addr;
	
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons((unsigned short)port);
	server_addr.sin_addr.s_addr = WL_GetAddrFromIP(ip);

	iSocketResult = connect(*skt,(struct sockaddr *)&server_addr,sizeof(server_addr));
	if (iSocketResult==0) 
		return WL_Connect_OK; // connect OK
	else 
	{
#ifdef _WINDOWS
		TRACE("Error:%d\r\n",GetLastError());
#endif
		WL_CloseSktHandle(skt); // connect fail, release it
		return WL_Connect_Err;
	}
}

/*
 * =============================================================================
 *  Function name: WL_BindPort_TCPConnect
 *  : ��Bind�@��port�A�M���~�b�h��TCP Connection�ʧ@
 *
 *  Parameter:
 *      (INPUT)
 *       int*           - skt       : TCP's socket handle
 *       char*          - ip        : Server ip address
 *       SOCKADDR_IN*   - Cli_Addr  : Client Address info
 *       unsigned int   - Cli_Port  : Client Port
 *       unsigned int   - Ser_Port  : Server Port
 *       int*           - stop      : stop connecting
 *
 *  Return value:
 *       int            - WL_Connect_OK   : �s�u���\
 *                      - WL_Connect_Err  : �����s�u���~
 *                      - WL_Stop         : �����s�u
 *
 *  History:
 *       Kidd Hsu       2/23/2009          Create
 * =============================================================================
 */

int WL_BindPort_TCPConnect(int *skt, char *ip, SOCKADDR_IN *Cli_Addr, unsigned int Cli_Port, unsigned int Ser_Port, int *stop)
{
	if (*stop) 
		return WL_Stop;

#ifdef _WINDOWS

	int error;
	struct sockaddr_in server_addr;
	server_addr.sin_addr.s_addr = WL_GetAddrFromIP(ip);
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(Ser_Port);	
	
	int rlen =0;
	char name[64] = {0};
	HOSTENT *host;
	char client_list[16][32];	//	Used to initialize client_list_p
	char *client_list_p[16];	//	Used to save all possible IP
	int index=0;
	int count=0;				//	Count of all possible IP
	char *tmp = NULL;			//	tmp for IP
	for (index=0;index<16;index++)
		client_list_p[index]=(char *)&client_list[index];
	
	if (gethostname(name,64)==0) 
	{
		host=gethostbyname(name);
		if (host) 
		{
			index = 0;
			//	Retrieve all possible IP
			while (host->h_addr_list[index]) 
			{
				tmp = inet_ntoa( *(in_addr *)(host->h_addr_list[index++]) );
				if( tmp )
					strcpy( client_list_p[count++] , tmp );
			}
			
			//	Bind port, test the connection
			Cli_Addr->sin_port = htons(Cli_Port);
			Cli_Addr->sin_family = AF_INET;

			for(index=0;index<count;index++)
			{
				Cli_Addr->sin_addr.s_addr = inet_addr( client_list_p[index] );
				
				if(*skt!=INVALID_SOCKET) 
					WL_CloseSktHandle(skt);
		
				*skt=socket(AF_INET,SOCK_STREAM,0);
				
				if(!bind(*skt, (SOCKADDR *)Cli_Addr, sizeof(SOCKADDR)))
				{
					rlen = connect(*skt,(sockaddr *)&server_addr,sizeof(sockaddr_in));
					if( rlen == 0)
						return WL_Connect_OK;
				}
				else // bind ����port
				{
					error = WSAGetLastError();
					TRACE("bind failed with error %d\n", error);
					break;
				}
			}
		}
	}
	WL_CloseSktHandle(skt);
	return WL_Connect_Err;

#else
#ifdef LINUX
	struct sockaddr_in   server_addr;
	int 	flag = 1;
	int		ipaddress;
	int		i = 1;
	struct ifreq ifr;
	unsigned char *tmp_str;

	if( ip == NULL ) {
		return WL_Connect_Err;
	} else if( *stop ) {
		return WL_Stop;
	}

	//domain name�ѪR
	ipaddress = WL_GetAddrFromIP(ip);
	if( ipaddress == 0 ) {
		return WL_Connect_Err;
	}
	
	*skt = socket(AF_INET,SOCK_STREAM,0);
	if( *skt != INVALID_SOCKET )
	{
		server_addr.sin_family      = AF_INET;	
		server_addr.sin_addr.s_addr = ipaddress;
		server_addr.sin_port        = htons(Ser_Port);		
	
		flag = 1;
		ioctl(*skt, FIONBIO, &flag);	//�]�wnon-block mode
		do
		{
			strncpy(ifr.ifr_name, "egiga0", 16);
		
			if(ioctl(*skt, SIOCGIFADDR, &ifr) < 0){
				printf("Can't get interface %s\n", "egiga0");
			}
	
			tmp_str = (unsigned char*)ifr.ifr_addr.sa_data+2;

			Cli_Addr->sin_addr.s_addr = inet_addr( (char*)tmp_str );

			if(*skt!=INVALID_SOCKET) 
				WL_CloseSktHandle(skt);
			
			*skt=socket(AF_INET,SOCK_STREAM,0);
			Cli_Addr->sin_family = AF_INET;
			Cli_Addr->sin_port = htons(Cli_Port);
			if(!bind(*skt, (SOCKADDR *)Cli_Addr, sizeof(SOCKADDR)))
			{
				if( connect(*skt, (struct sockaddr *)&server_addr, sizeof(server_addr)) >= 0 )
				{
					flag = 0;
					ioctl(*skt, FIONBIO, &flag);	//���^block mode
					return WL_Connect_OK;
				}
			}
			else // bind ����port
			{
				printf("bind failed with error \n");
				break;
			}
			i++;
		}while(i < 3 );
	}
	WL_CloseSktHandle(skt);
	return WL_Connect_Err;
#endif//LINUX
#endif//_WINDOWS
}


/*
 * =============================================================================
 *  Function name: WLCloseSktHandle
 *  : �?���@Socket Handle
 *
 *  Parameter:
 *      (INPUT)
 *       int*           - skt       : TCP's socket handle
 *
 *  Return value:
 *
 *  History:
 *       Kidd Hsu      10/29/2008          Modified
 * =============================================================================
 */
void WL_CloseSktHandle(int *skt)
{
	if( *skt == INVALID_SOCKET ) {
		return;
	}
#ifdef _WINDOWS
	BOOL bOptVal = TRUE;
    int bOptLen = sizeof(BOOL);

	setsockopt(*skt, SOL_SOCKET, SO_LINGER, (char*)&bOptVal, bOptLen);
	closesocket( *skt );
#else
#ifdef LINUX
	shutdown( *skt, SHUT_RDWR );
	close( *skt );
#endif//LINUX
#endif//_WINDOWS
	*skt = INVALID_SOCKET;
}

/*
 * =============================================================================
 *  Function name: WLNblkRecvHeaderByte
 *  : nonblocking�B�@��1 Byte���覡��HTTP Header�C
 *
 *  Parameter:
 *      (INPUT)
 *       int            - skt       : TCP's socket handle
 *       char*          - result    : Receive buffer pointer
 *       int&           - size      : �`�@�����h��Byte������
 *       int            - bufsize   : Receive buffer�̤jsize�e�q
 *
 *  Return value:
 *       int            - WL_Recv_OK     : Recv���\
 *                      - WL_Recv_Err    : Recv����
 *                      - WL_Buffer_Full : ���o�����Ƥj���x�sBuff�e�q
 *
 *  History:
 *       Kidd Hsu      10/29/2008          Modified
 * =============================================================================
 */
int WL_NblkRecvHeaderByte(int *skt, char *result, int *size, int bufsize, int *stop)
{
#ifdef _WINDOWS
	int rc,iLen=0;
	fd_set dsmask;
	struct timeval timeout;
	
	FD_ZERO(&dsmask);
	FD_SET(*skt,&dsmask);
	timeout.tv_sec=WL_RECV_TIMEOUT;
	timeout.tv_usec=0;
	*size = 0;

	do
	{
		rc=select(*skt+1,&dsmask,(fd_set *)NULL,(fd_set *)NULL,&timeout);
		if(rc > 0)
		{
			if (*stop)
			{
				FD_CLR(( unsigned int )*skt, &dsmask );
				WL_CloseSktHandle(skt);
				return WL_Stop;
			}
			rc = recv(*skt, result+iLen, 1, 0);
			if ( rc == SOCKET_ERROR || rc == 0) {
				FD_CLR(( unsigned int )*skt, &dsmask );
				WL_CloseSktHandle(skt);
				return WL_Recv_Err;
			}
	
			iLen += rc;
			*size = iLen;
			if ( iLen == bufsize ) {
				FD_CLR(( unsigned int )*skt, &dsmask );
				WL_CloseSktHandle(skt);
				return WL_Buffer_Full;
			}

			char* prev = strstr(result,"\r\n\r\n");
			if(prev) {
				FD_CLR(( unsigned int )*skt, &dsmask );
				return WL_Recv_OK;
			}
		}
	}while(rc>0);
	
	FD_CLR(( unsigned int )*skt, &dsmask );
	WL_CloseSktHandle(skt);
#else
#ifdef LINUX
	char	recvbuf[1], *ptr;
	int		recv_total_size;
	
	*size = 0;
	ptr = result;
	recv_total_size = 0;
	for(;;) {
		if( *stop )
		{
			return WL_Stop;
		}
		
		if( recv( *skt, recvbuf, 1, 0 ) <= 0 )
		{
			return WL_Recv_Err;
		}
		*ptr++ = *recvbuf;			//���Jhttp Header
		recv_total_size++;
		
		if( recv_total_size == bufsize )
		{
			return WL_Buffer_Full;
		}
		if( recv_total_size > 4 )  //�P�_�O�_http header����
		{
			if( strncmp( result + recv_total_size - 4, "\r\n\r\n" ,4 ) == 0 )
			{
				result[recv_total_size] = 0;
				*size = recv_total_size;
				return WL_Recv_OK;
			}
		}
	}
#endif//LINUX
#endif//_WINDOWS
	return WL_Recv_Err;
}

/*
 * =============================================================================
 *  Function name: WL_NblkRecvHeaderByteTimeout
 *  : nonblocking�B�@��1 Byte���覡��HTTP Header�C
 *
 *  Parameter:
 *      (INPUT)
 *       int            - skt       : TCP's socket handle
 *       char*          - result    : Receive buffer pointer
 *       int&           - size      : �`�@�����h��Byte������
 *       int            - bufsize   : Receive buffer�̤jsize�e�q
 *		 int			- iTime		: �̪����ݮɶ�, ����:��
 *
 *  Return value:
 *       int            - WL_Recv_OK     : Recv���\
 *                      - WL_Recv_Err    : Recv����
 *                      - WL_Buffer_Full : ���o�����Ƥj���x�sBuff�e�q
 *
 *  History:
 *       Jay      05/12/2010          Add
 * =============================================================================
 */
int WL_NblkRecvHeaderByteTimeout(int *skt, char *result, int *size, int bufsize, int *stop, int iTime)
{
#ifdef _WINDOWS
	int rc,iLen=0;
	fd_set dsmask;
	struct timeval timeout;
	
	FD_ZERO(&dsmask);
	FD_SET(*skt,&dsmask);
	timeout.tv_sec=iTime;
	timeout.tv_usec=0;
	*size = 0;

	do
	{
		rc=select(*skt+1,&dsmask,(fd_set *)NULL,(fd_set *)NULL,&timeout);
		if(rc > 0)
		{
			if (*stop)
			{
				FD_CLR(( unsigned int )*skt, &dsmask );
				WL_CloseSktHandle(skt);
				return WL_Stop;
			}
			rc = recv(*skt, result+iLen, 1, 0);
			if ( rc == SOCKET_ERROR || rc == 0) {
				FD_CLR(( unsigned int )*skt, &dsmask );
				WL_CloseSktHandle(skt);
				return WL_Recv_Err;
			}
	
			iLen += rc;
			*size = iLen;
			if ( iLen == bufsize ) {
				FD_CLR(( unsigned int )*skt, &dsmask );
				WL_CloseSktHandle(skt);
				return WL_Buffer_Full;
			}

			char* prev = strstr(result,"\r\n\r\n");
			if(prev) {
				FD_CLR(( unsigned int )*skt, &dsmask );
				return WL_Recv_OK;
			}
		}
	}while(rc>0);
	
	FD_CLR(( unsigned int )*skt, &dsmask );
	WL_CloseSktHandle(skt);
#else
#ifdef LINUX
	char	recvbuf[1], *ptr;
	int		recv_total_size;
	
	*size = 0;
	ptr = result;
	recv_total_size = 0;
	struct timeval timeout;
	timeout.tv_sec=iTime;
	timeout.tv_usec=0;

	if( *skt == INVALID_SOCKET)
		return WL_Recv_Err;

	else if( setsockopt( *skt, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(timeout) ) < 0)
		return WL_Recv_Err;

	for(;;) {
		if( *stop )
		{
			return WL_Stop;
		}
		
		if( recv( *skt, recvbuf, 1, 0 ) <= 0 )
		{
			return WL_Recv_Err;
		}
		*ptr++ = *recvbuf;			//���Jhttp Header
		recv_total_size++;
		
		if( recv_total_size == bufsize )
		{
			return WL_Buffer_Full;
		}
		if( recv_total_size > 4 )  //�P�_�O�_http header����
		{
			if( strncmp( result + recv_total_size - 4, "\r\n\r\n" ,4 ) == 0 )
			{
				result[recv_total_size] = 0;
				*size = recv_total_size;
				return WL_Recv_OK;
			}
		}
	}
#endif//LINUX
#endif//_WINDOWS
	return WL_Recv_Err;
}

/*
 * =============================================================================
 *  Function name: WLNblkRecvHeaderValue
 *  : nonblocking�B�ব�̤j�q���覡��HTTP Header�C
 *
 *  Parameter:
 *      (INPUT)
 *       int            - skt       : TCP's socket handle
 *       char*          - result    : Receive buffer pointer
 *       int            - recvsize  : �`�@�����h��Byte������
 *       int            - bufsize   : Receive buffer�̤jsize�e�q
 *       char*          - dataptr	: ��ƶ}�l pointer���}
 *
 *  Return value:
 *       int            - WL_Recv_OK     : Recv���\
 *                      - WL_Recv_Err    : Recv����
 *                      - WL_Buffer_Full : ���o�����Ƥj���x�sBuff�e�q
 *
 *  History:
 *       Kidd Hsu      10/29/2008          Modified
 * =============================================================================
 */
int WL_NblkRecvHeaderValue(int *skt, char *result, int recvsize, int bufsize, char** dataptr, int *stop)
{
#ifdef _WINDOWS
	int rc,iLen=0;
	fd_set dsmask;
	struct timeval timeout;
	
	FD_ZERO(&dsmask);
	FD_SET(*skt,&dsmask);
	timeout.tv_sec=WL_RECV_TIMEOUT;
	timeout.tv_usec=0;
	
	if( recvsize > bufsize )
	{
		FD_CLR(( unsigned int )*skt, &dsmask );
		WL_CloseSktHandle(skt);
		return WL_Buffer_Full;
	}
	
	do
	{
		rc=select(*skt+1,&dsmask,(fd_set *)NULL,(fd_set *)NULL,&timeout);
		if(rc<=0)
		{	
			FD_CLR(( unsigned int )*skt, &dsmask );
			WL_CloseSktHandle(skt);
			return WL_Recv_Err;
		}
		if (*stop)
		{
			FD_CLR(( unsigned int )*skt, &dsmask );
			WL_CloseSktHandle(skt);
			return WL_Stop;
		}
		rc = recv(*skt, result+iLen, recvsize-iLen, 0);
		if ( rc == SOCKET_ERROR || rc == 0) {
			FD_CLR(( unsigned int )*skt, &dsmask );
			WL_CloseSktHandle(skt);
			return WL_Recv_Err;
		}

		iLen += rc;
		
	}while(iLen < recvsize);
	FD_CLR(( unsigned int )*skt, &dsmask );

	*dataptr = strstr(result,"\r\n\r\n");
	if(*dataptr != NULL) 
	{
		*dataptr += 4;
		return WL_Recv_OK;
	}
	WL_CloseSktHandle(skt);
	
#else
#ifdef LINUX
	int		rc;
	char	*ptr;
	if( recvsize > bufsize )
	{
		return WL_Buffer_Full;
	}

	ptr = result;
	while( recvsize > 0 )
	{
		if( *stop )
		{
			return WL_Stop;
		}
		rc = recv( *skt, ptr, recvsize, 0 );
		if( rc <= 0 )
		{
			return WL_Recv_Err;
		}
		ptr += rc;
		recvsize -= rc;
	}
	*ptr = 0;
		
	*dataptr = strstr( result, "\r\n\r\n" );
	if( *dataptr != NULL )
	{
		*dataptr += 4;
		return WL_Recv_OK;
	}
#endif//LINUX
#endif//_WINDOWS
	return WL_Recv_Err;
}


int WL_NblkRecvHeaderValueTimeout(int *skt, char *result, int recvsize, int bufsize, char** dataptr, int *stop, int time)
{
#ifdef _WINDOWS
	int rc,iLen=0;
	fd_set dsmask;
	struct timeval timeout;
	
	FD_ZERO(&dsmask);
	FD_SET(*skt,&dsmask);
	timeout.tv_sec=time;
	timeout.tv_usec=0;
	
	if( recvsize > bufsize )
	{
		FD_CLR(( unsigned int )*skt, &dsmask );
		WL_CloseSktHandle(skt);
		return WL_Buffer_Full;
	}
	
	do
	{
		rc=select(*skt+1,&dsmask,(fd_set *)NULL,(fd_set *)NULL,&timeout);
		if(rc<=0)
		{	
			FD_CLR(( unsigned int )*skt, &dsmask );
			WL_CloseSktHandle(skt);
			return WL_Recv_Err;
		}
		if (*stop)
		{
			FD_CLR(( unsigned int )*skt, &dsmask );
			WL_CloseSktHandle(skt);
			return WL_Stop;
		}
		rc = recv(*skt, result+iLen, recvsize-iLen, 0);
		if ( rc == SOCKET_ERROR || rc == 0) {
			FD_CLR(( unsigned int )*skt, &dsmask );
			WL_CloseSktHandle(skt);
			return WL_Recv_Err;
		}

		iLen += rc;
		
	}while(iLen < recvsize);
	FD_CLR(( unsigned int )*skt, &dsmask );

	*dataptr = strstr(result,"\r\n\r\n");
	if(*dataptr != NULL) 
	{
		*dataptr += 4;
		return WL_Recv_OK;
	}
	WL_CloseSktHandle(skt);
	
#else
#ifdef LINUX
	int		rc;
	char	*ptr;
	if( recvsize > bufsize )
	{
		return WL_Buffer_Full;
	}

	ptr = result;
	while( recvsize > 0 )
	{
		if (*stop)
		{
			return WL_Stop;
		}
		rc = recv( *skt, ptr, recvsize, 0 );
		if( rc <= 0 )
		{
			return WL_Recv_Err;
		}
		ptr += rc;
		recvsize -= rc;
	}
	*ptr = 0;
		
	*dataptr = strstr( result, "\r\n\r\n" );
	if( *dataptr != NULL )
	{
		*dataptr += 4;
		return WL_Recv_OK;
	}
#endif//LINUX
#endif//_WINDOWS
	return WL_Recv_Err;
}
/*
 * =============================================================================
 *  Function name: WLNblkRecvHTMLFile
 *  : nonblocking�B�ব�̤j�q���覡��HTML File�A�̤j���쪺���Ƭ�Buffer�e�q�j�p�C
 *
 *  Parameter:
 *      (INPUT)
 *       int            - skt       : TCP's socket handle
 *       char*          - result    : Receive buffer pointer
 *       int&           - size      : �`�@�����h��Byte������
 *       int            - bufsize   : Receive buffer�̤jsize�e�q
 *
 *  Return value:
 *       int            - WL_Recv_OK     : �s�u���\
 *                      - WL_Recv_Err    : �s�u����
 *                      - WL_Buffer_Full  : ���o�����Ƥj���x�sBuff�e�q
 *
 *  History:
 *       Kidd Hsu      10/29/2008          Modified
 * =============================================================================
 */
int WL_NblkRecvHTMLFile(int *skt, char *result, int *size, int bufsize, int *stop)
{
#ifdef _WINDOWS
	int rc,iLen=0;
	fd_set dsmask;
	struct timeval timeout;

	FD_ZERO(&dsmask);
	FD_SET(*skt,&dsmask);
	timeout.tv_sec=WL_RECV_TIMEOUT;
	timeout.tv_usec=0;
	*size = 0;


	while(TRUE)
	{
		rc=select(*skt+1,&dsmask,(fd_set *)NULL,(fd_set *)NULL,&timeout);
		if(rc<=0)
		{	
			FD_CLR(( unsigned int )*skt, &dsmask );
			WL_CloseSktHandle(skt);
			return WL_Recv_Err;
		}
		if (*stop)
		{
			FD_CLR(( unsigned int )*skt, &dsmask );
			WL_CloseSktHandle(skt);
			return WL_Stop;
		}
		rc = recv(*skt, result + iLen, bufsize - iLen, 0);
		if ( rc == SOCKET_ERROR )
		{
			FD_CLR(( unsigned int )*skt, &dsmask );
			WL_CloseSktHandle(skt);
			return WL_Recv_Err;
		}
		if ( rc == 0)
		{
			FD_CLR(( unsigned int )*skt, &dsmask );
			return WL_Recv_OK;
		}
		iLen += rc;
		*size = iLen;

		if ( iLen == bufsize )
		{
			FD_CLR(( unsigned int )*skt, &dsmask );
			return WL_Buffer_Full;
		}
	}
#else
#ifdef LINUX
	int rc, iLen=0;
	*size = 0;
	while (1)
	{
		if (*stop)
		{
			WL_CloseSktHandle(skt);
			return WL_Stop;
		}
		rc = recv( *skt, result + iLen, bufsize - iLen, 0 );
		if ( rc == SOCKET_ERROR )
		{
			WL_CloseSktHandle(skt);
			return WL_Recv_Err;
		}
		if ( rc == 0)
			return WL_Recv_OK;
		iLen += rc;
		*size = iLen;

		if ( iLen == bufsize )
		{
			WL_CloseSktHandle(skt);
			return WL_Buffer_Full;
		}
	}
#endif//LINUX
#endif//_WINDOWS
	return WL_Recv_Err;
}

/*
 * =============================================================================
 *  Function name: WL_NblkRecvHTMLFileTimeout
 *  : nonblocking且能?��?大�??�方式收HTML File，�?大收?��?資�??�Buffer容�?大�??? *
 *  Parameter:
 *      (INPUT)
 *       int            - skt       : TCP's socket handle
 *       char*          - result    : Receive buffer pointer
 *       int&           - size      : 總共?�收多�?Byte?��??? *       int            - bufsize   : Receive buffer?�大size容�?
 *       int            - timeout   : recv TimeOut (sec)
 *
 *  Return value:
 *       int            - WL_Recv_OK     : ????��?
 *                      - WL_Recv_Err    : ???失�?
 *                      - WL_Buffer_Full  : ?��??��??�大?�儲存Buff容�?
 *
 *  History:
 *       Eric Liang      03/05/2012          Modified
 * =============================================================================
 */
int WL_NblkRecvHTMLFileTimeout(int *skt, char *result, int *size, int bufsize, int *stop,int TimeOut/*sec*/)
{
#ifdef _WINDOWS
	int rc,iLen=0;
	fd_set dsmask;
	struct timeval timeout;

	FD_ZERO(&dsmask);
	FD_SET(*skt,&dsmask);
	if(TimeOut<0 || TimeOut > WL_RECV_TIMEOUT)
		TimeOut = WL_RECV_TIMEOUT;

	timeout.tv_sec=TimeOut;
	timeout.tv_usec=0;
	*size = 0;


	while(TRUE)
	{
		rc=select(*skt+1,&dsmask,(fd_set *)NULL,(fd_set *)NULL,&timeout);
		if(rc<=0)
		{	
			FD_CLR(( unsigned int )*skt, &dsmask );
			WL_CloseSktHandle(skt);
			return WL_Recv_Err;
		}
		if (*stop)
		{
			FD_CLR(( unsigned int )*skt, &dsmask );
			WL_CloseSktHandle(skt);
			return WL_Stop;
		}
		rc = recv(*skt, result + iLen, bufsize - iLen, 0);
		if ( rc == SOCKET_ERROR )
		{
			FD_CLR(( unsigned int )*skt, &dsmask );
			WL_CloseSktHandle(skt);
			return WL_Recv_Err;
		}
		if ( rc == 0)
		{
			FD_CLR(( unsigned int )*skt, &dsmask );
			return WL_Recv_OK;
		}
		iLen += rc;
		*size = iLen;

		if ( iLen == bufsize )
		{
			FD_CLR(( unsigned int )*skt, &dsmask );
			return WL_Buffer_Full;
		}
	}
#else
#ifdef LINUX
	int rc, iLen=0;
	*size = 0;
	while (1)
	{
		if (*stop)
		{
			WL_CloseSktHandle(skt);
			return WL_Stop;
		}
		rc = recv( *skt, result + iLen, bufsize - iLen, 0 );
		if ( rc == SOCKET_ERROR )
		{
			WL_CloseSktHandle(skt);
			return WL_Recv_Err;
		}
		if ( rc == 0)
			return WL_Recv_OK;
		iLen += rc;
		*size = iLen;

		if ( iLen == bufsize )
		{
			WL_CloseSktHandle(skt);
			return WL_Buffer_Full;
		}
	}
#endif//LINUX
#endif//_WINDOWS
	return WL_Recv_Err;
}

/*
 * =============================================================================
 *  Function name: WLNblkRecvFullData
 *  : ?�收一?��?大�??��??��?(nonblocking)
 *
 *  Parameter:
 *      (INPUT)
 *       int            - skt       : TCP's socket handle
 *       char*          - result    : Receive buffer pointer
 *       int            - datasize  : ?��??��?資�??��?大�?
 *       int            - bufsize   : ?��?Buffer之�?大容?? *
 *		 int			- timeout   : default?�WL_RECV_TIMEOUT(30sec)
 *
 *  Return value:
 *       int            - WL_Recv_OK  : Recv?��?
 *                      - WL_Recv_Err : Recv失�?
 *
 *  History:
 *       Kidd Hsu      10/29/2008          Modified
 * =============================================================================
 */
int WL_NblkRecvFullData(int *skt, char* result, int datasize, int bufsize, int *stop)
{
#ifdef _WINDOWS
	int count = 0;
	int rc    = 0;
	fd_set dsmask;
	struct timeval timeout;

	FD_ZERO(&dsmask);
	FD_SET(*skt,&dsmask);
	timeout.tv_sec=WL_RECV_TIMEOUT;
	timeout.tv_usec=0;

	if (datasize > bufsize)
	{
		FD_CLR(( unsigned int )*skt, &dsmask );
		WL_CloseSktHandle(skt);
		return WL_Buffer_Full;
	}

	while (count < datasize) 
	{
		rc=select(*skt+1,&dsmask,(fd_set *)NULL,(fd_set *)NULL,&timeout);
		if (*stop)
		{
			FD_CLR(( unsigned int ) *skt, &dsmask );
			WL_CloseSktHandle(skt);
			return WL_Stop;
		}
		if(rc<=0)
		{	
			FD_CLR(( unsigned int )*skt, &dsmask );
			WL_CloseSktHandle(skt);
			return WL_Recv_Err;
		}

		if(FD_ISSET(*skt,&dsmask)){
			int n = recv(*skt,result+count,datasize-count,0);
			if (n == SOCKET_ERROR || n==0)  {
				FD_CLR(( unsigned int )*skt, &dsmask );
				WL_CloseSktHandle(skt);
				return WL_Recv_Err; // ?��?
			}
			count += n;
		}		
	}
	FD_CLR(( unsigned int )*skt, &dsmask );
#else
#ifdef LINUX
	int count = 0;
	int n;

	if ( datasize > bufsize )
	{
		WL_CloseSktHandle(skt);
		return WL_Buffer_Full;
	}
	
	while ( count < datasize ) 
	{
		if (*stop)
		{
			WL_CloseSktHandle(skt);
			return WL_Stop;
		}
		n = recv( *skt, result+count, datasize-count, 0 );
		if ( n == SOCKET_ERROR || n == 0 )
		{
			WL_CloseSktHandle(skt);
			return WL_Recv_Err; // �_�u
		}
		count += n;
	}
#endif//LINUX
#endif//_WINDOWS
	return WL_Recv_OK;
}

int WL_NblkRecvFullDataTimeout(int *skt, char* result, int datasize, int bufsize, int *stop, int time)
{
#ifdef _WINDOWS
	int count = 0;
	int rc    = 0;
	fd_set dsmask;
	struct timeval timeout;

	FD_ZERO(&dsmask);
	FD_SET(*skt,&dsmask);
	timeout.tv_sec=time;
	timeout.tv_usec=0;

	if (datasize > bufsize)
	{
		FD_CLR(( unsigned int )*skt, &dsmask );
		WL_CloseSktHandle(skt);
		return WL_Buffer_Full;
	}

	while (count < datasize) 
	{
		rc=select(*skt+1,&dsmask,(fd_set *)NULL,(fd_set *)NULL,&timeout);
		if (*stop)
		{
			FD_CLR(( unsigned int )*skt, &dsmask );
			WL_CloseSktHandle(skt);
			return WL_Stop;
		}
		if(rc<=0)
		{	
			FD_CLR(( unsigned int )*skt, &dsmask );
			WL_CloseSktHandle(skt);
			return WL_Recv_Err;
		}

		if(FD_ISSET(*skt,&dsmask)){
			int n = recv(*skt,result+count,datasize-count,0);
			if (n == SOCKET_ERROR || n==0)  {
				FD_CLR(( unsigned int )*skt, &dsmask );
				WL_CloseSktHandle(skt);
				return WL_Recv_Err; // �_�u
			}
			count += n;
		}		
	}
	FD_CLR(( unsigned int )*skt, &dsmask );
#else
#ifdef LINUX
	int count = 0;
	int n;

	if ( datasize > bufsize )
	{
		WL_CloseSktHandle(skt);
		return WL_Buffer_Full;
	}
	
	while ( count < datasize ) 
	{
		if (*stop)
		{
			WL_CloseSktHandle(skt);
			return WL_Stop;
		}
		n = recv( *skt, result+count, datasize-count, 0 );
		if ( n == SOCKET_ERROR || n == 0 )
		{
			WL_CloseSktHandle(skt);
			return WL_Recv_Err; // �_�u
		}
		count += n;
	}
#endif//LINUX
#endif//_WINDOWS
	return WL_Recv_OK;
}
/*
 * =============================================================================
 *  Function name: WLNblkRecvData
 *  : �����@�Q�n�j�p�����ƶq(nonblocking)
 *
 *  Parameter:
 *      (INPUT)
 *       int            - skt       : TCP's socket handle
 *       char*          - result    : Receive buffer pointer
 *       int            - datasize  : �Q�n���o���ƶq���j�p
 *       int            - recvsize  : ���ڤW���o�����ƶq�j�p
 *
 *  Return value:
 *       int            - rc              : Recv���\�A�B�^�Ǭ��������ƪ��j�p
 *                      - WL_Recv_Err     : Recv����
 *
 *  History:
 *       Kidd Hsu      10/29/2008          Modified
 * =============================================================================
 */
int WL_NblkRecvData(int *skt, char *result, int datasize, int *recvsize, int *stop)
{
#ifdef _WINDOWS
	int    rc=0;
	fd_set dsmask;
	struct timeval timeout;

	FD_ZERO(&dsmask);
	FD_SET(*skt,&dsmask);
	timeout.tv_sec=WL_RECV_TIMEOUT;
	timeout.tv_usec=0;
	
	rc=select(*skt+1,&dsmask,(fd_set *)NULL,(fd_set *)NULL,&timeout);
	if(rc<=0)
	{
		FD_CLR(( unsigned int )*skt, &dsmask );
		WL_CloseSktHandle(skt);
		return WL_Recv_Err;
	}
	if (*stop)
	{
		FD_CLR(( unsigned int )*skt, &dsmask );
		WL_CloseSktHandle(skt);
		return WL_Stop;
	}
	rc = recv(*skt,result,datasize,0);
	FD_CLR(( unsigned int )*skt, &dsmask );
	
	if (rc <= 0)
	{
		WL_CloseSktHandle(skt);
		return WL_Recv_Err;
	}
	*recvsize = rc;

#else
#ifdef LINUX
	int    rc=0;
	if (*stop)
	{
		WL_CloseSktHandle(skt);
		return WL_Stop;
	}
	rc = recv( *skt, result, datasize, 0 );
	if (rc <= 0)
	{
		WL_CloseSktHandle(skt);
		return WL_Recv_Err;
	}
	*recvsize = rc;
#endif//LINUX
#endif//_WINDOWS
	return WL_Recv_OK;
}

/*
 * =============================================================================
 *  Function name: WL_NblkRecvUntilStr
 *  : nonblocking�B�@�� 1 Byte ���覡���ơA���� stop string �X�{�A��������ơC
 *
 *  Parameter:
 *      (INPUT)
 *       int            - skt       : TCP's socket handle
 *       char*          - result    : Receive buffer pointer
 *       int&           - size      : �`�@�����h��Byte������
 *       int            - bufsize   : Receive buffer�̤jsize�e�q
 *       char*          - StopStr   : �����ܦ� String ����
 *       int*           - stop      : �t�� sotp ����
 *
 *  Return value:
 *       int            - WL_Recv_OK     : Recv���\
 *                      - WL_Recv_Err    : Recv����
 *                      - WL_Buffer_Full : ���o�����Ƥj���x�sBuff�e�q
 *
 *  History:
 *       Kidd Hsu      6/29/2009          Modified
 * =============================================================================
 */
int WL_NblkRecvUntilStr(int *skt, char *result, int *size, int bufsize, char* StopStr, int *stop)
{
	#ifdef _WINDOWS
	int rc,iLen=0;
	fd_set dsmask;
	struct timeval timeout;
	
	FD_ZERO(&dsmask);
	FD_SET(*skt,&dsmask);
	timeout.tv_sec=WL_RECV_TIMEOUT;
	timeout.tv_usec=0;
	*size = 0;

	do
	{
		rc=select(*skt+1,&dsmask,(fd_set *)NULL,(fd_set *)NULL,&timeout);
		if(rc > 0)
		{
			if (*stop)
			{
				FD_CLR(( unsigned int )*skt, &dsmask );
				WL_CloseSktHandle(skt);
				return WL_Stop;
			}
			rc = recv(*skt, result+iLen, 1, 0);
			if ( rc == SOCKET_ERROR || rc == 0) {
				FD_CLR(( unsigned int )*skt, &dsmask );
				WL_CloseSktHandle(skt);
				return WL_Recv_Err;
			}
	
			iLen += rc;
			*size = iLen;
			if ( iLen == bufsize ) {
				FD_CLR(( unsigned int )*skt, &dsmask );
				WL_CloseSktHandle(skt);
				return WL_Buffer_Full;
			}

			char* prev = strstr(result,StopStr);
			if(prev) {
				FD_CLR(( unsigned int )*skt, &dsmask );
				return WL_Recv_OK;
			}
		}
	}while(rc>0);
	
	FD_CLR(( unsigned int )*skt, &dsmask );
	WL_CloseSktHandle(skt);
#else
#ifdef LINUX
	char	recvbuf[1], *ptr;
	int		recv_total_size;
	int     StopStrLen = 0;
	
	*size = 0;
	ptr = result;
	recv_total_size = 0;
	StopStrLen = strlen(StopStr);
	for(;;) {
		if( *stop )
		{
			return WL_Stop;
		}
		
		if( recv( *skt, recvbuf, 1, 0 ) <= 0 )
		{
			return WL_Recv_Err;
		}
		*ptr++ = *recvbuf;			//���Jhttp Header
		recv_total_size++;
		
		if( recv_total_size == bufsize )
		{
			return WL_Buffer_Full;
		}
		if( recv_total_size > StopStrLen )  //�P�_�O�_http header����
		{
			if( strncmp( result + recv_total_size - StopStrLen, StopStr ,StopStrLen ) == 0 )
			{
				result[recv_total_size] = 0;
				*size = recv_total_size;
				return WL_Recv_OK;
			}
		}
	}
#endif//LINUX
#endif//_WINDOWS
	return WL_Recv_Err;
}

/*
 * =============================================================================
 *  Function name: WL_SendAllData
 *  : �ǰe�@�j�p�����ƶq(nonblocking)
 *
 *  Parameter:
 *      (INPUT)
 *       int            - skt       : TCP's socket handle
 *       char*          - result    : Receive buffer pointer
 *       int            - datasize  : �Q�n�ǰe���ƶq���j�p
 *
 *  Return value:
 *       int            - WL_Send_OK      : Send���\
 *                      - WL_Send_Err     : Send����
 *
 *  History:
 *       Kidd Hsu      10/29/2008          Modified
 * =============================================================================
*/
int WL_SendAllData(int *skt, char* sendcmd, int datasize, int *stop)
{
	int count = 0;
	int rc = 0;
	while (count < datasize)
	{
		if(*skt==INVALID_SOCKET)
			return WL_Send_Err;
		if (*stop)
		{
			WL_CloseSktHandle(skt);
			return WL_Stop;
		}
		rc = send(*skt,sendcmd+count,datasize-count,0);
		if (rc <= 0)
		{
			WL_CloseSktHandle(skt);
			return WL_Send_Err; // �_�u
		}
		count += rc;
	}
	return WL_Send_OK;
}


/*
 * =============================================================================
 *  Function name: WLGetUDPPort
 *  : ���o�@UDP�i�H�ϥΪ�port
 *
 *  Parameter:
 *      (INPUT)
 *       int            - startport  : �ˬdUDP Port�i�H�ϥΤ��_�lport���m(1025~65535)
 *       int            - index      : �ˬdstartport ~ startport+index-1��port
 *       int*           - port       : �i�H�ϥΪ�UDP Port
 *
 *  Return value:
 *       int            - TRUE      : Bind port���\
 *                      - FALSE     : Bind port����
 *
 *  History:
 *       Kidd Hsu      10/29/2008          Modified
 * =============================================================================
*/
int WL_GetUDPPort(int startport, int index, int *port)
{
    int skt = INVALID_SOCKET;
	int tmport = -1;
	int i;
	struct sockaddr_in   addr;

	skt = socket(AF_INET, SOCK_DGRAM, 0);

	for(i=0; i < index; i++)
	{
		tmport = startport+i;
		
		memset(&addr , 0 , sizeof(addr));
		addr.sin_family      = AF_INET;	
		addr.sin_addr.s_addr = 0;
		addr.sin_port        = htons(tmport);
		if( bind(skt, (struct sockaddr *)&addr, sizeof(struct sockaddr_in)) >= 0 )
		{
			*port = tmport;
			WL_CloseSktHandle(&skt);
			return TRUE;
		}
	}
	WL_CloseSktHandle(&skt);
	return FALSE;
}

/*
 * =============================================================================
 *  Function name: WLGetRandomRangePort
 *  : �H�����o�@���w�d�?i�H�ϥΪ��s��port,�M���d�?�W�L10000
 *
 *  Parameter:
 *      (INPUT)
 *       int            - iStartPort	: �ˬd�i��port���_�l���m
 *       int            - iEndPort		: �ˬd�i��port���������m
 *       int            - iRange		: �i�H�ϥΪ��s��port��
 *       BOOL           - bIsUDP		: �M�䪺�OUDP port��TCP port
 *      (OUTPUT)
 *       int*           - piFindPort	: ���쪺�Ĥ@��port
 *
 *  Return value:
 *       int            - TRUE      : �����ŦX��
 *                      - FALSE     : ����
 *
 *  History:
 *       Jay Wang      06/28/2010          Add
 * =============================================================================
*/
int WLGetRandomRangePort(int iStartPort, int iEndPort, int iRange, int *piFindPort, BOOL bIsUDP)
{
	if( ( iStartPort <= 0) || ( iEndPort > 65535) || ( iRange <= 0) )
		return FALSE;
	
	if( iEndPort < iStartPort)
		return FALSE;

	if( ( iEndPort - iStartPort + 1) > 10000)
		return FALSE;

	if( iRange > ( iEndPort - iStartPort + 1) )
		return FALSE;
	

	int i,j;
	int PortArray[10000];
	for( i = 0; i < (iEndPort-iStartPort+2-iRange); i++)
		PortArray[i] = iStartPort + i;


	int iSkt = INVALID_SOCKET;
	struct sockaddr_in   addr;

	
	for( i = 0; i < (iEndPort-iStartPort+2-iRange); i++)
	{

		int iRandomSelectedIndex = ( rand() % (iEndPort-iStartPort+2-iRange-i) ) +i;
		int iRandomSelectedPort = PortArray[iRandomSelectedIndex];
		PortArray[iRandomSelectedIndex] = PortArray[i];

		for( j = 0; j < iRange; j++)
		{

			if( bIsUDP)
				iSkt = socket(AF_INET, SOCK_DGRAM, 0);
			else
				iSkt = socket(AF_INET, SOCK_STREAM, 0);

			memset( &addr , 0 , sizeof(addr) );
			addr.sin_family = AF_INET;	
			addr.sin_addr.s_addr = 0;
			addr.sin_port = htons(iRandomSelectedPort+j);
			
			if( bind( iSkt, (struct sockaddr *)&addr, sizeof(struct sockaddr_in) ) == 0 )
				WL_CloseSktHandle( &iSkt);
			else
			{

				WL_CloseSktHandle( &iSkt);
				break;
			}
		}

		if( j >= iRange)
		{
			*piFindPort = iRandomSelectedPort;
			return TRUE;
		}
	}
	
	return FALSE;
}

int  WL_CreatUDPSkt(int *skt)
{
	*skt = socket(AF_INET, SOCK_DGRAM, 0);

	if (*skt == INVALID_SOCKET)
	{
		WL_CloseSktHandle(skt);
		return WL_Connect_Err;
	}
	else
		return WL_Connect_OK;
}
/*
 * =============================================================================
 *  Function name: WLCreateSerUDPSkt
 *  : �إߤ@UDP Server Socket�ç@bind port�ʧ@
 *
 *  Parameter:
 *      (INPUT)
 *       int*           - skt   : UDP Server Socket
 *       int            - port  : ���ƶǿ��J��port
 *
 *  Return value:
 *       int            - WL_Connect_OK      : �s�u���\
 *                      - WL_Connect_Err     : �s�u����
 *
 *  History:
 *       Kidd Hsu      10/29/2008          Modified
 * =============================================================================
*/
int WL_CreateSerUDPSkt(int *skt, int port)
{

	struct sockaddr_in   addr;

	*skt = socket(AF_INET, SOCK_DGRAM, 0);

	memset(&addr , 0 , sizeof(addr));
	addr.sin_family      = AF_INET;
	addr.sin_addr.s_addr = 0;
	addr.sin_port        = htons(port);
	if(bind(*skt, (struct sockaddr *)&addr, sizeof(struct sockaddr_in)) < 0)
	{
		WL_CloseSktHandle(skt);
		return WL_Connect_Err;
	}

	return WL_Connect_OK;
}
/*
 * =============================================================================
 *  Function name: WL_CreateUDPSktForSpeificIP
 *  : إߤ@UDP Server Socketç@bind portʧ@
 *
 *  Parameter:
 *      (INPUT)
 *       int*           - skt   : UDP Server Socket
 *       int            - port  : ƶǿJport
 *		 char*          - pszIP : wSwIP
 *
 *  Return value:
 *       int            - WL_Connect_OK      : su\
 *                      - WL_Connect_Err     : su
 *
 *  History:
 *       Janet Yao      09/01/2010          Modified
 * =============================================================================
*/
int WL_CreateUDPSktForSpeificIP(int *skt, int port , char* pszIP)
{

	struct sockaddr_in   addr;

	*skt = socket(AF_INET, SOCK_DGRAM, 0);

	memset(&addr , 0 , sizeof(addr));
	addr.sin_family      = AF_INET;
	addr.sin_addr.s_addr = inet_addr(pszIP);
	addr.sin_port        = htons(port);
	if(bind(*skt, (struct sockaddr *)&addr, sizeof(struct sockaddr_in)) < 0)
	{
		WL_CloseSktHandle(skt);
		return WL_Connect_Err;
	}

	return WL_Connect_OK;
}
/*
 * =============================================================================
 *  Function name: WLCreateCliUDPSkt
 *  : �إߤ@UDP Client Socket
 *
 *  Parameter:
 *      (INPUT)
 *       int*           - skt       : UDP Client Socket
 *       char*          - ser_ip    : Server IP Address
 *       int            - ser_port  : Server Port
 *
 *  Return value:
 *       int            - WL_Connect_OK      : �s�u���\
 *                      - WL_Connect_Err     : �s�u����
 *
 *  History:
 *       Kidd Hsu      10/29/2008          Modified
 * =============================================================================
*/
int WL_CreateCliUDPSkt(int *skt, char* ser_ip, int ser_port, SOCKADDR_IN *ser_addr)
{
	memset(ser_addr , 0 , sizeof(ser_addr));
	ser_addr->sin_family      = AF_INET;
	ser_addr->sin_addr.s_addr = inet_addr(ser_ip);
	ser_addr->sin_port        = htons(ser_port);

	*skt = socket(AF_INET, SOCK_DGRAM, 0);

	if (*skt == INVALID_SOCKET)
	{
		WL_CloseSktHandle(skt);
		return WL_Connect_Err;
	}
	else
		return WL_Connect_OK;
}

/*
 * =============================================================================
 *  Function name: WLNblkSendUDPData
 *  : Send UDP Data
 *
 *  Parameter:
 *      (INPUT)
 *       int            - skt       : Send UDP Socket
 *       SOCKADDR_IN    - ser_addr  : Server SOCKADDR_IN info
 *       char*          - sendcmd   : �ǰe������
 *       int            - datasize  : �ǰe�����Ƥj�p
 *
 *  Return value:
 *       int            - WL_Send_OK      : Send���\
 *                      - WL_Send_Err     : Send����
 *
 *  History:
 *       Kidd Hsu      10/29/2008          Modified
 * =============================================================================
*/
int WL_SendUDPData(int *skt, SOCKADDR_IN *ser_addr, char *sendcmd, int datasize, int *stop)
{
   	int Ret;

   	if (*stop)
	{
		WL_CloseSktHandle(skt);
		return WL_Stop;
	}

   	Ret = sendto(*skt, sendcmd, datasize, 0, (SOCKADDR *)ser_addr, sizeof(SOCKADDR));

   	if(Ret <= 0) {
	   	WL_CloseSktHandle(skt);
       		return WL_Send_Err;
   	}

   	return WL_Send_OK;
}

/*
 * =============================================================================
 *  Function name: WLNblkRecvUDPdata
 *  : Receive UDP Data
 *
 *  Parameter:
 *      (INPUT)
 *       int            - skt       : Reveive UDP Socket
 *       char*          - result    : �������Ƥ�pointer
 *       int            - size      : �������Ƥj�p
 *       SOCKADDR_IN    - cli_addr  : Client SOCKADDR_IN info
 *		 int            - wait_time : recv��timeout�ɶ��A�H������
 *
 *  Return value:
 *       int            - Ret             : Recv���\�A�^�Ǫ����Ƥj�p
 *                      - WL_Recv_Err     : Recv����
 *
 *  History:
 *       Kidd Hsu      10/29/2008          Modified
 * =============================================================================
*/
int WL_NblkRecvUDPData(int *skt, char *result, int size, SOCKADDR_IN *cli_addr, int wait_time, int *stop)
{

	fd_set dsmask;
	struct timeval timeout;
#ifdef _WINDOWS
	int SenderAddrSize = sizeof(SOCKADDR_IN);
#else
#ifdef LINUX
	unsigned int SenderAddrSize = sizeof(SOCKADDR_IN);
#endif
#endif

	FD_ZERO(&dsmask);
	FD_SET(*skt,&dsmask);

	//�]�wtimeout�ɶ�
	if( wait_time == 0 ) {
		timeout.tv_sec  = WL_RECV_TIMEOUT;
	} else {
		timeout.tv_sec  = wait_time;
	}
	timeout.tv_usec = 0;
	int Ret = 0;
#ifdef _WINDOWS	
	Ret=select(*skt+1,&dsmask,(fd_set *)NULL,(fd_set *)NULL,&timeout);
	if(Ret<=0)
	{
		WL_CloseSktHandle(skt);
		return WL_Recv_Err;
	}
#endif	
	if (*stop)
	{
		WL_CloseSktHandle(skt);
		return WL_Stop;
	}
	if ((Ret = recvfrom(*skt, result, size, 0,
		(SOCKADDR *)cli_addr, &SenderAddrSize)) == SOCKET_ERROR)
	{
		WL_CloseSktHandle(skt);
		return WL_Recv_Err;
	}
	return Ret;
}

/*
 * =============================================================================
 *  Function name: WL_NblkRecvUDPDataTimeout
 *  : Receive UDP Data
 *
 *  Parameter:
 *      (INPUT)
 *       int            - skt       : Reveive UDP Socket: �w�g�b�~���]�n Recv Timeout �� socket handle
 *       char*          - result    : �������Ƥ�pointer
 *       int            - size      : �������Ƥj�p
 *       SOCKADDR_IN    - cli_addr  : Client SOCKADDR_IN info
 *
 *  Return value:
 *       int            - Ret             : Recv���\�A�^�Ǫ����Ƥj�p
 *                      - WL_Recv_Err     : Recv����
 *
 *  History:
 *       Kidd Hsu      10/29/2008          Modified
 * =============================================================================
*/
int WL_NblkRecvUDPDataTimeout(int *skt, char *result, int size, SOCKADDR_IN *cli_addr, int *stop)
{
	int Ret;
#ifdef _WINDOWS
	int SenderAddrSize = sizeof(SOCKADDR_IN);
#else
#ifdef LINUX
	unsigned int SenderAddrSize = sizeof(SOCKADDR_IN);
#endif
#endif

	if (*stop)
	{
		WL_CloseSktHandle(skt);
		return WL_Stop;
	}
	if ((Ret = recvfrom(*skt, result, size, 0,
		(SOCKADDR *)cli_addr, &SenderAddrSize)) == SOCKET_ERROR)
	{
		WL_CloseSktHandle(skt);
		return WL_Recv_Err;
	}
	return Ret;
}

/*
 * =============================================================================
 *  Function name: WLFindFlageHeader
 *  : �b�YBuffer���j�M�@Token�A�æ^�Ǧ�Token�}�l�����}
 *
 *  Parameter:
 *      (INPUT)
 *       char*          - pSrc      : �n�Q�j�M��Buffer pointer
 *       int            - size      : �n�Q�j�M��Buffer size
 *       char*          - Token     : �j�M��Token
 *       int            - TokenLen  : Token Length
 *
 *  Return value:
 *       char*          - NULL      : �j�M���ѡA�L��Token���e
 *                      - &pSrc[i]  : �j�M���\�A�^��Token�Ҧb���}
 *
 *  History:
 *       Kidd Hsu      10/29/2008          Modified
 * =============================================================================
*/
char* WL_FindFlageHeader(char* pSrc,int size,char *Token,int TonkeLen)
{
	int i = 0;
	int j = 0;

	while (size >= TonkeLen) 
	{
		j = 0;
		while(j < TonkeLen)
		{
			if (pSrc[i+j] != Token[j])
			{
				break;
			}
			j++;
			if (j == TonkeLen)
				return  &pSrc[i];
		}
		
		i++;
		if (i+TonkeLen > size)
			break;
	} 

	return NULL;
}

/*
 * =============================================================================
 *  Function name: WLRecvOneByte
 *  : �@����1460 Byte���ơA�M���A�@��Byte�B�@��Byte�Ǧ^
 *
 *  Parameter:
 *      (INPUT)
 *       int            - skt       : �������Ƥ�Socket
 *       char*          - RecvBuff  : �������Ƥ�pointer
 *       int*           - BuffLen : �Ѿl���Ƥ��j�p
 *       int*           - RecvLen   : �������Ƥ��j�p
 *       int*           - stop      : �����ʧ@��Flag
 *
 *  Return value:
 *       int            - WL_Recv_OK      : �������\
 *                      - WL_Recv_Err     : ��������
 *                      - WL_Stop         : �����
 *
 *  History:
 *       Kidd Hsu      10/29/2008          Modified
 * =============================================================================
*/
int WL_RecvOneByte(int *skt, char *RecvBuff, int BuffLen, int *RecvLen, int *stop)
{
	char*	pBuff = RecvBuff;
	struct timeval timeout;
	fd_set  fdmask;
	
	if (*stop)
	{
		WL_CloseSktHandle(skt);
		return WL_Stop;
	}

	timeout.tv_sec = WL_RECV_TIMEOUT;
	timeout.tv_usec = 0;
	
	FD_ZERO(&fdmask);
	FD_SET((unsigned int) *skt,&fdmask);
	
	int rc = select(*skt+1,&fdmask,NULL,NULL,&timeout);
	if (rc<=0) 
	{
		FD_CLR(( unsigned int )*skt, &fdmask );
		return WL_Recv_Err; // 0=time-out -1=error
	}
	if (BuffLen > WL_MAX_RECVSIZE)
		BuffLen = WL_MAX_RECVSIZE; 
		
	*RecvLen = recv(*skt,pBuff,BuffLen,0);
	//TRACE("recvLen =%d\r\n",*RecvLen);
	if (*RecvLen == 0 || *RecvLen == INVALID_SOCKET) 
	{
		FD_CLR(( unsigned int )*skt, &fdmask );
		return WL_Recv_Err;
	}


	return WL_Recv_OK;
}

/*
 * =============================================================================
 *  Function name: WL_GetSearchRegion
 *  : �̾ڶǤJ�nsearch���_�l�r���P�����r���A��n�@���ϰ��^�ǡC
 *
 *  Parameter:
 *      (INPUT)
 *       int*           - skt       : �������Ƥ�Socket
 *       char**         - beginptr  : FF D8�bstreaming���_�l���}
 *       char*          - recvbuf   : �����v����pointer
 *       int            - bufsize   : �����v��buffer���j�p
 *       int*           - stop      : ���Flag
 *       int*           - framesize : �^�Ǥ@�iMJPEG���v���j�p
 *       int*           - remainsize: �ѤU�٨S���諸size
 *       int*           - tmpsize   : buffer�_�l���}��frame�����������}��size
 *       char*          - StrBeg    : �nSearch Region���_�l�r��
 *       int            - StrBegLen : �_�l�r�ꤧLength
 *       char*          - StrEnd    : �nSearch Region�������r��
 *       int            - StrEndLen : �����r�ꤧLength
 *       int            - SearchType: 0: �d���q�_�l�ܵ����e(���]�t�����r��), 
 *                                    1: �d���q�_�l�ܵ���(�]�t�����r��)
 *
 *  <��> (remainsize�Btmpbufsize�u�O�����ݭn�Ψ쪺�ѼơA�ä��ӻݭn�h�B�z)
 *
 *  Return value:
 *       int            - WL_Recv_Err   : �����s�u����
 *                      - WL_Recv_OK    : ���\�A�����@�iJPEG�v��
 *                      - WL_Stop       : �������
 *                      - WL_Buffer_Full: buffer is Full
 *
 *  History:
 *       Kidd Hsu       3/12/2008          Created
 * =============================================================================
*/
int WL_GetSearchRegion(int *skt, char **beginptr, char *recvbuf, int bufsize, int *stop, int *framesize
	, int *remainsize, int* tmpsize,char* StrBeg, int StrBegLen, char* StrEnd, int StrEndLen, int SearchType)
{
	int len=0;
	int ilen=0;
	char *sp=NULL;
	char *sp1=NULL;
	char *precvbuf = recvbuf;
	int  matchlen = StrBegLen;
	int  recvLen = 0;

	
	while(!*stop)
	{	
		if(len+1 > bufsize)
			return WL_Buffer_Full;
		
		if (!*remainsize && recvLen < matchlen)
		{
			if (len+20000 > bufsize)
				return WL_Buffer_Full;
			if (!recvLen)
				ilen = WL_RecvOneByte(skt,precvbuf,bufsize,&recvLen,stop);
			else 
			{
				int tmplen = recvLen;
				ilen = WL_RecvOneByte(skt,precvbuf+tmplen,bufsize,&recvLen,stop);
				recvLen += tmplen;
			}
		}
		else if (*remainsize && !recvLen)
		{
			recvLen = *remainsize;
			memcpy(precvbuf,precvbuf+*tmpsize,recvLen);
			*remainsize = 0;
			*tmpsize = 0;
			ilen = WL_Recv_OK;
		}
		
		if(ilen == WL_Recv_OK && recvLen > 0)
		{
			len++;	
			if(len>=matchlen) 
			{
				if(!sp)  // �QFrame�J��
				{
					sp = WL_FindFlageHeader(precvbuf,matchlen,StrBeg,matchlen);
					recvLen--;
					precvbuf++;
					len--;
					if (sp)
						matchlen = StrEndLen;
				}
				else if(!sp1) // �QFrame?��
				{
					sp1 = WL_FindFlageHeader(precvbuf,matchlen,StrEnd,matchlen);
					recvLen--;
					precvbuf++;
					len--;
					if (sp1 && recvLen)
					{
						if (!SearchType)// �����@�i�BSerachType = 0
						{
							recvLen  ++;
							precvbuf --;
						}
						else if (SearchType)
						{
							// �����@�i�BSerachType = 1
							recvLen  -= matchlen - 1;
							precvbuf += matchlen - 1;
							sp1 = precvbuf;
						}

						if (recvLen > 0)
						{
							*remainsize = recvLen;
							*tmpsize = precvbuf - recvbuf;
						}
					}
				}

				if(sp && sp1) 
				{	
					*beginptr = sp;
					*framesize = sp1-sp;	
					return WL_Recv_OK;
				}
			}
		}
		else if (ilen == WL_Stop || ilen == WL_Recv_Err)
			return ilen;	//WL_Stop or WL_Recv_Err	
		
	}
	
	return WL_Stop;
}


/*
 * =============================================================================
 *  Function name: WLGetOneMJPEGFrame
 *  : ��MJPEG Stream���ơA�ä�@�iMJPEG�Ǧ^
 *
 *  Parameter:
 *      (INPUT)
 *       int*           - skt       : �������Ƥ�Socket
 *       int*           - beginptr  : FF D8�bstreaming���_�l���}
 *       char*          - recvbuf   : �����v����pointer
 *       int            - bufsize   : �����v��buffer���j�p
 *       int*           - stop      : ���Flag
 *       int*           - framesize : �^�Ǥ@�iMJPEG���v���j�p
 *       int*           - remainsize: �ѤU�٨S���諸size
 *       int*           - tmpbufsize: buffer�_�l���}��frame�����������}��size
 *
 *  <��> (remainsize�Btmpbufsize�u�O�����ݭn�Ψ쪺�ѼơA�ä��ӻݭn�h�B�z)
 *
 *  Return value:
 *       int            - WL_Recv_Err   : �����s�u����
 *                      - WL_Recv_OK    : ���\�A�����@�iJPEG�v��
 *                      - WL_Stop       : �������
 *                      - WL_Buffer_Full: buffer is Full
 *
 *  History:
 *       Kidd Hsu      10/29/2008          Created
 *       Kidd Hsu       3/11/2008          Modified
 * =============================================================================
*/
int WL_GetOneMJPEGFrame(int *skt, int *beginptr, char *recvbuf, int bufsize, int *stop, int *framesize, int *remainsize, int* tmpbufsize)
{
	int len=0;
	int ilen=0;
	char *sp=NULL;
	char *sp1=NULL;
	char *precvbuf = recvbuf;
	int  matchlen = 2;
	static char JPEGBegin[2]  = {0xFF,0xD8};
	static char JPEGEnd[2] = {0xFF,0xD9};
	int  recvLen = 0;

	while(!*stop)
	{	
		if(len+1 > bufsize)
			return WL_Buffer_Full;
		
		if (!*remainsize && recvLen < matchlen)
		{	
			if (len+20000 > bufsize)
				return WL_Buffer_Full;
			if (!recvLen)
				ilen = WL_RecvOneByte(skt,precvbuf,bufsize,&recvLen,stop);	
			else 
			{
				ilen = WL_RecvOneByte(skt,precvbuf+1,bufsize,&recvLen,stop);	
				recvLen += 1;
			}
		}
		else if (*remainsize && !recvLen)
		{
			recvLen = *remainsize;
			memcpy(precvbuf,precvbuf+*tmpbufsize,recvLen);
			*remainsize = 0;
			*tmpbufsize = 0;
			ilen = WL_Recv_OK;
		}
		
		if(ilen == WL_Recv_OK && recvLen > 0)
		{
			len++;	
			if(len>=matchlen) 
			{
				if(!sp)  // �QFrame�J��
				{
					sp = WL_FindFlageHeader(recvbuf+len-matchlen,matchlen,JPEGBegin,matchlen);
					recvLen--;
					precvbuf++;
				}
				else if(!sp1) // �QFrame?��
				{
					sp1 = WL_FindFlageHeader(recvbuf+len-matchlen,matchlen,JPEGEnd,matchlen);
					recvLen--;
					precvbuf++;
					if (sp1 && recvLen)
					{
						recvLen--;
						precvbuf ++;
						if (recvLen)
						{
							*remainsize = recvLen;
							*tmpbufsize = precvbuf - recvbuf;
						}
					}
				}

				if(sp && sp1) 
				{	
					*beginptr = sp-recvbuf;
					*framesize = sp1-sp+matchlen;		
					return WL_Recv_OK;
				}
			}
		}
		else if (ilen == WL_Stop || ilen == WL_Recv_Err)
			return ilen;	//WL_Stop or WL_Recv_Err	
	}
	
	return WL_Stop;
}

/*
 * =============================================================================
 *  Function name: WL_GetOneMPEG4Frame
 *  : ��MPEG4 Stream���ơA�ä�^�@�iFrame�Ǧ^
 *
 *  Parameter:
 *      (INPUT)
 *       int*           - skt       : �������Ƥ�Socket
 *       char**         - beginptr  : MPEG4 header�bstreaming���_�l���}
 *       char*          - recvbuf   : �����v����pointer
 *       int            - bufsize   : �����v��buffer���j�p
 *       int*           - stop      : ���Flag
 *       int*           - framesize : �^�Ǥ@�iMPEG4���v���j�p
 *       int*           - remainsize: �ѤU�٨S���諸size
 *       int*           - tmpbufsize: buffer�_�l���}��frame�����������}��size
 *
 *  <��> (remainsize�Btmpbufsize�u�O�����ݭn�Ψ쪺�ѼơA�ä��ӻݭn�h�B�z)
 *
 *  Return value:
 *       int            - WL_Recv_Err   : �����s�u����
 *                      - WL_Recv_OK    : ���\�A�����@�iJPEG�v��
 *                      - WL_Stop       : �������
 *                      - WL_Buffer_Full: buffer is Full
 *
 *  History:
 *       Kidd Hsu      3/11/2009          Created
 * =============================================================================
*/
int WL_GetOneMPEG4Frame(int *skt, char **beginptr, char *recvbuf, int bufsize, int *stop, int *framesize, int *remainsize, int* tmpbufsize)
{
	int len=0;
	int ilen=0;
	char *sp=NULL;
	char *sp1=NULL;
	char *precvbuf = recvbuf;
	int  matchlen = 4;
	int  recvLen = 0;
	unsigned char strBegI[8] = {0x00,0x00,0x01,0xB0};
	unsigned char strBegP[8] = {0x00,0x00,0x01,0xB6};
	int  header_B0 = 0;
	int  header_B6 = 0;
	int  isFrame = 0;
	
	while(!*stop)
	{	
		
		if (!*remainsize && recvLen < matchlen)
		{
			if(precvbuf-recvbuf+WL_MAX_RECVSIZE > bufsize)
				return WL_Buffer_Full;
			precvbuf += recvLen;
			ilen = WL_RecvOneByte(skt,precvbuf,bufsize,&recvLen,stop);
		}
		else if (*remainsize && !recvLen)
		{
			recvLen = *remainsize;
			memcpy(precvbuf,precvbuf+*tmpbufsize,recvLen);
			*remainsize = 0;
			*tmpbufsize = 0;
			ilen = WL_Recv_OK;
		}
		
		if(ilen == WL_Recv_OK && recvLen > 0)
		{
			len++;	
			if (len >= matchlen)
			{
				if (!sp)
				{
					// search 000001B0
					sp = WL_FindFlageHeader(precvbuf,len,(char*)strBegI,matchlen);
					if (!sp)
					{
						// search 000001B6
						sp = WL_FindFlageHeader(precvbuf,len,(char*)strBegP,matchlen);
						if (sp)
							header_B6++;
					}
					else 
						header_B0++;

					recvLen--;
					precvbuf++;
					len--;
				}
				else
				{
					// search 000001B0
					sp1 = WL_FindFlageHeader(precvbuf,len,(char*)strBegI,matchlen);
					if (!sp1)
					{
						// search 000001B6
						sp1 = WL_FindFlageHeader(precvbuf,len,(char*)strBegP,matchlen);
						if (sp1)
						{
							if (header_B6 == 1 && !header_B0) // P+P
								isFrame = TRUE;
							else if (header_B6 == 1 && header_B0 == 1) // I+P
								isFrame = TRUE;
							header_B6++;
						}
					}
					else 
					{
						if (header_B6 == 1 && !header_B0) // P+I
							isFrame = TRUE;
						else if (header_B6 == 1 && header_B0 == 1) // I+I
							isFrame = TRUE;
						else if (header_B0 == 1 || !header_B6) // ���X�{����000001B0�A�o�S��000001B6�A�O�����D��
							isFrame = TRUE;
						header_B0++;
					}

					if (isFrame && recvLen) // �����@�iFrame
					{
						*remainsize = recvLen;
						*tmpbufsize = precvbuf - recvbuf;
					}
					else 
					{
						recvLen--;
						precvbuf++;
						len--;
					}
				}
				if(isFrame) 
				{	
					*beginptr = sp;
					*framesize = sp1-sp;	
					return WL_Recv_OK;
				}
				else if (header_B0 > 2 || header_B6 > 2)
					return WL_Recv_Err;
			}
		}
		else
			return ilen;	//WL_Stop or WL_Recv_Err
		
	}
	
	return WL_Stop;
}

/*
 * =============================================================================
 *  Function name: WL_SendHttpCommand
 *  : �B�z�@��socket()->connect->send()->recv()���ʧ@�A�Ω�Send�@��URL Command
 *
 *  Parameter:
 *      (INPUT)
 *       int            - *sockfd   : �������Ƥ�Socket
 *       Url_Command*   - url       : �]�turl command/ip/port�����T
 *       char*          - netbuf    : �Ω�send/recv��buffer�A�̫��^�Ǹ��ƩҦb
 *       int            - bufsize   : �����v��buffer���j�p
 *       int*           - stop      : ���Flag
 *
 *  Return value:
 *       int            - WL_Recv_Err   : �����s�u����
 *                      - WL_Recv_OK    : ���\�A����url command���^�ǭ�
 *                      - WL_Stop       : �������
 *                      - WL_Buffer_Full: buffer is Full
 *
 *  History:
 *       Catter Wang    11/21/2008          Modified
 * =============================================================================
*/
int WL_SendHttpCommand( int *sockfd, WL_Url_Command *url, char *netbuf, int bufsize, int *stop )
{
	int		recv_size;
	char	*p_netbuf;

	//�ǳ�http request����
	memset( netbuf, 0, bufsize );
	p_netbuf = netbuf;

	//HTTP Request
	sprintf( p_netbuf, "%s %s HTTP/1.1\r\n",  url->HttpMethod, url->Command );
	p_netbuf = netbuf + strlen(netbuf);
	
	//Host
	sprintf( p_netbuf, "Host: %s:%d\r\n", url->ipaddress, url->port );
	p_netbuf = netbuf + strlen(netbuf);

	if( strlen(url->Cookie) > 0 )
	{
		sprintf( p_netbuf, "Cookie: %s\r\n", url->Cookie );
		p_netbuf = netbuf + strlen(netbuf);
	}

	//Auth Basic
	if( strlen(url->AuthBasic) > 0 )
	{
		sprintf( p_netbuf, "Authorization: Basic %s\r\n", url->AuthBasic );
		p_netbuf = netbuf + strlen(netbuf);
	}

	//Post Data
	if( strlen(url->HttpData) > 0 )
	{
		sprintf( p_netbuf, "Content-Length: %d\r\n", strlen(url->HttpData) );
		p_netbuf = netbuf + strlen(netbuf);
	}
	
	//HTTP����
	sprintf( p_netbuf, "\r\n" );
	p_netbuf = netbuf + strlen(netbuf);
	
	//POST Data
	if( strlen(url->HttpData) > 0 )
	{
		sprintf( p_netbuf, "%s", url->HttpData );
	}
	
	//socket() & connect()
	if( WL_NblkTCPConnect( sockfd, url->ipaddress, url->port, 0, stop ) != WL_Connect_OK )
	{
		return WL_Connect_Err;
	}

	//�e�XRequest
	if( WL_SendAllData( sockfd, netbuf, strlen(netbuf), stop ) <= 0 ) {
		WL_CloseSktHandle( sockfd );
		return WL_Recv_Err;
	}
	
	memset( netbuf, 0, bufsize );
	p_netbuf = netbuf;
	while( !*stop )	//�����^�Ǫ�����
	{
		recv_size = recv( *sockfd, p_netbuf, bufsize, 0 );
		//���P�_�O�_����http 200�A�ѩI�s��function�ۦ��P�_
		if( recv_size == 0 ) {
			WL_CloseSktHandle( sockfd );
			return WL_Recv_OK;
		}

		if( recv_size < 0 ) {
			WL_CloseSktHandle( sockfd );
			return WL_Recv_Err;
		}

		p_netbuf += recv_size;
		bufsize -= recv_size;	//�קK����D�r���������^?20080717 catter

		if( bufsize <= 0 ) {
			WL_CloseSktHandle( sockfd );
			return WL_Buffer_Full;
		}
	}

	WL_CloseSktHandle( sockfd );
	return WL_Stop;
}


// Server Site API
//#define MY_SOCK_PATH "./localSkt"

int LS_TCPListenServer(int *sockfd, const char *LocalSockPath /*INADDR_ANY */)
{
    int listenfd = 0, connfd = 0;
    struct sockaddr_un serv_addr; 
	
#ifdef _WINDOWS
		return Server_Listen_Err;
#else
    *sockfd = socket(AF_UNIX, SOCK_STREAM, 0);
	if (*sockfd == -1)
		return Server_Skt_Err;

    memset(&serv_addr, '0', sizeof(serv_addr));
	
    serv_addr.sun_family = AF_UNIX;
	strncpy(serv_addr.sun_path, LocalSockPath, sizeof(serv_addr.sun_path) -1 );
	unlink(serv_addr.sun_path);

    if( bind(*sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) == -1 )
		return Server_Bind_Err;

	if( listen(*sockfd, 10) == -1 )
		return Server_Listen_Err;

	return Server_Listen_OK;
#endif
}

int Accept( int *skt, struct sockaddr *addr,socklen_t *addrlen)
{
	return accept(*skt, addr, addrlen );
}


/*
 * =============================================================================
 *  Function name: WLNblkTCPConnect
 *  : nonblocking TCP Connect
 *
 *  Parameter:
 *      (INPUT)
 *       int*           - skt       : TCP's socket handle
 *       const char*          - localSockPath        : Local Socket Path
 *       int            - wait_time : 
 *       int*           - stop      : flage
 *
 *  Return value:
 *       int            - WL_Connect_OK   : �s�u���\
 *                      - WL_Connect_Err  : �����s�u���~
 *                      - WL_Stop         : �����s�u
 *
 *  History:
 *       Kidd Hsu      10/29/2008          Modified
 * =============================================================================
 */
int LS_NblkTCPConnect(int *skt, const char *localSockPath, int wait_time, int *stop)
{
#ifdef _WINDOWS
	return WL_Connect_Err;
#else
	if (*skt != INVALID_SOCKET)
		WL_CloseSktHandle(skt);

	struct sockaddr_un   server_addr;
	int 	flag = 1;
	int		ipaddress;
	int		retrytimes;
	int    len = 0;
	struct	timeval TimeOut;
	TimeOut.tv_sec  = 20;
	TimeOut.tv_usec = 0;


	if( wait_time == 0 ) {
		retrytimes = WL_RECV_TIMEOUT * 10;	//time out 100ms
	} else {
		retrytimes = wait_time * 10;
	}

	if( localSockPath == NULL ) {
		return WL_Connect_Err;
	} else if( *stop ) {
		return WL_Stop;
	}

	
	*skt = socket(AF_UNIX,SOCK_STREAM,0);
	if( *skt != INVALID_SOCKET )
	{
		server_addr.sun_family      = AF_UNIX;	
		strcpy(server_addr.sun_path, localSockPath );
		len = strlen(server_addr.sun_path) + sizeof(server_addr.sun_family);


		flag = 1;
		ioctl(*skt, FIONBIO, &flag);	//�]�wnon-block mode
		do
		{
			if( *stop ) {
				WL_CloseSktHandle(skt);
				return WL_Stop;
			}
			if( connect(*skt, (struct sockaddr *)&server_addr, len) >= 0 )
			{
				flag = 0;
				ioctl(*skt, FIONBIO, &flag);	//set socket to block mode
				
				if( setsockopt( *skt, SOL_SOCKET, SO_RCVTIMEO, (char*)&TimeOut, sizeof(TimeOut) ) < 0)
				{
					//PRINTF("\n<**IPCAM**> %s setsockopt failed\n\n", __FUNCTION__);
					WL_CloseSktHandle(skt);
					return WL_Connect_Err;
				}

				return WL_Connect_OK;
			}
			usleep( 100 * 1000 );
		} while ( retrytimes-- > 1 );
	}

	WL_CloseSktHandle(skt);
	return WL_Connect_Err;
#endif
}
