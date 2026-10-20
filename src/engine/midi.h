// SPDX-License-Identifier: GPL-3.0-only
#ifndef MIDI_H
#define MIDI_H
#include "engine.h"
typedef struct { unsigned char status,data1,data2; double time; } MidiEvent;
typedef struct { unsigned char status,data[2],count,sysex; } MidiParser;
typedef void (*MidiEmit)(void *,MidiEvent);
void midi_parse(MidiParser *parser,const unsigned char *data,unsigned count,double time,MidiEmit emit,void *context);
typedef struct {
    int active,pattern,channel,lane,clip,failed,in_place,created_pattern;
    float loop_start,loop_end,note_started[NOTES];
    unsigned previous_color;
    float origin,bpm; double time;
    unsigned char lane_mute[AUTOMATIONS+1];
    Note previous_notes[CHANNELS][NOTES];
    char previous_name[PATTERN_NAME];
    float previous_steps,previous_start,previous_cap,previous_offset;
    int held[16][128],curves[AUTOMATIONS],curve_lane[AUTOMATIONS],curve_clip[AUTOMATIONS],count;
} MidiTake;
int midi_bind(Project *p,unsigned channel,unsigned controller,ParameterTarget target);
void midi_unbind(Project *p,ParameterTarget target);
int midi_take_begin(MidiTake *take,Project *p,int channel,int notes,float origin,double time);
/* Pattern -1 captures automation only; create chooses a fresh note pattern.
   Pattern notes loop inside [loop_start,loop_end), without creating Song clips. */
int midi_take_begin_pattern(MidiTake *take,Project *p,int channel,int pattern,int create,float origin,float loop_start,float loop_end,double time);
int midi_take_note(MidiTake *take,Project *p,int midi_channel,int pitch,int velocity,double time);
/* Caller-owned held slot keeps typing notes independent of all 16 MIDI inputs.
   Initialize to -1; velocity -1 updates duration without releasing. */
int midi_take_key_note(MidiTake *take,Project *p,int *slot,int pitch,int velocity,double time);
int midi_take_control(MidiTake *take,Project *p,ParameterTarget target,float normalized,double time);
void midi_take_update(MidiTake *take,Project *p,double time);
void midi_take_finish(MidiTake *take,Project *p,double time);
#endif
