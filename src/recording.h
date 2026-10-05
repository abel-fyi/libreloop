// SPDX-License-Identifier: GPL-3.0-only
#ifndef RECORDING_H
#define RECORDING_H
#include "engine.h"
#include "waveform.h"
#include "recording_writer.h"
typedef struct {
    int channel,lane,clip,bus;
    Sample sample;
    Waveform wave;
    RecordingWriter *writer;
    char path[PATH_MAX];
} RecordingTake;
typedef struct {
    int active,count;
    float bpm,start;
    RecordingTake takes[CHANNELS];
} RecordingSession;
int recording_prepare(RecordingSession *session,Project *next,const int buses[],int count,float start,const char *directory);
int recording_start_workers(RecordingSession *session);
int recording_refresh(RecordingSession *session,Project *project);
int recording_finish_writers(RecordingSession *session,Project *project);
void recording_cancel(RecordingSession *session);
#endif
