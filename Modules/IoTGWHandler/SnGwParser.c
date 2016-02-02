#include "SnGwParser.h"
#include "SnMJSONParser.h"
#include "IoTGWHandler.h"
#include "inc/SensorNetwork_Manager_API.h"

int ParseReceivedData(void* data, int datalen, int * cmdID, char *sessionId, int nLenSessionId )
{
	/*{"susiCommData":{"commCmd":251,"catalogID":4,"requestID":10}}*/

	cJSON* root = NULL;
	cJSON* body = NULL;
	cJSON* target = NULL;

	if(!data) return 0;
	if(datalen<=0) return 0;
	root = cJSON_Parse((char *)data);
	if(!root) { 
		PRINTF("Parse Error\n");
		return 0;
	}

	body = cJSON_GetObjectItem(root, AGENTINFO_BODY_STRUCT);
	if(!body)
	{
		PRINTF("Parse Error 2n");
		cJSON_Delete(root);
		return 0;
	}

	target = cJSON_GetObjectItem(body, AGENTINFO_CMDTYPE);
	if(target)
		*cmdID = target->valueint;

	target = cJSON_GetObjectItem( body, AGENTINFO_SESSION_ID ); 

	if(target)
		snprintf(sessionId,nLenSessionId,"%s",target->valuestring );

	cJSON_Delete(root);
	return 1;
}

//   topic: /cagent/admin/%s/agentcallbackreq
int GetSenHubUIDfromTopic(const char *topic, char *uid , const int size )
{
	int rc = 0;
	char *start = NULL;
	char *end = NULL;
	char *sp = NULL;
	int len = 0;

	if( topic == NULL || strlen(topic) == 0 || uid == NULL ) return rc;

	sp = strstr(topic,"admin/");
	if( sp )
		start = sp + 6; // after /

	sp = strstr(topic,"/agentcallbackreq");
	if(sp)
	end = sp;

	if( start == NULL || end == NULL ) return rc;

	len = end - start;

	if( size < len ) return rc;

	memcpy( uid, start, len );

	rc = 1;

	return rc;
}

void RePack_GetResultToWISECloud( const char *name, const char *data , char *szResult, int bufsize )
{
	int len = 0;
	char tmpbuf[MAX_FUNSET_DATA_SIZE]={0};
	//char szResult[MAX_FUNSET_DATA_SIZE]={0};

	unsigned int nStatusCode = 200;

	cJSON* root = NULL;
	cJSON  *pSubItem = NULL; 

	if(szResult == NULL)
		return;

	if( data == NULL ) { 
		sprintf(szResult, REPLY_SENSOR_400ERROR, name );
		return;
	}

	root = cJSON_Parse((char *)data);
	if(!root) { 
		sprintf(szResult, REPLY_SENSOR_400ERROR, name );
		return;
	}

	pSubItem = cJSON_GetObjectItem( root, SN_STATUS_CODE ); 

	if(!pSubItem)  { 
		sprintf(szResult, REPLY_SENSOR_400ERROR, name );
		return;
	}

	nStatusCode = pSubItem->valueint;

	// In:    { "StatusCode":200, "Result":{"n":"door","v":26,"min":0, "max":100} }
	// Out: {"n":"SenData/door temp","v":26, "StatusCode": 200}
	
	// tmpbuf => "v": 26,   "sv":"123", "bv":1
	pSubItem = cJSON_GetObjectItem( root, SN_RESUTL ); 
	
	if(!pSubItem)  { 
		sprintf(szResult, REPLY_SENSOR_400ERROR, name );
		return;
	}

	GetSenMLValue(cJSON_Print(pSubItem), tmpbuf, sizeof(tmpbuf));

	if(strlen(tmpbuf) == 0 ) {
		snprintf(tmpbuf,sizeof(tmpbuf),"\"sv\":%s",cJSON_Print(pSubItem));
	}

                                                        /* { "n":"%s", %s, "StatusCode":  %d} */
	snprintf(szResult, bufsize, REPLY_SENSOR_FORMAT, name, tmpbuf, nStatusCode );
	return;
}


