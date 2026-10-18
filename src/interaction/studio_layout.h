// SPDX-License-Identifier: GPL-3.0-only
#ifndef LIBRELOOP_STUDIO_LAYOUT_H
#define LIBRELOOP_STUDIO_LAYOUT_H
#include "editor_windows.h"
/* Concrete startup/reflow layout. Hidden editors remain available as floats. */
typedef struct {
    Rect menus[4],pitch,volume,stop,play,record,record_types[3],metronome;
    Rect position,tempo,mode,editors[4],follow,typing,spectrum,levels;
    int compact;
} StudioToolbar;
float studio_ui_scale(float width,float height,float preference);
float studio_workspace_default_mixer(float height);
void studio_workspace_init(Windows *w,float width,float height,float sidebar);
void studio_workspace_layout(Windows *w,float width,float height,float sidebar);
void studio_workspace_open(Windows *w,int id);
void studio_workspace_hide(Windows *w,int id);
void studio_workspace_float(Windows *w,int id);
void studio_workspace_maximize(Windows *w,int id);
int studio_workspace_split(const Windows *w,float x,float y);
void studio_workspace_update(Windows *w,Rect desktop,float x,float y,int pressed,int down,double time);
StudioToolbar studio_toolbar_layout(float width);
void studio_windows_layout(Windows *windows,float width,float height,float sidebar);
#endif
