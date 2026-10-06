// SPDX-License-Identifier: GPL-3.0-only
#ifndef FM_SYNTH_H
#define FM_SYNTH_H
/* Two operators: a sine modulator changes the carrier's phase. */
typedef struct { float ratio,depth,attack,decay,sustain,release; } FMSettings;
typedef struct {
    double carrier,modulator,frequency,current_frequency;
    float envelope,ratio,depth,release_step;
    int stage;
} FMVoice;
FMSettings fm_default(void);
int fm_valid(FMSettings settings);
void fm_note_on(FMVoice *voice,double frequency,FMSettings settings);
void fm_note_off(FMVoice *voice,FMSettings settings);
int fm_active(const FMVoice *voice);
/* Fixed 48 kHz, allocation-free. pitch is the channel/master tuning ratio.
   Valid settings required. Returns a mono sample for ordinary mixer pan/routing. */
float fm_sample(FMVoice *voice,FMSettings settings,double pitch);
#endif
