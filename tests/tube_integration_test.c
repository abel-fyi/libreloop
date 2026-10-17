// SPDX-License-Identifier: GPL-3.0-only
#include "engine.h"
#include "preset.h"
#include "effect_chain.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CHECK(x) do{if(!(x)){fprintf(stderr,"%d: %s\n",__LINE__,#x);return 1;}}while(0)
static Project project,loaded;
static float pcm[48000];
int main(void) {
    project_new(&project);project.effect_type[0][0]=EFFECT_TUBE;project.tube[0][0]=(TubeSettings){19,7200,-5};project.effect_mix[0][0]=.7f;
    CHECK(project_save("tube.llp",&project) && project_load("tube.llp",&loaded));remove("tube.llp");
    CHECK(loaded.effect_type[0][0]==EFFECT_TUBE && !memcmp(&project.tube[0][0],&loaded.tube[0][0],sizeof(TubeSettings)) && loaded.effect_mix[0][0]==.7f);
    for(unsigned id=PARAM_TUBE_FIRST;id<=PARAM_TUBE_LAST;id++) {
        const float *value=device_parameter(effect_descriptor(EFFECT_TUBE),&project.tube[0][0],id);ParameterTarget target;float v,lo,hi;
        CHECK(value && parameter_from_pointer(&project,value,&target) && target.parameter==id && target.owner==0 && !target.slot);
        CHECK(parameter_info(&project,target,&v,&lo,&hi) && v==*value && parameter_write(&loaded,target,.5f) && !parameter_write(&loaded,target,NAN));
    }
    DevicePreset preset={.kind=PRESET_TUBE,.tube={19,7200,-5},.mix=.7f},restored;
    CHECK(preset_save("tube.llpreset",&preset) && preset_load("tube.llpreset",&restored));remove("tube.llpreset");
    CHECK(restored.kind==PRESET_TUBE && !memcmp(&restored.tube,&preset.tube,sizeof(TubeSettings)) && restored.mix==preset.mix);
    for(int i=0;i<48000;i++)pcm[i]=.3f*sinf(6.28318530718f*440*i/48000);
    project.notes[0][0][0]=(Note){60,127,0,16};project.clips[0][0]=1;Sample samples[CHANNELS]={{.data=pcm,.frames=48000,.channels=1}};
    EffectRack *rack=effects_create(INSERTS+1);CHECK(rack && effects_prepare(rack,&project.effect_type[0][0],(INSERTS+1)*EFFECT_SLOTS));
    Player player;player_reset(&player);player.song=1;player.effects=rack;float out[1024];render(&player,&project,samples,out,512);
    Tube expected;tube_reset(&expected);
    for(int i=0;i<512;i++){float frame[2]={pcm[i],pcm[i]};tube_process(&expected,project.tube[0][0],.7f,frame);CHECK(!memcmp(frame,&out[i*2],sizeof frame));}
    CHECK(export_wav("tube.wav",&project,samples));FILE *f=fopen("tube.wav","rb");CHECK(f && !fseek(f,44,SEEK_SET));
    for(int i=0;i<1024;i++){int a=fgetc(f),b=fgetc(f);CHECK(a>=0 && b>=0 && abs((int16_t)(a|(b<<8))-(int16_t)(out[i]*32767))<=1);}fclose(f);remove("tube.wav");
    /* Automation at dry mix zero must reproduce the source without a delay. */
    int a=automation_create(&project,(ParameterTarget){PARAM_EFFECT_MIX,0,0},"Tube mix",16);CHECK(a>=0);
    project.automations[a].points[0].value=project.automations[a].points[1].value=0;project.clips[1][0]=AUTOMATION_SOURCE+a+1;
    effects_reset(rack);player_reset(&player);player.song=1;player.effects=rack;render(&player,&project,samples,out,512);
    for(int i=0;i<512;i++)CHECK(out[i*2]==pcm[i] && out[i*2+1]==pcm[i]);
    project.automations[a].target=(ParameterTarget){PARAM_TUBE_DRIVE,0,0};project.automations[a].points[0].value=project.automations[a].points[1].value=0;
    project.tube[0][0].drive=36;effects_reset(rack);player_reset(&player);player.song=1;player.effects=rack;render(&player,&project,samples,out,512);
    TubeSettings zero_drive=project.tube[0][0];zero_drive.drive=0;tube_reset(&expected);
    for(int i=0;i<512;i++){float frame[2]={pcm[i],pcm[i]};tube_process(&expected,zero_drive,.7f,frame);CHECK(!memcmp(frame,&out[i*2],sizeof frame));}
    project.midi_bindings[0]=(MidiBinding){0,10,{PARAM_TUBE_TONE,0,0}};project.midi_binding_count=1;
    CHECK(effect_chain_move(&project,0,0,2) && project.effect_type[0][2]==EFFECT_TUBE && project.tube[0][2].drive==36);
    CHECK(project.automations[a].target.slot==2 && project.midi_bindings[0].target.slot==2);
    effects_reset_bus(rack,0);effects_free(rack);
    project.tube[0][2].tone=NAN;CHECK(!project_save("invalid-tube.llp",&project));preset.tube.drive=INFINITY;CHECK(!preset_save("invalid-tube.llpreset",&preset));
    puts("Tube registry, project/preset persistence, parameter targets, realtime render/export, automation and reordering passed.");return 0;
}
