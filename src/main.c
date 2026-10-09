// SPDX-License-Identifier: GPL-3.0-only
#include "ui_internal.h"

int main(int argc,char **argv) {
    int smoke=0;
    for(int i=1;i<argc;i++) {
        if(!strcmp(argv[i],"--smoke")) smoke=1;
        else if(!strcmp(argv[i],"--version")) { printf("LibreLoop %s\n",LIBRELOOP_VERSION); return 0; }
    }
    resource_paths();
    if(smoke && (!DirectoryExists(ui.samples_path) || !FileExists(ui.font_path) || !FileExists(ui.logo_path))) {
        fprintf(stderr,"Smoke check failed: bundled samples, font or logo are missing.\n"); return 1;
    }
#ifdef __APPLE__
    /* Finder launches may start in /, where default saves cannot be written. */
    if(!smoke && !strcmp(GetWorkingDirectory(),"/")) {
        const char *home=getenv("HOME"); char directory[PATH_MAX];
        if(!home || snprintf(directory,sizeof directory,"%s/Music/LibreLoop",home)>=(int)sizeof directory
            || MakeDirectory(directory)!=0 || !ChangeDirectory(directory)) {
            fprintf(stderr,"Cannot open a writable LibreLoop working directory.\n"); return 1;
        }
    }
#endif
    project_new(&ui.project); project_document_saved(&ui.document,&ui.project,NULL); history_checkpoint();
    for(int c=0;c<CHANNELS;c++) ui.sampler_applied[c]=ui.project.sampler[c];
    unsigned flags=FLAG_WINDOW_RESIZABLE;
#ifdef __APPLE__
    flags|=FLAG_WINDOW_HIGHDPI;
#endif
    /* On Linux, raylib 5.5's high-DPI flag rescales mouse/scissors separately
       from our 2D cameras. Keep window coordinates consistent; rasterize fonts
       using actual framebuffer density rather than the monitor's DPI setting. */
    SetConfigFlags(flags); InitWindow(1200,675,"LibreLoop " LIBRELOOP_VERSION " - pattern workstation");
    Image logo_image=LoadImage(ui.logo_path);
    if(logo_image.data) {
        SetWindowIcon(logo_image); UnloadImage(logo_image);
    }
    circles_init(); arcs_init(); cables_init();
    SetWindowMinSize(900,506); SetTargetFPS(60); SetExitKey(KEY_NULL);
    int audio_ok=audio_start(&ui.project,ui.samples);
    if(!audio_ok) snprintf(ui.status,sizeof ui.status,"Audio device unavailable. Editing and WAV export are available.");
    ui.app_window=glfwGetCurrentContext();
    ui.raylib_mouse_callback=glfwSetMouseButtonCallback(ui.app_window,mouse_callback);
#ifdef __APPLE__
    macos_navigation_init(ui.app_window);
#endif
    ui.resize_cursor=glfwCreateStandardCursor(GLFW_HRESIZE_CURSOR);
    ui.track_cursor=glfwCreateStandardCursor(GLFW_VRESIZE_CURSOR);
    ui.hand_cursor=glfwCreateStandardCursor(GLFW_HAND_CURSOR);
    browser_init(&ui.browser,ui.samples_path); midi_initialize(); theme_init(); file_chooser_locations(ui.browser.config); if(smoke) new_project(1); int frames=0,initialized=0; double last_activity=GetTime();
    while(!ui.document.quit) {
        if(WindowShouldClose()) {
            glfwSetWindowShouldClose(ui.app_window,GLFW_FALSE);
            if(!smoke) replace_project(REPLACE_QUIT); else break;
            if(ui.document.quit) break;
        }
        if(ui.sampler_job.busy && atomic_load(&ui.sampler_job.done)) last_activity=GetTime();
        sampler_update();
        if(ui.edit_clipboard.kind==COPY_CLIPS && (ui.edit_clipboard.channels!=ui.project.channel_count || ui.edit_clipboard.patterns!=ui.project.pattern_count || ui.edit_clipboard.automations!=ui.project.automation_count)) ui.edit_clipboard.kind=COPY_EMPTY;
        Vector2 wheel=GetMouseWheelMoveV();
        ui.navigation_input=(NavigationInput){wheel.x,wheel.y,0,0};
#ifdef __APPLE__
        NavigationInput native=macos_navigation_poll();
        if(native.precise) ui.navigation_input=native;
        else ui.navigation_input.zoom=native.zoom;
#endif
        Vector2 movement=GetMouseDelta();
        if(movement.x || movement.y || ui.navigation_input.x || ui.navigation_input.y || ui.navigation_input.zoom || IsWindowResized()) last_activity=GetTime();
        for(int key=32;key<=KEY_KB_MENU;key++) if(IsKeyPressed(key) || IsKeyReleased(key) || IsKeyPressedRepeat(key)) last_activity=GetTime();
        for(int b=0;b<8;b++) if(ui.pending_clicks[b] || IsMouseButtonReleased(b)) last_activity=GetTime();
        for(int b=0;b<8;b++) { ui.frame_clicks[b]=ui.pending_clicks[b]>0; if(ui.frame_clicks[b]) ui.pending_clicks[b]--; }
        float scale=ui_scale();
        int framebuffer_width,framebuffer_height,window_width,window_height;
        glfwGetFramebufferSize(ui.app_window,&framebuffer_width,&framebuffer_height);
        glfwGetWindowSize(ui.app_window,&window_width,&window_height);
        float raster_scale=scale*(window_width>0 && framebuffer_width>0?framebuffer_width/(float)window_width:1);
        (void)framebuffer_height; (void)window_height;
        fonts_update(raster_scale);
        float width=GetScreenWidth()/scale,height=GetScreenHeight()/scale;
        float sidebar=ui.browser_hidden?24:ui.browser_width;
        Rect desktop={sidebar+6,42,width-sidebar-10,height-66};
        if(!initialized) { windows_init(&ui.windows,width,height); initialized=1; }
        ui.mouse=GetMousePosition(); ui.mouse.x/=scale; ui.mouse.y/=scale; ui.reset=0; ui.popup_opened=0; ui.context_opened=0;
        SetMouseCursor(MOUSE_CURSOR_DEFAULT);
        navigate_editors(scale);
        if(IsMouseButtonPressed(MOUSE_BUTTON_LEFT) || IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) arrangement_release(&ui.arrangement);
        if(ui.arrangement.gesture) {
            Rect r=ui.windows.editors[1].rect;
            arrangement_drag(&ui.arrangement,&ui.project,ui.arrangement.view_start+(ui.mouse.x-r.x-212)/((r.w-236)/BARS*ui.arrangement.zoom),track_at(ui.track_scroll+ui.mouse.y-r.y-PLAYLIST_GRID_TOP));
            int held=ui.arrangement.gesture==ERASE_CLIPS?(IsMouseButtonDown(MOUSE_BUTTON_RIGHT) || IsMouseButtonDown(MOUSE_BUTTON_LEFT)):IsMouseButtonDown(MOUSE_BUTTON_LEFT);
            if(!held) arrangement_release(&ui.arrangement);
        }
        if(ui.fm_graph_drag>=0) {
            if(!IsMouseButtonDown(MOUSE_BUTTON_LEFT) || IsMouseButtonPressed(MOUSE_BUTTON_LEFT) || ui.pattern_popup || ui.context_kind || !IsWindowFocused() ||
               ui.instrument_channel!=ui.fm_graph_channel || ui.project.instrument[ui.fm_graph_channel]!=INSTRUMENT_FM || !ui.windows.editors[4].visible) ui.fm_graph_drag=-1;
            else fm_graph_update();
        }
        if(ui.row_zoom_drag>=0) {
            if(!IsMouseButtonDown(MOUSE_BUTTON_LEFT) || IsMouseButtonPressed(MOUSE_BUTTON_LEFT) || ui.pattern_popup || ui.context_kind || !IsWindowFocused()) ui.row_zoom_drag=-1;
            else { row_zoom_update(ui.mouse.y); SetMouseCursor(MOUSE_CURSOR_RESIZE_NS); }
        }
        if(ui.playlist_pan) {
            ui.arrangement.view_start=fmaxf(0,ui.playlist_pan_start+(ui.mouse.x-ui.playlist_pan_x)*ui.playlist_pan_range);
            if(!IsMouseButtonDown(MOUSE_BUTTON_LEFT) || IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) ui.playlist_pan=0;
        }
        if(ui.playlist_vpan) {
            Rect r=ui.windows.editors[1].rect; float area=r.h-PLAYLIST_GRID_TOP-PLAYLIST_BOTTOM+14;
            float total=track_position(LANES),travel=area-timeline_thumb(area,area-14,total),maximum=fmaxf(0,total-(area-14));
            ui.track_scroll=fmaxf(0,fminf(maximum,ui.playlist_vpan_start+(ui.mouse.y-ui.playlist_vpan_y)/fmaxf(1,travel)*maximum));
            if(!IsMouseButtonDown(MOUSE_BUTTON_LEFT) || IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) ui.playlist_vpan=0;
        }
        if(ui.track_resize>=0) {
            ui.track_heights[ui.track_resize]=roundf(fmaxf(32,fminf(320,ui.track_resize_height+ui.mouse.y-ui.track_resize_y)))/ui.track_zoom;
            SetMouseCursor(MOUSE_CURSOR_RESIZE_NS);
            if(!IsMouseButtonDown(MOUSE_BUTTON_LEFT)) ui.track_resize=-1;
        }
        if(ui.mixer_pan) {
            Rect r=ui.windows.editors[3].rect; float area=r.w-MIXER_PANEL-MIXER_LEFT-8; int visible=fmaxf(1,area/51-1);
            ui.mixer_scroll=fmaxf(0,fminf(INSERTS-visible,(ui.mouse.x-r.x-MIXER_LEFT-area*visible/INSERTS/2)/area*INSERTS));
            if(!IsMouseButtonDown(MOUSE_BUTTON_LEFT) || IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) ui.mixer_pan=0;
        }
        if(ui.cable_drag>0) {
            if(IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) ui.cable_drag=-1;
            else if(!IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
                int destination=mixer_input_at(ui.mouse);
                if(destination>=0) snprintf(ui.status,sizeof ui.status,insert_connect(&ui.project,ui.cable_drag,destination)?"Mixer cable connected.":"Connection blocked: it would create a feedback loop.");
                ui.cable_drag=-1;
            }
        }
        if(ui.rack_paint>=0) {
            if(IsMouseButtonPressed(MOUSE_BUTTON_LEFT) || IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) ui.rack_paint=-1;
            else {
                Rect r=ui.windows.editors[0].rect; paint_rack(r.w,ui.mouse.x-r.x,ui.mouse.y-r.y);
                if(!IsMouseButtonDown(ui.rack_paint)) ui.rack_paint=-1;
            }
        }
        int modal=ui.pattern_popup!=0 || ui.context_kind!=0,dragging=ui.sample_drag[0]!=0;
        if(!IsMouseButtonDown(MOUSE_BUTTON_LEFT) || IsMouseButtonPressed(MOUSE_BUTTON_LEFT) || modal) { ui.route_drag=-1; ui.control_drag=NULL; ui.note_drag=NULL; }
        if(!modal && !captured() && shortcut_down() && IsKeyPressed(KEY_Z)) undo_redo(IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT)?1:-1);
#ifndef __APPLE__
        if(!modal && !captured() && shortcut_down() && IsKeyPressed(KEY_Y)) undo_redo(1);
#endif
        if(!modal && !captured() && !dragging && shortcut_down() && IsKeyPressed(KEY_S)) project_file_action(IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT)?3:2);
        if(!modal && !captured() && !dragging && shortcut_down() && IsKeyPressed(KEY_O)) replace_project(3);
        if(!modal && !captured() && !dragging && !ui.browser_focus && shortcut_down()) {
            if(ui.windows.focused==2 && ui.piano_tool==PENCIL && (IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_K))) piano_octave(1);
            else if(ui.windows.focused==2 && ui.piano_tool==PENCIL && (IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_J))) piano_octave(-1);
            else if(IsKeyPressed(KEY_X)) edit_selection(0);
            else if(IsKeyPressed(KEY_C)) edit_selection(1);
            else if(IsKeyPressed(KEY_V)) edit_selection(2);
            else if(IsKeyPressed(KEY_A)) edit_selection(3);
        }
        if(!modal && !captured() && !dragging && !ui.browser_focus && !shortcut_down() &&
           !IsKeyDown(KEY_LEFT_ALT) && !IsKeyDown(KEY_RIGHT_ALT) &&
           (IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT))) {
            if(IsKeyPressed(KEY_LEFT) || IsKeyPressedRepeat(KEY_LEFT)) piano_shift(-1,0,"left one step");
            else if(IsKeyPressed(KEY_RIGHT) || IsKeyPressedRepeat(KEY_RIGHT)) piano_shift(1,0,"right one step");
            else if(IsKeyPressed(KEY_UP) || IsKeyPressedRepeat(KEY_UP)) piano_shift(0,1,"up one semitone");
            else if(IsKeyPressed(KEY_DOWN) || IsKeyPressedRepeat(KEY_DOWN)) piano_shift(0,-1,"down one semitone");
        }
        if(!modal && IsKeyPressed(KEY_SPACE)) transport_toggle();
        if(ui.rack_hdrag) {
            ui.rack_view[ui.pattern]=fmaxf(0,ui.rack_hstart+(ui.mouse.x-ui.rack_hx)*ui.rack_hscale);
            if(!IsMouseButtonDown(MOUSE_BUTTON_LEFT) || IsMouseButtonPressed(MOUSE_BUTTON_LEFT) || modal) ui.rack_hdrag=0;
        }
        if(ui.rack_vdrag) {
            ui.rack_scroll=fmaxf(0,ui.rack_vstart+(ui.mouse.y-ui.rack_vy)*ui.rack_vscale);
            if(!IsMouseButtonDown(MOUSE_BUTTON_LEFT) || IsMouseButtonPressed(MOUSE_BUTTON_LEFT) || modal) ui.rack_vdrag=0;
        }
        if(ui.piano_key_drag) {
            Rect r=ui.windows.editors[2].rect;
            piano_key_update(ui.mouse.x-r.x,ui.mouse.y-r.y,modal || !IsMouseButtonDown(MOUSE_BUTTON_LEFT) || IsMouseButtonPressed(MOUSE_BUTTON_LEFT) || !IsWindowFocused() || !ui.windows.editors[2].visible);
        }
        if(ui.marker_drag>=0) {
            int id=ui.marker_drag; Rect r=ui.windows.editors[id].rect;
            float gx=id==0?200:id==1?212:60,width=r.w-(id==0?226:id==1?236:84);
            float span=id==0?width/RACK_STEP_WIDTH:id==1?BARS/ui.arrangement.zoom*STEPS:ui.piano_span[ui.pattern];
            float start=id==0?ui.rack_view[ui.pattern]:id==1?ui.arrangement.view_start*STEPS:ui.piano_pan[ui.pattern];
            float q=id==0?1:grid_interval(width/span);
            int button=ui.ruler_loop_drag?MOUSE_BUTTON_RIGHT:MOUSE_BUTTON_LEFT;
            if(modal || IsMouseButtonPressed(button)) ui.marker_drag=-1;
            else {
                float at=fmaxf(0,snap_round(start+(ui.mouse.x-r.x-gx)/width*span,q));
                if(ui.ruler_loop_drag) {
                    float *range=id==1?ui.song_loop:ui.pattern_loop[ui.pattern];
                    range[0]=fminf(ui.ruler_anchor,at); range[1]=fmaxf(ui.ruler_anchor,at);
                    if(range[1]>range[0]) { if(id==1) ui.playlist_start=range[0]/STEPS; else ui.piano_start[ui.pattern]=range[0]; }
                } else if(id==1) ui.playlist_start=at/STEPS; else ui.piano_start[ui.pattern]=at;
                if(!ui.playing) ui.reset=1;
                if(!IsMouseButtonDown(button)) ui.marker_drag=-1;
            }
        }
        if(!modal && !dragging && !captured() && IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && ui.mouse.x>=sidebar && ui.mouse.x<sidebar+6 && ui.mouse.y>=42 && ui.mouse.y<height-22) ui.browser_resize=1;
        if(ui.browser_resize) {
            ui.browser_hidden=ui.mouse.x<120;
            if(!ui.browser_hidden) ui.browser_width=fminf(fminf(600,width-460),ui.mouse.x);
            sidebar=ui.browser_hidden?24:ui.browser_width;
            desktop=(Rect){sidebar+6,42,width-sidebar-10,height-66};
            if(!IsMouseButtonDown(MOUSE_BUTTON_LEFT)) ui.browser_resize=0;
        }
        if(sidebar!=ui.last_sidebar) {
            Editor *list=&ui.windows.editors[1];
            if(fabsf(list->rect.x-ui.last_sidebar-6)<1 && fabsf(list->rect.w-(width-ui.last_sidebar-10))<1) list->rect=desktop;
            ui.last_sidebar=sidebar;
        }
        if(dragging) {
            if(fabsf(ui.mouse.x-ui.sample_origin.x)+fabsf(ui.mouse.y-ui.sample_origin.y)>5) ui.sample_moved=1;
            if(IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) ui.sample_drag[0]=0;
            else if(IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
                if(ui.sample_moved && !modal) drop_sample(ui.sample_drag);
                ui.sample_drag[0]=0;
            } else if(!IsMouseButtonDown(MOUSE_BUTTON_LEFT) || IsKeyPressed(KEY_ESCAPE)) ui.sample_drag[0]=0;
        }
        dragging=ui.sample_drag[0]!=0;
        if(IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) ui.windows.grab=-1;
        if(!modal && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) ui.browser_focus=ui.mouse.x<sidebar && ui.mouse.y>=42 && ui.mouse.y<height-22;
        if(ui.control_drag) {
            float value=ui.control_fader?fader_gain(fmaxf(0,fminf(1,(ui.control_bottom-ui.mouse.y)/ui.control_range))):*ui.control_drag-GetMouseDelta().y/scale*(ui.control_reverse?-1:1)*(ui.control_drag==&ui.project.bpm?.25f:(ui.control_high-ui.control_low)/160);
            if(ui.control_logarithmic) value=*ui.control_drag*powf(ui.control_high/ui.control_low,-GetMouseDelta().y/scale/160);
            if(ui.control_integer) { ui.control_raw=fmaxf(ui.control_low,fminf(ui.control_high,ui.control_raw-GetMouseDelta().y/scale*(ui.control_high-ui.control_low)/160)); value=roundf(ui.control_raw); }
            *ui.control_drag=fmaxf(ui.control_low,fminf(ui.control_high,value));
        }
        if(ui.piano_scroll_drag) {
            ui.piano_pan[ui.pattern]=fmaxf(0,ui.piano_scroll_start+(ui.mouse.x-ui.piano_scroll_x)*ui.piano_scroll_range);
            if(!IsMouseButtonDown(MOUSE_BUTTON_LEFT) || IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) ui.piano_scroll_drag=0;
        }
        if(ui.piano_vdrag) {
            Rect r=ui.windows.editors[2].rect; float area=r.h-PIANO_GRID_PADDING+14;
            ui.piano_top=127-(int)fmaxf(0,fminf(127-piano_min_top(),ui.piano_vscroll_start+(ui.mouse.y-ui.piano_vscroll_y)/area*128));
            if(!IsMouseButtonDown(MOUSE_BUTTON_LEFT) || IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) ui.piano_vdrag=0;
        }
        if(ui.piano_gesture) {
            int right=ui.piano_gesture==PIANO_ERASE;
            if(modal || IsMouseButtonPressed(MOUSE_BUTTON_LEFT) || IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) ui.piano_gesture=PIANO_IDLE;
            else {
                Rect r=ui.windows.editors[2].rect; float cw=(r.w-84)/ui.piano_span[ui.pattern],rh=(r.h-PIANO_GRID_PADDING)/piano_visible_rows();
                piano_gesture_update(ui.piano_pan[ui.pattern]+(ui.mouse.x-r.x-60)/cw,ui.piano_top-(ui.mouse.y-r.y-PIANO_GRID_TOP)/rh,grid_interval(cw));
                if(!IsMouseButtonDown(right?MOUSE_BUTTON_RIGHT:MOUSE_BUTTON_LEFT)) ui.piano_gesture=PIANO_IDLE;
            }
        }
        if(ui.note_drag) {
            Rect r=ui.windows.editors[2].rect; float cw=(r.w-84)/ui.piano_span[ui.pattern],rh=(r.h-PIANO_GRID_PADDING)/piano_visible_rows();
            piano_note_drag_update(ui.piano_pan[ui.pattern]+(ui.mouse.x-r.x-60)/cw,(ui.mouse.y-r.y-PIANO_GRID_TOP)/rh,grid_interval(cw));
        }
        if(ui.velocity_drag>=0) {
            int button=ui.velocity_drag?MOUSE_BUTTON_RIGHT:MOUSE_BUTTON_LEFT;
            if(modal || ui.pattern!=ui.velocity_pattern || ui.piano_channel!=ui.velocity_channel ||
               IsMouseButtonPressed(MOUSE_BUTTON_LEFT) || IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) ui.velocity_drag=-1;
            else {
                Rect r=ui.windows.editors[2].rect; float cw=(r.w-84)/ui.piano_span[ui.pattern];
                Vector2 point={ui.piano_pan[ui.pattern]+(ui.mouse.x-r.x-60)/cw,
                    fmaxf(1,fminf(127,(r.y+r.h-22-ui.mouse.y)/.48f))};
                Vector2 from=ui.velocity_drag?ui.velocity_from:ui.velocity_now;
                notes_velocity(ui.project.notes[ui.pattern][ui.piano_channel],ui.velocity_drag?ui.velocity_before:NULL,
                    from.x,point.x,from.y,point.y,3/cw);
                ui.velocity_now=point;
                if(!IsMouseButtonDown(button)) ui.velocity_drag=-1;
            }
        }
        if(ui.route_drag>=0) {
            ui.project.route[ui.route_drag]=(int)fmaxf(0,fminf(ui.project.insert_count,ui.route_start+(int)((ui.route_y-ui.mouse.y)/3)));
            ui.mixer_selected=ui.project.route[ui.route_drag];
            int visible=fmaxf(1,(ui.windows.editors[3].rect.w-MIXER_PANEL-MIXER_LEFT-8)/51-1);
            if(ui.mixer_selected>ui.mixer_scroll+visible) ui.mixer_scroll=ui.mixer_selected-visible;
            else if(ui.mixer_selected>0 && ui.mixer_selected<=ui.mixer_scroll) ui.mixer_scroll=ui.mixer_selected-1;
            snprintf(ui.status,sizeof ui.status,"%s -> %s",ui.project.channel_names[ui.route_drag],mixer_name(ui.mixer_selected));
        }
        Rect filter_rect=ui.windows.editors[0].rect;
        if(!modal && !captured() && !dragging && windows_hit(&ui.windows,ui.mouse.x,ui.mouse.y)==0 && ui.mouse.x>=filter_rect.x+124 && ui.mouse.x<filter_rect.x+180 && ui.mouse.y>=filter_rect.y+2 && ui.mouse.y<filter_rect.y+TITLE-2 && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            windows_focus(&ui.windows,0); open_context(6,0,(Vector2){filter_rect.x+124,filter_rect.y+TITLE}); modal=1;
        }
        if(!modal && !captured() && !dragging) windows_update(&ui.windows,desktop,ui.mouse.x,ui.mouse.y,IsMouseButtonPressed(MOUSE_BUTTON_LEFT),IsMouseButtonDown(MOUSE_BUTTON_LEFT),GetTime());
        else { if(ui.browser_resize) windows_update(&ui.windows,desktop,ui.mouse.x,ui.mouse.y,0,0,GetTime()); ui.windows.owner=-1; ui.windows.grab=-1; }
        int focused=EDITORS-1;
        while(focused>0 && !ui.windows.editors[ui.windows.order[focused]].visible) focused--;
        if(!modal && !dragging && !captured() && !ui.browser_focus && !ui.midi_take.active && ui.windows.order[focused]==1 && ui.windows.editors[1].visible && delete_pressed()) {
            for(int l=0;l<LANES;l++) for(int b=0;b<CLIPS;b++) if(ui.arrangement.selected[l][b]) { ui.project.clips[l][b]=0; ui.project.clip_steps[l][b]=0; }
            memset(ui.arrangement.selected,0,sizeof ui.arrangement.selected);
        }
        if(!modal && !dragging && !captured() && !ui.browser_focus && !ui.midi_take.active && ui.windows.order[focused]==2 && ui.windows.editors[2].visible && delete_pressed()) {
            for(int i=0;i<NOTES;i++) if(ui.note_selected[ui.pattern][ui.piano_channel][i]) ui.project.notes[ui.pattern][ui.piano_channel][i].velocity=0;
            memset(ui.note_selected[ui.pattern][ui.piano_channel],0,NOTES);
        }
        if(!modal && IsFileDropped()) {
            FilePathList dropped=LoadDroppedFiles();
            for(unsigned i=0;i<dropped.count;i++) {
                const char *path=dropped.paths[i];
                if(IsFileExtension(path,PROJECT_FILE_SUFFIX)) request_load_project(path);
                else drop_sample(path);
            }
            UnloadDroppedFiles(dropped);
        }
        meters_update();
        spectrum_update();
        track_activity_update();
        if(ui.windows.editors[1].visible) previews_update(raster_scale);
        ui.knob_context=-1; ui.browser_progress=-1;
        BeginDrawing(); ClearBackground(bg); BeginMode2D((Camera2D){.zoom=scale});
        ui.input_enabled=!modal && !dragging && !captured() && ui.windows.grab<0 && ui.mouse.y<42;
        ui_surface((Rectangle){0,0,width,40},panel);
        ui_surface((Rectangle){8,8,176,22},cell);
        if(button("FILE",8,8,44,22,ui.pattern_popup==1)) open_popup(1);
        if(hover(8,8,44,22)) snprintf(ui.status,sizeof ui.status,"File: New, Demo, Save, Open, Export or Collect samples");
        if(button("EDIT",52,8,44,22,ui.pattern_popup==10)) open_popup(10);
        if(hover(52,8,44,22)) snprintf(ui.status,sizeof ui.status,"Edit: Undo, Redo, Cut, Copy, Paste and Select all");
        if(button("VIEW",96,8,44,22,ui.pattern_popup==7)) open_popup(7);
        if(hover(96,8,44,22)) snprintf(ui.status,sizeof ui.status,"View: MIDI recording settings");
        if(button("HELP",140,8,44,22,ui.pattern_popup==9)) open_popup(9);
        label(fit_text(TextFormat("%s%s",GetFileName(ui.document.path),project_dirty()?" *":""),176,11),8,29,11,muted);
        const int gap=4,pitch_x=184+gap+KNOB_RADIUS,volume_x=pitch_x+KNOB_RADIUS*2+gap;
        const int mode_x=volume_x+KNOB_RADIUS+gap,play_x=mode_x+52+gap,stop_x=play_x+24+gap,record_x=stop_x+24+gap;
        const int metro_x=record_x+24+3*22+gap,tempo_x=metro_x+22+gap;
        const int position_x=tempo_x+56+12;
        /* Keep editor shortcuts visible while the output monitor shrinks on narrow windows. */
        const int audio_x=position_x+88+12;
        const int analyzer_width=(int)fminf(108,fmaxf(0,width-audio_x-208));
        const int editors_x=audio_x+analyzer_width+gap+40+12;
        const int follow_x=editors_x+4*24,keys_x=follow_x+24;
        label("Pitch",pitch_x-text_width("Pitch",8)/2,30,8,muted);
        label("Vol",volume_x-text_width("Vol",8)/2,30,8,muted);
        knob_style(pitch_x,19,&ui.project.master_pitch,-12,12,0,"Master pitch (semitones)",KNOB_CENTER);
        if(hover(pitch_x-11,8,22,22) || ui.control_drag==&ui.project.master_pitch) snprintf(ui.status,sizeof ui.status,"Master pitch: %+.2f semitones (sample speed; right-click to enter)",ui.project.master_pitch);
        knob_style(volume_x,19,&ui.output_volume,0,VOLUME_KNOB_MAX,1,"LibreLoop output volume",KNOB_VOLUME);
        if(hover(volume_x-11,8,22,22) || ui.control_drag==&ui.output_volume) snprintf(ui.status,sizeof ui.status,"LibreLoop output volume: listening level; Master and WAV export unchanged");
        if(button("",mode_x,8,52,22,0) && !recording_active()) { ui.song=!ui.song; ui.reset=1; }
        DrawRectangle(mode_x,8,52,11,ui.song?cell:ui_theme.patt);
        DrawRectangle(mode_x,19,52,11,ui.song?accent:cell);
        label("PATT",mode_x+(52-text_width("PATT",10))/2,8,10,ui.song?muted:ui_theme.selected_text);
        label("SONG",mode_x+(52-text_width("SONG",10))/2,19,10,ui.song?ui_theme.selected_text:muted);
        if(hover(mode_x,8,52,22)) snprintf(ui.status,sizeof ui.status,"Playback mode: click to switch Pattern (orange) / Song (green)");
        int transport_active=ui.playing || audio_preview_position(ui.audition)>=0 || audio_key_position(127,ui.instrument_channel)>=0;
        if(button(transport_active?"||":">",play_x,8,24,22,transport_active)) transport_toggle();
        if(hover(play_x,8,24,22)) snprintf(ui.status,sizeof ui.status,"Play from ruler / stop all audio, including previews (Space)");
        if(button("[]",stop_x,8,24,22,0)) transport_stop();
        if(hover(stop_x,8,24,22)) snprintf(ui.status,sizeof ui.status,"Stop all audio; press again while stopped to return the start marker to the beginning");
        if(button("",record_x,8,24,22,recording_active())) recording_start();
        circle(record_x+12,19,7,ui_theme.meter_high);
        if(hover(record_x,8,24,22) && IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)){midi_refresh();open_popup(13);}
        const int record_icons[]={ICON_WAVE,ICON_PIANO,ICON_AUTOMATION};
        const char *record_names[]={"Audio from armed mixer tracks","MIDI notes into a new pattern","MIDI / mouse control automation"};
        int record_input=ui.input_enabled;if(recording_active())ui.input_enabled=0;
        for(int i=0;i<3;i++){int xx=record_x+24+i*22;
            if(button("",xx,8,22,22,ui.record_mask&(1u<<i))){ui.record_mask^=1u<<i;midi_save_settings();}
            icon(record_icons[i],xx+11,19,17,ui.record_mask&(1u<<i)?ui_theme.selected_text:muted);
            if(hover(xx,8,22,22))snprintf(ui.status,sizeof ui.status,"Record %s: %s",record_names[i],ui.record_mask&(1u<<i)?"enabled":"disabled");
        }ui.input_enabled=record_input;
        if(hover(record_x,8,24,22)) snprintf(ui.status,sizeof ui.status,recording_active()?"Recording: click to finish the takes":"Record enabled types; right-click for MIDI inputs and recording settings");
        if(button("",metro_x,8,22,22,ui.metronome)) { ui.metronome=!ui.metronome; audio_metronome(ui.metronome); }
        icon(ICON_METRO,metro_x+11,19,20,ui.metronome?ui_theme.selected_text:ink);
        if(hover(metro_x,8,22,22)) snprintf(ui.status,sizeof ui.status,"Metronome: %s | beat clicks during playback; first beat accented; excluded from WAV export",ui.metronome?"On":"Off");
        if(drag_button(TextFormat("%.2f",ui.project.bpm),tempo_x,8,56,22,ui.control_drag==&ui.project.bpm) && !recording_active()) capture_control(&ui.project.bpm,30,300,0);
        if(hover(tempo_x,8,56,22) && !recording_active()) {
            ui.project.bpm=fmaxf(30,fminf(300,ui.project.bpm+GetMouseWheelMove()));
            snprintf(ui.status,sizeof ui.status,"Tempo: drag up/down; right-click for Reset or Enter value");
            if(IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) control_menu(&ui.project.bpm,30,300,120,"Tempo (BPM)");
        }
        uint64_t step=(uint64_t)(audio_position()/(RATE*60.0/ui.project.bpm/4));
        ui_surface((Rectangle){position_x,8,88,22},bg);
        label(TextFormat("%03llu:%02llu",(unsigned long long)(step/16+1),(unsigned long long)(step%16+1)),position_x+10,10,18,ui_theme.signal);
        if(hover(position_x,8,88,22)) snprintf(ui.status,sizeof ui.status,"Playback position: bar and sixteenth-note step");
        for(int i=0;i<4;i++) { int id=i==1?2:i==2?1:i; editor_button(id,editors_x+i*24); }
        if(button("",follow_x,8,22,22,ui.follow_playhead)) { ui.follow_playhead=!ui.follow_playhead; if(!ui.follow_playhead) { ui.arrangement.view_start=fmaxf(0,ui.arrangement.view_start); for(int p=0;p<PATTERNS;p++) ui.piano_pan[p]=fmaxf(0,ui.piano_pan[p]); } }
        icon(ICON_FOLLOW,follow_x+11,19,20,ui.follow_playhead?ui_theme.selected_text:ink);
        if(hover(follow_x,8,22,22)) snprintf(ui.status,sizeof ui.status,"Follow playhead: center playback in the Playlist (Song) or Piano Roll (PAT)");
        if(button("",keys_x,8,22,22,ui.typing_keys)) ui.typing_keys=!ui.typing_keys;
        icon(ICON_KEYS,keys_x+11,19,20,ui.typing_keys?ui_theme.selected_text:ink);
        if(hover(keys_x,8,22,22)) snprintf(ui.status,sizeof ui.status,"Typing piano: Z row C3, Q row C4; S/D/G/H/J and 2/3/5/6/7 sharps; disabled in Browser/text fields");
        recording_poll();midi_poll();
        ui.visual_step=ui.playing && !ui.reset?audio_visual_position()/(RATE*60.0/ui.project.bpm/4):playback_start(); automation_display_update();
        int browser_hover=!ui.browser_hidden && ui.mouse.x<sidebar && ui.mouse.y>=42 && ui.mouse.y<height-22;
        typing_piano(modal || dragging || browser_hover);
        if(analyzer_width>0) {
            DrawRectangle(audio_x,8,analyzer_width,22,bg);
            spectrum_draw(&ui.master_spectrum,(Rectangle){audio_x+2,9,analyzer_width-4,20},0);
            int mx=audio_x+analyzer_width+gap; DrawRectangle(mx,8,40,22,bg);
            for(int side=0;side<2;side++) {
                float level=fmaxf(0,fminf(1,(20*log10f(fmaxf(.001f,ui.meter_level[0][side]))+60)/60));
                DrawRectangle(mx+2,10+side*10,36*level,7,accent);
                if(ui.meter_hold[0][side]>=1) DrawRectangle(mx+36,10+side*10,2,7,ui_theme.meter_high);
            }
            if(hover(audio_x,8,analyzer_width+gap+40,22)) snprintf(ui.status,sizeof ui.status,audio_ok?"Master frequency spectrum and stereo level · red indicates clipping":"Audio device unavailable");
        }
        ui.input_enabled=!modal && !ui.pattern_popup && !ui.context_kind && !dragging && !captured() && ui.windows.grab<0 && ui.mouse.x<sidebar+6 && ui.mouse.y>=42;
        DrawRectangle(0,42,sidebar,height-66,ui_theme.browser);
        if(!ui.browser_hidden) {
            if(hover(0,42,sidebar,height-66)) snprintf(ui.status,sizeof ui.status,"Browser: click to preview; drag to load | Up/Down or j/k: select | Left/Right or h/l: fold");
            label("Browser",10,50,14,ink);
            if(button("+ Add folder",8,76,sidebar-16,23,0)) { snprintf(ui.folder_text,sizeof ui.folder_text,"%s",ui.browser.path); ui.rename_select_all=1; open_popup(3); }
            int fy=108,rows=fmaxf(1,(height-fy-98)/23);
            if(!modal && !ui.pattern_popup && !ui.context_kind && (ui.browser_focus || browser_hover) && !dragging && !captured() && !command_down()) {
                if(IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_K)) audition_entry((int)fmaxf(0,ui.browser.selected-1));
                if(IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_J)) audition_entry((int)fminf(ui.browser.items-1,ui.browser.selected+1));
                if(IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_H)) browser_left(&ui.browser);
                if(IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_L)) browser_right(&ui.browser);
            }
            if(ui.browser.follow && ui.browser.selected>=0) {
                ui.browser.follow=0;
                if(ui.browser.selected<ui.browser.scroll) ui.browser.scroll=ui.browser.selected;
                if(ui.browser.selected>=ui.browser.scroll+rows) ui.browser.scroll=ui.browser.selected-rows+1;
            }
            if(hover(0,fy,sidebar,rows*23)) ui.browser.scroll-=(int)GetMouseWheelMove()*3;
            ui.browser.scroll=(int)fmaxf(0,fminf(fmaxf(0,ui.browser.items-rows),ui.browser.scroll));
            BeginScissorMode(0,fy*scale,(sidebar-4)*scale,rows*23*scale);
            for(int entry=ui.browser.scroll;entry<ui.browser.items && entry<ui.browser.scroll+rows;entry++) {
                BrowserNode *node=&ui.browser.nodes[entry]; const char *path=node->path;
                int y=fy+(entry-ui.browser.scroll)*23,indent=12+node->depth*14;
                int clicked=button_color("",8,y,sidebar-16,21,ui.browser.selected==entry,ui_theme.browser);
                Color color=ui.browser.selected==entry?ui_theme.selected_text:ink;
                if(node->dir) {
                    if(node->open) DrawTriangle((Vector2){indent,y+8},(Vector2){indent+4,y+13},(Vector2){indent+8,y+8},color);
                    else DrawTriangle((Vector2){indent,y+6},(Vector2){indent,y+14},(Vector2){indent+6,y+10},color);
                }
                const char *name=GetFileName(path); if(!*name) name=path;
                label(fit_text(node->depth==0 && !strcmp(path,ui.browser.samples)?"LibreLoop samples":name,fmaxf(0,sidebar-indent-24),13),indent+14,y+4,13,color);
                if(hover(8,y,sidebar-16,21)) snprintf(ui.status,sizeof ui.status,"%.80s: %s | j/k: select; h/l: fold",path,node->dir?"click to expand/collapse":IsFileExtension(path,PROJECT_FILE_SUFFIX)?"click to open project":"click to preview; drag to Rack or Playlist");
                if(clicked) {
                    browser_select(&ui.browser,entry);
                    if(node->dir) { if(!browser_toggle(&ui.browser,entry)) snprintf(ui.status,sizeof ui.status,"Folder unavailable: %.180s",path); break; }
                    if(IsFileExtension(path,PROJECT_FILE_SUFFIX)) request_load_project(path);
                    else {
                        audition_entry(entry); snprintf(ui.sample_drag,sizeof ui.sample_drag,"%s",path);
                        ui.sample_origin=ui.mouse; ui.sample_moved=0;
                        ui.sample_offset=(Vector2){ui.mouse.x-8,ui.mouse.y-y}; ui.sample_drag_width=sidebar-16; ui.sample_text_offset=indent+6;
                    }
                }
            }
            EndScissorMode();
            if(ui.browser.selected>=0 && ui.browser.selected<ui.browser.items && ui.audition.data && !strcmp(ui.browser.nodes[ui.browser.selected].path,ui.audition_path)) {
                draw_waveform((WaveDisplay){.sample=ui.audition,.wave=&ui.audition_wave,.origin=8,.width=sidebar-16},
                    (Rectangle){8,height-98,sidebar-16,68},8,sidebar-8,ui_theme.waveform);
                if(hover(8,height-98,sidebar-16,68)) {
                    snprintf(ui.status,sizeof ui.status,"Click waveform to replay %.100s | Up/Down or j/k: select; Left/Right or h/l: fold",GetFileName(ui.audition_path));
                    if(IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) { ui.browser_focus=1; audio_preview(ui.audition); }
                }
                ui.browser_progress=audio_preview_position(ui.audition);
                if(ui.browser_progress>=0) {
                    float x=8+ui.browser_progress*(sidebar-16);
                    DrawLineEx((Vector2){x,height-98},(Vector2){x,height-30},1.5f,ink);
                }
            }
        }
        int resize_hover=hover(sidebar,42,6,height-66);
        DrawRectangle(sidebar,42,6,height-66,ui.browser_resize || resize_hover?accent:cell);
        if(resize_hover || ui.browser_resize) { SetMouseCursor(MOUSE_CURSOR_RESIZE_EW); snprintf(ui.status,sizeof ui.status,"Drag to resize Browser; below minimum width collapses it; drag outward to restore"); }
        EndMode2D();
        /* Freeze draw order: opening Piano Roll while drawing must not draw a window twice. */
        int order[EDITORS]; memcpy(order,ui.windows.order,sizeof order);
        for(int i=0;i<EDITORS;i++) { ui.input_enabled=!modal && !ui.pattern_popup && !ui.context_kind && !dragging && !ui.sample_drag[0] && !captured(); draw_editor(order[i],scale); }

        BeginMode2D((Camera2D){.zoom=scale}); DrawRectangle(0,height-22,width,22,panel); label(fit_text(ui.status,width-16,11),8,height-17,11,ink);
        draw_browser_drag();
        draw_picker_drag();
        draw_popup(); draw_context(); EndMode2D();

        if(!captured() && !recording_active()) history_checkpoint();
        if(ui.pattern_popup!=11 && ui.file_picker.entries) file_chooser_close(&ui.file_picker);
        sampler_queue();
        if(recording_active()) { ui.project.bpm=ui.recording_ui.active?ui.recording_ui.bpm:ui.midi_take.bpm; ui.song=ui.playing=1; ui.reset=0; }
        flush_cursor(); audio_update(&ui.project,ui.playing,ui.song,ui.pattern,ui.reset,ui.output_volume,playback_start(),recording_active()?0:playback_loop()[0],recording_active()?0:playback_loop()[1]);
        int animate=audio_active() || ui.mixer_decay_active || ui.spectrum_active || (ui.windows.editors[0].visible && ui.channel_decay_active) || ui.browser_progress>=0 || (ui.windows.editors[3].visible && (audio_active() || ui.mixer_decay_active)) || ui.playing || captured() || ui.windows.grab>=0 || ui.popup_drag || ui.sample_drag[0] ||
            (ui.windows.editors[4].visible && audio_key_position(127,ui.instrument_channel)>=0);
        if(!smoke && !animate && GetTime()-last_activity>.15) EnableEventWaiting(); else DisableEventWaiting();
        EndDrawing();
        if(smoke && ++frames%10==0) {
            int stage=frames/10;
            TakeScreenshot(stage==1?"libreloop-smoke.png":TextFormat("libreloop-view-%d.png",stage-1));
            if(stage==4) break;
            if(stage==1) windows_focus(&ui.windows,1);
            if(stage==2) windows_focus(&ui.windows,2);
            if(stage==3) windows_focus(&ui.windows,3);
        }
    }
    recording_finish();midi_panic();midi_input_wake(NULL);midi_input_close(); history_clear(&ui.edit_history); file_chooser_close(&ui.file_picker);
    if(ui.sampler_job.busy) { pthread_join(ui.sampler_job.thread,NULL); sample_free(ui.sampler_job.input); sample_free(ui.sampler_job.result); }
    for(int pat=0;pat<PATTERNS;pat++) if(ui.previews[pat].image.id) UnloadRenderTexture(ui.previews[pat].image);
    #ifdef __APPLE__
    macos_navigation_close();
#endif
    text_fonts_close(); UnloadTexture(ui.circle_texture); UnloadTexture(ui.icons); UnloadTexture(ui.knob_arcs); UnloadTexture(ui.cable_texture);
    browser_close(&ui.browser); audio_close(); sample_free(ui.audition); free(ui.audition_wave.tree); if(ui.resize_cursor) glfwDestroyCursor(ui.resize_cursor); if(ui.track_cursor) glfwDestroyCursor(ui.track_cursor); if(ui.hand_cursor) glfwDestroyCursor(ui.hand_cursor); CloseWindow();
    for(int c=0;c<CHANNELS;c++) { free(ui.audio_waves[c].tree); free(ui.sampler_views[c].wave.tree); sample_free(ui.samples[c]); sample_free(ui.originals[c]); }
    return 0;
}
