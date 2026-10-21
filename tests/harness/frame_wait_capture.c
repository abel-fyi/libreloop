// SPDX-License-Identifier: GPL-3.0-only
/* Host-native wait arguments are captured; this performs no real sleep. */
#include <stdio.h>
#include <unistd.h>
static int sleep_calls;static unsigned long sleep_value;
#if defined(__APPLE__)
static int captured_usleep(useconds_t value){sleep_calls++;sleep_value=value;return 0;}
#define usleep captured_usleep
#include "../../src/platform/frame_wait.c"
#define fixture_wait platform_frame_wait
#include "frame_wait_sequence.h"
int main(void){FILE *f=fopen("wait.txt","wb");if(!f)return 1;int result=frame_wait_sequence(f);return fclose(f) || result;}
#else
#error This original-derived capture currently verifies the macOS usleep branch only.
#endif
