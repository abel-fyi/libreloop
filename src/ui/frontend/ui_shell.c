// SPDX-License-Identifier: GPL-3.0-only
#include "ui_internal.h"
#include "editor_navigation.h"
#include "rack.h"

int shortcut_down(void) {
#ifdef __APPLE__
    return host_is_key_down(HOST_KEY_LEFT_SUPER) || host_is_key_down(HOST_KEY_RIGHT_SUPER);
#else
    return host_is_key_down(HOST_KEY_LEFT_CONTROL) || host_is_key_down(HOST_KEY_RIGHT_CONTROL);
#endif
}

int command_down(void) {
    return shortcut_down() || host_is_key_down(HOST_KEY_LEFT_CONTROL) || host_is_key_down(HOST_KEY_RIGHT_CONTROL)
        || host_is_key_down(HOST_KEY_LEFT_SUPER) || host_is_key_down(HOST_KEY_RIGHT_SUPER);
}

int delete_pressed(void) {
#ifdef __APPLE__
    return host_is_key_pressed(HOST_KEY_DELETE) || host_is_key_pressed(HOST_KEY_BACKSPACE);
#else
    return host_is_key_pressed(HOST_KEY_DELETE);
#endif
}

void resource_paths(void) {
    const char *base=host_get_application_directory();
#ifdef __APPLE__
    snprintf(ui.logo_path,sizeof ui.logo_path,"%sbranding/logo.png",base);
    snprintf(ui.samples_path,sizeof ui.samples_path,"%ssamples",base);
    if(ui.studio_style)snprintf(ui.font_path,sizeof ui.font_path,"%sfonts/PxPlus_IBM_VGA_8x16.ttf",base);
    else snprintf(ui.font_path,sizeof ui.font_path,"%sfonts/LiberationSans-Regular.ttf",base);
#else
    snprintf(ui.logo_path,sizeof ui.logo_path,"%s../share/libreloop/branding/logo.png",base);
    snprintf(ui.samples_path,sizeof ui.samples_path,"%s../share/libreloop/samples",base);
    if(ui.studio_style)snprintf(ui.font_path,sizeof ui.font_path,"%s../share/libreloop/fonts/PxPlus_IBM_VGA_8x16.ttf",base);
    else snprintf(ui.font_path,sizeof ui.font_path,"%s../share/libreloop/fonts/LiberationSans-Regular.ttf",base);
#endif
    if(!host_file_exists(ui.logo_path))snprintf(ui.logo_path,sizeof ui.logo_path,"%s",LIBRELOOP_LOGO);
    if(!host_directory_exists(ui.samples_path) || !host_file_exists(ui.font_path)) {
        snprintf(ui.samples_path,sizeof ui.samples_path,"%s",LIBRELOOP_SAMPLES);
#ifdef LIBRELOOP_STUDIO_FONT
        snprintf(ui.font_path,sizeof ui.font_path,"%s",ui.studio_style?LIBRELOOP_STUDIO_FONT:LIBRELOOP_FONT);
#else
        snprintf(ui.font_path,sizeof ui.font_path,"%s",LIBRELOOP_FONT);
#endif
    }
}

int mouse_pressed(int button) { return button>=0 && button<8 && ui.frame_clicks[button]; }

void request_cursor(int cursor) { ui.requested_cursor=cursor; }

void flush_cursor(void) { platform_window_cursor(ui.app_window,ui.requested_cursor); ui.current_cursor=ui.requested_cursor; }

float *playback_loop(void) { return ui.song?ui.song_loop:ui.pattern_loop[ui.pattern]; }