int ProcGetSenHubValue(const char *uid, const char *sessionId, const char *data, char *buffer, const int bufsize)
{
	int len = 0;
	int i =0;
	char URI[MAX_PATH]={0};
	char handlename[MAX_HANDL_NAME]={0};
	char tmpbuf[MAX_FUNSET_DATA_SIZE]={0};
	char sendatas[MAX_BUFFER_SIZE]={0};
	int nStatusCode = 200;


	cJSON* root = NULL;
	cJSON* body = NULL;
	cJSON* target = NULL;
	cJSON* pElement = NULL;
	cJSON* pSubItem = NULL;
	cJSON* jsName = NULL;
	int nCount = 0;

	if(!data) return len;

	root = cJSON_Parse((char *)data);
	if(!root) {
		return 0;
	}

	body = cJSON_GetObjectItem(root, AGENTINFO_BODY_STRUCT);
	if(!body)
	{
		cJSON_Delete(root);
		return 0;
	}
	pSubItem = cJSON_GetObjectItem( body, SEN_HANDLER_NAME ); 

	if(!pSubItem)
	{
		cJSON_Delete(root);
		return 0;
	}
	snprintf(handlename,sizeof(handlename),"%s",pSubItem->valuestring );

	target = cJSON_GetObjectItem(body, SEN_IDLIST);
	if(!target)
	{
		cJSON_Delete(root);
		return 0;
	}

	pElement = cJSON_GetObjectItem( target, SENML_E_FLAG ); // "e"
	nCount = cJSON_GetArraySize(pElement);

	for( i = 0; i<nCount ; i ++ ) {
		pSubItem = cJSON_GetArrayItem(pElement, i);
		if( pSubItem == NULL ) continue;

		jsName = cJSON_GetObjectItem( pSubItem, SENML_N_FLAG );
		
		if( jsName == NULL ) continue;

		memset(URI, 0, sizeof(URI) );
		//snprintf(URI,sizeof(URI),"%s/%s/%s", uid,handlename, jsName->valuestring ); // => SenHub_UID/SenHub/SenData/co2
		snprintf(URI,sizeof(URI),"%s/%s", uid, jsName->valuestring ); // => SenHub_UID/SenHub/SenData/co2
		snprintf(tmpbuf,sizeof(tmpbuf),"%s",pSNManagerAPI->SN_Manager_GetData(URI, CACHE_MODE));
		RePack_GetResultToWISECloud( jsName->valuestring, tmpbuf, sendatas, sizeof(sendatas) ); 
		if( i != nCount -1 ) // append ","
			strcat( sendatas, "," );
	}

	cJSON_Delete(root);

	/*{"sessionID":"%s", "sensorInfoList":{"e":[%s]} }*/
	snprintf( buffer, bufsize, REPLY_FORMAT, sessionId, sendatas );

	return strlen(buffer);
}

