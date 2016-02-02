#ifndef   _TERMINAL_HANDLER_H_
#define   _TERMINAL_HANDLER_H_ 

#include  "common.h"

#define  cagent_request_terminal 17
#define  cagent_reply_terminal 111

typedef enum{
	unknown_cmd = 0,
	//-------------------
	terminal_session_cmd_run_req = 301,
	terminal_session_cmd_run_rep = 302,  
	terminal_session_stop_req = 305,
	terminal_session_stop_rep =306,
	terminal_session_ret_req = 307,     
	terminal_session_ret_rep = 308,
	terminal_error_rep = 320,

	terminal_get_capability_req = 521,
	terminal_get_capability_rep = 522,
}susi_comm_cmd_t;

#define DEF_INVALID_SESID      (-1) 

#define TM_HANDLE          CAGENT_HANDLE

typedef struct tmn_session_context_t{
	int sesID;
	BOOL hasNewCmdStr;
	BOOL isReady;
	BOOL isStop;
	BOOL isValid;
	int synWRFlag;
	char * cmdStr;
	CAGENT_MUTEX_TYPE sesMutex;
	TM_HANDLE  sesPrcHandle;
	TM_HANDLE  cmdInputPipeReadHandle;
	TM_HANDLE  cmdInputPipeWriteHandle;
	TM_HANDLE  errorWriteHandle;
	TM_HANDLE  retOutputPipeReadHandle;
	TM_HANDLE  retOutputPipeWriteHandle;
	BOOL isSesWriteThreadRunning;
	CAGENT_THREAD_HANDLE sesWriteThreadHandle;
	BOOL isSesReadThreadRunning;
	CAGENT_THREAD_HANDLE sesReadThreadHandle;
}tmn_session_context_t;

typedef struct tmn_session_context_node_t{
	tmn_session_context_t sesContext;
	struct tmn_session_context_node_t * next;
}tmn_session_context_node_t;

typedef tmn_session_context_node_t * tmn_session_context_list;

#define TMN_NONE_FUNC_FLAG       0x00          //None
#define TMN_INTERNAL_FUNC_FLAG   0x01          //Internal
#define TMN_SSH_FUNC_FLAG        0x02          //SSH

#define TMN_NONE_FUNC_STR        "disable"
#define TMN_INTERNAL_FUNC_STR    "internal" 
#define TMN_SSH_FUNC_STR         "ssh"


typedef struct tmn_capability_info_t{
	char sshId[256];
	char sshPwd[256];
	char funcsStr[256];
   unsigned int funcsCode;
}tmn_capability_info_t;

void Handler_Uninitialize();

#endif
