// SPDX-License-Identifier: GPL-3.0-only
#ifndef THEME_H
#define THEME_H
#include "draw.h"
typedef struct {
    DrawColor background,surface,control,text,secondary,highlight;
    DrawColor hover,active_hover,selected_text,border,title,title_focus;
    DrawColor disabled,step_alt,track,clip[2],note_drag,piano_white,patt;
    DrawColor grid_major,grid_minor,signal,loop;
    DrawColor piano_black,piano_c,piano_row[2];
    DrawColor meter_low,meter_mid,meter_high;
    DrawColor rack,mixer,effects,browser,note,step_on[2],waveform;
    DrawColor knob,knob_track,swing,pan_left,pan_right,stereo,mono;
    DrawColor fader,fader_mark;
} Theme;
extern Theme ui_theme;
typedef struct { DrawColor display,graph,graph_line,graph_fill,field; } DesktopSkin;
extern DesktopSkin ui_skin;
void theme_init(void);
void theme_studio(void);
/* Review preview only: original geometry/font/behavior and legacy default stay. */
void theme_visual_preview(void);
DrawColor theme_foreground(DrawColor background);
DrawColor ui_source_palette_color(int index);
void ui_level_fill(DrawContext *draw,DrawRect bar,float level,float amber,float red,int vertical);
void ui_gradient(DrawContext *draw,DrawRect rect,DrawColor base,float strength,int horizontal);
void ui_display_grid(DrawContext *draw,DrawRect rect,int columns,int rows);
void ui_bevel(DrawContext *draw,DrawRect rect,int inset);
void ui_scrollbar(DrawContext *draw,DrawRect rail,DrawRect thumb,int active);
void ui_fader_handle(DrawContext *draw,DrawRect rect,int selected);
void ui_surface(DrawContext *draw,DrawRect rect,DrawColor color);
void ui_frame(DrawContext *draw,DrawRect rect);
/* Shared steel-desktop skin. Drawing only: callers own geometry and input. */
void ui_panel(DrawContext *draw,DrawRect rect,DrawColor color);
void ui_button_face(DrawContext *draw,DrawRect rect,DrawColor color,int pressed);
DrawColor ui_note_velocity_color(int velocity,int selected);
/* Shared rail fill/hover color and five dots; independent of caption colors. */
void ui_divider_grip(DrawContext *draw,DrawRect rail,int vertical,int active);
void ui_check_face(DrawContext *draw,DrawRect rect,int active,DrawColor mark);
void ui_titlebar(DrawContext *draw,DrawRect rect,int focused);
void ui_knob_face(DrawContext *draw,float x,float y,float radius,float angle,int active);
int theme_draw_init(DrawContext *draw);
void theme_draw_close(void);
/* Dark instrument display; dots are always beneath its content. */
void ui_display_well(DrawContext *draw,DrawRect rect);
void ui_graph_paper(DrawContext *draw,DrawRect rect);
void ui_graph_dots(DrawContext *draw,DrawRect rect);
void ui_graph_area(DrawContext *draw,DrawPoint a,DrawPoint b,float baseline);
void ui_graph_handle(DrawContext *draw,DrawPoint point);
void ui_lamp(DrawContext *draw,DrawPoint center,float radius,DrawColor color);
/* Paint the arrow cell inside an existing choice control; no new hit target. */
void ui_dropdown_arrow(DrawContext *draw,DrawRect control,int pressed);
void ui_text_well(DrawContext *draw,DrawRect rect,int focused);
void ui_folder_icon(DrawContext *draw,float x,float y);
#endif
