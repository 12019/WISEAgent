#ifndef _CAGENT_SOFTWARE_CONFIG_H_
#define _CAGENT_SOFTWARE_CONFIG_H_
#include "susiaccess_def.h"

#define DEF_CONFIG_FILE_NAME	"agent_config.xml"

#define CFG_PROCESS_GATHER_LEVEL             "ProcessGatherLevel"
#define CFG_SYSTEM_LOGON_USER                "SystemLogonUser"

typedef struct {
	/*Custom KVM*/
	char kvmMode[DEF_KVM_MODE_LENGTH];
	char custVNCPwd[DEF_USER_PASS_LENGTH];
	char custVNCPort[DEF_PORT_LENGTH];
}susiaccess_kvm_conf_body_t;

#ifdef __cplusplus
extern "C" {
#endif

int proc_config_load(char const * configFile, susiaccess_kvm_conf_body_t * conf);
int proc_config_save(char const * configFile, susiaccess_kvm_conf_body_t const * const conf);
int proc_config_create(char const * configFile, susiaccess_kvm_conf_body_t * conf);
int proc_config_get(char const * const configFile, char const * const itemName, char * itemValue, int valueLen);
int proc_config_set(char const * const configFile, char const * const itemName, char const * const itemValue);

#ifdef __cplusplus
}
#endif

#endif