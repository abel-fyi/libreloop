// SPDX-License-Identifier: GPL-3.0-only
#include "engine.h"
#include "preset.h"
#include "effect_chain.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#define CHECK(x) do{if(!(x)){fprintf(stderr,"%d: %s\n",__LINE__,#x);return 1;}}while(0)
static Project project,loaded;
static float pcm[2048];
int main(void) {
    project_new(&project);project.effect_type[0][0]=EFFECT_DELAY;
    project.delay[0][0]=(DelaySettings){1,.5f,20000,0,1,0,0};project.effect_mix[0][0]=1;
    CHECK(project_save("delay.llp",&project) && project_load("delay.llp",&loaded));remove("delay.llp");
    CHECK(loaded.effect_type[0][0]==EFFECT_DELAY && !memcmp(&loaded.delay[0][0],&project.delay[0][0],sizeof(DelaySettings)));
    for(unsigned id=PARAM_DELAY_FIRST;id<=PARAM_DELAY_LAST;id++) {
        float *pointer=(float *)device_parameter(effect_descriptor(EFFECT_DELAY),&project.delay[0][0],id);
        ParameterTarget t;CHECK(pointer && parameter_from_pointer(&project,pointer,&t) && t.parameter==id && t.owner==0 && t.slot==0);
        float value,lo,hi;CHECK(parameter_info(&project,t,&value,&lo,&hi) && value==*pointer);
    }
    DevicePreset preset={.kind=PRESET_DELAY,.delay=delay_default(),.mix=.37f},restored;
    CHECK(preset_save("delay.llpreset",&preset) && preset_load("delay.llpreset",&restored));remove("delay.llpreset");
    CHECK(restored.kind==PRESET_DELAY && restored.mix==preset.mix && !memcmp(&restored.delay,&preset.delay,sizeof preset.delay));
    EffectRack *rack=effects_create(INSERTS+1);CHECK(rack && effects_prepare(rack,&project.effect_type[0][0],(INSERTS+1)*EFFECT_SLOTS));
    /* Preparation is idempotent and never changes already sounding history. */
    for(int i=0;i<200;i++) {
        if(i==32)CHECK(effects_prepare(rack,&project.effect_type[0][0],(INSERTS+1)*EFFECT_SLOTS));
        float frame[2]={i==0,0};
        effects_process_all(rack,0,project.effect_type[0],project.chorus[0],project.eq[0],project.delay[0],120,project.effect_mix[0],project.effect_bypass[0],frame);
        CHECK(frame[0]==(i==48?1:i==96?.5f:i==144?.25f:i==192?.125f:0) && frame[1]==0);
    }
    effects_reset_bus(rack,0);
    for(int i=0;i<200;i++) {
        float frame[2]={0};effects_process_all(rack,0,project.effect_type[0],project.chorus[0],project.eq[0],project.delay[0],120,project.effect_mix[0],project.effect_bypass[0],frame);
        CHECK(!frame[0] && !frame[1]);
    }
    /* Bypass must be unity dry, even when the delay output knob is at zero. */
    effects_reset(rack);project.delay[0][0].output=0;project.effect_bypass[0][0]=1;
    float frame[2]={.2f,-.3f};effects_process_all(rack,0,project.effect_type[0],project.chorus[0],project.eq[0],project.delay[0],120,project.effect_mix[0],project.effect_bypass[0],frame);
    CHECK(frame[0]==.2f && frame[1]==-.3f);
    project.effect_bypass[0][0]=0;project.delay[0][0].output=1;
    /* Realtime engine and export use the exact same prepared rack path. */
    pcm[0]=.2f;Sample samples[CHANNELS]={{.data=pcm,.frames=2048,.channels=1}};
    project.notes[0][0][0]=(Note){60,127,0,16};project.clips[0][0]=1;
    Player player;player_reset(&player);player.song=1;player.effects=rack;effects_reset(rack);
    float out[1024];render(&player,&project,samples,out,512);CHECK(out[96]==.2f && out[192]==.1f && out[0]==0);
    CHECK(export_wav("delay.wav",&project,samples));FILE *f=fopen("delay.wav","rb");CHECK(f && !fseek(f,44,SEEK_SET));
    for(int i=0;i<1024;i++){int a=fgetc(f),b=fgetc(f);CHECK(a>=0 && b>=0 && abs((int16_t)(a|(b<<8))-(int16_t)(out[i]*32767))<=1);}fclose(f);remove("delay.wav");
    int a=automation_create(&project,(ParameterTarget){PARAM_DELAY_FEEDBACK,0,0},"Echo decay",16);CHECK(a>=0);
    project.automations[a].points[0].value=project.automations[a].points[1].value=0;project.clips[1][0]=AUTOMATION_SOURCE+a+1;
    effects_reset(rack);player_reset(&player);player.song=1;player.effects=rack;render(&player,&project,samples,out,512);
    CHECK(out[96]==.2f && out[192]==0);
    project.delay[0][0].feedback=.61f;
    project.midi_bindings[0]=(MidiBinding){0,10,{PARAM_DELAY_TIME,0,0}};project.midi_binding_count=1;
    CHECK(effect_chain_move(&project,0,0,2) && project.effect_type[0][2]==EFFECT_DELAY && project.delay[0][2].feedback==.61f);
    CHECK(project.automations[a].target.slot==2 && project.midi_bindings[0].target.slot==2);
    effects_free(rack);project.delay[0][0].feedback=NAN;CHECK(!project_save("invalid-delay.llp",&project));
    puts("Delay registry, project/preset roundtrips, parameter targets, rack ownership/reset, bypass, automation and export passed.");return 0;
}
