#ifndef __INSTALL_UPDATE_PRTT_H__
#define __INSTALL_UPDATE_PRTT_H__

#ifdef _WIN32
//-----------------------------------------------------------------------------
// Global variables declare:
//-----------------------------------------------------------------------------
BOOL IsDownloadAction;
BOOL McAfeeIsInstalling;
BOOL IsUpdateAction;
BOOL IsInstallAction;
BOOL IsDownloadAction;
BOOL IsInstallThenActive;

//-----------------------------------------------------------------------------
// Global functions declare:
//-----------------------------------------------------------------------------
void InstallAction(prtt_installer_dl_params_t * pDLParams);
void McAfeeInstallThreadStop();
BOOL IsInstallerRunning();

#endif /* _WIN32 */

#endif /*__INSTALL_UPDATE_PRTT_H__*/