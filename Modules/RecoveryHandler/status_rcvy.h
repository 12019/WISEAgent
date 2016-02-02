#ifndef __STATUS_RCVY_H__
#define __STATUS_RCVY_H__

//-----------------------------------------------------------------------------
// Global functions declare:
//-----------------------------------------------------------------------------
BOOL IsAcrocmdRunning();
void DeactivateARSM();
void GetRcvyCurStatus();
BOOL GetRcvyStatus(recovery_status_t * rcvyStatus);
void RunASRM(BOOL isNeedHotkey);
void SendAgentStartMessage(BOOL isCallHotkey);

#endif /* __STATUS_RCVY_H__ */