int ProcGetSenHubCapability(const char *uid, const char *data, char *buffer, const int bufsize)
{
	int len = 0;
	int i =0;
	char URI[MAX_PATH]={0};
	char handlename[MAX_HANDL_NAME]={0};
	char tmpbuf[MAX_BUFFER_SIZE]={0};
	int nStatusCode = 200;

	cJSON* root = NULL;
	cJSON* body = NULL;
	cJSON* target = NULL;
	cJSON* pSubItem = NULL;

	int nCount = 0;

	if(!data) return len;

	root = cJSON_Parse((char *)data);
	if(!root) {		
		snprintf(buffer, bufsize,"%s",g_szSenHubErrorCapability);
		return  strlen(buffer);
	}

	body = cJSON_GetObjectItem(root, AGENTINFO_BODY_STRUCT);
	if(!body)
	{	
		cJSON_Delete(root);
		snprintf(buffer, bufsize,"%s",g_szSenHubErrorCapability);
		return  strlen(buffer);
	}
	pSubItem = cJSON_GetObjectItem( body, SEN_HANDLER_NAME ); 

	if(!pSubItem)
	{
		cJSON_Delete(root);
		snprintf(buffer, bufsize,"%s",g_szSenHubErrorCapability);
		return  strlen(buffer);
	}

	snprintf(handlename,sizeof(handlename),"%s",pSubItem->valuestring ); // SenHub

	
	// SenHubID/SenHub
	memset(URI, 0, sizeof(URI) );
	snprintf(URI,sizeof(URI),"%s/%s", uid,handlename ); // => SenHub_UID/SenHub
	
	memset(tmpbuf, 0, sizeof(tmpbuf));

	snprintf(tmpbuf,sizeof(tmpbuf),"%s",pSNManagerAPI->SN_Manager_GetData(URI, CACHE_MODE));
	//PRINTF("URI=%s Ret=%s\n", URI, tmpbuf);

	cJSON_Delete(root);
	root = NULL;

	//{ \"StatusCode\":  %s, \"Result\": \"%s\"}
	root = cJSON_Parse((char *)tmpbuf);
	if(!root) {
		// "{\"sessionID\":\"%s\",\"StatusCode\":%d}"
		snprintf(buffer, bufsize,"%s",g_szSenHubErrorCapability);
		return  strlen(buffer);;
	}

	pSubItem = cJSON_GetObjectItem( root, SN_STATUS_CODE ); 

	if(!pSubItem) {
		// "{\"sessionID\":\"%s\",\"StatusCode\":%d}"
		snprintf(buffer, bufsize,"%s",g_szSenHubErrorCapability);
		return  strlen(buffer);;
	}

	nStatusCode = pSubItem->valueint;

	//PRINTF("Status=%d\n",nStatusCode);

	if( nStatusCode != 200 ) {  // "{\"sessionID\":\"%s\",\"StatusCode\":%d}"
		snprintf(buffer, bufsize,"%s",g_szSenHubErrorCapability);
	}else {
		target = cJSON_GetObjectItem( root, SN_RESUTL ); 
		if( !target ) // "{\"sessionID\":\"%s\",\"StatusCode\":%d}"
			snprintf(buffer, bufsize,"%s",g_szSenHubErrorCapability);
		else {
			snprintf(buffer,bufsize,"%s",cJSON_Print(target) );
		}
	}

	//PRINTF("Get SenHub=%s Capability=%s\n", uid, buffer );

	len = strlen(buffer);
	return len;
}


int RePack_SetResultToWISECloud( const char *name, const char *data, int *code, char *szResult, int iLenResult)
{
	int len = 0;
	char tmpbuf[MAX_FUNSET_DATA_SIZE]={0};

	unsigned int nStatusCode = 202;

	cJSON* root = NULL;
	cJSON  *pSubItem = NULL; 

	if(szResult == NULL)
	{
		nStatusCode = 400;
		*code = nStatusCode;
		return nStatusCode;
	}

	if( data == NULL ) { 
		nStatusCode = 400;
		*code = nStatusCode;
		sprintf(szResult, REPLY_SENSOR_400ERROR, name );
		return nStatusCode;
	}

	root = cJSON_Parse((char *)data);
	if(!root) { 
		nStatusCode = 400;
		*code = nStatusCode;
		sprintf(szResult, REPLY_SENSOR_400ERROR, name );
		return false;
	}

	// In:    { "StatusCode":200, "Result":{"n":"door","v":26,"min":0, "max":100} }
	// Out: {"n":"SenData/door temp","v":26, "StatusCode": 200}
	
	// tmpbuf => "v": 26,   "sv":"123", "bv":1
	GetSenMLValue(data, tmpbuf, sizeof(tmpbuf));

	pSubItem = cJSON_GetObjectItem( root, SN_STATUS_CODE ); 

	if(!pSubItem) {
		nStatusCode = 400;
		*code = nStatusCode;
		sprintf(szResult, REPLY_SENSOR_400ERROR, name );
		return nStatusCode;
	}

	nStatusCode = pSubItem->valueint;
	*code = pSubItem->valueint;

	// In:    { "StatusCode":200, "Result":{"n":"door","v":26,"min":0, "max":100} }
	// Out: {"n":"SenData/door temp","v":26, "StatusCode": 200}
	
	// tmpbuf => "v": 26,   "sv":"123", "bv":1
	pSubItem = cJSON_GetObjectItem( root, SN_RESUTL ); 
	
	if(!pSubItem)  { 
		sprintf(szResult, REPLY_SENSOR_400ERROR, name );
		return nStatusCode;
	}

	GetSenMLValue(cJSON_Print(pSubItem), tmpbuf, sizeof(tmpbuf));
	if(strlen(tmpbuf) == 0 ) {
		snprintf(tmpbuf,sizeof(tmpbuf),"\"sv\":%s",cJSON_Print(pSubItem));
	}

	                                                      /* { "n":"%s", %s, "StatusCode":  %d} */
	snprintf(szResult, iLenResult, REPLY_SENSOR_FORMAT, name, tmpbuf, nStatusCode );

	return nStatusCode;
}

