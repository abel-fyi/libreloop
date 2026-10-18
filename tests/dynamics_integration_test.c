// SPDX-License-Identifier: GPL-3.0-only
#include "engine.h"
#include "preset.h"
#include "effect_chain.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#define CHECK(x) do{if(!(x)){fprintf(stderr,"line %d: %s\n",__LINE__,#x);return 1;}}while(0)
static Project project,loaded;
static float pcm[48000],out[8192];
int main(void){
    project_new(&project);project.effect_type[0][0]=EFFECT_DYNAMICS;project.dynamics[0][0]=dynamics_factory(1);
    CHECK(effect_descriptor(EFFECT_DYNAMICS) && device_descriptor(DEVICE_DYNAMICS));
    for(unsigned id=PARAM_DYNAMICS_FIRST;id<=PARAM_DYNAMICS_LAST;id++){CHECK(parameter_descriptor(id));ParameterTarget t={id,0,0};float value,lo,hi;CHECK(parameter_info(&project,t,&value,&lo,&hi));ParameterTarget found;CHECK(parameter_from_pointer(&project,dynamics_parameter(&project.dynamics[0][0],id-PARAM_DYNAMICS_FIRST),&found));CHECK(found.parameter==id && found.owner==0 && found.slot==0);}
    CHECK(project_save("dynamics-test.llp",&project));CHECK(project_load("dynamics-test.llp",&loaded));CHECK(!memcmp(&loaded.dynamics[0][0],&project.dynamics[0][0],sizeof(DynamicsSettings)));remove("dynamics-test.llp");
    DevicePreset preset={.kind=PRESET_DYNAMICS,.dynamics=project.dynamics[0][0],.mix=.7f},back;
    CHECK(preset_save("dynamics-test.llpreset",&preset));CHECK(preset_load("dynamics-test.llpreset",&back));CHECK(back.kind==PRESET_DYNAMICS && back.mix==.7f && !memcmp(&back.dynamics,&preset.dynamics,sizeof preset.dynamics));remove("dynamics-test.llpreset");
    EffectRack *rack=effects_create(1);CHECK(rack && effects_prepare(rack,&project.effect_type[0][0],EFFECT_SLOTS));Dynamics *reference=dynamics_create();CHECK(reference);
    const DynamicsSettings *settings[EFFECT_SLOTS];for(int i=0;i<EFFECT_SLOTS;i++)settings[i]=&project.dynamics[0][i];
    for(int i=0;i<4800;i++){float f[2]={.4f*sinf(i*.03f),.2f*sinf(i*.03f)},expected[2];memcpy(expected,f,sizeof f);dynamics_process(reference,&project.dynamics[0][0],1,expected);effects_process_all(rack,0,project.effect_type[0],project.chorus[0],project.eq[0],project.delay[0],project.tube[0],project.reverb[0],settings,120,project.effect_mix[0],project.effect_bypass[0],f);CHECK(!memcmp(f,expected,sizeof f));}
    DynamicsMonitor m;effects_dynamics_monitor(rack,0,0,&m);CHECK(m.count>0);effects_reset(rack);float silence[2]={0};effects_process_all(rack,0,project.effect_type[0],project.chorus[0],project.eq[0],project.delay[0],project.tube[0],project.reverb[0],settings,120,project.effect_mix[0],project.effect_bypass[0],silence);CHECK(!silence[0] && !silence[1]);
    dynamics_free(reference);effects_free(rack);
    /* Actual engine and offline export use the same prepared processor. */
    project_new(&project);project.effect_type[0][0]=EFFECT_DYNAMICS;project.dynamics[0][0]=dynamics_factory(1);
    for(int i=0;i<48000;i++)pcm[i]=.2f*sinf(6.28318530718f*440*i/48000);
    project.notes[0][0][0]=(Note){60,127,0,16};project.clips[0][0]=1;Sample samples[CHANNELS]={{.data=pcm,.frames=48000,.channels=1}};
    rack=effects_create(INSERTS+1);CHECK(rack && effects_prepare(rack,&project.effect_type[0][0],(INSERTS+1)*EFFECT_SLOTS));
    Player player;player_reset(&player);player.song=1;player.effects=rack;render(&player,&project,samples,out,4096);
    reference=dynamics_create();CHECK(reference);
    for(int i=0;i<4096;i++){float frame[2]={pcm[i],pcm[i]};dynamics_process(reference,&project.dynamics[0][0],1,frame);CHECK(!memcmp(frame,&out[i*2],sizeof frame));}
    CHECK(export_wav("dynamics.wav",&project,samples));FILE *f=fopen("dynamics.wav","rb");CHECK(f && !fseek(f,44,SEEK_SET));
    for(int i=0;i<8192;i++){int a=fgetc(f),b=fgetc(f);CHECK(a>=0 && b>=0 && abs((int16_t)(a|(b<<8))-(int16_t)(out[i]*32767))<=1);}fclose(f);remove("dynamics.wav");
    int a=automation_create(&project,(ParameterTarget){PARAM_DYNAMICS_FIRST+4+3*12+1,0,0},"Master post",16);CHECK(a>=0);
    project.automations[a].points[0].value=project.automations[a].points[1].value=.5f;project.clips[1][0]=AUTOMATION_SOURCE+a+1;
    project.dynamics[0][0].band[3].post=12;
    effects_reset(rack);player_reset(&player);player.song=1;player.effects=rack;render(&player,&project,samples,out,4096);
    DynamicsSettings automated=project.dynamics[0][0];automated.band[3].post=0;dynamics_reset(reference);
    for(int i=0;i<4096;i++){float frame[2]={pcm[i],pcm[i]};dynamics_process(reference,&automated,1,frame);CHECK(!memcmp(frame,&out[i*2],sizeof frame));}
    project.midi_bindings[0]=(MidiBinding){0,10,{PARAM_DYNAMICS_FIRST+4,0,0}};project.midi_binding_count=1;
    CHECK(effect_chain_move(&project,0,0,2));CHECK(project.effect_type[0][2]==EFFECT_DYNAMICS && project.dynamics[0][2].band[3].post==12 && project.automations[a].target.slot==2 && project.midi_bindings[0].target.slot==2);
    dynamics_free(reference);effects_free(rack);puts("Dynamics registry, parameters, project and preset roundtrip, rack equivalence and reset passed.");return 0;
}
