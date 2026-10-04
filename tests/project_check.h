// SPDX-License-Identifier: GPL-3.0-only
#ifndef PROJECT_CHECK_H
#define PROJECT_CHECK_H
#include "engine.h"
#include <string.h>
#include <stdio.h>
/* Struct padding and bytes after string terminators are not project data. */
static int project_equal(const Project *a,const Project *b) {
#define FIELD(member) do { if(memcmp(&a->member,&b->member,sizeof a->member)) { fprintf(stderr,"Project field differs: %s\n",#member); return 0; } } while(0)
#define STRING(member) do { if(strcmp(a->member,b->member)) { fprintf(stderr,"Project string differs: %s\n",#member); return 0; } } while(0)
    FIELD(bpm); FIELD(pattern_count); FIELD(channel_count);
    FIELD(volume); FIELD(pan); FIELD(master); FIELD(channel_pitch); FIELD(pitch_range); FIELD(mute);
    for(int p=0;p<PATTERNS;p++) for(int c=0;c<CHANNELS;c++) for(int n=0;n<NOTES;n++) {
        FIELD(notes[p][c][n].pitch); FIELD(notes[p][c][n].velocity);
        FIELD(notes[p][c][n].start); FIELD(notes[p][c][n].length);
    }
    FIELD(clips); FIELD(clip_steps); FIELD(clip_starts); FIELD(clip_offsets);
    FIELD(channel_audio); FIELD(audio_seconds); FIELD(pattern_steps); FIELD(pattern_colors); FIELD(channel_colors);
    for(int c=0;c<CHANNELS;c++) { STRING(paths[c]); STRING(channel_names[c]); }
    for(int p=0;p<PATTERNS;p++) STRING(pattern_names[p]);
    FIELD(insert_count); FIELD(route); FIELD(insert_mute); FIELD(insert_volume); FIELD(insert_pan); FIELD(insert_output);
    for(int c=0;c<CHANNELS;c++) {
        FIELD(sampler[c].pitch); FIELD(sampler[c].time); FIELD(sampler[c].start);
        FIELD(sampler[c].length); FIELD(sampler[c].trim); FIELD(sampler[c].flags); FIELD(sampler[c].stretch); FIELD(sampler[c].fit_bpm);
    }
    FIELD(master_pitch); FIELD(insert_width); FIELD(master_width); FIELD(master_mute); FIELD(lane_mute);
    for(int i=0;i<=INSERTS;i++) for(int side=0;side<2;side++) STRING(audio_io[i][side]);
    FIELD(effect_mix); FIELD(effect_bypass);
    for(int l=0;l<LANES;l++) STRING(track_names[l]);
    for(int i=0;i<INSERTS;i++) STRING(insert_names[i]);
    FIELD(swing); FIELD(automation_count);
    for(int i=0;i<a->automation_count;i++) {
        FIELD(automations[i].target.parameter); FIELD(automations[i].target.owner); FIELD(automations[i].target.slot);
        FIELD(automations[i].steps); FIELD(automations[i].count); FIELD(automations[i].color); STRING(automations[i].name);
        for(int n=0;n<a->automations[i].count;n++) { FIELD(automations[i].points[n].step); FIELD(automations[i].points[n].value); }
    }
#undef FIELD
#undef STRING
    return 1;
}
#endif