int EnableSenHubAutoReport(const char *uid , int enable )
{
	char URI[MAX_PATH]={0};
	char setvalue[MAX_SET_RFORMAT_SIZE]={0};
	char tmpbuf[MAX_FUNSET_DATA_SIZE]={0};
	char sendatas[MAX_BUFFER_SIZE]={0};
	int statuscode = 200;
	int rc = 0;

	AsynSetParam *pAsynSetParam = NULL;

	pAsynSetParam = AllocateMemory(sizeof(AsynSetParam));
	if( pAsynSetParam == NULL ) return rc;

	memset(pAsynSetParam, 0 , sizeof(AsynSetParam));
	pAsynSetParam->index = -1; // It is AP behavior not process, only check not reply to WISE-PaSS

	snprintf(URI, sizeof(URI), "%s/SenHub/Action/AutoReport", uid );
	snprintf(setvalue,sizeof(setvalue),"{\"bv\":%d}",enable);
	snprintf(tmpbuf,sizeof(tmpbuf),"%s",pSNManagerAPI->SN_Manager_SetData(URI, setvalue, pAsynSetParam ));

	RePack_SetResultToWISECloud(URI, tmpbuf, &statuscode, sendatas, sizeof(sendatas));

	if( statuscode != 202 ) { // Error or Success  => pAsynSetParam (x)
			PRINTF("Enable %s   Action/AutoReport Failed", uid);
			FreeMemory( pAsynSetParam ); 
	} else
		rc = 1; // Successful

	return rc;
}

