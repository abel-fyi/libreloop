// SPDX-License-Identifier: GPL-3.0-only
#ifndef THEME_H
#define THEME_H
#include "raylib.h"
typedef struct {
    Color background,surface,control,text,secondary,highlight;
    Color hover,active_hover,selected_text,border,title,title_focus;
    Color disabled,step_alt,track,clip[2],note_drag,piano_white,patt;
    Color grid_major,grid_minor,signal,loop;
    Color piano_black,piano_c,piano_row[2];
    Color meter_low,meter_mid,meter_high;
    Color rack,mixer,effects,browser,note,step_on[2],waveform;
    Color knob,knob_track,swing,pan_left,pan_right,stereo,mono;
    Color fader,fader_mark;
} Theme;
extern Theme ui_theme;
void theme_init(void);
Color theme_foreground(Color background);
void ui_fader_handle(Rectangle rect,int selected);
void ui_surface(Rectangle rect,Color color);
void ui_frame(Rectangle rect);
#endif
