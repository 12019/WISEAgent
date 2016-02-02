/****************************************************************************/
/* Copyright(C) : Advantech Technologies, Inc.														 */
/* Create Date  : 2014/06/18 by Eric Liang															     */
/* Modified Date: 2014/06/27 by Eric Liang															 */
/* Abstract     : Handler API                                     													*/
/* Reference    : None																									 */
/****************************************************************************/
//#include <stdio.h>
//#include <dlfcn.h>
//#include <unistd.h>
//#include <string.h>
//#include <malloc.h>
//#include "stdafx.h"
//#include "platform.h"
//#include "common.h"

//#include <stdio.h>
//#include <tchar.h>
//#include <windows.h>
//#include <sys/types.h>
//#include <cJSON.h>

#include "Modbus_Handler.h"
#include <string.h>
#include <time.h>

#include "wrapper.h"
#include "pthread.h"
#include "util_path.h"
#include "unistd.h"
#include "susiaccess_handler_api.h"
//#include "DeviceMessageGenerate.h"
#include "IoTMessageGenerate.h"
//#include "Modbus_Handler.h"
#include "Modbus_HandlerLog.h"
#include "Modbus_Parser.h"
#include "ReadINI.h"





//-----------------------------------------------------------------------------
// Defines:
//-----------------------------------------------------------------------------
#define DEF_HANDLER_NAME	"Modbus_Handler"
#define DEF_SIMULATOR_NAME	"Simulator"

#define INI_PATH "\Modbus.ini"

#define cagent_request_custom 2002
#define cagent_custom_action 30002
#define SET_STR_LENGTH 200

//-----------------------------------------------------------------------------
//----------------------sensor info item list function define------------------
//-----------------------------------------------------------------------------
static sensor_info_list CreateSensorInfoList();
static void DestroySensorInfoList(sensor_info_list sensorInfoList);
static int InsertSensorInfoNode(sensor_info_list sensorInfoList, sensor_info_t * pSensorInfo);
static sensor_info_node_t * FindSensorInfoNodeWithID(sensor_info_list sensorInfoList, int id);
static int DeleteSensorInfoNodeWithID(sensor_info_list sensorInfoList, int id);
static int DeleteAllSensorInfoNode(sensor_info_list sensorInfoList);
static bool IsSensorInfoListEmpty(sensor_info_list sensorInfoList);
//-----------------------------------------------------------------------------




//-----------------------------------------------------------------------------
// Internal Prototypes:
//-----------------------------------------------------------------------------
//

typedef struct{
   pthread_t threadHandler;
   bool isThreadRunning;
}handler_context_t;



typedef struct report_data_params_t{
	unsigned int intervalTimeMs;
	unsigned int continueTimeMs;
	char repFilter[4096];
}report_data_params_t;

static report_data_params_t AutoUploadParams;
static report_data_params_t AutoReportParams;


//-----------------------------------------------------------------------------
// Variables
//-----------------------------------------------------------------------------
static void* g_loghandle = NULL;

static Handler_info  g_PluginInfo;
static handler_context_t g_RetrieveContex;
static handler_context_t g_AutoReportContex;
static handler_context_t g_AutoUploadContex;
//clock_t AutoUpload_start;
//clock_t AutoUpload_continue;
static HANDLER_THREAD_STATUS g_status = handler_status_no_init;
static bool g_bRetrieve = false;
static bool g_bAutoReport = false;
static bool g_bAutoUpload = false;
static time_t g_monitortime;
static HandlerSendCbf  g_sendcbf = NULL;						// Client Send information (in JSON format) to Cloud Server	
static HandlerSendCustCbf  g_sendcustcbf = NULL;			    // Client Send information (in JSON format) to Cloud Server with custom topic	
static HandlerSubscribeCustCbf g_subscribecustcbf = NULL;
static HandlerAutoReportCbf g_sendreportcbf = NULL;				// Client Send report (in JSON format) to Cloud Server with AutoReport topic
static HandlerSendCapabilityCbf g_sendcapabilitycbf = NULL;		
static HandlerSendEventCbf g_sendeventcbf = NULL;

static char * CurIotDataJsonStr = NULL;
//-----------------------------------------------------------------------------
// Types
//-----------------------------------------------------------------------------
const char strPluginName[MAX_TOPIC_LEN] = {"Modbus_Handler"};
const int iRequestID = cagent_request_custom;
const int iActionID = cagent_custom_action;
MSG_CLASSIFY_T *g_Capability = NULL;

bool bIsSimtor=false;
bool bFind=false; //Find INI file
char WISE_Name[20];
char Modbus_Slave_IP[16]; 
int Modbus_Slave_Port=502;
modbus_t *ctx=NULL;
int numberOfDI=0;
int numberOfAI=0;
int numberOfDO=0;
int numberOfAO=0;
WISE_Data *DI;
WISE_Data *AI;
WISE_Data *DO;
WISE_Data *AO;
uint8_t *DI_Bits;
uint8_t  *DO_Bits;
uint16_t *AI_Regs;
uint16_t *AO_Regs;
time_t Start_Upload;
time_t Continue_Upload;
//-----------------------------------------------------------------------------
// AutoReport:
//-----------------------------------------------------------------------------
cJSON *Report_root=NULL;
cJSON *Stop_Report_root=NULL;
int Report_array_size=0, Report_interval=0;
bool Report_Reply_All=false;
char **Report_Data_paths=NULL;

cJSON *Report_item, *Report_it, *Report_js_name, *Report_first,*Report_second_interval,*Report_second,*Report_third,*Report_js_list;

//-----------------------------------------------------------------------------
// Function:
//-----------------------------------------------------------------------------
void Handler_Uninitialize();
bool Modbus_Rev();
#ifdef _MSC_VER
BOOL WINAPI DllMain(HINSTANCE module_handle, DWORD reason_for_call, LPVOID reserved)
{
	if (reason_for_call == DLL_PROCESS_ATTACH) // Self-explanatory
	{
		printf("DllInitializer\r\n");
		DisableThreadLibraryCalls(module_handle); // Disable DllMain calls for DLL_THREAD_*
		if (reserved == NULL) // Dynamic load
		{
			// Initialize your stuff or whatever
			// Return FALSE if you don't want your module to be dynamically loaded
		}
		else // Static load
		{
			// Return FALSE if you don't want your module to be statically loaded
			return FALSE;
		}
	}

	if (reason_for_call == DLL_PROCESS_DETACH) // Self-explanatory
	{
		printf("DllFinalizer\r\n");
		if (reserved == NULL) // Either loading the DLL has failed or FreeLibrary was called
		{
			// Cleanup
			Handler_Uninitialize();
		}
		else // Process is terminating
		{
			// Cleanup
			Handler_Uninitialize();
		}
	}
	return TRUE;
}
#else
__attribute__((constructor))
/**
 * initializer of the shared lib.
 */
static void Initializer(int argc, char** argv, char** envp)
{
    printf("DllInitializer\r\n");
}

__attribute__((destructor))
/** 
 * It is called when shared lib is being unloaded.
 * 
 */
static void Finalizer()
{
    printf("DllFinalizer\r\n");
	Handler_Uninitialize();
}
#endif

static sensor_info_list CreateSensorInfoList()
{
	sensor_info_node_t * head = NULL;
	head = (sensor_info_node_t *)malloc(sizeof(sensor_info_node_t));
	if(head)
	{
		memset(head, 0, sizeof(sensor_info_node_t));
		head->next = NULL;
	}
	return head;
}

static void DestroySensorInfoList(sensor_info_list sensorInfoList)
{
	if(NULL == sensorInfoList) return;
	DeleteAllSensorInfoNode(sensorInfoList);
	free(sensorInfoList); 
	sensorInfoList = NULL;
}

static int DeleteAllSensorInfoNode(sensor_info_list sensorInfoList)
{
	int iRet = -1;
	sensor_info_node_t * delNode = NULL, *head = NULL;
	int i = 0;
	if(sensorInfoList == NULL) return iRet;
	head = sensorInfoList;
	delNode = head->next;
	while(delNode)
	{
		head->next = delNode->next;
		if(delNode->sensorInfo.jsonStr != NULL)
		{
			free(delNode->sensorInfo.jsonStr);
			delNode->sensorInfo.jsonStr = NULL;
		}
		if(delNode->sensorInfo.pathStr != NULL)
		{
			free(delNode->sensorInfo.pathStr);
			delNode->sensorInfo.pathStr = NULL;
		}
		free(delNode);
		delNode = head->next;
	}

	iRet = 0;
	return iRet;
}

//==========================================================
bool read_INI_Platform(char *modulePath,char *iniPath)
{

	FILE *fPtr;
     
	// Load ini file
	util_module_path_get(modulePath);
	util_path_combine(iniPath,modulePath,INI_PATH);

	//printf("iniFile: %s\n",iniPath);

    fPtr = fopen(iniPath, "r");
    if (fPtr) {
        printf("INI Open Success...\n");
		//GetPrivateProfileString("Platform", "Name", "", WISE_Name, sizeof(WISE_Name), iniPath);
		strcpy(WISE_Name,GetIniKeyString("Platform","Name",iniPath));
		//GetPrivateProfileString("Platform", "SlaveIP", "", Modbus_Slave_IP, sizeof(Modbus_Slave_IP), iniPath);
		strcpy(Modbus_Slave_IP,GetIniKeyString("Platform","SlaveIP",iniPath));
		Modbus_Slave_Port=GetIniKeyInt("Platform","SlavePort",iniPath);

		if(strcmp(WISE_Name,DEF_SIMULATOR_NAME)==0)
		{
			//printf("bIsSimtor=true;");		
			bIsSimtor=true;
		}
		else
		{
			printf("bIsSimtor=false;");	
			bIsSimtor=false;
		}
        fclose(fPtr);
		return true;
    }
    else {
        printf("INI Open Fail...\n");
		return false;
    }


}

bool read_INI()
{
	
	char modulePath[200]={0};
	char iniPath[200]={0};
	char str[100]={0};
	char *pstr;

	FILE *fPtr;

	// Load ini file
	util_module_path_get(modulePath);
	util_path_combine(iniPath,modulePath,INI_PATH);
	//printf("iniFile: %s\n",iniPath);
    fPtr = fopen(iniPath, "r");

	if (fPtr) {				
									printf("INI Open Success...\n");
									#pragma region Find_INI
									//--------------------DI
									//numberOfDI=GetPrivateProfileInt("Digital Input", "numberOfDI", 0, iniPath); 
									numberOfDI=GetIniKeyInt("Digital Input","numberOfDI",iniPath);
									if(numberOfDI!=0)
									{	
										DI_Bits=(uint8_t *)calloc(numberOfDI,sizeof(uint8_t));
										DI=(WISE_Data *)calloc(numberOfDI,sizeof(WISE_Data));
									}
									for(int i=0;i<numberOfDI;i++){
										char strNumberOfDI[5];
										sprintf(strNumberOfDI, "DI%d", i);
										//GetPrivateProfileString("Digital Input", strNumberOfDI, "", str, sizeof(str), iniPath); 			
										strcpy(str,GetIniKeyString("Digital Input",strNumberOfDI,iniPath));

										pstr=strtok(str, ",");
										if (pstr!=NULL)
											DI[i].address = atoi(pstr);//get the address
										else
											DI[i].address=0;

										pstr=strtok(NULL, ",");
										if (pstr!=NULL)
											strcpy(DI[i].name,pstr); // get the name
										else
											strcpy(DI[i].name,""); 

										printf("%d. DI address: %d name: %s\n",i,DI[i].address,DI[i].name);
									}

									//--------------------DO
									//numberOfDO=GetPrivateProfileInt("Digital Output", "numberOfDO", 0, iniPath); 
									numberOfDO=GetIniKeyInt("Digital Output","numberOfDO",iniPath);
									if(numberOfDO!=0)
									{
										DO_Bits=(uint8_t *)calloc(numberOfDO,sizeof(uint8_t));
										DO=(WISE_Data *)calloc(numberOfDO,sizeof(WISE_Data));	
									}
									for(int i=0;i<numberOfDO;i++){
										char strNumberOfDO[5];
										sprintf(strNumberOfDO, "DO%d", i);
										//GetPrivateProfileString("Digital Output", strNumberOfDO, "", str, sizeof(str), iniPath); 			
										strcpy(str,GetIniKeyString("Digital Output",strNumberOfDO,iniPath));

										pstr=strtok(str, ",");
										if (pstr!=NULL)
											DO[i].address = atoi(pstr);//get the address
										else
											DO[i].address=0;

										pstr=strtok(NULL, ",");
										if (pstr!=NULL)
											strcpy(DO[i].name,pstr); // get the name
										else
											strcpy(DO[i].name,""); 

										printf("%d. DO address: %d name: %s\n",i,DO[i].address,DO[i].name);
									}

									//--------------------AI
									pstr=NULL;
									//numberOfAI=GetPrivateProfileInt("Analog Input", "numberOfAI", 0, iniPath); 
									numberOfAI=GetIniKeyInt("Analog Input","numberOfAI",iniPath);
									if(numberOfAI!=0)
									{	
										AI_Regs=(uint16_t *)calloc(numberOfAI,sizeof(uint16_t));	
										AI=(WISE_Data *)calloc(numberOfAI,sizeof(WISE_Data));
									}
									for(int i=0;i<numberOfAI;i++){
										char strNumberOfAI[5];
										sprintf(strNumberOfAI, "AI%d", i);
										//GetPrivateProfileString("Analog Input", strNumberOfAI, "", str, sizeof(str), iniPath); 		
										strcpy(str,GetIniKeyString("Analog Input",strNumberOfAI,iniPath));

										pstr=strtok(str, ",");
										if (pstr!=NULL)
											AI[i].address = atoi(pstr);//get the address
										else
											AI[i].address=0;

										pstr=strtok(NULL, ",");
										if (pstr!=NULL)
											strcpy(AI[i].name,pstr); // get the name
										else
											strcpy(AI[i].name,""); 

										pstr=strtok(NULL, ",");
										if (pstr!=NULL)
											AI[i].min=atof(pstr); // get the min
										else
											AI[i].min=0;

										pstr=strtok(NULL, ",");
										if (pstr!=NULL)
											AI[i].max=atof(pstr); // get the max
										else
											AI[i].max=0; 

										pstr=strtok(NULL, ",");
										if (pstr!=NULL)
											AI[i].precision=atof(pstr); // get the precision
										else
											AI[i].precision=0; 

										pstr=strtok(NULL, ",");
										if (pstr!=NULL)
											strcpy(AI[i].unit,pstr); // get the unit
										else
											strcpy(AI[i].unit,""); 

										printf("%d. AI address: %d name: %s min: %lf max: %lf pre: %lf unit: %s\n",i,AI[i].address,AI[i].name,AI[i].min,AI[i].max,AI[i].precision,AI[i].unit);
									}

									//--------------------AO
									pstr=NULL;
									//numberOfAO=GetPrivateProfileInt("Analog Output", "numberOfAO", 0, iniPath); 
									numberOfAO=GetIniKeyInt("Analog Output","numberOfAO",iniPath);
									if(numberOfAO!=0)
									{	
										AO_Regs=(uint16_t *)calloc(numberOfAO,sizeof(uint16_t));	
										AO=(WISE_Data *)calloc(numberOfAO,sizeof(WISE_Data));
									}
									for(int i=0;i<numberOfAO;i++){
										char strNumberOfAO[5];
										sprintf(strNumberOfAO, "AO%d", i);
										//GetPrivateProfileString("Analog Output", strNumberOfAO, "", str, sizeof(str), iniPath); 	
										strcpy(str,GetIniKeyString("Analog Output",strNumberOfAO,iniPath));

										pstr=strtok(str, ",");
										if (pstr!=NULL)
											AO[i].address = atoi(pstr);//get the address
										else
											AO[i].address=0;

										pstr=strtok(NULL, ",");
										if (pstr!=NULL)
											strcpy(AO[i].name,pstr); // get the name
										else
											strcpy(AO[i].name,""); 

										pstr=strtok(NULL, ",");
										if (pstr!=NULL)
											AO[i].min=atof(pstr); // get the min
										else
											AO[i].min=0;

										pstr=strtok(NULL, ",");
										if (pstr!=NULL)
											AO[i].max=atof(pstr); // get the max
										else
											AO[i].max=0; 

										pstr=strtok(NULL, ",");
										if (pstr!=NULL)
											AO[i].precision=atof(pstr); // get the precision
										else
											AO[i].precision=0; 

										pstr=strtok(NULL, ",");
										if (pstr!=NULL)
											strcpy(AO[i].unit,pstr); // get the unit
										else
											strcpy(AO[i].unit,""); 
										printf("%d. AO address: %d name: %s min: %lf max: %lf pre: %lf unit: %s\n",i,AO[i].address,AO[i].name,AO[i].min,AO[i].max,AO[i].precision,AO[i].unit);
									}
									#pragma endregion Find_INI
									fclose(fPtr);
									return true;
								

	}
    else {
        printf("INI Open Fail...\n");
		return false;
    }


}


