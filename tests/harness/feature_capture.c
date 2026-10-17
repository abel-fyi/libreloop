/* SPDX-License-Identifier: GPL-3.0-only
 * Phase 0 harness only. Links the unmodified frozen original libengine.a. */
#include "engine.h"
#include "playback_plan.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
static Project p;
static Player seq,live;
static Sample source[CHANNELS],processed[CHANNELS];
static PlaybackPlan plan;
int main(int argc,char **argv) {
    if(argc!=3) return 1;
    int mode=atoi(argv[1]); unsigned block=(unsigned)atoi(argv[2]);
    if(mode<0 || mode>7 || (block!=64 && block!=512)) return 1;
    project_new(&p); p.channel_count=2; p.bpm=123.5f;
    for(int c=0;c<2;c++) {
        source[c]=(Sample){.data=calloc(48000,2*sizeof(float)),.frames=48000,.channels=2};
        if(!source[c].data) return 2;
        for(unsigned i=0;i<48000;i++) {
            source[c].data[i*2]=i%97<2?.25f:(int)(i%127)*.0005f-.03f;
            source[c].data[i*2+1]=i%83<3?-.2f:(int)(i%109)*.0003f-.02f;
        }
        p.volume[c]=.4f; p.pan[c]=c?.25f:-.3f; p.route[c]=c+1;
        p.notes[0][c][0]=(Note){60+c*7,100,.25f,2.5f};
        p.notes[0][c][1]=(Note){64+c*7,64,4.125f,3.75f};
        p.notes[0][c][2]=(Note){67+c*7,127,12.5f,0};
    }
    p.clips[0][0]=1; p.clip_steps[0][0]=16;
    p.clips[0][1]=1; p.clip_starts[0][1]=1; p.clip_steps[0][1]=8; p.clip_offsets[0][1]=4;
    p.insert_output[0]=2; p.insert_volume[0]=.8f; p.insert_volume[1]=.7f; p.insert_width[1]=1.5f;
    p.effect_type[2][0]=EFFECT_CHORUS;p.effect_mix[2][0]=.3f;
    p.effect_type[0][0]=EFFECT_EQ;p.effect_mix[0][0]=.8f;p.eq[0][0].bands[2].gain=4;
    if(mode==1) p.swing=.7f;
    if(mode==2 || mode==3) {
        p.channel_audio[0]=1; p.audio_seconds[0]=1;
        p.clips[0][0]=PATTERNS+1;p.clip_steps[0][0]=.5f;p.clip_offsets[0][0]=.125f;
        p.clips[0][1]=PATTERNS+1;p.clip_steps[0][1]=0;p.clip_offsets[0][1]=.25f;
        int a=automation_create(&p,(ParameterTarget){PARAM_CHANNEL_PITCH,0,0},"Pitch",24);
        p.automations[a].points[0].value=.5f;p.automations[a].points[1].value=.8f;
        p.clips[2][0]=AUTOMATION_SOURCE+a+1;p.clip_steps[2][0]=24;
        if(mode==3) {p.sampler[0].fit_bpm=110;p.sampler[0].stretch=1;}
    }
    if(mode==4) {
        p.sampler[0]=(Sampler){.pitch=3,.time=1.25f,.start=.125f,.length=.75f,.trim=.2f,.stretch=1,.flags=7};
        p.master_pitch=-2;p.channel_pitch[1]=.3f;p.pitch_range[1]=12;
    }
    if(mode==5) {
        p.instrument[1]=INSTRUMENT_FM;p.fm[1]=fm_legacy();p.fm[1].depth=3;
        int a=automation_create(&p,(ParameterTarget){PARAM_CHANNEL_VOLUME,1,0},"Volume",24);
        p.automations[a].points[0].value=.1f;p.automations[a].points[1].value=.6f;
        p.clips[2][0]=AUTOMATION_SOURCE+a+1;p.clip_steps[2][0]=24;
    }
    if(mode==6) {p.instrument[1]=INSTRUMENT_FM;p.fm[1]=fm_factory(0);}
    if(mode==7) {p.instrument[1]=INSTRUMENT_FM;p.fm[1]=fm_factory(5);}
    for(int c=0;c<2;c++) if(!sample_process(source[c],p.sampler[c],&processed[c])) return 3;
    if(!project_save("input.llp",&p) || !export_wav("expected-export.wav",&p,processed)) return 4;
    for(int c=0;c<2;c++) {char path[32];snprintf(path,sizeof path,"source-%d.f32",c);FILE *f=fopen(path,"wb");if(!f)return 5;fwrite(source[c].data,sizeof(float),source[c].frames*2,f);fclose(f);}
    player_reset(&seq);player_reset(&live);seq.song=1;seq.effects=live.effects=effects_create(INSERTS+1);
    if(!seq.effects)return 6;
    playback_plan_prepare(&plan,&p,1,NULL,1);seq.plan=live.plan=&plan;
    if(mode==1) {player_seek(&seq,&p,1.5f);seq.loop_start=1.5f;seq.loop_end=10.25f;seq.plan=&plan;}
    live.voices[0]=(Voice){.channel=0,.gain=.1f,.remaining=2400,.lane=-1};sampler_voice_reset(&live.voices[0].sampler,0,1);
    FILE *out=fopen("expected.f32","wb"),*state=fopen("expected-state.txt","w");if(!out || !state)return 7;
    float pcm[1024],peaks[INSERTS+1][2];
    for(unsigned at=0;at<144000;at+=block) {
        unsigned n=144000-at<block?144000-at:block;
        if(at==16384) {p.bpm=80;p.master_pitch=2;playback_plan_prepare(&plan,&p,1,NULL,1);}
        if(at==32768 && (mode==2 || mode==3)) {p.clip_offsets[0][0]=.25f;seq.audio_resync=1;playback_plan_prepare(&plan,&p,1,NULL,1);}
        memset(pcm,0,sizeof pcm);render_mixer_io(&seq,&live,&p,processed,pcm,n,1,peaks,NULL);
        if(fwrite(pcm,2*sizeof(float),n,out)!=n)return 8;
        fprintf(state,"%u %llu %lld %.9g\n",at+n,(unsigned long long)seq.frame,(long long)seq.last_step,seq.clock_bpm);
    }
    fclose(out);fclose(state);effects_free(seq.effects);
    for(int c=0;c<2;c++){sample_free(source[c]);sample_free(processed[c]);}
    return 0;
}
