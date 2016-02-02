#ifndef _MESSAGE_GENERATE_H_
#define _MESSAGE_GENERATE_H_
#include <stdbool.h>

#define DEF_NAME_SIZE				260
#define DEF_CLASSIFY_SIZE			32
#define DEF_UNIT_SIZE				64
#define DEF_VERSION_SIZE			16
#define DEF_ASM_SIZE				3

typedef enum {
	attr_type_unknown = 0,
	attr_type_numeric = 1,
	attr_type_boolean = 2,
	attr_type_string = 3,
} attr_type;

typedef enum {
	class_type_root = 0,
	class_type_object = 1,
	class_type_array = 2,
} class_type;

typedef struct msg_attr{
	char name[DEF_NAME_SIZE];
	char readwritemode[DEF_ASM_SIZE];
	attr_type type;
	double v;
	bool bv;
	char* sv;
	double max;
	double min;
	char unit[DEF_UNIT_SIZE];
	bool bRange;
	bool bSensor;
	struct msg_attr *next;
}MSG_ATTRIBUTE_T;

typedef struct msg_class{
	char classname[DEF_NAME_SIZE];
	char version[DEF_VERSION_SIZE];
	bool bIoTFormat;
	class_type type;
	struct msg_attr *attr_list;
	struct msg_class *sub_list;
	struct msg_class *next;
}MSG_CLASSIFY_T;

#ifdef __cplusplus
extern "C" {
#endif

	MSG_CLASSIFY_T* MSG_CreateRoot();
	void MSG_ReleaseRoot(MSG_CLASSIFY_T* classify);

	MSG_CLASSIFY_T* MSG_AddClassify(MSG_CLASSIFY_T *pNode, char const* name, char const* version, bool bArray, bool isIoT);
	MSG_CLASSIFY_T* MSG_FindClassify(MSG_CLASSIFY_T* pNode, char const* name);
	bool MSG_DelClassify(MSG_CLASSIFY_T* pNode, char* name);

	MSG_ATTRIBUTE_T* MSG_AddAttribute(MSG_CLASSIFY_T* pClass, char const* attrname, bool isSensorData);
	MSG_ATTRIBUTE_T* MSG_FindAttribute(MSG_CLASSIFY_T* root, char const* attrname, bool isSensorData);
	bool MSG_DelAttribute(MSG_CLASSIFY_T* pNode, char* name, bool isSensorData);

	bool MSG_SetFloatValue(MSG_ATTRIBUTE_T* attr, float value, char* readwritemode, char *unit);
	bool MSG_SetFloatValueWithMaxMin(MSG_ATTRIBUTE_T* attr, float value, char* readwritemode, float max, float min, char *unit);
	bool MSG_SetDoubleValue(MSG_ATTRIBUTE_T* attr, double value, char* readwritemode, char *unit);
	bool MSG_SetDoubleValueWithMaxMin(MSG_ATTRIBUTE_T* attr, double value, char* readwritemode, double max, double min, char *unit);
	bool MSG_SetBoolValue(MSG_ATTRIBUTE_T* attr, bool bvalue, char* readwritemode);
	bool MSG_SetStringValue(MSG_ATTRIBUTE_T* attr, char *svalue, char* readwritemode);

	char* MSG_PrintUnformatted(MSG_CLASSIFY_T* msg);
	char *MSG_PrintWithFiltered(MSG_CLASSIFY_T* msg, char** filter, int length);

//----------------------------------others
	bool MSG_Find_Sensor(MSG_CLASSIFY_T *msg,char *path);
//	int  MSG_Find_Type_Sensor(MSG_CLASSIFY_T *msg,char *path);

#define MSG_AddJSONClassify(pNode, name, version, bArray) MSG_AddClassify(pNode, name, version, bArray, false);
#define MSG_AddIoTClassify(pNode, name, version, bArray) MSG_AddClassify(pNode, name, version, bArray, true);

#define MSG_AddJSONAttribute(pClass, attrname) MSG_AddAttribute(pClass, attrname, false);
#define MSG_AddIoTSensor(pClass, attrname) MSG_AddAttribute(pClass, attrname, true);

#define MSG_FindJSONAttribute(pClass, attrname) MSG_FindAttribute(pClass, attrname, false);
#define MSG_FindIoTSensor(pClass, attrname) MSG_FindAttribute(pClass, attrname, true);

#define MSG_DelJSONAttribute(pClass, attrname) MSG_DelAttribute(pClass, attrname, false);
#define MSG_DelIoTSensor(pClass, attrname) MSG_DelAttribute(pClass, attrname, true);

#ifdef __cplusplus
}
#endif
#endif