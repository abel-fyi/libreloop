// SPDX-License-Identifier: Apache-2.0
#ifndef DX7_H
#define DX7_H
#include <stdint.h>
#include "parameter.h"
#define DX7_NATIVE_PARAMETERS 145
#define DX7_PARAMETERS (DX7_NATIVE_PARAMETERS+6)
#define DX7_BLOCK 64
typedef struct { float value[DX7_PARAMETERS]; } DX7Settings;
typedef struct {
    int rates[4],levels[4],stage,down,outlevel,rate_scale,rising,hold;
    int32_t level,target,increment;
} DX7Envelope;
typedef struct {
    DX7Envelope envelope[6],pitch;
    uint32_t phase[6],frequency[6],lfo_phase,lfo_step,delay_state,delay_inc[2],random;
    int32_t gain[6],feedback[2],base_pitch[6],output[DX7_BLOCK],bus[2][DX7_BLOCK];
    uint8_t patch[DX7_NATIVE_PARAMETERS];
    float key_tracking[6];
    int midi,base_midi,velocity,down,cursor,algorithm;
    double note_frequency,pitch_ratio;
} DX7Voice;
void dx7_init(void); /* Call outside playback; tables become immutable. */
DX7Settings dx7_factory(int preset); /* EP, Bass, Marimba, Tubular Bells; -1 brighter EP. */
const char *dx7_factory_name(int preset);
int dx7_valid(DX7Settings settings);
void dx7_legacy_tracking(DX7Settings *settings);
const ParameterDescriptor *dx7_parameter_descriptor(unsigned index);
void dx7_note_on(DX7Voice *v,double frequency,float velocity,DX7Settings settings);
void dx7_note_off(DX7Voice *v);
int dx7_active(const DX7Voice *v);
float dx7_sample(DX7Voice *v,const DX7Settings *settings,double pitch);
#endif
