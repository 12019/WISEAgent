/****************************************************************************/
/* Copyright(C) : Advantech Technologies, Inc.														 */
/* Create Date  : 2015/03/08 by Eric Liang															     */
/* Modified Date: 2015/07/02 by Eric Liang															 */
/* Abstract       :  SENSOR NETWORK API     													             */
/* Reference    : None																									 */
/****************************************************************************/
#ifndef  __SENSOR_NETWORK_API_H__
#define __SENSOR_NETWORK_API_H__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once


#ifndef SNAPI
	#define SNAPI __stdcall
#endif
#else
	#define SNAPI
#endif

#ifdef __cplusplus
extern "C" {
#endif

#include "SensorNetwork_BaseDef.h"

#define MAX_SN_INF_NAME               16
#define MAX_SN_INF_ID                      64
#define MAX_SN_INF_NUM                  8
#define MAX_SN_COM_NAME           32
#define MAX_SN_UID                           64  // Max Sensor Network Uinque ID
#define MAX_SN_HOSTNAME            32
#define MAX_SN_SNO                         32 
#define MAX_SN_PRODUCT_NAME  32


//-----------------------------------------------------------------------------
// Base Sen Net Driver API Structure
//-----------------------------------------------------------------------------
typedef struct _InBaseData
{
		char					   *psType;   // 64      ( reference size )
		int					 iSizeType;   // Buffer size of sbn
		char                    *psData;   // 2048 ( reference size )
		int					 iSizeData;   // Buffer size of sData
		void                 *pExtend;   // Extend	

}InBaseData;
	
typedef struct _OutBaseData
{
		char					   *psType;   // 64       ( reference size )
		int					 iSizeType;   // Buffer size of sbn
		char                    *psData;   // 2048   ( reference size ) => If the data length is more than buffer size, user can reallocate this buffer point and reasign the iSizeData.
		int				    *iSizeData;   // Buffer size of sData
		void                  *pExtend;   // Extend

}OutBaseData;


typedef struct _InDataClass
{
		int						       iTypeCount;  // Number of Type
		InBaseData  *pInBaseDataArray;  // Point of first BaseInData Array
		void				                 *pExtened;  // Extend

}InDataClass;

typedef struct _OutDataClass
{
		int					                  iTypeCount;  // Number of Type
		OutBaseData   *pOutBaseDataArray;  // Point of the first BaseOutData Array
		void						   				*pExtened;  // Extend

}OutDataClass;   
	

//-----------------------------------------------------------------------------
// For updating Sensor Hub Data Structure
//-----------------------------------------------------------------------------
// SN_EVENT_ID: SN_SenHub_Register ( 2000 )
typedef struct _SenHubInfo
{
	char                                 sUID[MAX_SN_UID];   // MAC: remove '-' and ':')
	char      sHostName[MAX_SN_HOSTNAME];   // WISE1020-MAC(4)
	char                                 sSN[MAX_SN_SNO];   // Sensor Hub Serial Number
	char  sProduct[MAX_SN_PRODUCT_NAME];  // WISE1020
	void												      *pExtened;		 	
}SenHubInfo;

typedef struct _InSenData
{
		char                sUID[MAX_SN_UID];   // (MAC: remove '-' and ':')
		InDataClass                inDataClass;   // psType: SenData, psData: {"n": "room temp","v": 26}, 	{"n": "mcu temp","v": 42}			
                                                                     // psType: Net, psData: {"n":"Health","v":80,"asm":"r"}
		void					   		       *pExtened;		 	

}InSenData;

typedef struct _OutSenData
{
	char                  sUID[MAX_SN_UID];	// (Unique ID: remove '-' and ':')
	OutDataClass            outDataClass;   // psType: SenData, psData: {"n": "room temp","v": 26}, 	{"n": "mcu temp","v": 42}						
	void								      *pExtened;		 	

}OutSenData;	


//-----------------------------------------------------------------------------
// Interface Data Structure
//-----------------------------------------------------------------------------

// For Get GetCapability
typedef struct _SNInfInfo
{
	char		     sInfName[MAX_SN_INF_NAME];  // Interface Name
	char                        sInfID[MAX_SN_INF_ID];  // Unique ID for identify (ex: 0000+MAC: remove '-' and ':' )
	OutDataClass					     outDataClass;  // iTypeCount = 1,   psType: Info, psData: {"n":"SenNodeList","sv":"","asm":"r"},{"n":"Topology","sv":"","asm":"r"},{"n":"Health","v":-1,"asm":"r"},{"n":"Name","sv":"WSN0","asm":"r"},\
																																						   {"n":"sw","sv":"1.0.0.1","asm":"r"},{"n":"reset","bv":0,"asm":"rw"},{"n":"joint_key","sv":"xxx","asm":"rw"},{"n":"net_id","v":"9158","asm":"rw"}       
	void                                               *pExtened;

}SNInfterfaceInfo,SNInfInfo;

// For Get GetCapability
typedef struct _SNInfInfos
{
	char              sComType[MAX_SN_COM_NAME];  // Communication Type Name
	int                                                                    iNum;  // Number of Sensor Network Interface
	SNInfInfo                 SNInfs[MAX_SN_INF_NUM];  // Detail Information of Sensor Network Interface

}SNInterfaceInfos,SNInfInfos,SNMultiInfInfo;

// For Update Interface's InfoSpec
typedef struct _SNInterfaceSpec
{
	char		     sInfName[MAX_SN_INF_NAME];  // Interface Name
	char                        sInfID[MAX_SN_INF_ID];  // Unique ID for identify (ex: 0000+MAC: remove '-' and ':' )
	InDataClass					            inDataClass;  // iTypeCount = 1,   psType: Info, psData: {"n":"SenNodeList","sv":"","asm":"r"},{"n":"Topology","sv":"","asm":"r"},{"n":"Health","v":-1,"asm":"r"},{"n":"Name","sv":"WSN0","asm":"r"},\
																																						   {"n":"sw","sv":"1.0.0.1","asm":"r"},{"n":"reset","bv":0,"asm":"rw"}
	void                                               *pExtened;

}SNInterfaceSpec,SNInfSpec;


// For Update Interface's InfoSpec
typedef struct _SNMultiInterfaceInfoSpec
{
	char              sComType[MAX_SN_COM_NAME];  // Communication Type Name
	int                                                                    iNum;  // Number of Sensor Network Interface
	SNInfSpec       aSNInfSpec[MAX_SN_INF_NUM];  // Detail Information of Sensor Network Interface

}SNInterfaceInfoSpec,SNMultiInfInfoSpec;

// For updating Sensor Network Interface Data Value
typedef struct _SNInfData
{
		char						sComType[MAX_SN_COM_NAME];  // WSN
		char										   sInfID[MAX_SN_INF_ID];  // Unique ID for identify (ex: 0000+MAC: remove '-' and ':' )
		InDataClass											    inDataClass;  // psType: Info, psData : {"n":"SenHubList","sv": "0000000EC6F0F831,0000000EC6F0F830,0000000EC6F0F832"},{"n": "Topology","sv": "0-1,0-3,1-2,1-3"},{"n": "Health","v": 90},{"n":"sw","sv":"1.0.0.1"},{"n":"reset","bv":0},{"n":"joint_key","sv":"xxxx"},{"n":"net_id","v":9158}
		void																   *pExtened;

}SNInterfaceData,_SNInfData;  


//-----------------------------------------------------------------------------
//  Callback Function 
//-----------------------------------------------------------------------------
typedef SN_CODE  (*UpdateSNDataCbf) ( const int cmdId, const void *pInData, const int InDatalen, void *pUserData, void *pOutParam, void *pRev1, void *pRev2 ); 


//-----------------------------------------------------------------------------
// Sensor Netowrk Function Define
//-----------------------------------------------------------------------------
SN_CODE SNAPI SN_Initialize( void *pInUserData ); 

SN_CODE SNAPI SN_Uninitialize( void *pInParam );

SN_CODE SNAPI SN_GetVersion( char *psVersion, int BufLen );

SN_CODE SNAPI SN_GetCapability( SNMultiInfInfo *pOutSNMultiInfInfo );

void SNAPI SN_SetUpdateDataCbf( UpdateSNDataCbf pUpdateSNDataCbf );

SN_CODE SNAPI SN_GetData( SN_CTL_ID CmdID, InSenData *pInSenData, OutSenData *pOutSenData );

SN_CODE SNAPI SN_SetData( SN_CTL_ID CmdID, InSenData *pInSenData, OutSenData *pOutSenData );



//-----------------------------------------------------------------------------
// Dynamic Library Function Point 
//-----------------------------------------------------------------------------

int GetSNAPILibFn( const char *LibPath, void **pFunInfo );

typedef SN_CODE (SNAPI *SN_Initialize_API )					( void *pInUserData );
typedef SN_CODE (SNAPI *SN_Uninitialize_API )				( void *pInParam );
typedef SN_CODE (SNAPI *SN_GetVersion_API )              ( char *psVersion, int BufLen );
typedef SN_CODE (SNAPI *SN_SetUpdateDataCbf_API )	( UpdateSNDataCbf pUpdateDataCbf );
typedef SN_CODE (SNAPI *SN_ActionProc_API )				( const int cmdId, void *pParam1, void *pParam2, void *pRev1, void *pRev2 );
typedef SN_CODE (SNAPI *SN_GetCapability_API )			( SNInfInfos *pOutSNInfInfo );
typedef SN_CODE (SNAPI *SN_GetData_API )					( SN_CTL_ID CmdID, InSenData *pInSenData, OutSenData *pOutSenData );
typedef SN_CODE (SNAPI *SN_SetData_API )					( SN_CTL_ID CmdID, InSenData *pInSenData, OutSenData *pOutSenData	);

typedef struct SNAPI_INTERFACE
{
	void*																		Handler;                              
	SN_Initialize_API											 SN_Initialize;  
	SN_Uninitialize_API									SN_Uninitialize;
	SN_GetVersion_API									SN_GetVersion;
    SN_ActionProc_API									 SN_ActionProc;
	SN_SetUpdateDataCbf_API	   SN_SetUpdateDataCbf;
	SN_GetCapability_API							SN_GetCapability;
	SN_GetData_API											  SN_GetData;
	SN_SetData_API											  SN_SetData;
	int    																	  Workable;
}SNAPI_Interface;

extern SNAPI_Interface    *pSNAPI;
#ifdef __cplusplus
}
#endif

#endif // __SENSOR_NETWORK_API_H__


