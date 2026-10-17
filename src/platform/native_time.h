// SPDX-License-Identifier: GPL-3.0-only
#ifndef LIBRELOOP_NATIVE_TIME_H
#define LIBRELOOP_NATIVE_TIME_H
/* Native audio/MIDI delivery clocks. No allocation or deferred initialization. */
#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
static inline double platform_native_time(void) {
    LARGE_INTEGER counter,frequency;
    if(!QueryPerformanceFrequency(&frequency) || !QueryPerformanceCounter(&counter) || frequency.QuadPart<=0)return 0;
    return counter.QuadPart/(double)frequency.QuadPart;
}
#else
#include <time.h>
static inline double platform_native_time(void) {
    struct timespec t;clock_gettime(CLOCK_MONOTONIC,&t);return t.tv_sec+t.tv_nsec/1e9;
}
#endif
#endif
