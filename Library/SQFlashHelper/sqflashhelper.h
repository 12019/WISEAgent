
#ifndef _SQFLASH_HELPER_H_
#define _SQFLASH_HELPER_H_

#ifdef WIN32
#include <SQFlash.h>
#else

typedef struct _ATA_SMART_ATTR_TABLE
{
   unsigned int dwMaxProgram;
   unsigned int dwAverageProgram;
   unsigned int dwEnduranceCheck;
   unsigned int dwPowerOnTime;
   unsigned int dwEccCount;
   unsigned int dwMaxReservedBlock;
   unsigned int dwCurrentReservedBlock;
   unsigned int dwGoodBlockRate;
} ATA_SMART_ATTR_TABLE, *PATA_SMART_ATTR_TABLE;

#endif
#define INVALID_DEVMON_VALUE  (-999)

typedef enum{
	HddTypeUnknown = 0,
	SQFlash,
	StdDisk,
}hdd_type_t;

typedef enum{
	SmartAttriTypeUnknown       = 0x00,
	ReadErrorRate               = 0x01,
	ThroughputPerformance       = 0x02,
	SpinUpTime                  = 0x03,
	StartStopCount              = 0x04,
	ReallocatedSectorsCount     = 0x05,
	ReadChannelMargin           = 0x06,
	SeekErrorRate               = 0x07,
	SeekTimePerformance         = 0x08,
	PowerOnHoursPOH             = 0x09,
	SpinRetryCount              = 0x0A,
	CalibrationRetryCount       = 0x0B,
	PowerCycleCount             = 0x0C,
	SoftReadErrorRate           = 0x0D,
	SQAverageProgram            = 0xAD,       //SQ Flash
	SATADownshiftErrorCount     = 0xB7,
	EndtoEnderror               = 0xB8,
	HeadStability               = 0xB9,
	InducedOpVibrationDetection = 0xBA,
	ReportedUncorrectableErrors = 0xBB,
	CommandTimeout              = 0xBC,
	HighFlyWrites               = 0xBD,
	AirflowTemperatureWDC       = 0xBE,
//TemperatureDifferencefrom100  = 0xBE,
	GSenseErrorRate             = 0xBF,
	PoweroffRetractCount        = 0xC0,
	LoadCycleCount              = 0xC1,
	Temperature                 = 0xC2,
	HardwareECCRecovered        = 0xC3,
	ReallocationEventCount      = 0xC4,
	CurrentPendingSectorCount   = 0xC5,
	UncorrectableSectorCount    = 0xC6,
	UltraDMACRCErrorCount       = 0xC7,
	MultiZoneErrorRate          = 0xC8,
//WriteErrorRateFujitsu         = 0xC8,
	OffTrackSoftReadErrorRate   = 0xC9,
	DataAddressMarkerrors       = 0xCA,
	RunOutCancel                = 0xCB,
	SoftECCCorrection           = 0xCC,
	ThermalAsperityRateTAR      = 0xCD,
	FlyingHeight                = 0xCE,
	SpinHighCurrent             = 0xCF,
	SpinBuzz                    = 0xD0,
	OfflineSeekPerformance      = 0xD1,
	VibrationDuringWrite        = 0xD3,
	ShockDuringWrite            = 0xD4,
	DiskShift                   = 0xDC,
	GSenseErrorRateAlt          = 0xDD,
	LoadedHours                 = 0xDE,
	LoadUnloadRetryCount        = 0xDF,
	LoadFriction                = 0xE0,
	LoadUnloadCycleCount        = 0xE1,
	LoadInTime                  = 0xE2,
	TorqueAmplificationCount    = 0xE3,
	PowerOffRetractCycle        = 0xE4,
	GMRHeadAmplitude            = 0xE6,
	DriveTemperature            = 0xE7,
	SQEnduranceRemainLife       = 0xE8, //SQ Flash
	SQPowerOnTime               = 0xE9, //SQ Flash
	SQECCLog                    = 0xEA, //SQ Flash
	SQGoodBlockRate             = 0xEB, //SQ Flash
	HeadFlyingHours             = 0xF0,
//TransferErrorRateFujitsu      = 0xF0,
	TotalLBAsWritten            = 0xF1,
	TotalLBAsRead               = 0xF2,
	ReadErrorRetryRate          = 0xFA,
	FreeFallProtection          = 0xFE,
}smart_attri_type_t;


typedef struct{
	smart_attri_type_t attriType;
	char attriName[64];
	int attriFlags;
	int attriWorst;
	int attriThresh;
	int attriValue;
	char attriVendorData[6];
}smart_attri_info_t;


typedef struct smart_attri_info_node_t{
	smart_attri_info_t attriInfo;
	struct smart_attri_info_node_t * next;
}smart_attri_info_node_t;

typedef smart_attri_info_node_t * smart_attri_info_list;


typedef struct{
	hdd_type_t hdd_type;
	char hdd_name[128];
	int hdd_index; 
	unsigned int max_program;
	unsigned int average_program;
	unsigned int endurance_check;
	unsigned int power_on_time;
	unsigned int ecc_count;
	unsigned int max_reserved_block;
	unsigned int current_reserved_block;
	unsigned int good_block_rate;
	unsigned int hdd_health_percent;
	int hdd_temp;
	smart_attri_info_list smartAttriInfoList;
}hdd_mon_info_t;

typedef struct hdd_mon_info_node_t{
	hdd_mon_info_t hddMonInfo;
	struct hdd_mon_info_node_t * next;
}hdd_mon_info_node_t;

typedef hdd_mon_info_node_t * hdd_mon_info_list;

typedef struct{
	int hddCount;
	hdd_mon_info_list hddMonInfoList;
}hdd_info_t;

#ifdef __cplusplus
extern "C" {
#endif

	bool hdd_IsExistSQFlashLib();

	bool hdd_StartupSQFlashLib();

	bool hdd_CleanupSQFlashLib();

	hdd_mon_info_list hdd_CreateHDDInfoList();

	void hdd_DestroyHDDInfoList(hdd_mon_info_list hddInfoList);

	bool hdd_GetHDDInfo(hdd_info_t * pHDDInfo);

#ifdef __cplusplus
}
#endif

#endif /* _SQFLASH_HELPER_H_ */