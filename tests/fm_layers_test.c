// SPDX-License-Identifier: GPL-3.0-only
#include "engine.h"
#include "preset.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#define CHECK(x) do { if(!(x)) { fprintf(stderr,"Failed line %d: %s\n",__LINE__,#x); return 1; } } while(0)
static Project project,loaded;
static int legacy_project(const char *source,const char *destination) {
    FILE *f=fopen(source,"r"); if(!f) return 0;
    char line[4096]; int count=0;
    while(fgets(line,sizeof line,f)) count++;
    int extra=count-(INSERTS+1)*EFFECT_SLOTS*EQ_BANDS-CHANNELS;
    rewind(f); FILE *out=fopen(destination,"w"); if(!out) { fclose(f); return 0; }
    int index=0;
    while(fgets(line,sizeof line,f)) {
        if(index==0) fputs("HOMEBEAT 37\n",out);
        else if(index<extra || index>=extra+CHANNELS) fputs(line,out);
        index++;
    }
    fclose(f); return !fclose(out);
}
int main(void) {
    FMSettings bright=fm_default(); CHECK(fm_valid(bright) && bright.attack_depth>0 && bright.mod_decay>bright.attack_decay);
    FMVoice strong,soft; FMSettings body=bright; body.attack_depth=0;
    fm_note_on(&strong,220,bright); fm_note_on(&soft,220,body);
    double high_strong=0,high_soft=0,tail_difference=0; float previous_strong=0,previous_soft=0;
    for(int i=0;i<RATE+4096;i++) {
        float a=fm_sample(&strong,bright,1),b=fm_sample(&soft,body,1);
        CHECK(isfinite(a) && fabsf(a)<=.201f && isfinite(b));
        if(i>100 && i<RATE/20) { high_strong+=(a-previous_strong)*(a-previous_strong); high_soft+=(b-previous_soft)*(b-previous_soft); }
        if(i>RATE) tail_difference+=fabsf(a-b);
        previous_strong=a; previous_soft=b;
    }
    CHECK(high_strong>high_soft*2 && tail_difference<.001);
    CHECK(strong.mod_envelope>.12f && strong.attack_envelope<.000001f);
    FMSettings tuning=fm_legacy(); tuning.depth=0; tuning.carrier_ratio=2; tuning.ratio=1; tuning.body_pitch=12; tuning.attack_ratio=5;
    fm_note_on(&strong,220,tuning); fm_sample(&strong,tuning,1);
    CHECK(fabs(strong.carrier-6.283185307179586*440/RATE)<1e-6);
    CHECK(fabs(strong.modulator-6.283185307179586*440/RATE)<1e-6);
    CHECK(fabs(strong.attack_modulator-6.283185307179586*1100/RATE)<1e-6);
    tuning.carrier_detune=100; fm_note_on(&strong,220,tuning); fm_sample(&strong,tuning,1);
    CHECK(fabs(strong.carrier-6.283185307179586*440*pow(2,1./12)/RATE)<1e-6);
    CHECK(fabs(strong.modulator-6.283185307179586*440/RATE)<1e-6);
    for(int mode=0;mode<2;mode++) for(int shape=0;shape<4;shape++) {
        bright.routing=mode; bright.lfo_shape=shape; bright.vibrato=100; bright.tremolo=1;
        fm_note_on(&strong,15000,bright);
        for(int i=0;i<4000;i++) CHECK(isfinite(fm_sample(&strong,bright,16)));
    }
    CHECK(fm_lfo_value(.25f,0)>.99f && fm_lfo_value(.5f,1)==1 && fm_lfo_value(.25f,2)==-.5f && fm_lfo_value(.75f,3)==-1);
    bright=fm_default(); bright.mod_release=.01f; bright.release=1;
    fm_note_on(&strong,220,bright); for(int i=0;i<1000;i++) fm_sample(&strong,bright,1);
    fm_note_off(&strong,bright); for(int i=0;i<1000;i++) fm_sample(&strong,bright,1);
    CHECK(!strong.mod_envelope && strong.envelope>0);
    project_new(&project); project.instrument[0]=INSTRUMENT_FM; project.fm[0]=fm_default();
    project.fm[0].carrier_ratio=2.5f; project.fm[0].body_pitch=-12; project.fm[0].attack_detune=23;
    project.fm[0].routing=1; project.fm[0].lfo_shape=2;
    for(unsigned id=PARAM_FM_RATIO;id<=PARAM_FM_LAST;id++) {
        ParameterTarget target; const float *pointer=fm_parameter_pointer(&project.fm[0],id);
        CHECK(pointer && parameter_descriptor(id) && parameter_from_pointer(&project,pointer,&target) && target.parameter==id);
        float value,low,high; CHECK(parameter_info(&project,target,&value,&low,&high));
    }
    CHECK(project_save("fm-layers.hbt",&project) && project_load("fm-layers.hbt",&loaded));
    CHECK(!memcmp(&project.fm[0],&loaded.fm[0],sizeof(FMSettings)));
    CHECK(legacy_project("fm-layers.hbt","fm-legacy.hbt") && project_load("fm-legacy.hbt",&loaded));
    CHECK(loaded.fm[0].carrier_ratio==1 && loaded.fm[0].attack_depth==0 && loaded.fm[0].mod_attack==0 && loaded.fm[0].body_pitch==0 && loaded.fm[0].lfo_fade==0);
    for(unsigned id=PARAM_FM_RATIO;id<=PARAM_FM_TREMOLO;id++) CHECK(*fm_parameter_pointer(&project.fm[0],id)==*fm_parameter_pointer(&loaded.fm[0],id));
    DevicePreset preset={.kind=PRESET_FM,.fm=project.fm[0]},restored;
    CHECK(preset_save("fm-layers.llpreset",&preset) && preset_load("fm-layers.llpreset",&restored));
    CHECK(!memcmp(&preset.fm,&restored.fm,sizeof(FMSettings)));
    FILE *file=fopen("fm-old.llpreset","w"); CHECK(file);
    fputs("LIBRELOOP_PRESET 3 2\n1 3.8 .002 2.5 .08 .45\n.85 .08 .8 4.8 3 .12\n",file); fclose(file);
    CHECK(preset_load("fm-old.llpreset",&restored) && restored.fm.attack_depth==0 && restored.fm.carrier_ratio==1 && restored.fm.mod_attack==0);
    int a=automation_create(&project,(ParameterTarget){PARAM_FM_ATTACK_DEPTH,0,0},"Strike",16); CHECK(a>=0);
    project.automations[a].points[0].value=project.automations[a].points[1].value=0;
    project.clips[0][0]=AUTOMATION_SOURCE+a+1; project.clips[1][0]=1; project.notes[0][0][0]=(Note){60,127,0,16};
    Player automated,manual; player_reset(&automated); player_reset(&manual); automated.song=manual.song=1;
    loaded=project; loaded.automation_count=0; loaded.clips[0][0]=0; loaded.fm[0].attack_depth=0;
    Sample samples[CHANNELS]={0}; float x[1024],y[1024];
    render(&automated,&project,samples,x,512); render(&manual,&loaded,samples,y,512);
    for(int i=0;i<1024;i++) CHECK(fabsf(x[i]-y[i])<.00001f);
    remove("fm-layers.hbt"); remove("fm-legacy.hbt"); remove("fm-layers.llpreset"); remove("fm-old.llpreset");
    puts("Bright independent attack/body, tuning, release, LFO shapes, routing, legacy files and new automation passed."); return 0;
}
