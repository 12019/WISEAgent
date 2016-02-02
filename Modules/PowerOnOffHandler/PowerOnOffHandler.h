#ifndef   _POWER_ONOFF_HANDLER_H_
#define   _POWER_ONOFF_HANDLER_H_

#define cagent_request_power_onoff  11
#define cagent_reply_power_onoff    104

#define OUT

//--------------------------Power On Off command define(71--100)---------------------------
typedef enum
{
unknown_cmd = 0,
power_mac_list_req = 71,
power_mac_list_rep,
power_last_bootup_time_req,
power_last_bootup_time_rep,
power_bootup_period_req,
power_bootup_period_rep,
power_off_req,
power_off_rep,
power_restart_req,
power_restart_rep,
power_suspend_req,
power_suspend_rep,
power_hibernate_req,
power_hibernate_rep,
power_wake_on_lan_req,
power_wake_on_lan_rep,
power_get_amt_params_req,
power_get_amt_params_rep,
power_error_rep = 100,

power_get_capability_req = 521,
power_get_capability_rep = 522

}susi_comm_cmd_t;


//--------------------------Power off/on data define----------------------------
typedef struct{
	char amtEn[8];
	char amtID[128];
	char amtPwd[128];
}power_amt_params;

//------------------------------------------------------------------------------

typedef enum{
	cagent_callback_abort = -1,
	cagent_callback_continue   
}cagent_callback_status_t;

static char AMT_EN[8] = {0};
static char AMT_ID[128] = {0};
static char AMT_PWD[128] = {0};

#define AMT_EN_KEY               "AmtEn"
#define AMT_ID_KEY               "AmtID"
#define AMT_PWD_KEY              "AmtPwd"

#define DEF_CONFIG_FILE_NAME     "agent_config.xml"

//Define Function Flag:
#define None					0x00
#define WOL						0x01
#define Shutdown				0x02
#define Restart					0x04
#define Hibernate				0x08
#define Suspend					0x10

#define NoneStr					"none"
#define WOLStr					"wol"
#define ShutdownStr				"shutdown"
#define RestartStr				"restart"
#define HibernateStr			"hibernate"
#define SuspendStr				"suspend"

typedef struct power_capability_info_t{
	char MacList[256];
	power_amt_params AmtParam;
	char funcsStr[256];
	unsigned int funcsCode;
}power_capability_info_t;


#endif