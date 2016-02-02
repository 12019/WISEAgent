#include "Parser.h"
#include "HDDLog.h"
#include "ThresholdHelper.h"
#define MyTopic "HDDMonitor"

//************************************
// Method:    Parser_ParseAutoReportCmd
// Returns:   bool
// Parameter-In:    void * data (JSON string)
// Parameter-InOut: int * interval 
// Parameter-Out:   cJSON * * ppENodeArray  //save  pathArray as custom looks like: 
//************************************
/* ["e":[
		{"n":"HDDMonitor/hddInfoList"},
		 {"n":"HDDMonitor/hddSmartInfoList/Disk0-Crucial_CT250MX200SSD1/PowerOnHoursPOH"}]
*/
bool Parser_ParseAutoReportCmd( void * data, bool * bEnable, cJSON ** ppRequestPathArray, int * intervalMs, int * timeoutMs) 
{ 
	cJSON * pRoot = NULL;
	cJSON * pRootItem = NULL;
	cJSON * params = NULL;
	cJSON * target = NULL;
	cJSON * pRequest = * ppRequestPathArray;

	if ( !  data  ) return false;
	pRoot = cJSON_Parse( ( const char * ) data ) ; if ( !  pRoot  ) return false;
	pRootItem = cJSON_GetObjectItem( pRoot, AGENTINFO_BODY_STRUCT ); if ( ! pRootItem  ) { cJSON_Delete( pRoot ); return false; }
	params = cJSON_GetObjectItem( pRootItem, HDD_AUTOUPLOAD_INTERVAL_MS ); if (params) * intervalMs = params->valueint;
	params = cJSON_GetObjectItem( pRootItem, HDD_AUTOUPLOAD_INTERVAL_SEC); if (params) * intervalMs = params->valueint * 1000;
	params = cJSON_GetObjectItem( pRootItem, HDD_AUTOUPLOAD_TIMEOUT_MS  ); if (params) *  timeoutMs = params->valueint;
	params = cJSON_GetObjectItem( pRootItem, HDD_AUTOUPLOAD_TIMEOUT_SEC ); if (params) *  timeoutMs = params->valueint * 1000;
	params = cJSON_GetObjectItem( pRootItem, HDD_AUTOUPLOAD_REQ_ITEMS )  ; if ( ! params ) { cJSON_Delete( pRoot ) ; return false; }

	target = cJSON_GetObjectItem(params, "All");
	if ( target )
	{
		char * pInfo = NULL;
		cJSON * item = cJSON_CreateObject();
		cJSON_AddItemToObject(item, HDD_SPEC_NAME, cJSON_CreateString(MyTopic));
		if (pRequest) cJSON_Delete(pRequest);
		pRequest = cJSON_CreateArray();
		cJSON_AddItemToArray(pRequest, item);
		* bEnable = true;
	}
	else
	{
		//   {"HDDMonitor":{"e":[...] }}
		target = cJSON_GetObjectItem(params, MyTopic);
		if (target) {
			target = cJSON_GetObjectItem(target, HDD_SPEC_ELEMENT);
			if (target) {
				if (pRequest)  cJSON_Delete(pRequest);
				pRequest = cJSON_Duplicate(target, 1);
				* bEnable = true;
			}
			else * bEnable = false;

		}
		// == there is no request for HDDMonitor 
		else
		{
			//   {"e":[...]}
			target = cJSON_GetObjectItem(params, HDD_SPEC_ELEMENT);
			if (target){
				if (pRequest)  cJSON_Delete(pRequest);
				pRequest = cJSON_Duplicate(target, 1);
				* bEnable = true;
			}
			else * bEnable = false;
		}
	}

	if ( * bEnable == false ) {
		if (pRequest)  cJSON_Delete(pRequest);
		pRequest = NULL;
	}

	cJSON_Delete( pRoot ) ;
	* ppRequestPathArray = pRequest;
	return true;
}


//=== private function called by others e.g. CreateInfoSpec
cJSON* CreateENodeItem( char const * name, char const * type, int value, char const * svalue, char const * unit ) 
{ 
	cJSON* pItem = cJSON_CreateObject( ) ;
	cJSON_AddStringToObject( pItem, HDD_SPEC_NAME, name ) ;
	if ( ! strcmp( type,HDD_SPEC_VALUE ) )         cJSON_AddNumberToObject( pItem, HDD_SPEC_VALUE, value ) ;
	else if ( ! strcmp( type,HDD_SPEC_SVALUE ) )   cJSON_AddStringToObject( pItem, HDD_SPEC_SVALUE, svalue ) ;
	if ( unit )                                    cJSON_AddStringToObject( pItem, HDD_SPEC_UNIT, unit ) ;
	return pItem;
}


//=== private function called by others e.g. CreateInfoSpec
cJSON* CreateSmartBaseInfo( hdd_mon_info_t * hddMonInfo ) 
{ 
/*
	{ 
		e:[
			{ "n": "hddType", "sv": "hdd"},
			{ "n": "hddName", "sv": "FUJITSU MHZ2160BJ FFS G2"},
			{ "n": "hddIndex", "v": "0"}
		],
		"bn": "BaseInfo",
		"basm": "R",
		"btype": "d",
		"bst": "",
		"bextend": ""
	}
*/
	cJSON * root = NULL;
	cJSON * pENode = NULL;
	cJSON * pTemp = NULL;		// a cJSON object , contain the most small item in a pEnode,  such as :{ "n": "value", "v":"100"}
	char cTemp[128] = { 0};			
	if ( !  hddMonInfo  ) return NULL;

	root = cJSON_CreateObject( ) ;
	pENode = cJSON_CreateArray( ) ;
	cJSON_AddItemToObject		( root, HDD_SPEC_ELEMENT, pENode ) ;
	cJSON_AddStringToObject		( root, HDD_SPEC_BASENAME, SMART_BASEINFO ) ;
	cJSON_AddStringToObject		( root,HDD_SPEC_ASM, HDD_SPEC_BASM_DEF ) ;
	//cJSON_AddStringToObject		( root,HDD_SPEC_BTYPE, HDD_SPEC_BTYPE_DEF ) ;
	//cJSON_AddStringToObject		( root,HDD_SPEC_BST, HDD_SPEC_BST_DEF ) ;
	//cJSON_AddStringToObject		( root,HDD_SPEC_BEXTEND, HDD_SPEC_BEXTEND_DEF ) ;

	pTemp = cJSON_CreateObject( ) ;
	cJSON_AddStringToObject( pTemp, HDD_SPEC_NAME, HDD_TYPE ) ;
	cJSON_AddStringToObject( pTemp, HDD_SPEC_SVALUE, HDD_TYPE_HDD ) ;
	cJSON_AddItemToArray( pENode, pTemp ) ;
	pTemp = NULL;

	pTemp = cJSON_CreateObject( ) ;
	cJSON_AddStringToObject( pTemp, HDD_SPEC_NAME, HDD_NAME ) ;
	cJSON_AddStringToObject( pTemp, HDD_SPEC_SVALUE, hddMonInfo->hdd_name ) ;
	cJSON_AddItemToArray( pENode, pTemp ) ;
	pTemp = NULL;

	pTemp = cJSON_CreateObject( ) ;
	cJSON_AddStringToObject( pTemp, HDD_SPEC_NAME, HDD_INDEX ) ;
	cJSON_AddNumberToObject( pTemp, HDD_SPEC_VALUE, ( double ) hddMonInfo->hdd_index  ) ;
	cJSON_AddItemToArray( pENode, pTemp ) ;
	pTemp = NULL;

	return root;
}


