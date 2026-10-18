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
static float pcm[48000],out[8192];
int main(void) {
    project_new(&project);project.effect_type[0][0]=EFFECT_REVERB;project.reverb[0][0]=(ReverbSettings){.4f,1.2f,20000,10,1.1f,.2f,0};project.effect_mix[0][0]=.35f;
    CHECK(project_save("reverb.llp",&project) && project_load("reverb.llp",&loaded));remove("reverb.llp");
    CHECK(loaded.effect_type[0][0]==EFFECT_REVERB && !memcmp(&project.reverb[0][0],&loaded.reverb[0][0],sizeof(ReverbSettings)) && loaded.effect_mix[0][0]==.35f);
    for(unsigned id=PARAM_REVERB_FIRST;id<=PARAM_REVERB_LAST;id++) {
        const float *value=device_parameter(effect_descriptor(EFFECT_REVERB),&project.reverb[0][0],id);ParameterTarget target;float v,lo,hi;
        CHECK(value && parameter_from_pointer(&project,value,&target) && target.parameter==id && target.owner==0 && !target.slot);
        CHECK(parameter_info(&project,target,&v,&lo,&hi) && v==*value && parameter_write(&loaded,target,.5f) && !parameter_write(&loaded,target,NAN));
    }
    for(unsigned i=0;i<REVERB_FACTORY_COUNT;i++) {
        DevicePreset preset={.kind=PRESET_REVERB,.reverb=reverb_factory(i),.mix=reverb_factory_mix(i)},restored;
        CHECK(preset_save("reverb.llpreset",&preset) && preset_load("reverb.llpreset",&restored));remove("reverb.llpreset");
        CHECK(restored.kind==PRESET_REVERB && !memcmp(&restored.reverb,&preset.reverb,sizeof(ReverbSettings)) && restored.mix==preset.mix);
    }
    for(int i=0;i<48000;i++)pcm[i]=.2f*sinf(6.28318530718f*440*i/48000);
    project.notes[0][0][0]=(Note){60,127,0,16};project.clips[0][0]=1;Sample samples[CHANNELS]={{.data=pcm,.frames=48000,.channels=1}};
    EffectRack *rack=effects_create(INSERTS+1);CHECK(rack && effects_prepare(rack,&project.effect_type[0][0],(INSERTS+1)*EFFECT_SLOTS));
    Player player;player_reset(&player);player.song=1;player.effects=rack;render(&player,&project,samples,out,4096);
    Reverb *expected=reverb_create();CHECK(expected);
    for(int i=0;i<4096;i++){float frame[2]={pcm[i],pcm[i]};reverb_process(expected,project.reverb[0][0],.35f,frame);CHECK(!memcmp(frame,&out[i*2],sizeof frame));}
    CHECK(export_wav("reverb.wav",&project,samples));FILE *f=fopen("reverb.wav","rb");CHECK(f && !fseek(f,44,SEEK_SET));
    for(int i=0;i<8192;i++){int a=fgetc(f),b=fgetc(f);CHECK(a>=0 && b>=0 && abs((int16_t)(a|(b<<8))-(int16_t)(out[i]*32767))<=1);}fclose(f);remove("reverb.wav");
    CHECK(effects_prepare(rack,&project.effect_type[0][0],(INSERTS+1)*EFFECT_SLOTS));
    render(&player,&project,samples,out,512);
    for(int i=0;i<512;i++){float frame[2]={pcm[4096+i],pcm[4096+i]};reverb_process(expected,project.reverb[0][0],.35f,frame);CHECK(!memcmp(frame,&out[i*2],sizeof frame));}
    int a=automation_create(&project,(ParameterTarget){PARAM_EFFECT_MIX,0,0},"Reverb mix",16);CHECK(a>=0);
    project.automations[a].points[0].value=project.automations[a].points[1].value=0;project.clips[1][0]=AUTOMATION_SOURCE+a+1;
    effects_reset(rack);player_reset(&player);player.song=1;player.effects=rack;render(&player,&project,samples,out,4096);
    for(int i=0;i<4096;i++)CHECK(out[i*2]==pcm[i] && out[i*2+1]==pcm[i]);
    project.automations[a].target=(ParameterTarget){PARAM_REVERB_WIDTH,0,0};project.automations[a].points[0].value=project.automations[a].points[1].value=0;
    project.reverb[0][0].width=2;effects_reset(rack);player_reset(&player);player.song=1;player.effects=rack;render(&player,&project,samples,out,4096);
    for(int i=0;i<4096;i++)CHECK(out[i*2]==out[i*2+1]);
    project.midi_bindings[0]=(MidiBinding){0,10,{PARAM_REVERB_DECAY,0,0}};project.midi_binding_count=1;
    CHECK(effect_chain_move(&project,0,0,2) && project.effect_type[0][2]==EFFECT_REVERB && project.reverb[0][2].width==2);
    CHECK(project.automations[a].target.slot==2 && project.midi_bindings[0].target.slot==2);
    CHECK(effects_prepare(rack,&project.effect_type[0][0],(INSERTS+1)*EFFECT_SLOTS));
    effects_reset_bus(rack,0);player_reset(&player);player.song=1;player.effects=rack;render(&player,&project,samples,out,4096);
    for(int i=0;i<4096;i++)CHECK(out[i*2]==out[i*2+1]);
    effects_free(rack);reverb_free(expected);
    project.reverb[0][2].decay=NAN;CHECK(!project_save("invalid-reverb.llp",&project));
    puts("Reverb registry, project/factory-preset persistence, parameter targets, prepared render/export, automation and reordering passed.");return 0;
}
