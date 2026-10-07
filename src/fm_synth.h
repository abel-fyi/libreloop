// SPDX-License-Identifier: GPL-3.0-only
#ifndef FM_SYNTH_H
#define FM_SYNTH_H
#include "parameter.h"
#include "dx7.h"
#include "analog.h"
/* Carrier plus independently tuned Body and Attack modulators. Legacy fields
   keep their saved IDs; new fields are appended and default to neutral on load. */
typedef struct {
    float ratio,depth,attack,decay,sustain,release,mod_decay,mod_sustain,velocity,lfo_rate,vibrato,tremolo;
    float carrier_ratio,carrier_detune,body_detune,mod_attack,mod_release;
    float attack_ratio,attack_detune,attack_depth,attack_attack,attack_decay,attack_sustain,attack_release;
    float routing,lfo_shape,lfo_fade,body_pitch,engine;
    AnalogSettings analog;
    DX7Settings dx7;
} FMSettings;
typedef struct {
    double carrier,modulator,frequency,current_frequency,lfo_phase,attack_modulator;
    float envelope,ratio,depth,release_step,mod_envelope,velocity,lfo_rate,vibrato,tremolo;
    float carrier_ratio,attack_ratio,attack_depth,carrier_detune,body_detune,attack_detune;
    float body_pitch,attack_envelope,body_release_step,attack_release_step,lfo_age;
    int stage,body_stage,attack_stage,engine;
    union { DX7Voice dx7; AnalogVoice analog; };
} FMVoice;
enum { FM_FACTORY_COUNT=9 };
FMSettings fm_default(void);
FMSettings fm_epiano(void);
FMSettings fm_factory(int preset);
const char *fm_factory_name(int preset);
FMSettings fm_legacy(void);
int fm_valid(FMSettings settings);
const float *fm_parameter_pointer(const FMSettings *settings,unsigned id);
float fm_lfo_value(float phase,int shape);
void fm_note_on(FMVoice *voice,double frequency,FMSettings settings);
void fm_note_on_velocity(FMVoice *voice,double frequency,FMSettings settings,float velocity);
void fm_note_off(FMVoice *voice,FMSettings settings);
int fm_active(const FMVoice *voice);
/* Fixed 48 kHz, allocation-free. pitch is channel/master tuning. */
void fm_sample_stereo(FMVoice *voice,const FMSettings *settings,double pitch,float stereo[2]);
float fm_sample(FMVoice *voice,FMSettings settings,double pitch);
#endif
