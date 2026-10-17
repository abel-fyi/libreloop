// SPDX-License-Identifier: GPL-3.0-only
#include "editor_navigation.h"
#include <math.h>
#include <stdio.h>
#include <string.h>
#define CHECK(x) do { if(!(x)){fprintf(stderr,"Failed line %d: %s\n",__LINE__,#x);return 1;} } while(0)

static void initialize(EditorNavigation *state,Windows *windows,Arrangement *arrangement,Rect rect) {
    windows_init(windows,1448,1086);
    for(int i=0;i<EDITORS;i++)windows->editors[i].visible=i==2;
    windows->editors[2].rect=rect;
    windows_focus(windows,2);
    memset(state,0,sizeof *state);
    state->windows=windows;state->arrangement=arrangement;state->drag=-1;
    state->piano_span[0]=64;state->piano_pan[0]=13.5f;
    state->piano_top=72;state->piano_zoom=1;
}
int main(void) {
    Rect outer={236,596,858,274},content={402,562,692,308};
    for(int density=1;density<=2;density++) {
        Windows actual_windows,reference_windows;
        Arrangement actual_arrangement={0},reference_arrangement={0};
        EditorNavigation actual,reference;
        initialize(&actual,&actual_windows,&actual_arrangement,outer);
        initialize(&reference,&reference_windows,&reference_arrangement,content);
        for(int i=0;i<24;i++) {
            NavigationFrame frame={.layout_scale=density,.piano_content=content};
            frame.pointer=(NavigationPoint){content.x+60+(content.w-84)*(.25f+i*.01f),content.y+86+50};
            frame.scroll=i<6?(NavigationInput){.x=-.5f,.y=-1}:
                i<12?(NavigationInput){.x=-3.5f,.y=5.25f*powf(.6f,i-6),.precise=1}:
                (NavigationInput){.zoom=i%2?.2f:-.1f};
            frame.shift=i%3==1;frame.zoom_modifier=i%4==3;
            frame.middle_pressed=i==18;frame.middle_held=i>=18 && i<21;
            frame.captured=actual.drag>=0;frame.blocked=i==21;
            NavigationFrame reference_frame=frame;reference_frame.piano_content=(Rect){0};
            float anchor=(frame.pointer.x-content.x-60)/(content.w-84);
            float anchored_step=actual.piano_pan[0]+anchor*actual.piano_span[0];
            editor_navigate(&actual,frame);editor_navigate(&reference,reference_frame);
            if(i>=12 && i<18)CHECK(fabsf(actual.piano_pan[0]+anchor*actual.piano_span[0]-anchored_step)<.0001f);
            CHECK(actual.piano_span[0]==reference.piano_span[0]);
            CHECK(actual.piano_pan[0]==reference.piano_pan[0]);
            CHECK(actual.piano_top==reference.piano_top);
            CHECK(actual.piano_remainder==reference.piano_remainder);
            CHECK(actual.drag==reference.drag && actual.active==reference.active);
            CHECK(actual.last.x==reference.last.x && actual.last.y==reference.last.y);
        }
        float span=actual.piano_span[0],pan=actual.piano_pan[0];
        NavigationFrame tools={.pointer={outer.x+40,outer.y+100},.layout_scale=density,
            .scroll={.y=1,.zoom=.2f},.zoom_modifier=1,.piano_content=content};
        editor_navigate(&actual,tools);
        CHECK(!actual.active && actual.piano_span[0]==span && actual.piano_pan[0]==pan);
        tools.pointer=(NavigationPoint){content.x+160,outer.y+outer.h-50};
        editor_navigate(&actual,tools);
        CHECK(!actual.active && actual.piano_span[0]==span && actual.piano_pan[0]==pan);
    }
    for(int density=1;density<=2;density++) {
        Windows actual_windows,reference_windows;
        Arrangement actual_arrangement={.zoom=1,.view_start=13.5f},reference_arrangement=actual_arrangement;
        EditorNavigation actual,reference;
        Rect outer={244,98,858,468},legacy={180,76,922,490};
        initialize(&actual,&actual_windows,&actual_arrangement,outer);
        initialize(&reference,&reference_windows,&reference_arrangement,legacy);
        for(int i=0;i<EDITORS;i++)actual_windows.editors[i].visible=reference_windows.editors[i].visible=i==1;
        actual_windows.editors[1].rect=(Rect){outer.x,outer.y-14,outer.w,outer.h+14};reference_windows.editors[1].rect=legacy;
        windows_focus(&actual_windows,1);windows_focus(&reference_windows,1);
        actual.track_zoom=reference.track_zoom=1;
        for(int i=0;i<24;i++) {
            NavigationFrame frame={.layout_scale=density,.playlist_content=outer,.playlist_grid_x=148,.playlist_grid_top=74,.playlist_header_x=4};
            frame.pointer=(NavigationPoint){outer.x+148+(outer.w-172)*(.25f+i*.01f),outer.y+74+50};
            frame.scroll=i<6?(NavigationInput){.x=-.5f,.y=-1}:
                i<12?(NavigationInput){.x=-3.5f,.y=5.25f*powf(.6f,i-6),.precise=1}:
                (NavigationInput){.zoom=i%2?.2f:-.1f};
            frame.shift=i%3==1;frame.zoom_modifier=i%4==3;
            frame.middle_pressed=i==18;frame.middle_held=i>=18 && i<21;
            frame.captured=actual.drag>=0;frame.blocked=i==21;
            NavigationFrame old=frame;old.playlist_content=(Rect){0};old.playlist_grid_x=old.playlist_grid_top=old.playlist_header_x=0;
            float anchor=(frame.pointer.x-outer.x-148)/(outer.w-172);
            float anchored_bar=actual_arrangement.view_start+anchor*BARS/actual_arrangement.zoom;
            editor_navigate(&actual,frame);editor_navigate(&reference,old);
            CHECK(actual_arrangement.zoom==reference_arrangement.zoom);
            CHECK(actual_arrangement.view_start==reference_arrangement.view_start);
            CHECK(actual.track_scroll==reference.track_scroll && actual.active==reference.active);
            CHECK(actual.drag==reference.drag);
            if(i>=12 && i<18)CHECK(fabsf(actual_arrangement.view_start+anchor*BARS/actual_arrangement.zoom-anchored_bar)<.0001f);
        }
        float scroll=actual.track_scroll;
        editor_navigate(&actual,(NavigationFrame){.pointer={outer.x+30,outer.y+130},.scroll={.y=-1},.layout_scale=density,
            .playlist_content=outer,.playlist_grid_x=148,.playlist_grid_top=74,.playlist_header_x=4});
        CHECK(actual.active && actual.track_scroll==scroll+48);
    }
    puts("Classic content translation preserves wheel, momentum, pinch and drag navigation; tools and velocity remain outside the navigation grid.");
    return 0;
}
