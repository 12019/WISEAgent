#ifndef _CAGENT_KVM_CONFIG_H_
#define _CAGENT_KVM_CONFIG_H_
#include "susiaccess_def.h"

#define DEF_CONFIG_FILE_NAME	"agent_config.xml"

#define DEF_NORMAL_KVM_MODE			1
#define DEF_LISTEN_KVM_MODE			2
#define DEF_REPEATER_KVM_MODE		3

typedef struct {
	/*Custom KVM*/
	char kvmMode[DEF_KVM_MODE_LENGTH];
	char custVNCPwd[DEF_USER_PASS_LENGTH];
	char custVNCPort[DEF_PORT_LENGTH];
}susiaccess_kvm_conf_body_t;

#ifdef __cplusplus
extern "C" {
#endif

int kvm_load(char const * configFile, susiaccess_kvm_conf_body_t * conf);
int kvm_save(char const * configFile, susiaccess_kvm_conf_body_t const * const conf);
int kvm_create(char const * configFile, susiaccess_kvm_conf_body_t * conf);
int kvm_get(char const * const configFile, char const * const itemName, char * itemValue, int valueLen);
int kvm_set(char const * const configFile, char const * const itemName, char const * const itemValue);

#ifdef __cplusplus
}
#endif

#endif