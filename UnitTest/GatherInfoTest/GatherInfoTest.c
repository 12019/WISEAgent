#include "platform.h"
#include "common.h"
#include "susiaccess_def.h"
#include "version.h"
#include <SAGatherInfo.h>
#include "agentlog.h"

//-------------------------Memory leak check define--------------------------------
#ifdef MEM_LEAK_CHECK
#include <crtdbg.h>
_CrtMemState memStateStart, memStateEnd, memStateDiff;
#endif
//---------------------------------------------------------------------------------

int main(int argc, char *argv[])
{
	int iRet = 0;
	char moudlePath[MAX_PATH] = {0};
	char CAgentLogPath[MAX_PATH] = {0};
	susiaccess_agent_profile_body_t profile;

#ifdef MEM_LEAK_CHECK
	_CrtSetDbgFlag ( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF ); 
	//_CrtSetBreakAlloc(3719);
	_CrtMemCheckpoint( &memStateStart);
#endif

	memset(moudlePath, 0 , sizeof(moudlePath));
	app_os_get_module_path(moudlePath);

	path_combine(CAgentLogPath, moudlePath, DEF_SUSIACCESSAGENT_LOG_NAME);
	//sprintf_s(CAgentLogPath, sizeof(CAgentLogPath), "%s%s", moudlePath, DEF_SUSIACCESSAGENT_LOG_NAME);

	SUSIAccessAgentLogHandle = InitLog(CAgentLogPath);
	SUSIAccessAgentLog(Normal, "Current path: %s", moudlePath);

	memset(&profile, 0 , sizeof(susiaccess_agent_profile_body_t));
	sagetInfo_gatherplatforminfo(&profile);

	snprintf(profile.version, DEF_VERSION_LENGTH, "%d.%d.%d.%d", VER_MAJOR, VER_MINOR, VER_BUILD, VER_FIX);

	SUSIAccessAgentLog(Normal, "Version: %s", profile.version);
	SUSIAccessAgentLog(Normal, "hostname: %s", profile.hostname);
	SUSIAccessAgentLog(Normal, "devId: %s", profile.devId);
	SUSIAccessAgentLog(Normal, "sn: %s", profile.sn);
	SUSIAccessAgentLog(Normal, "mac: %s", profile.mac);
	SUSIAccessAgentLog(Normal, "lal: %s", profile.lal);

	SUSIAccessAgentLog(Normal, "type: %s", profile.type);
	SUSIAccessAgentLog(Normal, "product: %s", profile.product);
	SUSIAccessAgentLog(Normal, "manufacture: %s", profile.manufacture);

	SUSIAccessAgentLog(Normal, "osversion: %s", profile.osversion);
	SUSIAccessAgentLog(Normal, "biosversion: %s", profile.biosversion);
	SUSIAccessAgentLog(Normal, "platformname: %s", profile.platformname);
	SUSIAccessAgentLog(Normal, "processorname: %s", profile.processorname);
	SUSIAccessAgentLog(Normal, "osarchitect: %s", profile.osarchitect);
	SUSIAccessAgentLog(Normal, "totalmemsize: %d", profile.totalmemsize);
	SUSIAccessAgentLog(Normal, "maclist: %s", profile.maclist);
	SUSIAccessAgentLog(Normal, "localip: %s", profile.localip);

	printf("Click enter to exit");
	fgetc(stdin);

	UninitLog(SUSIAccessAgentLogHandle);
#ifdef MEM_LEAK_CHECK
	_CrtMemCheckpoint( &memStateEnd );
	if ( _CrtMemDifference( &memStateDiff, &memStateStart, &memStateEnd) )
		_CrtMemDumpStatistics( &memStateDiff );
#endif

	return 0;
}

