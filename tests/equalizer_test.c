// SPDX-License-Identifier: GPL-3.0-only
#include "engine.h"
#include "preset.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#define CHECK(x) do { if(!(x)) { fprintf(stderr,"Failed %d: %s\n",__LINE__,#x); return 1; } } while(0)
static double amplitude(EQSettings s,int side,float frequency) {
    Equalizer eq={0}; double power=0;
    for(int n=0;n<48000;n++) {
        float x=.1f*sinf(6.2831853f*frequency*n/48000),v[2]={side==0?x:0,side==1?x:0};
        equalizer_process(&eq,s,1,v); if(n>=24000) power+=v[side]*v[side];
        if(v[1-side]!=0 || !isfinite(v[side])) return -1;
    }
    return sqrt(power/24000)*sqrt(2)/.1;
}
int main(void) {
    EQSettings s=equalizer_default(); CHECK(equalizer_valid(s));
    CHECK(fabs(amplitude(s,0,1000)-1)<.001);
    CHECK(fabs(equalizer_response(s,1000))<.001);
    s.bands[1]=(EQBand){1000,6,1};
    CHECK(fabs(amplitude(s,0,1000)-pow(10,.3))<.01);
    CHECK(fabs(equalizer_response(s,1000)-6)<.01);
    s.bands[1].gain=-12; CHECK(fabs(amplitude(s,1,1000)-pow(10,-.6))<.01);
    s=equalizer_default(); s.bands[0].gain=6; s.bands[0].frequency=100; CHECK(amplitude(s,0,30)>1.9 && amplitude(s,0,5000)<1.01);
    s=equalizer_default(); s.bands[6].gain=-6; s.bands[6].frequency=10000; CHECK(amplitude(s,1,18000)<.53 && amplitude(s,1,100)<1.01);
    s=equalizer_default(); s.bands[3]=(EQBand){1000,0,.707f,EQ_LOW_CUT};
    CHECK(amplitude(s,0,100)<.011 && amplitude(s,1,10000)>.99);
    CHECK(equalizer_response(s,100)<-39 && fabsf(equalizer_response(s,1000)+3.01f)<.02);
    s.bands[3].shape=EQ_HIGH_CUT; CHECK(amplitude(s,1,100)>.99 && amplitude(s,0,10000)<.011);
    s.bands[3].shape=EQ_OFF; s.bands[3].gain=18; CHECK(fabs(amplitude(s,0,1000)-1)<.001);
    s.bands[3].shape=EQ_HIGH_SHELF; CHECK(amplitude(s,0,10000)>7.8);
    s=equalizer_default(); Equalizer switcher={0}; float previous=.1f;
    for(int n=0;n<24000;n++) {
        if(n==4000) s.bands[3].shape=EQ_LOW_CUT;
        if(n==10000) s.bands[3].shape=EQ_HIGH_CUT;
        if(n==16000) s.bands[3].shape=EQ_OFF;
        float v[2]={.1f,.1f}; equalizer_process(&switcher,s,1,v);
        CHECK(isfinite(v[0]) && fabsf(v[0]-previous)<.005f); previous=v[0];
    }
    CHECK(fabsf(previous-.1f)<.0001f);
    Equalizer eq={0}; s=equalizer_default(); s.bands[1].gain=18;
    float last=0,jump=0;
    for(int n=0;n<96000;n++) {
        if(n==24000) { s.bands[1].gain=-18; s.bands[1].frequency=20; s.bands[1].q=10; }
        float v[2]={.1f,.1f}; equalizer_process(&eq,s,n<48000?1:0,v);
        CHECK(isfinite(v[0]) && fabsf(v[0])<3); if(n>10000) jump=fmaxf(jump,fabsf(v[0]-last)); last=v[0];
        if(n>90000) CHECK(fabsf(v[0]-.1f)<.0001f);
    }
    CHECK(jump<.02);
    static Project p,loaded; project_default(&p); p.effect_type[1][0]=EFFECT_EQ; p.eq[1][0].bands[2].gain=5; p.eq[1][0].bands[5].shape=EQ_LOW_CUT;
    ParameterTarget target; CHECK(parameter_from_pointer(&p,&p.eq[1][0].bands[2].gain,&target) && target.parameter==PARAM_EQ_FIRST+7);
    CHECK(automation_create(&p,target,"EQ",16)>=0);
    char path[]="/tmp/libreloop-eq-XXXXXX"; int fd=mkstemp(path); CHECK(fd>=0); close(fd);
    CHECK(project_save(path,&p) && project_load(path,&loaded)); CHECK(loaded.effect_type[1][0]==EFFECT_EQ && loaded.eq[1][0].bands[2].gain==5 && loaded.eq[1][0].bands[5].shape==EQ_LOW_CUT);
    DevicePreset preset={.kind=PRESET_EQ,.eq=p.eq[1][0],.mix=.75f},read={0}; CHECK(preset_save(path,&preset) && preset_load(path,&read)); CHECK(read.kind==PRESET_EQ && read.eq.bands[2].gain==5 && read.mix==.75f && read.eq.bands[5].shape==EQ_LOW_CUT);
    FILE *f=fopen(path,"w"); CHECK(f); fputs("LIBRELOOP_PRESET 1 4\n100 3 .707\n500 6 1\n2500 -2 1\n10000 4 .707\n.75\n",f); fclose(f);
    CHECK(preset_load(path,&read) && read.eq.bands[3].shape==EQ_HIGH_SHELF && read.eq.bands[3].gain==4);
    for(int i=4;i<EQ_BANDS;i++) CHECK(read.eq.bands[i].shape==EQ_OFF);
    static float source[48000],output[48000*2];
    for(int n=0;n<48000;n++) source[n]=.1f*sinf(6.2831853f*1000*n/48000);
    Sample samples[CHANNELS]={{source,48000,1}};
    project_default(&p); memset(p.notes,0,sizeof p.notes); memset(p.clips,0,sizeof p.clips);
    p.channel_count=1; p.channel_audio[0]=1; p.audio_seconds[0]=1; p.route[0]=1;
    p.clips[0][0]=PATTERNS+1; p.effect_type[1][0]=EFFECT_EQ;
    p.eq[1][0].bands[1].frequency=1000;
    target=(ParameterTarget){PARAM_EQ_FIRST+4,1,0}; int a=automation_create(&p,target,"Gain",16); CHECK(a>=0);
    p.automations[a].points[0].value=p.automations[a].points[1].value=2.f/3;
    p.clips[1][0]=AUTOMATION_SOURCE+a+1;
    Player player; player_reset(&player); player.song=1; player.effects=effects_create(INSERTS+1); CHECK(player.effects);
    render(&player,&p,samples,output,48000);
    double power=0; for(int n=24000;n<36000;n++) { CHECK(isfinite(output[n*2])); power+=output[n*2]*output[n*2]; }
    CHECK(fabs(sqrt(power/12000)*sqrt(2)/.1-pow(10,.3))<.02);
    CHECK(p.eq[1][0].bands[1].gain==0); effects_free(player.effects);
    s.bands[0].q=NAN; CHECK(!equalizer_valid(s));
    insert_reset(&p,1); CHECK(p.effect_type[1][0]==0 && p.automation_count==0);
    unlink(path); puts("EQ response, stereo isolation, smoothing, bypass, project/preset persistence and bindings passed."); return 0;
}
