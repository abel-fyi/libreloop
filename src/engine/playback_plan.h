// SPDX-License-Identifier: GPL-3.0-only
#ifndef PLAYBACK_PLAN_H
#define PLAYBACK_PLAN_H
#include "engine.h"
/* Derived geometry/routing only. Caller prepares after project edits and owns
   storage for the duration of rendering. No DSP state or allocations here.
   geometry=0 leaves lengths unused; rendering only reads them in Song mode. */
struct PlaybackPlan {
    float lengths[LANES][CLIPS],song_end;
    uint8_t io_active[INSERTS+1],input_audible[INSERTS+1],effect_audible[INSERTS+1],channel_audible[CHANNELS];
    int order[INSERTS+1],buses_count,effects,geometry;
};
void playback_plan_prepare(PlaybackPlan *plan,const Project *p,int effects,const MixerIO *io,int geometry);
int playback_plan_compatible(const PlaybackPlan *plan,int effects,const MixerIO *io,int geometry);
/* Zero-initialize once. Revisions advance only on actual project/context changes;
   the callback receives coherent project + plan snapshots under its mailbox lock. */
enum { SNAPSHOT_PROJECT=1,SNAPSHOT_PLAN=2 };
typedef struct { Project project; PlaybackPlan plan; uint64_t revision; } ProjectSnapshot;
int project_snapshot_update(ProjectSnapshot *state,const Project *p,int effects,const MixerIO *io);
#endif
