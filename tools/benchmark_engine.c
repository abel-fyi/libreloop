// SPDX-License-Identifier: GPL-3.0-only
/* Offline engine measurements: one scenario/process keeps memory figures isolated. */
#include "engine.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <sys/resource.h>
static Project project;
static Player player;
static Sample samples[CHANNELS];
static volatile float checksum;
static double seconds(clockid_t clock) { struct timespec t; clock_gettime(clock,&t); return t.tv_sec+t.tv_nsec/1e9; }
static int compare(const void *a,const void *b) { double x=*(const double *)a,y=*(const double *)b; return (x>y)-(x<y); }
static double memory(const char *field) {
#ifdef __linux__
    FILE *file=fopen("/proc/self/smaps_rollup","r"); if(!file) return -1;
    char line[256],name[64]; double kb,value=-1;
    while(fgets(line,sizeof line,file)) if(sscanf(line,"%63s %lf",name,&kb)==2 && !strcmp(name,field)) { value=kb/1024; break; }
    fclose(file); return value;
#else
    (void)field; return -1;
#endif
}
static void monitor(void *context,int bus,float left,float right) { (void)context; (void)bus; checksum+=left*.000001f+right*.000002f; }
int main(int argc,char **argv) {
    if(argc<2) { fprintf(stderr,"Usage: benchmark_engine SCENARIO [audio_seconds=10] [block_frames=512]\n"); return 1; }
    const char *name=argv[1]; double duration=argc>2?atof(argv[2]):10; unsigned block=argc>3?atoi(argv[3]):512;
    if(!isfinite(duration) || duration<=0 || duration>25 || block<32 || block>4096) return 1;
    const char *scenarios[]={"idle_no_rack","idle_rack","idle_monitor","demo_song","sampler_32","sampler_128","song_100","sampler_32_stretch","sampler_32_chorus","sampler_32_eq","fm_8","fm_32","fm_128","fm_legacy_32","fm_32_motion"};
    int known=0;
    for(unsigned i=0;i<sizeof scenarios/sizeof *scenarios;i++) if(!strcmp(name,scenarios[i])) known=1;
    if(!known) { fprintf(stderr,"Unknown scenario\n"); return 1; }
    int verify=argc>4 && !strcmp(argv[4],"verify"); uint64_t hash=1469598103934665603ULL;
    int voices=0,sequence=0,monitoring=!strcmp(name,"idle_monitor");
    project_new(&project); player_reset(&player);
    if(strcmp(name,"idle_no_rack")) { player.effects=effects_create(INSERTS+1); if(!player.effects) return 2; }
    if(!strncmp(name,"sampler",7)) voices=strstr(name,"128")?128:32;
    else if(!strncmp(name,"fm",2)) voices=strstr(name,"128")?128:strstr(name,"32")?32:8;
    else if(!strcmp(name,"song_100")) { voices=100; sequence=1; player.song=1; }
    else if(!strcmp(name,"demo_song")) { project_demo(&project); sequence=1; player.song=1; samples_default(samples); }
    else if(strncmp(name,"idle",4)) { fprintf(stderr,"Unknown scenario\n"); return 1; }
    float *pcm=NULL;
    if(voices && strncmp(name,"fm",2)) {
        unsigned frames=(unsigned)ceil(fmax(30,(duration+128.*block/RATE+1)*1.2)*RATE); pcm=malloc((size_t)frames*2*sizeof(float)); if(!pcm) return 2;
        for(unsigned i=0;i<frames;i++) { pcm[i*2]=.01f*sinf(i*.037f); pcm[i*2+1]=.01f*cosf(i*.041f); }
        for(int c=0;c<CHANNELS;c++) samples[c]=(Sample){.data=pcm,.frames=frames,.channels=2};
    }
    if(voices) {
        project.channel_count=32;
        for(int c=0;c<32;c++) { project.volume[c]=.02f; project.route[c]=0; }
        if(!strncmp(name,"fm",2)) {
            FMSettings s=strstr(name,"legacy")?fm_legacy():fm_default();
            if(strstr(name,"motion")) { s.vibrato=20; s.tremolo=.3f; }
            for(int c=0;c<32;c++) { project.instrument[c]=INSTRUMENT_FM; project.fm[c]=s; }
            for(int v=0;v<voices;v++) {
                player.voices[v]=(Voice){.channel=v%32,.instrument=INSTRUMENT_FM,.remaining=-1,.gain=.1f,.lane=-1};
                fm_note_on(&player.voices[v].fm,220*pow(2,(v%12)/12.),s);
            }
        } else if(!sequence) for(int v=0;v<voices;v++) {
            player.voices[v]=(Voice){.channel=v%32,.remaining=-1,.gain=.1f,.lane=-1}; sampler_voice_reset(&player.voices[v].sampler,0,1);
        } else {
            for(int l=0;l<100;l++) { int c=l%32; project.channel_audio[c]=1; project.audio_seconds[c]=samples[c].frames/(float)RATE; project.clips[l][0]=PATTERNS+c+1; }
        }
        if(strstr(name,"stretch")) for(int c=0;c<32;c++) { project.sampler[c].fit_bpm=100; project.sampler[c].stretch=1; }
        if(strstr(name,"chorus")) { project.effect_type[0][0]=EFFECT_CHORUS; project.effect_mix[0][0]=.4f; }
        if(strstr(name,"eq")) { project.effect_type[0][0]=EFFECT_EQ; project.effect_mix[0][0]=1; project.eq[0][0].bands[2].gain=3; }
    }
    MixerIO taps={.output=monitor,.monitor_only=1}; float *out=calloc(block*2,sizeof(float));
    unsigned loops=(unsigned)ceil(duration*RATE/block); double *wall=malloc(loops*sizeof *wall); if(!out || !wall) return 2;
    float peaks[INSERTS+1][2];
    for(int i=0;i<128;i++) { memset(out,0,block*2*sizeof(float)); render_mixer_io(&player,NULL,&project,samples,out,block,sequence,peaks,monitoring?&taps:NULL); }
    double start_wall=seconds(CLOCK_MONOTONIC),start_cpu=seconds(CLOCK_PROCESS_CPUTIME_ID),maximum=0; unsigned misses=0;
    for(unsigned i=0;i<loops;i++) {
        memset(out,0,block*2*sizeof(float)); double start=seconds(CLOCK_MONOTONIC);
        render_mixer_io(&player,NULL,&project,samples,out,block,sequence,peaks,monitoring?&taps:NULL);
        wall[i]=seconds(CLOCK_MONOTONIC)-start; if(wall[i]>maximum) maximum=wall[i]; if(wall[i]>block/(double)RATE) misses++;
        checksum+=out[i%(block*2)];
        if(verify) for(unsigned byte=0;byte<block*2*sizeof(float);byte++) { hash^=((unsigned char *)out)[byte]; hash*=1099511628211ULL; }
    }
    double cpu=seconds(CLOCK_PROCESS_CPUTIME_ID)-start_cpu,elapsed=seconds(CLOCK_MONOTONIC)-start_wall;
    qsort(wall,loops,sizeof *wall,compare);
    struct rusage usage; getrusage(RUSAGE_SELF,&usage);
#ifdef __APPLE__
    double peak_rss=usage.ru_maxrss/(1024.*1024);
#else
    double peak_rss=usage.ru_maxrss/1024.;
#endif
    char hash_result[32]="null";
    if(verify) snprintf(hash_result,sizeof hash_result,"\"%016llx\"",(unsigned long long)hash);
    printf("{\"scenario\":\"%s\",\"block_frames\":%u,\"audio_seconds\":%.3f,\"cpu_percent\":%.4f,\"wall_seconds\":%.6f,\"block_p50_ms\":%.6f,\"block_p99_ms\":%.6f,\"block_max_ms\":%.6f,\"deadline_ms\":%.6f,\"deadline_misses\":%u,\"rss_mib\":%.3f,\"pss_mib\":%.3f,\"peak_rss_mib\":%.3f,\"pcm_hash\":%s}\n",
        name,block,loops*block/(double)RATE,cpu/(loops*block/(double)RATE)*100,elapsed,wall[loops/2]*1000,wall[(unsigned)((loops-1)*.99)]*1000,maximum*1000,block*1000./RATE,misses,memory("Rss:"),memory("Pss:"),peak_rss,hash_result);
    effects_free(player.effects); free(out); free(wall);
    if(pcm) free(pcm); else for(int c=0;c<CHANNELS;c++) sample_free(samples[c]);
    return 0;
}
