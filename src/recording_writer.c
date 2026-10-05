// SPDX-License-Identifier: GPL-3.0-only
#include "recording_writer.h"
#include <pthread.h>
#include <stdatomic.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <math.h>
struct RecordingWriter {
    int fd,started,take;
    pthread_t thread;
    atomic_uint frames;
    atomic_int stop,failed;
    RecordingRead read; void *context;
    char path[PATH_MAX];
};
static void le(unsigned char *out,uint32_t value,int bytes) { for(int i=0;i<bytes;i++) out[i]=(value>>(8*i))&255; }
static int header(RecordingWriter *w,unsigned frames) {
    unsigned char data[44]={0};
    memcpy(data,"RIFF",4); le(data+4,36+frames*8,4); memcpy(data+8,"WAVEfmt ",8);
    le(data+16,16,4); le(data+20,3,2); le(data+22,2,2); le(data+24,RATE,4);
    le(data+28,RATE*8,4); le(data+32,8,2); le(data+34,32,2); memcpy(data+36,"data",4); le(data+40,frames*8,4);
    size_t at=0;
    while(at<sizeof data) {
        ssize_t n=pwrite(w->fd,data+at,sizeof data-at,(off_t)at);
        if(n<0 && errno==EINTR) continue;
        if(n<=0) return 0;
        at+=(size_t)n;
    }
    return 1;
}
RecordingWriter *recording_writer_open(const char *directory) {
    RecordingWriter *w=calloc(1,sizeof *w); if(!w) return NULL;
    if(snprintf(w->path,sizeof w->path,"%s/take-XXXXXX.wav",directory)>=(int)sizeof w->path) { free(w); return NULL; }
    w->fd=mkstemps(w->path,4);
    if(w->fd<0) { free(w); return NULL; }
    atomic_init(&w->frames,0); atomic_init(&w->stop,0); atomic_init(&w->failed,0);
    if(!header(w,0) || lseek(w->fd,44,SEEK_SET)!=44) { close(w->fd); unlink(w->path); free(w); return NULL; }
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
            struct timespec pause={0,5000000}; nanosleep(&pause,NULL); continue;
        }
        if(n>4096 || n>SAMPLE_MAX_FRAMES-old) { atomic_store(&w->failed,1); break; }
        for(unsigned i=0;i<n*2;i++) {
            float value=isfinite(pcm[i])?pcm[i]:0; uint32_t bits; memcpy(&bits,&value,4); le(bytes+i*4,bits,4);
        }
        size_t at=0,total=(size_t)n*8;
        while(at<total) {
            ssize_t written=write(w->fd,bytes+at,total-at);
            if(written<0 && errno==EINTR) continue;
            if(written<=0) { atomic_store(&w->failed,1); break; } at+=(size_t)written;
        }
        if(atomic_load(&w->failed)) break;
        atomic_store_explicit(&w->frames,old+n,memory_order_release);
        if(old+n-checkpoint>=RATE) { if(!header(w,old+n)) { atomic_store(&w->failed,1); break; } checkpoint=old+n; }
    }
    unsigned frames=atomic_load(&w->frames);
    if(ftruncate(w->fd,44+(off_t)frames*8) || !header(w,frames) || fsync(w->fd)) atomic_store(&w->failed,1);
    return NULL;
}
const char *recording_writer_path(const RecordingWriter *w) { return w->path; }
int recording_writer_start(RecordingWriter *w,RecordingRead read,void *context,int take) {
    if(w->started || !read || w->fd<0) return 0;
    w->read=read; w->context=context; w->take=take;
    if(pthread_create(&w->thread,NULL,worker,w)) return 0;
    w->started=1; return 1;
}
unsigned recording_writer_frames(const RecordingWriter *w) { return atomic_load_explicit(&w->frames,memory_order_acquire); }
int recording_writer_failed(const RecordingWriter *w) { return atomic_load(&w->failed); }
int recording_writer_finish(RecordingWriter *w) {
    if(w->started) { atomic_store(&w->stop,1); pthread_join(w->thread,NULL); w->started=0; }
    if(w->fd>=0) { if(close(w->fd)) atomic_store(&w->failed,1); w->fd=-1; }
    return !recording_writer_failed(w);
}
void recording_writer_free(RecordingWriter *w,int remove_file) {
    if(!w) return;
    recording_writer_finish(w); if(remove_file) unlink(w->path); free(w);
}