//=== private function called by others e.g. CreateInfoSpec===
cJSON * CreateSmartAttrItem( smart_attri_info_t * pSmart ) 
{ 
/*
	{ 
		e:[
				{ "n": "type", "v":"252"},
				{ "n": "flags", "v":"12800"},
				{ "n": "worst", "v":"100"},
				{ "n": "value", "v":"100"},
				{ "n": "vendorData", "sv":"15C7A3AA0004"}
		],
		"bn": "TotalLBAsRead",
		"basm": "R",
		"btype": "d",
		"bst": "",
		"bextend": ""
	}
*/
	cJSON * root = NULL;
	cJSON* pENode = NULL;
	cJSON * pTemp = NULL;		// a cJSON object , contain the most small item in a pEnode,  such as :{ "n": "value", "v":"100"}
	char vendorDataStr[16] = { 0};
	if ( ! pSmart  ) return NULL;

	root = cJSON_CreateObject( ) ;
	pENode = cJSON_CreateArray( ) ;
	cJSON_AddItemToObject  ( root, HDD_SPEC_ELEMENT, pENode ) ;
	cJSON_AddStringToObject( root, HDD_SPEC_BASENAME, pSmart->attriName ) ;
	cJSON_AddStringToObject( root, HDD_SPEC_ASM, HDD_SPEC_BASM_DEF ) ;
	//cJSON_AddStringToObject		( root,HDD_SPEC_BTYPE, HDD_SPEC_BTYPE_DEF ) ;
	//cJSON_AddStringToObject		( root,HDD_SPEC_BST, HDD_SPEC_BST_DEF ) ;
	//cJSON_AddStringToObject		( root,HDD_SPEC_BEXTEND, HDD_SPEC_BEXTEND_DEF ) ;
	pTemp = cJSON_CreateObject( ) ;
	cJSON_AddStringToObject( pTemp, HDD_SPEC_NAME, SMART_ATTRI_TYPE ) ;
	cJSON_AddNumberToObject( pTemp, HDD_SPEC_VALUE,  pSmart->attriType ) ;
	cJSON_AddItemToArray( pENode, pTemp ) ;
	pTemp = NULL;
	pTemp = cJSON_CreateObject( ) ;
	cJSON_AddStringToObject( pTemp, HDD_SPEC_NAME, SMART_ATTRI_FLAGS ) ;
	cJSON_AddNumberToObject( pTemp, HDD_SPEC_VALUE,  pSmart->attriFlags ) ;
	cJSON_AddItemToArray( pENode, pTemp ) ;
	pTemp = NULL;
	pTemp = cJSON_CreateObject( ) ;
	cJSON_AddStringToObject( pTemp, HDD_SPEC_NAME, SMART_ATTRI_WORST ) ;
	cJSON_AddNumberToObject( pTemp, HDD_SPEC_VALUE,  pSmart->attriWorst ) ;
	cJSON_AddItemToArray( pENode, pTemp ) ;
	pTemp = NULL;
	//pTemp = cJSON_CreateObject( ) ;
	//cJSON_AddStringToObject( pTemp, HDD_SPEC_NAME, SMART_ATTRI_THRESH ) ;
	//cJSON_AddNumberToObject( pTemp, HDD_SPEC_VALUE,  pSmart->attriThresh ) ;
	//cJSON_AddItemToArray( pENode, pTemp ) ;
	//pTemp = NULL;
	pTemp = cJSON_CreateObject( ) ;
	cJSON_AddStringToObject( pTemp, HDD_SPEC_NAME, SMART_ATTRI_VALUE ) ;
	cJSON_AddNumberToObject( pTemp, HDD_SPEC_VALUE,  pSmart->attriValue ) ;
	cJSON_AddItemToArray( pENode, pTemp ) ;
	pTemp = NULL;
	sprintf_s( vendorDataStr, sizeof( vendorDataStr ) , "%02X%02X%02X%02X%02X%02X",
		( unsigned char ) pSmart->attriVendorData[5],
		( unsigned char ) pSmart->attriVendorData[4],
		( unsigned char ) pSmart->attriVendorData[3],
		( unsigned char ) pSmart->attriVendorData[2],
		( unsigned char ) pSmart->attriVendorData[1],
		( unsigned char ) pSmart->attriVendorData[0] ) ;
	pTemp = cJSON_CreateObject( ) ;
	cJSON_AddStringToObject( pTemp, HDD_SPEC_NAME, SMART_ATTRI_VENDOR ) ;
	cJSON_AddStringToObject( pTemp, HDD_SPEC_SVALUE,  vendorDataStr ) ;
	cJSON_AddItemToArray( pENode, pTemp ) ;
	pTemp = NULL;
	return root;
}


