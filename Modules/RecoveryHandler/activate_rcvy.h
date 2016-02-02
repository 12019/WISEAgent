#ifndef __ACTIVATE_RCVY_H__
#define __ACTIVATE_RCVY_H__

//-----------------------------------------------------------------------------
// Global functions declare:
//-----------------------------------------------------------------------------
#ifndef _is_linux
void Activate();
#endif /* _is_linux */
BOOL IsActive();
BOOL IsExpired();

#endif /* __ACTIVATE_RCVY_H__ */