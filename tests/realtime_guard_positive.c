// SPDX-License-Identifier: GPL-3.0-only
#include "realtime_guard.h"
#include <pthread.h>
int main(void) {
    pthread_mutex_t mutex=PTHREAD_MUTEX_INITIALIZER;
    realtime_guard_enter();
    pthread_mutex_trylock(&mutex);
    realtime_guard_leave();
    return 0; /* A functioning guard must abort before reaching this. */
}
