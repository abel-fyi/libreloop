// SPDX-License-Identifier: GPL-3.0-only
/* Pinned rcore.c WaitTime with the original CMake's busy-wait flags OFF.
   Adapted raylib5.5 code; Ramon Santamaria, zlib notice docs/licenses/raylib.txt. */
#include "frame_wait.h"
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <time.h>
#include <unistd.h>
#endif
void platform_frame_wait(double seconds){
    if(seconds<0)return;
#ifdef _WIN32
    Sleep((unsigned long)(seconds*1000.0));
#elif defined(__APPLE__)
    usleep(seconds*1000000.0);
#else
    struct timespec req={0};
    time_t sec=seconds;long nsec=(seconds-sec)*1000000000L;
    req.tv_sec=sec;req.tv_nsec=nsec;
    while(nanosleep(&req,&req)==-1)continue;
#endif
}