int captured(void) { return ui.dynamics_drag || ui.fx_press || ui.velocity_lane.dragging || (ui.windows.workspace.enabled && ui.windows.workspace.drag>=0) || ui.fm_graph_drag>=0 || ui.row_zoom_drag>=0 || ui.eq_drag>=0 || ui.automation_node>=0 || ui.picker_drag>=0 || ui.navigation_drag>=0 || ui.track_resize>=0 || ui.rack_hdrag || ui.rack_vdrag || ui.piano_key_drag || ui.control_drag || ui.route_drag>=0 || ui.note_drag || ui.velocity_drag>=0 || ui.arrangement.gesture || ui.playlist_pan || ui.browser_resize || ui.rack_paint>=0 || ui.cable_drag>0 || ui.playlist_vpan || ui.mixer_pan || ui.piano_scroll_drag || ui.piano_vdrag || ui.piano_gesture || ui.marker_drag>=0 || ui.pattern_hit_active; }

float ui_scale(void) { if(ui.studio_style)return studio_ui_scale(host_get_screen_width(),host_get_screen_height(),ui.ui_scale_preference);return fmaxf(1,fminf(host_get_screen_width()/(ui.studio_style?1448.f:1200.f),host_get_screen_height()/(ui.studio_style?1086.f:675.f))); }

float playback_start(void) {
    float start=ui.song?ui.playlist_start*STEPS:ui.piano_start[ui.pattern];
    float end=ui.song?song_steps(&ui.project):pattern_playback_steps(&ui.project,ui.pattern);
    return ui.playing && !playback_loop()[1] && start>=end?0:start;
}

void transport_toggle(void) { ui.stop_armed=0; recording_finish(); int active=audio_transport_stop(); ui.playing=ui.playing || active?0:1; ui.reset=1; }

void transport_stop(void) {
    int rewind=ui.stop_armed && !ui.playing;
    recording_finish(); audio_stop(); ui.playing=0; ui.reset=1;
    if(rewind) { ui.playlist_start=0; if(!ui.song) ui.piano_start[ui.pattern]=0; }
    ui.stop_armed=1;
}

