// SPDX-License-Identifier: GPL-3.0-only
#ifndef ANALOG_H
#define ANALOG_H
#include "chorus.h"
typedef struct { float wave,detune,mix,sub,noise,pulse,pwm,cutoff,resonance,filter_env,chorus; } AnalogSettings;
typedef struct { double phase[3]; float filter[4],g,cutoff,dc_input,dc_output; unsigned tick,random; AnalogSettings current; int ready; Chorus chorus; } AnalogVoice;
AnalogSettings analog_default(void);
int analog_valid(AnalogSettings s);
void analog_note_on(AnalogVoice *v,double frequency);
float analog_sample(AnalogVoice *v,AnalogSettings s,double frequency,float lfo,float envelope);
#endif
