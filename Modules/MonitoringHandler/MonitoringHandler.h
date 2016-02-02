#ifndef _MONITORING_HANDLER_H_
#define _MONITORING_HANDLER_H_

#define cagent_request_device_monitoring 10
#define cagent_action_device_monitoring 103

typedef enum{
   unknown_cmd = 0,
   //------------------------Device monitor command define(1--10,251--300)---------------------
   devmon_set_hwminfo_auto_upload_req = 1,
   devmon_set_hwminfo_reqp_req = 2,
   devmon_hwminfo_req = 3,
   devmon_hwminfo_rep = 4,
   devmon_set_hddinfo_auto_upload_req = 5,
   devmon_set_hddinfo_reqp_req = 6,
   devmon_hddinfo_req = 7,
   devmon_hddinfo_rep = 8,

   devmon_get_pfminfo_req = 251,
   devmon_get_pfminfo_rep = 252,
   devmon_report_req = 253,
   devmon_report_rep = 254,
   devmon_set_thr_req = 257,
   devmon_set_thr_rep = 258,
   devmon_del_all_thr_req = 259,
   devmon_del_all_thr_rep = 260,

	devmon_get_os_info_req = 261,
	devmon_get_os_info_rep,
	devmon_stda_get_osinfo_req = 263, 

   devmon_set_hwminfo_auto_upload_rep = 271,
   devmon_set_hwminfo_reqp_rep = 272,
   devmon_set_hddinfo_auto_upload_rep = 273,
   devmon_set_hddinfo_reqp_rep = 274,
	devmon_get_hdd_smart_info_req = 275,
	devmon_get_hdd_smart_info_rep = 276,

   devmon_stda_get_pfminfo_req = 281,
   devmon_stda_get_hwminfo_req = 282,
   devmon_stda_get_hddinfo_req = 283,

   devmon_error_rep = 300,
   
}susi_comm_cmd_t;

void Handler_Uninitialize();

#endif /* _MONITORING_HANDLER_H_ */