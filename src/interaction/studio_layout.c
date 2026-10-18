// SPDX-License-Identifier: GPL-3.0-only
#include "studio_layout.h"
#include <math.h>
float studio_ui_scale(float width,float height,float preference) {
    float automatic=fminf(2,fmaxf(1,fminf(width/1448.f,height/1086.f)));
    float requested=isfinite(preference) && preference>=.75f && preference<=2?preference:automatic;
    return fminf(requested,fminf(width/900.f,height/562.f));
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
    StudioToolbar t={0};t.compact=width<1180;
    float y=t.compact?12:8,size=t.compact?32:42,cy=y+size/2;
    for(int i=0;i<4;i++)t.menus[i]=(Rect){8+(i%2)*72,(t.compact?8:16)+(i/2)*22,68,20};
    t.pitch=(Rect){(t.compact?116:206)-11,cy-11,22,22};
    t.volume=(Rect){(t.compact?146:238)-11,cy-11,22,22};
    t.stop=(Rect){t.compact?168:266,y,size,size};
    t.play=(Rect){t.stop.x+size+6,y,size,size};
    t.record=(Rect){t.play.x+size+6,y,size,size};
    for(int i=0;i<3;i++)t.record_types[i]=(Rect){(t.compact?286:416)+i*(t.compact?24:26),cy-(t.compact?11:12),t.compact?22:24,t.compact?22:24};
    t.metronome=(Rect){t.compact?366:510,y,size,size};
    t.position=(Rect){t.compact?410:564,y,t.compact?100:136,size};
    t.tempo=(Rect){t.compact?518:712,y,t.compact?74:88,size};
    t.mode=(Rect){t.compact?600:812,y,t.compact?50:52,size};
    t.follow=(Rect){t.compact?662:876,cy-(t.compact?12:14),t.compact?24:28,t.compact?24:28};
    t.typing=t.follow;t.typing.x+=t.compact?26:32;
    t.volume.x+=4;
    /* Labeled two-column menus need 56 more units in the compact toolbar. */
    if(t.compact) {
        Rect *controls[]={&t.pitch,&t.volume,&t.stop,&t.play,&t.record,
            &t.record_types[0],&t.record_types[1],&t.record_types[2],&t.metronome,
            &t.position,&t.tempo,&t.mode,&t.follow,&t.typing};
        for(unsigned i=0;i<sizeof controls/sizeof *controls;i++)controls[i]->x+=56;
        t.volume.x+=4;
        t.position.y=t.tempo.y=8;t.position.h=t.tempo.h=44;
    }
    float available=width-952-64;
    if(!t.compact && available>=96) {
        float spectrum_width=fminf(120,width-952-112);
        t.spectrum=(Rect){952,y,spectrum_width,size};
        t.levels=(Rect){t.spectrum.x+t.spectrum.w+4,y,104,size};
    }
    return t;
}
