// SPDX-License-Identifier: GPL-3.0-only
#ifndef FM_SYNTH_H
#define FM_SYNTH_H
/* Two operators: a sine modulator changes the carrier's phase. */
typedef struct { float ratio,depth,attack,decay,sustain,release,mod_decay,mod_sustain,velocity,lfo_rate,vibrato,tremolo; } FMSettings;
typedef struct {
    double carrier,modulator,frequency,current_frequency,lfo_phase;
    float envelope,ratio,depth,release_step,mod_envelope,velocity,lfo_rate,vibrato,tremolo;
    int stage;
} FMVoice;
FMSettings fm_default(void);
FMSettings fm_epiano(void);
int fm_valid(FMSettings settings);
void fm_note_on(FMVoice *voice,double frequency,FMSettings settings);
void fm_note_on_velocity(FMVoice *voice,double frequency,FMSettings settings,float velocity);
void fm_note_off(FMVoice *voice,FMSettings settings);
int fm_active(const FMVoice *voice);
/* Fixed 48 kHz, allocation-free. pitch is the channel/master tuning ratio.
   Valid settings required. Returns a mono sample for ordinary mixer pan/routing. */
float fm_sample(FMVoice *voice,FMSettings settings,double pitch);
#endif
