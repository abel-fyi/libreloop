// SPDX-License-Identifier: GPL-3.0-only
#include "project_check.h"
#include "project_format.h"
#include <math.h>
#include <float.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#define CHECK(x) do { if(!(x)) { fprintf(stderr,"Failed line %d: %s\n",__LINE__,#x); return 1; } } while(0)
static Project original,loaded,sentinel;
static char path[]="/tmp/libreloop-format-XXXXXX";
static char *read_file(void) {
    FILE *f=fopen(path,"rb"); if(!f) return NULL;
    if(fseek(f,0,SEEK_END)) { fclose(f); return NULL; }
    long length=ftell(f); rewind(f); if(length<0) { fclose(f); return NULL; }
    char *s=malloc((size_t)length+1); if(!s) { fclose(f); return NULL; }
    if(fread(s,1,(size_t)length,f)!=(size_t)length) { free(s); fclose(f); return NULL; }
    s[length]=0; fclose(f); return s;
}
static int write_file(const char *s,size_t n) {
    FILE *f=fopen(path,"wb"); if(!f) return 0;
    int ok=fwrite(s,1,n,f)==n; return fclose(f)==0 && ok;
}
static int replaced(const char *saved,const char *from,const char *to,int valid) {
    const char *match=strstr(saved,from); if(!match) return 0;
    size_t prefix=(size_t)(match-saved),a=strlen(from),b=strlen(to),length=strlen(saved);
    char *copy=malloc(length-a+b+1); if(!copy) return 0;
    memcpy(copy,saved,prefix); memcpy(copy+prefix,to,b); strcpy(copy+prefix+b,match+a);
    int ok=write_file(copy,length-a+b); free(copy);
    loaded=sentinel;
    if(!ok || project_load(path,&loaded)!=valid) return 0;
    return project_equal(&loaded,valid?&original:&sentinel);
}
int main(void) {
    int fd=mkstemp(path); CHECK(fd>=0); close(fd);
    project_new(&original); project_new(&sentinel); sentinel.bpm=211;
    original.bpm=123.456789f; original.pan[0]=-0.0f;
    snprintf(original.paths[0],sizeof original.paths[0],"samples/é piano: \"bright\".wav");
    snprintf(original.channel_names[0],PATTERN_NAME,"é piano: \"bright\"");
    original.pattern_count=2; original.pattern_steps[1]=48.5f;
    CHECK(note_add(&original,1,0,4.f/3,64,2.f/3));
    original.clips[0][0]=2; original.clip_starts[0][0]=.125f;
    original.clip_steps[0][0]=12.75f; original.clip_offsets[0][0]=1.f/3;
    for(int c=0;c<3;c++) {
        original.instrument[c]=INSTRUMENT_FM; original.fm[c]=fm_epiano(); original.fm[c].engine=c;
        original.fm[c].carrier_ratio=2.5f; original.fm[c].dx7.value[0]=89;
        original.fm[c].analog.cutoff=4321.5f;
    }
    original.effect_type[0][0]=EFFECT_CHORUS; original.chorus[0][0].depth=3;
    original.effect_type[3][2]=EFFECT_EQ; original.eq[3][2].bands[6].gain=7.5f;
    original.eq[3][2].bands[6].shape=EQ_HIGH_CUT; original.effect_mix[3][2]=.375f;
    CHECK(automation_create(&original,(ParameterTarget){PARAM_FM_ATTACK_DEPTH,0,0},"Strike",16)>=0);
    original.midi_binding_count=1; original.midi_bindings[0]=(MidiBinding){2,10,{PARAM_CHANNEL_VOLUME,0,0}};
    original.automations[AUTOMATIONS-1].points[AUTOMATION_POINTS-1]=(AutomationPoint){FLT_TRUE_MIN,FLT_MAX};
    CHECK(project_save(path,&original)); char *saved=read_file(); CHECK(saved);
    CHECK(strstr(saved,"LIBRELOOP_PROJECT 1\n") && strstr(saved,"bpm f32 1\n") && strlen(saved)<200000);
    CHECK(project_load(path,&loaded) && project_equal(&original,&loaded));
    CHECK(loaded.automations[AUTOMATIONS-1].points[AUTOMATION_POINTS-1].step==FLT_TRUE_MIN);
    CHECK(loaded.automations[AUTOMATIONS-1].points[AUTOMATION_POINTS-1].value==FLT_MAX);
    /* Same musical state yields the same samples, including effects and FM engines. */
    Sample samples[CHANNELS]={0}; Player *a=calloc(1,sizeof *a),*b=calloc(1,sizeof *b); CHECK(a && b);
    player_reset(a); player_reset(b); a->song=b->song=1;
    a->effects=effects_create(INSERTS+1); b->effects=effects_create(INSERTS+1); CHECK(a->effects && b->effects);
    float x[1024],y[1024]; double power=0;
    for(int n=0;n<200;n++) {
        render(a,&original,samples,x,512); render(b,&loaded,samples,y,512);
        CHECK(!memcmp(x,y,sizeof x)); for(unsigned i=0;i<1024;i++) power+=x[i]*x[i];
    }
    CHECK(power>0); effects_free(a->effects); effects_free(b->effects); free(a); free(b);
    /* Reorder complete fields without altering meaning. */
    const char *first=strstr(saved,"bpm f32"),*second=strstr(saved,"master f32"),*third=strstr(saved,"master_pitch f32");
    CHECK(first && second && third); size_t n=strlen(saved); char *reordered=malloc(n+1); CHECK(reordered);
    size_t header=first-saved,left=second-first,right=third-second;
    memcpy(reordered,saved,header); memcpy(reordered+header,second,right);
    memcpy(reordered+header+right,first,left); strcpy(reordered+header+right+left,third);
    CHECK(write_file(reordered,n) && project_load(path,&loaded) && project_equal(&original,&loaded)); free(reordered);
    CHECK(replaced(saved,"end\n","x-comment str 1\n1 5:hello\nend\n",1));
    CHECK(replaced(saved,"end\n","future_instrument u8 1\n1 0\nend\n",0));
    CHECK(replaced(saved,"end\n","bpm f32 1\n1 120\nend\n",0));
    CHECK(replaced(saved,"end\n","x-comment str 1\n1 1:a\nx-comment str 1\n1 1:b\nend\n",0));
    CHECK(replaced(saved,"end\n","",0));
    CHECK(replaced(saved,"end\n","end\ntrailing data\n",0));
    CHECK(replaced(saved,"LIBRELOOP_PROJECT 1","LIBRELOOP_PROJECT 2",0));
    CHECK(replaced(saved,"LIBRELOOP_PROJECT 1","LIBRELOOP_PROJECT 99999999999999999999999999999999",0));
    CHECK(replaced(saved,"bpm f32 1","bpm f32 99999999999999999999999999999",0));
    CHECK(replaced(saved,"bpm f32 1","bpm u32 1",0));
    CHECK(replaced(saved,"bpm f32 1","bpm f32 2",0));
    CHECK(replaced(saved,"bpm f32 1","x-bpm f32 1",0));
    CHECK(replaced(saved,"master f32 1\n1 1\n","master f32 1\n0 1\n",0));
    CHECK(replaced(saved,"master f32 1\n1 1\n","master f32 1\n2 1\n",0));
    const char *invalid[]={"nan","inf","-inf","1e9999","1e-9999","3","-1","1 garbage"};
    for(size_t i=0;i<sizeof invalid/sizeof *invalid;i++) {
        char text[128]; snprintf(text,sizeof text,"master f32 1\n1 %s\n",invalid[i]);
        CHECK(replaced(saved,"master f32 1\n1 1\n",text,0));
    }
    CHECK(replaced(saved,"master_mute u8 1\n1 0\n","master_mute u8 1\n1 256\n",0));
    CHECK(replaced(saved,"master_mute u8 1\n1 0\n","master_mute u8 1\n1 -1\n",0));
    CHECK(replaced(saved,"end\n","x-comment str 1\n1 999999999999999999999:bad\nend\n",0));
    CHECK(replaced(saved,"end\n","x-comment str 1\n1 7:short\nend\n",0));
    CHECK(replaced(saved,"insert_output u8 100\n100 0\n","insert_output u8 100\n1 1\n99 0\n",0));
    CHECK(write_file(saved,n-1) && !project_load(path,&loaded));
    char *nul=malloc(n); CHECK(nul); memcpy(nul,saved,n); nul[5]=0;
    loaded=sentinel; CHECK(write_file(nul,n) && !project_load(path,&loaded) && project_equal(&sentinel,&loaded)); free(nul);
    CHECK(write_file(saved,n)); original.master=NAN; CHECK(!project_save(path,&original));
    char *unchanged=read_file(); CHECK(unchanged && !strcmp(unchanged,saved)); free(unchanged); original.master=1;
    memset(original.paths[0],'x',sizeof original.paths[0]); CHECK(!project_save(path,&original));
    CHECK(write_file("HOMEBEAT 41\n",12) && !project_load(path,&loaded));
    printf("Named LLP fields, full round-trip/PCM equality, malformed input and atomic failure passed (%zu bytes).\n",n);
    free(saved); unlink(path); return 0;
}
