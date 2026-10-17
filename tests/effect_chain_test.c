// SPDX-License-Identifier: GPL-3.0-only
#include "effect_chain.h"
#include <stdio.h>
#include <string.h>
#define CHECK(x) do{if(!(x)){fprintf(stderr,"%d: %s\n",__LINE__,#x);return 1;}}while(0)
static Project project,before,loaded;
int main(void) {
    project_new(&project);
    for(int slot=0;slot<4;slot++) {
        project.effect_type[1][slot]=slot%2?EFFECT_EQ:EFFECT_CHORUS;
        project.chorus[1][slot].rate=.5f+slot;
        project.eq[1][slot].bands[0].gain=slot;
        project.effect_mix[1][slot]=slot*.2f;project.effect_bypass[1][slot]=slot%2;
    }
    project.automation_count=3;
    project.automations[0].target=(ParameterTarget){PARAM_CHORUS_RATE,1,0};
    project.automations[1].target=(ParameterTarget){PARAM_EQ_FIRST,1,1};
    project.automations[2].target=(ParameterTarget){PARAM_CHANNEL_VOLUME,1,0};
    project.midi_binding_count=2;
    project.midi_bindings[0].target=(ParameterTarget){PARAM_EFFECT_MIX,1,2};
    project.midi_bindings[1].target=(ParameterTarget){PARAM_EQ_FIRST,2,1};
    before=project;
    CHECK(effect_chain_move(&project,1,0,3));
    for(int slot=0;slot<4;slot++) {
        int to=effect_slot_after_move(slot,0,3);
        CHECK(project.effect_type[1][to]==before.effect_type[1][slot]);
        CHECK(!memcmp(&project.chorus[1][to],&before.chorus[1][slot],sizeof(ChorusSettings)));
        CHECK(!memcmp(&project.eq[1][to],&before.eq[1][slot],sizeof(EQSettings)));
        CHECK(project.effect_mix[1][to]==before.effect_mix[1][slot] && project.effect_bypass[1][to]==before.effect_bypass[1][slot]);
    }
    CHECK(project.automations[0].target.slot==3 && project.automations[1].target.slot==0 && project.automations[2].target.slot==0);
    CHECK(project.midi_bindings[0].target.slot==1 && project.midi_bindings[1].target.slot==1);
    CHECK(project.effect_revision[1]==1 && project.effect_revision[2]==0);
    CHECK(effect_chain_move(&project,1,3,0));
    project.effect_revision[1]=0;CHECK(!memcmp(&project,&before,sizeof project));
    CHECK(!effect_chain_move(&project,1,0,0) && !effect_chain_move(&project,1,4,0) && !effect_chain_move(&project,1,0,10));
    for(int from=0;from<10;from++)for(int to=0;to<10;to++) {
        int seen[10]={0};for(int slot=0;slot<10;slot++)seen[effect_slot_after_move(slot,from,to)]++;
        for(int slot=0;slot<10;slot++)CHECK(seen[slot]==1);
    }
    puts("FX insertion order, complete settings, automation/MIDI retargeting, inverse moves and chain invalidation passed.");return 0;
}
