// SPDX-License-Identifier: GPL-3.0-only
#include "engine.h"
#include "preset.h"
#include "midi.h"
#include "project_check.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#define CHECK(x) do{if(!(x)){fprintf(stderr,"%d: %s\n",__LINE__,#x);return 1;}}while(0)
static Project p,q;static Player a,b;static FMVoice voices[16];
static float whole[8192],chunks[8192];
int main(int argc,char **argv) {
    if(argc>1 && !strcmp(argv[1],"--benchmark")){
        volatile float sink=0;
        for(int model=0;model<2;model++)for(int count=1;count<=16;count*=4){
            FMSettings s=model?fm_analog_factory(0):fm_factory(5);s.analog.chorus=0;
            for(int i=0;i<count;i++)fm_note_on(&voices[i],110*exp2(i/12.),s);
            clock_t start=clock();for(int n=0;n<RATE;n++)for(int i=0;i<count;i++)sink+=fm_sample(&voices[i],s,1);
            printf("%s %d voices: %.6f CPU seconds / audio second\n",model?"Analog":"Legacy Analog",count,(double)(clock()-start)/CLOCKS_PER_SEC);
        }
        (void)sink;return 0;
    }
    project_new(&p);CHECK(channel_replace_choice(&p,0,CHOICE_ANALOG));
    CHECK(p.fm[0].analog.model==1 && p.fm[0].analog.chorus==0 && !strcmp(channel_instrument_name(&p,0),"Analog"));
    p.volume[0]=.4f;p.master=.8f;
    for(int i=0;i<3;i++)p.notes[0][0][i]=(Note){60+4*i,100,0,4};
    Sample empty[CHANNELS]={0};player_reset(&a);player_reset(&b);
    render(&a,&p,empty,whole,4096);
    for(int at=0;at<4096;){int n=at+37<4096?37:4096-at;render(&b,&p,empty,chunks+2*at,n);at+=n;}
    CHECK(!memcmp(whole,chunks,sizeof whole));double energy=0;
    for(int i=0;i<4096;i++){CHECK(isfinite(whole[2*i]) && whole[2*i]==whole[2*i+1]);energy+=whole[2*i]*whole[2*i];}
    CHECK(energy>.01);int active=0;for(int i=0;i<(int)(sizeof a.voices/sizeof a.voices[0]);i++)active+=a.voices[i].gain>0;CHECK(active>=3);
    ParameterTarget target;CHECK(parameter_from_pointer(&p,&p.fm[0].analog.drive,&target) && target.parameter==PARAM_VA_FIRST+12);
    CHECK(midi_bind(&p,0,1,target));CHECK(parameter_write(&p,target,.5f) && p.fm[0].analog.drive==12);
    CHECK(automation_create(&p,target,"Drive",16)==0);p.clips[0][0]=1;p.clip_steps[0][0]=16;p.clips[1][0]=AUTOMATION_SOURCE+1;p.clip_steps[1][0]=16;
    CHECK(project_save("analog-modern.llp",&p) && project_load("analog-modern.llp",&q) && project_equal(&p,&q));
    DevicePreset preset={.kind=PRESET_FM,.fm=p.fm[0]},loaded;
    CHECK(preset_save("analog-modern.llpreset",&preset) && preset_load("analog-modern.llpreset",&loaded));
    CHECK(!memcmp(&preset.fm,&loaded.fm,sizeof(FMSettings)));
    p.automations[0].points[0].value=p.automations[0].points[1].value=0;
    player_reset(&a);a.song=1;render(&a,&p,empty,whole,4096);
    p.automations[0].points[0].value=p.automations[0].points[1].value=1;
    player_reset(&b);b.song=1;render(&b,&p,empty,chunks,4096);double driven_difference=0;
    for(int i=0;i<8192;i++){CHECK(isfinite(whole[i]) && isfinite(chunks[i]));driven_difference+=fabsf(whole[i]-chunks[i]);}
    CHECK(driven_difference>.1 && p.fm[0].analog.drive==12);
    FMSettings controls=fm_analog_factory(0);double osc_difference=0;fm_note_on(&voices[0],220,controls);fm_note_on(&voices[1],220,controls);
    FMSettings muted=controls;muted.analog.osc_level[2]=0;
    for(int i=0;i<3000;i++)osc_difference+=fabsf(fm_sample(&voices[0],controls,1)-fm_sample(&voices[1],muted,1));
    CHECK(osc_difference>.01 && fabsf(voices[0].envelope-voices[0].mod_envelope)>.01f);
    for(int preset_id=0;preset_id<3;preset_id++){
        FMSettings s=fm_analog_factory(preset_id);CHECK(fm_valid(s));fm_note_on(&voices[0],220,s);
        for(int i=0;i<2000;i++)CHECK(isfinite(fm_sample(&voices[0],s,1)));
        fm_note_off(&voices[0],s);for(int i=0;i<RATE*2;i++)fm_sample(&voices[0],s,1);CHECK(!fm_active(&voices[0]));
    }
    for(int wave=0;wave<4;wave++)for(int cutoff=0;cutoff<2;cutoff++){
        FMSettings s=fm_analog_factory(0);s.analog.cutoff=cutoff?20000:20;s.analog.resonance=.95f;s.analog.drive=24;s.analog.noise=1;
        for(int osc=0;osc<3;osc++){s.analog.osc_wave[osc]=wave;s.analog.osc_level[osc]=1;}
        fm_note_on(&voices[0],8000,s);for(int n=0;n<2000;n++){float out=fm_sample(&voices[0],s,8);CHECK(isfinite(out) && fabsf(out)<2);}
    }
    FMSettings singing=fm_analog_factory(0);for(int osc=0;osc<3;osc++)singing.analog.osc_level[osc]=0;
    singing.analog.noise=singing.analog.filter_env=singing.analog.keytrack=0;singing.analog.resonance=.95f;singing.analog.cutoff=440;
    singing.attack=.001f;singing.decay=.01f;singing.sustain=1;fm_note_on(&voices[0],440,singing);double ringing=0;
    for(int n=0;n<RATE;n++){float out=fm_sample(&voices[0],singing,1);CHECK(isfinite(out));ringing+=out*out;}
    CHECK(ringing>.0001);
    player_reset(&a);a.song=1;for(int at=0;at<4096;){int n=at+37<4096?37:4096-at;render(&a,&p,empty,whole+2*at,n);at+=n;}
    CHECK(!memcmp(whole,chunks,sizeof whole)); /* Automated drive is block-exact too. */
    CHECK(channel_replace_choice(&p,0,CHOICE_DX7) && p.fm[0].engine==1 && p.automation_count==0 && p.midi_binding_count==0);
    CHECK(p.notes[0][0][0].pitch==60 && p.notes[0][0][1].pitch==64);
    CHECK(channel_replace_choice(&p,0,CHOICE_CUSTOM_FM) && p.fm[0].engine==0);
    CHECK(midi_bind(&p,0,2,(ParameterTarget){PARAM_FM_DEPTH,0,0}));
    CHECK(channel_replace_choice(&p,0,CHOICE_SAMPLER) && p.midi_binding_count==0);
    p.channel_audio[0]=1;CHECK(!channel_replace_choice(&p,0,CHOICE_ANALOG));p.channel_audio[0]=0;
    if(argc>1 && !strcmp(argv[1],"--renders")){
        for(int preset_id=0;preset_id<3;preset_id++){
            project_new(&p);channel_replace_choice(&p,0,CHOICE_ANALOG);p.fm[0]=fm_analog_factory(preset_id);p.volume[0]=.35f;
            p.notes[0][0][0]=(Note){48,110,0,4};p.notes[0][0][1]=(Note){55,100,0,4};p.notes[0][0][2]=(Note){60,100,0,4};
            p.clips[0][0]=1;p.clip_steps[0][0]=16;char name[64];snprintf(name,sizeof name,"analog-%d.wav",preset_id);CHECK(export_wav(name,&p,empty));
        }
    }
    remove("analog-modern.llp");remove("analog-modern.llpreset");
    puts("Independent polyphonic voices, block-exact rendering, model stability, release, automation/MIDI, project/preset persistence and replacement passed.");return 0;
}
