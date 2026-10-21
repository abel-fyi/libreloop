// SPDX-License-Identifier: GPL-3.0-only
#include "../../src/ui/frontend/ui_internal.h"
UiState ui;
#include "../../src/ui/frontend/ui_playlist.c"
static FILE *out;
static void press(int id,int pattern,int button,float at,double time,DrawPoint point){ui.pattern=pattern;desktop_ruler_press(id,button,at,time,point);}
static void drag(int pattern,float at,int cancel,int pressed,int held){ui.pattern=pattern;desktop_ruler_drag(at,cancel,pressed,held);}
static void snapshot(const char *name){fprintf(out,"%s %d %d %d %d %d %.17g %.9g %.9g %d %d %d %d %.9g %.9g %.9g %.9g\n",name,ui.ruler_last_id,ui.ruler_last_pattern,ui.ruler_last_button,ui.marker_drag,ui.ruler_loop_drag,ui.ruler_last_click,ui.ruler_last_position.x,ui.ruler_last_position.y,ui.stop_armed,ui.song,ui.reset,ui.playing,ui.ruler_anchor,ui.playlist_start,ui.song_loop[0],ui.song_loop[1]);/* Frozen replay covers the original eight slots. */for(int i=0;i<LEGACY_PATTERNS;i++)fprintf(out,"%.9g %.9g %.9g\n",ui.piano_start[i],ui.pattern_loop[i][0],ui.pattern_loop[i][1]);}
#define PRESS press
#define DRAG drag
#define SNAP snapshot
#define POINT DrawPoint
static void run(void){
    PRESS(2,0,0,3.5f,1,(POINT){100,72});SNAP("start");
    PRESS(2,0,1,8,1.1,(POINT){101,72});SNAP("new-loop");DRAG(0,12,0,0,1);SNAP("loop-drag");DRAG(0,14,0,0,0);SNAP("release");
    PRESS(2,0,1,6,1.5,(POINT){102,72});SNAP("nearest-left");
    PRESS(2,0,1,9,1.79,(POINT){107.9f,77.9f});SNAP("double-clear");
    PRESS(2,0,1,4,2,(POINT){100,72});SNAP("fresh-loop");
    PRESS(2,0,1,10,2.2,(POINT){106,72});SNAP("six-pixel-boundary");
    PRESS(2,1,0,7,2.3,(POINT){100,72});SNAP("pattern-switch");
    PRESS(1,1,0,20,2.5,(POINT){100,72});SNAP("song-mode");
    PRESS(1,2,0,24,2.6,(POINT){102,73});SNAP("song-double-other-pattern");
    PRESS(0,2,0,5,3,(POINT){100,72});SNAP("rack-start");DRAG(2,8,1,0,1);SNAP("modal-cancel");
    PRESS(2,2,1,5,4,(POINT){100,72});DRAG(2,8,0,1,1);SNAP("new-press-cancel");
    PRESS(2,2,1,9,5,(POINT){100,72});PRESS(2,2,1,10,4.9,(POINT){101,73});SNAP("original-negative-time-difference");
}

int main(void){ui.ruler_last_id=ui.marker_drag=-1;ui.ruler_last_click=-1;out=fopen("ruler.txt","wb");if(!out)return 1;run();fclose(out);return 0;}
