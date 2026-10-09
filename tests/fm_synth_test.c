// SPDX-License-Identifier: GPL-3.0-only
#include "fm_synth.h"
#include "engine.h"
#include "preset.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CHECK(x) do { if(!(x)) { fprintf(stderr,"Failed line %d: %s\n",__LINE__,#x); return 1; } } while(0)
static Project project,loaded;
int main(void) {
    FMSettings settings=fm_legacy(); CHECK(fm_valid(settings));
    CHECK(!fm_valid((FMSettings){2.5f,2,.005f,.3f,.65f,.2f}));
    FMVoice a,b; settings.depth=0; settings.attack=.001f; settings.decay=.01f; settings.sustain=1; settings.release=.01f;
    fm_note_on(&a,440,settings); fm_note_on(&b,880,settings);
    int crossings=0; float previous=0,peak=0;
    for(int i=0;i<RATE;i++) {
        float x=fm_sample(&a,settings,1); CHECK(isfinite(x) && fabsf(x)<=.201f);
        if(i>RATE/2 && previous<=0 && x>0) crossings++;
        previous=x; peak=fmaxf(peak,fabsf(x));
    }
    CHECK(abs(crossings-220)<=1 && peak>.19f && b.carrier==0); /* Tuning and independent voices. */
    fm_note_off(&a,settings); for(int i=0;i<RATE/20;i++) fm_sample(&a,settings,1); CHECK(!fm_active(&a));
    fm_note_on(&a,440,settings); settings.depth=8;
    for(int i=0;i<RATE/10;i++) { float x=fm_sample(&a,settings,1); CHECK(isfinite(x) && fabsf(x)<=.201f); }
    fm_note_on(&a,20000,settings); for(int i=0;i<1000;i++) CHECK(isfinite(fm_sample(&a,settings,16)));
    FMSettings ep=fm_epiano(); CHECK(fm_valid(ep));
    fm_note_on_velocity(&a,440,ep,1); fm_note_on_velocity(&b,440,ep,.2f); float difference=0;
    for(int i=0;i<RATE;i++) { float x=fm_sample(&a,ep,1),y=fm_sample(&b,ep,1); CHECK(isfinite(x) && fabsf(x)<=.201f); difference+=fabsf(x-y); }
    CHECK(difference>10 && a.mod_envelope<.2f && a.mod_envelope>=.119f);
    FMSettings motion=fm_legacy(); motion.depth=0; motion.attack=.001f; motion.sustain=1; motion.vibrato=25; motion.tremolo=.8f;
    fm_note_on(&a,440,motion); double low_frequency=1000,high_frequency=0,last_phase=0; float loud=0,quiet=0;
    for(int i=0;i<RATE/4;i++) {
        float x=fm_sample(&a,motion,1); CHECK(isfinite(x) && fabsf(x)<=.201f);
        double advance=a.carrier-last_phase; if(advance<0) advance+=6.283185307179586; last_phase=a.carrier;
        double hz=advance*RATE/6.283185307179586; low_frequency=fmin(low_frequency,hz); high_frequency=fmax(high_frequency,hz);
        if(i>RATE*.01 && i<RATE*.03) loud+=x*x;
        if(i>RATE*.09 && i<RATE*.11) quiet+=x*x;
    }
    CHECK(low_frequency<435 && high_frequency>445 && quiet<loud*.2f);
    DevicePreset factory; CHECK(preset_load(FM_FACTORY_PRESET,&factory) && factory.kind==PRESET_FM);
    CHECK(!memcmp(&factory.fm,&ep,sizeof ep));
    motion.tremolo=NAN; CHECK(!fm_valid(motion));
    project_new(&project); project.instrument[0]=INSTRUMENT_FM; project.fm[0]=settings;
    project.notes[0][0][0]=(Note){69,127,0,1}; project.notes[0][0][1]=(Note){72,100,0,1};
    Sample samples[CHANNELS]={0}; Player player; player_reset(&player);
    float out[1024]; render(&player,&project,samples,out,512);
    CHECK(player.channel_trigger[0] && player.voices[0].instrument==INSTRUMENT_FM && player.voices[1].gain>0);
    float energy=0; for(int i=0;i<1024;i++) energy+=fabsf(out[i]); CHECK(energy>1);
    for(int block=0;block<32;block++) render(&player,&project,samples,out,512);
    CHECK(!player.voices[0].gain && !player.voices[1].gain); /* Gate plus release, no stuck notes. */
    CHECK(project_save("fm.llp",&project) && project_load("fm.llp",&loaded));
    CHECK(loaded.instrument[0]==INSTRUMENT_FM && loaded.fm[0].depth==8 && loaded.fm[0].release==.01f && loaded.fm[0].mod_sustain==1 && loaded.fm[0].lfo_rate==5); remove("fm.llp");
    ParameterTarget target; CHECK(parameter_from_pointer(&project,&project.fm[0].ratio,&target) && target.parameter==PARAM_FM_RATIO);
    CHECK(parameter_from_pointer(&project,&project.fm[0].vibrato,&target) && target.parameter==PARAM_FM_VIBRATO);
    int index=automation_create(&project,(ParameterTarget){PARAM_FM_SUSTAIN,0,0},"FM sustain",16); CHECK(index>=0);
    project.automations[index].points[0].value=project.automations[index].points[1].value=0;
    project.clips[0][0]=AUTOMATION_SOURCE+index+1; project.clips[1][0]=1;
    project.notes[0][0][0].length=16; project.notes[0][0][1].velocity=0;
    player_reset(&player); player.song=1;
    for(int block=0;block<10;block++) render(&player,&project,samples,out,512);
    energy=0; for(int i=0;i<1024;i++) energy+=fabsf(out[i]); CHECK(energy<.001f);
    CHECK(export_wav("fm.wav",&project,samples)); remove("fm.wav");
    project.instrument[1]=INSTRUMENT_FM; project.fm[1]=fm_default(); project.channel_count=2;
    CHECK(channel_delete(&project,0) && project.instrument[0]==INSTRUMENT_FM && project.fm[0].depth==fm_default().depth && !project.automation_count);
    puts("FM tuning, polyphony, release, bounded output, automation, save/load and channel deletion passed."); return 0;
}
