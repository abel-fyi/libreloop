// SPDX-License-Identifier: GPL-3.0-only
#include "recording_writer.h"
#include "path.h"
#ifdef _WIN32
#include "windows_utf.h"
#include "file_io.h"
#else
#include <pthread.h>
#include <unistd.h>
#endif
#include <stdatomic.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <math.h>
struct RecordingWriter {
    int started,take;
#ifdef _WIN32
    HANDLE fd,thread;
#else
    int fd;pthread_t thread;
#endif
    atomic_uint frames;
    atomic_int stop,failed;
    RecordingRead read; void *context;
    char path[LL_PATH_CAPACITY];
};
#ifdef _WIN32
static atomic_uint writer_nonce;
static long long writer_write(RecordingWriter *w,const void *data,size_t bytes) {
    DWORD done=0;return WriteFile(w->fd,data,(DWORD)bytes,&done,NULL)?(long long)done:-1;
}
static long long writer_pwrite(RecordingWriter *w,const void *data,size_t bytes,size_t at) {
    LARGE_INTEGER zero={0},position,offset;offset.QuadPart=(LONGLONG)at;
    if(!SetFilePointerEx(w->fd,zero,&position,FILE_CURRENT) || !SetFilePointerEx(w->fd,offset,NULL,FILE_BEGIN))return -1;
    long long done=writer_write(w,data,bytes);
    if(!SetFilePointerEx(w->fd,position,NULL,FILE_BEGIN))return -1;return done;
}
static int writer_trim(RecordingWriter *w,unsigned frames) {
    LARGE_INTEGER end,current;end.QuadPart=44+(LONGLONG)frames*8;
    /* Windows rejects SetEndOfFile while a live waveform view maps this file,
       even when its length is already correct. Successful writes need no trim. */
    if(!GetFileSizeEx(w->fd,&current))return -1;
    if(current.QuadPart==end.QuadPart)return 0;
    return SetFilePointerEx(w->fd,end,NULL,FILE_BEGIN) && SetEndOfFile(w->fd)?0:-1;
}
#define WRITER_INTERRUPTED 0
#define WRITER_WRITE(w,p,n) writer_write(w,p,n)
#define WRITER_PWRITE(w,p,n,at) writer_pwrite(w,p,n,at)
#define WRITER_TRIM(w,n) writer_trim(w,n)
#define WRITER_SYNC(w) (FlushFileBuffers((w)->fd)?0:-1)
#else
#define WRITER_INTERRUPTED (errno==EINTR)
#define WRITER_WRITE(w,p,n) write((w)->fd,p,n)
#define WRITER_PWRITE(w,p,n,at) pwrite((w)->fd,p,n,(off_t)(at))
#define WRITER_TRIM(w,n) ftruncate((w)->fd,44+(off_t)(n)*8)
#define WRITER_SYNC(w) fsync((w)->fd)
#endif
static void le(unsigned char *out,uint32_t value,int bytes) { for(int i=0;i<bytes;i++) out[i]=(value>>(8*i))&255; }
static int header(RecordingWriter *w,unsigned frames) {
    unsigned char data[44]={0};
    memcpy(data,"RIFF",4); le(data+4,36+frames*8,4); memcpy(data+8,"WAVEfmt ",8);
    le(data+16,16,4); le(data+20,3,2); le(data+22,2,2); le(data+24,RATE,4);
    le(data+28,RATE*8,4); le(data+32,8,2); le(data+34,32,2); memcpy(data+36,"data",4); le(data+40,frames*8,4);
    size_t at=0;
    while(at<sizeof data) {
        long long n=WRITER_PWRITE(w,data+at,sizeof data-at,at);
        if(n<0 && WRITER_INTERRUPTED) continue;
        if(n<=0) return 0;
        at+=(size_t)n;
    }
    return 1;
}
RecordingWriter *recording_writer_open(const char *directory) {
    RecordingWriter *w=calloc(1,sizeof *w); if(!w) return NULL;
    if(snprintf(w->path,sizeof w->path,"%s/take-XXXXXX.wav",directory)>=(int)sizeof w->path) { free(w); return NULL; }
#ifdef _WIN32
    w->fd=INVALID_HANDLE_VALUE;
    size_t length=strlen(w->path);char *suffix=w->path+length-10;
    for(int attempt=0;attempt<128;attempt++) {
        unsigned id=atomic_fetch_add(&writer_nonce,1)+GetCurrentProcessId()+(unsigned)GetTickCount64();
        for(int i=0;i<6;i++){suffix[5-i]="0123456789abcdefghijklmnopqrstuvwxyz"[id%36];id/=36;}
        wchar_t *wide=windows_utf16(w->path);if(!wide){free(w);return NULL;}
        w->fd=CreateFileW(wide,GENERIC_READ|GENERIC_WRITE,FILE_SHARE_READ|FILE_SHARE_DELETE,
            NULL,CREATE_NEW,FILE_ATTRIBUTE_NORMAL,NULL);DWORD error=GetLastError();free(wide);
        if(w->fd!=INVALID_HANDLE_VALUE)break;
        if(error!=ERROR_FILE_EXISTS && error!=ERROR_ALREADY_EXISTS){free(w);return NULL;}
    }
    if(w->fd==INVALID_HANDLE_VALUE){free(w);return NULL;}
#else
    w->fd=mkstemps(w->path,4);
    if(w->fd<0) { free(w); return NULL; }
#endif
    atomic_init(&w->frames,0); atomic_init(&w->stop,0); atomic_init(&w->failed,0);
#ifdef _WIN32
    LARGE_INTEGER offset;offset.QuadPart=44;
    if(!header(w,0) || !SetFilePointerEx(w->fd,offset,NULL,FILE_BEGIN)){CloseHandle(w->fd);platform_file_remove(w->path);free(w);return NULL;}
#else
    if(!header(w,0) || lseek(w->fd,44,SEEK_SET)!=44) { close(w->fd); unlink(w->path); free(w); return NULL; }
#endif
    return w;
}
static void *worker(void *context) {
    RecordingWriter *w=context; float pcm[4096*2]; unsigned char bytes[4096*8];
    unsigned checkpoint=0;
    for(;;) {
        unsigned n=w->read(w->context,w->take,pcm,4096);
        unsigned old=atomic_load_explicit(&w->frames,memory_order_relaxed);
        if(!n) {
            if(atomic_load(&w->stop)) break;
            #ifdef _WIN32
            Sleep(5);
#else
            struct timespec pause={0,5000000}; nanosleep(&pause,NULL);
#endif
            continue;
        }
        if(n>4096 || n>SAMPLE_MAX_FRAMES-old) { atomic_store(&w->failed,1); break; }
        for(unsigned i=0;i<n*2;i++) {
            float value=isfinite(pcm[i])?pcm[i]:0; uint32_t bits; memcpy(&bits,&value,4); le(bytes+i*4,bits,4);
        }
        size_t at=0,total=(size_t)n*8;
        while(at<total) {
            long long written=WRITER_WRITE(w,bytes+at,total-at);
            if(written<0 && WRITER_INTERRUPTED) continue;
            if(written<=0) { atomic_store(&w->failed,1); break; } at+=(size_t)written;
        }
        if(atomic_load(&w->failed)) break;
        atomic_store_explicit(&w->frames,old+n,memory_order_release);
        if(old+n-checkpoint>=RATE) { if(!header(w,old+n)) { atomic_store(&w->failed,1); break; } checkpoint=old+n; }
    }
    unsigned frames=atomic_load(&w->frames);
    if(WRITER_TRIM(w,frames) || !header(w,frames) || WRITER_SYNC(w)) atomic_store(&w->failed,1);
    return NULL;
}
#ifdef _WIN32
static DWORD WINAPI windows_writer(LPVOID context){worker(context);return 0;}
#endif
const char *recording_writer_path(const RecordingWriter *w) { return w->path; }
int recording_writer_start(RecordingWriter *w,RecordingRead read,void *context,int take) {
    if(w->started || !read) return 0;
#ifdef _WIN32
    if(w->fd==INVALID_HANDLE_VALUE)return 0;
#else
    if(w->fd<0)return 0;
#endif
    w->read=read; w->context=context; w->take=take;
    #ifdef _WIN32
    w->thread=CreateThread(NULL,0,windows_writer,w,0,NULL);if(!w->thread)return 0;
#else
    if(pthread_create(&w->thread,NULL,worker,w)) return 0;
#endif
    w->started=1; return 1;
}
unsigned recording_writer_frames(const RecordingWriter *w) { return atomic_load_explicit(&w->frames,memory_order_acquire); }
int recording_writer_failed(const RecordingWriter *w) { return atomic_load(&w->failed); }
int recording_writer_finish(RecordingWriter *w) {
    if(w->started) { atomic_store(&w->stop,1);
#ifdef _WIN32
        WaitForSingleObject(w->thread,INFINITE);CloseHandle(w->thread);
#else
        pthread_join(w->thread,NULL);
#endif
        w->started=0;
    }
#ifdef _WIN32
    if(w->fd!=INVALID_HANDLE_VALUE){if(!CloseHandle(w->fd))atomic_store(&w->failed,1);w->fd=INVALID_HANDLE_VALUE;}
#else
    if(w->fd>=0) { if(close(w->fd)) atomic_store(&w->failed,1); w->fd=-1; }
#endif
    return !recording_writer_failed(w);
}
void recording_writer_free(RecordingWriter *w,int remove_file) {
    if(!w) return;
    recording_writer_finish(w);
#ifdef _WIN32
    if(remove_file)platform_file_remove(w->path);
#else
    if(remove_file)unlink(w->path);
#endif
    free(w);
}
