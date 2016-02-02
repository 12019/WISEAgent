#ifndef _REMOTE_KVM_HANDLER_H_
#define _REMOTE_KVM_HANDLER_H_

#include "cJSON.h"

#ifndef BOOL
typedef int BOOL;
#define TRUE   1
#define FALSE  0
#endif

#define cagent_request_remote_kvm 12
#define cagent_reply_remote_kvm 105

//--------------------------Remote KVM data define------------------------------
typedef struct{
	char vnc_server_ip[16];
	unsigned int vnc_server_port;
	char vnc_password[9];
}kvm_vnc_connect_params;

typedef enum{
	cagent_callback_abort = -1,
	cagent_callback_continue   
}cagent_callback_status_t;

typedef struct{   
	char vnc_server_start_listen_ip[16];
	unsigned int vnc_server_listen_port;
	//char vnc_server_start_repeater_ip[16];
	unsigned int vnc_server_repeater_port;
	unsigned int vnc_server_repeater_id;
	unsigned int mode;
	unsigned int need_change_password;
}kvm_vnc_server_start_params;

typedef struct{
	char *vnc_mode;  
	char *custvnc_pwd;
	unsigned int custvnc_port;
}kvm_vnc_mode_params;

typedef struct{
	kvm_vnc_connect_params  vncConnectParams;
	kvm_vnc_mode_params  vncModeParams;
	BOOL isVNCServerRunning;
}remote_kvm_handler_context_t;

remote_kvm_handler_context_t    RemoteKVMHandlerContext;

typedef enum{
	unknown_cmd = 0,
	//--------------------------Remote KVM command define(131--150)----------------------------
	kvm_get_vnc_pwd_req = 131,
	kvm_get_vnc_pwd_rep,
	kvm_get_ip_req,
	kvm_get_ip_rep,
	kvm_get_port_req,
	kvm_get_port_rep,
	kvm_get_vnc_server_status_req,
	kvm_get_vnc_server_status_rep,
	kvm_get_connect_params_req,
	kvm_get_connect_params_rep,
	kvm_get_vnc_mode_req,
	kvm_get_vnc_mode_rep,
	kvm_start_vnc_server_req,
	kvm_start_vnc_server_rep,
	kvm_error_rep = 150,

	kvm_get_capability_req = 521,
	kvm_get_capability_rep = 522,
}susi_comm_cmd_t;

typedef enum EVNCServerStatus{
	VSS_UNKNOWN,
	VSS_RUNNING,
	VSS_STOPED,
	VSS_NOT_EXIST,
}EVNCSERVERSTATUS;

#define KVM_NONE_FUNC_FLAG       0x00          //None
#define KVM_DEFAULT_FUNC_FLAG    0x01          //Default
#define KVM_REPEATER_FUNC_FLAG   0x02          //Repeater
#define KVM_CUSTOM_FUNC_FLAG     0x04          //Custom
#define KVM_DEFAULT_MODE_STR     "default"
#define KVM_REPEATER_FUNC_STR    "repeater" 
#define KVM_CUSTOM_MODE_STR      "customvnc"
#define KVM_DISABLE_MODE_STR     "disable"

typedef struct kvm_capability_info_t{
	char vncMode[64];
	unsigned int vncPort;
	char vncPwd[256];
	char funcsStr[256];
	unsigned int funcsCode;
}kvm_capability_info_t;

#endif