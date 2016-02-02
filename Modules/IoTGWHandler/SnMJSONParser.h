#ifndef _SNM_JSON_PARSER_H_
#define _SNM_JSON_PARSER_H_

#include "platform.h"
#include "cJSON.h"


#define DEF_SENHUB_HANDLER_NAME              "SenHub"
#define DEF_IOTGW_HANDLER_NAME                 "IoTGW"

//--------------------------Sensor Hub ----------------------
#define SENHUB_CAPABILITY                   "SenHub"
#define SENHUB_SENDATA					    "SenData"
#define SENHUB_THR_MAX                      "max"
#define SENHUB_THR_MIN                       "min"
#define SENHUB_THR_TYPE                      "type"

// ------------------------- SenML -----------------------------
#define SENML_E_FLAG								"e"
#define SENML_N_FLAG								"n"
#define SENML_V_FLAG								"v"
#define SENML_SV_FLAG							"sv"
#define SENML_BV_FLAG							"bv"
#define SENML_ASM_FLAG                     "asm"
#define SENML_RW                                     "rw"
#define SENML_R                                          "r"
#define SENML_W                                        "w"



//-----------------------------------------------------------------
#ifdef __cplusplus
extern "C" {
#endif

typedef enum 
{
	SenML_Unknow_Type = -1, // Unknow Value Type  or Not Found
	SenML_Type_V,					    //  (v) Decimal, Integrate, Float  Value
	SenML_Type_SV,						//  (sv) String Value
	SenML_Type_BV,						//  (bv) Boolean Value
	SenML_Type_MAX,                 //   Size of SENML_TYPE
}SenML_V_Type;

typedef enum
{
	JSON_ROOT = 1000,
	JSON_CHLID = 1001,
	RESOURCE    = 1002,
}JSON_Type; // JSON Root, JSON It

typedef enum
{
	UNKNOW        = 0, 
	READ_ONLY    = 1,
	WRITE_ONLY   = 2,
	RW                    = 3, 
}ASM;

static const char SenML_Types[SenML_Type_MAX][16]={{"v"},{"sv"},{"bv"}};

int CheckDataIsWritable( const cJSON *jsonItem );
SenML_V_Type GetSenMLValueType( const cJSON *pItem );
SenML_V_Type GetSenMLValue( const char *pData , char *pOutValue, int nSize);
char *GetSenMLDataValue( const char *pData/*{"n":"door temp", "u":"Cel","v":0}*/,const char *key );
int GetJSONbySpecifCategory( char *data , char *pCategory1, char *pCategory2, char *pOutBuf, int BufSize );
int GetSenML_ElementCount( const char * data );
int GetElementSenMLNameValue( const char *data /*{"e":	[{"n":	"door temp","v":	0},{"n":"co2","v":1000}],"bn":	"SenData"} */,  const int i, 
															   char *pOutItem, int nItemSize,
															   char *poutName, int nNameSzie, 
															   char *pOutValue, int nValueSzie);

ASM GetSenMLVarableRW( const cJSON *pItem );
#ifdef __cplusplus
}
#endif

#endif // endif _SNM_JSON_PARSER_H_