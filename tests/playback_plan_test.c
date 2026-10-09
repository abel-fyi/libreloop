// SPDX-License-Identifier: GPL-3.0-only
#include "playback_plan.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#define CHECK(x) do { if(!(x)) { fprintf(stderr,"Failed line %d: %s\n",__LINE__,#x); return 1; } } while(0)
static Project p;
static ProjectSnapshot snapshot;
static Player uncached,cached;
static Sample samples[CHANNELS];
static float pcm[RATE*2],a[1024],b[1024];
static float peak_a[INSERTS+1][2],peak_b[INSERTS+1][2];
static void input(void *unused,float buses[INSERTS+1][2]) { (void)unused; buses[7][0]+=.013f; buses[7][1]-=.009f; }
static int compare(const MixerIO *io,unsigned frames) {
    memset(a,0,sizeof a); memset(b,0,sizeof b);
    render_mixer_io(&uncached,NULL,&p,samples,a,frames,1,peak_a,io);
    render_mixer_io(&cached,NULL,&p,samples,b,frames,1,peak_b,io);
    return !memcmp(a,b,frames*2*sizeof(float)) && !memcmp(peak_a,peak_b,sizeof peak_a) && uncached.frame==cached.frame &&
        !memcmp(uncached.lane_active,cached.lane_active,sizeof cached.lane_active) && !memcmp(uncached.channel_trigger,cached.channel_trigger,sizeof cached.channel_trigger);
}
int main(void) {
    project_demo(&p);
    for(unsigned i=0;i<RATE*2;i++) pcm[i]=.05f*sinf(i*.021f);
    for(int c=0;c<CHANNELS;c++) samples[c]=(Sample){pcm,RATE*2,1};
    p.channel_audio[3]=1; p.audio_seconds[3]=2; p.sampler[3].fit_bpm=120;
    p.clips[3][0]=PATTERNS+4; p.clip_steps[3][0]=.75f; p.clip_offsets[3][0]=.125f;
    p.instrument[0]=INSTRUMENT_FM; p.fm[0]=fm_epiano();
    CHECK(insert_connect(&p,1,7)); CHECK(insert_connect(&p,7,9));
    p.effect_type[7][0]=EFFECT_EQ; p.eq[7][0].bands[3].gain=3;
    p.effect_type[9][0]=EFFECT_CHORUS; p.effect_mix[9][0]=.3f;
    int curve=automation_create(&p,(ParameterTarget){PARAM_CHANNEL_PAN,0,0},"Pan",16); CHECK(curve>=0);
    p.clips[4][0]=AUTOMATION_SOURCE+curve+1;
    CHECK(project_snapshot_update(&snapshot,&p,1,NULL)==(SNAPSHOT_PROJECT|SNAPSHOT_PLAN));
    CHECK(snapshot.revision==1 && snapshot.plan.song_end==song_steps(&p));
    for(int n=0;n<100;n++) CHECK(!project_snapshot_update(&snapshot,&p,1,NULL) && snapshot.revision==1);
    player_reset(&uncached); player_reset(&cached); uncached.song=cached.song=1;
    uncached.effects=effects_create(INSERTS+1); cached.effects=effects_create(INSERTS+1); CHECK(uncached.effects && cached.effects);
    cached.plan=&snapshot.plan;
    double power=0;
    for(int n=0;n<300;n++) {
        if(n==45) { p.bpm=177; p.clip_steps[3][0]=.375f; p.clip_offsets[3][0]=.25f; uncached.audio_resync=cached.audio_resync=1; }
        if(n==90) { p.insert_mute[6]=2; p.pan[0]=-.75f; p.lane_mute[2]=1; }
        if(n==140) { CHECK(insert_connect(&p,1,9)); p.effect_type[7][0]=EFFECT_EMPTY; }
        if(n==200) { p.insert_mute[6]=0; p.master_pitch=3; p.pattern_steps[0]=32; }
        if(n==250) p.clips[4][0]=0;
        int changed=project_snapshot_update(&snapshot,&p,1,NULL);
        if(n==45 || n==90 || n==140 || n==200 || n==250) CHECK(changed==(SNAPSHOT_PROJECT|SNAPSHOT_PLAN)); else CHECK(!changed);
        CHECK(snapshot.plan.song_end==song_steps(&p));
        CHECK(compare(NULL,n%2?64:512)); for(unsigned i=0;i<1024;i++) power+=a[i]*a[i];
    }
    CHECK(power>0 && snapshot.revision==6);
    MixerIO io={.input=input,.monitor_only=1}; io.active[7]=1;
    CHECK(project_snapshot_update(&snapshot,&p,1,&io)==SNAPSHOT_PLAN && snapshot.revision==7);
    CHECK(!playback_plan_compatible(&snapshot.plan,1,NULL,1));
    for(int n=0;n<10;n++) CHECK(compare(&io,512));
    /* A changed recording context safely falls back until the UI publishes its next plan. */
    for(int n=0;n<10;n++) CHECK(compare(NULL,64));
    CHECK(project_snapshot_update(&snapshot,&p,1,NULL)==SNAPSHOT_PLAN);
    effects_free(uncached.effects); effects_free(cached.effects);
    player_reset(&cached); CHECK(!cached.plan);
    puts("Revisioned snapshots, cached/uncached PCM, routing/tails, edits, automation, tempo and recording context passed."); return 0;
}
