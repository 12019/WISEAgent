#ifndef _SUSI_IOT_API_H_
#define _SUSI_IOT_API_H_

#include <jansson.h>
#include "SusiIoT.h"

/* Definition */
#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#ifndef SUSI_IOT_API
#define SUSI_IOT_API __stdcall
#endif
#else
#define SUSI_IOT_API
#endif

#ifdef __cplusplus
extern "C" {
#endif

	/*-----------------------------------------------------------------------------
	//
	//	APIs
	//
	//-----------------------------------------------------------------------------
	//=============================================================================
	// Library
	//=============================================================================
	// Should be called before calling any other API function is called.
	//
	// Condition								| Return Values 
	// -----------------------------------------+------------------------------
	// Library Already initialized				| SUSI_STATUS_INITIALIZED
	// Else										| SUSI_STATUS_SUCCESS
	*/
	SusiIoTStatus_t SUSI_IOT_API SusiIoTInitialize(void);

	/* Should be called before program exit 
	//
	// Condition								| Return Values 
	// -----------------------------------------+------------------------------
	// Library Uninitialized					| SUSI_STATUS_NOT_INITIALIZED
	// Else										| SUSI_STATUS_SUCCESS
	*/
	SusiIoTStatus_t SUSI_IOT_API SusiIoTUninitialize(void);

	/* Capability Probe & Data Getter/Setter */
	typedef void (*SUSI_IOT_EVENT_CALLBACK)(SusiIoTId_t id, json_t *data);

	SusiIoTStatus_t SUSI_IOT_API SusiIoTGetPFCapability(json_t *capability);
	SusiIoTStatus_t SUSI_IOT_API SusiIoTGetPFData(SusiIoTId_t id, json_t *data);
	SusiIoTStatus_t SUSI_IOT_API SusiIoTSetPFData(json_t *data);
	SusiIoTStatus_t SUSI_IOT_API SusiIoTSetPFEventHandler (SUSI_IOT_EVENT_CALLBACK *eventCallbackFun);

	/* Getter for String */
	const char *SUSI_IOT_API SusiIoTGetPFCapabilityString();
	const char *SUSI_IOT_API SusiIoTGetPFDataString(SusiIoTId_t id);
	SusiIoTStatus_t SUSI_IOT_API SusiIoTSetPFDataString(char* jsonString);

	/* Data API */
	SusiIoTStatus_t SUSI_IOT_API SusiIoTGetValue(SusiIoTId_t id, json_t *jValue);
	SusiIoTStatus_t SUSI_IOT_API SusiIoTSetValue(SusiIoTId_t id, json_t *jValue);

	/* Memory API */
	SusiIoTStatus_t SUSI_IOT_API SusiIoTMemFree(void *address);

#ifdef __cplusplus
}
#endif

#endif /* _SUSI_IOT_API_H_ */
