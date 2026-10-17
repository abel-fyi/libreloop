// SPDX-License-Identifier: GPL-3.0-only
#include "engine.h"
#include "preset.h"
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>
#define CHECK(x) do { if(!(x)) { fprintf(stderr,"Failed %d: %s\n",__LINE__,#x); return 1; } } while(0)
static Project project,loaded;
int main(void) {
    dx7_init();
    /* Independent Dexed/MSFA Modern-engine renders, note 60/velocity 90,
       48 kHz, release at sample 24000, one second; Apache-2.0 reference. */
    const uint64_t reference[]={15330861422046978487ULL,15006292893180862588ULL,3371117517242455383ULL,11790427294620480679ULL};
    for(int preset=0;preset<4;preset++) {
        DX7Settings s=dx7_factory(preset); CHECK(dx7_valid(s)); DX7Voice voice;
        dx7_note_on(&voice,261.6255653005986,90/127.f,s); uint64_t hash=1469598103934665603ULL;
        for(int i=0;i<RATE;i++) {
            if(i==RATE/2) dx7_note_off(&voice);
            float sample=dx7_sample(&voice,&s,1); CHECK(isfinite(sample) && fabsf(sample)<2);
            uint32_t bits; memcpy(&bits,&sample,4); for(int b=0;b<4;b++) { hash^=(bits>>(b*8))&255; hash*=1099511628211ULL; }
        }
        CHECK(hash==reference[preset]);
    }
    DX7Settings simple=dx7_factory(0); simple.value[134]=31; simple.value[135]=0;
    for(int op=0;op<6;op++) {
        int b=op*21; simple.value[b+16]=op==5?99:0;
        for(int i=0;i<4;i++) { simple.value[b+i]=99; simple.value[b+4+i]=i==3?0:99; }
        simple.value[b+9]=simple.value[b+10]=simple.value[b+13]=simple.value[b+15]=0;
        simple.value[b+17]=0; simple.value[b+18]=1; simple.value[b+19]=0; simple.value[b+20]=7;
    }
    DX7Voice voice; dx7_note_on(&voice,440,1,simple); int crossings=0; float previous=0;
    for(int i=0;i<RATE;i++) { float x=dx7_sample(&voice,&simple,1); if(i>RATE/2 && previous<=0 && x>0) crossings++; previous=x; }
    CHECK(abs(crossings-220)<=1);
    simple.value[5*21+16]=50; float peak=0;
    for(int i=0;i<512;i++) peak=fmaxf(peak,fabsf(dx7_sample(&voice,&simple,1)));
    CHECK(peak<.17f); /* Live output edits affect held notes. */
    dx7_note_off(&voice); for(int i=0;i<1024;i++) dx7_sample(&voice,&simple,1); CHECK(!dx7_active(&voice));
    for(int a=0;a<32;a++) {
        DX7Settings s=dx7_factory(0); s.value[134]=a; s.value[135]=7; s.value[139]=s.value[140]=99; s.value[143]=7;
        dx7_note_on(&voice,12543.85,1,s); for(int i=0;i<256;i++) CHECK(isfinite(dx7_sample(&voice,&s,4)));
    }
    simple.value[142]=6; CHECK(!dx7_valid(simple));
    /* Better Chime uses quarter tracking and a constant envelope rate
       across the keyboard, while its carrier still follows the played note. */
    FMSettings chime=fm_factory(8); CHECK(fm_valid(chime));
    DX7Voice low,high;
    dx7_note_on(&low,65.40639132514966,90/127.f,chime.dx7);
    dx7_note_on(&high,1046.5022612023945,90/127.f,chime.dx7);
    CHECK(high.base_pitch[4]-low.base_pitch[4]==48*(16777216/12)/4);
    CHECK(low.base_pitch[5]!=high.base_pitch[5]);
    CHECK(low.envelope[4].rate_scale==high.envelope[4].rate_scale);
    CHECK(low.envelope[5].rate_scale==high.envelope[5].rate_scale);
    for(int tracking=0;tracking<=100;tracking+=50) {
        chime.dx7.value[DX7_NATIVE_PARAMETERS+4]=tracking;
        dx7_note_on(&low,65.40639132514966,90/127.f,chime.dx7);
        dx7_note_on(&high,1046.5022612023945,90/127.f,chime.dx7);
        CHECK(high.base_pitch[4]-low.base_pitch[4]==(int64_t)48*(16777216/12)*tracking/100);
    }
    chime.dx7.value[DX7_NATIVE_PARAMETERS+4]=25.5f;
    CHECK(dx7_valid(chime.dx7));
    for(int i=0;i<128;i++) CHECK(isfinite(dx7_sample(&low,&chime.dx7,1)));
    CHECK(low.key_tracking[4]==25.5f); /* Live edits preserve continuous values. */
    project_new(&project); project.instrument[0]=INSTRUMENT_FM; project.fm[0]=fm_factory(4);
    project.fm[0].dx7.value[DX7_NATIVE_PARAMETERS+4]=25.5f;
    ParameterTarget target; CHECK(parameter_from_pointer(&project,&project.fm[0].dx7.value[16],&target) && target.parameter==PARAM_DX7_FIRST+16);
    CHECK(automation_create(&project,target,"Output",16)>=0);
    CHECK(parameter_from_pointer(&project,&project.fm[0].dx7.value[DX7_NATIVE_PARAMETERS+4],&target));
    CHECK(parameter_descriptor(target.parameter)->kind==PARAMETER_CONTINUOUS);
    CHECK(project_save("dx7.llp",&project) && project_load("dx7.llp",&loaded)); CHECK(!memcmp(&project.fm[0],&loaded.fm[0],sizeof(FMSettings)));
    DevicePreset preset={.kind=PRESET_FM,.fm=project.fm[0]},read;
    CHECK(preset_save("dx7.llpreset",&preset) && preset_load("dx7.llpreset",&read) && !memcmp(&read.fm,&preset.fm,sizeof(FMSettings)));
    CHECK(channel_replace_instrument(&project,0,INSTRUMENT_SAMPLER) && project.automation_count==0);
    remove("dx7.llp"); remove("dx7.llpreset"); puts("DX7 reference PCM, tuning, live edits, release, algorithms, persistence and parameter bindings passed."); return 0;
}