// {"susiCommData":{"commCmd":525,"handlerName":"SenHub","sessionID":"XXX","sensorIDList":{"e":[{"n":"SenData/dout","bv":1}]}}}
int ProcSetSenHubValue(const char *uid, const char *sessionId, const char *cmd, const int index, const char *topic, char *buffer, const int bufsize)
{
	int len = 0;
	int i = 0;
	char URI[MAX_PATH]={0};
	char handlename[MAX_HANDL_NAME]={0};
	char tmpbuf[MAX_FUNSET_DATA_SIZE]={0};
	char sendatas[MAX_BUFFER_SIZE]={0};
	char setvalue[MAX_SET_RFORMAT_SIZE]={0};

	AsynSetParam *pAsynSetParam = NULL;
	int statuscode = 200;
	int nCount = 0;

	cJSON* root = NULL;
	cJSON* body = NULL;
	cJSON* target = NULL;
	cJSON* pElement = NULL;
	cJSON* pSubItem = NULL;
	cJSON* jsName = NULL;

	memset(sendatas, 0, sizeof(sendatas));


	if(!cmd) return len;

	root = cJSON_Parse((char *)cmd);
	if(!root) return len;

	body = cJSON_GetObjectItem(root, AGENTINFO_BODY_STRUCT); // susiCommData
	if(!body)
	{
		cJSON_Delete(root);
		return len;
	}
	pSubItem = cJSON_GetObjectItem( body, SEN_HANDLER_NAME ); // handlerName

	if(!pSubItem)
	{
		cJSON_Delete(root);
		return len;
	}
	snprintf(handlename,sizeof(handlename),"%s",pSubItem->valuestring ); // handlername -> SenHub


	target = cJSON_GetObjectItem(body, SEN_IDLIST);
	if(!target)
	{
		cJSON_Delete(root);
		return len;
	}

	pElement = cJSON_GetObjectItem( target, SENML_E_FLAG ); // "e"
	nCount = cJSON_GetArraySize(pElement);

	// only support nCount == 1

	for( i = 0; i<nCount ; i ++ ) {
		pSubItem = cJSON_GetArrayItem(pElement, i);
		if( pSubItem == NULL) break;

		jsName = cJSON_GetObjectItem( pSubItem, SENML_N_FLAG ); // SenData/co2
		memset(URI, 0, sizeof(URI) );
		//snprintf(URI,sizeof(URI),"%s/%s/%s", uid,handlename, jsName->valuestring ); // => SenHub_UID/SenHub/SenData/co2
		snprintf(URI,sizeof(URI),"%s/%s", uid, jsName->valuestring ); // => SenHub_UID/SenHub/SenData/co2

		pAsynSetParam = AllocateMemory(sizeof(AsynSetParam));
		if( pAsynSetParam != NULL ) {
			memset(pAsynSetParam, 0 , sizeof(AsynSetParam));
			pAsynSetParam->index = index;
			snprintf( pAsynSetParam->szTopic, sizeof(pAsynSetParam->szTopic), "%s", topic );
			snprintf( pAsynSetParam->szUID, sizeof(pAsynSetParam->szUID), "%s", uid );
			/* { \"sessionID\":\"%s\", \"sensorInfoList\":{\"e\":[ {\"n\":\"%s\", %%s, \"StatusCode\": %%d } ] } */
			snprintf( pAsynSetParam->szSetFormat, sizeof(pAsynSetParam->szSetFormat), SENHUB_SET_REPLY_FORMAT, sessionId, jsName->valuestring );
			memset( tmpbuf, 0, sizeof( tmpbuf ) );
			GetSenMLValue(cJSON_Print(pSubItem), tmpbuf, sizeof(tmpbuf));
			snprintf( setvalue,sizeof(setvalue),"{ %s }", tmpbuf );
			memset( tmpbuf, 0, sizeof( tmpbuf ) );

			snprintf(tmpbuf,sizeof(tmpbuf),"%s",pSNManagerAPI->SN_Manager_SetData(URI, setvalue, pAsynSetParam ));

			RePack_SetResultToWISECloud( jsName->valuestring, tmpbuf, &statuscode, sendatas, sizeof(sendatas) );

			if( statuscode != 202 ) // Error or Success  => pAsynSetParam (x)
				FreeMemory( pAsynSetParam );
		}else {
			snprintf( sendatas, sizeof(sendatas),  REPLY_SENSOR_400ERROR, jsName->valuestring );
		}
		break; // one sessionID -> one Set Param
	}

	cJSON_Delete(root);

	/*{"sessionID":"%s", "sensorInfoList":{"e":[%s]} }*/
	snprintf( buffer, bufsize, REPLY_FORMAT, sessionId, sendatas );

	len = strlen(buffer);
	return len;
}

// data: { "StatusCode":  200, "Result": "Success"}
int ProcAsynSetSenHubValueEvent(const char *data, const int datalen, const AsynSetParam *pAsynSetParam, char *buffer, const int bufsize)
{
	int len = 0;
	int i = 0;
	char result[128]={0};
	int statuscode = 200;

	cJSON* root = NULL;
	cJSON* body = NULL;
	cJSON* target = NULL;
	cJSON* pSubItem = NULL;

	if(!data) return len;

	root = cJSON_Parse((char *)data);
	if(!root) return len;

	target = cJSON_GetObjectItem(root, SN_STATUS_CODE);  // "StatusCode":  200
	if(!target)
	{
		cJSON_Delete(root);
		return len;
	}

	statuscode = target->valueint;

	target = cJSON_GetObjectItem( root, SN_RESUTL ); // "Result": "Success"

	if(!target)
	{
		cJSON_Delete(root);
		return len;
	}

	GetSenMLValue(cJSON_Print(target), result, sizeof(result));

	if(strlen(result) == 0 ) {
		snprintf(result,sizeof(result),"\"sv\":%s",cJSON_Print(target));
	}

	cJSON_Delete(root);

	// { "sessionID":"XXX", "sensorInfoList":{"e":[ {"n":"SenData/dout", %s, "StatusCode": %d } ] } }
	snprintf( buffer, bufsize, pAsynSetParam->szSetFormat, result, statuscode );

	len = strlen(buffer);
	return len;
}

/* =================== Interface Get Set ================================= */

