#include "hwmmanage.h"
#include <stdlib.h>
#include <string.h>
#include "AdvPlatform.h"

//----------------------------------Susi Item Management----------------------------------------
hwm_item_t * hwm_LastItem(hwm_info_t * pHWMInfo)
{
	hwm_item_t *item = pHWMInfo->items;
	hwm_item_t *target = NULL;
	//printf("Find Last\n"); 
	while(item != NULL)
	{
		//printf("Topic Name: %s\n", topic->name);
		target = item;
		item = item->next;
	}
	return target;
}

hwm_item_t * hwm_AddItem(hwm_info_t * pHWMInfo, char const * type, char const* name, char const * tag, char const * unit, float value)
{
	hwm_item_t *item = NULL;

	item = (hwm_item_t *)malloc(sizeof(hwm_item_t));

	if (item == NULL)
		return NULL;

	strncpy(item->type, type, strlen(type)+1);
	strncpy(item->name, name, strlen(name)+1);
	strncpy(item->tag, tag, strlen(tag)+1);
	strncpy(item->unit, unit, strlen(unit)+1);
	item->value = value;

	if(strcasecmp(type, DEF_SENSORTYPE_TEMPERATURE)==0)
	{
		item->maxThreshold = DEF_MAX_TEMP_TSHD;
		item->minThreshold = DEF_MIN_TEMP_TSHD;
		item->thresholdType = DEF_TEMP_THSHD_TYPE;
	}
	else if(strcasecmp(type, DEF_SENSORTYPE_VOLTAGE)==0)
	{
		item->maxThreshold = DEF_MAX_VOLTAGE_TSHD;
		item->minThreshold = DEF_MIN_VOLTAGE_TSHD;
		item->thresholdType = DEF_VOLT_THSHD_TYPE;
	}
	else if(strcasecmp(type, DEF_SENSORTYPE_FANSPEED)==0)
	{
		item->maxThreshold = DEF_MAX_FAN_TSHD;
		item->minThreshold = DEF_MIN_FAN_TSHD;
		item->thresholdType = DEF_FAN_THSHD_TYPE;
	}
	else if(strcasecmp(type, DEF_SENSORTYPE_CURRENT)==0)
	{
		item->maxThreshold = DEF_MAX_PRESET_TSHD;
		item->minThreshold = DEF_MIN_PRESET_TSHD;
		item->thresholdType = DEF_THR_MAXMIN_TYPE;
	}
	else if(strcasecmp(type, DEF_SENSORTYPE_CASEOPEN)==0)
	{
		item->maxThreshold = DEF_MAX_CASEOP_TSHD;
		item->minThreshold = DEF_MIN_CASEOP_TSHD;
		item->thresholdType = DEF_THR_BOOL_TYPE;
	}

	item->next = NULL;	
	item->prev = NULL;	

	if(pHWMInfo->items == NULL)
	{
		pHWMInfo->items = item;
	} else {
		hwm_item_t *lastitem = hwm_LastItem(pHWMInfo);
		//printf("Last Topic Name: %s\n", lasttopic->name);
		lastitem->next = item;
		item->prev = lastitem;
	}
	pHWMInfo->total++;
	return item;
}

void hwm_RemoveItem(hwm_info_t * pHWMInfo, char const * tag)
{
	hwm_item_t *item = pHWMInfo->items;
	hwm_item_t *target = NULL;
	//printf("Remove Topic\n");
	while(item != NULL)
	{
		//printf("Topic Name: %s\n", topic->name);
		if(strcmp(item->tag, tag) == 0)
		{
			if(pHWMInfo->items == item)
				pHWMInfo->items = item->next;
			if(item->prev != NULL)
				item->prev->next = item->next;
			if(item->next != NULL)
				item->next->prev = item->prev;
			target = item;
			break;
		}
		item = item->next;
	}
	if(target!=NULL)
	{
		pHWMInfo->total--;
		free(target);
		target = NULL;
	}
}

hwm_item_t * hwm_FindItem(hwm_info_t * pHWMInfo, char const * tag)
{
	hwm_item_t *item = pHWMInfo->items;
	hwm_item_t *target = NULL;

	//printf("Find Topic\n");
	while(item != NULL)
	{
		//printf("Topic Name: %s\n", topic->name);
		if(strcmp(item->tag, tag) == 0)
		{
			target = item;
			break;
		}
		item = item->next;
	}
	return target;
}
//----------------------------------------------------------------------------------------------