MSG_CLASSIFY_T * CreateCapability()
{
	MSG_CLASSIFY_T *myCapability = IoT_CreateRoot((char*) strPluginName);
	MSG_CLASSIFY_T *myGroup;
	MSG_ATTRIBUTE_T* attr;
	IoT_READWRITE_MODE mode=IoT_READONLY;

	int ret=false;
    char Slave_Port[6];
	
	if(bFind)
	{			if(!bIsSimtor)
					ret=Modbus_Rev();
				else
					ret=true;
				sprintf(Slave_Port,"%d",Modbus_Slave_Port);

				myGroup = IoT_AddGroup(myCapability,"Platform");
				
				if(myGroup)
				{	mode=IoT_READONLY;
					attr = IoT_AddSensorNode(myGroup, "Name");
					if(attr)
						IoT_SetStringValue(attr,WISE_Name,mode);

					attr = IoT_AddSensorNode(myGroup, "SlaveIP");
					if(attr)
						IoT_SetStringValue(attr,Modbus_Slave_IP,mode);

					attr = IoT_AddSensorNode(myGroup, "SlavePort");
					if(attr)
						IoT_SetStringValue(attr,Slave_Port,mode);

					attr = IoT_AddSensorNode(myGroup, "Connection");
					if(attr)
					{
						if(bIsSimtor)
						{
							IoT_SetBoolValue(attr,true,mode);
						}
						else
						{
							if(!ret)
								IoT_SetBoolValue(attr,false,mode);
							else
								IoT_SetBoolValue(attr,true,mode);
						}
								
					}	
				}

				
				if(numberOfDI!=0)
				{	
					myGroup = IoT_AddGroup(myCapability, "Digital Input");
					if(myGroup)
					{
							mode=IoT_READONLY;
							for(int i=0;i<numberOfDI;i++){
								if(bIsSimtor)
								{
									DI_Bits[i]=rand()%2;
								}
								else
								{
									if(!ret)
										DI_Bits[i]=false;
								}
								attr = IoT_AddSensorNode(myGroup, DI[i].name);
								if(attr)
								{
									DI[i].Bits=DI_Bits[i];
									IoT_SetBoolValue(attr,DI[i].Bits,mode);	
								}
							}
					}

				}
				
				if(numberOfDO!=0)
				{
					myGroup = IoT_AddGroup(myCapability, "Digital Output");
					if(myGroup)
					{
							mode=IoT_READWRITE;
							for(int i=0;i<numberOfDO;i++){
								if(bIsSimtor)
								{
									DO_Bits[i]=rand()%2;
								}
								else
								{
									if(!ret)
										DO_Bits[i]=false;
								}
								attr = IoT_AddSensorNode(myGroup, DO[i].name);
								if(attr)
								{
									DO[i].Bits=DO_Bits[i];
									IoT_SetBoolValue(attr,DO[i].Bits,mode);	
								}
							}
					}

				}

				if(numberOfAI!=0)
				{
					myGroup = IoT_AddGroup(myCapability, "Analog Input");
					if(myGroup)
					{
							mode=IoT_READONLY;
							for(int i=0;i<numberOfAI;i++){
								if(bIsSimtor)
								{
									while(true)
									{
										AI_Regs[i]=rand()%(int)AI[i].max/AI[i].precision;
										if(AI_Regs[i]>=AI[i].min)
											break;
										else
											continue;
									}								
								}
								else
								{
									if(!ret)
										AI_Regs[i]=0;
								}

								attr = IoT_AddSensorNode(myGroup, AI[i].name);
								if(attr)
								{	
									AI[i].Regs=AI_Regs[i];
									IoT_SetDoubleValueWithMaxMin(attr,AI[i].Regs*AI[i].precision,mode,AI[i].max,AI[i].min,AI[i].unit);
								}
							}
					}

				}

				if(numberOfAO!=0)
				{
					myGroup = IoT_AddGroup(myCapability, "Analog Output");
					if(myGroup)
					{
							mode=IoT_READWRITE;
							for(int i=0;i<numberOfAO;i++){
								if(bIsSimtor)
								{
									while(true)
									{
										AO_Regs[i]=rand()%(int)AO[i].max/AO[i].precision;
										if(AO_Regs[i]>=AO[i].min)
											break;
										else
											continue;
									}								
								}
								else
								{
									if(!ret)
										AO_Regs[i]=0;
								}
								attr = IoT_AddSensorNode(myGroup, AO[i].name);
								if(attr)
								{	
									AO[i].Regs=AO_Regs[i];
									IoT_SetDoubleValueWithMaxMin(attr,AO[i].Regs*AO[i].precision,mode,AO[i].max,AO[i].min,AO[i].unit);

								}
							}
					}

				}
				
				//printf("CreateCapability........\n");
	}

	return myCapability;
}


//--------------------------------------------------------------------------------------------------------------
//------------------------------------------------Modbus--------------------------------------------------------
//--------------------------------------------------------------------------------------------------------------

bool Modbus_Connect()
{

	if (modbus_connect(ctx) == -1) {
		printf("Modbus Connection failed!!\n");
		return false;
	}
	else
	{	printf("Modbus Connection established!!\n");
		return true;
	}

}

bool Modbus_Disconnect()
{
	modbus_close(ctx);
	printf("Modbus Disconnection!!\n");
	return false;
}

bool Modbus_Rev()
{

	int ret=0;
	int i=0;

	if(numberOfDI!=0)
	{	for(i=0;i<numberOfDI;i++)
		{
			ret = modbus_read_input_bits(ctx, DI[0].address, numberOfDI, DI_Bits);
			if (ret == -1) {
				printf("modbus_read_input_bits failed\n");	
				Modbus_Disconnect();
				Modbus_Connect();
				return false;
			}
			else{
				
					//printf("DI_Bits[%d] : %d\n",i,DI_Bits[i]);
			}
		}
	}

	if(numberOfDO!=0)
	{	
		for(i=0;i<numberOfDO;i++)
		{   
			ret = modbus_read_bits(ctx, DO[0].address, numberOfDO, DO_Bits);
			if (ret == -1) {
				printf("modbus_read_bits failed\n");
				Modbus_Disconnect();
				Modbus_Connect();
				return false;
			}
			else{
				
					//printf("DO_Bits[%d] : %d\n",i,DO_Bits[i]);
			}
			
		}
	}

	if(numberOfAI!=0)
	{	
		for(i=0;i<numberOfAI;i++)
		{
			ret = modbus_read_input_registers(ctx, AI[0].address, numberOfAI, AI_Regs);
			if (ret == -1) {
				printf("modbus_read_input_registers failed\n");
				Modbus_Disconnect();
				Modbus_Connect();
				return false;
			}
			else{

					//printf("AI_Regs[%d] : %d\n",i,AI_Regs[i]);
			}
		}
	}

	if(numberOfAO!=0)
	{	
		for(i=0;i<numberOfAO;i++)
		{
			ret = modbus_read_registers(ctx, AO[0].address, numberOfAO, AO_Regs);
			if (ret == -1) {
				printf("modbus_read_registers failed\n");
				Modbus_Disconnect();
				Modbus_Connect();
				return false;
			}
			else{

					//printf("AO_Regs[%d] : %d\n",i,AO_Regs[i]);
			}
		}
	}
	
	return true;

}

//--------------------------------------------------------------------------------------------------------------
//------------------------------------------------SensorInfo----------------------------------------------------
//--------------------------------------------------------------------------------------------------------------

static bool IsSensorInfoListEmpty(sensor_info_list sensorInfoList)
{
	bool bRet = TRUE;
	sensor_info_node_t * curNode = NULL, *head = NULL;
	if(sensorInfoList == NULL) return bRet;
	head = sensorInfoList;
	curNode = head->next;
	if(curNode != NULL) bRet = FALSE;
	return bRet;
}

