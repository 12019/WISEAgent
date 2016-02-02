/****************************************************************************/
/* Copyright(C) : Advantech Technologies, Inc.														 */
/* Create Date  : 2015/10/10 by Eric Liang															     */
/* Modified Date: 2015/10/10 by Eric Liang															 */
/* Abstract       :  Adv API Mux Server            													          */
/* Reference    : None																									 */
/****************************************************************************/
#ifndef  __ADV_API_MUX_SERVER_H__
#define __ADV_API_MUX_SERVER_H__

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

#ifdef _WIN32
#ifdef _MSC_VER
#pragma comment(lib, "ws2_32.lib")    // Linking with winsock library
#endif
typedef SOCKET sock_t;
#else
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
typedef int sock_t;
#endif
// External Function
#include "AdvAPIMuxBase.h"

// Server Mode
APIMUX_CODE    SNCALL    InitAdvAPIMux_Server();
void							SNCALL	  UnInitAdvAPIMux_Server();

// Client Mode
APIMUX_CODE	SNCALL		InitAdvAPIMux_Client();
void						SNCALL		UnInitAdvAPIMux_Client();
int SNCALL SendApiMuxRPCOnce( int *skt, char *sendbuf, const int bufsize, char *recvbuf, const int recvbufsize, CallApiMuxParams *pApiMuxParams, int timeout, int *Stop );

#endif // __ADV_API_MUX_SERVER_H__


