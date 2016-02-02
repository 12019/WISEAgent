#ifndef __INSTALL_UPDATE_RCVY_H__
#define __INSTALL_UPDATE_RCVY_H__


#ifndef _is_linux
//-----------------------------------------------------------------------------
// Global functions declare:
//-----------------------------------------------------------------------------
BOOL IsInstallerRunning();
BOOL SetAcrLatestActionToReg(char * action);
BOOL GetAcrLatestActionFromReg(char * action);
BOOL AcrInstallMsgSend(EINSTALLTYPE installType, char * msg, rcvy_reply_status_code statusCode);
void UpdateAction(recovery_install_params * pRcvyInstallParams);
void InstallAction(recovery_install_params * pRcvyInstallParams);

#endif /* _is_linux */


#endif /* __INSTALL_UPDATE_RCVY_H__ */