//---------------------------------------------------------------------------------------------------------------------
//------------------------------------------------CAPABILITY_GET_SET_UPLOAD-------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------
static void GetCapability()
{
	char* result = NULL;
	MSG_CLASSIFY_T *t_Capability = NULL;
	int len = 0;
		
	t_Capability=CreateCapability();

	if(t_Capability)
	{
		result = IoT_PrintCapability(t_Capability);
		len=strlen(result);
		if(len!=0)
			g_sendcbf(&g_PluginInfo, modbus_get_capability_rep, result, strlen(result)+1, NULL, NULL);

		//printf("GetCapability = %s\n",result);
		//printf("---------------------\n");

		IoT_ReleaseAll(t_Capability);
		t_Capability = NULL;
	}
	else
	{
		char * errorRepJsonStr = NULL;
		char errorStr[128];
		sprintf(errorStr, "Command(%d), Get capability error!", modbus_get_capability_req);
		int jsonStrlen = Parser_PackModbusError(errorStr, &errorRepJsonStr);
		if(jsonStrlen > 0 && errorRepJsonStr != NULL)
		{
			g_sendcbf(&g_PluginInfo, modbus_error_rep, errorRepJsonStr, strlen(errorRepJsonStr)+1, NULL, NULL);
		}
		if(errorRepJsonStr)free(errorRepJsonStr);
	}

	if(result)
		free(result);



}
static void GetSensorsDataEx(sensor_info_list sensorInfoList, char * pSessionID)
{
    int Num_Sensor=1000; // to avoid exceeding 
	WISE_Sensor *sensors=(WISE_Sensor *)calloc(Num_Sensor,sizeof(WISE_Sensor));
	MSG_CLASSIFY_T  *pSensorInofList=NULL,*pEArray=NULL, *pSensor=NULL;
	MSG_CLASSIFY_T  *pRoot=NULL;
	MSG_ATTRIBUTE_T	*attr;

	char * repJsonStr = NULL;
	int count=0;

	#pragma region IsSensorInfoListEmpty
	if(!IsSensorInfoListEmpty(sensorInfoList))
	{
		sensor_info_node_t *curNode = NULL;
		curNode = sensorInfoList->next;
	
				#pragma region pRoot
				pRoot = MSG_CreateRoot();
				if(pRoot)
				{
						attr = MSG_AddJSONAttribute(pRoot, "sessionID");
						if(attr)
							MSG_SetStringValue( attr, pSessionID, NULL); 

						pSensorInofList = MSG_AddJSONClassify(pRoot, "sensorInfoList", NULL, false);
						if(pSensorInofList)
							pEArray=MSG_AddJSONClassify(pSensorInofList,"e",NULL,true);
						while(curNode)
						{

								#pragma region pSensorInofList
								if(pSensorInofList)
								{		
										#pragma region pEArray		
										if(pEArray)
										{
													#pragma region pSensor
													pSensor = MSG_AddJSONClassify(pEArray, "sensor", NULL, false);	
													if(pSensor)
													{		attr = MSG_AddJSONAttribute(pSensor, "n");
															if(attr)
																MSG_SetStringValue(attr, curNode->sensorInfo.pathStr, NULL);
															if(count<Num_Sensor)
																Modbus_General_Node_Parser(curNode,sensors,count);
															//printf("\ntype: %s  name: %s\n",sensors[count].type,sensors[count].name);	
															//printf("path = %s\n",curNode->sensorInfo.pathStr);
															#pragma region MSG_Find_Sensor
															if(MSG_Find_Sensor(g_Capability,curNode->sensorInfo.pathStr))
															{		
																	if(attr)
																	{	
																			#pragma region Simulator
																			if(bIsSimtor)
																			{
																									#pragma region Platform-DI-DO-AI-AO
																									int i=0;
																									if(strcmp(sensors[count].type,"Platform")==0)
																									{
																										if(strcmp(sensors[count].name,"Name")==0)
																										{
																											attr = MSG_AddJSONAttribute(pSensor, "sv");
																											if(attr)
																												if(MSG_SetStringValue(attr, WISE_Name, "r"))
																												{	attr = MSG_AddJSONAttribute(pSensor, "StatusCode");
																													if(attr)
																														MSG_SetDoubleValue(attr, IOT_SGRC_SUCCESS,"r", NULL);																			
																												}
																										}
																										if(strcmp(sensors[count].name,"SlaveIP")==0)
																										{
																											attr = MSG_AddJSONAttribute(pSensor, "sv");
																											if(attr)
																												if(MSG_SetStringValue(attr, Modbus_Slave_IP, "r"))
																												{	attr = MSG_AddJSONAttribute(pSensor, "StatusCode");
																													if(attr)
																														MSG_SetDoubleValue(attr, IOT_SGRC_SUCCESS,"r", NULL);																			
																												}
																										}
																										if(strcmp(sensors[count].name,"SlavePort")==0)
																										{
																											attr = MSG_AddJSONAttribute(pSensor, "sv");
																											if(attr)
																											{
																												char temp[6];
																												sprintf(temp,"%d",Modbus_Slave_Port);
																												if(MSG_SetStringValue(attr, temp, "r"))
																												{	attr = MSG_AddJSONAttribute(pSensor, "StatusCode");
																													if(attr)
																														MSG_SetDoubleValue(attr, IOT_SGRC_SUCCESS,"r", NULL);																			
																												}
																										
																											}	
																										}
																										if(strcmp(sensors[count].name,"Connection")==0)
																										{
																											attr = MSG_AddJSONAttribute(pSensor, "sv");
																											if(attr)
																												if(MSG_SetBoolValue(attr, true, "r"))
																												{	attr = MSG_AddJSONAttribute(pSensor, "StatusCode");
																													if(attr)
																														MSG_SetDoubleValue(attr, IOT_SGRC_SUCCESS,"r", NULL);																			
																												}
																										}

																									}
																									if(strcmp(sensors[count].type,"Digital Input")==0)
																									{
																										for(i=0;i<numberOfDI;i++)
																										{
																											if(strcmp(DI[i].name,sensors[count].name)==0)
																											{	
																												DI[i].Bits=rand()%2;
																												attr = MSG_AddJSONAttribute(pSensor, "bv");
																												if(attr)
																													if(MSG_SetBoolValue(attr, DI[i].Bits, "r"))
																													{	attr = MSG_AddJSONAttribute(pSensor, "StatusCode");
																														if(attr)
																															MSG_SetDoubleValue(attr, IOT_SGRC_SUCCESS,"r", NULL);																			
																													}	
																											}
																											if(i==numberOfDI)
																											{	
																												attr = MSG_AddJSONAttribute(pSensor, "sv");
																												if(attr)
																													MSG_SetStringValue(attr,IOT_SGRC_STR_NOT_FOUND,"r");
																												attr = MSG_AddJSONAttribute(pSensor, "StatusCode");
																												if(attr)
																													MSG_SetDoubleValue(attr, IOT_SGRC_NOT_FOUND,"r", NULL);	
																											}
																										}
																									}
																									if(strcmp(sensors[count].type,"Digital Output")==0)
																									{
																										for(i=0;i<numberOfDO;i++)
																										{
																											if(strcmp(DO[i].name,sensors[count].name)==0)
																											{	
																												DO[i].Bits=rand()%2;									
																												attr = MSG_AddJSONAttribute(pSensor, "bv");
																												if(attr)
																													if(MSG_SetBoolValue(attr, DO[i].Bits, "rw"))
																													{	attr = MSG_AddJSONAttribute(pSensor, "StatusCode");
																														if(attr)
																															MSG_SetDoubleValue(attr, IOT_SGRC_SUCCESS,"rw", NULL);																			
																													}	
																											}
																											if(i==numberOfDO)
																											{	
																												attr = MSG_AddJSONAttribute(pSensor, "sv");
																												if(attr)
																													MSG_SetStringValue(attr,IOT_SGRC_STR_NOT_FOUND,"rw");
																												attr = MSG_AddJSONAttribute(pSensor, "StatusCode");
																												if(attr)
																													MSG_SetDoubleValue(attr, IOT_SGRC_NOT_FOUND,"rw", NULL);	
																											}
																										}
																									}
																									if(strcmp(sensors[count].type,"Analog Input")==0)
																									{
																										for(i=0;i<numberOfAI;i++)
																										{
																											if(strcmp(AI[i].name,sensors[count].name)==0)
																											{	
																												while(true)
																												{
																													AI[i].Regs=rand()%(int)AI[i].max;
																													if(AI[i].Regs>=AI[i].min)
																														break;
																													else
																														continue;
																												}
																												attr = MSG_AddJSONAttribute(pSensor, "v");
																												if(attr)
																													if(MSG_SetDoubleValue(attr, AI[i].Regs,"r", AI[i].unit))
																													{	attr = MSG_AddJSONAttribute(pSensor, "StatusCode");
																														if(attr)
																															MSG_SetDoubleValue(attr, IOT_SGRC_SUCCESS,"r", NULL);																			
																													}	
																											}
																											if(i==numberOfAI)
																											{	
																												attr = MSG_AddJSONAttribute(pSensor, "sv");
																												if(attr)
																													MSG_SetStringValue(attr,IOT_SGRC_STR_NOT_FOUND,"r");
																												attr = MSG_AddJSONAttribute(pSensor, "StatusCode");
																												if(attr)
																													MSG_SetDoubleValue(attr, IOT_SGRC_NOT_FOUND,"r", NULL);	
																											}
																										}
																									}
																									if(strcmp(sensors[count].type,"Analog Output")==0)
																									{
																										for(i=0;i<numberOfAO;i++)
																										{
																											if(strcmp(AO[i].name,sensors[count].name)==0)
																											{	
																												while(true)
																												{
																													AO[i].Regs=rand()%(int)AO[i].max;
																													if(AO[i].Regs>=AO[i].min)
																														break;
																													else
																														continue;
																												}									
																												attr = MSG_AddJSONAttribute(pSensor, "v");
																												if(attr)
																													if(MSG_SetDoubleValue(attr, AO[i].Regs,"rw", AO[i].unit))
																													{	attr = MSG_AddJSONAttribute(pSensor, "StatusCode");
																														if(attr)
																															MSG_SetDoubleValue(attr, IOT_SGRC_SUCCESS,"rw", NULL);																			
																													}	
																											}
																											if(i==numberOfAO)
																											{	
																												attr = MSG_AddJSONAttribute(pSensor, "sv");
																												if(attr)
																													MSG_SetStringValue(attr,IOT_SGRC_STR_NOT_FOUND,"rw");
																												attr = MSG_AddJSONAttribute(pSensor, "StatusCode");
																												if(attr)
																													MSG_SetDoubleValue(attr, IOT_SGRC_NOT_FOUND,"rw", NULL);	
																											}
																										}
																									}
																									#pragma endregion Platform-DI-DO-AI-AO
																			}
																			else
																			{
																						#pragma region g_bRetrieve
																						if(g_bRetrieve)
																						{	
																												#pragma region Platform-DI-DO-AI-AO
																												int i=0;
																												if(strcmp(sensors[count].type,"Platform")==0)
																												{
																													if(strcmp(sensors[count].name,"Name")==0)
																													{
																														attr = MSG_AddJSONAttribute(pSensor, "sv");
																														if(attr)
																															if(MSG_SetStringValue(attr, WISE_Name, "r"))
																															{	attr = MSG_AddJSONAttribute(pSensor, "StatusCode");
																																if(attr)
																																	MSG_SetDoubleValue(attr, IOT_SGRC_SUCCESS,"r", NULL);																			
																															}
																													}
																													if(strcmp(sensors[count].name,"SlaveIP")==0)
																													{
																														attr = MSG_AddJSONAttribute(pSensor, "sv");
																														if(attr)
																															if(MSG_SetStringValue(attr, Modbus_Slave_IP, "r"))
																															{	attr = MSG_AddJSONAttribute(pSensor, "StatusCode");
																																if(attr)
																																	MSG_SetDoubleValue(attr, IOT_SGRC_SUCCESS,"r", NULL);																			
																															}
																													}
																													if(strcmp(sensors[count].name,"SlavePort")==0)
																													{
																														attr = MSG_AddJSONAttribute(pSensor, "sv");
																														if(attr)
																														{
																															char temp[6];
																															sprintf(temp,"%d",Modbus_Slave_Port);
																															if(MSG_SetStringValue(attr, temp, "r"))
																															{	attr = MSG_AddJSONAttribute(pSensor, "StatusCode");
																																if(attr)
																																	MSG_SetDoubleValue(attr, IOT_SGRC_SUCCESS,"r", NULL);																			
																															}
																													
																														}	
																													}
																													if(strcmp(sensors[count].name,"Connection")==0)
																													{
																														attr = MSG_AddJSONAttribute(pSensor, "sv");
																														if(attr)
																															if(MSG_SetBoolValue(attr, true, "r"))
																															{	attr = MSG_AddJSONAttribute(pSensor, "StatusCode");
																																if(attr)
																																	MSG_SetDoubleValue(attr, IOT_SGRC_SUCCESS,"r", NULL);																			
																															}
																													}

																												}
																												if(strcmp(sensors[count].type,"Digital Input")==0)
																												{
																													for(i=0;i<numberOfDI;i++)
																													{
																														if(strcmp(DI[i].name,sensors[count].name)==0)
																														{	
																															DI[i].Bits=DI_Bits[i];									
																															attr = MSG_AddJSONAttribute(pSensor, "bv");
																															if(attr)
																																if(MSG_SetBoolValue(attr, DI[i].Bits, "r"))
																																{	attr = MSG_AddJSONAttribute(pSensor, "StatusCode");
																																	if(attr)
																																		MSG_SetDoubleValue(attr, IOT_SGRC_SUCCESS,"r", NULL);																			
																																}	
																														}
																														if(i==numberOfDI)
																														{	
																															attr = MSG_AddJSONAttribute(pSensor, "sv");
																															if(attr)
																																MSG_SetStringValue(attr,IOT_SGRC_STR_NOT_FOUND,"r");
																															attr = MSG_AddJSONAttribute(pSensor, "StatusCode");
																															if(attr)
																																MSG_SetDoubleValue(attr, IOT_SGRC_NOT_FOUND,"r", NULL);	
																														}
																													}
																												}
																												if(strcmp(sensors[count].type,"Digital Output")==0)
																												{
																													for(i=0;i<numberOfDO;i++)
																													{
																														if(strcmp(DO[i].name,sensors[count].name)==0)
																														{	
																															DO[i].Bits=DO_Bits[i];									
																															attr = MSG_AddJSONAttribute(pSensor, "bv");
																															if(attr)
																																if(MSG_SetBoolValue(attr, DO[i].Bits, "rw"))
																																{	attr = MSG_AddJSONAttribute(pSensor, "StatusCode");
																																	if(attr)
																																		MSG_SetDoubleValue(attr, IOT_SGRC_SUCCESS,"rw", NULL);																			
																																}	
																														}
																														if(i==numberOfDO)
																														{	
																															attr = MSG_AddJSONAttribute(pSensor, "sv");
																															if(attr)
																																MSG_SetStringValue(attr,IOT_SGRC_STR_NOT_FOUND,"rw");
																															attr = MSG_AddJSONAttribute(pSensor, "StatusCode");
																															if(attr)
																																MSG_SetDoubleValue(attr, IOT_SGRC_NOT_FOUND,"rw", NULL);	
																														}
																													}
																												}
																												if(strcmp(sensors[count].type,"Analog Input")==0)
																												{
																													for(i=0;i<numberOfAI;i++)
																													{
																														if(strcmp(AI[i].name,sensors[count].name)==0)
																														{	
																															AI[i].Regs=AI_Regs[i];									
																															attr = MSG_AddJSONAttribute(pSensor, "v");
																															if(attr)
																																if(MSG_SetDoubleValue(attr, AI[i].Regs*AI[i].precision,"r", AI[i].unit))
																																{	attr = MSG_AddJSONAttribute(pSensor, "StatusCode");
																																	if(attr)
																																		MSG_SetDoubleValue(attr, IOT_SGRC_SUCCESS,"r", NULL);																			
																																}	
																														}
																														if(i==numberOfAI)
																														{	
																															attr = MSG_AddJSONAttribute(pSensor, "sv");
																															if(attr)
																																MSG_SetStringValue(attr,IOT_SGRC_STR_NOT_FOUND,"r");
																															attr = MSG_AddJSONAttribute(pSensor, "StatusCode");
																															if(attr)
																																MSG_SetDoubleValue(attr, IOT_SGRC_NOT_FOUND,"r", NULL);	
																														}
																													}
																												}
																												if(strcmp(sensors[count].type,"Analog Output")==0)
																												{
																													for(i=0;i<numberOfAO;i++)
																													{
																														if(strcmp(AO[i].name,sensors[count].name)==0)
																														{	
																															AO[i].Regs=AO_Regs[i];									
																															attr = MSG_AddJSONAttribute(pSensor, "v");
																															if(attr)
																																if(MSG_SetDoubleValue(attr, AO[i].Regs*AO[i].precision,"rw", AO[i].unit))
																																{	attr = MSG_AddJSONAttribute(pSensor, "StatusCode");
																																	if(attr)
																																		MSG_SetDoubleValue(attr, IOT_SGRC_SUCCESS,"rw", NULL);																			
																																}	
																														}
																														if(i==numberOfAO)
																														{	
																															attr = MSG_AddJSONAttribute(pSensor, "sv");
																															if(attr)
																																MSG_SetStringValue(attr,IOT_SGRC_STR_NOT_FOUND,"rw");
																															attr = MSG_AddJSONAttribute(pSensor, "StatusCode");
																															if(attr)
																																MSG_SetDoubleValue(attr, IOT_SGRC_NOT_FOUND,"rw", NULL);	
																														}
																													}
																												}
																												#pragma endregion Platform-DI-DO-AI-AO


																						}
																						else
																						{
																												#pragma region FAIL	
																												if(strcmp(sensors[count].type,"Platform")==0)
																												{	attr = MSG_AddJSONAttribute(pSensor, "sv");
																													if(attr)
																														MSG_SetStringValue(attr,IOT_SGRC_STR_FAIL,"r");
																													attr = MSG_AddJSONAttribute(pSensor, "StatusCode");
																													if(attr)
																														MSG_SetDoubleValue(attr, IOT_SGRC_FAIL,"r", NULL);	
																												}
																												if(strcmp(sensors[count].type,"Digital Input")==0)
																												{	attr = MSG_AddJSONAttribute(pSensor, "sv");
																													if(attr)
																														MSG_SetStringValue(attr,IOT_SGRC_STR_FAIL,"r");
																													attr = MSG_AddJSONAttribute(pSensor, "StatusCode");
																													if(attr)
																														MSG_SetDoubleValue(attr, IOT_SGRC_FAIL,"r", NULL);	
																												}
																												if(strcmp(sensors[count].type,"Digital Output")==0)
																												{	attr = MSG_AddJSONAttribute(pSensor, "sv");
																													if(attr)
																														MSG_SetStringValue(attr,IOT_SGRC_STR_FAIL,"rw");
																													attr = MSG_AddJSONAttribute(pSensor, "StatusCode");
																													if(attr)
																														MSG_SetDoubleValue(attr, IOT_SGRC_FAIL,"rw", NULL);	
																												}
																												if(strcmp(sensors[count].type,"Analog Input")==0)
																												{	attr = MSG_AddJSONAttribute(pSensor, "sv");
																													if(attr)
																														MSG_SetStringValue(attr,IOT_SGRC_STR_FAIL,"r");
																													attr = MSG_AddJSONAttribute(pSensor, "StatusCode");
																													if(attr)
																														MSG_SetDoubleValue(attr, IOT_SGRC_FAIL,"r", NULL);	
																												}
																												if(strcmp(sensors[count].type,"Analog Output")==0)
																												{	attr = MSG_AddJSONAttribute(pSensor, "sv");
																													if(attr)
																														MSG_SetStringValue(attr,IOT_SGRC_STR_FAIL,"rw");
																													attr = MSG_AddJSONAttribute(pSensor, "StatusCode");
																													if(attr)
																														MSG_SetDoubleValue(attr, IOT_SGRC_FAIL,"rw", NULL);	
																												}
																												#pragma endregion FAIL
																						}	
																						#pragma endregion g_bRetrieve
																			}
																			#pragma endregion Simulator																		
																	
																	}
															}
															else
															{
																									#pragma region NOT FOUND	
																									if(strcmp(sensors[count].type,"Platform")==0)
																									{	attr = MSG_AddJSONAttribute(pSensor, "sv");
																										if(attr)
																											MSG_SetStringValue(attr,IOT_SGRC_STR_NOT_FOUND,"r");
																										attr = MSG_AddJSONAttribute(pSensor, "StatusCode");
																										if(attr)
																											MSG_SetDoubleValue(attr, IOT_SGRC_NOT_FOUND,"r", NULL);	
																									}
																									else if(strcmp(sensors[count].type,"Digital Input")==0)
																									{	attr = MSG_AddJSONAttribute(pSensor, "sv");
																										if(attr)
																											MSG_SetStringValue(attr,IOT_SGRC_STR_NOT_FOUND,"r");
																										attr = MSG_AddJSONAttribute(pSensor, "StatusCode");
																										if(attr)
																											MSG_SetDoubleValue(attr, IOT_SGRC_NOT_FOUND,"r", NULL);	
																									}
																									else if(strcmp(sensors[count].type,"Digital Output")==0)
																									{	attr = MSG_AddJSONAttribute(pSensor, "sv");
																										if(attr)
																											MSG_SetStringValue(attr,IOT_SGRC_STR_NOT_FOUND,"rw");
																										attr = MSG_AddJSONAttribute(pSensor, "StatusCode");
																										if(attr)
																											MSG_SetDoubleValue(attr, IOT_SGRC_NOT_FOUND,"rw", NULL);	
																									}
																									else if(strcmp(sensors[count].type,"Analog Input")==0)
																									{	attr = MSG_AddJSONAttribute(pSensor, "sv");
																										if(attr)
																											MSG_SetStringValue(attr,IOT_SGRC_STR_NOT_FOUND,"r");
																										attr = MSG_AddJSONAttribute(pSensor, "StatusCode");
																										if(attr)
																											MSG_SetDoubleValue(attr, IOT_SGRC_NOT_FOUND,"r", NULL);	
																									}
																									else if(strcmp(sensors[count].type,"Analog Output")==0)
																									{	attr = MSG_AddJSONAttribute(pSensor, "sv");
																										if(attr)
																											MSG_SetStringValue(attr,IOT_SGRC_STR_NOT_FOUND,"rw");
																										attr = MSG_AddJSONAttribute(pSensor, "StatusCode");
																										if(attr)
																											MSG_SetDoubleValue(attr, IOT_SGRC_NOT_FOUND,"rw", NULL);	
																									}
																									else
																									{	attr = MSG_AddJSONAttribute(pSensor, "sv");
																										if(attr)
																											MSG_SetStringValue(attr,IOT_SGRC_STR_NOT_FOUND,NULL);
																										attr = MSG_AddJSONAttribute(pSensor, "StatusCode");
																										if(attr)
																											MSG_SetDoubleValue(attr, IOT_SGRC_NOT_FOUND,NULL, NULL);	
																									}
																									#pragma endregion NOT FOUND
															}
															#pragma endregion MSG_Find_Sensor
													}
													#pragma endregion pSensor
										}
										#pragma endregion pEArray
								}
								#pragma endregion pSensorInofList
							curNode = curNode->next;
							count++;
						}
						{

							repJsonStr = IoT_PrintData(pRoot);
							printf("\nGET Handler_Data = %s\n",repJsonStr);
							printf("---------------------\n");
							if(repJsonStr != NULL)
							{
								g_sendcbf(&g_PluginInfo, modbus_get_sensors_data_rep, repJsonStr, strlen(repJsonStr)+1, NULL, NULL);
							}
							if(repJsonStr)free(repJsonStr);
						}
				}
				#pragma endregion pRoot
	}
	#pragma endregion IsSensorInfoListEmpty

	if(pRoot)
		MSG_ReleaseRoot(pRoot);
	if(sensors)		
		free(sensors);

}


