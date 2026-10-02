// SPDX-License-Identifier: GPL-3.0-only
#ifndef WINDOWS_H
#define WINDOWS_H
#define EDITORS 5
#define TITLE 24
typedef struct { float x,y,w,h; } Rect;
typedef struct { Rect rect,restore; float minw,minh; int visible,maximized,pinned; } Editor;
typedef struct {
    Editor editors[EDITORS];
    int order[EDITORS],owner,grab,resize;
    float dx,dy;
} Windows;
void windows_init(Windows *w,float width,float height);
int windows_hit(const Windows *w,float x,float y);
void windows_focus(Windows *w,int id);
void windows_pin(Windows *w,int id);
void windows_update(Windows *w,Rect desktop,float x,float y,int pressed,int down);
#endif
