/****************************************************************************/
/* Copyright(C) : Advantech Technologies, Inc.														 */
/* Create Date  : 2015/03/05 by Eric Liang															     */
/* Modified Date: 2015/03/05 by Eric Liang															 */
/* Abstract       :  Sensor Network Base Definition		     							             */
/* Reference    : None																									 */
/****************************************************************************/
#ifndef  __SENSOR_NETWORK_BASE_DEF_H__
#define __SENSOR_NETWORK_BASE_DEF_H__

#define MAX_NAME 32

typedef enum {
	CACHE_MODE = 0, // Default
	DIRECT_MODE = 1, // Direct to get from Mote
}ACTION_MODE;

typedef enum {
	// Interface: 1000 ~ 1999
	SN_Inf_UpdateInterface_Data   = 1000,  // Update Interface data from Sensor Network Library  

	// Sensor Node: 2000 ~ 2999
	SN_SenHub_Register               = 2000,
	SN_SenHub_SendInfoSpec     = 2001,
	SN_SenHub_AutoReportData = 2002,
	SN_SenHub_Disconnect          = 2003,

	SN_SetResult							 = 3000,
} SN_EVENT_ID;

typedef enum {
		SN_Set_ReportSNManagerDataCbf  = 3000,

		// Get / Set
		SN_Inf_Get_Info									= 6000,
		SN_Inf_Set_Info									= 6001,

		SN_SenHub_Get_Info							= 6020,  
		SN_SenHub_Set_Info							= 6021,

		SN_SenHub_Get_SenData					= 6022,
		SN_SenHub_Set_SenData					= 6023,


}SN_CTL_ID;



typedef enum {														
	SN_ER_NOT_IMPLEMENT				= -13,		/*		Does Not Support this command		(501)		*/
	SN_ER_TIMEOUT								= -12,     /*		Request Timeout									(408)		*/
	SN_ER_SYS_BUSY								= -11,     /*		System is busy										(503)		*/
	SN_ER_VALUE_OUT_OF_RNAGE	= -10,     /*		Value is out of range							(416)		*/
	SN_ER_SYNTAX_ERROR					=   -9,		/*		Format is correct but syntax error		(422)		*/
	SN_ER_FORMAT_ERROR					=   -8,		/*		Format error											(415)		*/
	SN_ER_REQUEST_ERROR					=   -7,		/*		Request error										(400)		*/
	SN_ER_RESOURCE_LOSE					=   -6,		/*		SenHub disconnect								(410)		*/
	SN_ER_RESOURCE_LOCKED			=   -5,		/* 	Resource is in setting							(426)		*/
	SN_ER_NOT_FOUND						=   -4,		/*		Resource Not Found							(404)		*/
	SN_ER_WRITE_ONLY						=   -3,		/*		Read Only												(405)		*/
	SN_ER_READ_ONLY							=   -2,		/*		Write Only												(405)		*/
	SN_ER_FAILED									=   -1,		/*		Failed														(500)		*/
	SN_OK												=    0,		/*		Success													(200)		*/
	SN_INITILIZED									=    1,		/*		Library had initilized											*/
} SN_CODE;




#endif // __SENSOR_NETWORK_BASE_DEF_H__

