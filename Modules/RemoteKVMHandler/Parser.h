#ifndef _REMOTE_KVM_PARSER_H_
#define _REMOTE_KVM_PARSER_H_

#include "platform.h"
#include "cJSON.h"
#include "RemoteKVMHandler.h"
#include "kvmconfig.h"
//------------------------KVM--------------------------------------
#define KVM_VNC_CONNECT_PARAMS                    "vncConnectParams"
#define KVM_VNC_SERVER_IP                         "vncServerIP"
#define KVM_VNC_SERVER_PORT                       "vncServerPort"
#define KVM_VNC_SERVER_PWD                        "vncServerPassword"
#define KVM_VNC_SERVER_STATUS                     "vncServerStatus"
#define KVM_VNC_SERVER_START_MODE                 "vncServerStartMode"
#define KVM_VNC_SERVER_START_LISTEN_HOST          "vncServerStartListenHost"
#define KVM_VNC_SERVER_START_LISTEN_PORT          "vncServerStartListenPort"
//#define KVM_VNC_SERVER_START_REPEATER_HOST      "vncServerStartRepeaterHost"
#define KVM_VNC_SERVER_START_REPEATER_PORT        "vncServerStartRepeaterPort"
#define KVM_VNC_SERVER_START_REPEATER_ID          "vncServerStartRepeaterID"
#define KVM_VNC_SERVER_NEEDCHANGEPASS             "vncServerStartNeedChangePassword"
#define KVM_ERROR_REP                             "errorRep"
#define KVM_VNC_MODE_INFO                         "vncModeInfo"
#define KVM_VNC_MODE                              "vncMode"
#define KVM_VNC_PORT                              "custvncPort"
#define KVM_VNC_PWD                               "custvncPwd"

#define AGENTINFO_BODY_STRUCT			              "susiCommData"
#define AGENTINFO_REQID					              "requestID"
#define AGENTINFO_CMDTYPE				              "commCmd"

#define KVM_INFOMATION                            "Information"
#define KVM_E_FLAG                                "e"
#define KVM_N_FLAG                                "n"
#define KVM_BN_FLAG                               "bn"
#define KVM_V_FLAG                                "v"
#define KVM_SV_FLAG                               "sv"
#define KVM_FUNCTION_LIST                         "functionList"
#define KVM_FUNCTION_CODE                         "functionCode"
#define KVM_NS_DATA                               "nonSensorData"

bool ParseReceivedData(void* data, int datalen, int * cmdID);
bool ParseKVMRecvCmd(void* data, kvm_vnc_server_start_params * kvmParms);
int Parser_PackKVMGetConnectParamsRep(kvm_vnc_connect_params *pConnectParams, char** outputStr);
int Parser_PackKVMErrorRep(char * errorStr, char ** outputStr);
int Parser_PackVNCModeParamsRep(susiaccess_kvm_conf_body_t *pConnectParams, char** outputStr);

int Parser_PackCpbInfo(kvm_capability_info_t * cpbInfo, char **outputStr);
int Parser_PackSpecInfoRep(char * cpbStr, char * handlerName, char ** outputStr);

#endif