void draw_editor(int id,float scale) {
    Editor *e=&ui.windows.editors[id]; if(!e->visible) return;
    int saved_bus=ui.eq_bus,saved_slot=ui.eq_slot,saved_band=ui.eq_band;
    if(ui.windows.workspace.enabled && id>=5){ui.eq_bus=ui.effect_windows[id].bus;ui.eq_slot=ui.effect_windows[id].slot;ui.eq_band=ui.effect_windows[id].band;}
    ui.knob_context=id;
    if(id==4 && (!ui.windows.workspace.enabled || ui.windows.workspace.floating[id])) { int voice=ui.studio_style && ui.instrument_voice_page[ui.instrument_channel];e->minh=ui.studio_style?studio_instrument_height(voice,ui.project.instrument[ui.instrument_channel]==INSTRUMENT_FM,ui.project.fm[ui.instrument_channel].engine==1):ui.project.instrument[ui.instrument_channel]==INSTRUMENT_FM?420:sampler_min_height(0); e->rect.h=voice?e->minh:fmaxf(e->minh,e->rect.h); }
    Rect r=e->rect,frame=r;int title=windows_title_height(&ui.windows); int top=windows_count(&ui.windows)-1;
    while(top>0 && !ui.windows.editors[ui.windows.order[top]].visible) top--;
    int focused=(ui.windows.editors[ui.windows.focused].visible?ui.windows.focused:ui.windows.order[top])==id;
    host_begin_scissor_mode((int)(r.x*scale),(int)(r.y*scale),(int)(r.w*scale),(int)(r.h*scale));
    ui.text_origin=(DrawPoint){r.x*scale,r.y*scale};
    host_begin_mode2_d((HostCamera){.offset=ui.text_origin,.zoom=scale});
    DrawColor body=id==0?ui_theme.rack:id==3?ui_theme.mixer:panel;
    if(ui.studio_style && !ui.profile_flat)ui_panel(host_draw(),(DrawRect){0,0,r.w,r.h},body);
    else ui_surface(host_draw(),(DrawRect){0,0,r.w,r.h},body);
    if(ui.studio_style && !ui.profile_flat)ui_titlebar(host_draw(),(DrawRect){0,0,r.w,title},focused);
    else ui_surface(host_draw(),(DrawRect){0,0,r.w,title},focused?ui_theme.title_focus:ui_theme.title);
    DrawColor title_ink=ui.studio_style?theme_foreground(focused?ui_theme.title_focus:ui_theme.title):ink;
    if(ui.studio_style && !ui.profile_flat)for(int slot=0;slot<3;slot++) {
        if(ui.windows.workspace.enabled && id<4)continue;
        if(ui.windows.workspace.enabled && id>=4 && slot!=2)continue;
        DrawRect chrome_rect=ui.windows.workspace.enabled?(DrawRect){r.w-21,3,18,18}:(DrawRect){r.w-54+slot*18,2,16,TITLE-4};
        ui_button_face(host_draw(),chrome_rect,cell,0);
    }
    const char *titles[]={"Channel Rack","Arrangement","Piano Roll","Mixer","Sampler","Equalizer"};
    int title_size=ui.windows.workspace.enabled?14:13,title_y=3;
    if(ui.windows.workspace.enabled && (id==1 || id==2)) {
        label("VIEW",8,title_y,UI_TEXT_SIZE,title_ink);
        DrawPoint saved_mouse=ui.mouse;int saved_input=ui.input_enabled;
        ui.input_enabled=saved_input && windows_hit(&ui.windows,ui.mouse.x,ui.mouse.y)==id;
        ui.mouse.x-=r.x;ui.mouse.y-=r.y;
        int page=segmented_switch("Pattern","Arrangement",(DrawRect){48,2,192,title-4},id==1,1,ui_theme.highlight);
        if(hover(48,2,192,title-4))snprintf(ui.status,sizeof ui.status,"View: Pattern / Arrangement; playback stays unchanged");
        ui.mouse=saved_mouse;ui.input_enabled=saved_input;
        if(page!=(id==1))windows_focus(&ui.windows,page?1:2);
        if(id==2)label(fit_text(host_text_format("%s / %s",ui.project.pattern_names[ui.pattern],ui.project.channel_names[ui.piano_channel]),r.w-260,title_size),252,title_y,title_size,title_ink);
    }
    else if(id==4 && ui.studio_style){
        const char *name=channel_instrument_name(&ui.project,ui.instrument_channel),*channel=ui.project.channel_names[ui.instrument_channel];
        label(fit_text(!strcmp(name,channel)?name:host_text_format("%s · %s",name,channel),r.w-48,title_size),8,title_y,title_size,title_ink);
    }
    else if(id==2 && ui.studio_style)label(fit_text(host_text_format("Pattern · %s / %s",ui.project.pattern_names[ui.pattern],ui.project.channel_names[ui.piano_channel]),r.w-16,title_size),8,title_y,title_size,title_ink);
    else if(id>=5 && ui.windows.workspace.enabled)label(fit_text(host_text_format("%s · %s / %d",effect_descriptor(ui.project.effect_type[ui.eq_bus][ui.eq_slot])?effect_descriptor(ui.project.effect_type[ui.eq_bus][ui.eq_slot])->name:"Effect",mixer_name(ui.eq_bus),ui.eq_slot+1),r.w-48,title_size),8,title_y,title_size,title_ink);
    else label(host_text_format("%s%s",id==4 && ui.project.instrument[ui.instrument_channel]==INSTRUMENT_FM?"FM Synth":titles[id],e->pinned?" (on top)":""),8,title_y,title_size,title_ink);
    int rack_title_knob=id==0 && ((ui.mouse.x>=r.x+r.w-72-SWING_RADIUS-1 && ui.mouse.x<r.x+r.w-72+SWING_RADIUS+1) || (ui.mouse.x>=r.x+124 && ui.mouse.x<r.x+180));
    if(ui.input_enabled && !rack_title_knob && windows_hit(&ui.windows,ui.mouse.x,ui.mouse.y)==id && ui.mouse.y<r.y+title && host_is_mouse_button_pressed(HOST_MOUSE_BUTTON_RIGHT)) open_context(1,id,ui.mouse);
    int chrome=ui.input_enabled && !rack_title_knob && windows_hit(&ui.windows,ui.mouse.x,ui.mouse.y)==id && ui.mouse.y<r.y+title;
    int hover_slot=-1;
    if(chrome && ui.mouse.x>=r.x+r.w-54) {
        int slot=fminf(2,(ui.mouse.x-r.x-r.w+54)/18); hover_slot=slot;
        if(ui.windows.workspace.enabled && id<4)hover_slot=-1;
        if(ui.windows.workspace.enabled && id>=4)hover_slot=ui.mouse.x>=r.x+r.w-STUDIO_TITLE?2:-1;
        if(hover_slot>=0) {
            if(ui.windows.workspace.enabled)ui_button_face(host_draw(),(DrawRect){r.w-21,3,18,18},cell,host_is_mouse_button_down(HOST_MOUSE_BUTTON_LEFT));
            else host_draw_rectangle(r.w-54+slot*18,1,17,TITLE-2,ui_theme.hover);
        }
        if(hover_slot>=0)snprintf(ui.status,sizeof ui.status,"%s window",slot==0?"Minimize":slot==1?(ui.windows.workspace.enabled && !ui.windows.workspace.floating[id] && id>=4)?"Float":e->maximized?"Restore":"Maximize":"Hide");
    } else if(chrome && !(ui.windows.workspace.enabled && (id==1 || id==2) && ui.mouse.x<r.x+240))snprintf(ui.status,sizeof ui.status,ui.windows.workspace.enabled?(id>=4?"Drag title to move; close hides the editor and keeps audio running":"VIEW changes the editor; PLAYBACK chooses what plays; drag the Mixer divider to resize"):"Drag title bar to move; double-click to maximize/restore; right-click for window options");
    DrawColor hover_ink=theme_foreground(ui_theme.hover);
    if(!ui.windows.workspace.enabled)host_draw_line_ex((DrawPoint){r.w-49,9},(DrawPoint){r.w-41,9},2,hover_slot==0?hover_ink:ink);
    int mx=r.w-27; DrawColor maximize_ink=hover_slot==1?hover_ink:ink;
    if(ui.windows.workspace.enabled) {}
    else if(e->maximized) { host_draw_rectangle_lines(mx-2,4,7,7,maximize_ink); host_draw_rectangle(mx-4,6,7,7,hover_slot==1?ui_theme.hover:cell); host_draw_rectangle_lines(mx-4,6,7,7,maximize_ink); }
    else host_draw_rectangle_lines(mx-4,5,8,8,maximize_ink);
    int close_pressed=ui.studio_style && hover_slot==2 && host_is_mouse_button_down(HOST_MOUSE_BUTTON_LEFT);
    if(!ui.windows.workspace.enabled || id>=4)symbol("x",r.w-(ui.windows.workspace.enabled?12:9)+close_pressed,title/2+close_pressed,ui.windows.workspace.enabled?ink:hover_slot==2?hover_ink:ink);
    ui.input_enabled=ui.input_enabled && ui.windows.owner==id && ui.windows.grab<0 && !ui.velocity_lane.dragging;
    r=desktop_editor_rect(id);
    ui.text_origin=(DrawPoint){r.x*scale,r.y*scale};
    host_begin_mode2_d((HostCamera){.offset=ui.text_origin,.zoom=scale});
    DrawPoint global=ui.mouse; ui.mouse.x-=r.x; ui.mouse.y-=r.y;
    if(id==0) {
        button("",124,2,56,TITLE-4,0);
        label(ui.rack_filters[ui.rack_filter],124+(56-text_width(ui.rack_filters[ui.rack_filter],11))/2,3,11,ink);
        if(hover(124,2,56,TITLE-4)) snprintf(ui.status,sizeof ui.status,"Channel filter: All, Audio (Playlist drops), Unsorted (Rack samples)");
        knob_style(r.w-72,TITLE/2,&ui.project.swing,0,1,0,"Swing",KNOB_SWING);
        if(hover(r.w-72-SWING_RADIUS-1,TITLE/2-SWING_RADIUS-1,2*SWING_RADIUS+2,2*SWING_RADIUS+2) || ui.control_drag==&ui.project.swing) snprintf(ui.status,sizeof ui.status,"Swing: %.0f%% | delays alternate steps; drag or wheel; right-click to enter",ui.project.swing*100);
    }
    ui.input_enabled=ui.input_enabled && ui.mouse.y>=TITLE && !(!ui.windows.workspace.enabled && ui.mouse.x>r.w-12 && ui.mouse.y>r.h-12);
    if(ui.midi_take.active && id<=2) ui.input_enabled=0;
    if(id==0) rack(r.w,r.h); else if(id==1) playlist(r.w,r.h,scale); else if(id==2) {
        if(ui.studio_style) {
            int content_input=ui.input_enabled;
            ui.input_enabled=content_input && ui.mouse.x<166;
            piano_tool_panel(166,r.h);
            Rect content=desktop_piano_rect();
            ui.mouse=(DrawPoint){global.x-content.x,global.y-content.y};
            ui.input_enabled=content_input && global.x>=content.x;
            ui.text_origin=(DrawPoint){content.x*scale,content.y*scale};
            host_begin_mode2_d((HostCamera){.offset=ui.text_origin,.zoom=scale});
            piano(content.w,content.h);
            host_begin_scissor_mode(r.x*scale,r.y*scale,r.w*scale,r.h*scale);
            ui.text_origin=(DrawPoint){r.x*scale,r.y*scale};
            host_begin_mode2_d((HostCamera){.offset=ui.text_origin,.zoom=scale});
        } else piano(r.w,r.h);
    } else if(id==3) mixer(r.w,r.h); else if(id>=5) effect_editor(r.w,r.h); else if(ui.studio_style && ui.instrument_voice_page[ui.instrument_channel])instrument_voice_editor(r.w,r.h); else if(ui.project.instrument[ui.instrument_channel]==INSTRUMENT_FM) fm_editor(r.w,r.h); else sampler(r.w,r.h);
    ui.mouse=global;
    r=frame;ui.text_origin=(DrawPoint){r.x*scale,r.y*scale};
    host_begin_mode2_d((HostCamera){.offset=ui.text_origin,.zoom=scale});
    if(ui.studio_style && !ui.profile_flat) {
        draw_border(host_draw(),(DrawRect){0,0,r.w,r.h},1,ui_theme.border);
    }
    if(!ui.windows.workspace.enabled)host_draw_line(r.w-9,r.h-2,r.w-2,r.h-9,muted);
    host_end_mode2_d(); host_end_scissor_mode(); ui.text_origin=(DrawPoint){0};
    if(ui.windows.workspace.enabled && id>=5){ui.effect_windows[id].band=ui.eq_band;ui.eq_bus=saved_bus;ui.eq_slot=saved_slot;ui.eq_band=saved_band;}
}

