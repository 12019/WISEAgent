/************************************
FileName        : Checkusb.c
Discription     : Check USB Device
Data            : 10/25/2012
Author          : Reyan.xin
mail            : jinlong.xin@advantech.com.cn
************************************/

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "Checkusb.h"

char UsbDisk[6][9];
int UsbDiskNum = 0;

static void GetAllUsbDisk(void)
 {
	 FILE* fp = popen("find /dev/disk/by-path/ -type l -iname \\*usb\\*part1\\* -print0|xargs -0 -iD readlink -f D", "r");
	 char line[256] = {0x0};
	 char *Usbdisk;

	 while(fgets(line, sizeof(line), fp) != NULL) 
	 {
		 Usbdisk = strndup(line, 8);
		 strcpy(UsbDisk[UsbDiskNum++], Usbdisk);
	 }
	 pclose(fp);
 }

int CheckUsbDisk(char *device)
{
	int i = 0;

	GetAllUsbDisk();

	for(i = 0; i < UsbDiskNum; i++)
	{
		if(0 == strcmp(device, UsbDisk[i]))
		{
			return 0;
		}
	}

	return 1;
}
