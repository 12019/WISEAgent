#ifndef _SUSI_H_
#define _SUSI_H_

#ifdef __cplusplus 
extern "C" { 
#endif 

#ifndef _WINDOWS_TYPE_DEF
#define _WINDOWS_TYPE_DEF

typedef int             BOOL;
typedef unsigned int   DWORD;
typedef unsigned short  WORD;
typedef unsigned char   BYTE;
typedef char            CHAR;
typedef char            TCHAR;
typedef unsigned char   UCHAR;

#endif /* _WINDOWS_TYPE_DEF*/

#define TRUE            (1)
#define FALSE           (0)

/******************************************************************************
//	Core Definition 
// ****************************************************************************
//-----------------------------------------------------------------------------
//	CPU Information
//----------------------------------------------------------------------------*/
/* Vendor */
#define INTEL		1 << 0
#define VIA  		1 << 1
#define SIS  		1 << 2
#define NVIDIA  	1 << 3
#define AMD  		1 << 4
#define RDC  		1 << 5

/******************************************************************************
//	GPIO Definition 
// ****************************************************************************/
typedef enum
{
   /* Static mask */
   ESIO_SMASK_PIN_FULL			= 0x0001,
   ESIO_SMASK_CONFIGURABLE		= 0x0002,
   ESIO_SMASK_FIXIN				= 0x0004,
   ESIO_SMASK_FIXOUT			= 0x0008,
   ESIO_SMASK_IF_OUT_READABLE	= 0x0010,
   /* Dynamic mask */
   ESIO_DMASK_DIRECTION			= 0x0020,
   ESIO_DMASK_IN				= 0x0040,   
   ESIO_DMASK_OUT				= 0x0080,
   ESIO_DMASK_READABLEOUT		= 0x0100
} ESIO_MASKFLAG;


/******************************************************************************
//	HWM Definition
// ****************************************************************************/
// Voltage
#define VMAX_SHIFT	15	
#define VCORE	(1 << 0)			// Vcore
#define V25		(1 << 1)			// 2.5V
#define V33		(1 << 2)			// 3.3V
#define V50		(1 << 3)			// 5V
#define V120	(1 << 4)			// 12V
#define V5SB	(1 << 5)			// V5 standby
#define V3SB	(1 << 6)			// V3 standby
#define VBAT	(1 << 7)			// VBAT
#define VN50	(1 << 8)			// -5V
#define VN120	(1 << 9)			// -12V
#define VTT		(1 << 10)			// VTT
#define VCORE2	(1 << 11)			// Vcore2
#define V105	(1 << 12)			// 1.05V
#define V15		(1 << 13)			// 1.5V
#define V18		(1 << 14)	// 1.8V
#define V240		(1 << VMAX_SHIFT)	// 24V

/* Temperature */
#define TCPU	(1 << 0)	/* CPU temperaturez */
#define TSYS	(1 << 1)	/* System temperature */
#define TAUX    (1 << 2)	/* 3'rd thermal dioad */
#define TCPU2	(1 << 3)	/* CPU 2 temperature */

/* Fan Speed */
#define FCPU	(1 << 0)	/* CPU FAN Speed */
#define FSYS	(1 << 1)	/* System FAN Speed */
#define F2ND	(1 << 2)	/* Other FAN Speed */
#define FCPU2	(1 << 3)
#define FAUX2	(1 << 4)

/******************************************************************************
//	IIC Definition 
// ****************************************************************************/
#define SUSI_IIC_TYPE_PRIMARY				1
#define SUSI_IIC_TYPE_SMBUS					2
#define SUSI_IIC_TYPE_BOTH					3


/******************************************************************************
// WDT Definition
// ****************************************************************************/
#define WDTOUT0		0
#define WDTOUT1		1
#define WDTMAXNUM	2		/* WDT Max. Number, for count amount. */


/******************************************************************************
// SusiVCAvailable return value Definition
// ****************************************************************************/
#define VC_NONE_SUPPORT		    0   /* 0  - none available */
#define VC_BRIGHT_CTL_SUPPORT	1   /* 1  - Only Brightness control available */
#define VC_VGA_CTL_SUPPORT	    2   /* 2  - Only VGA control available */
#define VC_ALL_SUPPORT	        3   /* 3  - all available */


/******************************************************************************
// Linux Susi CPU Speedstep Mode Definition
// ****************************************************************************/
#define CORE_CPU_FULL_SPEED     0
#define CORE_CPU_LOW_SPEED      1
#define CORE_CPU_DYNAMIC        2


/******************************************************************************
//	SUSI APIs 
// ****************************************************************************
//-----------------------------------------------------------------------------
//	Driver Independent APIs  
//----------------------------------------------------------------------------*/
int SusiDllGetLastError(void);
void SusiDllGetVersion(DWORD *major, DWORD *minor);
BOOL SusiDllInit(void);
BOOL SusiDllUnInit(void);

/*-----------------------------------------------------------------------------
//	CORE
//----------------------------------------------------------------------------*/
int SusiCoreAvailable(void);
BOOL SusiCoreGetBIOSVersion(TCHAR *BIOSVersion, DWORD *size);
BOOL SusiCoreGetPlatformName(TCHAR *PlatformName, DWORD *size);
BOOL SusiCoreGetBIOSString(TCHAR *stringBIOS, DWORD *size);

BOOL SusiPlusSpeedIsActive(void);
int SusiPlusSpeedSetActive(void);
int SusiPlusSpeedSetInactive(void);
int SusiPlusSpeedRead(BYTE *CpuMode, BYTE *none);
int SusiPlusSpeedWrite(BYTE CpuMode, BYTE none);

BOOL SusiCoreEnableBootfail(void);
BOOL SusiCoreDisableBootfail(void);
BOOL SusiCoreRefreshBootfail(void);

/*-----------------------------------------------------------------------------
//	WDT
//----------------------------------------------------------------------------*/
int SusiWDAvailable(void);
BOOL SusiWDDisable(void);
BOOL SusiWDGetRange(DWORD *minimum, DWORD *maximum, DWORD *stepping);
BOOL SusiWDSetConfig(DWORD delay, DWORD timeout);
BOOL SusiWDTrigger(void);
BOOL SusiWDDisableEx(int group_number);
BOOL SusiWDSetConfigEx(int group_number, DWORD delay, DWORD timeout);
BOOL SusiWDTriggerEx(int group_number);

/*-----------------------------------------------------------------------------
//	GPIO
//----------------------------------------------------------------------------*/
int SusiIOAvailable(void);
BOOL SusiIOCountEx(DWORD *inCount, DWORD *outCount);
BOOL SusiIOQueryMask(DWORD flag, DWORD *Mask);
BOOL SusiIOReadEx(BYTE PinNum, BOOL *status);
BOOL SusiIOReadMultiEx(DWORD TargetPinMask, DWORD *StatusMask);
BOOL SusiIOSetDirection(BYTE PinNum, BYTE IO, DWORD *PinDirMask); 
BOOL SusiIOSetDirectionMulti(DWORD TargetPinMask, DWORD *PinDirMask);
BOOL SusiIOWriteEx(BYTE PinNum, BOOL status); 
BOOL SusiIOWriteMultiEx(DWORD TargetPinMask, DWORD StatusMask);

/*-----------------------------------------------------------------------------
//	SMBus
//----------------------------------------------------------------------------*/
int SusiSMBusAvailable(void);
BOOL SusiSMBusReadBlock(BYTE SlaveAddress, BYTE RegisterOffset, BYTE *Result, BYTE *ByteCount);
BOOL SusiSMBusI2CReadBlock(BYTE SlaveAddress, BYTE RegisterOffset, BYTE *Result, BYTE *ByteCount);
BOOL SusiSMBusReadByte(BYTE SlaveAddress, BYTE RegisterOffset, BYTE *Result);
BOOL SusiSMBusReadByteMulti(BYTE SlaveAddress, BYTE RegisterOffset, BYTE *Result, BYTE ByteCount);
BOOL SusiSMBusReadQuick(BYTE SlaveAddress);
BOOL SusiSMBusReadWord(BYTE SlaveAddress, BYTE RegisterOffset, WORD *Result);
BOOL SusiSMBusReceiveByte(BYTE SlaveAddress, BYTE *Result);
int SusiSMBusScanDevice(BYTE SlaveAddress_7);
BOOL SusiSMBusSendByte(BYTE SlaveAddress, BYTE Result);
BOOL SusiSMBusWriteBlock(BYTE SlaveAddress, BYTE RegisterOffset, BYTE *Result, BYTE ByteCount);
BOOL SusiSMBusI2CWriteBlock(BYTE SlaveAddress, BYTE RegisterOffset, BYTE *Result, BYTE ByteCount);
BOOL SusiSMBusWriteByte(BYTE SlaveAddress, BYTE RegisterOffset, BYTE Result);
BOOL SusiSMBusWriteByteMulti(BYTE SlaveAddress, BYTE RegisterOffset, BYTE *Result, BYTE ByteCount);
BOOL SusiSMBusWriteQuick(BYTE SlaveAddress);
BOOL SusiSMBusWriteWord(BYTE SlaveAddress, BYTE RegisterOffset, WORD Result);
BOOL SusiSMBusReset(void);

/*-----------------------------------------------------------------------------
//	IIC
//----------------------------------------------------------------------------*/
int SusiIICAvailable(void);
BOOL SusiIICRead(DWORD IICType, BYTE SlaveAddress, BYTE *ReadBuf, DWORD ReadLen);
BOOL SusiIICWrite(DWORD IICType, BYTE SlaveAddress, BYTE *WriteBuf, DWORD WriteLen);
BOOL SusiIICWriteReadCombine(DWORD IICType, BYTE SlaveAddress, BYTE *WriteBuf, DWORD WriteLen, BYTE *ReadBuf, DWORD ReadLen);

/*-----------------------------------------------------------------------------
//	VC
//----------------------------------------------------------------------------*/
int SusiVCAvailable(void);
BOOL SusiVCGetBright(BYTE *brightness);
BOOL SusiVCGetBrightRange(BYTE *minimum, BYTE *maximum, BYTE *stepping);
BOOL SusiVCScreenOff(void);
BOOL SusiVCScreenOn(void);
BOOL SusiVCSetBright(BYTE brightness);

/*-----------------------------------------------------------------------------
//	HWM
//----------------------------------------------------------------------------*/
int SusiHWMAvailable(void);
BOOL SusiHWMGetFanSpeed(WORD fanType, WORD *retval, WORD *typeSupport);
BOOL SusiHWMGetTemperature(WORD tempType, float *retval, WORD *typeSupport);
BOOL SusiHWMGetVoltage(DWORD voltType, float *retval, DWORD *typeSupport);
BOOL SusiHWMSetFanSpeed(WORD fanType, BYTE setval, WORD *typeSupport);

/*-----------------------------------------------------------------------------
//	PWS
//----------------------------------------------------------------------------*/
BOOL SusiPWRSetCPUFrequency(DWORD sel);

//*****************************************************************************
//	SUSI APIs (old)
//*****************************************************************************
//-----------------------------------------------------------------------------
//	Susi
//-----------------------------------------------------------------------------
void SusiGetVersion(WORD *major, WORD *minor);		// now use SusiDllGetVersion
BOOL SusiInit(void);										// now use SusiDllInit
BOOL SusiUnInit(void);									// now use SusiDllUnInit

//-----------------------------------------------------------------------------
//	Core
//-----------------------------------------------------------------------------
int SusiGetBIOSVersion(TCHAR *BIOSVersion, BYTE size);		// Old API for backward compatible, now use SusiCoreGetBIOSVersion!
int SusiGetPlatformName(TCHAR *PlatformName, BYTE size);		// Old API for backward compatible, now use SusiCoreGetPlatformName!

//-----------------------------------------------------------------------------
//	IIC
//-----------------------------------------------------------------------------
BOOL SusiIICReadByteMulti(BYTE SlaveAddress, BYTE *ReadBuf, DWORD ReadLen);		// Old API for backward compatible, now use SusiIICRead!
BOOL SusiIICWriteByteMulti(BYTE SlaveAddress, BYTE *WriteBuf, DWORD WriteLen);	// Old API for backward compatible, now use SusiIICWrite!  

//-----------------------------------------------------------------------------
//	GPIO
//-----------------------------------------------------------------------------
BOOL SusiIOInitial(DWORD statuses);
BOOL SusiIOCount(WORD *inCount, WORD *outCount); 
BOOL SusiIORead(BYTE pin, BOOL *status);
BOOL SusiIOReadMulti(DWORD pins, DWORD *statuses);
BOOL SusiIOWrite(BYTE pin, BOOL status);
BOOL SusiIOWriteMulti(DWORD pins, DWORD statuses);


//*****************************************************************************
//	AdvLib APIs 
//*****************************************************************************
//  Only for backward compatible with Adv Library,
//	new users of SUSI Library can simply ignore these
//-----------------------------------------------------------------------------
//	Driver Independent APIs
//-----------------------------------------------------------------------------
void AdvLibGetVersion(WORD *major, WORD *minor);
BOOL AdvLibInit(void);
BOOL AdvLibUnInit(void);

//-----------------------------------------------------------------------------
//	CORE
//-----------------------------------------------------------------------------
int AdvLibGetBIOSVersion(TCHAR *BIOSVersion, BYTE size);
int AdvLibGetPlatformName(TCHAR *PlatformName, BYTE size);

//-----------------------------------------------------------------------------
//	WDT
//-----------------------------------------------------------------------------
int AdvLibWDAvailable(void);
BOOL AdvLibWDDisable(void);
BOOL AdvLibWDGetRange(DWORD *minimum, DWORD *maximum, DWORD *stepping);
BOOL AdvLibWDSetConfig(DWORD delay, DWORD timeout);
BOOL AdvLibWDTrigger(void);

//-----------------------------------------------------------------------------
//	GPIO
//-----------------------------------------------------------------------------
BOOL AdvLibIOAvailable(void);
BOOL AdvLibIOCount(WORD *inCount, WORD *outCount);
BOOL AdvLibIOInitial(DWORD statuses);
BOOL AdvLibIORead(BYTE pin, BOOL *status);
BOOL AdvLibIOReadMulti(DWORD pins, DWORD *statuses);
BOOL AdvLibIOWrite(BYTE pin, BOOL status);
BOOL AdvLibIOWriteMulti(DWORD pins, DWORD statuses);

//-----------------------------------------------------------------------------
//	SMBus
//-----------------------------------------------------------------------------
BOOL AdvLibSMBusAvailable(void);
BOOL AdvLibSMBusReadByte(BYTE SlaveAddress, BYTE RegisterOffset, BYTE *Result);
BOOL AdvLibSMBusReadByteMulti(BYTE SlaveAddress,BYTE RegisterOffset, BYTE *Result, BYTE ByteCount);
BOOL AdvLibSMBusReadWord(BYTE SlaveAddress, BYTE RegisterOffset, WORD *Result);
BOOL AdvLibSMBusWriteByte(BYTE SlaveAddress, BYTE RegisterOffset, BYTE Result);
BOOL AdvLibSMBusWriteByteMulti(BYTE SlaveAddress,BYTE RegisterOffset, BYTE *Result, BYTE ByteCount);
BOOL AdvLibSMBusWriteWord(BYTE SlaveAddress, BYTE RegisterOffset, WORD Result);

//-----------------------------------------------------------------------------
//	VC
//-----------------------------------------------------------------------------
int AdvLibVCAvailable(void);
BOOL AdvLibVCGetBright(BYTE *brightness);
BOOL AdvLibVCGetBrightRange(BYTE *minimum, BYTE *maximum, BYTE *stepping);
BOOL AdvLibVCScreenOff(void);
BOOL AdvLibVCScreenOn(void);
BOOL AdvLibVCSetBright(BYTE brightness);

//-----------------------------------------------------------------------------
//	HWM
//-----------------------------------------------------------------------------
int AdvLibHWMAvailable(void);
BOOL AdvLibHWMGetFanSpeed(WORD fanType, WORD *retval, WORD *typeSupport);
BOOL AdvLibHWMGetTemperature(WORD tempType, float *retval, WORD *typeSupport);
BOOL AdvLibHWMGetVoltage(DWORD voltType, float *retval, DWORD *typeSupport);



#ifdef __cplusplus 
} 
#endif 

#endif /* _SUSI_H_ */