//************************************
// Method:    Parser_CreateHDDSMART
// Access:    public 
// Returns:   cJSON *
//************************************
cJSON * Parser_CreateHDDSMART( hdd_info_t * pHDDInfo ) 
{ 
	/*
	{ "requestID":103,"commCmd":276,"hddSmartInfoList":[{ "hddName":"Disk0-SQF-S25M8-128G-S5C","smartAttrList":[],"hddType":1,"hddIndex":0}]}
	*/
	cJSON* root = NULL;			//root,to be return
	cJSON* rootItem = NULL;		//the item of root, ( the content behind "susiCommData:" ) 
	cJSON* pSmartList = NULL;	//contain "HDDSmartInfo" in a array
	cJSON* pSmartNode = NULL;	//used to temporary save a HDDSmartInfo in a object
	cJSON* pENode = NULL;		//contain lots of element in a array

	root = cJSON_CreateObject( ) ;
	rootItem = cJSON_CreateObject( ) ;
	pSmartList = cJSON_CreateArray( ) ;
	//cJSON_AddItemToObject( root,AGENTINFO_BODY_STRUCT,rootItem ) ;
	//cJSON_AddNumberToObject( rootItem,AGENTINFO_CMDTYPE,devmon_hddinfo_rep ) ;
	//cJSON_AddNumberToObject( rootItem,AGENTINFO_REQID,cagent_request_hdd_monitoring ) ;
	cJSON_AddItemToObject( root, ( const char * ) MyTopic,rootItem ) ;
	cJSON_AddItemToObject( rootItem, HDD_SMART_INFO_LIST, pSmartList ) ;
	if ( pHDDInfo && pHDDInfo->hddMonInfoList ) 
	{ 
		char cTemp[128] = { 0};
		char hddType[32] = { 0};
		hdd_mon_info_node_t * item = NULL;
		item = pHDDInfo->hddMonInfoList->next;
		while ( item !=  NULL ) 
		{ 
			hdd_mon_info_t * pHDDMonInfo = &item->hddMonInfo;
			if ( pHDDMonInfo->hdd_type !=   StdDisk ) { 
				item = item->next;
				continue;
			}
			if ( pHDDMonInfo->smartAttriInfoList ) 
			{ 	
				smart_attri_info_node_t  * curSmartAttri = NULL;
				curSmartAttri = pHDDMonInfo->smartAttriInfoList->next;
				pSmartNode = cJSON_CreateObject( ) ; 
				cJSON_AddItemToArray( pSmartList,  pSmartNode ) ;
				cJSON_AddItemToObject( pSmartNode, SMART_BASEINFO, CreateSmartBaseInfo( pHDDMonInfo  ) ) ;
				while ( curSmartAttri ) 
				{ 
					cJSON * pSmartItem = CreateSmartAttrItem( &curSmartAttri->attriInfo ) ;
					cJSON_AddItemToObject( pSmartNode,( const char * ) & ( curSmartAttri->attriInfo.attriName  ) , pSmartItem ) ;
					curSmartAttri = curSmartAttri->next;
				}
				memset(cTemp, 0, sizeof(cTemp));
				sprintf(cTemp, "Disk%d-%s", pHDDMonInfo->hdd_index, pHDDMonInfo->hdd_name);
				cJSON_AddStringToObject(pSmartNode, HDD_SPEC_BASENAME, cTemp);
				cJSON_AddNumberToObject	( pSmartNode, HDD_SPEC_VERSION, HDD_SPEC_VERSION_DEF ) ;
				//cJSON_AddStringToObject	( pSmartNode, HDD_SPEC_BASM,"R" ) ;
				//cJSON_AddStringToObject	( pSmartNode, HDD_SPEC_BTYPE,"d" ) ;
				//cJSON_AddStringToObject	( pSmartNode, HDD_SPEC_BST,"" ) ;
				//cJSON_AddStringToObject	( pSmartNode, HDD_SPEC_BEXTEND,"" ) ;
			}
			pSmartNode = NULL;
			item = item->next;
		} //while End
	}
	return root;
}


