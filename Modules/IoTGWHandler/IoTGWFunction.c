/****************************************************************************/
/* Copyright(C) : Advantech Technologies, Inc.														 */
/* Create Date  : 2015/03/09 by Eric Liang															     */
/* Modified Date: 2015/03/09 by Eric Liang															 */
/* Abstract       :  IoTGW Function                 													             */
/* Reference    : None																									 */
/****************************************************************************/

#include <stdlib.h>
#include <platform.h>

#include "IoTGWFunction.h"
#include "inc/IoTGW_Def.h"

#include "BasicFun_Tool.h"

int GetAgentInfoData(char *outData, int buflen, void *pInput )
{
	Handler_info *pSenHubHandler = (Handler_info*)pInput;

	snprintf(outData, buflen, sAgentInfoFormat, pSenHubHandler->agentInfo->devId,
																					pSenHubHandler->agentInfo->hostname,
																					pSenHubHandler->agentInfo->sn,
																					pSenHubHandler->agentInfo->mac,
																					pSenHubHandler->agentInfo->version,
																					pSenHubHandler->agentInfo->type,
																					pSenHubHandler->agentInfo->product,
																					pSenHubHandler->agentInfo->manufacture,
																					pSenHubHandler->agentInfo->status);
	return strlen(outData);

}

int PackSenHubPlugInfo(Handler_info *pSenHubHandler, Handler_info *pGwHandlerInfo, const char *mac, const char *hostname, const char *product, const int status )
{
	if( pSenHubHandler == NULL || pGwHandlerInfo == NULL ) return -1;

	if( pSenHubHandler->agentInfo == NULL ) return -1;

	memset( pSenHubHandler->agentInfo,0,sizeof(cagent_agent_info_body_t));

	memcpy( pSenHubHandler->agentInfo, pGwHandlerInfo->agentInfo, sizeof(cagent_agent_info_body_t));

	snprintf(pSenHubHandler->Name,MAX_TOPIC_LEN, "%s", CAGENG_HANDLER_NAME); // "general"

	snprintf( pSenHubHandler->agentInfo->hostname, DEF_HOSTNAME_LENGTH, "%s", hostname );
	snprintf( pSenHubHandler->agentInfo->devId, DEF_DEVID_LENGTH, "%s", mac );
	snprintf( pSenHubHandler->agentInfo->sn, DEF_SN_LENGTH, "%s", mac );
	snprintf( pSenHubHandler->agentInfo->mac, DEF_MAC_LENGTH, "%s", mac );
	snprintf( pSenHubHandler->agentInfo->type, DEF_MAX_STRING_LENGTH, "SenHub");
	snprintf( pSenHubHandler->agentInfo->product, DEF_MAX_STRING_LENGTH, "%s",product);
	pSenHubHandler->agentInfo->status = status;

	return 1;
}


int GetSenHubAgentInfobyUID(const void *pAgentInfoArray, const int Max, const char * uid)
{
	// Critical Section
	int index = -1;
	int i = 0;
	cagent_agent_info_body_t *pAgentInfo = (cagent_agent_info_body_t*)pAgentInfoArray;

	if( pAgentInfoArray == NULL ) return index;

	for( i= 0;  i < Max ; i ++ ) {
		if( pAgentInfo->status == 1 ) {
			if( !memcmp(pAgentInfo->devId, uid, strlen(uid)) )	{			
				index =i;
			    break;
			}
			pAgentInfo++;
		} else
			pAgentInfo++;
	}

	if( index == -1 ) {
		PRINTF("Can't Find =%s AgentInfo in Table\r\n", uid);
	}
	return index;
}


int GetUsableIndex(void *pAgentInfoArray, int Max )
{
	// Critical Section
	int index = -1;
	int i = 0;
	cagent_agent_info_body_t *pAgentInfo = (cagent_agent_info_body_t*)pAgentInfoArray;

	if( pAgentInfoArray == NULL ) return index;

	for( i= 0;  i < Max ; i ++ ) {
		if( pAgentInfo->status == 0 ) {
			pAgentInfo->status = 1;
			index = i;
			break;
		} else
			pAgentInfo++;
	}

	if( index == -1 ) {
		PRINTF("Can't get an empty AgentInfo Table\r\n");
	}

	return index;
}




