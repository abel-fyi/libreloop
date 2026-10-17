// SPDX-License-Identifier: GPL-3.0-only
/* Test-only dyld interposition covers dynamic libc/pthread calls reached from
   the shared callback path. It is not linked into the application. */
#include "realtime_guard.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <pthread.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
static _Thread_local int active;
void realtime_guard_enter(void) { active=1; }
void realtime_guard_leave(void) { active=0; }
static void check(const char *operation) {
    if(active) {
        active=0; fprintf(stderr,"Real-time forbidden call: %s\n",operation); abort();
    }
}
static void *guard_malloc(size_t n) { check("malloc"); return malloc(n); }
static void *guard_calloc(size_t n,size_t s) { check("calloc"); return calloc(n,s); }
static void *guard_realloc(void *p,size_t n) { check("realloc"); return realloc(p,n); }
static void guard_free(void *p) { check("free"); free(p); }
static int guard_lock(pthread_mutex_t *p) { check("pthread_mutex_lock"); return pthread_mutex_lock(p); }
static int guard_trylock(pthread_mutex_t *p) { check("pthread_mutex_trylock"); return pthread_mutex_trylock(p); }
static int guard_unlock(pthread_mutex_t *p) { check("pthread_mutex_unlock"); return pthread_mutex_unlock(p); }
static int guard_once(pthread_once_t *p,void (*fn)(void)) { check("pthread_once"); return pthread_once(p,fn); }
static FILE *guard_fopen(const char *p,const char *m) { check("fopen"); return fopen(p,m); }
static size_t guard_fread(void *p,size_t s,size_t n,FILE *f) { check("fread"); return fread(p,s,n,f); }
static size_t guard_fwrite(const void *p,size_t s,size_t n,FILE *f) { check("fwrite"); return fwrite(p,s,n,f); }
static int guard_printf(const char *format,...) { check("printf"); va_list args;va_start(args,format);int n=vprintf(format,args);va_end(args);return n; }
static int guard_fprintf(FILE *f,const char *format,...) { check("fprintf"); va_list args;va_start(args,format);int n=vfprintf(f,format,args);va_end(args);return n; }
static ssize_t guard_read(int fd,void *p,size_t n) { check("read"); return read(fd,p,n); }
static ssize_t guard_write(int fd,const void *p,size_t n) { check("write"); return write(fd,p,n); }
static int guard_open(const char *p,int flags,...) {
    check("open");mode_t mode=0;if(flags&O_CREAT){va_list args;va_start(args,flags);mode=va_arg(args,int);va_end(args);}return open(p,flags,mode);
}
static int guard_sleep(const struct timespec *p,struct timespec *r) { check("nanosleep");return nanosleep(p,r); }
#define INTERPOSE(replacement,original) \
    __attribute__((used,section("__DATA,__interpose"))) \
    static struct { const void *replacement;const void *original; } pair_##original = { (const void *)&replacement,(const void *)&original }
INTERPOSE(guard_malloc,malloc);
INTERPOSE(guard_calloc,calloc);
INTERPOSE(guard_realloc,realloc);
INTERPOSE(guard_free,free);
INTERPOSE(guard_lock,pthread_mutex_lock);
INTERPOSE(guard_trylock,pthread_mutex_trylock);
INTERPOSE(guard_unlock,pthread_mutex_unlock);
INTERPOSE(guard_once,pthread_once);
INTERPOSE(guard_fopen,fopen);
INTERPOSE(guard_fread,fread);
INTERPOSE(guard_fwrite,fwrite);
INTERPOSE(guard_printf,printf);
INTERPOSE(guard_fprintf,fprintf);
INTERPOSE(guard_read,read);
INTERPOSE(guard_write,write);
INTERPOSE(guard_open,open);
INTERPOSE(guard_sleep,nanosleep);
