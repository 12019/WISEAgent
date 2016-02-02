/*
Copyright (c) 2013 Roger Light <roger@atchoo.org>
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.
3. Neither the name of mosquitto nor the names of its
   contributors may be used to endorse or promote products derived from
   this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
POSSIBILITY OF SUCH DAMAGE.
*/

#ifdef __APPLE__
#include <mach/mach.h>
#include <mach/mach_time.h>
#endif

#ifdef WIN32
//#  define _WIN32_WINNT _WIN32_WINNT_VISTA
#  include <windows.h>
#else
#  include <unistd.h>
#endif
#include <time.h>

#include "mosquitto.h"
#include "time_mosq.h"

#ifdef WIN64
static bool tick64 = false;

void _windows_time_version_check(void)
{
	OSVERSIONINFO vi;

	tick64 = false;

	memset(&vi, 0, sizeof(OSVERSIONINFO));
	vi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
	if(GetVersionEx(&vi)){
		if(vi.dwMajorVersion > 5){
			tick64 = true;
		}
	}
}
#endif

unsigned long long _GetTickCount()
{
	LARGE_INTEGER qpc;
	LARGE_INTEGER qpf;
	unsigned long long tickMs = 0;
	QueryPerformanceCounter(&qpc);
	QueryPerformanceFrequency(&qpf);
	tickMs = qpc.QuadPart*1000/qpf.QuadPart;
	return tickMs;
}

time_t mosquitto_time(void)
{
#ifdef WIN32
//	#if WINVER >= 0x0600
//		return GetTickCount64()/1000;
//	#else
		return _GetTickCount()/1000; /* FIXME - need to deal with overflow. */
//	#endif
#elif WIN64 //Original source code
	if(tick64){
		return GetTickCount64()/1000;
	}else{
		return GetTickCount()/1000; /* FIXME - need to deal with overflow. */
	}
#elif _POSIX_TIMERS>0 && defined(_POSIX_MONOTONIC_CLOCK)
	struct timespec tp;

	clock_gettime(CLOCK_MONOTONIC, &tp);
	return tp.tv_sec;
#elif defined(__APPLE__)
	static mach_timebase_info_data_t tb;
    uint64_t ticks;
	uint64_t sec;

	ticks = mach_absolute_time();

	if(tb.denom == 0){
		mach_timebase_info(&tb);
	}
	sec = ticks*tb.numer/tb.denom/1000000000;

	return (time_t)sec;
#else
	return time(NULL);
#endif
}

