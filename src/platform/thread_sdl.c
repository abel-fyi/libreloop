// SPDX-License-Identifier: GPL-3.0-only
#include "thread.h"
#include <SDL3/SDL.h>
#include <stdlib.h>
struct PlatformThread { SDL_Thread *native;void *(*routine)(void *);void *context; };
static int run(void *context){PlatformThread *thread=context;thread->routine(thread->context);return 0;}
PlatformThread *platform_thread_start(const char *name,void *(*routine)(void *),void *context) {
    PlatformThread *thread=calloc(1,sizeof *thread);if(!thread)return NULL;thread->routine=routine;thread->context=context;
    thread->native=SDL_CreateThread(run,name,thread);if(!thread->native){free(thread);return NULL;}return thread;
}
void platform_thread_join(PlatformThread *thread){if(thread){SDL_WaitThread(thread->native,NULL);free(thread);}}