int ProcGetInterfaceValue(const char *sessionId, const char *data, char *buffer, const int bufsize)
{
	int len = 0;
	int i =0;
	char URI[MAX_PATH]={0};
	char handlename[MAX_HANDL_NAME]={0};
	char tmpbuf[MAX_FUNSET_DATA_SIZE]={0};
	char sendatas[MAX_BUFFER_SIZE]={0};
	int nStatusCode = 200;
	int nCount = 0;
	cJSON* root = NULL;
	cJSON* body = NULL;
	cJSON* target = NULL;
	cJSON* pElement = NULL;
	cJSON* pSubItem = NULL;
	cJSON* jsName = NULL;


	if(!data) return len;

	root = cJSON_Parse((char *)data);
	if(!root) {
		return 0;
	}

	body = cJSON_GetObjectItem(root, AGENTINFO_BODY_STRUCT);
	if(!body)
	{
		cJSON_Delete(root);
		return 0;
	}
	pSubItem = cJSON_GetObjectItem( body, SEN_HANDLER_NAME ); 

	if(!pSubItem)
	{
		cJSON_Delete(root);
		return 0;
	}

	snprintf(handlename,sizeof(handlename),"%s",pSubItem->valuestring ); // IoTGW

	target = cJSON_GetObjectItem(body, SEN_IDLIST);
	if(!target)
	{
		cJSON_Delete(root);
		return 0;
	}

	pElement = cJSON_GetObjectItem( target, SENML_E_FLAG ); // "e"
	nCount = cJSON_GetArraySize(pElement);

	for( i = 0; i<nCount ; i ++ ) {
		pSubItem = cJSON_GetArrayItem(pElement, i);
		if( pSubItem == NULL ) continue;

		jsName = cJSON_GetObjectItem( pSubItem, SENML_N_FLAG );
		
		if( jsName == NULL ) continue;

		memset(URI, 0, sizeof(URI) );
		//snprintf(URI,sizeof(URI),"%s/%s", handlename, jsName->valuestring ); // => IoTGW/WSN/0000852CF4B7B0E8/Info/SenHubList
		snprintf(URI,sizeof(URI),"%s", jsName->valuestring ); // => IoTGW/WSN/0000852CF4B7B0E8/Info/SenHubList
		snprintf(tmpbuf,sizeof(tmpbuf),"%s",pSNManagerAPI->SN_Manager_GetData(URI, CACHE_MODE));
	
		RePack_GetResultToWISECloud( jsName->valuestring, tmpbuf, sendatas, sizeof(sendatas) );
		if( i != nCount -1 ) // append ","
			strcat( sendatas, "," );
	}

	cJSON_Delete(root);

	/*{"sessionID":"%s", "sensorInfoList":{"e":[%s]} }*/
	snprintf( buffer, bufsize, REPLY_FORMAT, sessionId, sendatas );

	return strlen(buffer);
}


