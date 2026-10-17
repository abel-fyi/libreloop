// SPDX-License-Identifier: GPL-3.0-only
#ifndef MIDI_INPUT_H
#define MIDI_INPUT_H
#include "midi.h"
typedef struct { char id[128],name[128]; } MidiDevice;
int midi_input_devices(MidiDevice *devices,int capacity);
int midi_input_open(const char *id,char error[256]);
void midi_input_close(void);
int midi_input_poll(MidiEvent *event);
int midi_input_failed(void);
int midi_input_changed(void);
void midi_input_flush(void);
double midi_input_time(void);
void midi_input_wake(void (*wake)(void));
#endif
