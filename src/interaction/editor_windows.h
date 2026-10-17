// SPDX-License-Identifier: GPL-3.0-only
#ifndef WINDOWS_H
#define WINDOWS_H
#define EDITORS 6
#define WINDOW_CAPACITY 21
#define TITLE 18
#define STUDIO_TITLE 24
typedef struct { float x,y,w,h; } Rect;
typedef struct { Rect rect,restore; float minw,minh; int visible,maximized,pinned; } Editor;
typedef struct {
    int editor_right;
    float editor_width;
    int enabled,show[WINDOW_CAPACITY],floating[WINDOW_CAPACITY],inspector,maximized,drag,last_split;
    int mixer_fx_rows; /* -1 hidden, otherwise occupied rows capped at three */
    float mixer,piano,rack,inspector_width,width,height,sidebar;
    float start_pointer,start_size,last_x,last_y;
    double last_time;
} StudioWorkspace;
typedef struct {
    StudioWorkspace workspace;
    Editor editors[WINDOW_CAPACITY];
    int order[WINDOW_CAPACITY],focused,owner,grab,resize;
    float dx,dy,title_x,title_y;
    double title_time;
    int title_id;
} Windows;
static inline int windows_title_height(const Windows *w) { return w->workspace.enabled?STUDIO_TITLE:TITLE; }
/* Keep carried editor content in its original 18-unit caption coordinate
   system, translated beneath the taller studio caption as a single frame. */
static inline Rect windows_content_frame(const Windows *w,int id) {
    Rect r=w->editors[id].rect;float pad=windows_title_height(w)-TITLE;
    r.y+=pad;r.h-=pad;return r;
}
int windows_count(const Windows *w);
void windows_init(Windows *w,float width,float height);
int windows_hit(const Windows *w,float x,float y);
void windows_focus(Windows *w,int id);
void windows_pin(Windows *w,int id);
void windows_update(Windows *w,Rect desktop,float x,float y,int pressed,int down,double time);
typedef struct { int title,slot,rack_control; } WindowChromeHit;
WindowChromeHit windows_chrome_hit(const Windows *windows,int editor,float x,float y,int enabled);
#endif
