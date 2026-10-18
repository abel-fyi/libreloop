// SPDX-License-Identifier: GPL-3.0-only
#ifndef EFFECTS_H
#define EFFECTS_H
#include "device.h"
#include "chorus.h"
#include "equalizer.h"
#include "delay.h"
#include "tube.h"
#include "reverb.h"
#include "dynamics.h"
#include <stdint.h>
#define EFFECT_SLOTS 10

typedef struct EffectRack EffectRack;
/* Prepare bounded delay storage outside the audio callback; rack owns it. */
EffectRack *effects_create(unsigned buses);
/* Control thread only: publish once; cached storage lives until rack destruction. */
int effects_prepare(EffectRack *rack,const uint8_t *types,unsigned count);
void effects_process_all(EffectRack *rack,unsigned bus,const uint8_t types[EFFECT_SLOTS],const ChorusSettings chorus[EFFECT_SLOTS],const EQSettings eq[EFFECT_SLOTS],const DelaySettings delay[EFFECT_SLOTS],const TubeSettings tube[EFFECT_SLOTS],const ReverbSettings reverb[EFFECT_SLOTS],const DynamicsSettings *const dynamics[EFFECT_SLOTS],float bpm,const float mixes[EFFECT_SLOTS],const uint8_t bypass[EFFECT_SLOTS],float stereo[2]);
void effects_free(EffectRack *rack);
void effects_dynamics_monitor(const EffectRack *rack,unsigned bus,unsigned slot,DynamicsMonitor *monitor);
/* Increment generation; individual used slots reset lazily during processing. */
void effects_reset(EffectRack *rack);
void effects_reset_bus(EffectRack *rack,unsigned bus);
void effects_process(EffectRack *rack,unsigned bus,const uint8_t types[EFFECT_SLOTS],const ChorusSettings settings[EFFECT_SLOTS],const float mixes[EFFECT_SLOTS],const uint8_t bypass[EFFECT_SLOTS],float stereo[2]);
void effects_process_eq(EffectRack *rack,unsigned bus,const uint8_t types[EFFECT_SLOTS],const ChorusSettings chorus[EFFECT_SLOTS],const EQSettings eq[EFFECT_SLOTS],const float mixes[EFFECT_SLOTS],const uint8_t bypass[EFFECT_SLOTS],float stereo[2]);
#endif
