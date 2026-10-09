// SPDX-License-Identifier: GPL-3.0-only
#include "instrument.h"
void instrument_start(InstrumentVoice *voice,unsigned type,InstrumentSettings settings,InstrumentNote note) {
    if(type==INSTRUMENT_FM) fm_note_on_velocity(&voice->fm,note.frequency,*settings.fm,note.velocity);
    else sampler_voice_reset(&voice->sampler,note.position,note.speed);
}
void instrument_release(InstrumentVoice *voice,unsigned type,InstrumentSettings settings) {
    if(type==INSTRUMENT_FM) fm_note_off(&voice->fm,*settings.fm);
}
