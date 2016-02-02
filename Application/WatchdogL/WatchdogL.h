#ifndef _SUSIACCESSWATCHDOG_H_
#define _SUSIACCESSWATCHDOG_H_

//-------------------------Memory leak check define--------------------------------
#ifdef MEM_LEAK_CHECK
#include <crtdbg.h>
_CrtMemState memStateStart, memStateEnd, memStateDiff;
#endif
//---------------------------------------------------------------------------------


#endif