//************************************
// Method:    Parser_CreateInfoSpec
// Access:    public 
// Returns:   cJSON *
//************************************
cJSON * Parser_CreateInfoSpec( hdd_info_t * pHDDInfo ) 
{ 
	cJSON* root = NULL;			//root,to be return
	cJSON* rootItem = NULL;		//the item of root, ( the content behind "susiCommData:" ) 
	cJSON* pHDDList = NULL;		//contain "HDDInfo" in a array
	cJSON* pSmartList = NULL;	//contain "HDDSmartInfo" in a array
	cJSON* pHDDNode = NULL;		//used to temporary save a HDDInfo in a object
	cJSON* pSmartNode = NULL;	//used to temporary save a HDDSmartInfo in a object
	cJSON* pENode = NULL;		//contain lots of element in a array

	root = cJSON_CreateObject( ) ;
	rootItem = cJSON_CreateObject( ) ;
	pHDDList = cJSON_CreateArray( ) ;
	pSmartList = cJSON_CreateArray( ) ;
	cJSON_AddItemToObject( root, ( const char * ) MyTopic,rootItem ) ;
	cJSON_AddItemToObject( rootItem, HDD_INFO_LIST, pHDDList ) ;
	cJSON_AddItemToObject( rootItem, HDD_SMART_INFO_LIST, pSmartList ) ;
	if ( pHDDInfo && pHDDInfo->hddMonInfoList ) 
	{ 
		char cTemp[128] = { 0};
		char hddType[32] = { 0};
		hdd_mon_info_node_t * item = pHDDInfo->hddMonInfoList->next;
		//construct CJSON of HDD Info and S.M.A.R.T. Info, notice that SQFlash and Std HDD are different
		while ( item !=  NULL ) 
		{ 
			hdd_mon_info_t * pHDDMonInfo = &item->hddMonInfo;
			pHDDNode = cJSON_CreateObject( ) ;
			pENode =  cJSON_CreateArray( ) ;
			cJSON_AddItemToObject		( pHDDNode,HDD_SPEC_ELEMENT, pENode ) ;
			memset ( cTemp, 0, sizeof( cTemp  ) ) ;
			sprintf( cTemp, "Disk%d-%s", pHDDMonInfo->hdd_index, pHDDMonInfo->hdd_name ) ;
			cJSON_AddStringToObject		( pHDDNode,HDD_SPEC_BASENAME, cTemp ) ;
			cJSON_AddNumberToObject		( pHDDNode,HDD_SPEC_VERSION, HDD_SPEC_VERSION_DEF ) ;
			cJSON_AddStringToObject		( pHDDNode,HDD_SPEC_ASM, HDD_SPEC_BASM_DEF ) ;
			//cJSON_AddStringToObject		( pHDDNode,HDD_SPEC_BTYPE, HDD_SPEC_BTYPE_DEF ) ;
			//cJSON_AddStringToObject		( pHDDNode,HDD_SPEC_BST, HDD_SPEC_BST_DEF ) ;
			//cJSON_AddStringToObject		( pHDDNode,HDD_SPEC_BEXTEND, HDD_SPEC_BEXTEND_DEF ) ;

			//switch ( hddInfo->hdd_type ) 
			//{ 
			//	case SQFlash	:	sprintf( hddType, "%s", "SQFlash" ) ;	break;
			//	case StdDisk	:	sprintf( hddType, "%s", "STDDisk" ) ;	break;
			//	default:			sprintf( hddType, "%s", "Unknown" ) ;	break;		
			//}
			if ( pHDDMonInfo->hdd_type == SQFlash ) 
			{ 
				//memset(cTemp, 0, sizeof(cTemp));
				//sprintf(cTemp, "%s-%s", HDD_TYPE_SQFLASH, pHDDMonInfo->hdd_name);
				cJSON_AddItemToArray( pENode, CreateENodeItem( HDD_TYPE, HDD_SPEC_SVALUE, 0 ,HDD_TYPE_SQFLASH, NULL  ) ) ;
				cJSON_AddItemToArray(pENode, CreateENodeItem(HDD_NAME, HDD_SPEC_SVALUE, 0, pHDDMonInfo->hdd_name, NULL));
				cJSON_AddItemToArray( pENode, CreateENodeItem( HDD_INDEX, HDD_SPEC_VALUE, pHDDMonInfo->hdd_index, NULL, NULL  ) ) ;
				cJSON_AddItemToArray( pENode, CreateENodeItem( HDD_HEALTH, HDD_SPEC_VALUE, pHDDMonInfo->hdd_health_percent, NULL, SENSOR_UNIT_PERCENT  ) ) ;
				cJSON_AddItemToArray( pENode, CreateENodeItem( HDD_POWER_ON_TIME, HDD_SPEC_VALUE, pHDDMonInfo->power_on_time, NULL, SENSOR_UNIT_HOUR  ) ) ;
				cJSON_AddItemToArray( pENode, CreateENodeItem( HDD_ECC_COUNT, HDD_SPEC_VALUE, pHDDMonInfo->ecc_count, NULL, NULL  ) ) ;
				cJSON_AddItemToArray( pENode, CreateENodeItem( HDD_MAX_PROGRAM, HDD_SPEC_VALUE, pHDDMonInfo->max_program, NULL, NULL  ) ) ;
				cJSON_AddItemToArray( pENode, CreateENodeItem( HDD_AVERAGE_PROGRAM, HDD_SPEC_VALUE, pHDDMonInfo->average_program, NULL, NULL  ) ) ;
				cJSON_AddItemToArray( pENode, CreateENodeItem( HDD_ENDURANCE_CHECK, HDD_SPEC_VALUE, pHDDMonInfo->endurance_check, NULL, NULL  ) ) ;
				cJSON_AddItemToArray( pENode, CreateENodeItem( HDD_GOOD_BLOCK_RATE, HDD_SPEC_VALUE, pHDDMonInfo->good_block_rate, NULL, SENSOR_UNIT_PERCENT  ) ) ;
				cJSON_AddItemToArray( pENode, CreateENodeItem( HDD_MAX_RESERVED_BLOCK, HDD_SPEC_VALUE, pHDDMonInfo->max_reserved_block, NULL, NULL ) ) ;
				cJSON_AddItemToArray( pENode, CreateENodeItem( HDD_CURRENT_RESERVED_BLOCK, HDD_SPEC_VALUE, pHDDMonInfo->current_reserved_block, NULL, NULL  ) ) ;
			}
			else if ( pHDDMonInfo->hdd_type == StdDisk ) 
			{ 
				//memset(cTemp, 0, sizeof(cTemp));
				//sprintf(cTemp, "%s-%s", HDD_TYPE_HDD, pHDDMonInfo->hdd_name);
				cJSON_AddItemToArray( pENode, CreateENodeItem( HDD_TYPE, HDD_SPEC_SVALUE, 0, HDD_TYPE_HDD, NULL  ) ) ;
				cJSON_AddItemToArray( pENode, CreateENodeItem( HDD_NAME, HDD_SPEC_SVALUE, 0, pHDDMonInfo->hdd_name, NULL  ) ) ;
				cJSON_AddItemToArray( pENode, CreateENodeItem( HDD_INDEX, HDD_SPEC_VALUE, pHDDMonInfo->hdd_index, NULL, NULL  ) ) ;
				cJSON_AddItemToArray( pENode, CreateENodeItem( HDD_POWER_ON_TIME, HDD_SPEC_VALUE, pHDDMonInfo->power_on_time, NULL, SENSOR_UNIT_HOUR  ) ) ;
				cJSON_AddItemToArray( pENode, CreateENodeItem( HDD_HEALTH_PERCENT, HDD_SPEC_VALUE, pHDDMonInfo->hdd_health_percent, NULL, SENSOR_UNIT_PERCENT  ) ) ;
				cJSON_AddItemToArray( pENode, CreateENodeItem( HDD_TEMP, HDD_SPEC_VALUE, pHDDMonInfo->hdd_temp, NULL, SENSOR_UNIT_CELSIUS  ) ) ;
			}
			cJSON_AddItemToArray( pHDDList, pHDDNode ) ;
			pHDDNode = NULL;
			pENode = NULL;
			item = item->next;
		}// while End

		//to construct HDD S.M.A.R.T. info
		item = pHDDInfo->hddMonInfoList->next;
		while ( item !=  NULL ) 
		{ 
			hdd_mon_info_t * pHDDMonInfo = &item->hddMonInfo;
			if ( pHDDMonInfo->hdd_type !=   StdDisk ) { 
				item = item->next;
				continue;
			}
			if ( pHDDMonInfo->smartAttriInfoList ) 
			{ 	
				smart_attri_info_node_t  * curSmartAttri = NULL;
				curSmartAttri = pHDDMonInfo->smartAttriInfoList->next;
				pSmartNode = cJSON_CreateObject( ) ; 
				cJSON_AddItemToArray( pSmartList,  pSmartNode ) ;
				cJSON_AddItemToObject( pSmartNode, SMART_BASEINFO, CreateSmartBaseInfo( pHDDMonInfo  ) ) ;
				while ( curSmartAttri ) 
				{ 
					cJSON * pSmartItem = CreateSmartAttrItem( &curSmartAttri->attriInfo ) ;
					cJSON_AddItemToObject( pSmartNode,( const char * ) & curSmartAttri->attriInfo.attriName , pSmartItem ) ;
					curSmartAttri = curSmartAttri->next;
				}
				memset(cTemp, 0, sizeof(cTemp));
				sprintf(cTemp, "Disk%d-%s", pHDDMonInfo->hdd_index, pHDDMonInfo->hdd_name);
				cJSON_AddStringToObject(pSmartNode, HDD_SPEC_BASENAME, cTemp);
				cJSON_AddNumberToObject(pSmartNode, HDD_SPEC_VERSION, HDD_SPEC_VERSION_DEF);
				//cJSON_AddStringToObject	( pSmartNode, HDD_SPEC_BASM,"R" ) ;
				//cJSON_AddStringToObject	( pSmartNode, HDD_SPEC_BTYPE,"d" ) ;
				//cJSON_AddStringToObject	( pSmartNode, HDD_SPEC_BST,"" ) ;
				//cJSON_AddStringToObject	( pSmartNode, HDD_SPEC_BEXTEND,"" ) ;
			}
			item = item->next;
		} //while End
	}
	return root;
}



