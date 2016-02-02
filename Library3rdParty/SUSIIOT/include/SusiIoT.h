#ifndef _SUSI_IOT_H_
#define _SUSI_IOT_H_

typedef uint32_t SusiIoTStatus_t;
typedef uint32_t SusiIoTId_t;

/*-----------------------------------------------------------------------------
//
//	Status codes
//
//-----------------------------------------------------------------------------*/
/* Description
 *   The SUSI API library is not yet or unsuccessfully initialized. 
 *   SusiLibInitialize needs to be called prior to the first access of any 
 *   other SUSI API function.
 * Actions
 *   Call SusiLibInitialize..
 */
#define SUSIIOT_STATUS_NOT_INITIALIZED				((SusiIoTStatus_t)0xFFFFFFFF)

/* Description
 *   Library is initialized.
 * Actions
 *   none.
 */
#define SUSIIOT_STATUS_INITIALIZED					((SusiIoTStatus_t)0xFFFFFFFE)

/* Description
 *   Memory Allocation Error.
 * Actions
 *   Free memory and try again..
 */
#define SUSIIOT_STATUS_ALLOC_ERROR					((SusiIoTStatus_t)0xFFFFFFFD)

/* Description 
 *   Time out in driver. This is Normally caused by hardware/software 
 *   semaphore timeout. 
 * Actions
 *   Retry.
 */
#define SUSIIOT_STATUS_DRIVER_TIMEOUT				((SusiIoTStatus_t)0xFFFFFFFC)

/* Description 
 *  One or more of the SUSI API function call parameters are out of the 
 *  defined range.
 * Actions
 *   Verify Function Parameters.
 */
#define SUSIIOT_STATUS_INVALID_PARAMETER			((SusiIoTStatus_t)0xFFFFFEFF)

/* Description
 *   The Block Alignment is incorrect.
 * Actions
 *   Use Inputs and Outputs to correctly select input and outputs. 
 */
#define SUSIIOT_STATUS_INVALID_BLOCK_ALIGNMENT		((SusiIoTStatus_t)0xFFFFFEFE)

/* Description
 *   This means that the Block length is too long.
 * Actions
 *   Use Alignment Capabilities information to correctly align write access.
 */
#define SUSIIOT_STATUS_INVALID_BLOCK_LENGTH			((SusiIoTStatus_t)0xFFFFFEFD)

/* Description
 *   The current Direction Argument attempts to set GPIOs to a unsupported 
 *   directions. I.E. Setting GPI to Output.
 * Actions
 *   Use Inputs and Outputs to correctly select input and outputs. 
 */
#define SUSIIOT_STATUS_INVALID_DIRECTION			((SusiIoTStatus_t)0xFFFFFEFC)

/* Description
 *   The Bitmask Selects bits/GPIOs which are not supported for the current ID.
 * Actions
 *   Use Inputs and Outputs to probe supported bits..
 */
#define SUSIIOT_STATUS_INVALID_BITMASK				((SusiIoTStatus_t)0xFFFFFEFB)

/* Description
 *   Watchdog timer already started.
 * Actions
 *   Call SusiWDogStop, before retrying.
 */
#define SUSIIOT_STATUS_RUNNING						((SusiIoTStatus_t)0xFFFFFEFA)

/* Description
 *   This function or ID is not supported at the actual hardware environment.
 * Actions
 *   none.
 */
#define SUSIIOT_STATUS_UNSUPPORTED					((SusiIoTStatus_t)0xFFFFFCFF)

/* Description
 *   Selected device was not found
 * Actions
 *   none.
 */
#define SUSIIOT_STATUS_NOT_FOUND					((SusiIoTStatus_t)0xFFFFFBFF)

/* Description
 *    Device has no response.
 * Actions
 *   none.
 */
#define SUSIIOT_STATUS_TIMEOUT						((SusiIoTStatus_t)0xFFFFFBFE)

/* Description
 *   The selected device or ID is busy or a data collision was detected.
 * Actions
 *   Retry.
 */
#define SUSIIOT_STATUS_BUSY_COLLISION				((SusiIoTStatus_t)0xFFFFFBFD)

/* Description
 *   An error was detected during a read operation.
 * Actions
 *   Retry.
 */
#define SUSIIOT_STATUS_READ_ERROR					((SusiIoTStatus_t)0xFFFFFAFF)

/* Description
 *   An error was detected during a write operation.
 * Actions
 *   Retry.
 */
#define SUSIIOT_STATUS_WRITE_ERROR					((SusiIoTStatus_t)0xFFFFFAFE)


/* Description
 *   The amount of available data exceeds the buffer size. Storage buffer 
 *   overflow was prevented. Read count was larger than the defined buffer
 *   length.
 * Actions
 *   Either increase the buffer size or reduce the block length.
 */
#define SUSIIOT_STATUS_MORE_DATA					((SusiIoTStatus_t)0xFFFFF9FF)

/* Description
 *   Generic error message. No further error details are available.
 * Actions
 *   none.
 */
#define SUSIIOT_STATUS_ERROR						((SusiIoTStatus_t)0xFFFFF0FF)

/* Description
 *   The operation was successful.
 * Actions
 *   none.
 */
#define SUSIIOT_STATUS_SUCCESS						((SusiIoTStatus_t)0)

/* Description
 *   The json type is not correctly.
 * Actions
 *   none.
 */
#define SUSIIOT_STATUS_JSON_TYPE_ERROR				((SusiIoTStatus_t)0xFFFFF8FF)

/* Description
 *   The json object is empty.
 * Actions
 *   none.
 */
#define SUSIIOT_STATUS_JSON_OBJECT_EMPTY				((SusiIoTStatus_t)0xFFFFF8FE)

/* ----------------------------------------------------------------------------
 *	APIs
 * ----------------------------------------------------------------------------
 * ID format:
 *		31		28		24		20		16		12		8		4		0
 *		+-------+-------+-------+-------+-------+-------+-------+-------+
 *		|      Lib	    |	  Class	    |	  Type		| 	  Index		|
 *		+-------+-------+-------+-------+-------+-------+-------+-------+
 *
 *		Lib:		Library class code, ex: SUSI, SAB2000...
 *		Class:		Main Class code, ex: HWM, WDT, SMBus...
 *		Type:		Type code, ex: Temperature, Voltage...
 *		Index:		The index of class items
 */
#define SUSI_IOT_ID_GET_LIB(Id)						(((Id) & 0xFF000000) >> 3*8)
#define SUSI_IOT_ID_GET_CLASS(Id)					(((Id) & 0x00FF0000) >> 2*8)
#define SUSI_IOT_ID_GET_TYPE(Id)					(((Id) & 0x0000FF00) >> 1*8)
#define SUSI_IOT_ID_GET_INDEX(Id)					((Id) & 0xFF)

#endif /* _SUSI_IOT_H_ */