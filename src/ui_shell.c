// SPDX-License-Identifier: GPL-3.0-only
#include "ui_internal.h"

int shortcut_down(void) {
#ifdef __APPLE__
    return IsKeyDown(KEY_LEFT_SUPER) || IsKeyDown(KEY_RIGHT_SUPER);
#else
    return IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL);
#endif
}

int command_down(void) {
    return shortcut_down() || IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL)
        || IsKeyDown(KEY_LEFT_SUPER) || IsKeyDown(KEY_RIGHT_SUPER);
}

int delete_pressed(void) {
#ifdef __APPLE__
    return IsKeyPressed(KEY_DELETE) || IsKeyPressed(KEY_BACKSPACE);
#else
    return IsKeyPressed(KEY_DELETE);
#endif
}

void resource_paths(void) {
#ifdef __APPLE__
    snprintf(ui.logo_path,sizeof ui.logo_path,"%s../Resources/branding/logo.png",GetApplicationDirectory());
    snprintf(ui.samples_path,sizeof ui.samples_path,"%s../Resources/samples",GetApplicationDirectory());
    snprintf(ui.font_path,sizeof ui.font_path,"%s../Resources/fonts/LiberationSans-Regular.ttf",GetApplicationDirectory());
#else
    snprintf(ui.logo_path,sizeof ui.logo_path,"%s../share/libreloop/branding/logo.png",GetApplicationDirectory());
    if(!FileExists(ui.logo_path)) snprintf(ui.logo_path,sizeof ui.logo_path,"%s",LIBRELOOP_LOGO);
    snprintf(ui.samples_path,sizeof ui.samples_path,"%s../share/libreloop/samples",GetApplicationDirectory());
    snprintf(ui.font_path,sizeof ui.font_path,"%s../share/libreloop/fonts/LiberationSans-Regular.ttf",GetApplicationDirectory());
    if(!DirectoryExists(ui.samples_path) || !FileExists(ui.font_path)) {
        snprintf(ui.samples_path,sizeof ui.samples_path,"%s",LIBRELOOP_SAMPLES);
        snprintf(ui.font_path,sizeof ui.font_path,"%s",LIBRELOOP_FONT);
    }
#endif
}

void mouse_callback(GLFWwindow *window,int button,int action,int mods) {
    if(button>=0 && button<8 && action==GLFW_PRESS) ui.pending_clicks[button]++;
    if(ui.raylib_mouse_callback) ui.raylib_mouse_callback(window,button,action,mods);
}

int mouse_pressed(int button) { return button>=0 && button<8 && ui.frame_clicks[button]; }

void request_cursor(int cursor) { ui.requested_cursor=cursor; }

void flush_cursor(void) {
    if(ui.requested_cursor==ui.current_cursor) return;
    glfwSetCursor(ui.app_window,ui.requested_cursor==MOUSE_CURSOR_RESIZE_EW?ui.resize_cursor:ui.requested_cursor==MOUSE_CURSOR_RESIZE_NS?ui.track_cursor:ui.requested_cursor==MOUSE_CURSOR_POINTING_HAND?ui.hand_cursor:NULL);
    ui.current_cursor=ui.requested_cursor;
}

float *playback_loop(void) { return ui.song?ui.song_loop:ui.pattern_loop[ui.pattern]; }

int captured(void) { return ui.fm_graph_drag>=0 || ui.row_zoom_drag>=0 || ui.eq_drag>=0 || ui.automation_node>=0 || ui.picker_drag>=0 || ui.navigation_drag>=0 || ui.track_resize>=0 || ui.rack_hdrag || ui.rack_vdrag || ui.piano_key_drag || ui.control_drag || ui.route_drag>=0 || ui.note_drag || ui.velocity_drag>=0 || ui.arrangement.gesture || ui.playlist_pan || ui.browser_resize || ui.rack_paint>=0 || ui.cable_drag>0 || ui.playlist_vpan || ui.mixer_pan || ui.piano_scroll_drag || ui.piano_vdrag || ui.piano_gesture || ui.marker_drag>=0; }

float ui_scale(void) { return fmaxf(1,fminf(GetScreenWidth()/1200.f,GetScreenHeight()/675.f)); }

float playback_start(void) {
    float start=ui.song?ui.playlist_start*STEPS:ui.piano_start[ui.pattern];
    float end=ui.song?song_steps(&ui.project):ui.project.pattern_steps[ui.pattern];
    return ui.playing && !playback_loop()[1] && start>=end?0:start;
}

void transport_toggle(void) { ui.stop_armed=0; recording_finish(); int active=audio_stop(); ui.playing=ui.playing || active?0:1; ui.reset=1; }

