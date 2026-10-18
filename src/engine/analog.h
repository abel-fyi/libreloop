// SPDX-License-Identifier: GPL-3.0-only
#ifndef ANALOG_H
#define ANALOG_H
#include "chorus.h"
typedef struct { float wave,detune,mix,sub,noise,pulse,pwm,cutoff,resonance,filter_env,chorus;
    float model,osc_wave[3],osc_tune[3],osc_fine[3],osc_level[3],drive,drift,keytrack; } AnalogSettings;
typedef struct { double phase[3]; float filter[4],g,cutoff,dc_input,dc_output; unsigned tick,random; AnalogSettings current; int ready;
    float ladder_y[4],drift_value[3],drift_target[3],fir[2][19];
    unsigned fir_at[2],substep;
    float env_time[6],env_factor[6];
    Chorus chorus; } AnalogVoice;
AnalogSettings analog_default(void);
AnalogSettings analog_modern_default(void);
int analog_valid(AnalogSettings s);
void analog_note_on(AnalogVoice *v,double frequency);
float analog_sample(AnalogVoice *v,AnalogSettings s,double frequency,float lfo,float envelope);
#endif
