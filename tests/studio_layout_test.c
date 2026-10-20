// SPDX-License-Identifier: GPL-3.0-only
#include "studio_layout.h"
#include <stdio.h>
#include <string.h>
#include <math.h>
#define CHECK(x) do{if(!(x)){fprintf(stderr,"Failed line %d: %s\n",__LINE__,#x);return 1;}}while(0)
static int overlap(Rect a,Rect b){return a.x<b.x+b.w && b.x<a.x+a.w && a.y<b.y+b.h && b.y<a.y+a.h;}
int main(void) {
    CHECK(studio_ui_scale(1448,1086,0)==1);
    CHECK(studio_ui_scale(3840,2160,2)==2);
    CHECK(studio_ui_scale(900,562,.75f)==.75f);
    CHECK(fabsf(studio_ui_scale(900,562,2)-900.f/1180)<.00001f);
    CHECK(fabsf(studio_ui_scale(1448,1086,1.5f)-1448.f/1180)<.00001f);
    CHECK(studio_ui_scale(1448,1086,NAN)==1);
    CHECK(fabsf(studio_ui_scale(900,562,0)-900.f/1448)<.00001f);
    CHECK(fabsf(studio_ui_scale(1920,1080,0)-1920.f/1448)<.00001f);
    for(int w=900;w<=3840;w+=73)for(int h=562;h<=2160;h+=67)for(float preference=.75f;preference<=2;preference+=.25f){
        float scale=studio_ui_scale(w,h,preference);CHECK(w/scale>=1179.99f && h/scale>=561.99f && scale<=preference);
    }

    const int sizes[][2]={{900,562},{1024,768},{1200,675},{1280,800},{1448,1086},{1920,1080}};
    for(unsigned n=0;n<sizeof sizes/sizeof *sizes;n++) {
        Windows w;windows_init(&w,sizes[n][0],sizes[n][1]);
        studio_windows_layout(&w,sizes[n][0],sizes[n][1],238);
        for(int i=1;i<=4;i++)if(w.editors[i].visible) {
            Editor e=w.editors[i];
            CHECK(e.rect.x>=244 && e.rect.y>=60);
            CHECK(e.rect.x+e.rect.w<=sizes[n][0]-4 && e.rect.y+e.rect.h<=sizes[n][1]-24);
            CHECK(e.rect.w>=e.minw && e.rect.h>=e.minh);
            for(int j=i+1;j<=4;j++)if(w.editors[j].visible)CHECK(!overlap(e.rect,w.editors[j].rect));
        }
        CHECK(w.editors[1].visible);
        if(n==0)CHECK(!w.editors[2].visible && !w.editors[3].visible && !w.editors[4].visible);
        if(n==4){Rect expected={244,60,858,506};CHECK(!memcmp(&w.editors[1].rect,&expected,sizeof expected));}
        studio_windows_layout(&w,1448,1086,238);
        CHECK(w.editors[2].visible && w.editors[3].visible && w.editors[4].visible);
    }
    const float widths[]={1180,1200,1448,1920};
    StudioToolbar reference=studio_toolbar_layout(1448);
    for(unsigned n=0;n<sizeof widths/sizeof *widths;n++) {
        StudioToolbar t=studio_toolbar_layout(widths[n]);
        CHECK(!memcmp(&t,&reference,sizeof t)); /* No resize breakpoints or hidden meters. */
        Rect all[24];int count=0;
        for(int i=0;i<4;i++){all[count++]=t.menus[i];CHECK(t.menus[i].w>=64 && t.menus[i].h>=20); }
        all[count++]=t.pitch;all[count++]=t.volume;all[count++]=t.stop;all[count++]=t.play;all[count++]=t.record;
        for(int i=0;i<3;i++)CHECK(t.record_types[i].w==0 && t.record_types[i].h==0);
        all[count++]=t.metronome;all[count++]=t.position;all[count++]=t.tempo;all[count++]=t.mode;
        for(int i=0;i<4;i++)CHECK(t.editors[i].w==0 && t.editors[i].h==0);
        all[count++]=t.follow;all[count++]=t.typing;
        if(t.spectrum.w){all[count++]=t.spectrum;all[count++]=t.levels;}
        CHECK(t.position.h>=42 && t.tempo.h>=42); /* Separate 16-unit digits and captions. */
        CHECK(t.stop.w==t.stop.h && t.play.w==t.play.h && t.record.w==t.record.h);
        CHECK(t.follow.w==t.follow.h && t.typing.w==t.typing.h);
        for(int i=0;i<count;i++) {
            CHECK(all[i].x>=0 && all[i].y>=0 && all[i].x+all[i].w<=widths[n] && all[i].y+all[i].h<=58);
            for(int j=i+1;j<count;j++)CHECK(!overlap(all[i],all[j]));
        }
        if(widths[n]==1448){CHECK(t.spectrum.w==120 && t.levels.w==104);CHECK(t.levels.x+t.levels.w==1138);}
        CHECK(t.mode.x+t.mode.w+6==t.play.x);
        CHECK(t.play.x+t.play.w<t.stop.x && t.stop.x+t.stop.w<t.record.x);
        CHECK(t.pitch.x>t.menus[1].x+t.menus[1].w);
        CHECK(t.volume.x>t.pitch.x+t.pitch.w && t.stop.x>t.volume.x+t.volume.w);
    }
    puts("Reference proportions and uniform toolbar scaling keep visible editors within bounds without overlap.");return 0;
}
