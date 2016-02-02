/****************************************************************************/
/* Copyright(C) : Advantech Technologies, Inc.														 */
/* Create Date  : 2015/03/06 by Eric Liang															     */
/* Modified Date: 2015/03/06 by Eric Liang															 */
/* Abstract       :  IoT Sensor GW Handler     													             */
/* Reference    : None																									 */
/****************************************************************************/
#ifndef  __ADV_IOT_SENSOR_GW_HANDLER_H__
#define __ADV_IOT_SENSOR_GW_HANDLER_H__

#if defined(__linux)//linux
#define IoTGW_HANDLE_NAME		"IoTSensorHandler.so"
#define SN_MANAGER_LIB_NAME "libIoTSensorManager.so"
#elif _WIN32
#define SN_MANAGER_LIB_NAME "module/SNManagerAPI.dll"
#endif

#define MAX_TOPIC_SIZE 128

#define MAX_SET_RFORMAT_SIZE 256

#define MAX_BUFFER_SIZE 4096
#define MAX_FUNSET_DATA_SIZE 2048

#define MAX_SENNODES  256


typedef enum{
	UNKNOW_CMD										= 0,
	//-----------------------------IoTGW  Control Command Define (521--600)-----------------------
	IOTGW_GET_CAPABILITY_REQUEST     = 521,
	IOTGW_GET_CAPABILITY_REPLY		    = 522,
	IOTGW_GET_SENSOR_REQUEST			= 523,
	IOTGW_GET_SENSOR_REPLY					= 524,
	IOTGW_SET_SENSOR_REQUEST			= 525,
	IOTGW_SET_SENSOR_REPLY					= 526,
		
	IOTGW_SET_THR_REQUEST				   	= 527,
	IOTGW_SET_THR_REPLY							= 528,
	IOTGW_DEL_THR_REQUEST				    = 529,
	IOTGW_DEL_THR_REPLY							= 530,
	
	IOTGW_THR_CHECK_REPLY					= 532,
	
	
	IOTGW_GET_SENSORS_ERROR_REPLY  = 598,
	IOTGW_ERROR_REPLY								 = 600,
	//-----------------------------------------------------------------------------------------
}IOTGW_CMD_ID;


// AsynSetParam
typedef struct AsynSetParam {
	int		index;
	char     szUID[64];
	char     szTopic[MAX_TOPIC_SIZE];
	char     szSetFormat[MAX_SET_RFORMAT_SIZE];   // { "sessionID":"XXX", "sensorInfoList":{"e":[ {"n":"SenData/dout", %s, "StatusCode": %d } ] } } => { "sessionID":"XXX", "sensorInfoList":{"e":[ {"n":"SenData/dout", "sv":"Setting", "StatusCode": 202 } ] } }
} AsynSetParam;


int	 InitSNGWHandler();
void UnInitSNGWHandler();
int    StartSNGWHandler();
int    StopSNGWHandler();

typedef int  (*SendMsgCbf) (  const char* Data, unsigned int const DataLen, void *pRev1, void* pRev2 ); 




#endif // __ADV_IOT_SENSOR_GW_HANDLER_H__
																				


