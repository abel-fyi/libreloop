// SPDX-License-Identifier: GPL-3.0-only
#include "playback_plan.h"
#include <string.h>
#include <math.h>
static int insert_solo(const Project *p) {
    for(int i=0;i<p->insert_count;i++) if(p->insert_mute[i]&2) return 1;
    return 0;
}
int playback_plan_compatible(const PlaybackPlan *plan,int effects,const MixerIO *io,int geometry) {
    static const uint8_t silent[INSERTS+1]={0};
    return plan && plan->effects==!!effects && (!geometry || plan->geometry) &&
        !memcmp(plan->io_active,io?io->active:silent,sizeof plan->io_active);
}
void playback_plan_prepare(PlaybackPlan *plan,const Project *p,int effects,const MixerIO *io,int geometry) {
    /* Pattern/live fallback rendering does not use clip geometry. */
    memset((char *)plan+offsetof(PlaybackPlan,song_end),0,sizeof *plan-offsetof(PlaybackPlan,song_end));
    if(geometry) memset(plan->lengths,0,sizeof plan->lengths);
    plan->effects=!!effects; plan->geometry=!!geometry; plan->song_end=STEPS;
    if(io) memcpy(plan->io_active,io->active,sizeof plan->io_active);
    if(geometry) for(int l=0;l<LANES;l++) for(int b=0;b<CLIPS;b++) if(p->clips[l][b]) {
        plan->lengths[l][b]=clip_length(p,l,b);
        plan->song_end=fmaxf(plan->song_end,p->clip_starts[l][b]*STEPS+plan->lengths[l][b]);
    }
    int solo=insert_solo(p),used[INSERTS+1]={1},pending[INSERTS+1]={0};
    memset(plan->input_audible,1,sizeof plan->input_audible);
    memset(plan->effect_audible,1,sizeof plan->effect_audible);
    if(io) for(int bus=0;bus<=p->insert_count;bus++) if(io->active[bus]) {
        int id=bus,hops=0,audible=!solo;
        while(id && id!=255 && hops++<INSERTS) { used[id]=1; audible|=p->insert_mute[id-1]&2; id=p->insert_output[id-1]; }
        plan->input_audible[bus]=audible;
    }
    for(int c=0;c<p->channel_count;c++) {
        int id=p->route[c],hops=0,audible=!solo;
        while(id && id!=255 && hops++<INSERTS) { used[id]=1; audible|=p->insert_mute[id-1]&2; id=p->insert_output[id-1]; }
        plan->channel_audible[c]=audible;
    }
    for(int bus=0;effects && bus<=p->insert_count;bus++) {
        int active=0; for(int slot=0;slot<EFFECT_SLOTS;slot++) active|=p->effect_type[bus][slot]!=EFFECT_EMPTY;
        if(!active) continue;
        int id=bus,hops=0,audible=!solo || bus==0;
        while(id && id!=255 && hops++<INSERTS) { used[id]=1; audible|=p->insert_mute[id-1]&2; id=p->insert_output[id-1]; }
        plan->effect_audible[bus]=audible;
    }
    for(int id=1;id<=p->insert_count;id++) if(used[id] && p->insert_output[id-1]!=255) pending[p->insert_output[id-1]]++;
    for(int id=0;id<=p->insert_count;id++) if(used[id] && !pending[id]) plan->order[plan->buses_count++]=id;
    for(int at=0;at<plan->buses_count;at++) {
        int id=plan->order[at]; if(!id) continue;
        int dest=p->insert_output[id-1];
        if(dest!=255 && !--pending[dest]) plan->order[plan->buses_count++]=dest;
    }
}
int project_snapshot_update(ProjectSnapshot *state,const Project *p,int effects,const MixerIO *io) {
    int changed=!state->revision || memcmp(&state->project,p,sizeof *p),flags=0;
    if(changed) { state->project=*p; flags|=SNAPSHOT_PROJECT; }
    if(changed || !playback_plan_compatible(&state->plan,effects,io,1)) {
        playback_plan_prepare(&state->plan,p,effects,io,1); flags|=SNAPSHOT_PLAN;
    }
    if(flags) state->revision++;
    return flags;
}
