// SPDX-License-Identifier: GPL-3.0-only
/* Actual project/preset/asset I/O and WAV mapping; hardware publication is captured. */
#include <unistd.h>
#include <sys/stat.h>
/* Control only mkdtemp's random suffix; directory creation and I/O stay real. */
char *mkdtemp(char *path){size_t n=strlen(path);if(n<6)return NULL;memcpy(path+n-6,"REF001",6);return mkdir(path,0700)==0?path:NULL;}
static int publications,updates,stops,finishes,panics;
static Project canonical;
void recording_finish(void){finishes++;}
void midi_panic(void){panics++;}
int audio_stop(void){stops++;return 0;}
void audio_sample(int channel,Sample sample){(void)channel;(void)sample;publications++;}
void audio_channels(const Project *project,const Sample samples[CHANNELS]){(void)project;(void)samples;publications++;}
void audio_update(const Project *project,int run,int song,int pattern,int reset,float output,float start,float a,float b){(void)project;(void)run;(void)song;(void)pattern;(void)reset;(void)output;(void)start;(void)a;(void)b;updates++;}
int sample_load(const char *path,Sample *sample){return sample_map_wav(path,sample);}
void open_popup(int kind){ui.pattern_popup=kind;}
void project_file_action(int action){ui.file_action=action;}
static void put32(FILE *f,unsigned n){for(int i=0;i<4;i++)fputc((n>>(i*8))&255,f);}
static int make_wave(void){
    FILE *f=fopen("source.wav","wb");if(!f)return 0;
    fwrite("RIFF",4,1,f);put32(f,36+96*8);fwrite("WAVEfmt ",8,1,f);put32(f,16);
    fputc(3,f);fputc(0,f);fputc(2,f);fputc(0,f);put32(f,RATE);put32(f,RATE*8);fputc(8,f);fputc(0,f);fputc(32,f);fputc(0,f);fwrite("data",4,1,f);put32(f,96*8);
    for(int i=0;i<96;i++){float pcm[2]={(i%13-6)*.02f,(i%17-8)*.015f};fwrite(pcm,sizeof pcm,1,f);}return !fclose(f);
}
static void normalized_project(const char *file,const Project *project){
    canonical=*project;char root[4096];if(!getcwd(root,sizeof root))exit(2);size_t length=strlen(root);
    for(int c=0;c<CHANNELS;c++)if(!strncmp(canonical.paths[c],root,length) && canonical.paths[c][length]=='/'){
        char path[1024];snprintf(path,sizeof path,"/fixture%s",canonical.paths[c]+length);
        memset(canonical.paths[c],0,sizeof canonical.paths[c]);snprintf(canonical.paths[c],sizeof canonical.paths[c],"%s",path);
    }
    if(!project_save(file,&canonical))exit(2);
}
static void commit_snapshot(FILE *trace,const char *label,int result,int stage){
    char path[64];snprintf(path,sizeof path,"state-%d.llp",stage);normalized_project(path,&ui.project);
    snprintf(path,sizeof path,"saved-%d.llp",stage);normalized_project(path,&ui.document.saved);
    fprintf(trace,"%s %d %d %d %d %d %d %d %d %d %d %u %.9g %d %d %d %d %s\n",label,result,project_dirty(),ui.document.saved_on_disk,ui.document.pending_action,ui.pattern_popup,ui.file_confirm,ui.pattern,ui.channel,ui.song,ui.reset,ui.sample_epoch,ui.project.audio_seconds[0],publications,updates,stops,ui.edit_history.count,ui.status);
    FILE *pcm=fopen(stage?"pcm-after.bin":"pcm-before.bin","wb");if(!pcm)exit(3);
    for(int c=0;c<2;c++){
        unsigned frames[]={ui.originals[c].frames,ui.originals[c].channels,ui.samples[c].frames,ui.samples[c].channels,ui.sample_generation[c],ui.sampler_generation[c]};
        fwrite(frames,sizeof frames,1,pcm);
        if(ui.originals[c].frames)fwrite(ui.originals[c].data,sizeof(float),(size_t)ui.originals[c].frames*ui.originals[c].channels,pcm);
        if(ui.samples[c].frames)fwrite(ui.samples[c].data,sizeof(float),(size_t)ui.samples[c].frames*ui.samples[c].channels,pcm);
    }
    fclose(pcm);
}
static int project_commit_sequence(int mode){
    if(!make_wave())return 1;memset(&ui,0,sizeof ui);project_new(&ui.project);ui.project.insert_count=INSERTS; /* Frozen reference project geometry. */
    strcpy(ui.project.paths[0],"source.wav");strcpy(ui.project.channel_names[0],"Keep channel name");
    /* Preset callers normally receive the canonical path produced by import. */
    if(mode==8 || mode==9){char root[4096];if(!getcwd(root,sizeof root))return 1;snprintf(ui.project.paths[0],sizeof ui.project.paths[0],"%s/source.wav",root);}
    ui.project.sampler[0].flags=SAMPLE_REVERSE;ui.project.sampler[0].start=.125f;ui.project.sampler[0].length=.75f;
    ui.project.notes[0][0][0]=(Note){60,96,0,2};ui.project.clips[0][0]=1;ui.project.clip_steps[0][0]=4;
    if(!sample_map_wav("source.wav",&ui.originals[0]) || !sample_process(ui.originals[0],ui.project.sampler[0],&ui.samples[0]))return 2;
    for(int c=0;c<CHANNELS;c++)ui.sampler_applied[c]=ui.project.sampler[c];
    sample_duration(&ui.project,0,ui.samples[0],ui.originals[0]);project_document_saved(&ui.document,&ui.project,NULL);history_checkpoint();
    FILE *trace=fopen("trace.txt","wb");if(!trace)return 3;commit_snapshot(trace,"before",1,0);
    int result=0;
    if(mode==0 || mode==1){ui.file_action=mode?6:3;result=project_file_commit("written.llp");}
    if(mode==2){ui.file_action=5;result=project_file_commit("render.wav");}
    if(mode==3 || mode==4){
        if(!project_save_assets("input.llp",&ui.project,ui.originals,0))return 4;
        if(mode==4)remove("source.wav");ui.file_action=4;result=project_file_commit("input.llp");
    }
    if(mode==5){FILE *bad=fopen("bad.llp","wb");if(!bad)return 4;fputs("invalid",bad);fclose(bad);ui.file_action=4;result=project_file_commit("bad.llp");}
    if(mode==6){ui.file_action=3;result=project_file_commit("missing/written.llp");}
    if(mode==7){ui.file_action=7;ui.relink_channel=0;result=project_file_commit("source.wav");}
    if(mode==8 || mode==9){
        ui.preset_kind=PRESET_SAMPLER;ui.preset_owner=0;ui.file_action=8;result=project_file_commit("device.llpreset");
        if(mode==9)remove("source.wav");ui.project.sampler[0].flags=0;ui.file_action=9;result=project_file_commit("device.llpreset");
    }
    if(mode==10){ui.file_action=3;strcpy(ui.file_pending,"written.llp");ui.document.pending_action=REPLACE_QUIT;file_picker_submit_direct();result=ui.document.quit;}
    if(mode==11){ui.file_action=3;strcpy(ui.file_pending,"missing/written.llp");ui.document.pending_action=REPLACE_QUIT;file_picker_submit_direct();result=ui.document.quit;}
    commit_snapshot(trace,"after",result,1);fprintf(trace,"services %d %d %d\n",finishes,panics,ui.document.quit);fclose(trace);
    history_clear(&ui.edit_history);for(int c=0;c<CHANNELS;c++){sample_free(ui.samples[c]);sample_free(ui.originals[c]);}
    return 0;
}
