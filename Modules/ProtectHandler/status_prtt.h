#ifndef __STATUS_PRTT_H__
#define __STATUS_PRTT_H__

//-----------------------------------------------------------------------------
// Global functions declare:
//-----------------------------------------------------------------------------
BOOL GetPrttStatus(prtt_status_t * prttStatus);
void GetProtectionCurStatus();
BOOL IsProtect();
BOOL IsSolidify();
//void UpdateStatus(char* pValidMsg);

#endif /*__STATUS_PRTT_H__*/