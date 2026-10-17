// SPDX-License-Identifier: GPL-3.0-only
#ifndef RECORDING_WRITER_H
#define RECORDING_WRITER_H
#include "engine.h"
typedef struct RecordingWriter RecordingWriter;
typedef unsigned (*RecordingRead)(void *context,int take,float *stereo,unsigned frames);
/* One worker drains one SPSC take ring. No writer work occurs in the audio callback. */
RecordingWriter *recording_writer_open(const char *directory);
const char *recording_writer_path(const RecordingWriter *writer);
int recording_writer_start(RecordingWriter *writer,RecordingRead read,void *context,int take);
unsigned recording_writer_frames(const RecordingWriter *writer);
int recording_writer_failed(const RecordingWriter *writer);
int recording_writer_finish(RecordingWriter *writer);
/* Finish before freeing. remove_file is for canceled setup, never completed takes. */
void recording_writer_free(RecordingWriter *writer,int remove_file);
#endif
