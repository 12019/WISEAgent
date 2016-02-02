#ifndef __ACTIVATE_PRTT_H__
#define __ACTIVATE_PRTT_H__

//-----------------------------------------------------------------------------
// Global variables declare:
//-----------------------------------------------------------------------------
BOOL IsActive();
BOOL IsExpired();
void Activate();

#ifdef _WIN32
void IniPasswd();
#else
BOOL IsInstalled();
void InitLicense();
BOOL DeleteLicense();
#endif /*_WIN32*/

#endif /*__ACTIVATE_PRTT_H__*/