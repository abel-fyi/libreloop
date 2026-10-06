// SPDX-License-Identifier: GPL-3.0-only
#ifndef EFFECTS_H
#define EFFECTS_H
#include "chorus.h"
#include "equalizer.h"
#include <stdint.h>
#define EFFECT_SLOTS 10
enum { EFFECT_EMPTY, EFFECT_CHORUS, EFFECT_EQ };
typedef struct EffectRack EffectRack;
/* Prepare bounded delay storage outside the audio callback; rack owns it. */
EffectRack *effects_create(unsigned buses);
void effects_free(EffectRack *rack);
/* Increment generation; individual used slots reset lazily during processing. */
void effects_reset(EffectRack *rack);
void effects_process(EffectRack *rack,unsigned bus,const uint8_t types[EFFECT_SLOTS],const ChorusSettings settings[EFFECT_SLOTS],const float mixes[EFFECT_SLOTS],const uint8_t bypass[EFFECT_SLOTS],float stereo[2]);
void effects_process_eq(EffectRack *rack,unsigned bus,const uint8_t types[EFFECT_SLOTS],const ChorusSettings chorus[EFFECT_SLOTS],const EQSettings eq[EFFECT_SLOTS],const float mixes[EFFECT_SLOTS],const uint8_t bypass[EFFECT_SLOTS],float stereo[2]);
#endif
