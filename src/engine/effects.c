// SPDX-License-Identifier: GPL-3.0-only
#include "effects.h"
#include <stdlib.h>
#include <stdatomic.h>
_Static_assert(ATOMIC_POINTER_LOCK_FREE==2,"Effect preparation needs lock-free pointer publication");
struct EffectSlot { union { Chorus chorus; Equalizer eq; Tube tube; }; _Atomic(Delay *) delay; DelaySettings delay_settings; TubeSettings tube_settings; unsigned generation; uint8_t type; };
struct EffectRack { unsigned buses,generation; struct EffectSlot slots[]; };
EffectRack *effects_create(unsigned buses) {
    if(!buses || buses>101) return NULL;
    EffectRack *rack=calloc(1,sizeof *rack+buses*EFFECT_SLOTS*sizeof *rack->slots);
    if(rack) { rack->buses=buses; rack->generation=1; for(unsigned i=0;i<buses*EFFECT_SLOTS;i++)atomic_init(&rack->slots[i].delay,NULL); } return rack;
}
int effects_prepare(EffectRack *rack,const uint8_t *types,unsigned count) {
    if(!rack || !types || count>rack->buses*EFFECT_SLOTS)return 0;
    for(unsigned i=0;i<count;i++)if(types[i]==EFFECT_DELAY && !atomic_load_explicit(&rack->slots[i].delay,memory_order_acquire)) {
        Delay *d=delay_create();if(!d)return 0;
        atomic_store_explicit(&rack->slots[i].delay,d,memory_order_release);
    }
    return 1;
}
void effects_free(EffectRack *rack) {
    if(rack)for(unsigned i=0;i<rack->buses*EFFECT_SLOTS;i++)delay_free(atomic_load_explicit(&rack->slots[i].delay,memory_order_relaxed));
    free(rack);
}
void effects_reset(EffectRack *rack) { if(rack) rack->generation++; }
void effects_reset_bus(EffectRack *rack,unsigned bus) {
    if(rack && bus<rack->buses)for(int slot=0;slot<EFFECT_SLOTS;slot++)rack->slots[bus*EFFECT_SLOTS+slot].generation=0;
}
void effects_process_all(EffectRack *rack,unsigned bus,const uint8_t types[EFFECT_SLOTS],const ChorusSettings settings[EFFECT_SLOTS],const EQSettings eq[EFFECT_SLOTS],const DelaySettings delays[EFFECT_SLOTS],const TubeSettings tubes[EFFECT_SLOTS],float bpm,const float mixes[EFFECT_SLOTS],const uint8_t bypass[EFFECT_SLOTS],float stereo[2]) {
    if(!rack || bus>=rack->buses) return;
    for(int i=0;i<EFFECT_SLOTS;i++) {
        struct EffectSlot *slot=&rack->slots[bus*EFFECT_SLOTS+i];
        if(!types[i]) {
            if(slot->type==EFFECT_CHORUS && slot->generation==rack->generation && slot->chorus.wet>.00001f)
                chorus_process(&slot->chorus,settings[i],0,stereo);
            else if(slot->type==EFFECT_EQ && slot->generation==rack->generation && slot->eq.wet>.00001f)
                equalizer_process(&slot->eq,slot->eq.target,0,stereo);
            else if(slot->type==EFFECT_DELAY && slot->generation==rack->generation && delay_audible(atomic_load_explicit(&slot->delay,memory_order_acquire)))
                delay_process(atomic_load_explicit(&slot->delay,memory_order_acquire),slot->delay_settings,bpm,0,stereo);
            else if(slot->type==EFFECT_TUBE && slot->generation==rack->generation && slot->tube.wet>.00001f)
                tube_process(&slot->tube,slot->tube_settings,0,stereo);
            else slot->type=0;
            continue;
        }
        if(slot->type!=types[i] || slot->generation!=rack->generation) {
            if(types[i]==EFFECT_TUBE)tube_reset(&slot->tube);
            else if(types[i]==EFFECT_DELAY) delay_reset(atomic_load_explicit(&slot->delay,memory_order_acquire));
            else if(types[i]==EFFECT_EQ) equalizer_reset(&slot->eq); else chorus_reset(&slot->chorus); slot->type=types[i]; slot->generation=rack->generation;
        }
        if(types[i]==EFFECT_TUBE && tubes){slot->tube_settings=tubes[i];tube_process(&slot->tube,tubes[i],bypass[i]?0:mixes[i],stereo);}
        if(types[i]==EFFECT_DELAY && delays) {
            slot->delay_settings=delays[i];
            delay_process(atomic_load_explicit(&slot->delay,memory_order_acquire),delays[i],bpm,bypass[i]?0:mixes[i],stereo);
        }
        if(types[i]==EFFECT_EQ) equalizer_process(&slot->eq,eq?eq[i]:equalizer_default(),bypass[i]?0:mixes[i],stereo);
        if(types[i]==EFFECT_CHORUS) chorus_process(&slot->chorus,settings[i],bypass[i]?0:mixes[i],stereo);
    }
}

void effects_process(EffectRack *rack,unsigned bus,const uint8_t types[EFFECT_SLOTS],const ChorusSettings settings[EFFECT_SLOTS],const float mixes[EFFECT_SLOTS],const uint8_t bypass[EFFECT_SLOTS],float stereo[2]) {
    effects_process_eq(rack,bus,types,settings,NULL,mixes,bypass,stereo);
}

void effects_process_eq(EffectRack *rack,unsigned bus,const uint8_t types[EFFECT_SLOTS],const ChorusSettings settings[EFFECT_SLOTS],const EQSettings eq[EFFECT_SLOTS],const float mixes[EFFECT_SLOTS],const uint8_t bypass[EFFECT_SLOTS],float stereo[2]) {
    effects_process_all(rack,bus,types,settings,eq,NULL,NULL,120,mixes,bypass,stereo);
}