static void SetSensorsDataEx(sensor_info_list sensorInfoList, char * pSessionID)
{
    int Num_Sensor=1000; // to avoid exceeding 
	WISE_Sensor *sensors=(WISE_Sensor *)calloc(Num_Sensor,sizeof(WISE_Sensor));
	MSG_CLASSIFY_T  *pSensorInofList=NULL,*pEArray=NULL, *pSensor=NULL;
	MSG_CLASSIFY_T  *pRoot=NULL;
	MSG_ATTRIBUTE_T	*attr;

	char * repJsonStr = NULL;
	int count=0;

	bool bVal=false;
	double dVal=0;
	char *sVal=(char *)calloc(SET_STR_LENGTH,sizeof(char));
	bool bFormat=false;


	#pragma region IsSensorInfoListEmpty
	if(!IsSensorInfoListEmpty(sensorInfoList))
	{
		sensor_info_node_t *curNode = NULL;
		curNode = sensorInfoList->next;
				#pragma region pRoot
				pRoot = MSG_CreateRoot();
				if(pRoot)
				{
						attr = MSG_AddJSONAttribute(pRoot, "sessionID");
						if(attr)
							MSG_SetStringValue( attr, pSessionID, NULL); 

						pSensorInofList = MSG_AddJSONClassify(pRoot, "sensorInfoList", NULL, false);
						if(pSensorInofList)
							pEArray=MSG_AddJSONClassify(pSensorInofList,"e",NULL,true);
						
						while(curNode)
						{

								bFormat=Modbus_Parser_Set_FormatCheck(curNode,&bVal,&dVal,sVal);
								if(bFormat)
									;//printf("\nSet Format Succeed bVal=%d \ndVal=%lf \nsVal=%s\n",bVal,dVal,sVal);
								else
									printf("\nFormat Error!!\n");

								#pragma region pSensorInofList
								if(pSensorInofList)
								{		
										#pragma region pEArray		
										if(pEArray)
										{
													#pragma region pSensor
													pSensor = MSG_AddJSONClassify(pEArray, "sensor", NULL, false);	
													if(pSensor)
													{		attr = MSG_AddJSONAttribute(pSensor, "n");
															if(attr)
																MSG_SetStringValue(attr, curNode->sensorInfo.pathStr, NULL);
															if(count<Num_Sensor)
																Modbus_General_Node_Parser(curNode,sensors,count);
															//printf("\ntype: %s  name: %s\n",sensors[count].type,sensors[count].name);	

																			#pragma region bFormat
																			if(!bFormat)
																			{
																						if(strcmp(sensors[count].type,"Platform")==0)
																						{
																							attr = MSG_AddJSONAttribute(pSensor, "sv");
																							if(attr)
																								MSG_SetStringValue(attr,IOT_SGRC_STR_FORMAT_ERROR,"r");
																							attr = MSG_AddJSONAttribute(pSensor, "StatusCode");
																							if(attr)
																								MSG_SetDoubleValue(attr, IOT_SGRC_FORMAT_ERROR,"r", NULL);	
																						}

																						if(strcmp(sensors[count].type,"Digital Input")==0)
																						{
																							attr = MSG_AddJSONAttribute(pSensor, "sv");
																							if(attr)
																								MSG_SetStringValue(attr,IOT_SGRC_STR_FORMAT_ERROR,"r");
																							attr = MSG_AddJSONAttribute(pSensor, "StatusCode");
																							if(attr)
																								MSG_SetDoubleValue(attr, IOT_SGRC_FORMAT_ERROR,"r", NULL);	
																						}

																						if(strcmp(sensors[count].type,"Digital Output")==0)
																						{
																							attr = MSG_AddJSONAttribute(pSensor, "sv");
																							if(attr)
																								MSG_SetStringValue(attr,IOT_SGRC_STR_FORMAT_ERROR,"rw");
																							attr = MSG_AddJSONAttribute(pSensor, "StatusCode");
																							if(attr)
																								MSG_SetDoubleValue(attr, IOT_SGRC_FORMAT_ERROR,"rw", NULL);	
																						}

																						if(strcmp(sensors[count].type,"Analog Input")==0)
																						{
																							attr = MSG_AddJSONAttribute(pSensor, "sv");
																							if(attr)
																								MSG_SetStringValue(attr,IOT_SGRC_STR_FORMAT_ERROR,"r");
																							attr = MSG_AddJSONAttribute(pSensor, "StatusCode");
																							if(attr)
																								MSG_SetDoubleValue(attr, IOT_SGRC_FORMAT_ERROR,"r", NULL);	
																						}

																						if(strcmp(sensors[count].type,"Analog Output")==0)
																						{
																							attr = MSG_AddJSONAttribute(pSensor, "sv");
																							if(attr)
																								MSG_SetStringValue(attr,IOT_SGRC_STR_FORMAT_ERROR,"rw");
																							attr = MSG_AddJSONAttribute(pSensor, "StatusCode");
																							if(attr)
																								MSG_SetDoubleValue(attr, IOT_SGRC_FORMAT_ERROR,"rw", NULL);	
																						}

																			}	
																			else
																			{
																						#pragma region MSG_Find_Sensor
																						if(MSG_Find_Sensor(g_Capability,curNode->sensorInfo.pathStr))
																						{		
																								if(attr)
																								{	
																														#pragma region bIsSimtor
																														if(bIsSimtor)
																														{
																																#pragma region Platform-DI-DO-AI-AO
																																int i=0;
																																if(strcmp(sensors[count].type,"Platform")==0)
																																{
																																	attr = MSG_AddJSONAttribute(pSensor, "sv");
																																	if(attr)
																																		MSG_SetStringValue(attr,IOT_SGRC_STR_READ_ONLY,"r");
																																	attr = MSG_AddJSONAttribute(pSensor, "StatusCode");
																																	if(attr)
																																		MSG_SetDoubleValue(attr, IOT_SGRC_READ_ONLY,"r", NULL);	
																																}
																																if(strcmp(sensors[count].type,"Digital Input")==0)
																																{
							
																																	attr = MSG_AddJSONAttribute(pSensor, "sv");
																																	if(attr)
																																		MSG_SetStringValue(attr,IOT_SGRC_STR_READ_ONLY,"r");
																																	attr = MSG_AddJSONAttribute(pSensor, "StatusCode");
																																	if(attr)
																																		MSG_SetDoubleValue(attr, IOT_SGRC_READ_ONLY,"r", NULL);	
																																		
																																}
																																if(strcmp(sensors[count].type,"Digital Output")==0)
																																{
																																	for(i=0;i<numberOfDO;i++)
																																	{
																																		if(strcmp(DO[i].name,sensors[count].name)==0)
																																		{	
																																			attr = MSG_AddJSONAttribute(pSensor, "sv");
																																			if(attr)
																																				MSG_SetStringValue(attr,IOT_SGRC_STR_SUCCESS,"rw");
																																			attr = MSG_AddJSONAttribute(pSensor, "StatusCode");
																																			if(attr)
																																				MSG_SetDoubleValue(attr, IOT_SGRC_SUCCESS,"rw", NULL);
																																		}
																																		if(i==numberOfDO)
																																		{	
																																			attr = MSG_AddJSONAttribute(pSensor, "sv");
																																			if(attr)
																																				MSG_SetStringValue(attr,IOT_SGRC_STR_NOT_FOUND,"rw");
																																			attr = MSG_AddJSONAttribute(pSensor, "StatusCode");
																																			if(attr)
																																				MSG_SetDoubleValue(attr, IOT_SGRC_NOT_FOUND,"rw", NULL);	
																																		}
																																	}
																																}
																																if(strcmp(sensors[count].type,"Analog Input")==0)
																																{
																														
																																	attr = MSG_AddJSONAttribute(pSensor, "sv");
																																	if(attr)
																																		MSG_SetStringValue(attr,IOT_SGRC_STR_READ_ONLY,"r");
																																	attr = MSG_AddJSONAttribute(pSensor, "StatusCode");
																																	if(attr)
																																		MSG_SetDoubleValue(attr, IOT_SGRC_READ_ONLY,"r", NULL);	
																																		
																																}
																																if(strcmp(sensors[count].type,"Analog Output")==0)
																																{
																																	for(i=0;i<numberOfAO;i++)
																																	{
																																		if(strcmp(AO[i].name,sensors[count].name)==0)
																																		{		
																																				if(dVal>AO[i].max || dVal<AO[i].min)
																																				{
																																					attr = MSG_AddJSONAttribute(pSensor, "sv");
																																					if(attr)
																																						MSG_SetStringValue(attr,IOT_SGRC_STR_OUT_RANGE,"rw");
																																					attr = MSG_AddJSONAttribute(pSensor, "StatusCode");
																																					if(attr)
																																						MSG_SetDoubleValue(attr,IOT_SGRC_OUT_RANGE,"rw", NULL);	
																																				}
																																				else
																																				{
																																					attr = MSG_AddJSONAttribute(pSensor, "sv");
																																					if(attr)
																																						MSG_SetStringValue(attr,IOT_SGRC_STR_SUCCESS,"rw");
																																					attr = MSG_AddJSONAttribute(pSensor, "StatusCode");
																																					if(attr)
																																						MSG_SetDoubleValue(attr, IOT_SGRC_SUCCESS,"rw", NULL);	

																																				}

																																		}
																																		if(i==numberOfAO)
																																		{	
																																			attr = MSG_AddJSONAttribute(pSensor, "sv");
																																			if(attr)
																																				MSG_SetStringValue(attr,IOT_SGRC_STR_NOT_FOUND,"rw");
																																			attr = MSG_AddJSONAttribute(pSensor, "StatusCode");
																																			if(attr)
																																				MSG_SetDoubleValue(attr, IOT_SGRC_NOT_FOUND,"rw", NULL);	
																																		}
																																	}
																																}

																																#pragma endregion Platform-DI-DO-AI-AO
																													}
																													else
																													{
																																#pragma region g_bRetrieve
																																if(g_bRetrieve)
																																{	
																																						#pragma region Platform-DI-DO-AI-AO
																																						int i=0;
																																						if(strcmp(sensors[count].type,"Platform")==0)
																																						{
																																									attr = MSG_AddJSONAttribute(pSensor, "sv");
																																									if(attr)
																																										MSG_SetStringValue(attr,IOT_SGRC_STR_READ_ONLY,"r");
																																									attr = MSG_AddJSONAttribute(pSensor, "StatusCode");
																																									if(attr)
																																										MSG_SetDoubleValue(attr, IOT_SGRC_READ_ONLY,"r", NULL);	
																																						}
																																						if(strcmp(sensors[count].type,"Digital Input")==0)
																																						{
													
																																									attr = MSG_AddJSONAttribute(pSensor, "sv");
																																									if(attr)
																																										MSG_SetStringValue(attr,IOT_SGRC_STR_READ_ONLY,"r");
																																									attr = MSG_AddJSONAttribute(pSensor, "StatusCode");
																																									if(attr)
																																										MSG_SetDoubleValue(attr, IOT_SGRC_READ_ONLY,"r", NULL);	
																																								
																																						}
																																						if(strcmp(sensors[count].type,"Digital Output")==0)
																																						{
																																							for(i=0;i<numberOfDO;i++)
																																							{
																																								if(strcmp(DO[i].name,sensors[count].name)==0)
																																								{	

																																									int ret=modbus_write_bit(ctx,DO[i].address,bVal);

																																									if(ret!=-1)
																																									{
																																										attr = MSG_AddJSONAttribute(pSensor, "sv");
																																										if(attr)
																																											MSG_SetStringValue(attr,IOT_SGRC_STR_SUCCESS,"rw");
																																										attr = MSG_AddJSONAttribute(pSensor, "StatusCode");
																																										if(attr)
																																											MSG_SetDoubleValue(attr, IOT_SGRC_SUCCESS,"rw", NULL);
																																									}
																																									else
																																									{
																																										attr = MSG_AddJSONAttribute(pSensor, "sv");
																																										if(attr)
																																											MSG_SetStringValue(attr,IOT_SGRC_STR_FAIL,"rw");
																																										attr = MSG_AddJSONAttribute(pSensor, "StatusCode");
																																										if(attr)
																																											MSG_SetDoubleValue(attr, IOT_SGRC_FAIL,"rw", NULL);	

																																									}

																																								}
																																								if(i==numberOfDO)
																																								{	
																																									attr = MSG_AddJSONAttribute(pSensor, "sv");
																																									if(attr)
																																										MSG_SetStringValue(attr,IOT_SGRC_STR_NOT_FOUND,"rw");
																																									attr = MSG_AddJSONAttribute(pSensor, "StatusCode");
																																									if(attr)
																																										MSG_SetDoubleValue(attr, IOT_SGRC_NOT_FOUND,"rw", NULL);	
																																								}
																																							}
																																						}
																																						if(strcmp(sensors[count].type,"Analog Input")==0)
																																						{
																																				
																																									attr = MSG_AddJSONAttribute(pSensor, "sv");
																																									if(attr)
																																										MSG_SetStringValue(attr,IOT_SGRC_STR_READ_ONLY,"r");
																																									attr = MSG_AddJSONAttribute(pSensor, "StatusCode");
																																									if(attr)
																																										MSG_SetDoubleValue(attr, IOT_SGRC_READ_ONLY,"r", NULL);	
																																								
																																						}
																																						if(strcmp(sensors[count].type,"Analog Output")==0)
																																						{
																																							for(i=0;i<numberOfAO;i++)
																																							{
																																								if(strcmp(AO[i].name,sensors[count].name)==0)
																																								{		
																																										if(dVal>AO[i].max || dVal<AO[i].min)
																																										{
																																												attr = MSG_AddJSONAttribute(pSensor, "sv");
																																												if(attr)
																																													MSG_SetStringValue(attr,IOT_SGRC_STR_OUT_RANGE,"rw");
																																												attr = MSG_AddJSONAttribute(pSensor, "StatusCode");
																																												if(attr)
																																													MSG_SetDoubleValue(attr,IOT_SGRC_OUT_RANGE,"rw", NULL);	
																																										}
																																										else
																																										{
																																												int ret=modbus_write_register(ctx,AO[i].address,dVal/AO[i].precision);

																																												if(ret!=-1)
																																												{
																																													attr = MSG_AddJSONAttribute(pSensor, "sv");
																																													if(attr)
																																														MSG_SetStringValue(attr,IOT_SGRC_STR_SUCCESS,"rw");
																																													attr = MSG_AddJSONAttribute(pSensor, "StatusCode");
																																													if(attr)
																																														MSG_SetDoubleValue(attr, IOT_SGRC_SUCCESS,"rw", NULL);	
																																												}
																																												else
																																												{
																																													attr = MSG_AddJSONAttribute(pSensor, "sv");
																																													if(attr)
																																														MSG_SetStringValue(attr,IOT_SGRC_STR_FAIL,"rw");
																																													attr = MSG_AddJSONAttribute(pSensor, "StatusCode");
																																													if(attr)
																																														MSG_SetDoubleValue(attr, IOT_SGRC_FAIL,"rw", NULL);	

																																												}
																																										}

																																								}
																																								if(i==numberOfAO)
																																								{	
																																									attr = MSG_AddJSONAttribute(pSensor, "sv");
																																									if(attr)
																																										MSG_SetStringValue(attr,IOT_SGRC_STR_NOT_FOUND,"rw");
																																									attr = MSG_AddJSONAttribute(pSensor, "StatusCode");
																																									if(attr)
																																										MSG_SetDoubleValue(attr, IOT_SGRC_NOT_FOUND,"rw", NULL);	
																																								}
																																							}
																																						}

																																						#pragma endregion Platform-DI-DO-AI-AO


																																}
																																else
																																{
																																						#pragma region FAIL	
																																						if(strcmp(sensors[count].type,"Platform")==0)
																																						{	attr = MSG_AddJSONAttribute(pSensor, "sv");
																																							if(attr)
																																								MSG_SetStringValue(attr,IOT_SGRC_STR_FAIL,"r");
																																							attr = MSG_AddJSONAttribute(pSensor, "StatusCode");
																																							if(attr)
																																								MSG_SetDoubleValue(attr, IOT_SGRC_FAIL,"r", NULL);	
																																						}

																																						if(strcmp(sensors[count].type,"Digital Input")==0)
																																						{	attr = MSG_AddJSONAttribute(pSensor, "sv");
																																							if(attr)
																																								MSG_SetStringValue(attr,IOT_SGRC_STR_FAIL,"r");
																																							attr = MSG_AddJSONAttribute(pSensor, "StatusCode");
																																							if(attr)
																																								MSG_SetDoubleValue(attr, IOT_SGRC_FAIL,"r", NULL);	
																																						}
																																						if(strcmp(sensors[count].type,"Digital Output")==0)
																																						{	attr = MSG_AddJSONAttribute(pSensor, "sv");
																																							if(attr)
																																								MSG_SetStringValue(attr,IOT_SGRC_STR_FAIL,"rw");
																																							attr = MSG_AddJSONAttribute(pSensor, "StatusCode");
																																							if(attr)
																																								MSG_SetDoubleValue(attr, IOT_SGRC_FAIL,"rw", NULL);	
																																						}
																																						if(strcmp(sensors[count].type,"Analog Input")==0)
																																						{	attr = MSG_AddJSONAttribute(pSensor, "sv");
																																							if(attr)
																																								MSG_SetStringValue(attr,IOT_SGRC_STR_FAIL,"r");
																																							attr = MSG_AddJSONAttribute(pSensor, "StatusCode");
																																							if(attr)
																																								MSG_SetDoubleValue(attr, IOT_SGRC_FAIL,"r", NULL);	
																																						}
																																						if(strcmp(sensors[count].type,"Analog Output")==0)
																																						{	attr = MSG_AddJSONAttribute(pSensor, "sv");
																																							if(attr)
																																								MSG_SetStringValue(attr,IOT_SGRC_STR_FAIL,"rw");
																																							attr = MSG_AddJSONAttribute(pSensor, "StatusCode");
																																							if(attr)
																																								MSG_SetDoubleValue(attr, IOT_SGRC_FAIL,"rw", NULL);	
																																						}
																																						#pragma endregion FAIL
																																}	
																																#pragma endregion g_bRetrieve
																													}
																													#pragma endregion bIsSimtor
																										
																									
																								}
																						}
																						else
																						{
																																#pragma region NOT FOUND		
																																if(strcmp(sensors[count].type,"Platform")==0)
																																{	attr = MSG_AddJSONAttribute(pSensor, "sv");
																																	if(attr)
																																		MSG_SetStringValue(attr,IOT_SGRC_STR_NOT_FOUND,"r");
																																	attr = MSG_AddJSONAttribute(pSensor, "StatusCode");
																																	if(attr)
																																		MSG_SetDoubleValue(attr, IOT_SGRC_NOT_FOUND,"r", NULL);	
																																}
																																else if(strcmp(sensors[count].type,"Digital Input")==0)
																																{	attr = MSG_AddJSONAttribute(pSensor, "sv");
																																	if(attr)
																																		MSG_SetStringValue(attr,IOT_SGRC_STR_NOT_FOUND,"r");
																																	attr = MSG_AddJSONAttribute(pSensor, "StatusCode");
																																	if(attr)
																																		MSG_SetDoubleValue(attr, IOT_SGRC_NOT_FOUND,"r", NULL);	
																																}
																																else if(strcmp(sensors[count].type,"Digital Output")==0)
																																{	attr = MSG_AddJSONAttribute(pSensor, "sv");
																																	if(attr)
																																		MSG_SetStringValue(attr,IOT_SGRC_STR_NOT_FOUND,"rw");
																																	attr = MSG_AddJSONAttribute(pSensor, "StatusCode");
																																	if(attr)
																																		MSG_SetDoubleValue(attr, IOT_SGRC_NOT_FOUND,"rw", NULL);	
																																}
																																else if(strcmp(sensors[count].type,"Analog Input")==0)
																																{	attr = MSG_AddJSONAttribute(pSensor, "sv");
																																	if(attr)
																																		MSG_SetStringValue(attr,IOT_SGRC_STR_NOT_FOUND,"r");
																																	attr = MSG_AddJSONAttribute(pSensor, "StatusCode");
																																	if(attr)
																																		MSG_SetDoubleValue(attr, IOT_SGRC_NOT_FOUND,"r", NULL);	
																																}
																																else if(strcmp(sensors[count].type,"Analog Output")==0)
																																{	attr = MSG_AddJSONAttribute(pSensor, "sv");
																																	if(attr)
																																		MSG_SetStringValue(attr,IOT_SGRC_STR_NOT_FOUND,"rw");
																																	attr = MSG_AddJSONAttribute(pSensor, "StatusCode");
																																	if(attr)
																																		MSG_SetDoubleValue(attr, IOT_SGRC_NOT_FOUND,"rw", NULL);	
																																}
																																else
																																{	attr = MSG_AddJSONAttribute(pSensor, "sv");
																																	if(attr)
																																		MSG_SetStringValue(attr,IOT_SGRC_STR_NOT_FOUND,NULL);
																																	attr = MSG_AddJSONAttribute(pSensor, "StatusCode");
																																	if(attr)
																																		MSG_SetDoubleValue(attr, IOT_SGRC_NOT_FOUND,NULL, NULL);	
																																}
																																#pragma endregion NOT FOUND
																						}
																						#pragma endregion MSG_Find_Sensor
																			}
																			#pragma endregion bFormat
															 

													}
													#pragma endregion pSensor
										}
										#pragma endregion pEArray
								}
								#pragma endregion pSensorInofList								
							curNode = curNode->next;
							count++;
							
						}
						{

							repJsonStr = IoT_PrintData(pRoot);
							printf("\nSET Handler_Data = %s\n",repJsonStr);
							printf("---------------------\n");
							if(repJsonStr != NULL)
							{
								g_sendcbf(&g_PluginInfo, modbus_set_sensors_data_rep, repJsonStr, strlen(repJsonStr)+1, NULL, NULL);
							}
							if(repJsonStr)
								free(repJsonStr);
						}
				}
				#pragma endregion pRoot
	}
	#pragma endregion IsSensorInfoListEmpty

	if(pRoot)
		MSG_ReleaseRoot(pRoot);
	if(sensors)
		free(sensors);
	if(sVal)
		free(sVal);

}