cJSON * Parser_CreateThrChkRet( bool bResult, char* chkMsg ) 
{ 
   cJSON* root = cJSON_CreateObject( ) ;
   if ( bResult )	cJSON_AddStringToObject( root, THR_CHECK_STATUS, "True" ) ;
   else				cJSON_AddStringToObject( root, THR_CHECK_STATUS, "False" ) ;
   if ( chkMsg ) 	cJSON_AddStringToObject( root, THR_CHECK_MSG, chkMsg ) ;
   return root;
}


//************************************
// Method:   Parser_ParsePathLayer
// Describe: Separate the sensorPath into a Layer array, which will be used in JSON node parsing
// Parameter IN:  const char * path
// Parameter OUT: char * * layer   ( Warring ):	Caller have to free the pointer in layer)
// Parameter OUT: int * cnt
// Date:     2015-4-14
//************************************
bool Parser_ParsePathLayer(const char * path, char *layer[], int *cnt)
{
	int start = 0, end = 0, cur = 0;
	int totalLen=0 ,layerLen = 0;
	totalLen=strlen(path) ;
	*cnt = 0;
	
	for (cur = 1; cur < totalLen + 1; cur++) //include null-terminal
	{
		//== if a layer is complete, then get it!
		if ('/' != path[cur - 1] && '/' == path[cur]     ||
			'/' == path[cur]     && '/' != path[cur + 1] ||
			'\0' == path[cur] )
		{
			end = cur;
			layerLen = end - start;
			if ( layerLen > 0 ) {
				char  * newPtr = (char *)malloc( layerLen + 1);
				if (! newPtr) return false;
				memset(newPtr, 0, layerLen + 1);
				strncpy(newPtr, (path + start), layerLen);
				layer[(*cnt)++] = newPtr;
				if (*cnt >= MAX_PATH_LAYER) return false;
			}
			start = cur + 1;
		}
	}
	return true;
}


//************************************
// Method:    Parser_JointPathLayer
// Describe:  Joint Layer array as a sensorPath,  the path separator is '\'
// Return:    (char *)  return the pointer of path,which have to be free later
// Parameter IN char * * layer = the array of pointer
// Parameter IN: int cnt = the count of array
// Date:      2015-10-28
//************************************
char * Parser_JointPathLayer(char ** layer, int cnt)
{
	char * path = NULL;
	char buf[MAX_PATH_LENGTH]={0};
	int i = 0;
	int offset = 0;
	for (i = 0; i < cnt; ++i) {
		offset += sprintf(buf + offset, "%s/",layer[i]);
	}
	buf[offset-1] = buf[offset] = 0;
	path = (char *)malloc(offset + 1);
	memcpy(path, buf, offset);
	return path;
}



//************************************
// Method:    Parser_SearchPathLayer
// Describe:  locate the position of sensor data in by Searching DataSorceRoot according to the pathLayer,the answer is the cJSON pointer
// Returns:   bool
// Qualifier:
// Parameter InOut : cJSON * * ppTarget			    //== return the pointer of Target,which is in pRetRoot
// Parameter In :    const cJSON * pSourceRoot		//==input cJSON of DataSource, such as pMonitor 
// Parameter In:     const char * const * pathLayer //==search path
// Parameter In:     const int layerCnt
// Time:             2015-6-16 
//************************************
bool Parser_SearchPathLayer(cJSON ** ppTarget, const cJSON * pMonitor, const char * pathLayer[], const int layerCnt)
{
	// check previous ENode, maybe boost the speed
	int i = 0;
	cJSON * pTargetItem = *ppTarget;
	cJSON * pLastObj = (cJSON *)pMonitor;
	cJSON * pTemp = NULL;
	if (pMonitor == NULL) return false;

	//Search from the head of root
	while (i < layerCnt) //layer by layer
	{
		bool flag_continue = false; // flag to jump out multiple loop
		/*== cJSON_Object:search <string> or "e"==
		==== cJSON_Array: search "bn" or "n" ====*/
		switch (pLastObj->type)
		{
			//== cJSON_Object:search <string> or "e"
		case cJSON_Object:
		{
			pTemp = cJSON_GetObjectItem(pLastObj, pathLayer[i]);
			if (pTemp == NULL) {
				pTemp = cJSON_GetObjectItem(pLastObj, HDD_SPEC_ELEMENT);
				if (pTemp == NULL)	return false;
			}
			else {
				if (i < layerCnt -1) i++;
				else if (i == layerCnt -1 ){
					*ppTarget = pTargetItem = pTemp;
					return true;
				}
			}
			pLastObj = pTemp;
			continue;
			break;
		} //end cJSON_Object

		//== cJSON_Array: search "bn" or "n"
		case cJSON_Array:
		{
			cJSON * pItem = NULL;
			for (pItem = pLastObj->child; NULL != pItem; pItem = pItem->next) {
				pTemp = cJSON_GetObjectItem(pItem, HDD_SPEC_BASENAME);					// "bn"
				if (pTemp == NULL) pTemp = cJSON_GetObjectItem(pItem, HDD_SPEC_NAME);	//"n"
				if (pTemp == NULL) return false; // both "bn" and "n" are not found 
				if (0 == strcmp(pTemp->valuestring, pathLayer[i])) {
					if (i < layerCnt - 1) {
						pLastObj = pItem;
						i++;
						flag_continue = true;
						break;
					}
					if (i == layerCnt - 1) {
						*ppTarget = pTargetItem = pItem;
						return true;
					}
				}
			}
			if (flag_continue) continue;
			else return false;// pathLayer[j] is not found
			break;
		}// end case cJSON_Array
		default:
			break;
		}// end switch
	}// End for
	*ppTarget = pTargetItem = NULL;
	return false;
}


