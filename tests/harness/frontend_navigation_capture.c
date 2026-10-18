// SPDX-License-Identifier: GPL-3.0-only
#include "../../src/ui/frontend/ui_internal.h"
#include "editor_navigation.h"
#include "scroll_input.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "../../src/ui/frontend/ui_state.c"
#define LIBRELOOP_LOGO ""
#define LIBRELOOP_SAMPLES ""
#define LIBRELOOP_FONT ""
static NavigationFrame test_frame;
int host_is_window_focused(void){return !test_frame.blocked;}
int host_is_mouse_button_down(int b){return b==HOST_MOUSE_BUTTON_MIDDLE && test_frame.middle_held;}
int host_is_key_down(int key){if(key==HOST_KEY_LEFT_SHIFT || key==HOST_KEY_RIGHT_SHIFT)return test_frame.shift;return (key==HOST_KEY_LEFT_SUPER || key==HOST_KEY_RIGHT_SUPER || key==HOST_KEY_LEFT_CONTROL || key==HOST_KEY_RIGHT_CONTROL) && test_frame.zoom_modifier;}
#include "../../src/ui/frontend/ui_shell.c"
int main(int argc,char **argv) {
    ui_state_init();
    if(argc!=3)return 1;int mode=atoi(argv[1]);float scale=atof(argv[2]);if(mode<0 || mode>3)return 1;
    ui.navigation_drag=-1;ui.browser_focus=1;windows_init(&ui.windows,1200,675);int id=mode%2?2:1;windows_focus(&ui.windows,id);
    ui.arrangement.zoom=1;ui.arrangement.view_start=3.25f;ui.track_scroll=100.5f;ui.track_zoom=1;
    ui.piano_span[0]=16;ui.piano_pan[0]=3.5f;ui.piano_top=72;ui.piano_zoom=1;
    FILE *out=fopen("state.txt","w");if(!out)return 2;
    NavigationFrame frame={.layout_scale=scale};
    for(int i=0;i<18;i++) {
        Rect r=ui.windows.editors[id].rect;float gx=id==1?212:60,gy=id==1?96:86;
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
        frame.captured=ui.navigation_drag>=0;
        test_frame=frame;ui.frame_clicks[HOST_MOUSE_BUTTON_MIDDLE]=frame.middle_pressed;ui.mouse=(DrawPoint){frame.pointer.x,frame.pointer.y};ui.navigation_input=frame.scroll;navigate_editors(scale);
        float scalar=scroll_input_scalar(&scroll);
        fprintf(out,"%d %.6f %.9g %.9g %.9g %.9g %.9g %.9g %d %d %d %d %d %.9g\n",i,.125+i/60.,ui.arrangement.zoom,ui.arrangement.view_start,ui.track_scroll,ui.piano_span[0],ui.piano_pan[0],ui.piano_scroll_remainder,ui.piano_top,ui.navigation_drag,ui.navigation_active,ui.browser_focus,ui.windows.focused,scalar);
    }
    fclose(out);return 0;
}