static void UploadAllSensorsData(bool bReport_Upload,void *args)
{	
	handler_context_t *pHandlerContex = (handler_context_t *)args;

	char *repJsonStr = NULL;

	MSG_CLASSIFY_T  *classify=NULL;
	MSG_ATTRIBUTE_T *attr=NULL;

	IoT_READWRITE_MODE mode;

	int i;


	#pragma region UploadAllSensorsData
	printf("\nUploadAllSensorsData..........\n");

	if(!g_Capability)
		g_Capability =  CreateCapability();

	if(g_Capability)
	{		
			classify=IoT_FindGroup(g_Capability,"Platform");
			if(classify)
				attr=IoT_FindSensorNode(classify,"Connection");
			if(attr)
			{	mode=IoT_READONLY;

				if(bIsSimtor)
				{
					IoT_SetBoolValue(attr,true,mode);
				}
				else
				{
					if(attr)
						if(!g_bRetrieve)
							IoT_SetBoolValue(attr,false,mode);
						else
							IoT_SetBoolValue(attr,true,mode);
				}	
			}
			classify=IoT_FindGroup(g_Capability,"Digital Input");		
			if(classify)
			{  
				if(numberOfDI!=0)
				{	mode=IoT_READONLY;
					for(i=0;i<numberOfDI;i++)
					{	attr=IoT_FindSensorNode(classify,DI[i].name);
						if(bIsSimtor)
						{
							DI_Bits[i]=rand()%2;
						}
						else
						{
							if(!g_bRetrieve)
								DI_Bits[i]=false;
						}
						if(attr)
						{	
                            DI[i].Bits=DI_Bits[i];
							IoT_SetBoolValue(attr,(bool)DI[i].Bits,mode);
						}
					}
					attr=NULL;
				}
			}
			classify=NULL;
	

			classify=IoT_FindGroup(g_Capability,"Digital Output");		
			if(classify)
			{  
				if(numberOfDO!=0)
				{	mode=IoT_READWRITE;
					for(i=0;i<numberOfDO;i++)
					{	attr=IoT_FindSensorNode(classify,DO[i].name);
						if(bIsSimtor)
						{
							DO_Bits[i]=rand()%2;
						}
						else
						{
							if(!g_bRetrieve)
								DO_Bits[i]=false;
						}
						if(attr)
						{
                            DO[i].Bits=DO_Bits[i];
							IoT_SetBoolValue(attr,(bool)DO[i].Bits,mode);
						}
					}
					attr=NULL;
				}
			}
			classify=NULL;

			classify=IoT_FindGroup(g_Capability,"Analog Input");		
			if(classify)
			{  
				if(numberOfAI!=0)
				{	mode=IoT_READONLY;
					for(i=0;i<numberOfAI;i++)
					{	attr=IoT_FindSensorNode(classify,AI[i].name);
						if(bIsSimtor)
						{
							while(true)
							{
								AI_Regs[i]=rand()%(int)AI[i].max/AI[i].precision;
								if(AI_Regs[i]>=AI[i].min)
									break;
								else
									continue;
							}
						}
						else
						{
							if(!g_bRetrieve)
								AI_Regs[i]=0;
						}
						if(attr)
						{	
							AI[i].Regs=AI_Regs[i];
							IoT_SetDoubleValueWithMaxMin(attr,AI[i].Regs*AI[i].precision,mode,AI[i].max,AI[i].min,AI[i].unit);
						}	
						attr=NULL;
					}
			
				}
			}
			classify=NULL;

			classify=IoT_FindGroup(g_Capability,"Analog Output");		
			if(classify)
			{  
				if(numberOfAO!=0)
				{	mode=IoT_READWRITE;
					for(i=0;i<numberOfAO;i++)
					{	attr=IoT_FindSensorNode(classify,AO[i].name);
						if(bIsSimtor)
						{
							while(true)
							{
								AO_Regs[i]=rand()%(int)AO[i].max/AO[i].precision;
								if(AO_Regs[i]>=AO[i].min)
									break;
								else
									continue;
							}
						}
						else
						{
							if(!g_bRetrieve)
								AO_Regs[i]=0;
						}
						if(attr)
						{
							AO[i].Regs=AO_Regs[i];												
							IoT_SetDoubleValueWithMaxMin(attr,AO[i].Regs*AO[i].precision,mode,AO[i].max,AO[i].min,AO[i].unit);
						}
						attr=NULL;
					}
				}
			}
			classify=NULL;


			repJsonStr = IoT_PrintData(g_Capability);
			printf("\nALL Handler_Data = %s\n",repJsonStr);
			printf("---------------------\n");
			if(bReport_Upload)
			{	if(g_sendreportcbf)
					g_sendreportcbf(&g_PluginInfo, repJsonStr, strlen(repJsonStr), NULL, NULL);
			}					
			else
			{	if(g_sendcbf)
					g_sendcbf(&g_PluginInfo, modbus_auto_upload_rep, repJsonStr, strlen(repJsonStr)+1, NULL, NULL);
			}
			
			free(repJsonStr);
			
	}
	#pragma endregion UploadAllSensorsData




}
static void UploadSensorsDataEx(char **Data_paths, int Data_num, bool Reply_All,bool bReport_Upload, void *args)
{
	int Num_Sensor=1000; // to avoid exceeding 
	WISE_Sensor *sensors=(WISE_Sensor *)calloc(Num_Sensor,sizeof(WISE_Sensor));
	MSG_CLASSIFY_T *myUpload = IoT_CreateRoot((char*) strPluginName);

	MSG_CLASSIFY_T  *classify_Find=NULL;
	MSG_ATTRIBUTE_T *attr_Find=NULL;

	MSG_CLASSIFY_T *myGroup=NULL;
	MSG_ATTRIBUTE_T* attr=NULL;
	IoT_READWRITE_MODE mode=IoT_READONLY;

	char * repJsonStr = NULL;
	int Check_Level=0;
	bool Have_name=false;
	int count=0;

	for(int i=0;i<Num_Sensor;i++)
	{	strcpy(sensors[i].handler,"");
		strcpy(sensors[i].type,"");
		strcpy(sensors[i].name,"");		
	}
	if(Reply_All)
		UploadAllSensorsData(bReport_Upload,args);
	else
	{			printf("UploadSensorsDataEx.........\n");
				for(count=0;count<Data_num;count++)
					#pragma region Data_paths
					if(Data_paths[count])
					{
							Modbus_General_Paths_Parser(Data_paths[count],sensors,count);
							//printf("\nhandler: %s type: %s  name: %s\n",sensors[count].handler,sensors[count].type,sensors[count].name);
							if(strcmp(sensors[count].name,"")==0)
								Have_name=false;
							else
								Have_name=true;

							#pragma region g_Capability
							if(!g_Capability)
								g_Capability =  CreateCapability();

							if(g_Capability)
							{		
									#pragma region DEF_HANDLER_NAME
									Check_Level=0;
									if(strcmp(sensors[count].handler,DEF_HANDLER_NAME)==0)
									{		
												Check_Level++;
												classify_Find=IoT_FindGroup(g_Capability,sensors[count].type);
												#pragma region classify_Find
												if(classify_Find)
												{		
														Check_Level++;	
														attr_Find=IoT_FindSensorNode(classify_Find,sensors[count].name);
														if(attr_Find)
															Check_Level++;														
												
												}
												#pragma endregion classify_Find

												#pragma region Check_Level
												switch(Check_Level)
												{
													case 1:	
															#pragma region Check_Level=1
															//printf("Check_Level=1\n");
															UploadAllSensorsData(bReport_Upload,args); //not enter in here, just for incident
															break;
															#pragma endregion Check_Level=1
													case 2:
															#pragma region Check_Level=2
															//printf("Check_Level=2\n");
																	#pragma region Have_name
																	if(!Have_name)
																	{

																					if(strcmp(sensors[count].type,"Platform")==0)
																					{			char Slave_Port[6];
																								sprintf(Slave_Port,"%d",Modbus_Slave_Port);

																								myGroup = IoT_AddGroup(myUpload,"Platform");
																								if(myGroup)
																								{			
																											mode=IoT_READONLY;
																											attr = IoT_AddSensorNode(myGroup, "Name");
																											if(attr)
																												IoT_SetStringValue(attr,WISE_Name,mode);

																											attr = IoT_AddSensorNode(myGroup, "SlaveIP");
																											if(attr)
																												IoT_SetStringValue(attr,Modbus_Slave_IP,mode);

																											attr = IoT_AddSensorNode(myGroup, "SlavePort");
																											if(attr)
																												IoT_SetStringValue(attr,Slave_Port,mode);

																											attr = IoT_AddSensorNode(myGroup, "Connection");

																											if(bIsSimtor)
																											{
																												IoT_SetBoolValue(attr,true,mode);
																											}
																											else
																											{
																												if(attr)
																													if(!g_bRetrieve)
																														IoT_SetBoolValue(attr,false,mode);
																													else
																														IoT_SetBoolValue(attr,true,mode);
																											}
	
																								}

																					}
																					else if(strcmp(sensors[count].type,"Digital Input")==0)
																					{
																								myGroup = IoT_AddGroup(myUpload, "Digital Input");
																								if(myGroup)
																								{	
																											mode=IoT_READONLY;
																											for(int i=0;i<numberOfDI;i++){
																												if(sensors[count].name,DI[i].name)
																												{
																													if(bIsSimtor)
																													{
																														DI_Bits[i]=rand()%2;
																													}
																													else
																													{
																														if(!g_bRetrieve)
																															DI_Bits[i]=false;
																													}

																													attr = IoT_AddSensorNode(myGroup, DI[i].name);
																													if(attr)
																													{
																														DI[i].Bits=DI_Bits[i];
																														IoT_SetBoolValue(attr,DI[i].Bits,mode);	
																													}
																												}

																											}
																								}

																					}
																					else if(strcmp(sensors[count].type,"Digital Output")==0)
																					{
																								myGroup = IoT_AddGroup(myUpload, "Digital Output");
																								if(myGroup)
																								{
																											mode=IoT_READWRITE;
																											for(int i=0;i<numberOfDO;i++){
																												if(sensors[count].name,DO[i].name)
																												{
																													if(bIsSimtor)
																													{
																														DO_Bits[i]=rand()%2;
																													}
																													else
																													{
																														if(!g_bRetrieve)
																															DO_Bits[i]=false;
																													}
																													attr = IoT_AddSensorNode(myGroup, DO[i].name);
																													if(attr)
																													{
																														DO[i].Bits=DO_Bits[i];
																														IoT_SetBoolValue(attr,DO[i].Bits,mode);	
																													}
																												}
																											}
																								}

																					}
																					else if(strcmp(sensors[count].type,"Analog Input")==0)
																					{
																								myGroup = IoT_AddGroup(myUpload, "Analog Input");
																								if(myGroup)
																								{
																											mode=IoT_READONLY;
																											for(int i=0;i<numberOfAI;i++){
																												if(sensors[count].name,AI[i].name)
																												{
																													if(bIsSimtor)
																													{
																														while(true)
																														{
																															AI_Regs[i]=rand()%(int)AI[i].max/AI[i].precision;
																															if(AI_Regs[i]>=AI[i].min)
																																break;
																															else
																																continue;
																														}
																													}
																													else
																													{
																														if(!g_bRetrieve)
																															AI_Regs[i]=0;
																													}

																													attr = IoT_AddSensorNode(myGroup, AI[i].name);
																													if(attr)
																													{	
																														AI[i].Regs=AI_Regs[i];
																														IoT_SetDoubleValueWithMaxMin(attr,AI[i].Regs*AI[i].precision,mode,AI[i].max,AI[i].min,AI[i].unit);
																													}
																												}

																											}
																								}
																					}
																					else if(strcmp(sensors[count].type,"Analog Output")==0)
																					{
																								myGroup = IoT_AddGroup(myUpload, "Analog Output");
																								if(myGroup)
																								{
																											mode=IoT_READWRITE;
																											for(int i=0;i<numberOfAO;i++){
																												if(bIsSimtor)
																												{
																													while(true)
																													{
																														AO_Regs[i]=rand()%(int)AO[i].max/AO[i].precision;
																														if(AO_Regs[i]>=AO[i].min)
																															break;
																														else
																															continue;
																													}
																												}
																												else
																												{
																													if(!g_bRetrieve)
																														AO_Regs[i]=0;
																												}
																												attr = IoT_AddSensorNode(myGroup, AO[i].name);
																												if(attr)
																												{	
																													AO[i].Regs=AO_Regs[i];
																													IoT_SetDoubleValueWithMaxMin(attr,AO[i].Regs*AO[i].precision,mode,AO[i].max,AO[i].min,AO[i].unit);
																												}
																											}
																								}
																					}

																	}
																	#pragma endregion Have_name
															

															break;
															#pragma endregion Check_Level=2
													case 3:
															#pragma region Check_Level=3
															//printf("Check_Level=3\n");
															if(strcmp(sensors[count].type,"Platform")==0)
															{			char Slave_Port[6];
																		sprintf(Slave_Port,"%d",Modbus_Slave_Port);

																		myGroup = IoT_AddGroup(myUpload,"Platform");
																		if(myGroup)
																		{
																						mode=IoT_READONLY;
																						if(strcmp(sensors[count].name,"Name")==0)
																						{
																									attr = IoT_AddSensorNode(myGroup, "Name");
																									if(attr)
																										IoT_SetStringValue(attr,WISE_Name,mode);
																						}
																						else if(strcmp(sensors[count].name,"SlaveIP")==0)
																						{
																									attr = IoT_AddSensorNode(myGroup, "SlaveIP");
																									if(attr)
																										IoT_SetStringValue(attr,Modbus_Slave_IP,mode);
																						}
																						else if(strcmp(sensors[count].name,"SlavePort")==0)
																						{
																									attr = IoT_AddSensorNode(myGroup, "SlavePort");
																									if(attr)
																										IoT_SetStringValue(attr,Slave_Port,mode);
																						}
																						else if(strcmp(sensors[count].name,"Connection")==0)
																						{
																									attr = IoT_AddSensorNode(myGroup, "Connection");
																											
																									if(bIsSimtor)
																									{
																										IoT_SetBoolValue(attr,true,mode);
																									}
																									else
																									{
																										if(attr)
																											if(!g_bRetrieve)
																												IoT_SetBoolValue(attr,false,mode);
																											else
																												IoT_SetBoolValue(attr,true,mode);
																									}
																						}
																		
																		}

															}
															else if(strcmp(sensors[count].type,"Digital Input")==0)
															{
																		myGroup = IoT_AddGroup(myUpload, "Digital Input");
																		if(myGroup)
																		{
																					mode=IoT_READONLY;
																					for(int i=0;i<numberOfDI;i++){
																						if(strcmp(sensors[count].name,DI[i].name)==0)
																						{
																								if(bIsSimtor)
																								{
																									DI_Bits[i]=rand()%2;
																								}
																								else
																								{
																									if(!g_bRetrieve)
																										DI_Bits[i]=false;
																								}
																								attr = IoT_AddSensorNode(myGroup, DI[i].name);
																								if(attr)
																								{
																									DI[i].Bits=DI_Bits[i];
																									IoT_SetBoolValue(attr,DI[i].Bits,mode);	
																								}
																						}

																					}
																		}

															}
															else if(strcmp(sensors[count].type,"Digital Output")==0)
															{
																		myGroup = IoT_AddGroup(myUpload, "Digital Output");
																		if(myGroup)
																		{
																					mode=IoT_READWRITE;
																					for(int i=0;i<numberOfDO;i++){
																						if(strcmp(sensors[count].name,DO[i].name)==0)
																						{
																								if(bIsSimtor)
																								{
																									DO_Bits[i]=rand()%2;
																								}
																								else
																								{
																									if(!g_bRetrieve)
																										DO_Bits[i]=false;
																								}
																								attr = IoT_AddSensorNode(myGroup, DO[i].name);
																								if(attr)
																								{
																									DO[i].Bits=DO_Bits[i];
																									IoT_SetBoolValue(attr,DO[i].Bits,mode);	
																								}
																						}
																					}
																		}

															}
															else if(strcmp(sensors[count].type,"Analog Input")==0)
															{
																		myGroup = IoT_AddGroup(myUpload, "Analog Input");
																		if(myGroup)
																		{
																					mode=IoT_READONLY;
																					for(int i=0;i<numberOfAI;i++){
																						if(strcmp(sensors[count].name,AI[i].name)==0)
																						{
																								if(bIsSimtor)
																								{
																									while(true)
																									{
																										AI_Regs[i]=rand()%(int)AI[i].max/AI[i].precision;
																										if(AI_Regs[i]>=AI[i].min)
																											break;
																										else
																											continue;
																									}
																								}
																								else
																								{
																									if(!g_bRetrieve)
																										AI_Regs[i]=0;
																								}
																								attr = IoT_AddSensorNode(myGroup, AI[i].name);
																								if(attr)
																								{	
																									AI[i].Regs=AI_Regs[i];
																									IoT_SetDoubleValueWithMaxMin(attr,AI[i].Regs*AI[i].precision,mode,AI[i].max,AI[i].min,AI[i].unit);
																								}
																						}

																					}
																		}
															}
															else if(strcmp(sensors[count].type,"Analog Output")==0)
															{
																		myGroup = IoT_AddGroup(myUpload, "Analog Output");
																		if(myGroup)
																		{
																					mode=IoT_READWRITE;
																					for(int i=0;i<numberOfAO;i++){
																						if(strcmp(sensors[count].name,AO[i].name)==0)
																						{
																								if(bIsSimtor)
																								{
																									while(true)
																									{
																										AO_Regs[i]=rand()%(int)AO[i].max/AO[i].precision;
																										if(AO_Regs[i]>=AO[i].min)
																											break;
																										else
																											continue;
																									}
																								}
																								else
																								{
																									if(!g_bRetrieve)
																										AO_Regs[i]=0;
																								}
																								attr = IoT_AddSensorNode(myGroup, AO[i].name);
																								if(attr)
																								{	
																									AO[i].Regs=AO_Regs[i];
																									IoT_SetDoubleValueWithMaxMin(attr,AO[i].Regs*AO[i].precision,mode,AO[i].max,AO[i].min,AO[i].unit);
																								}
																						}

																					}
																		}

															}
															break;
															#pragma endregion Check_Level=3
												}
												#pragma endregion Check_Level

												

									}
									#pragma endregion DEF_HANDLER_NAME
							}			
							#pragma endregion g_Capability

					
					}
					#pragma endregion Data_paths

					repJsonStr = IoT_PrintData(myUpload);
					printf("\nNOT ALL Handler_Data = %s\n",repJsonStr);
					printf("---------------------\n");
					if(bReport_Upload)
					{	if(g_sendreportcbf)
							g_sendreportcbf(&g_PluginInfo, repJsonStr, strlen(repJsonStr), NULL, NULL);
					}					
					else
					{	if(g_sendcbf)
							g_sendcbf(&g_PluginInfo, modbus_auto_upload_rep, repJsonStr, strlen(repJsonStr)+1, NULL, NULL);
					}
					
					free(repJsonStr);
	}


	if(sensors)
		free(sensors);
	if(myUpload)
		IoT_ReleaseAll(myUpload);

}
//--------------------------------------------------------------------------------------------------------------
//------------------------------------------------Threads-------------------------------------------------------
//--------------------------------------------------------------------------------------------------------------
static void* RetrieveThreadStart(void *args)
{

	handler_context_t *pHandlerContex = (handler_context_t *)args;

	#pragma region Modbus_Retrieve
	while (g_PluginInfo.agentInfo->status == 0)
	{
		if(!pHandlerContex->isThreadRunning)
			return 0;
		sleep(1);
	}
	
	while(pHandlerContex->isThreadRunning)
	{  // printf("Retrieve.......\n");
		if(!bIsSimtor)
			g_bRetrieve=Modbus_Rev();
		sleep(0.5);
	}
	#pragma endregion Modbus_Retrieve

	return 0;

}

