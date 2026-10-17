// SPDX-License-Identifier: GPL-3.0-only
#ifndef LIBRELOOP_PIANO_INTERACTION_H
#define LIBRELOOP_PIANO_INTERACTION_H
#include "engine.h"
typedef struct { float x,y; } PianoPoint;
enum { PIANO_IDLE,PIANO_BRUSH,PIANO_ERASE,PIANO_BOX };
/* Borrowed arrays/pointer belong to the active source pattern/channel; no draw,
   texture, clock, SDL/window or audio-backend types are part of this state. */
typedef struct {
    int kind,pattern,channel,additive,moving,melodic;
    PianoPoint from,now,grab;
    float length;
    Note *drag;
    Note *before;
    uint8_t *selected;
    uint8_t *selection_before;
} PianoGesture;
typedef struct { float width,height,span,pan,zoom; int top;
    /* Positive reference grid height keeps row height independent of resizing. */
    float reference_area;
    int grid_choice; } PianoViewport;
typedef struct { float x,y,width,height; } PianoBounds;
typedef struct { PianoPoint pointer; int enabled,left,right,shift,shortcut,tool; } PianoPress;
typedef struct {
    int vertical,horizontal;
    float y,pitch_start,x,pan_start,range_per_pixel;
} PianoScrollbars;
typedef struct { int pitch,channel,drag; } PianoKeys;
typedef void (*PianoKeyPreview)(void *context,int slot,int channel,int pitch,int down);
typedef struct { int ramp,pattern,channel; PianoPoint from,now; Note *before; } PianoVelocity;
enum { PIANO_TOOL_PENCIL,PIANO_TOOL_BRUSH,PIANO_TOOL_SELECT };
enum { PIANO_PRESS_CONSUMED=1,PIANO_PRESS_INACTIVE=2,PIANO_PRESS_NOTE_LIMIT=4 };
typedef void (*PianoPreview)(void *context,int channel,Note note);
int piano_inactive(const Project *project,int pattern,float step);
void piano_gesture_update(PianoGesture *state,Project *project,float step,float pitch,float quantum,PianoPreview preview,void *context);
void piano_note_drag_update(PianoGesture *state,Project *project,float step,float row,float quantum,PianoPreview preview,void *context);
float piano_rows_for_area(float area,float zoom,float reference_area);
float piano_rows(const PianoViewport *view);
int piano_is_black_key(int pitch);
PianoBounds piano_key_bounds(const PianoViewport *view,int pitch);
void piano_note_edges(const Project *project,int pattern,int channel,const PianoViewport *view,PianoPoint pointer,int enabled,int shortcut,uint8_t edges[NOTES]);
/* Last drawn overlapping note wins, including the original visible-region
   exclusions. Press uses original body/7px right-edge distinction. */
Note *piano_pointer_note(Project *project,int pattern,int channel,const PianoViewport *view,PianoPoint pointer);
int piano_press(PianoGesture *state,Project *project,const PianoViewport *view,PianoPress press,PianoPreview preview,void *context);
int piano_scrollbar_press(PianoViewport *view,PianoScrollbars *bars,float range,PianoPoint window_origin,PianoPress press);
void piano_scrollbar_update(PianoViewport *view,PianoScrollbars *bars,PianoPoint global_pointer,int left_held,int new_left);
int piano_key_hit(const PianoViewport *view,PianoPoint pointer);
void piano_key_update(PianoKeys *keys,const PianoViewport *view,PianoPoint pointer,int cancel,PianoKeyPreview preview,void *context);
int piano_key_press(PianoKeys *keys,const PianoViewport *view,int channel,PianoPress press,PianoKeyPreview preview,void *context);
int piano_velocity_press(PianoVelocity *velocity,Project *project,int pattern,int channel,const PianoViewport *view,PianoPress press);
void piano_velocity_update(PianoVelocity *velocity,Project *project,int pattern,int channel,const PianoViewport *view,PianoPoint pointer,int cancel,int new_press,int held);
#endif
