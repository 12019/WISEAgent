/****************************************************************************/
/* Copyright(C) : Advantech Technologies, Inc.														 */
/* Create Date  : 2015/03/05 by Eric Liang															     */
/* Modified Date: 2015/03/05 by Eric Liang															 */
/* Abstract       :  Sensor Network Manager API											             */
/* Reference    : None																									 */
/****************************************************************************/
#ifndef  __SENSOR_NETWORK_MANAGER_API_H__
#define __SENSOR_NETWORK_MANAGER_API_H__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once

#ifndef SNManagerAPI
	#define SNManagerAPI __stdcall
#endif
#else
	#define SNManagerAPI
#endif

#ifdef __cplusplus
extern "C" {
#endif

#include "SensorNetwork_BaseDef.h"

typedef SN_CODE  (SNManagerAPI *ReportSNManagerDataCbf) ( const int cmdId, const char *pInJson, const int InDatalen, void **pOutParam, void *pRev1, void *pRev2 ); 

SN_CODE SNManagerAPI SN_Manager_Initialize(void);

SN_CODE SNManagerAPI SN_Manager_Uninitialize(void);

SN_CODE SNManagerAPI SN_Manager_Start(void);

SN_CODE SNManagerAPI SN_Manager_Stop(void);

SN_CODE SNManagerAPI SN_Manager_ActionProc( const int cmdId, ReportSNManagerDataCbf pParam1, void *pParam2, void *pRev1, void *pRev2 ); 

//SN_CODE SNManagerAPI SN_Manager_GetCapability(char **ppzOutDataBuffer, int *pnLen );
char* SNManagerAPI  SN_Manager_GetCapability( );

// pszInUriPath:  IoTGW/WSN/Interface'MAC/Info/SenHubList
//							 SenHub'ID/SenHub/SenData/door temp
//                          SenHub'ID/SenHub/Info/Name
//			   Return:  { "StatusCode":  200, "Result": { "n":"Temp", "v":26 }}  in JSON Format
char* SNManagerAPI  SN_Manager_GetData( const char *pszInUriPath /*URI of Resource*/, const ACTION_MODE mode /* Cache or Direct */ );

// pszInUriPath:  SenHub'ID/SenHub/SenData/dout
// pszInValue   " {"bv":1} in JSON Format
// Return: { "StatusCode":  200, "Result": "Success"}  in JSON Format
char* SNManagerAPI  SN_Manager_SetData( const char *pszInUriPath /*URI of Resource*/, const char *pszInValue, const void *pUserData );

//-----------------------------------------------------------------------------
// Dynamic Library Function Point 
//----------------------------------------------------------------------------- 

int GetSNManagerAPILibFn( const char *LibPath, void **pFunInfo );

typedef SN_CODE (SNManagerAPI *SN_Manager_Initialize_API)           ( void );
typedef SN_CODE (SNManagerAPI *SN_Manager_Uninitialize_API)      ( void );
typedef SN_CODE (SNManagerAPI *SN_Manager_ActionProc_API)       ( const int cmdId, ReportSNManagerDataCbf pParam1, void *pParam2, void *pRev1, void *pRev2 );
//typedef SN_CODE (*SN_Manager_GetCapability_API)  (char **ppzOutDataBuffer, int *pnLen);
typedef char* (SNManagerAPI *SN_Manager_GetCapability_API)           ( );
typedef char* (SNManagerAPI *SN_Manager_GetData_API)					 ( const char *pszInUriPath /*URI of Resource*/, const ACTION_MODE mode /* Cache or Direct */);
typedef char* (SNManagerAPI *SN_Manager_SetData_API)					     ( const char *pszInUriPath /*URI of Resource*/,  const char *pszInValue, const void *pUserData );

typedef struct SNMANAGERAPI_INTERFACE
{
	void*													Handler;                              
	SN_Manager_Initialize_API				SN_Manager_Initialize;  
	SN_Manager_Uninitialize_API		SN_Manager_Uninitialize;
    SN_Manager_ActionProc_API			SN_Manager_ActionProc;
	SN_Manager_GetCapability_API	SN_Manager_GetCapability;
	SN_Manager_GetData_API             SN_Manager_GetData;
    SN_Manager_SetData_API             SN_Manager_SetData;
	int    													Workable;
}SNManagerAPI_Interface;

extern SNManagerAPI_Interface    *pSNManagerAPI;

#ifdef __cplusplus
}
#endif

#endif // __SENSOR_NETWORK_MANAGER_API_H__