static void* AutoReportThreadStart(void *args)
{	

	handler_context_t *pHandlerContex = (handler_context_t *)args;
	int i = 0;
	bool bReport_Upload;

	while (g_PluginInfo.agentInfo->status == 0)
	{
		if(!pHandlerContex->isThreadRunning)
			return 0;
		sleep(1);
	}

	while(g_AutoReportContex.isThreadRunning)
	{
		#pragma region g_bAutoReport
		if(g_bAutoReport)
		{	
			bReport_Upload=true;
			printf("\nAutoReport...........\n");
			UploadSensorsDataEx(Report_Data_paths,Report_array_size,Report_Reply_All,bReport_Upload,args);
			sleep(Report_interval);
		}
		#pragma endregion g_bAutoReport


	}
	
	return 0;

}




static void* AutoUploadThreadStart(void *args)
{
	bool bReport_Upload;
	char tmpRepFilter[4096] = {0};
	cJSON *root=NULL;
	unsigned int diff=0;

	handler_context_t *pHandlerContex = (handler_context_t *)args;
	int i = 0;

	while (g_PluginInfo.agentInfo->status == 0)
	{
		if(!pHandlerContex->isThreadRunning)
			return 0;
		sleep(1);
	}

	while(g_AutoUploadContex.isThreadRunning)
	{
		#pragma region g_bAutoUpload
		if(g_bAutoUpload)
		{
					bool Reply_All=false;
					char **Data_paths=NULL;
					int array_size=0;

					cJSON *item, *it, *js_name,*js_list;

					#pragma region Reply_All 
					if(!Reply_All)
					{
							#pragma region root 
							root = cJSON_Parse(AutoUploadParams.repFilter);
							if(!root) 
							{
								printf("get root faild !\n");
								
							}
							else
							{
								js_list = cJSON_GetObjectItem(root, "e");
								#pragma region js_list
								if(!js_list)
								{
									printf("no list!\n");
								}
								else
								{	
									array_size = cJSON_GetArraySize(js_list);
									Data_paths = (char **)calloc(array_size, sizeof(char *));

									char *p  = NULL;
				
									for(i=0; i< array_size; i++)
									{

											item = cJSON_GetArrayItem(js_list, i);
											if(Data_paths)
												Data_paths[i] = (char *)calloc(200, sizeof(char));   //set Data_path size as 200*char
											if(!item)    
											{
												printf("no item!\n");
											}
											else
											{
												p = cJSON_PrintUnformatted(item);
												it = cJSON_Parse(p);
												
												if(!it)
													continue;
												else
												{
													js_name = cJSON_GetObjectItem(it, "n");
													if(Data_paths)
														if(Data_paths[i])
															strcpy(Data_paths[i],js_name->valuestring);
												}

												if(p)
													free(p);
												if(it)
													cJSON_Delete(it);

												if(strcmp(Data_paths[i],DEF_HANDLER_NAME)==0)
												{	Reply_All=true;
													printf("Reply_All=true;\n");
													break;
												}
																
											}
			
									}
							
									//printf("\n\n%d %d %s\n\n",AutoUploadParams.intervalTimeMs,AutoUploadParams.continueTimeMs,AutoUploadParams.repFilter);
													
								}
								#pragma endregion js_list
										
							}
							
							if(root)
								cJSON_Delete(root);
							#pragma endregion root
					}
					#pragma endregion Reply_All

					//AutoUpload_continue=clock();
					Continue_Upload=time(NULL);
					//printf("start: %d  con: %d\n",AutoUpload_start,AutoUpload_continue);
					//printf("diff: %d\n",AutoUpload_continue-AutoUpload_start);
					diff=(unsigned int)difftime(Continue_Upload,Start_Upload);
					printf("timeout: %d\n",AutoUploadParams.continueTimeMs/1000);
					printf("diff: %d\n",diff);
					//if((unsigned int)(AutoUpload_continue-AutoUpload_start) < AutoUploadParams.continueTimeMs)
					if(diff<AutoUploadParams.continueTimeMs/1000)
					{	

						bReport_Upload=false;
						UploadSensorsDataEx(Data_paths,array_size,Reply_All,bReport_Upload,args);
						sleep(AutoUploadParams.intervalTimeMs/1000);
					}
					else
					{
						g_bAutoUpload=false;
					}
					for(i=0;i<array_size; i++)
						if(Data_paths)
							if(Data_paths[i])
								free(Data_paths[i]);
					if(Data_paths)
						free(Data_paths);

		}
		#pragma endregion g_bAutoUpload


	}
	
	return 0;
}

