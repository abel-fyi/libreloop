// SPDX-License-Identifier: GPL-3.0-only
#include "effect_chain.h"
#include <string.h>
int effect_slot_after_move(int slot,int from,int to) {
    if(slot==from)return to;
    if(from<to && slot>from && slot<=to)return slot-1;
    if(to<from && slot>=to && slot<from)return slot+1;
    return slot;
}
void effect_target_after_move(ParameterTarget *target,int bus,int from,int to) {
    if(target->owner==(unsigned)bus && parameter_is_effect(target->parameter))target->slot=effect_slot_after_move(target->slot,from,to);
}
int effect_chain_move(Project *p,int bus,int from,int to) {
    if(bus<0 || bus>p->insert_count || from<0 || from>=EFFECT_SLOTS || to<0 || to>=EFFECT_SLOTS || from==to || !p->effect_type[bus][from])return 0;
    int step=from<to?1:-1;
    for(int at=from;at!=to;at+=step) {
        int next=at+step;
#define SWAP(field,type) do {type value=p->field[bus][at];p->field[bus][at]=p->field[bus][next];p->field[bus][next]=value;}while(0)
        SWAP(effect_type,uint8_t);SWAP(effect_bypass,uint8_t);SWAP(effect_mix,float);
        SWAP(chorus,ChorusSettings);SWAP(eq,EQSettings);SWAP(delay,DelaySettings);SWAP(tube,TubeSettings);SWAP(reverb,ReverbSettings);
#undef SWAP
    }
    for(int i=0;i<p->automation_count+p->midi_binding_count;i++) {
        ParameterTarget *target=i<p->automation_count?&p->automations[i].target:&p->midi_bindings[i-p->automation_count].target;
        effect_target_after_move(target,bus,from,to);
    }
    p->effect_revision[bus]++;return 1;
}
