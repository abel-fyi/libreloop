// SPDX-License-Identifier: GPL-3.0-only
#include "studio_layout.h"
#include <math.h>
float studio_ui_scale(float width,float height,float preference) {
    /* A fixed logical width keeps proportions stable while resizing. */
    float automatic=width/1448.f;
    float requested=isfinite(preference) && preference>=.75f && preference<=2?preference:automatic;
    return fminf(requested,fminf(width/1180.f,height/562.f));
}
void studio_windows_layout(Windows *w,float width,float height,float sidebar) {
    float left=sidebar+6,top=60,available=height-top-24;
    int inspector=width-left-346>=440;
    int mixer=available>=720,piano=available>=506;
    float center=inspector?width-left-346:width-left-4;
    float piano_height=mixer?fminf(276,available-460):fminf(276,available-246);
    float bottom=height-24-(mixer?208:0),piano_top=bottom-(mixer?6:0)-(piano?piano_height:0);
    float arrangement_height=piano?piano_top-top-6:bottom-top-(mixer?6:0);
    w->editors[0].visible=0;
    w->editors[1].rect=(Rect){left,top,center,arrangement_height};
    w->editors[1].maximized=0;w->editors[1].visible=1;
    w->editors[2].rect=(Rect){left,piano?piano_top:top,center,piano?piano_height:fminf(276,available)};
    w->editors[2].minh=260;w->editors[2].visible=piano;
    w->editors[3].rect=(Rect){left,mixer?bottom:top,width-left-4,208};
    w->editors[3].minh=208;w->editors[3].visible=mixer;
    w->editors[4].rect=(Rect){width-338,top,334,mixer?bottom-top-6:available};
    w->editors[4].minw=330;w->editors[4].visible=inspector;
    for(int id=1;id<=4;id++)w->editors[id].restore=w->editors[id].rect;
}

StudioToolbar studio_toolbar_layout(float width) {
    /* Positions are in reference units; the uniform UI transform scales the
       whole toolbar, including text and hit targets. Never reflow controls. */
    (void)width;
    StudioToolbar t={0};
    const float y=8,size=42,cy=y+size/2;
    for(int i=0;i<4;i++)t.menus[i]=(Rect){8+(i%2)*72,16+(i/2)*22,68,20};
    t.pitch=(Rect){164,cy-11,22,22};
    t.volume=(Rect){204,cy-11,22,22};
    t.mode=(Rect){244,y,80,size};
    t.play=(Rect){330,y,size,size};
    t.stop=(Rect){378,y,size,size};
    t.record=(Rect){426,y,size,size};
    for(int i=0;i<3;i++)t.record_types[i]=(Rect){480+i*26,cy-12,24,24};
    t.metronome=(Rect){568,y,size,size};
    t.position=(Rect){622,y,136,size};
    t.tempo=(Rect){770,y,88,size};
    t.follow=(Rect){864,cy-14,28,28};
    t.typing=(Rect){896,cy-14,28,28};
    t.spectrum=(Rect){944,y,120,size};
    t.levels=(Rect){1068,y,104,size};
    return t;
}
