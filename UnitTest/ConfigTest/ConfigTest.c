// ConfigTest.cpp : Defines the entry point for the console application.
//
#include "platform.h"
#include "common.h"
#include "susiaccess_def.h"
#include "version.h"
#include "agentlog.h"
#include <configuration.h>
#include <profile.h>
#include "amtconfig.h"
#include "kvmconfig.h"

#define DEF_SUSIACCESSAGENT_CONFIG_NAME "agent_config.xml"
//#define DEF_SUSIACCESSAGENT_PROFILE_NAME "agent_profile.xml"

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
	//char CAgentLogPath[MAX_PATH] = {0};
	char CAgentConfigPath[MAX_PATH] = {0};
	//char CAgentProfilePath[MAX_PATH] = {0};
	char TestValue[MAX_PATH] = {0};
	susiaccess_agent_conf_body_t config;
	susiaccess_agent_profile_body_t profile;
	susiaccess_amt_conf_body_t myamt;
	susiaccess_kvm_conf_body_t mykvm;

#ifdef MEM_LEAK_CHECK
	_CrtSetDbgFlag ( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF ); 
	//_CrtSetBreakAlloc(3719);
	_CrtMemCheckpoint( &memStateStart);
#endif

	memset(moudlePath, 0 , sizeof(moudlePath));
	app_os_get_module_path(moudlePath);

	//path_combine(CAgentLogPath, moudlePath, DEF_SUSIACCESSAGENT_LOG_NAME);
	path_combine(CAgentConfigPath, moudlePath, DEF_SUSIACCESSAGENT_CONFIG_NAME);
	//path_combine(CAgentProfilePath, moudlePath, DEF_SUSIACCESSAGENT_PROFILE_NAME);
	//sprintf_s(CAgentLogPath, sizeof(CAgentLogPath), "%s%s", moudlePath, DEF_SUSIACCESSAGENT_LOG_NAME);

	SUSIAccessAgentLogHandle = InitLog(moudlePath);
	SUSIAccessAgentLog(Normal, "Current path: %s", moudlePath);

	//path_combine(CAgentLogPath, moudlePath, DEF_SUSIACCESSAGENT_LOG_NAME);

	if(!cfg_load(CAgentConfigPath, &config))
	{
		SUSIAccessAgentLog(Error, "CFG load failed: %s", CAgentConfigPath);
		if(cfg_create(CAgentConfigPath, &config))
			SUSIAccessAgentLog(Error, "CFG create failed: %s", CAgentConfigPath);
		else
			SUSIAccessAgentLog(Normal, "CFG Created: %s", CAgentConfigPath);
	}
	else
	{
		SUSIAccessAgentLog(Normal, "CFG loaded: %s", CAgentConfigPath);

		SUSIAccessAgentLog(Normal, "Run Mode: %s", config.runMode);
		//SUSIAccessAgentLog(Normal, "Lunch Connect: %s", config.lunchConnect);
		SUSIAccessAgentLog(Normal, "Auto Start: %s", config.autoStart);
		//SUSIAccessAgentLog(Normal, "Auto Report: %s", config.autoReport);
		SUSIAccessAgentLog(Normal, "Server IP: %s", config.serverIP);
		SUSIAccessAgentLog(Normal, "Server Port: %s", config.serverPort);
		SUSIAccessAgentLog(Normal, "Auth: %s", config.serverAuth);
		SUSIAccessAgentLog(Normal, "TLSType: %d", config.tlstype);
		SUSIAccessAgentLog(Normal, "CACert: %s", config.cafile);
		SUSIAccessAgentLog(Normal, "CAPath: %s", config.capath);
		SUSIAccessAgentLog(Normal, "CertFile: %s", config.certfile);
		SUSIAccessAgentLog(Normal, "KeyFile: %s", config.keyfile);
		SUSIAccessAgentLog(Normal, "CertPW: %s", config.cerpasswd);
		SUSIAccessAgentLog(Normal, "TLSPSK: %s", config.psk);
		SUSIAccessAgentLog(Normal, "Identify: %s", config.identity);
		SUSIAccessAgentLog(Normal, "Ciphers: %s", config.ciphers);

		/*SUSIAccessAgentLog(Normal, "Login ID: %s", config.loginID);
		SUSIAccessAgentLog(Normal, "Password: %s", config.loginPwd);*/
	}

	if(!cfg_get(CAgentConfigPath, "Test", TestValue, sizeof(TestValue)))
	{
		SUSIAccessAgentLog(Error, "CFG get Test value on: %s", CAgentConfigPath);
		if(cfg_set(CAgentConfigPath, "Test", "test" ))
			SUSIAccessAgentLog(Error, "CFG set Test value on: %s", CAgentConfigPath);
		else
			SUSIAccessAgentLog(Normal, "CFG set Test value failed on: %s", CAgentConfigPath);

	}
	else
	{
		SUSIAccessAgentLog(Normal, "CFG get Test value: %s", TestValue);
	}

	if(cfg_save(CAgentConfigPath, &config))
		SUSIAccessAgentLog(Normal, "CFG saved on: %s", CAgentConfigPath);
	else
		SUSIAccessAgentLog(Error, "CFG save failed on: %s", CAgentConfigPath);

	
	SUSIAccessAgentLog(Normal, "/*******Profile Test*******/");

	if(!profile_load(CAgentConfigPath, &profile))
	{
		SUSIAccessAgentLog(Error, "Profile load failed: %s", CAgentConfigPath);
		if(profile_create(CAgentConfigPath, &profile))
			SUSIAccessAgentLog(Error, "Profile create failed: %s", CAgentConfigPath);
		else
			SUSIAccessAgentLog(Normal, "Profile Created: %s", CAgentConfigPath);
	}
	else
	{
		SUSIAccessAgentLog(Normal, "Profile loaded: %s", CAgentConfigPath);
	}
	snprintf(profile.version, DEF_VERSION_LENGTH, "%d.%d.%d.%d", VER_MAJOR, VER_MINOR, VER_BUILD, VER_FIX);

	SUSIAccessAgentLog(Normal, "Version: %s", profile.version);
	SUSIAccessAgentLog(Normal, "hostname: %s", profile.hostname);
	SUSIAccessAgentLog(Normal, "devId: %s", profile.devId);
	SUSIAccessAgentLog(Normal, "sn: %s", profile.sn);
	SUSIAccessAgentLog(Normal, "mac: %s", profile.mac);

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

	if(!profile_get(CAgentConfigPath, "Profile_Test", TestValue, sizeof(TestValue)))
	{
		SUSIAccessAgentLog(Error, "Profile get Test value on: %s", CAgentConfigPath);
		if(profile_set(CAgentConfigPath, "Profile_Test", "profiletest" ))
			SUSIAccessAgentLog(Normal, "Profile set Test value on: %s", CAgentConfigPath);
		else
			SUSIAccessAgentLog(Error, "Profile set Test value failed on: %s", CAgentConfigPath);

	}
	else
	{
		SUSIAccessAgentLog(Normal, "Profile get Test value: %s", TestValue);
	}

	if(profile_save(CAgentConfigPath, &profile))
		SUSIAccessAgentLog(Normal, "Profile saved on: %s", CAgentConfigPath);
	else
		SUSIAccessAgentLog(Error, "Profile save failed on: %s", CAgentConfigPath);

	SUSIAccessAgentLog(Normal, "/*******Customize Test*******/");
	if(!amt_load(CAgentConfigPath, &myamt))
	{
		SUSIAccessAgentLog(Error, "AMT load failed: %s", CAgentConfigPath);
		if(amt_create(CAgentConfigPath, &myamt))
			SUSIAccessAgentLog(Error, "AMT create failed: %s", CAgentConfigPath);
		else
			SUSIAccessAgentLog(Normal, "AMT Created: %s", CAgentConfigPath);
	}
	else
	{
		SUSIAccessAgentLog(Normal, "AMT loaded: %s", CAgentConfigPath);
	}

	SUSIAccessAgentLog(Normal, "amtEn: %s", myamt.amtEn);
	SUSIAccessAgentLog(Normal, "amtID: %s", myamt.amtID);
	SUSIAccessAgentLog(Normal, "amtPwd: %s", myamt.amtPwd);

	if(!kvm_load(CAgentConfigPath, &mykvm))
	{
		SUSIAccessAgentLog(Error, "KVM load failed: %s", CAgentConfigPath);
		if(kvm_create(CAgentConfigPath, &mykvm))
			SUSIAccessAgentLog(Error, "KVM create failed: %s", CAgentConfigPath);
		else
			SUSIAccessAgentLog(Normal, "KVM Created: %s", CAgentConfigPath);
	}
	else
	{
		SUSIAccessAgentLog(Normal, "KVM loaded: %s", CAgentConfigPath);
	}

	SUSIAccessAgentLog(Normal, "kvmMode: %s", mykvm.kvmMode);
	SUSIAccessAgentLog(Normal, "custVNCPwd: %s", mykvm.custVNCPwd);
	SUSIAccessAgentLog(Normal, "custVNCPort: %s", mykvm.custVNCPort);

	printf("Click enter to exit");
	fgetc(stdin);

	UninitLog(SUSIAccessAgentLogHandle);
#ifdef MEM_LEAK_CHECK
	_CrtMemCheckpoint( &memStateEnd );
	if ( _CrtMemDifference( &memStateDiff, &memStateStart, &memStateEnd) )
		_CrtMemDumpStatistics( &memStateDiff );
#endif
	return iRet;
}