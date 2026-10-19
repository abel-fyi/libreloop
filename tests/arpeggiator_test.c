// SPDX-License-Identifier: GPL-3.0-only
#include "engine.h"
#include <stdio.h>
#include <math.h>
#include <string.h>
#define CHECK(x) do{if(!(x)){fprintf(stderr,"%d: %s\n",__LINE__,#x);return 1;}}while(0)
static Project p,q;static Player a,b,live;static float out[48000],other[48000];
static void on(Player *v,int slot,int pitch){player_arp_on(v,&p,0,slot,pitch,.8f,-1,-1,0,0);}
static Voice *current(Player *v){int slot=v->arp.channels[0].slot;return slot?&v->voices[slot-1]:NULL;}
static void tick(int frames){for(int i=0;i<frames;i++){player_arp_tick(&a,&live,&p,p.fm);player_voice_tick_pair(&a,&live,&p);}}
int main(void){
    project_new(&p);CHECK(arp_valid(p.arp[0]));p.arp[0].direction=ARP_UP;p.arp[0].chord=ARP_AUTO;p.arp[0].time=.125f;p.arp[0].gate=1;p.bpm=120;
    on(&live,0,67);on(&live,1,60);on(&live,2,64);tick(1);CHECK(current(&a) && current(&a)->note_pitch==60);
    tick(750);CHECK(current(&a)->note_pitch==64);tick(750);CHECK(current(&a)->note_pitch==67);tick(750);CHECK(current(&a)->note_pitch==60);
    p.arp[0].direction=ARP_DOWN;player_reset(&a);tick(1);CHECK(current(&a)->note_pitch==67);
    const int updown[]={60,64,67,64,60},bounce[]={60,64,67,67,64,60,60};
    for(int mode=ARP_UP_DOWN;mode<=ARP_BOUNCE;mode++){p.arp[0].direction=mode;player_reset(&a);tick(1);for(int i=0;i<(mode==ARP_UP_DOWN?5:7);i++){if(i)tick(750);CHECK(current(&a)->note_pitch==(mode==ARP_UP_DOWN?updown[i]:bounce[i]));}}
    p.arp[0].direction=ARP_UP;p.arp[0].range=2;p.arp[0].repeat=2;player_reset(&a);tick(1);
    const int repeated[]={60,64,67,60,64,67,72,76,79};for(int i=0;i<9;i++){if(i)tick(750);CHECK(current(&a)->note_pitch==repeated[i]);}
    player_reset(&a);player_reset(&live);p.arp[0].range=p.arp[0].repeat=1;p.arp[0].chord=ARP_MAJOR;on(&live,0,60);tick(1);CHECK(current(&a)->note_pitch==60);tick(750);CHECK(current(&a)->note_pitch==64);tick(750);CHECK(current(&a)->note_pitch==67);
    player_reset(&a);player_reset(&live);p.arp[0].chord=ARP_AUTO_SUSTAIN;on(&live,0,60);tick(1);uint64_t order=current(&a)->order;tick(3000);CHECK(current(&a)->order==order);CHECK(player_arp_off(&live,0));tick(1);CHECK(!current(&a));
    /* Auto sustain does not restart a finished one-shot while its key is held. */
    player_reset(&a);player_reset(&live);float tiny_pcm[16]={1};Sample tiny[CHANNELS]={0};tiny[0]=(Sample){.data=tiny_pcm,.frames=16,.channels=1};on(&live,0,60);
    render_mixer(&a,&live,&p,tiny,out,1,0,NULL);order=a.voice_order;
    render_mixer(&a,&live,&p,tiny,out,100,0,NULL);CHECK(a.voice_order==order && live.arp.count==1);
    /* Gate boundaries, deterministic random order and shared live/song chords. */
    player_reset(&a);player_reset(&live);p.arp[0].chord=ARP_AUTO;p.arp[0].gate=.5f;on(&live,0,60);tick(375);CHECK(current(&a));tick(1);CHECK(!current(&a));
    p.arp[0].gate=1;p.arp[0].direction=ARP_RANDOM;
    int random_notes[12];for(int replay=0;replay<2;replay++){
        player_reset(&a);player_reset(&live);on(&live,0,60);on(&live,1,64);on(&a,100,67);tick(1);
        for(int i=0;i<12;i++){if(i)tick(750);int pitch=current(&a)->note_pitch;CHECK(pitch==60 || pitch==64 || pitch==67);if(!replay)random_notes[i]=pitch;else CHECK(pitch==random_notes[i]);}
    }
    /* Slide uses the same Glide time, without needing Mono or retriggering ADSR. */
    CHECK(channel_replace_choice(&p,0,CHOICE_ANALOG));p.arp[0].direction=ARP_UP;p.arp[0].slide=1;p.voice[0].glide=.01f;
    player_reset(&a);player_reset(&live);on(&live,0,60);on(&live,1,72);tick(1);Voice *sliding=current(&a);sliding->fm.envelope=.6f;tick(750);
    CHECK(current(&a)==sliding && sliding->fm.envelope==.6f && sliding->note_pitch==72 && sliding->current_pitch==60 && sliding->glide_left==480);
    for(int i=0;i<480;i++)player_voice_pitch(sliding);CHECK(sliding->current_pitch==72);
    /* Off restores held live keys at their real key slots, so key-up still works. */
    p.arp[0].direction=ARP_OFF;tick(1);CHECK(!live.arp.count && live.voices[0].gain && live.voices[1].gain && !current(&a));
    player_voice_off(&live,&p,0);CHECK(live.voices[0].held==-1);
    p.arp[0].direction=ARP_UP;p.arp[0].slide=0;p.voice[0].glide=0;
    /* Actual synthesis, gates, stop/reset, persistence and block replay. */
    CHECK(channel_replace_choice(&p,0,CHOICE_ANALOG));p.arp[0].chord=ARP_AUTO;p.arp[0].gate=.5f;p.arp[0].time=1.f/3;
    memset(p.notes,0,sizeof p.notes);p.notes[0][0][0]=(Note){60,100,0,4};p.notes[0][0][1]=(Note){64,90,0,4};p.notes[0][0][2]=(Note){67,80,0,4};
    Sample empty[CHANNELS]={0};player_reset(&a);player_reset(&b);render(&a,&p,empty,out,24000);
    for(int i=0;i<24000;){int n=i+37<24000?37:24000-i;render(&b,&p,empty,other+i*2,n);i+=n;}CHECK(!memcmp(out,other,sizeof out));double energy=0;for(int i=0;i<48000;i++){CHECK(isfinite(out[i]));energy+=out[i]*out[i];}CHECK(energy>.01);
    /* Shared note generation works through all four instrument engines. */
    float source_pcm[1024];for(int i=0;i<1024;i++)source_pcm[i]=.1f*sinf(i*.1f);
    Sample sources[CHANNELS]={0};sources[0]=(Sample){.data=source_pcm,.frames=1024,.channels=1};
    for(int choice=CHOICE_SAMPLER;choice<INSTRUMENT_CHOICES;choice++){
        CHECK(channel_replace_choice(&p,0,choice));player_reset(&a);player_reset(&live);on(&live,0,60);on(&live,1,67);
        render_mixer(&a,&live,&p,sources,out,4096,0,NULL);double sum=0;for(int i=0;i<8192;i++){CHECK(isfinite(out[i]));sum+=out[i]*out[i];}CHECK(sum>1e-6 && a.arp.channels[0].running);
        CHECK(player_arp_off(&live,0) && player_arp_off(&live,1));render_mixer(&a,&live,&p,sources,out,1,0,NULL);CHECK(!a.arp.channels[0].running && !live.arp.count);
    }
    CHECK(project_save("arp.llp",&p) && project_load("arp.llp",&q) && !memcmp(p.arp,q.arp,sizeof p.arp));remove("arp.llp");
    q.arp[0].time=NAN;CHECK(!project_save("invalid-arp.llp",&q));
    player_seek(&a,&p,0);CHECK(!a.arp.count && !a.arp.channels[0].running);
    p.channel_count=2;p.arp[1]=p.arp[0];ArpSettings expected=p.arp[1];CHECK(channel_delete(&p,0) && !memcmp(&p.arp[0],&expected,sizeof expected));
    puts("Arpeggiator directions, held chord ordering, range/repeat, templates, sustain, note-off, triplet replay and persistence passed.");return 0;
}