void navigate_editors(float scale) {
    EditorNavigation state={.windows=&ui.windows,.arrangement=&ui.arrangement,
        .pattern=ui.pattern,.drag=ui.navigation_drag,.active=ui.navigation_active,
        .browser_focus=ui.browser_focus,.last={ui.navigation_last.x,ui.navigation_last.y},
        .track_scroll=ui.track_scroll,.track_zoom=ui.track_zoom,.piano_zoom=ui.piano_zoom,
        .piano_remainder=ui.piano_scroll_remainder,.piano_top=ui.piano_top,
        .rack_enabled=ui.studio_style,.rack_count=0,.rack_scroll=ui.rack_scroll+ui.rack_scroll_remainder};
    memcpy(state.track_heights,ui.track_heights,sizeof ui.track_heights);
    memcpy(state.piano_span,ui.piano_span,sizeof ui.piano_span);
    memcpy(state.piano_pan,ui.piano_pan,sizeof ui.piano_pan);
    RackState rack_state={.filter=ui.rack_filter};state.rack_count=rack_channels(&rack_state,&ui.project,NULL);
    memcpy(state.rack_width,ui.rack_step_width,sizeof ui.rack_step_width);
    memcpy(state.rack_view,ui.rack_view,sizeof ui.rack_view);
    NavigationFrame frame={.pointer={ui.mouse.x,ui.mouse.y},.scroll=ui.navigation_input,
        .layout_scale=scale,.blocked=ui.pattern_popup || ui.context_kind || ui.sample_drag[0] || !host_is_window_focused(),
        .captured=captured(),.middle_pressed=host_is_mouse_button_pressed(HOST_MOUSE_BUTTON_MIDDLE),
        .middle_held=host_is_mouse_button_down(HOST_MOUSE_BUTTON_MIDDLE),
        .shift=host_is_key_down(HOST_KEY_LEFT_SHIFT) || host_is_key_down(HOST_KEY_RIGHT_SHIFT),.zoom_modifier=shortcut_down(),
        .piano_content=desktop_piano_grid_rect(),.playlist_content=desktop_editor_rect(1),.piano_reference_area=desktop_piano_reference_area(),.playlist_grid_x=playlist_grid_x(),
        .playlist_grid_top=playlist_grid_top(),.playlist_header_x=playlist_header_x()};
    editor_navigate(&state,frame);
    ui.navigation_drag=state.drag;ui.navigation_active=state.active;ui.browser_focus=state.browser_focus;
    ui.navigation_last=(DrawPoint){state.last.x,state.last.y};ui.track_scroll=state.track_scroll;
    ui.piano_zoom=state.piano_zoom;ui.piano_scroll_remainder=state.piano_remainder;ui.piano_top=state.piano_top;
    memcpy(ui.piano_span,state.piano_span,sizeof ui.piano_span);memcpy(ui.piano_pan,state.piano_pan,sizeof ui.piano_pan);
    ui.rack_scroll=(int)state.rack_scroll;ui.rack_scroll_remainder=state.rack_scroll-ui.rack_scroll;
    memcpy(ui.rack_step_width,state.rack_width,sizeof ui.rack_step_width);
    memcpy(ui.rack_view,state.rack_view,sizeof ui.rack_view);
    if(state.pan_cursor)host_set_mouse_cursor(HOST_MOUSE_CURSOR_RESIZE_ALL);
}

