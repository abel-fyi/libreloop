// SPDX-License-Identifier: GPL-3.0-only
#ifndef LIBRELOOP_RECORD_COUNT_IN_H
#define LIBRELOOP_RECORD_COUNT_IN_H
/* One step is a sixteenth note: 15/BPM seconds. No project/audio mutation. */
typedef struct {int active,steps;float bpm;double start,end;} RecordCountIn;
int record_count_in_begin(RecordCountIn *count,int steps,float bpm,double time);
int record_count_in_remaining(const RecordCountIn *count,double time);
#endif
