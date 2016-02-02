// ConfigTest.cpp : Defines the entry point for the console application.
//
#include "platform.h"
#include "GPSMessageGenerate.h"
#include "DeviceMessageGenerate.h"
#include "IoTMessageGenerate.h"

//-------------------------Memory leak check define--------------------------------
#ifdef MEM_LEAK_CHECK
#include <crtdbg.h>
_CrtMemState memStateStart, memStateEnd, memStateDiff;
#endif
//---------------------------------------------------------------------------------

int main(int argc, char *argv[])
{
	int iRet = 0;
	
	//susiaccess_agent_conf_body_t config;
	susiaccess_agent_profile_body_t profile;

	char *buffer = NULL;

#ifdef MEM_LEAK_CHECK
	_CrtSetDbgFlag ( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF ); 
	//_CrtSetBreakAlloc(3719);
	_CrtMemCheckpoint( &memStateStart);
#endif

	/*
	memset(&config, 0 , sizeof(susiaccess_agent_conf_body_t));
	strcpy(config.runMode,"remote");
	strcpy(config.autoStart,"True");
	strcpy(config.serverIP,"172.22.12.12");
	strcpy(config.serverPort,"1883");
	strcpy(config.serverAuth,"F0PE1/aaU8o=");
	config.tlstype = tls_type_none;
	switch(config.tlstype)
	{
	case tls_type_none:
		break;
	case tls_type_tls:
		{
			strcpy(config.cafile, "ca.crt");
			strcpy(config.capath, "");
			strcpy(config.certfile, "server.crt");
			strcpy(config.keyfile, "server.key");
			strcpy(config.cerpasswd, "123456");
		}
		break;
	case tls_type_psk:
		{
			strcpy(config.psk, "");
			strcpy(config.identity, "SAClientSample");
			strcpy(config.ciphers, "");
		}
		break;
	}
*/
	memset(&profile, 0 , sizeof(susiaccess_agent_profile_body_t));
	sprintf_s(profile.version, DEF_VERSION_LENGTH, "%d.%d.%d.%d", 3, 1, 0, 0);
	strcpy(profile.hostname,"SAClientSample");
	strcpy(profile.devId,"000014DAE996BE04");
	strcpy(profile.sn,"14DAE996BE04");
	strcpy(profile.mac,"14DAE996BE04");
	strcpy(profile.type,"IPC");
	strcpy(profile.product,"Sample Agent");
	strcpy(profile.manufacture,"test");
	strcpy(profile.osversion,"NA");
	strcpy(profile.biosversion,"NA");
	strcpy(profile.platformname,"NA");
	strcpy(profile.processorname,"NA");
	strcpy(profile.osarchitect,"NA");
	profile.totalmemsize = 40832;
	strcpy(profile.maclist,"14DAE996BE04");
	strcpy(profile.localip,"172.22.12.21");
	strcpy(profile.account,"anonymous");
	strcpy(profile.passwd,"");
	strcpy(profile.workdir, "");
	
	/*GPS Test*/
	{
		MSG_CLASSIFY_T *dev = NULL;
		MSG_CLASSIFY_T *gps = NULL;
		gps = GPS_CreateGPS();
		dev = GPS_AddDevice(gps, "/dev/0", "1.0");
		GPS_SetLatitudeAttribute(dev, 0, 180, -180, NULL);
		GPS_SetLongitudeAttribute(dev, 0, 90, -90, NULL);
		GPS_SetCustomFloatAttribute(dev, "Skey", "xdop", "r", 2.53, NULL);

		buffer = GPS_PrintUnformatted(gps);
		printf("GPS Format:\r\n%s\r\n", buffer);
		free(buffer);
		GPS_ReleaseGPS(gps);
	}
	printf("Click enter to next");
	fgetc(stdin);

	/*AgentInfo Test*/
	{
		MSG_CLASSIFY_T *myDev = NULL;
		myDev = DEV_CreateAgentInfo(&profile);

		buffer = DEV_PrintUnformatted(myDev);
		printf("AgentInfo Format:\r\n%s\r\n", buffer);
		free(buffer);
		DEV_ReleaseDevice(myDev);
		

		buffer = DEV_GetAgentInfoTopic(profile.devId);
		printf("AgentInfo topic:\r\n%s\r\n", buffer);
		free(buffer);
	}
	printf("Click enter to next");
	fgetc(stdin);

	/*WillMessage Test*/
	{
		MSG_CLASSIFY_T *myDev = NULL;
		myDev = DEV_CreateWillMessage(&profile);

		buffer = DEV_PrintUnformatted(myDev);
		printf("Will Message Format:\r\n%s\r\n", buffer);
		free(buffer);
		DEV_ReleaseDevice(myDev);
		
		buffer = DEV_GetWillMessageTopic(profile.devId);
		printf("Will Message topic:\r\n%s\r\n", buffer);
		free(buffer);
	}
	printf("Click enter to next");
	fgetc(stdin);

	/*OSInfo Test*/
	{
		MSG_CLASSIFY_T *myDev = NULL;		
		myDev = DEV_CreateOSInfo(&profile);

		buffer = DEV_PrintUnformatted(myDev);
		printf("OSInfo Format:\r\n%s\r\n", buffer);
		free(buffer);
		DEV_ReleaseDevice(myDev);
		

		buffer = DEV_GetOSInfoTopic(profile.devId);
		printf("OSInfo topic:\r\n%s\r\n", buffer);
		free(buffer);
	}
	printf("Click enter to next");
	fgetc(stdin);

	/*HandlerList Test*/
	{
		MSG_CLASSIFY_T *myDev = NULL;
		char* list[] = {"handler1","handler2","handler3"};
		myDev = DEV_CreateHandlerList(profile.devId, list, 3);

		buffer = DEV_PrintUnformatted(myDev);
		printf("HandlerList Format:\r\n%s\r\n", buffer);
		free(buffer);
		DEV_ReleaseDevice(myDev);
		

		buffer = DEV_GetHandlerListTopic(profile.devId);
		printf("HandlerList topic:\r\n%s\r\n", buffer);
		free(buffer);
	}

	printf("Click enter to next");
	fgetc(stdin);

	/*IoT Test*/
	{
		MSG_CLASSIFY_T *myCapability = IoT_CreateRoot("Test");

		MSG_CLASSIFY_T *myVoltGroup = IoT_AddGroup(myCapability, "Voltage");
		MSG_CLASSIFY_T *myTempGroup = IoT_AddGroup(myCapability, "Temperature");
		MSG_ATTRIBUTE_T *attrVolt1 = NULL, *attrVolt2 = NULL, *attrTemp1 = NULL, *attrTemp2 = NULL;

		MSG_ATTRIBUTE_T* attr = IoT_AddGroupAttribute(myVoltGroup, "bu");
		if(attr)
			IoT_SetStringValue(attr, "V", IoT_READONLY);
		attr = IoT_AddGroupAttribute(myTempGroup, "bu");
		if(attr)
			IoT_SetStringValue(attr, "Celsius", IoT_READONLY);
		attrVolt1 = IoT_AddSensorNode(myVoltGroup, "5V");
		if(attrVolt1)
			IoT_SetFloatValueWithMaxMin(attrVolt1, 4.9, IoT_READONLY, 5.5, 4.5, "V");
		attrVolt2 = IoT_AddSensorNode(myVoltGroup, "12V");
		if(attrVolt2)
			IoT_SetFloatValueWithMaxMin(attrVolt2, 11, IoT_READONLY, 10, 14, "V");
		attrTemp1 = IoT_AddSensorNode(myTempGroup, "CPU");
		if(attrTemp1)
			IoT_SetFloatValue(attrTemp1, 65, IoT_READONLY, "Celsius");
		attrTemp2 = IoT_AddSensorNode(myTempGroup, "SYS");
		if(attrTemp2)
			IoT_SetFloatValue(attrTemp2, 50, IoT_READONLY, "Celsius");

		buffer = IoT_PrintCapability(myCapability);
		printf("IoT Format:\r\n%s\r\n", buffer);
		free(buffer);

		printf("Click enter to next");
		fgetc(stdin);

		if(attrVolt2)
			IoT_SetFloatValue(attrVolt2, 12, IoT_READONLY, "V");
		if(attrTemp1)
			IoT_SetFloatValueWithMaxMin(attrTemp1, 80, IoT_READONLY, 100, 40, "Celsius");

		buffer = IoT_PrintCapability(myCapability);
		printf("IoT Format2:\r\n%s\r\n", buffer);
		free(buffer);

		printf("Click enter to next");
		fgetc(stdin);

		if(myVoltGroup)
			IoT_DelSensorNode(myVoltGroup, "SYS"); 
		if(myTempGroup)
			IoT_DelSensorNode(myTempGroup, "SYS"); 

		buffer = IoT_PrintCapability(myCapability);
		printf("IoT Format3:\r\n%s\r\n", buffer);
		free(buffer);

		printf("Click enter to next");
		fgetc(stdin);

		buffer = IoT_PrintData(myCapability);
		printf("IoT Data:\r\n%s\r\n", buffer);
		free(buffer);

		IoT_ReleaseAll(myCapability);
		
	}

	printf("Click enter to next");
	fgetc(stdin);

	/*EventNotify Test*/
	{
		MSG_CLASSIFY_T *myEvent = NULL;
		myEvent = DEV_CreateEventNotify("MsgGen", "My MsgGenTest");

		buffer = DEV_PrintUnformatted(myEvent);
		printf("EventNotify Format:\r\n%s\r\n", buffer);
		free(buffer);
		DEV_ReleaseDevice(myEvent);	
	}
	
	printf("Click enter to next");
	fgetc(stdin);
	/*Full EventNotify Test*/
	{
		MSG_CLASSIFY_T *myEvent = NULL;
		myEvent = DEV_CreateFullEventNotify(profile.devId, 2, "MsgGen", "MsgGen", "My MsgGenTest");

		buffer = DEV_PrintUnformatted(myEvent);
		printf("EventNotify Format2:\r\n%s\r\n", buffer);
		free(buffer);
		DEV_ReleaseDevice(myEvent);	

		buffer = DEV_GetEventNotifyTopic(profile.devId);
		printf("EventNotify topic:\r\n%s\r\n", buffer);
		free(buffer);
	}


	printf("Click enter to next");
	fgetc(stdin);
	/*Get/Set Sensor Data Test*/
	{
		MSG_ATTRIBUTE_T *attr = NULL;
		MSG_CLASSIFY_T *pInfoList = NULL,  *pEArray = NULL, *pSensor = NULL;
		MSG_CLASSIFY_T *pRoot = MSG_CreateRoot();

		if(pRoot)
		{
			attr = MSG_AddJSONAttribute(pRoot, "sessionID");
			if(attr)
				MSG_SetStringValue(attr, "1234", NULL); 

			pInfoList = MSG_AddJSONClassify(pRoot, "sensorInfoList", NULL, false);

			if(pInfoList)
				pEArray = MSG_AddJSONClassify(pInfoList, "e", NULL, true);

			pSensor = MSG_AddJSONClassify(pEArray, "sensor", NULL, false);
			if(pSensor)
			{
				attr = MSG_AddJSONAttribute(pSensor, "n");
				if(attr)
					MSG_SetStringValue(attr, "SUSIControl/Hardware Monitor/Voltage/CPU", NULL); 

				attr = MSG_AddJSONAttribute(pSensor, "v");
				if(attr)
					MSG_SetDoubleValue(attr, 10, NULL, NULL); 

				attr = MSG_AddJSONAttribute(pSensor, "statusCode");
				if(attr)
					MSG_SetDoubleValue(attr, 220, NULL, NULL); 
			}

			pSensor = MSG_AddJSONClassify(pEArray, "sensor", NULL, false);
			if(pSensor)
			{
				attr = MSG_AddJSONAttribute(pSensor, "n");
				if(attr)
					MSG_SetStringValue(attr, "SUSIControl/Hardware Monitor/Temperature/CPU", NULL); 

				attr = MSG_AddJSONAttribute(pSensor, "v");
				if(attr)
					MSG_SetDoubleValue(attr, 10, NULL, NULL); 

				attr = MSG_AddJSONAttribute(pSensor, "statusCode");
				if(attr)
					MSG_SetDoubleValue(attr, 220, NULL, NULL); 
			}	

		}

		buffer = MSG_PrintUnformatted(pRoot);
		printf("Get/Set Sensor Data:\r\n%s\r\n", buffer);
		free(buffer);
		MSG_ReleaseRoot(pRoot);
	}

	
	printf("Click enter to exit");
	fgetc(stdin);

#ifdef MEM_LEAK_CHECK
	_CrtMemCheckpoint( &memStateEnd );
	if ( _CrtMemDifference( &memStateDiff, &memStateStart, &memStateEnd) )
		_CrtMemDumpStatistics( &memStateDiff );
#endif
	return iRet;
}