void workspace_view_scale(float *aw,float *pw) {
    float a=ui.windows.editors[1].rect.w-playlist_grid_x()-24,p=ui.windows.editors[2].rect.w-250;
    if(*aw>0 && a>0)ui.arrangement.zoom*=*aw/a;
    if(*pw>0 && p>0)for(int i=0;i<PATTERNS;i++)ui.piano_span[i]*=p/(*pw);
    *aw=a;*pw=p;
}
void workspace_dividers(void) {
    float scale=ui_scale();
    if(!ui.windows.workspace.show[3]) {
        StudioWorkspace *s=&ui.windows.workspace;
        host_begin_mode2_d((HostCamera){.zoom=scale});
        if(button("Mixer (collapsed)  ^",s->sidebar+6,s->height-24-STUDIO_TITLE,s->width-s->sidebar-10,STUDIO_TITLE,0))windows_focus(&ui.windows,3);
        DrawRect rail={s->sidebar+6,s->height-24-STUDIO_TITLE,s->width-s->sidebar-10,STUDIO_TITLE};
        ui_divider_grip(host_draw(),rail,0,hover(rail.x,rail.y,rail.width,rail.height));
        label("Mixer (collapsed)  ^",rail.x+8,rail.y+(rail.height-UI_TEXT_SIZE)/2,UI_TEXT_SIZE,ink);
        host_end_mode2_d();
    }
    if(ui.windows.editors[3].visible) {
        Rect r=ui.windows.editors[3].rect;
        host_begin_mode2_d((HostCamera){.zoom=scale});
        ui_divider_grip(host_draw(),(DrawRect){r.x,r.y-6,r.w,6},0,
            ui.windows.workspace.drag==3 || hover(r.x,r.y-6,r.w,6));
        host_end_mode2_d();
    }
}