//************************************
// Method:  cJSON_DuplicateValueChild
// Detail:  Duplicate item with its ValueChild(the type of child is equal to cJSON_Number or cJOSN_String),
//              It means that the object /Array-type child is not duplicate.
//              This function is used for copy the IPSO base info of a item, which are looks like: "bn"¡¢"n"¡¢"asm"¡¢"version" and so on
// Access:    public 
// Returns:   cJSON *
// Parameter: cJSON * item
//************************************
cJSON * cJSON_DuplicateValueChild(cJSON * item)
{
	cJSON * newItem = cJSON_Duplicate(item, 0);
	cJSON * newChild = NULL;
	cJSON * nptr = NULL;
	cJSON * curChild = item->child;
	while (curChild)
	{
		if ((curChild->type == cJSON_String) || (curChild->type == cJSON_Number)) {
			newChild = cJSON_Duplicate(curChild, 0);
			if (newChild == NULL) { cJSON_Delete(newItem); return NULL; }
			if (nptr == NULL) { newItem->child = newChild; nptr = newChild; }
			else              { nptr->next = newChild; newChild->prev = nptr; nptr = newChild; }
		}
		curChild = curChild->next;
	}
	return newItem;
}


//************************************
// Method:    Parsr_CreateCustomReportByOnePath
// Detail:    To copy cJSON item from SourceRoot to CustomRoot as a copy of a sub-collection of SourceRoot, follow the path pre-defined ,
// Access:    public 
// Returns:   bool
// Parameter INOUT: cJSON * * p   pCustomRoot
// Parameter In:    const cJSON * pSourceRoot
// Parameter IN:    const char *  pathLayer[MAX_PATH_LAYER]
// Parameter IN:    const int  layerCnt
//************************************
bool Parser_CreateCustomReportByOnePath(cJSON ** ppCustomRoot, const cJSON *pMonitor, const char * pathLayer[MAX_PATH_LAYER], const int layerCnt)
{
	bool bRet = true;
	cJSON * rootCopy = *ppCustomRoot;
	cJSON * cur = NULL;
	cJSON * prev = NULL;
	cJSON * tmpTarget = NULL;
	char * pInfo = NULL;
	int i = 0;

	//== Copy the cJSON tree layer by layer as the path defined
	for (i = 0; i < layerCnt; ++i)
	{
		/*== check rootCopy whether exist the target ==
		  == prevent duplicate ==					*/
		bool bExist = Parser_SearchPathLayer(&tmpTarget, rootCopy, pathLayer, i+1);
		if (bExist){
			prev = cur = tmpTarget;
			continue;
		}

		//== get copy from SourceRoot
		cur = NULL;
		bRet = Parser_SearchPathLayer(&tmpTarget, pMonitor, pathLayer, i+1);
		if (!bRet) return bRet;
		if (i == layerCnt - 1) {
			cur = cJSON_Duplicate(tmpTarget, 2);    //duplicate in recurse
		}
		if (i < layerCnt - 1) {
			if (prev ? prev->type == cJSON_Object : 1) {
				cur = cJSON_Duplicate(tmpTarget, 0);
			}
			if (prev ? prev->type == cJSON_Array : 0) {
				cur = cJSON_DuplicateValueChild(tmpTarget);  // only duplicate "e"¡¢"n"¡¢"bn"¡¢"asm"¡¢"version" and so on
			}
		}

		//== add copy to rootCpy
		if (prev == NULL) {
			rootCopy = cJSON_CreateObject();
			cJSON_AddItemToObject(rootCopy,pathLayer[0],cur);
		}
		else {
			if (prev->type == cJSON_Object){
				cJSON_AddItemToObject(prev, pathLayer[i], cur);
			}
			if (prev->type == cJSON_Array) {
				cJSON_AddItemToArray(prev, cur);
			}
		}
		prev = cur;
	}
	*ppCustomRoot= rootCopy;
	return bRet;
}


//************************************
// Method:    Parser_CreateCustomAutoReportOverall

