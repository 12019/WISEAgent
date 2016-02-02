#ifndef _SERVER_REDUNDANCY_HANDLER_H_
#define _SERVER_REDUNDANCY_HANDLER_H_

#include "susiaccess_handler_ex_api.h"

#define cagent_request_server_redundancy  21
#define cagent_reply_server_redundancy   115

#define DEF_CONFIG_FILE_NAME     "agent_config.xml"

typedef enum
{
	unknown_cmd = 0,
	server_redundancy_control_req = 125,
}susi_comm_cmd_t;

typedef enum{
	SERVER_UNDEFINED = -1,
	SERVER_LOST_CONNECTION = 0,
	SERVER_AUTH_SUCCESS = 1,
	SERVER_AUTH_FAILED,
	SERVER_CONNECTION_FULL,
	SERVER_RECONNECT,
	SERVER_CONNECT_TO_MASTER,
	SERVER_CONNECT_TO_SEPCIFIC,
	SERVER_PRESERVED_MESSAGE,
	SERVER_SET_SERVER_IP_LIST,
}server_status_code_t;

#endif