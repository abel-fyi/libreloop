// SPDX-License-Identifier: GPL-3.0-only
#ifndef LIBRELOOP_THREAD_H
#define LIBRELOOP_THREAD_H
typedef struct PlatformThread PlatformThread;
PlatformThread *platform_thread_start(const char *name,void *(*routine)(void *),void *context);
void platform_thread_join(PlatformThread *thread);
#endif
