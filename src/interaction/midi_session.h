// SPDX-License-Identifier: GPL-3.0-only
#ifndef LIBRELOOP_MIDI_SESSION_H
#define LIBRELOOP_MIDI_SESSION_H
#include "midi.h"
#include "input.h"
enum { MIDI_RECORD_AUDIO=1,MIDI_RECORD_NOTES=2,MIDI_RECORD_AUTOMATION=4 };
typedef struct {int used,pressed,mchannel,pitch,target,velocity;} MidiHeld;
/* Initialize to zero; project/take and both command callbacks are required.
   One UI thread owns this state and calls the engine/audio command producer. */
typedef struct {
    Project *project;MidiTake *take;
    MidiHeld midi_keys[32];unsigned char midi_sustain[16];
    uint8_t keyboard_notes[CHANNELS][128],typing_held[37];
    int typing_slots[37],typing_recording;
    unsigned record_mask;
    int recording_audio,midi_record_target,focused_editor,piano_channel,instrument_channel,channel;
    int midi_learning,typing_keys,browser_focus;
    ParameterTarget midi_learn_target;
    char status[256];
    void *context;
    void (*key_velocity)(void *,int slot,int channel,int pitch,int velocity);
    void (*key)(void *,int slot,int channel,int pitch,int down);
} MidiSession;
int midi_session_target(const MidiSession *session);
void midi_session_panic(MidiSession *session,double time);
void midi_session_release(MidiSession *session,int slot,double time);
void midi_session_control(MidiSession *session,ParameterTarget target,float normalized,double time);
void midi_session_event(MidiSession *session,MidiEvent event);
/* Returns original recording_finish condition; owner performs service teardown. */
int midi_session_update(MidiSession *session,double time,int playing,float *control_drag);
void midi_session_typing(MidiSession *session,const AppInput *input,int blocked,int command_down);
void midi_session_typing_at(MidiSession *session,const AppInput *input,int blocked,int command_down,double time);
void midi_session_typing_begin(MidiSession *session,double time);
void midi_session_typing_end(MidiSession *session,double time);
#endif
