// SPDX-License-Identifier: GPL-3.0-only
/* Frozen ui_shell.c navigation extracted without SDL/window polling/drawing. */
#include "editor_navigation.h"
#include "piano.h"
#include "rack.h"
#include <math.h>
static float track_height(const EditorNavigation *state,int lane) { return fmaxf(28,fminf(640,(state->track_heights[lane]>0?state->track_heights[lane]:52)*state->track_zoom)); }

static float track_position(const EditorNavigation *state,float lane) {
    float y=0; int l=fmaxf(0,fminf(LANES,floorf(lane)));
    for(int i=0;i<l;i++) y+=track_height(state,i);
    return y+(l<LANES?(lane-l)*track_height(state,l):0);
}

void editor_navigate(EditorNavigation *state,NavigationFrame input) {
    float scale=input.layout_scale;
    state->active=state->pan_cursor=0;
    int blocked=input.blocked;
    if(state->drag>=0 && (blocked || !input.middle_held)) state->drag=-1;
    int id=state->drag>=0?state->drag:windows_hit(state->windows,input.pointer.x,input.pointer.y);
    if(blocked || (id!=1 && id!=2 && !(id==0 && state->rack_enabled)) || (state->drag<0 && input.captured) || state->windows->grab>=0) return;
    Rect r=state->windows->editors[id].rect;
    if(id==2 && input.piano_content.w>0)r=input.piano_content;
    if(id==1 && input.playlist_content.w>0)r=input.playlist_content;
    float gx=id==0?200:id==1?(input.playlist_grid_x>0?input.playlist_grid_x:212):60;
    float gy=id==0?32:id==1?(input.playlist_grid_top>0?input.playlist_grid_top:96):86,gridw=r.w-gx-(id==0?26:24);
    float header=id==0?4:id==1?(input.playlist_header_x>0?input.playlist_header_x:120):4;
    float area=id==0?fmaxf(1,floorf((r.h-80)/28))*28:id==1?r.h-gy-18:r.h-180;
    NavigationPoint local={input.pointer.x-r.x,input.pointer.y-r.y};
    if(state->drag<0 && (local.x<header || local.x>=r.w-24 || local.y<gy || local.y>=gy+area)) return;
    int middle=state->drag>=0;
    if(!middle && input.middle_pressed) {
        windows_focus(state->windows,id); state->drag=id; state->last=input.pointer; middle=1;
    }
    NavigationMotion motion={0};
    if(middle) {
        motion.x=state->last.x-input.pointer.x; motion.y=state->last.y-input.pointer.y;
        state->last=input.pointer; state->pan_cursor=1;
    } else motion=navigation_motion(input.scroll,input.shift,input.zoom_modifier,scale);
    if(motion.x || motion.y || motion.zoom || middle) { state->browser_focus=0; state->active=1; }
    if(id==0) {
        float *width=&state->rack_width[state->pattern],*view=&state->rack_view[state->pattern];
        float step=*width>0?*width:16;
        *view=fmaxf(0,*view+motion.x/step);
        if(motion.zoom && local.x>=gx)rack_zoom(width,view,gridw,motion.zoom,fmaxf(0,fminf(1,(local.x-gx)/gridw)));
        float rows=motion.y/28;
        if(!middle && !input.scroll.precise && !input.shift && !input.zoom_modifier)rows=-input.scroll.y;
        int visible=fmaxf(1,(r.h-80)/28);
        state->rack_scroll=fmaxf(0,fminf(fmaxf(0,state->rack_count-visible),state->rack_scroll+rows));
    } else if(id==1) {
        float span=BARS/state->arrangement->zoom;
        state->arrangement->view_start=fmaxf(0,state->arrangement->view_start+motion.x*span/gridw);
        if(motion.zoom) arrangement_zoom(state->arrangement,motion.zoom,fmaxf(0,fminf(1,(local.x-gx)/gridw)));
        state->track_scroll=fmaxf(0,fminf(fmaxf(0,track_position(state,LANES)-area),state->track_scroll+motion.y));
    } else {
        state->piano_pan[state->pattern]=fmaxf(0,state->piano_pan[state->pattern]+motion.x*state->piano_span[state->pattern]/gridw);
        if(motion.zoom && local.x>=gx) timeline_zoom(&state->piano_span[state->pattern],&state->piano_pan[state->pattern],motion.zoom,fmaxf(0,fminf(1,(local.x-gx)/gridw)),STEPS);
        float rows_visible=piano_rows_for_area(area,state->piano_zoom,input.piano_reference_area);
        int minimum=(int)fminf(128,ceilf(rows_visible))-1;
        state->piano_remainder+=motion.y/(area/rows_visible);
        int rows=(int)state->piano_remainder;
        state->piano_top=fmaxf(minimum,fminf(127,state->piano_top-rows)); state->piano_remainder-=rows;
        if(state->piano_top==minimum || state->piano_top==127) state->piano_remainder=0;
    }
}
