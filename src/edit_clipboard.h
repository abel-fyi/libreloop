// SPDX-License-Identifier: GPL-3.0-only
#ifndef EDIT_CLIPBOARD_H
#define EDIT_CLIPBOARD_H
#include "engine.h"
enum { COPY_EMPTY, COPY_NOTES, COPY_CLIPS };
typedef struct { int lane,source; float start,length,offset; } CopiedClip;
typedef struct {
    int kind,count,lane,channels,patterns,automations;
    Note notes[NOTES];
    CopiedClip clips[LANES*CLIPS];
} EditClipboard;
int clipboard_copy_notes(EditClipboard *c,const Project *p,int pattern,int channel,const uint8_t selected[NOTES]);
int clipboard_copy_clips(EditClipboard *c,const Project *p,const uint8_t selected[LANES][CLIPS]);
/* Success is the pasted count. Errors: -1 source changed, -2 capacity,
   -3 occupied position, -4 invalid destination. Failures leave project/selection unchanged. */
int clipboard_paste_notes(const EditClipboard *c,Project *p,int pattern,int channel,float start,uint8_t selected[NOTES]);
int clipboard_paste_clips(const EditClipboard *c,Project *p,int lane,float bar,uint8_t selected[LANES][CLIPS]);
#endif
