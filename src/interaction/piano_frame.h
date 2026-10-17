// SPDX-License-Identifier: GPL-3.0-only
#ifndef LIBRELOOP_PIANO_FRAME_H
#define LIBRELOOP_PIANO_FRAME_H
#include "piano.h"
/* Update existing captures before processing this frame's new presses, as in
   original main.c. Origin and pointer use logical UI units. */
typedef struct {
    PianoPoint pointer,origin;
    int modal,focused,visible,left_pressed,right_pressed,left_held,right_held;
} PianoFrame;
void piano_frame_advance(PianoGesture *gesture,PianoKeys *keys,PianoVelocity *velocity,
    PianoScrollbars *bars,Project *project,PianoViewport *view,PianoFrame frame,
    PianoPreview preview,PianoKeyPreview key_preview,void *context);
/* Same press priority as ui_piano.c: keys, notes, velocity, scrollbars.
   Return whether the frame's new press was consumed. */
int piano_frame_press(PianoGesture *gesture,PianoKeys *keys,PianoVelocity *velocity,
    PianoScrollbars *bars,Project *project,PianoViewport *view,float range,
    PianoPoint origin,PianoPress press,PianoPreview preview,
    PianoKeyPreview key_preview,void *context);
#endif
