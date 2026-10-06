// SPDX-License-Identifier: GPL-3.0-only
#include "effects.h"
#include <stdlib.h>
struct EffectSlot { union { Chorus chorus; Equalizer eq; }; unsigned generation; uint8_t type; };
struct EffectRack { unsigned buses,generation; struct EffectSlot slots[]; };
EffectRack *effects_create(unsigned buses) {
    if(!buses || buses>101) return NULL;
    EffectRack *rack=calloc(1,sizeof *rack+buses*EFFECT_SLOTS*sizeof *rack->slots);
    if(rack) { rack->buses=buses; rack->generation=1; } return rack;
}
void effects_free(EffectRack *rack) { free(rack); }
void effects_reset(EffectRack *rack) { if(rack) rack->generation++; }
void effects_process_eq(EffectRack *rack,unsigned bus,const uint8_t types[EFFECT_SLOTS],const ChorusSettings settings[EFFECT_SLOTS],const EQSettings eq[EFFECT_SLOTS],const float mixes[EFFECT_SLOTS],const uint8_t bypass[EFFECT_SLOTS],float stereo[2]) {
    if(!rack || bus>=rack->buses) return;
    for(int i=0;i<EFFECT_SLOTS;i++) {
        struct EffectSlot *slot=&rack->slots[bus*EFFECT_SLOTS+i];
        if(!types[i]) {
            if(slot->type==EFFECT_CHORUS && slot->generation==rack->generation && slot->chorus.wet>.00001f)
                chorus_process(&slot->chorus,settings[i],0,stereo);
            else if(slot->type==EFFECT_EQ && slot->generation==rack->generation && slot->eq.wet>.00001f)
                equalizer_process(&slot->eq,slot->eq.target,0,stereo);
            else slot->type=0;
            continue;
        }
        if(slot->type!=types[i] || slot->generation!=rack->generation) {
            if(types[i]==EFFECT_EQ) equalizer_reset(&slot->eq); else chorus_reset(&slot->chorus); slot->type=types[i]; slot->generation=rack->generation;
        }
        if(types[i]==EFFECT_EQ) equalizer_process(&slot->eq,eq?eq[i]:equalizer_default(),bypass[i]?0:mixes[i],stereo);
        if(types[i]==EFFECT_CHORUS) chorus_process(&slot->chorus,settings[i],bypass[i]?0:mixes[i],stereo);
    }
}

void effects_process(EffectRack *rack,unsigned bus,const uint8_t types[EFFECT_SLOTS],const ChorusSettings settings[EFFECT_SLOTS],const float mixes[EFFECT_SLOTS],const uint8_t bypass[EFFECT_SLOTS],float stereo[2]) {
    effects_process_eq(rack,bus,types,settings,NULL,mixes,bypass,stereo);
}
