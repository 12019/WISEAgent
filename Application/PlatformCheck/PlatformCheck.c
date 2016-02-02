// PlatformCheck.cpp : Defines the entry point for the console application.
//
#include <stdio.h>
#include <advcarehelper.h>
#include <hwmhelper.h>

#define DEF_OS_NAME_LEN           34
#define DEF_OS_VERSION_LEN        64
#define DEF_BIOS_VERSION_LEN      256
#define DEF_PLATFORM_NAME_LEN     128
#define DEF_PROCESSOR_NAME_LEN    64

int main(int argc, char *argv[])
{
	if(advcare_StartupAdvCareLib())
	{
		char platform[DEF_PLATFORM_NAME_LEN] = {0};
		char bios[DEF_BIOS_VERSION_LEN] = {0};
		if(advcare_GetPlatformName(platform, DEF_PLATFORM_NAME_LEN))
		{
			printf("<AdvCare> Platform Name: %s\r\n", platform);
		}
		else
		{
			printf("<AdvCare> Cannot access Platform Name\r\n");
		}
		if(advcare_GetBIOSVersion(bios, DEF_BIOS_VERSION_LEN))
		{
			printf("<AdvCare> BIOS Version: %s\r\n", bios);
		}
		else
		{
			printf("<AdvCare> Cannot access BIOS Version\r\n");
		}
		advcare_CleanupAdvCareLib();
	}
	else
	{
		printf("Load AdvCare Failed\r\n");
	}

	if(hwm_StartupSUSILib() == true)
	{
		char platform[DEF_PLATFORM_NAME_LEN] = {0};
		char bios[DEF_BIOS_VERSION_LEN] = {0};
		if(hwm_GetPlatformName(platform, DEF_PLATFORM_NAME_LEN) == true)
		{
			printf("<SUSI> Platform Name: %s\r\n", platform);
		}
		else
		{
			printf("<SUSI> Cannot access Platform Name\r\n");
		}
		if(hwm_GetBIOSVersion(bios, DEF_BIOS_VERSION_LEN) == true)
		{
			printf("<SUSI> BIOS Version: %s\r\n", bios);
		}
		else
		{
			printf("<SUSI> Cannot access BIOS Version\r\n");
		}
		//hwm_CleanupSUSILib();
	}
	else
	{
		printf("Load SUSI Failed\r\n");
	}

	printf("Click enter to exit");
	fgetc(stdin);
	return 0;
}