void transport_stop(void) {
    int rewind=ui.stop_armed && !ui.playing;
    recording_finish(); audio_stop(); ui.playing=0; ui.reset=1;
    if(rewind) { ui.playlist_start=0; if(!ui.song) ui.piano_start[ui.pattern]=0; }
    ui.stop_armed=1;
}

void draw_editor(int id,float scale) {
    Editor *e=&ui.windows.editors[id]; if(!e->visible) return;
    ui.knob_context=id;
    if(id==4) { e->minh=ui.project.instrument[ui.instrument_channel]==INSTRUMENT_FM?420:300; e->rect.h=fmaxf(e->minh,e->rect.h); }
    Rect r=e->rect; int top=EDITORS-1;
    while(top>0 && !ui.windows.editors[ui.windows.order[top]].visible) top--;
    int focused=(ui.windows.editors[ui.windows.focused].visible?ui.windows.focused:ui.windows.order[top])==id;
    BeginScissorMode((int)(r.x*scale),(int)(r.y*scale),(int)(r.w*scale),(int)(r.h*scale));
    ui.text_origin=(Vector2){r.x*scale,r.y*scale};
    BeginMode2D((Camera2D){.offset=ui.text_origin,.zoom=scale});
    ui_surface((Rectangle){0,0,r.w,r.h},id==0?ui_theme.rack:id==3?ui_theme.mixer:panel);
    ui_surface((Rectangle){0,0,r.w,TITLE},focused?ui_theme.title_focus:ui_theme.title);
    const char *titles[]={"Channel Rack","Arrangement","Piano Roll","Mixer","Sampler","Equalizer"};
    label(TextFormat("%s%s",id==4 && ui.project.instrument[ui.instrument_channel]==INSTRUMENT_FM?"FM Synth":titles[id],e->pinned?" (on top)":""),8,3,13,ink);
    int rack_title_knob=id==0 && ((ui.mouse.x>=r.x+r.w-72-SWING_RADIUS-1 && ui.mouse.x<r.x+r.w-72+SWING_RADIUS+1) || (ui.mouse.x>=r.x+124 && ui.mouse.x<r.x+180));
    if(ui.input_enabled && !rack_title_knob && windows_hit(&ui.windows,ui.mouse.x,ui.mouse.y)==id && ui.mouse.y<r.y+TITLE && IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) open_context(1,id,ui.mouse);
    int chrome=ui.input_enabled && !rack_title_knob && windows_hit(&ui.windows,ui.mouse.x,ui.mouse.y)==id && ui.mouse.y<r.y+TITLE;
    int hover_slot=-1;
    if(chrome && ui.mouse.x>=r.x+r.w-54) {
        int slot=fminf(2,(ui.mouse.x-r.x-r.w+54)/18); hover_slot=slot;
        DrawRectangle(r.w-54+slot*18,1,17,TITLE-2,ui_theme.hover);
        snprintf(ui.status,sizeof ui.status,"%s window",slot==0?"Minimize":slot==1?e->maximized?"Restore":"Maximize":"Hide");
    } else if(chrome) snprintf(ui.status,sizeof ui.status,"Drag title bar to move; double-click to maximize/restore; right-click for window options");
    Color hover_ink=theme_foreground(ui_theme.hover);
    DrawLineEx((Vector2){r.w-49,9},(Vector2){r.w-41,9},2,hover_slot==0?hover_ink:ink);
    int mx=r.w-27; Color maximize_ink=hover_slot==1?hover_ink:ink;
    if(e->maximized) { DrawRectangleLines(mx-2,4,7,7,maximize_ink); DrawRectangle(mx-4,6,7,7,hover_slot==1?ui_theme.hover:cell); DrawRectangleLines(mx-4,6,7,7,maximize_ink); }
    else DrawRectangleLines(mx-4,5,8,8,maximize_ink);
    symbol("x",r.w-9,9,hover_slot==2?hover_ink:ink);
    ui.input_enabled=ui.input_enabled && ui.windows.owner==id && ui.windows.grab<0;
    Vector2 global=ui.mouse; ui.mouse.x-=r.x; ui.mouse.y-=r.y;
    if(id==0) {
        button("",124,2,56,TITLE-4,0);
        label(ui.rack_filters[ui.rack_filter],124+(56-text_width(ui.rack_filters[ui.rack_filter],11))/2,3,11,ink);
        if(hover(124,2,56,TITLE-4)) snprintf(ui.status,sizeof ui.status,"Channel filter: All, Audio (Playlist drops), Unsorted (Rack samples)");
        knob_style(r.w-72,TITLE/2,&ui.project.swing,0,1,0,"Swing",KNOB_SWING);
        if(hover(r.w-72-SWING_RADIUS-1,TITLE/2-SWING_RADIUS-1,2*SWING_RADIUS+2,2*SWING_RADIUS+2) || ui.control_drag==&ui.project.swing) snprintf(ui.status,sizeof ui.status,"Swing: %.0f%% | delays alternate steps; drag or wheel; right-click to enter",ui.project.swing*100);
    }
    ui.input_enabled=ui.input_enabled && ui.mouse.y>=TITLE && !(ui.mouse.x>r.w-12 && ui.mouse.y>r.h-12);
    if(ui.midi_take.active && id<=2) ui.input_enabled=0;
    if(id==0) rack(r.w,r.h); else if(id==1) playlist(r.w,r.h,scale); else if(id==2) piano(r.w,r.h); else if(id==3) mixer(r.w,r.h); else if(id==5) eq_editor(r.w,r.h); else if(ui.project.instrument[ui.instrument_channel]==INSTRUMENT_FM) fm_editor(r.w,r.h); else sampler(r.w,r.h);
    ui.mouse=global;
    DrawLine(r.w-9,r.h-2,r.w-2,r.h-9,muted);
    EndMode2D(); EndScissorMode(); ui.text_origin=(Vector2){0};
}

