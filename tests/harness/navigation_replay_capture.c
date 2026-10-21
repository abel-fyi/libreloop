// SPDX-License-Identifier: GPL-3.0-only
#include "editor_navigation.h"
#include "scroll_input.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
static Windows windows;
static Arrangement arrangement;
static EditorNavigation navigation={.windows=&windows,.arrangement=&arrangement,.drag=-1,.browser_focus=1};
int main(int argc,char **argv) {
    if(argc!=3)return 1;int mode=atoi(argv[1]);float scale=atof(argv[2]);if(mode<0 || mode>3)return 1;
    windows_init(&windows,1200,675);int id=mode%2?2:1;windows_focus(&windows,id);
    arrangement.zoom=1;arrangement.view_start=3.25f;navigation.track_scroll=100.5f;navigation.track_zoom=1;
    navigation.piano_span[0]=16;navigation.piano_pan[0]=3.5f;navigation.piano_top=72;navigation.piano_zoom=1;
    FILE *out=fopen("state.txt","wb");if(!out)return 2;
    NavigationFrame frame={.layout_scale=scale};
    for(int i=0;i<18;i++) {
        Rect r=windows.editors[id].rect;float gx=id==1?212:60,gy=id==1?96:86;
        frame.pointer=(NavigationPoint){r.x+gx+(r.w-gx-24)*(.2f+.03f*i),r.y+gy+100};
        if(mode==3)frame.pointer.x=r.x+10;
        ScrollInput scroll;scroll_input_begin(&scroll);
        if(i<3){scroll_input_wheel(&scroll,-.5,-1.25);scroll_input_wheel(&scroll,-.25,-.75);}
        if(i>=3 && i<10){scroll_input_native(&scroll,5.125*pow(.7,i-3),-7.75*pow(.5,i-3),1);scroll_input_native(&scroll,-.25,.125,1);}
        if(i>=10){scroll_input_magnify(&scroll,i==13?-1.01:i==14?NAN:i%2?-.015:.03);scroll_input_magnify(&scroll,.01);}
        frame.scroll=scroll_input_navigation(&scroll);
        frame.shift=i%3==1;frame.zoom_modifier=i%4==3;
        frame.middle_pressed=mode==2 && i==7;frame.middle_held=mode==2 && i>=7 && i<11;
        if(mode==2 && i==8)frame.pointer=(NavigationPoint){-30,-50};
        frame.blocked=mode==2 && i==11;
        frame.captured=navigation.drag>=0;
        editor_navigate(&navigation,frame);
        float scalar=scroll_input_scalar(&scroll);
        fprintf(out,"%d %.6f %.9g %.9g %.9g %.9g %.9g %.9g %d %d %d %d %d %.9g\n",i,.125+i/60.,arrangement.zoom,arrangement.view_start,navigation.track_scroll,navigation.piano_span[0],navigation.piano_pan[0],navigation.piano_remainder,navigation.piano_top,navigation.drag,navigation.active,navigation.browser_focus,windows.focused,scalar);
    }
    fclose(out);return 0;
}
