#ifndef _SCREENSHOT_HANDLER_H_
#define _SCREENSHOT_HANDLER_H_

#define cagent_request_screenshot  18
#define cagent_reply_screenshot    112

typedef enum{
	unknown_cmd = 0,
	//--------------------------Screenshot command define(401--420)----------------------------
	screenshot_transfer_req = 403,
	screenshot_transfer_rep = 404,
	screenshot_error_rep = 420,
	screenshot_capability_req = 521,
	screenshot_capability_rep = 522,
}susi_comm_cmd_t;

typedef struct ScreenshotUploadRep
{
	char uplFileName[64];
	char * base64Str;
	char status[16];
	char uplMsg[256];
}ScreenshotUploadRep;

#endif