void navigate_editors(float scale) {
    ui.navigation_active=0;
    int blocked=ui.pattern_popup || ui.context_kind || ui.sample_drag[0] || !IsWindowFocused();
    if(ui.navigation_drag>=0 && (blocked || !IsMouseButtonDown(MOUSE_BUTTON_MIDDLE))) ui.navigation_drag=-1;
    int id=ui.navigation_drag>=0?ui.navigation_drag:windows_hit(&ui.windows,ui.mouse.x,ui.mouse.y);
    if(blocked || (id!=1 && id!=2) || (ui.navigation_drag<0 && captured()) || ui.windows.grab>=0) return;
    Rect r=ui.windows.editors[id].rect;
    float gx=id==1?212:60,gy=id==1?PLAYLIST_GRID_TOP:PIANO_GRID_TOP,gridw=r.w-gx-24;
    float area=id==1?r.h-gy-PLAYLIST_BOTTOM:r.h-PIANO_GRID_PADDING;
    Vector2 local={ui.mouse.x-r.x,ui.mouse.y-r.y};
    if(ui.navigation_drag<0 && (local.x<(id==1?120:4) || local.x>=r.w-24 || local.y<gy || local.y>=gy+area)) return;
    int middle=ui.navigation_drag>=0;
    if(!middle && IsMouseButtonPressed(MOUSE_BUTTON_MIDDLE)) {
        windows_focus(&ui.windows,id); ui.navigation_drag=id; ui.navigation_last=ui.mouse; middle=1;
    }
    NavigationMotion motion={0};
    if(middle) {
        motion.x=ui.navigation_last.x-ui.mouse.x; motion.y=ui.navigation_last.y-ui.mouse.y;
        ui.navigation_last=ui.mouse; SetMouseCursor(MOUSE_CURSOR_RESIZE_ALL);
    } else motion=navigation_motion(ui.navigation_input,IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT),shortcut_down(),scale);
    if(motion.x || motion.y || motion.zoom || middle) { ui.browser_focus=0; ui.navigation_active=1; }
    if(id==1) {
        float span=BARS/ui.arrangement.zoom;
        ui.arrangement.view_start=fmaxf(0,ui.arrangement.view_start+motion.x*span/gridw);
        if(motion.zoom) arrangement_zoom(&ui.arrangement,motion.zoom,fmaxf(0,fminf(1,(local.x-gx)/gridw)));
        ui.track_scroll=fmaxf(0,fminf(fmaxf(0,track_position(LANES)-area),ui.track_scroll+motion.y));
    } else {
        ui.piano_pan[ui.pattern]=fmaxf(0,ui.piano_pan[ui.pattern]+motion.x*ui.piano_span[ui.pattern]/gridw);
        if(motion.zoom && local.x>=gx) timeline_zoom(&ui.piano_span[ui.pattern],&ui.piano_pan[ui.pattern],motion.zoom,fmaxf(0,fminf(1,(local.x-gx)/gridw)),STEPS);
        ui.piano_scroll_remainder+=motion.y/(area/piano_visible_rows());
        int rows=(int)ui.piano_scroll_remainder;
        ui.piano_top=fmaxf(piano_min_top(),fminf(127,ui.piano_top-rows)); ui.piano_scroll_remainder-=rows;
        if(ui.piano_top==piano_min_top() || ui.piano_top==127) ui.piano_scroll_remainder=0;
    }
}
