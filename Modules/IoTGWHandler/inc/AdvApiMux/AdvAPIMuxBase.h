/****************************************************************************/
/* Copyright(C) : Advantech Technologies, Inc.														 */
/* Create Date  : 2015/10/10 by Eric Liang															     */
/* Modified Date: 2015/10/10 by Eric Liang															 */
/* Abstract       :  Adv API Mux Base			      													          */
/* Reference    : None																									 */
/****************************************************************************/
#ifndef  __ADV_API_MUX_BASE_H__
#define __ADV_API_MUX_BASE_H__

#include "LoadAPIs.h"

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once

#ifndef SNCALL
#define SNCALL __stdcall
#endif

#ifndef EXPORT
#define EXPORT __declspec(dllexport)
#endif

#else
#ifndef SNCALL
#define SNCALL
#endif

#ifndef EXPORT
#define EXPORT
#endif
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define API_MUX_VER	1

#define API_MUX_HEADER_SIZE 8  // 8 Bytes => ApiMuxHeaser
#define API_MUX_FN_HEADER_SIZE  3 // 3 Bytes 
#define API_MUX_PARAM_HEADER_SIZE 3 

#define API_MUX_MAX_FUNCTION_NAME  128

#define API_MUX_SERVER_NAME_SIZE 256
#define API_MUX_LIB_PATH_SIZE 256

#define MAX_ALIVE_TIMEOUT 20  // If skt is not geting any data -> blocking in recv and Close Socket & re-connect
#define DEF_ADVAPIMUX_LOCAL_SOCKET_PATH "./AdvAPIMux_SOCKET_PATH"


#define REPLY_FAIL "{\"StatusCode\":500, \"Result\":{\"sv\":\"Fail\"}}"
#define REQUEST_TIMEOUT  60  // sec

#define ASYNC_MAGIC_KEY "APIMUX_ASYNC"
#define ASYNC_TIMEOUT   50  // 50 sec

// Config
#define APIMUX_CONFIG_FILE "AdvApiMux.ini"
#define SERVER_INFO "Server"
#define SERVER_MODE_NAME "Mode"
#define SERVER_NAME "ServerName"
#define LIB_INFO "Library"
#define LIB_PATH "FilePath"


// 0:LocalSocket, 1:Socket
typedef enum _SERVER_MODE
{
	MODE_LocalSocket = 0,
	MODE_Socket = 1,

}SERVER_MODE;

typedef enum {														
	APIMUX_ER_FAILED									=   -1,		/*		Failed														(500)		*/
	APIMUX_OK												=    0,		/*		Success													(200)		*/
	APIMUX_INITILIZED									=    1,		/*		Library had initilized											*/
} APIMUX_CODE;

typedef struct _ApiMuxConfig
{
	int Mode; // 0:LocalSocket, 1:LocalHost, 2:Socket
	char ServerName[API_MUX_SERVER_NAME_SIZE];
	int	 ServerPort;
	char LibPath[API_MUX_LIB_PATH_SIZE];	
}ApiMuxConfig;

// 64 (8) = 4 + 4 + 8 + 16 + 32
typedef struct _ApiMuxHeader
{
	unsigned int ver : 4;         // Version of AdvAPIMux Header
	unsigned int type: 4;       // Type of call: 0: general api, 1: register a callback function ( and creat a socket channel )    /  8: return result, 9: recall by callback
	unsigned int param: 8;   // Number of Params: 0 ~ 255
	unsigned short id;    // Identify of call: 0 ~ 65535
	unsigned int len;            // Total len of each calling ( not including header size )
} ApiMuxHeader;

// 24 = 16 + 8
typedef struct _ApiFnHeader
{
	unsigned short len;  // len of function name:  1 ~ 65535
	unsigned int   ret:8;  //  return? 0: no return, 1: need return: 
}ApiFnHeader;

// 24 = 16 + 8
typedef struct _ApiParamHeader
{
	unsigned short len;       // Length of Param: 0 ~ 65535
	unsigned int type : 8;    //  Type of Param: 0: In, 1:Out, 2:In/Out
}ApiParamHeader;

typedef struct _API_MUX_FUNCTION
{
	int			Index;
	void       *pFn;
}API_MUX_FUNCTION;

typedef struct _FnStruct
{
	int num;   // num of param
	int callid;
	int nIndex;	
	void *pFn;
	void *Paras[MAX_PAS];
	int ParaLen[MAX_PAS];
}FnStruct;

typedef struct _CallApiMuxParams
{
	char		FnName[API_MUX_MAX_FUNCTION_NAME];
	int		num;
	void	  *pParams[MAX_PAS];
	int		ParamLen[MAX_PAS];
}CallApiMuxParams;

// AsyncData
typedef struct _API_MUX_ASYNC_DATA
{
	int callid;
	char magickey[16];
	int nDone;
	char buf[MAX_SEND_BUF];
	int bufsize;
}API_MUX_ASYNC_DATA;

#endif // __ADV_API_MUX_BASE_H__


