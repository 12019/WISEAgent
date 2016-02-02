/****************************************************************************/
/* Copyright(C) : Advantech Technologies, Inc.														 */
/* Create Date  : 2015/02/05 by Eric Liang															     */
/* Modified Date: 2015/02/05 by Eric Liang															 */
/* Abstract       :  IoT GW Define                 													             */
/* Reference    : None																									 */
/****************************************************************************/
#ifndef  __ADV_IOTGW_DEF_H__
#define __ADV_IOTGW_DEF_H__


#define IOTGW_SIML_INI  "./Simulator.ini"
#define IOTGW_SIML_FOLDER "./Simulator/"

#define MAX_DATA_SIZE  4096
//#define MAX_FUNSET_DATA_SIZE 512

// IoTGW_InfoSpec
#define AppName_IoTGW     "IoTGW"
#define Key_ConNum			    "ConTypeNum"
#define Key_ConType			    "ConType%d"
#define Key_InterfaceNum    "Interfaces"
#define Key_InterfaceName  "InterfaceName%d"
#define Key_InterfaceID         "InterfaceID%d"
#define Key_Version               "Version"
#define Key_SenNodeNum   "SenNodeNum"
#define Key_IoTGWInterfaces "Interfaces"
#define Key_InterfaceSimulator "Simulator%d"

// < IoTGW JSON Format>
#define AppName_Format      "Format.%s"
#define Key_IoTGW_Format    "IoTGW.Format"
#define Key_ConType_Format "ConType.Format"
#define Key_Interface_Format   "Interface.Format"


// SenNode InfoSpec
#define AppName_SenNode     "SenNode"
#define Key_SenNum                  "SenNum"
#define Key_SenInfo                    "SenInfo%d"
#define Key_SenNodeName      "SenNodeName"
#define Key_SenNodeID             "SenNodeID"
#define Key_SenNodeConType "SenNodeConType"


//Simulator
#define AppName_Simulator     "Simulator"
#define Key_DemoCount            "TotalNum"
#define Key_Data                          "Data%d"
#define Key_Wait                          "Sleep%d"


// < SenNode JSON Format>
#define Key_SenNode_Format						"SenNode.Format"
#define Key_SenData_Format						"SenData.Format"
#define Key_SenNode_Info_Format			   "Info.Format"




#endif // __ADV_IOTGW_DEF_H__


