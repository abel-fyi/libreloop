// SPDX-License-Identifier: GPL-3.0-only
#include "engine.h"
#include "preset.h"
#include "effect_chain.h"
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#define CHECK(x) do{if(!(x)){fprintf(stderr,"line %d: %s\n",__LINE__,#x);return 1;}}while(0)
static Project project,loaded;
static float pcm[48000],out[8192];
int main(void){
    for(int effect=0;effect<2;effect++){
        int type=effect?EFFECT_FLANGER:EFFECT_PHASER,kind=effect?PRESET_FLANGER:PRESET_PHASER;unsigned first=effect?PARAM_FLANGER_FIRST:PARAM_PHASER_FIRST;
        project_new(&project);project.effect_type[0][0]=type;project.effect_mix[0][0]=.5f;project.phaser[0][0]=phaser_factory(1);project.flanger[0][0]=flanger_factory(1);
        for(unsigned id=first;id<first+6;id++){const float *ptr=effect?flanger_parameter(&project.flanger[0][0],id-first):phaser_parameter(&project.phaser[0][0],id-first);ParameterTarget t;float v,lo,hi;CHECK(parameter_descriptor(id) && parameter_from_pointer(&project,ptr,&t) && t.parameter==id && !t.slot && !t.owner && parameter_info(&project,t,&v,&lo,&hi) && v==*ptr);}
        CHECK(project_save("modulation.llp",&project) && project_load("modulation.llp",&loaded));remove("modulation.llp");
        CHECK(loaded.effect_type[0][0]==type && (effect?!memcmp(&loaded.flanger[0][0],&project.flanger[0][0],sizeof(FlangerSettings)):!memcmp(&loaded.phaser[0][0],&project.phaser[0][0],sizeof(PhaserSettings))));
        for(unsigned i=0;i<3;i++){DevicePreset preset={.kind=kind,.phaser=phaser_factory(i),.flanger=flanger_factory(i),.mix=.5f},back;CHECK(preset_save("modulation.llpreset",&preset) && preset_load("modulation.llpreset",&back));remove("modulation.llpreset");CHECK(back.kind==kind && back.mix==.5f && (effect?!memcmp(&back.flanger,&preset.flanger,sizeof preset.flanger):!memcmp(&back.phaser,&preset.phaser,sizeof preset.phaser)));}
        for(int i=0;i<48000;i++)pcm[i]=.2f*sinf(6.28318530718f*440*i/48000);
        project.notes[0][0][0]=(Note){60,127,0,16};project.clips[0][0]=1;Sample samples[CHANNELS]={{.data=pcm,.frames=48000,.channels=1}};
        EffectRack *rack=effects_create(INSERTS+1);CHECK(rack && effects_prepare(rack,&project.effect_type[0][0],(INSERTS+1)*EFFECT_SLOTS));
        Player player;player_reset(&player);player.song=1;player.effects=rack;render(&player,&project,samples,out,4096);
        Phaser p;phaser_reset(&p);Flanger *f=flanger_create();CHECK(f);
        for(int i=0;i<4096;i++){float frame[2]={pcm[i],pcm[i]};if(effect)flanger_process(f,project.flanger[0][0],.5f,frame);else phaser_process(&p,project.phaser[0][0],.5f,frame);CHECK(!memcmp(frame,&out[i*2],sizeof frame));}
        CHECK(export_wav("modulation.wav",&project,samples));FILE *file=fopen("modulation.wav","rb");CHECK(file && !fseek(file,44,SEEK_SET));for(int i=0;i<8192;i++){int a=fgetc(file),b=fgetc(file);CHECK(a>=0 && b>=0 && abs((int16_t)(a|(b<<8))-(int16_t)(out[i]*32767))<=1);}fclose(file);remove("modulation.wav");
        int a=automation_create(&project,(ParameterTarget){PARAM_EFFECT_MIX,0,0},"Modulation mix",16);CHECK(a>=0);project.automations[a].points[0].value=project.automations[a].points[1].value=0;project.clips[1][0]=AUTOMATION_SOURCE+a+1;
        effects_reset(rack);player_reset(&player);player.song=1;player.effects=rack;render(&player,&project,samples,out,4096);for(int i=0;i<4096;i++)CHECK(out[i*2]==pcm[i] && out[i*2+1]==pcm[i]);
        project.automations[a].target=(ParameterTarget){first+4,0,0};project.automations[a].points[0].value=project.automations[a].points[1].value=0;project.phaser[0][0].stereo=project.flanger[0][0].stereo=1;
        effects_reset(rack);player_reset(&player);player.song=1;player.effects=rack;render(&player,&project,samples,out,4096);for(int i=0;i<4096;i++)CHECK(out[i*2]==out[i*2+1]);
        project.midi_bindings[0]=(MidiBinding){0,10,{first,0,0}};project.midi_binding_count=1;CHECK(effect_chain_move(&project,0,0,2) && project.effect_type[0][2]==type && project.automations[a].target.slot==2 && project.midi_bindings[0].target.slot==2);
        CHECK((effect?project.flanger[0][2].stereo:project.phaser[0][2].stereo)==1);
        effects_free(rack);flanger_free(f);
    }
    puts("Phaser/Flanger registry, presets, project persistence, parameters, engine/export equivalence, automation and reorder passed.");return 0;
}