// Parameter INOUT: cJSON * * ppCopyRoot
// Parameter IN:    const cJSON * pSourceRoot
// Parameter IN:    const cJSON * pEnodeOfPathArray
// Parameter OUT:   char * errMsg
//************************************
bool Parser_CreateCustomReportOverall(cJSON ** ppCopyRoot, const cJSON * pSourceRoot, const cJSON * pRequestPathArray, char * errMsg)
{
	bool bRet = true;
	cJSON * pCopyRoot = *ppCopyRoot;
	char * pInfo = NULL;
	char * onePath = NULL;
	char * pathLayer[MAX_PATH_LAYER];
	int layerCnt = 0;
	int i = 0, j = 0;
	int arrayCnt = cJSON_GetArraySize( (cJSON * )pRequestPathArray);

	for (i = 0; i < arrayCnt; ++i)
	{
		onePath = cJSON_GetArrayItem((cJSON * )pRequestPathArray, i)->child->valuestring;
		memset(pathLayer, 0, sizeof(pathLayer));
		Parser_ParsePathLayer(onePath, pathLayer, &layerCnt);
		bRet &= Parser_CreateCustomReportByOnePath(&pCopyRoot, pSourceRoot, pathLayer, layerCnt);
		//	== free pathLayer ==//
		for (j = 0; j < MAX_PATH_LAYER && NULL != pathLayer[j]; ++j) free(pathLayer[j]);	
		// == pack errMsg ==
		if (!bRet)  sprintf(errMsg, "%s\r\nFailed to get value from the path ( %s ) when creating custom auto-report.", errMsg, onePath);
	}
	*ppCopyRoot = pCopyRoot;
	return bRet;
}


//************************************
// Method:    Parser_CreateSensorInfo
// FullName:  Parser_CreateSensorInfo
// Access:    public 
// Returns:   cJSON *
// Qualifier:
// Parameter: cJSON * reqRoot  //the root cJSON of request data, EX: { "susiCommData":XXX};
// Parameter: cJSON * pMonitor //the root cJSON of hddInfoSpec,  EX: {"HDDMonitor":{"hddInfoList":XXX,"hddSmartInfoList":XXX}};
//************************************
cJSON * Parser_CreateSensorInfo(cJSON * pReqRoot, cJSON * pMonitor)
{
	cJSON *pCommData = NULL,  *pSensorIDList = NULL,   *pSessionID1 = NULL, *pEnode1 = NULL, *pEnodeItem1 = NULL;	// cJSON of "request"
	cJSON *pReplyRoot = NULL, *pSensorInfoList = NULL, *pSessionID2 = NULL, *pEnode2 = NULL, *pEnodeItem2 = NULL;	// cJSON of "reply" 
	int i = 0, j = 0, count = 0;
	char * path = NULL;
	char * pathLayer[MAX_PATH_LAYER];

	//== pre work == traverse the cJSON £¨request£¬dataSource£©
	pCommData      = cJSON_GetObjectItem(pReqRoot, AGENTINFO_BODY_STRUCT);
	pSessionID1    = cJSON_GetObjectItem(pCommData, AGENTINFO_SESSIONID);
	pSensorIDList  = cJSON_GetObjectItem(pCommData, AGENTINFO_SENSORIDLIST);
	pEnode1        = cJSON_GetObjectItem(pSensorIDList, HDD_SPEC_ELEMENT);
	count          = cJSON_GetArraySize(pEnode1); if (0 == count) return NULL;	//empty request

	pReplyRoot     = cJSON_CreateObject();
	pSessionID2    = cJSON_Duplicate(pSessionID1, 1); cJSON_AddItemToObject(pReplyRoot, AGENTINFO_SESSIONID, pSessionID2);
	pSensorInfoList= cJSON_CreateObject();            cJSON_AddItemToObject(pReplyRoot, AGENTINFO_SENSORINFOLIST, pSensorInfoList);
	pEnode2        = cJSON_CreateArray();             cJSON_AddItemToObject(pSensorInfoList, HDD_SPEC_ELEMENT, pEnode2);
	
	//===== one by one get sensor data === parse path, get sensor data, compose reply cJSON
	for (i = 0; i < count; i++) 
	{
		int layerCnt = 0;
		cJSON * pTarget = NULL;
		cJSON * pValueItem = NULL;
		//== part 1 == get the path ( parse the request cJSON )
		pEnodeItem1 = cJSON_GetArrayItem(pEnode1, i);
		if (pEnodeItem1) path = pEnodeItem1->child->valuestring; // just set the pointer, (not strcpy)

		//== part 2 == parse the path == locate the dataSource
		memset(pathLayer, 0, MAX_PATH_LAYER);
		if (! Parser_ParsePathLayer(path, pathLayer, & layerCnt)) {
			pEnodeItem2 = cJSON_Duplicate(pEnodeItem1, 1); 
			cJSON_AddStringToObject(pEnodeItem2, HDD_SPEC_SVALUE, STATUS_SYNTAX_ERROR);
			cJSON_AddNumberToObject(pEnodeItem2, STATUSCODE, STATUSCODE_SYNTAX_ERROR);
			cJSON_AddItemToArray(pEnode2, pEnodeItem2);
			pEnodeItem2 = NULL;
		}
		else
		{
			bool bResult = false;
			bResult = Parser_SearchPathLayer(&pTarget, pMonitor, pathLayer, layerCnt);
			//	free pathLayer ==//
			for (j = 0; j < MAX_PATH_LAYER && NULL != pathLayer[j]; j++) free(pathLayer[j]);

			//== part 3 == get the sensor data  ==  compose the reply cJSON
			if (bResult) {
				if (pValueItem = cJSON_GetObjectItem(pTarget, HDD_SPEC_VALUE)) {
					pEnodeItem2 = cJSON_Duplicate(pEnodeItem1, 1); 
					cJSON_AddNumberToObject(pEnodeItem2, HDD_SPEC_VALUE, pValueItem->valueint);
					cJSON_AddNumberToObject(pEnodeItem2, STATUSCODE, STATUSCODE_SUCCESS);
				}
				else if (pValueItem = cJSON_GetObjectItem(pTarget, HDD_SPEC_SVALUE)) {
					pEnodeItem2 = cJSON_Duplicate(pEnodeItem1, 1);
					cJSON_AddStringToObject(pEnodeItem2, HDD_SPEC_SVALUE, pValueItem->valuestring);
					cJSON_AddNumberToObject(pEnodeItem2, STATUSCODE, STATUSCODE_SUCCESS);
				}
				if (pEnodeItem2) {
					cJSON_AddItemToArray(pEnode2, pEnodeItem2);
					pEnodeItem2 = NULL;
				}
			} 
			else {
				//not fount
				pEnodeItem2 = cJSON_Duplicate(pEnodeItem1, 1);
				cJSON_AddStringToObject(pEnodeItem2, HDD_SPEC_SVALUE, STATUS_NOT_FOUND);
				cJSON_AddNumberToObject(pEnodeItem2, STATUSCODE, STATUSCODE_NOT_FOUND);
				cJSON_AddItemToArray(pEnode2, pEnodeItem2);
				pEnodeItem2 = NULL;
			}
		}
	}
	return pReplyRoot;
}

