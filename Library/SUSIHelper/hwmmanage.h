#ifndef _HWM_MANAGE_H_
#define _HWM_MANAGE_H_

#define DEF_HWMTYPE_LENGTH				32
#define DEF_HWMNAME_LENGTH				32
#define DEF_HWMTAG_LENGTH				8
#define DEF_HWMUNIT_LENGTH				16
#define DEF_INVALID_VALUE				(-999)    
#define DEF_TEMP_KELVINS_OFFSET			2731

#define  DEF_MAX_VOLTAGE_TSHD         500
#define  DEF_MIN_VOLTAGE_TSHD         (-500)
#define  DEF_MAX_TEMP_TSHD            100
#define  DEF_MIN_TEMP_TSHD            (-100)
#define  DEF_MAX_FAN_TSHD             (90000)
#define  DEF_MIN_FAN_TSHD             0
#define  DEF_MIN_CASEOP_TSHD		  0
#define  DEF_MAX_CASEOP_TSHD		  1
#define  DEF_MIN_PRESET_TSHD		  -999
#define  DEF_MAX_PRESET_TSHD		  999

#define DEF_THR_UNKNOW_TYPE				0
#define DEF_THR_MAX_TYPE				1
#define DEF_THR_MIN_TYPE				2
#define DEF_THR_MAXMIN_TYPE				3
#define DEF_THR_BOOL_TYPE				4
#define DEF_TEMP_THSHD_TYPE           DEF_THR_MAX_TYPE
#define DEF_VOLT_THSHD_TYPE           DEF_THR_MAXMIN_TYPE
#define DEF_FAN_THSHD_TYPE            DEF_THR_MIN_TYPE

#define DEF_SENSORTYPE_TEMPERATURE		"temperature"
#define DEF_UNIT_TEMPERATURE_KELVIN		"kelvins"
#define DEF_UNIT_TEMPERATURE_CELSIUS	"Celsius"
#define DEF_UNIT_TEMPERATURE_FAHRENHEIT	"Fahrenheit"
#define DEF_SENSORTYPE_VOLTAGE			"voltage"
#define DEF_UNIT_VOLTAGE				"V"
#define DEF_SENSORTYPE_FANSPEED			"fanspeed"
#define DEF_UNIT_FANSPEED				"RPM"
#define DEF_SENSORTYPE_CURRENT			"current"
#define DEF_UNIT_CURRENT				"A"
#define DEF_SENSORTYPE_CASEOPEN			"caseopen"
#define DEF_UNIT_CASEOPEN				""

typedef struct hwm_item{
	char type[DEF_HWMTYPE_LENGTH];
	char name[DEF_HWMNAME_LENGTH];
	char tag[DEF_HWMTAG_LENGTH];
	char unit[DEF_HWMUNIT_LENGTH];
	float value;

	int maxThreshold;
	int minThreshold;
	int thresholdType;

	struct hwm_item *prev;
	struct hwm_item *next;
}hwm_item_t;

typedef struct{
	int total;
	hwm_item_t* items;
}hwm_info_t;

#ifdef __cplusplus
extern "C" {
#endif

hwm_item_t * hwm_LastItem(hwm_info_t * pHWMInfo);
hwm_item_t * hwm_AddItem(hwm_info_t * pHWMInfo, char const * type, char const* name, char const * tag, char const * unit, float value);
void hwm_RemoveItem(hwm_info_t * pHWMInfo, char const * tag);
hwm_item_t * hwm_FindItem(hwm_info_t * pHWMInfo, char const * tag);

#ifdef __cplusplus
}
#endif

#endif /* _HWM_MANAGE_H_ */