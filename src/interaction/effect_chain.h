// SPDX-License-Identifier: GPL-3.0-only
#ifndef LIBRELOOP_EFFECT_CHAIN_H
#define LIBRELOOP_EFFECT_CHAIN_H
#include "engine.h"
typedef struct {int count,slots[EFFECT_SLOTS],add_slot;} EffectChainList;
static inline EffectChainList effect_chain_list(const Project *p,int bus) {
    EffectChainList list={.add_slot=-1};
    for(int slot=0;slot<EFFECT_SLOTS;slot++) {
        if(p->effect_type[bus][slot])list.slots[list.count++]=slot;
        else if(list.add_slot<0 && slot<9)list.add_slot=slot;
    }
    return list;
}
int effect_slot_after_move(int slot,int from,int to);
void effect_target_after_move(ParameterTarget *target,int bus,int from,int to);
int effect_chain_move(Project *project,int bus,int from,int to);
#endif