//--------------------------------------------------------------------------------------------------------------
//--------------------------------------Handler Functions-------------------------------------------------------
//--------------------------------------------------------------------------------------------------------------
/* **************************************************************************************
 *  Function Name: Handler_Initialize
 *  Description: Init any objects or variables of this handler
 *  Input :  PLUGIN_INFO *pluginfo
 *  Output: None
 *  Return:  0  : Success Init Handler
 *              -1 : Fail Init Handler
 * ***************************************************************************************/
int HANDLER_API Handler_Initialize( HANDLER_INFO *pluginfo )
{
	if( pluginfo == NULL )
		return handler_fail;

	// 1. Topic of this handler
	snprintf( pluginfo->Name, sizeof(pluginfo->Name), "%s", strPluginName );
	pluginfo->RequestID = iRequestID;
	pluginfo->ActionID = iActionID;
	printf(" >Name: %s\r\n", strPluginName);
	// 2. Copy agent info 
	memcpy(&g_PluginInfo, pluginfo, sizeof(HANDLER_INFO));
	g_PluginInfo.agentInfo = pluginfo->agentInfo;

	// 3. Callback function -> Send JSON Data by this callback function
	g_sendcbf = g_PluginInfo.sendcbf = pluginfo->sendcbf;
	g_sendcustcbf = g_PluginInfo.sendcustcbf = pluginfo->sendcustcbf;
	g_subscribecustcbf = g_PluginInfo.subscribecustcbf = pluginfo->subscribecustcbf;
	g_sendreportcbf = g_PluginInfo.sendreportcbf = pluginfo->sendreportcbf;
	g_sendcapabilitycbf =g_PluginInfo.sendcapabilitycbf = pluginfo->sendcapabilitycbf;
	g_sendeventcbf = g_PluginInfo.sendeventcbf = pluginfo->sendeventcbf;

	g_RetrieveContex.threadHandler = NULL;
	g_RetrieveContex.isThreadRunning = false;
	g_AutoReportContex.threadHandler = NULL;
	g_AutoReportContex.isThreadRunning = false;
	g_AutoUploadContex.threadHandler = NULL;
	g_AutoUploadContex.isThreadRunning = false;

	g_status = handler_status_init;
	
	return handler_success;
}

/* **************************************************************************************
 *  Function Name: Handler_Uninitialize
 *  Description: Release the objects or variables used in this handler
 *  Input :  None
 *  Output: None
 *  Return:  void
 * ***************************************************************************************/
void Handler_Uninitialize()
{
	g_sendcbf = NULL;
	g_sendcustcbf = NULL;
	g_sendreportcbf = NULL;
	g_sendcapabilitycbf = NULL;
	g_subscribecustcbf = NULL;

	if(g_Capability)
	{
		IoT_ReleaseAll(g_Capability);
		g_Capability = NULL;
	}


}

/* **************************************************************************************
 *  Function Name: Handler_Get_Status
 *  Description: Get Handler Threads Status. CAgent will restart current Handler or restart CAgent self if busy.
 *  Input :  None
 *  Output: char * : pOutStatus       // cagent handler status
 *  Return:  handler_success  : Success Init Handler
 *			 handler_fail : Fail Init Handler
 * **************************************************************************************/
int HANDLER_API Handler_Get_Status( HANDLER_THREAD_STATUS * pOutStatus )
{
	int iRet = handler_fail; 

	if(!pOutStatus) return iRet;

	switch (g_status)
	{
	default:
	case handler_status_no_init:
	case handler_status_init:
	case handler_status_stop:
		*pOutStatus = g_status;
		break;
	case handler_status_start:
	case handler_status_busy:
		{
			/*time_t tv;
			time(&tv);
			if(difftime(tv, g_monitortime)>5)
				g_status = handler_status_busy;
			else
				g_status = handler_status_start;*/
			*pOutStatus = g_status;
		}
		break;
	}
	
	iRet = handler_success;
	return iRet;
}


/* **************************************************************************************
 *  Function Name: Handler_OnStatusChange
 *  Description: Agent can notify handler the status is changed.
 *  Input :  PLUGIN_INFO *pluginfo
 *  Output: None
 *  Return:  None
 * ***************************************************************************************/
void HANDLER_API Handler_OnStatusChange( HANDLER_INFO *pluginfo )
{
	printf(" %s> Update Status", strPluginName);
	if(pluginfo)
		memcpy(&g_PluginInfo, pluginfo, sizeof(HANDLER_INFO));
	else
	{
		memset(&g_PluginInfo, 0, sizeof(HANDLER_INFO));
		snprintf( g_PluginInfo.Name, sizeof( g_PluginInfo.Name), "%s", strPluginName );
		g_PluginInfo.RequestID = iRequestID;
		g_PluginInfo.ActionID = iActionID;
	}
}


/* **************************************************************************************
 *  Function Name: Handler_Start
 *  Description: Start Running
 *  Input :  None
 *  Output: None
 *  Return:  0  : Success Init Handler
 *              -1 : Fail Init Handler
 * ***************************************************************************************/
