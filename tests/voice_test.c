// SPDX-License-Identifier: GPL-3.0-only
#include "engine.h"
#include "project_check.h"
#include <math.h>
#include <stdio.h>
#include <string.h>
#define CHECK(x) do{if(!(x)){fprintf(stderr,"%d: %s\n",__LINE__,#x);return 1;}}while(0)
static Project p,q;static Player a,b;static float whole[24000],chunks[24000];
static void on(Player *player,int slot,int note) {
    player_voice_prepare(player,&p,slot);
    Voice *v=&player->voices[slot];*v=(Voice){.channel=0,.instrument=p.instrument[0],.gain=.8f,.remaining=-1,.lane=-1};
    instrument_start(&v->device,v->instrument,(InstrumentSettings){&p.sampler[0],&p.fm[0]},(InstrumentNote){.frequency=440*exp2((note-69)/12.),.speed=exp2((note-60)/12.),.velocity=.8f});
    player_voice_on(player,&p,slot,note);
}
int main(void) {
    project_new(&p);CHECK(channel_replace_choice(&p,0,CHOICE_ANALOG));p.voice[0]=(VoiceSettings){.mono=1,.legato=1,.glide=.01f};
    CHECK(voice_settings_valid(p.voice[0]));CHECK(!voice_settings_valid((VoiceSettings){.limit=1.5f}));CHECK(!voice_settings_valid((VoiceSettings){.glide=NAN}));
    player_reset(&a);on(&a,0,60);a.voices[0].fm.envelope=.6f;
    on(&a,1,72);CHECK(a.voices[1].fm.envelope==.6f && a.voices[1].base_pitch==60 && a.voices[1].glide_left==480);
    CHECK(!a.voices[0].selected && a.voices[1].selected);
    for(int i=0;i<480;i++)player_voice_pitch(&a.voices[1]);CHECK(a.voices[1].current_pitch==72 && player_voice_pitch(&a.voices[1])==2);
    player_voice_off(&a,&p,1);player_voice_tick(&a,&p);CHECK(a.voices[0].selected && a.voices[0].held==1 && !a.voices[1].gain);
    CHECK(a.voices[0].current_pitch==72 && a.voices[0].fm.envelope==.6f);
    player_voice_off(&a,&p,0);player_voice_tick(&a,&p);CHECK(a.voices[0].held==-1 && a.voices[0].fm.stage!=0);
    /* Releasing an older held key cannot release the foreground note. */
    player_reset(&a);on(&a,0,60);on(&a,1,64);player_voice_off(&a,&p,0);player_voice_tick(&a,&p);CHECK(a.voices[1].held==1 && a.voices[1].selected && !a.voices[0].gain);
    p.voice[0].legato=0;player_reset(&a);on(&a,0,60);a.voices[0].fm.envelope=.6f;on(&a,1,72);CHECK(a.voices[1].fm.envelope!=.6f && a.voices[1].base_pitch==72 && a.voices[1].current_pitch==60);
    /* One mono foreground voice across live keyboard and sequenced pools. */
    p.voice[0]=(VoiceSettings){.mono=1,.legato=1,.glide=.01f};player_reset(&a);player_reset(&b);
    on(&a,0,60);a.voices[0].fm.envelope=.6f;b.voice_order=a.voice_order;on(&b,0,72);
    player_voice_tick_pair(&a,&b,&p);CHECK(!a.voices[0].selected && b.voices[0].selected && b.voices[0].base_pitch==60 && b.voices[0].fm.envelope==.6f);
    player_voice_off(&b,&p,0);player_voice_tick_pair(&a,&b,&p);CHECK(a.voices[0].selected && !b.voices[0].gain);
    /* Voice limiting replaces the oldest active note. */
    p.voice[0]=(VoiceSettings){.limit=2};player_reset(&a);on(&a,0,60);on(&a,1,64);on(&a,2,67);CHECK(!a.voices[0].gain && a.voices[1].gain && a.voices[2].gain);
    b.voice_order=a.voice_order;on(&b,0,72);player_voice_tick_pair(&a,&b,&p);CHECK(!a.voices[1].gain && a.voices[2].gain && b.voices[0].gain);
    /* Same voice policy works for sampler note priority and pitch. */
    CHECK(channel_replace_choice(&p,0,CHOICE_SAMPLER));p.voice[0]=(VoiceSettings){.mono=1,.legato=1,.glide=.01f};
    player_reset(&a);on(&a,0,60);a.voices[0].sampler.position=123;on(&a,1,72);CHECK(a.voices[1].sampler.position==123 && a.voices[1].sampler.speed==1);
    player_voice_off(&a,&p,1);player_voice_tick(&a,&p);CHECK(a.voices[0].selected && a.voices[0].sampler.position==123);
    /* A finished one-shot remains the foreground held key until key-up. */
    float pcm[16]={1};Sample tiny[CHANNELS]={0};tiny[0]=(Sample){.data=pcm,.frames=16,.channels=1};
    p.voice[0].legato=0;player_reset(&a);player_reset(&b);on(&b,0,60);on(&b,1,72);
    render_mixer(&a,&b,&p,tiny,whole,128,0,0);CHECK(b.voices[1].selected && b.voices[1].held>0 && b.voices[1].gain && !b.voices[0].selected);
    player_voice_off(&b,&p,1);render_mixer(&a,&b,&p,tiny,whole,1,0,0);CHECK(b.voices[0].selected && !b.voices[1].gain);
    CHECK(project_save("voice.llp",&p) && project_load("voice.llp",&q) && project_equal(&p,&q));remove("voice.llp");
    /* Offline note transitions are independent of render block boundaries. */
    CHECK(channel_replace_choice(&p,0,CHOICE_ANALOG));p.voice[0]=(VoiceSettings){.mono=1,.legato=1,.glide=.01f};
    memset(p.notes,0,sizeof p.notes);p.notes[0][0][0]=(Note){60,100,0,1};p.notes[0][0][1]=(Note){72,100,.5f,.25f};
    Sample empty[CHANNELS]={0};player_reset(&a);player_reset(&b);render(&a,&p,empty,whole,12000);
    for(int i=0;i<12000;){int n=i+37<=12000?37:12000-i;render(&b,&p,empty,chunks+i*2,n);i+=n;}
    CHECK(!memcmp(whole,chunks,sizeof whole));double energy=0;for(int i=0;i<24000;i++){CHECK(isfinite(whole[i]));energy+=whole[i]*whole[i];}CHECK(energy>.01);
    CHECK(a.voices[0].selected && a.voices[0].note_pitch==60);
    /* Every synth family accepts shared pitch/priority through the real mixer. */
    for(int choice=CHOICE_DX7;choice<=CHOICE_CUSTOM_FM;choice++) {
        CHECK(channel_replace_choice(&p,0,choice));player_reset(&a);player_reset(&b);
        on(&b,0,60);b.voices[0].fm.envelope=.5f;
        render_mixer(&a,&b,&p,empty,whole,128,0,0);on(&b,1,72);
        render_mixer(&a,&b,&p,empty,whole,128,0,0);CHECK(b.voices[1].selected && !b.voices[0].selected);
        player_voice_off(&b,&p,1);render_mixer(&a,&b,&p,empty,whole,128,0,0);CHECK(b.voices[0].selected);
        for(int i=0;i<256;i++)CHECK(isfinite(whole[i]));
    }
    /* Policy switches preserve pitch and distinguish held keys from tails. */
    p.voice[0]=(VoiceSettings){0};player_reset(&a);on(&a,0,60);player_voice_off(&a,&p,0);CHECK(a.voices[0].held==-1);
    p.voice[0]=(VoiceSettings){.mono=1,.legato=1,.glide=.01f};player_reset(&a);player_reset(&b);on(&b,0,60);on(&b,1,72);
    p.voice[0].glide=0;player_voice_tick(&b,&p);CHECK(b.voices[1].current_pitch==72 && !b.voices[1].glide_left);
    p.voice[0].mono=0;render_mixer(&a,&b,&p,empty,whole,1,0,0);CHECK(b.voices[1].current_pitch==72 && player_voice_pitch(&b.voices[1])==2);
    /* Rapid release/repress in one callback stays legato over a held anchor. */
    p.voice[0]=(VoiceSettings){.mono=1,.legato=1};player_reset(&a);player_reset(&b);on(&b,0,60);
    render_mixer(&a,&b,&p,empty,whole,256,0,0);
    for(int i=0;i<100;i++){
        on(&b,1,64+i%12);render_mixer(&a,&b,&p,empty,whole,32,0,0);
        FMVoice before=b.voices[1].fm;
        player_voice_off(&b,&p,1);on(&b,1,67+i%12);
        CHECK(b.voices[1].fm.envelope==before.envelope && b.voices[1].fm.lfo_age==before.lfo_age && b.voices[1].fm.stage==before.stage);
        CHECK(b.voices[1].current_pitch==67+i%12 && !b.voices[1].glide_left);
    }
    /* Same-buffer replacement also keeps a held anchor in the other pool. */
    player_reset(&a);player_reset(&b);on(&a,0,60);b.voice_order=a.voice_order;on(&b,1,67);player_voice_tick_pair(&a,&b,&p);
    b.voices[1].fm.envelope=.72f;b.voices[1].fm.lfo_age=.5f;player_voice_off(&b,&p,1);on(&b,2,72);player_voice_tick_pair(&a,&b,&p);
    CHECK(b.voices[2].fm.envelope==.72f && b.voices[2].fm.lfo_age==.5f && b.voices[2].selected);
    /* Shared Mono pitch bypasses the synth's legacy smoothing. */
    CHECK(channel_replace_choice(&p,0,CHOICE_ANALOG));p.voice[0]=(VoiceSettings){.mono=1,.legato=1};player_reset(&a);player_reset(&b);
    on(&b,0,60);render_mixer(&a,&b,&p,empty,whole,256,0,0);on(&b,1,72);render_mixer(&a,&b,&p,empty,whole,1,0,0);
    CHECK(fabs(b.voices[1].fm.current_frequency*b.voices[1].fm.note_pitch_ratio-523.2511306011972)<1e-6);
    /* Reusing a nonzero Analog voice keeps a 5 ms retiring overlap. */
    p.voice[0]=(VoiceSettings){0};p.fm[0].attack=.25f;player_reset(&a);player_reset(&b);on(&b,0,60);
    for(int i=0;i<100;i++)render_mixer(&a,&b,&p,empty,whole,256,0,0);
    FMVoice continuing=b.voices[0].fm;float expected_sample=fm_sample(&continuing,p.fm[0],1)*.8f*p.volume[0]*p.master;
    on(&b,0,60);int retiring=-1;for(int i=0;i<128;i++)if(b.voices[i].fade_out)retiring=i;
    CHECK(retiring>=0 && b.voices[retiring].fade_out==240 && b.voices[retiring].fm.envelope>.5f && b.voices[0].fm.envelope==0);
    whole[0]=whole[1]=0; /* Live mixer adds to its caller-owned output buffer. */
    render_mixer(&a,&b,&p,empty,whole,1,0,0);CHECK(fabsf(whole[0]-expected_sample)<.0002f);
    render_mixer(&a,&b,&p,empty,whole,239,0,0);CHECK(!b.voices[retiring].gain);
    /* Shared settings follow channels when one is deleted. */
    p.channel_count=2;p.voice[1]=(VoiceSettings){.limit=3,.glide=.3f};CHECK(channel_delete(&p,0));CHECK(p.voice[0].limit==3 && p.voice[0].glide==.3f);
    return 0;
}