int Parser_GetSensorValueByPath(const char * path, cJSON * pMonitor)
{
	int nRet = DEF_INVALID_VALUE;
	int j = 0, cnt = 0;
	char * pathLayer[MAX_PATH_LAYER] = { NULL };
	cJSON * pTarget = NULL;
	if ( ! Parser_ParsePathLayer(path, pathLayer, &cnt)) return nRet;
	if (Parser_SearchPathLayer(&pTarget, pMonitor, pathLayer, cnt)) {
		cJSON * item = NULL;
		item = cJSON_GetObjectItem(pTarget, HDD_SPEC_VALUE);
		if (item) nRet = item->valueint;
		else      nRet = DEF_INVALID_VALUE;
	}	
	for (j = 0; j < MAX_PATH_LAYER && NULL != pathLayer[j]; j++) free(pathLayer[j]);
	return nRet;
}


void Parser_ConvertGroupRules(cJSON * inRules, cJSON ** pOutRules, struct hdd_thr_node_t * hddThrList)
{
	cJSON * outRules = NULL;
	cJSON * rawItem = NULL;
	cJSON * newItem = NULL;
	hdd_thr_node_t * curNode = NULL;
	char * hddNameArray[MAX_HDD_COUNT] = {NULL};
	int hddCnt = 0;
	int i = 0;

	if ( ! inRules || ! pOutRules ) return ;
	outRules = cJSON_CreateArray();

	for (curNode = hddThrList; curNode != NULL; curNode = curNode->next){
		int index = 0;
		index = atoi((char *)(curNode->hddBaseName + 4));
		hddNameArray[index] = (char *)curNode->hddBaseName;
		hddCnt++;
	}
	
	for (rawItem = inRules->child; rawItem != NULL; rawItem = rawItem->next)
	{
		char * pathLayer[MAX_PATH_LAYER] = { NULL };
		char * oldPath = NULL;
		char * newPath = NULL;
		int layerCnt = 0;
		oldPath = cJSON_GetObjectItem(rawItem, THR_N)->valuestring;
		if (!Parser_ParsePathLayer(oldPath, pathLayer, &layerCnt)) return;
		if (layerCnt < 3 || pathLayer[2] == NULL) return;

		//=== Just copy raw to new, NO conversion
		if (strlen((char *)pathLayer[2]) > strlen("DiskXX")) {
			newItem = cJSON_Duplicate(rawItem, true);
			cJSON_AddItemToArray(outRules, newItem);
		}
			
		//=== convert "All" to full HDDName (1 to cnt)
		else if (0 == strcmp(pathLayer[2], "All"))
		{
			int len   = 0; 
			int index = 0;
			cJSON * tmp = NULL;
			for (index ; index < hddCnt; ++index){
				free(pathLayer[2]);
				len = strlen(hddNameArray[index]) + 1;
				pathLayer[2] = (char *)malloc(len * (sizeof(char)));
				memcpy(pathLayer[2], hddNameArray[index], len);
				newPath = Parser_JointPathLayer(pathLayer, layerCnt);
				newItem = cJSON_Duplicate(rawItem,true);
				tmp = cJSON_GetObjectItem(newItem,THR_N);
				free(tmp->valuestring);
				tmp->valuestring = newPath;
				cJSON_AddItemToArray(outRules, newItem);
			}
		}

		//=== convert "DiskXX" to special HDDName
		else if ( 0 == strncmp(pathLayer[2],"Disk",4) ) 
		{
			cJSON * tmp = NULL;
			int index = 0;
			index = atoi(pathLayer[2]+4);
			if (index < hddCnt)
			{
				int  len = 0;
				free(pathLayer[2]);
				len = strlen(hddNameArray[index]) + 1;
				pathLayer[2] = (char *)malloc(len * (sizeof(char)));
				memcpy(pathLayer[2], hddNameArray[index], len);
				newPath = Parser_JointPathLayer(pathLayer, layerCnt);
				newItem = cJSON_Duplicate(rawItem, true);
				tmp = cJSON_GetObjectItem(newItem,THR_N);
				free(tmp->valuestring);
				tmp->valuestring = newPath;
				cJSON_AddItemToArray(outRules, newItem);
			}	
		}

		//=== free memory
		for (layerCnt = 0; layerCnt < MAX_PATH_LAYER; ++layerCnt) {
			if (pathLayer[layerCnt]) {
				free(pathLayer[layerCnt]);
			}
			else break;
		}
			
	}//finish loop rawRules

	*pOutRules = outRules;// save back
	return;
}




//************************************
// Method:    Parser_DropInvalidRules
// FullName:  Parser_DropInvalidRules
// Access:    public ; only used in set hdd Threshold
// Returns:   void; but the change of *pRuleArray may be write back
// Qualifier:
// Parameter: INOUT:cJSON * * pRuleArray ,the pointer of input actual rules,return valid rules
// Parameter: IN:   cJSON *   pMonitor, the root JSON of InfoSpec
//************************************
void Parser_DropInvalidRules(cJSON ** pRuleArray,cJSON * pMonitor)
{
	cJSON * ruleArray = * pRuleArray;
	cJSON * curRule = NULL;
	cJSON * nextRule = NULL;
	int which = 0;

	for(curRule = ruleArray->child, which = 0; 
		curRule; 
		curRule = nextRule, which++)
	{
		cJSON * item = NULL;
		char * path = NULL;
		int value = 0;
		nextRule = curRule->next;
		//== get the value, do validation
		item = cJSON_GetObjectItem(curRule,THR_N);
		if (item){
			path = item->valuestring;
			value = Parser_GetSensorValueByPath(path, pMonitor);
			if ( DEF_INVALID_VALUE != value ) continue;
		}
		//== drop the invalid rule
		if (path) printf("[%s] > Warrning: Ignore the invalide HDD Threshold rule \"%s\".\r\n", MyTopic, path ? path: "no path");
		cJSON_DeleteItemFromArray(ruleArray,which--);
	}//end for

}