int ProcSetInterfaceValue(const char *sessionId, const char *cmd, char *buffer, const int bufsize)
{
	int len = 0;
	int i = 0;
	char URI[MAX_PATH]={0};
	char handlename[MAX_HANDL_NAME]={0};
	char tmpbuf[MAX_FUNSET_DATA_SIZE]={0};
	char sendatas[MAX_BUFFER_SIZE]={0};
	char setvalue[MAX_SET_RFORMAT_SIZE]={0};

	AsynSetParam *pAsynSetParam = NULL;
	int statuscode = 200;
	int nCount = 0;

	cJSON* root = NULL;
	cJSON* body = NULL;
	cJSON* target = NULL;
	cJSON* pElement = NULL;
	cJSON* pSubItem = NULL;
	cJSON* jsName = NULL;

	memset(sendatas, 0, sizeof(sendatas));


	if(!cmd) return len;

	root = cJSON_Parse((char *)cmd);
	if(!root) return len;

	body = cJSON_GetObjectItem(root, AGENTINFO_BODY_STRUCT); // susiCommData
	if(!body)
	{
		cJSON_Delete(root);
		return len;
	}
	pSubItem = cJSON_GetObjectItem( body, SEN_HANDLER_NAME ); // handlerName

	if(!pSubItem)
	{
		cJSON_Delete(root);
		return len;
	}
	snprintf(handlename,sizeof(handlename),"%s",pSubItem->valuestring ); // handlername -> IoTGW


	target = cJSON_GetObjectItem(body, SEN_IDLIST);
	if(!target)
	{
		cJSON_Delete(root);
		return len;
	}

	pElement = cJSON_GetObjectItem( target, SENML_E_FLAG ); // "e"
	nCount = cJSON_GetArraySize(pElement);

	// only support nCount == 1

	for( i = 0; i<nCount ; i ++ ) {
		pSubItem = cJSON_GetArrayItem(pElement, i);
		if( pSubItem == NULL) break;

		jsName = cJSON_GetObjectItem( pSubItem, SENML_N_FLAG ); // SenData/co2
		memset(URI, 0, sizeof(URI) );
		//snprintf(URI,sizeof(URI),"%s/%s", handlename, jsName->valuestring ); // => IoTGW/WSN/0000852CF4B7B0E8/Info/Name
		snprintf(URI,sizeof(URI),"%s", jsName->valuestring ); // => IoTGW/WSN/0000852CF4B7B0E8/Info/Name

		pAsynSetParam = AllocateMemory(sizeof(AsynSetParam));
		if( pAsynSetParam != NULL ) {
			memset(pAsynSetParam, 0 , sizeof(AsynSetParam));
			pAsynSetParam->index = IOTGW_INFTERFACE_INDEX;
			/* { \"sessionID\":\"%s\", \"sensorInfoList\":{\"e\":[ {\"n\":\"%s\", %%s, \"StatusCode\": %%d } ] } */
			snprintf( pAsynSetParam->szSetFormat, sizeof(pAsynSetParam->szSetFormat), SENHUB_SET_REPLY_FORMAT, sessionId, jsName->valuestring );
			memset( tmpbuf, 0, sizeof( tmpbuf ) );
			GetSenMLValue(cJSON_Print(pSubItem), tmpbuf, sizeof(tmpbuf));
			snprintf( setvalue,sizeof(setvalue),"{ %s }", tmpbuf );
			memset( tmpbuf, 0, sizeof( tmpbuf ) );

			snprintf(tmpbuf,sizeof(tmpbuf),"%s",pSNManagerAPI->SN_Manager_SetData(URI, setvalue, pAsynSetParam ));

			RePack_SetResultToWISECloud( jsName->valuestring, tmpbuf, &statuscode, sendatas, sizeof(sendatas) );

			if( statuscode != 202 ) // Error or Success  => pAsynSetParam (x)
				FreeMemory( pAsynSetParam );
		}else {
			snprintf( sendatas, sizeof(sendatas),  REPLY_SENSOR_400ERROR, jsName->valuestring );
		}
		break; // one sessionID -> one Set Param
	}

	cJSON_Delete(root);

	/*{"sessionID":"%s", "sensorInfoList":{"e":[%s]} }*/
	snprintf( buffer, bufsize, REPLY_FORMAT, sessionId, sendatas );

	len = strlen(buffer);
	return len;
}

int ProcGetTotalInterface(char *buffer, const int bufsize)
{
	int len = 0;
	char URI[MAX_PATH]={0};
	char tmpbuf[MAX_FUNSET_DATA_SIZE]={0};
	char sendatas[MAX_BUFFER_SIZE]={0};
	cJSON* root = NULL;
	cJSON* body = NULL;
	cJSON* newroot = NULL;
	
	if(!pSNManagerAPI)
		return len;

	memset(URI, 0, sizeof(URI) );
	snprintf(URI,sizeof(URI),"%s", "IoTGW" );

	memset(buffer, 0, bufsize );

	strncpy(tmpbuf, pSNManagerAPI->SN_Manager_GetData(URI, CACHE_MODE), MAX_FUNSET_DATA_SIZE);

	root = cJSON_Parse(tmpbuf);
	if(!root)
		return len;

	body = cJSON_GetObjectItem(root, SN_ERROR_REP); // susiCommData

	newroot = cJSON_CreateObject();
	cJSON_AddItemToObject(newroot, "IoTGW", cJSON_Duplicate(body, true));
	cJSON_Delete(root);
	snprintf( buffer, bufsize, "%s", cJSON_PrintUnformatted(newroot));
	len = strlen(buffer);

	cJSON_Delete(newroot);

	return len;
}

