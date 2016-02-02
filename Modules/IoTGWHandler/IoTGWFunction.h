/****************************************************************************/
/* Copyright(C) : Advantech Technologies, Inc.														 */
/* Create Date  : 2015/03/09 by Eric Liang															     */
/* Modified Date: 2015/03/09 by Eric Liang															 */
/* Abstract       :  IoTGW Function                 													             */
/* Reference    : None																									 */
/****************************************************************************/
#ifndef  __IOTGW_FUNCTION_H__
#define __IOTGW_FUNCTION_H__

#include "platform.h"
#include "susiaccess_handler_api.h"

static const char* sAgentInfoFormat ="{\"devID\":\"%s\",\"hostname\":\"%s\",\"sn\":\"%s\",\
								\"mac\":\"%s\",\"version\":\"%s\",\"type\":\"%s\",\"product\":\"%s\",\
								\"manufacture\":\"%s\",\"status\":\"%d\"}";



#define CAGENG_HANDLER_NAME "general"

#define SENHUB_HANDLER_NAME "SenHub"
#define SENHUB_CALLBACKREQ_TOPIC "/cagent/admin/%s/agentactionreq"


//
//int GetIoTGWCapability
//(
//   char *buf,        /* buffer point : Put IoTGW's Capability*/
//   int buflen       /*  buffer length */
// );
//
//int GetSenNodeCapability 
//( 
//	int id,               /* sen id number for simulato only*/  
//	char *buf,        /* buffer point : Put IoTGW's Capability*/
//	int buflen       /*  buffer length */ 
//);
//
//int GetSenNodeDataforDemo 
//( 
//	int datanum,  /* Which data */
//	char *buf,        /* buffer point : Put IoTGW's Capability*/
//	int buflen,      /*  buffer length */ 
//	char *inifile
//);

int GetSenHubAgentInfobyUID(const void *pAgentInfoArray, const int Max, const char * uid);

int GetUsableIndex(void *pAgentInfoArray, int Max );

int GetAgentInfoData
(
	char *outData, 
	int buflen, 
	void *pSenNodeHandler
);

int PackSenHubPlugInfo
(
	Handler_info					*senNodeHandle,   // SenNode Plug-in Handler
	Handler_info					*gwHandlerInfo,     //  IoTGW Plug-in Handler
	const char                 *mac,                        //  SenNode's MAC
	const char                 *hostname,             //  Host Nam of the SenNode      "1020 Demo"
	const char                 *product,                 //  Model Name of the SenNode "WISE1020"
	const int                     status
);

#endif // __IOTGW_FUNCTION_H__