int HANDLER_API Handler_Start( void )
{
	char* result = NULL;
	char modulePath[200]={0};
	char iniPath[200]={0};
	printf("> %s Handler_Start\r\n", strPluginName);

	srand(time(NULL));

	// Load ini file
	bFind=read_INI_Platform(modulePath,iniPath);

	g_RetrieveContex.isThreadRunning = true;
	g_AutoReportContex.isThreadRunning = true;
	g_AutoUploadContex.isThreadRunning = true;
	
	if(bFind)
	{
		ctx = modbus_new_tcp(Modbus_Slave_IP, Modbus_Slave_Port);
		Modbus_Connect();
	}

	if(pthread_create(&g_RetrieveContex.threadHandler,NULL, RetrieveThreadStart, &g_RetrieveContex) != 0)
	{
		g_RetrieveContex.isThreadRunning = false;
		printf("> start Retrieve thread failed!\r\n");	
		return handler_fail;
    }

	if(pthread_create(&g_AutoReportContex.threadHandler,NULL, AutoReportThreadStart, &g_AutoReportContex) != 0)
	{
		g_AutoReportContex.isThreadRunning = false;
		printf("> start AutoReport thread failed!\r\n");	
		return handler_fail;
    }

	if(pthread_create(&g_AutoUploadContex.threadHandler,NULL, AutoUploadThreadStart, &g_AutoUploadContex) != 0)
	{
		g_AutoUploadContex.isThreadRunning = false;
		printf("> start AutoUpload thread failed!\r\n");	
		return handler_fail;
    }

	g_status = handler_status_start;


	time(&g_monitortime);
	return handler_success;
}

/* **************************************************************************************
 *  Function Name: Handler_Stop
 *  Description: Stop the handler
 *  Input :  None
 *  Output: None
 *  Return:  0  : Success Init Handler
 *              -1 : Fail Init Handler
 * ***************************************************************************************/
int HANDLER_API Handler_Stop( void )
{
	void *status;

	//printf("Handler Stop..................\n");
	
	if(g_RetrieveContex.isThreadRunning == true)
	{
		g_RetrieveContex.isThreadRunning = false;
		pthread_join((pthread_t)g_RetrieveContex.threadHandler,&status);
		g_RetrieveContex.threadHandler = NULL;
	}

	if(g_AutoReportContex.isThreadRunning == true)
	{
		g_AutoReportContex.isThreadRunning = false;
		pthread_join((pthread_t)g_AutoReportContex.threadHandler,&status);
		g_AutoReportContex.threadHandler = NULL;
	}

	if(g_AutoUploadContex.isThreadRunning == true)
	{
		int i=0;
		g_AutoUploadContex.isThreadRunning = false;
		pthread_join((pthread_t)g_AutoUploadContex.threadHandler,&status);
		g_AutoUploadContex.threadHandler = NULL;
		
		for(i=0;i<Report_array_size; i++)
		if(Report_Data_paths)
			if(Report_Data_paths[i])
				free(Report_Data_paths[i]);
		if(Report_Data_paths)
			free(Report_Data_paths);
	}


	if(DI_Bits)
		free(DI_Bits);
	if(DO_Bits)
		free(DO_Bits);
	if(AI_Regs)
		free(AI_Regs);
	if(AO_Regs)
		free(AO_Regs);
	if(DI)
		free(DI);
	if(DO)
		free(DO);
	if(AI)
		free(AI);
	if(AO)
		free(AO);
	if(modbus_get_socket(ctx)>0)
		Modbus_Disconnect();			
	modbus_free(ctx);

	g_status = handler_status_stop;
	return handler_success;
}

/* **************************************************************************************
 *  Function Name: Handler_Recv
 *  Description: Receive Packet from MQTT Server
 *  Input : char * const topic, 
 *			void* const data, 
 *			const size_t datalen
 *  Output: void *pRev1, 
 *			void* pRev2
 *  Return: None
 * ***************************************************************************************/
void HANDLER_API Handler_Recv(char * const topic, void* const data, const size_t datalen, void *pRev1, void* pRev2  )
{
	int cmdID = 0;
	char errorStr[128] = {0};
	ModbusLog(g_loghandle, Normal, " %s>Recv Topic [%s] Data %s", strPluginName, topic, (char*) data );
	printf(" >Recv Topic [%s] Data %s", topic, (char*) data );

	if(!ParseReceivedData(data, datalen, &cmdID))
		return;
	switch(cmdID)
	{
		case modbus_get_capability_req:
		{
			//printf("\n----------------------------------------\n");
			//printf("\n-------modbus_get_capability_req--------\n");
			//printf("\n----------------------------------------\n");
			GetCapability();
			break;
		}

		case modbus_get_sensors_data_req:
		{
			//printf("\n----------------------------------------\n");
			//printf("\n-------modbus_get_sensors_data_req------\n");
			//printf("\n----------------------------------------\n");
			char curSessionID[256] = {0};
			sensor_info_list sensorInfoList = CreateSensorInfoList();
			if(Parser_ParseGetSensorDataReqEx(data, sensorInfoList, curSessionID))
			{
				if(strlen(curSessionID))
				{   
					if(sensorInfoList != NULL || curSessionID != NULL)	
						GetSensorsDataEx(sensorInfoList, curSessionID);	
				}
			}
			else
			{
				char * errorRepJsonStr = NULL;
				char errorStr[128];
				sprintf(errorStr, "Command(%d) parse error!", modbus_set_sensors_data_req);
				int jsonStrlen = Parser_PackModbusError(errorStr, &errorRepJsonStr);
				//printf("error : %s\n",errorStr);
				if(jsonStrlen > 0 && errorRepJsonStr != NULL)
				{
					g_sendcbf(&g_PluginInfo, modbus_error_rep, errorRepJsonStr, strlen(errorRepJsonStr)+1, NULL, NULL);
				}
				if(errorRepJsonStr)free(errorRepJsonStr);
			}
			DestroySensorInfoList(sensorInfoList);
			break;

		}

		case modbus_set_sensors_data_req:
		{
			//printf("\n----------------------------------------\n");
			//printf("\n-------modbus_set_sensors_data_req------\n");
			//printf("\n----------------------------------------\n");
			char curSessionID[256] = {0};
			sensor_info_list sensorInfoList = CreateSensorInfoList();
			if(Parser_ParseSetSensorDataReqEx(data, sensorInfoList, curSessionID))
			{
				if(strlen(curSessionID))
				{   
					if(sensorInfoList != NULL || curSessionID != NULL)
						SetSensorsDataEx(sensorInfoList, curSessionID);
				}
			}
			else
			{
				char * errorRepJsonStr = NULL;
				char errorStr[128];
				sprintf(errorStr, "Command(%d) parse error!", modbus_set_sensors_data_req);
				int jsonStrlen = Parser_PackModbusError(errorStr, &errorRepJsonStr);
				//printf("error : %s\n",errorStr);
				if(jsonStrlen > 0 && errorRepJsonStr != NULL)
				{
					g_sendcbf(&g_PluginInfo, modbus_error_rep, errorRepJsonStr, strlen(errorRepJsonStr)+1, NULL, NULL);
				}
				if(errorRepJsonStr)free(errorRepJsonStr);
			}
			DestroySensorInfoList(sensorInfoList);
			break;

		}

		case modbus_auto_upload_req:
		{
			//printf("\n-----------------------------------\n");
			//printf("\n-------modbus_auto_upload_req------\n");
			//printf("\n-----------------------------------\n");
			unsigned int intervalTimeMs = 0; //ms
			unsigned int continueTimeMs = 0;
			char tmpRepFilter[4096] = {0};
			bool bRet = Parser_ParseAutoUploadCmd((char *)data, &intervalTimeMs, &continueTimeMs, tmpRepFilter);
			//printf("data = %s\n",(char *)data);
			//printf("\n\ntmpRepFilter = %s\n\n",tmpRepFilter);
			if(bRet)
			{

				AutoUploadParams.intervalTimeMs = intervalTimeMs; //ms
				AutoUploadParams.continueTimeMs = continueTimeMs;
				memset(AutoUploadParams.repFilter, 0, sizeof(AutoUploadParams.repFilter));
				if(strlen(tmpRepFilter)) strcpy(AutoUploadParams.repFilter, tmpRepFilter);
				g_bAutoUpload=true;
				//AutoUpload_start=clock();
				Start_Upload=time(NULL);
				//AutoUpload_continue=AutoUpload_start;
			}
			else
			{
				char * errorRepJsonStr = NULL;
				char errorStr[128];
				sprintf(errorStr, "Command(%d) parse error!", modbus_set_sensors_data_req);
				int jsonStrlen = Parser_PackModbusError(errorStr, &errorRepJsonStr);
				//printf("error : %s\n",errorStr);
				if(jsonStrlen > 0 && errorRepJsonStr != NULL)
				{
					g_sendcbf(&g_PluginInfo, modbus_error_rep, errorRepJsonStr, strlen(errorRepJsonStr)+1, NULL, NULL);
				}
				if(errorRepJsonStr)free(errorRepJsonStr);
			}
			
			break;

		}

	}

}

/* **************************************************************************************
 *  Function Name: Handler_AutoReportStart
 *  Description: Start Auto Report
 *  Input : char *pInQuery
 *  Output: None
 *  Return: None
 * ***************************************************************************************/
void HANDLER_API Handler_AutoReportStart(char *pInQuery)
{

#ifdef MY_DEBUG
	int i=0;

	if(g_bAutoReport)
	{
		for(i=0;i<Report_array_size; i++)
			if(Report_Data_paths)
				if(Report_Data_paths[i])
					free(Report_Data_paths[i]);
			if(Report_Data_paths)
				free(Report_Data_paths);
	}

	#pragma region root 
	//printf("\n\npInQuery = %s\n\n",pInQuery);

	Report_root=NULL;
	Report_Reply_All=false;
	Report_Data_paths=NULL;
	Report_array_size=0;

	Report_root = cJSON_Parse(pInQuery);
	if(!Report_root) 
	{
		printf("get root faild !\n");
		
	}
	else
	{	
				Report_first=cJSON_GetObjectItem(Report_root,"susiCommData");
				if(Report_first)
				{
					Report_second_interval=cJSON_GetObjectItem(Report_first,"autoUploadIntervalSec");
					if(Report_second_interval)
					{		Report_interval=Report_second_interval->valueint;
							//printf("interval : %d\n",Report_interval);
							Report_second=cJSON_GetObjectItem(Report_first,"requestItems");
							if(Report_second)
							{	Report_third=cJSON_GetObjectItem(Report_second,"All");
								if(Report_third)
									Report_Reply_All=true;
								else
								{
								Report_third=cJSON_GetObjectItem(Report_second,"Modbus_Handler");
								if(Report_third)
									Report_js_list=cJSON_GetObjectItem(Report_third,"e");
								}
												
							}	
					}
				}

				#pragma region Report_Reply_All
				if(!Report_Reply_All)
				{		
						#pragma region Report_js_list
						if(!Report_js_list)
						{
							printf("no list!\n");
							g_bAutoReport=false;
						}
						else
						{	
							Report_array_size = cJSON_GetArraySize(Report_js_list);
							Report_Data_paths = (char **)calloc(Report_array_size, sizeof(char *));

							char *p  = NULL;

							for(i=0; i< Report_array_size; i++)
							{

									Report_item = cJSON_GetArrayItem(Report_js_list, i);
									if(Report_Data_paths)
										Report_Data_paths[i] = (char *)calloc(200, sizeof(char));   //set Data_path size as 200*char
									if(!Report_item)    
									{
										printf("no item!\n");
									}
									else
									{
										p = cJSON_PrintUnformatted(Report_item);
										Report_it = cJSON_Parse(p);
										
										if(!Report_it)
											continue;
										else
										{
											Report_js_name = cJSON_GetObjectItem(Report_it, "n");
											if(Report_Data_paths)
												if(Report_Data_paths[i])
													strcpy(Report_Data_paths[i],Report_js_name->valuestring);
											//printf("Data : %s\n",Report_Data_paths[i]);
										}

										if(p)
											free(p);
										if(Report_it)
											cJSON_Delete(Report_it);

										if(strcmp(Report_Data_paths[i],DEF_HANDLER_NAME)==0)
										{	Report_Reply_All=true;
											printf("Report_Reply_All=true;\n");
											break;
										}
														
									}

							}
							if(Report_third)
								g_bAutoReport=true;
							else
								g_bAutoReport=false;
											
						}
						#pragma endregion Report_js_list
				}
				else
				{	printf("Report_Reply_All=true;\n");
					if(Report_third)
						g_bAutoReport=true;
					else
						g_bAutoReport=false;
				}
				#pragma endregion Report_Reply_All
	}
	
	if(Report_root)
		cJSON_Delete(Report_root);
	#pragma endregion Report_root
#else
	printf("Reply_All=true;\n");
	Report_interval=1;
	Report_Reply_All=true;
	g_bAutoReport=true;
#endif
	
}

/* **************************************************************************************
 *  Function Name: Handler_AutoReportStop
 *  Description: Stop Auto Report
 *  Input : None
 *  Output: None
 *  Return: None
 * ***************************************************************************************/
void HANDLER_API Handler_AutoReportStop(char *pInQuery)
{
	/*TODO: Parsing received command*/
#ifdef MY_DEBUG
	int i=0;
	for(i=0;i<Report_array_size; i++)
	if(Report_Data_paths)
		if(Report_Data_paths[i])
			free(Report_Data_paths[i]);
	if(Report_Data_paths)
		free(Report_Data_paths);
	g_bAutoReport = false;
#else
	g_bAutoReport = false;
#endif

}

/* **************************************************************************************
 *  Function Name: Handler_Get_Capability
 *  Description: Get Handler Information specification. 
 *  Input :  None
 *  Output: char ** : pOutReply       // JSON Format
 *  Return:  int  : Length of the status information in JSON format
 *                :  0 : no data need to trans
 * **************************************************************************************/
int HANDLER_API Handler_Get_Capability( char ** pOutReply ) // JSON Format
{
	char* result = NULL;
	int len = 0;
	if(!pOutReply) return len;
	bFind = read_INI();
	if(g_Capability)
	{
		IoT_ReleaseAll(g_Capability);
		g_Capability = NULL;
	}

	g_Capability = CreateCapability();
	
	result = IoT_PrintCapability(g_Capability);
   
	//printf("Handler_Get_Capability=%s\n",result);
	//printf("---------------------\n");

	len = strlen(result);
	*pOutReply = (char *)malloc(len + 1);
	memset(*pOutReply, 0, len + 1);
	strcpy(*pOutReply, result);
	free(result);
	return len;
}
