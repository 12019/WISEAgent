/****************************************************************************/
/* Copyright(C) : Advantech Technologies, Inc.														 */
/* Create Date  : 2015/03/06 by Eric Liang															     */
/* Modified Date: 2015/03/06 by Eric Liang															 */
/* Abstract     : Sensor Network API                       													*/
/* Reference    : None																									 */
/****************************************************************************/
#include <stdio.h>
#include <common.h>
#include <platform.h>


#include "inc/SensorNetwork_Manager_API.h"
#include "BasicFun_Tool.h"

// Global Variable

SNManagerAPI_Interface           g_SNManagerAPI_Interface;
SNManagerAPI_Interface           *pSNManagerAPI = NULL ;
//-----------------------------------------------------------------------------
// function:
//-----------------------------------------------------------------------------
//


int LoadcSNManagerAPILib( const char *path, SNManagerAPI_Interface *pAPI_Loader )
{
	pAPI_Loader->Handler    = OpenLib( path );

	if( pAPI_Loader->Handler )  {
		pAPI_Loader->SN_Manager_Initialize			     =  ( SN_Manager_Initialize_API ) GetLibFnAddress( pAPI_Loader->Handler, "SN_Manager_Initialize");
		pAPI_Loader->SN_Manager_Uninitialize			 =  ( SN_Manager_Initialize_API ) GetLibFnAddress( pAPI_Loader->Handler, "SN_Manager_Uninitialize");
		pAPI_Loader->SN_Manager_ActionProc          =  ( SN_Manager_ActionProc_API ) GetLibFnAddress( pAPI_Loader->Handler, "SN_Manager_ActionProc");
		pAPI_Loader->SN_Manager_GetCapability     =  ( SN_Manager_GetCapability_API ) GetLibFnAddress( pAPI_Loader->Handler, "SN_Manager_GetCapability");
	    pAPI_Loader->SN_Manager_GetData               =  ( SN_Manager_GetData_API ) GetLibFnAddress( pAPI_Loader->Handler, "SN_Manager_GetData");
        pAPI_Loader->SN_Manager_SetData                =  ( SN_Manager_SetData_API ) GetLibFnAddress( pAPI_Loader->Handler, "SN_Manager_SetData");
	}

	if( pAPI_Loader->Handler != NULL && pAPI_Loader->SN_Manager_Initialize != NULL ) {
		PRINTF("Load Lib=%s Successful\r\n",path );
		pAPI_Loader->Workable = 1;
		return 1;
	}

	return 0;
}


int GetSNManagerAPILibFn( const char *LibPath, void **pFunInfo )
{
	int Ret = 0;
	if( g_SNManagerAPI_Interface.Workable == 1 ) {
		*pFunInfo = (void*)&g_SNManagerAPI_Interface;
		Ret = 1;
	} else if( 1 == LoadcSNManagerAPILib( LibPath, &g_SNManagerAPI_Interface ) ) {
		*pFunInfo = (void*)&g_SNManagerAPI_Interface;
		Ret = 1;
	}
	return Ret;
}
