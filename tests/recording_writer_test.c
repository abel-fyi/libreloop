// SPDX-License-Identifier: GPL-3.0-only
#include "recording_writer.h"
#include "history.h"
#include "waveform.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdatomic.h>
#include <time.h>
#include <math.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <signal.h>
#define CHECK(x) do { if(!(x)) { fprintf(stderr,"Failed line %d: %s\n",__LINE__,#x); return 1; } } while(0)
typedef struct { unsigned read; atomic_uint available; } Source;
static unsigned read_audio(void *context,int take,float *stereo,unsigned count) {
    Source *s=context; (void)take;
    unsigned available=atomic_load(&s->available),n=available-s->read; if(n>count) n=count;
    for(unsigned i=0;i<n;i++) { stereo[i*2]=.25f; stereo[i*2+1]=-.5f; }
    s->read+=n; return n;
}
static Project project,restored;
int main(void) {
    char directory[]="/tmp/libreloop-recording-XXXXXX"; CHECK(mkdtemp(directory));
    RecordingWriter *writer=recording_writer_open(directory); CHECK(writer);
    char path[PATH_MAX]; snprintf(path,sizeof path,"%s",recording_writer_path(writer));
    Source source={0}; atomic_init(&source.available,8192);
    CHECK(recording_writer_start(writer,read_audio,&source,0));
    for(int i=0;i<200 && recording_writer_frames(writer)<8192;i++) { struct timespec pause={0,5000000}; nanosleep(&pause,NULL); }
    CHECK(recording_writer_frames(writer)==8192 && !recording_writer_failed(writer));
    Sample first,second; CHECK(sample_map_recording(path,8192,&first)); CHECK(first.storage && first.data[0]==.25f && first.data[1]==-.5f);
    atomic_store(&source.available,16384); CHECK(recording_writer_finish(writer));
    CHECK(recording_writer_frames(writer)==16384); CHECK(sample_map_wav(path,&second) && second.frames==16384);
    /* Earlier views stay valid as the writer grows and closes the file. */
    CHECK(first.data[8191*2+1]==-.5f); sample_free(first);
    Sample processed; CHECK(sample_process(second,(Sampler){.time=1,.length=1},&processed));
    CHECK(processed.storage==second.storage && processed.data==second.data);
    Waveform wave={0}; CHECK(waveform_build(&wave,second)); CHECK(waveform_range(&wave,second,0,second.frames).low==-.5f); free(wave.tree);
    History history={0}; Sample samples[CHANNELS]={second},retained[CHANNELS]; uint64_t stamps[CHANNELS]={1};
    project_new(&project); CHECK(history_capture(&history,&project,samples,stamps));
    project.volume[0]=.5f; CHECK(history_capture(&history,&project,samples,stamps));
    CHECK(history_peek(&history,-1,&restored,retained) && retained[0].storage==second.storage);
    sample_free(second); CHECK(processed.data[16383*2]==.25f);
    sample_free(processed); CHECK(retained[0].data[0]==.25f); history_clear(&history);
    recording_writer_free(writer,0); unlink(path);
    writer=recording_writer_open(directory); CHECK(writer);
    snprintf(path,sizeof path,"%s",recording_writer_path(writer));
    pid_t child=fork(); CHECK(child>=0);
    if(!child) {
        signal(SIGXFSZ,SIG_IGN); struct rlimit limit={64,64};
        if(setrlimit(RLIMIT_FSIZE,&limit)) _exit(2);
        Source failing={0}; atomic_init(&failing.available,8192);
        if(!recording_writer_start(writer,read_audio,&failing,0)) _exit(3);
        int ok=recording_writer_finish(writer);
        _exit(!ok && recording_writer_failed(writer) && !recording_writer_frames(writer)?0:1);
    }
    int status; CHECK(waitpid(child,&status,0)==child && WIFEXITED(status) && !WEXITSTATUS(status));
    recording_writer_free(writer,0);
    FILE *file=fopen(path,"rb"); CHECK(file && !fseek(file,0,SEEK_END) && ftell(file)==44); fclose(file);
    unlink(path); CHECK(!rmdir(directory));
    puts("Background recording, live file mappings, final drain, shared playback and undo storage passed."); return 0;
}
