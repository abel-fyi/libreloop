// SPDX-License-Identifier: GPL-3.0-only
#include "ui_internal.h"
#include "shortcuts.h"
#include "studio_layout.h"
#include <time.h>
static int profile_order(const void *a,const void *b){double x=*(const double *)a,y=*(const double *)b;return (x>y)-(x<y);}

/* A real, editable project for reviewing the complete classic desktop. */
static void smoke_showcase_prepare(void) {
    Note drums[3][NOTES],bass[NOTES];
    memcpy(drums,ui.project.notes[0],sizeof drums);memcpy(bass,ui.project.notes[1][3],sizeof bass);
    memset(ui.project.notes,0,sizeof ui.project.notes);memset(ui.project.clips,0,sizeof ui.project.clips);
    memset(ui.project.clip_steps,0,sizeof ui.project.clip_steps);
    ui.project.pattern_count=4;ui.project.channel_count=7;
    const char *patterns[]={"Drums","Bass","Pad","Lead"};
    const uint32_t colors[]={0x7fa6ba,0x9aad7b,0x95bcb5,0xc8b77a};
    for(int p=0;p<4;p++){snprintf(ui.project.pattern_names[p],PATTERN_NAME,"%s",patterns[p]);ui.project.pattern_colors[p]=colors[p];ui.project.pattern_steps[p]=p==2?64:p?32:16;}
    memcpy(ui.project.notes[0],drums,sizeof drums);memcpy(ui.project.notes[1][3],bass,sizeof bass);
    ui.project.instrument[3]=ui.project.instrument[4]=ui.project.instrument[6]=INSTRUMENT_FM;
    ui.project.fm[3]=fm_factory(6);ui.project.fm[4]=fm_factory(5);ui.project.fm[6]=fm_factory(7);
    snprintf(ui.project.channel_names[3],PATTERN_NAME,"Bass");snprintf(ui.project.channel_names[4],PATTERN_NAME,"Pad");snprintf(ui.project.channel_names[6],PATTERN_NAME,"Lead");
    for(int bar=0;bar<4;bar++)for(int voice=0;voice<3;voice++)ui.project.notes[2][4][bar*3+voice]=(Note){48+voice*7+(bar%2)*2,78,bar*16,12};
    for(int n=0;n<8;n++)ui.project.notes[3][6][n]=(Note){60+n%5*2,90,n*4,2};
    ui.project.sampler[5].time=4;char path[LL_PATH_CAPACITY];
    snprintf(path,sizeof path,"%s/melodic/Tone.wav",ui.samples_path);
    if(!import_sample(path,5))return;
    ui.project.channel_audio[5]=1;ui.project.channel_colors[5]=0x978ab6;
    for(int c=0;c<7;c++){ui.project.route[c]=c+1;ui.project.volume[c]=c>=3?.35f:ui.project.volume[c];}
    const char *tracks[]={"Drums","Bass","Pad","Lead","Sample","Variation","Sample reprise","Pad volume"};
    for(int lane=0;lane<8;lane++)snprintf(ui.project.track_names[lane],PATTERN_NAME,"%s",tracks[lane]);
    for(int c=0;c<7;c++)snprintf(ui.project.insert_names[c],PATTERN_NAME,"%s",ui.project.channel_names[c]);
    for(int b=0;b<16;b++){ui.project.clips[0][b]=1;ui.project.clip_starts[0][b]=b;}
    for(int b=0;b<16;b+=2){ui.project.clips[1][b]=2;ui.project.clip_starts[1][b]=b;}
    for(int b=0;b<12;b+=4){ui.project.clips[2][b]=3;ui.project.clip_starts[2][b]=b;}
    for(int b=4;b<16;b+=4){ui.project.clips[3][b]=4;ui.project.clip_starts[3][b]=b;}
    for(int b=2;b<16;b+=4){ui.project.clips[4][b]=PATTERNS+6;ui.project.clip_starts[4][b]=b;}
    ui.project.clips[5][8]=4;ui.project.clip_starts[5][8]=8;
    ui.project.clips[6][0]=PATTERNS+6;ui.project.clip_starts[6][0]=0;
    int a=automation_create(&ui.project,(ParameterTarget){PARAM_CHANNEL_VOLUME,4,0},"Pad volume",256);
    if(a>=0){Automation *curve=&ui.project.automations[a];curve->count=5;curve->color=0xa5b0b1;
        curve->points[0]=(AutomationPoint){0,.6f};curve->points[1]=(AutomationPoint){64,.8f};curve->points[2]=(AutomationPoint){128,.45f};curve->points[3]=(AutomationPoint){192,.75f};curve->points[4]=(AutomationPoint){256,.6f};
        ui.project.clips[7][0]=AUTOMATION_SOURCE+a+1;ui.project.clip_starts[7][0]=0;}
    for(int lane=0;lane<8;lane++)for(int b=0;b<CLIPS;b++)if(ui.project.clips[lane][b]) {
        int source=ui.project.clips[lane][b]-1;
        if(source<PATTERNS)ui.project.clip_steps[lane][b]=ui.project.pattern_steps[source];
        else if(source>=AUTOMATION_SOURCE)ui.project.clip_steps[lane][b]=ui.project.automations[source-AUTOMATION_SOURCE].steps;
    }
    ui.pattern=2;ui.channel=ui.piano_channel=ui.instrument_channel=4;ui.piano_channels[2][4]=1;ui.piano_span[2]=64;
    ui.piano_top=72;ui.track_heights[0]=ui.track_heights[1]=ui.track_heights[2]=ui.track_heights[3]=ui.track_heights[4]=ui.track_heights[5]=ui.track_heights[6]=ui.track_heights[7]=46;
    ui.arrangement.source_pattern=2;ui.arrangement.source_steps=64;
    snprintf(ui.status,sizeof ui.status,"Classic showcase: editable notes, samples and automation using existing LibreLoop features.");
}

/* Development capture states use only existing instruments/effects. */
static void smoke_scene_prepare(int scene) {
    if(scene==24){ui.project.effect_type[1][0]=EFFECT_DYNAMICS;ui.project.dynamics[1][0]=dynamics_default();ui.project.dynamics[1][0].lowcut=80;ui.project.effect_mix[1][0]=1;ui.eq_bus=1;ui.eq_slot=0;return;}
    if(scene==23){ui.project.effect_type[1][0]=EFFECT_TUBE;ui.project.tube[1][0]=tube_default();ui.project.tube[1][0].natural=1;ui.project.effect_mix[1][0]=1;ui.eq_bus=1;ui.eq_slot=0;return;}
    if(scene==21 || scene==22){ui.project.effect_type[1][0]=scene==21?EFFECT_PHASER:EFFECT_FLANGER;ui.project.effect_mix[1][0]=.5f;ui.eq_bus=1;ui.eq_slot=0;return;}
    if(scene==19 || scene==20){ui.project.effect_type[1][0]=EFFECT_DYNAMICS;ui.project.dynamics[1][0]=dynamics_factory(scene==20?0:1);ui.project.effect_mix[1][0]=1;ui.eq_bus=1;ui.eq_slot=0;return;}
    if(scene==17 || scene==18){ui.project.effect_type[1][0]=EFFECT_REVERB;ui.project.reverb[1][0]=reverb_factory(scene==18?1:0);ui.project.effect_mix[1][0]=reverb_factory_mix(scene==18?1:0);ui.eq_bus=1;ui.eq_slot=0;return;}
    if(scene==16){ui.project.effect_type[1][0]=EFFECT_TUBE;ui.project.effect_mix[1][0]=1;ui.eq_bus=1;ui.eq_slot=0;return;}
    if(scene==15){ui.project.effect_type[1][0]=EFFECT_DELAY;ui.project.effect_mix[1][0]=1;ui.eq_bus=1;ui.eq_slot=0;return;}
    if(scene==14){channel_replace_instrument(&ui.project,0,INSTRUMENT_FM);ui.project.fm[0].engine=1;ui.fm_dx_operator=6;ui.fm_dx_page=1;return;}
    if(scene==12){smoke_showcase_prepare();return;}
    if(scene==13){channel_replace_instrument(&ui.project,0,INSTRUMENT_FM);ui.project.fm[0].engine=0;ui.fm_tab=3;return;}
    if(scene>=1 && scene<=8) {
        channel_replace_instrument(&ui.project,0,INSTRUMENT_FM);
        ui.project.fm[0].engine=scene==1?0:(scene==2 || scene==7 || scene==8)?1:2;
        ui.fm_analog_tab=scene>=4 && scene<=6?scene-3:0;
        ui.fm_dx_page=scene==7;
        ui.fm_dx_operator=scene==8?6:0;
    } else if(scene>=9 && scene<=11) {
        int type=scene==9?EFFECT_CHORUS:EFFECT_EQ;
        ui.project.effect_type[1][0]=type;
        if(type==EFFECT_CHORUS)effect_descriptor(type)->defaults(&ui.project.chorus[1][0]);
        else effect_descriptor(type)->defaults(&ui.project.eq[1][0]);
        ui.project.effect_mix[1][0]=type==EFFECT_CHORUS?.5f:1;
        if(scene==9 || scene==11){ui.eq_bus=1;ui.eq_slot=0;}
    }
}

int main(int argc,char **argv) {
    ui_state_init();
    int smoke_playing=0,smoke_piano_maximized=0,smoke_new_project=0;
    float smoke_velocity=0,smoke_mixer_offset=0,smoke_fx_scroll=0;int smoke_seconds=0,smoke_fx_hidden=0,smoke_inserts=0,smoke_routing=0,smoke_fx_count=0;
    float smoke_ui_scale=0;
    float smoke_rack_zoom=0;int smoke_grid_menu=0,smoke_grid=-1,smoke_workspace=0,smoke_fill=0,smoke_count=0;
    int smoke_patterns=0;
    int smoke=0,visual_preview=0,legacy_ui=0,smoke_scene=0,smoke_width=0,smoke_height=0,smoke_popup=0,profile_frames=0,profile_motion=0;
    for(int i=1;i<argc;i++) {
        if(!strcmp(argv[i],"--smoke")) smoke=1;
        else if(!strcmp(argv[i],"--smoke-patterns") && i+1<argc)smoke_patterns=atoi(argv[++i]);
        else if(!strcmp(argv[i],"--smoke-fx-count") && i+1<argc)smoke_fx_count=atoi(argv[++i]);
        else if(!strcmp(argv[i],"--smoke-routing"))smoke_routing=1;
        else if(!strcmp(argv[i],"--smoke-mixer-offset") && i+1<argc)smoke_mixer_offset=atof(argv[++i]);
        else if(!strcmp(argv[i],"--smoke-fx-scroll") && i+1<argc)smoke_fx_scroll=atof(argv[++i]);
        else if(!strcmp(argv[i],"--smoke-fx-hidden"))smoke_fx_hidden=1;
        else if(!strcmp(argv[i],"--smoke-inserts") && i+1<argc)smoke_inserts=atoi(argv[++i]);
        else if(!strcmp(argv[i],"--smoke-count-in") && i+1<argc)smoke_count=atoi(argv[++i]);
        else if(!strcmp(argv[i],"--smoke-fill-dialog"))smoke_fill=1;
        else if(!strcmp(argv[i],"--smoke-velocity-height") && i+1<argc)smoke_velocity=atof(argv[++i]);
        else if(!strcmp(argv[i],"--smoke-seconds"))smoke_seconds=1;
        else if(!strcmp(argv[i],"--smoke-workspace") && i+1<argc)smoke_workspace=atoi(argv[++i]);
        else if(!strcmp(argv[i],"--smoke-rack-zoom") && i+1<argc)smoke_rack_zoom=atof(argv[++i]);
        else if(!strcmp(argv[i],"--smoke-grid-menu") && i+1<argc)smoke_grid_menu=atoi(argv[++i]);
        else if(!strcmp(argv[i],"--smoke-grid") && i+1<argc)smoke_grid=atoi(argv[++i]);
        else if(!strcmp(argv[i],"--smoke-playing"))smoke_playing=1;
        else if(!strcmp(argv[i],"--smoke-piano-maximized"))smoke_piano_maximized=1;
        else if(!strcmp(argv[i],"--render-profile") && i+1<argc)profile_frames=atoi(argv[++i]);
        else if(!strcmp(argv[i],"--profile-flat"))ui.profile_flat=1;
        else if(!strcmp(argv[i],"--profile-motion"))profile_motion=1;
        else if(!strcmp(argv[i],"--smoke-size") && i+1<argc){if(sscanf(argv[++i],"%dx%d",&smoke_width,&smoke_height)!=2)return 1;}
        else if(!strcmp(argv[i],"--smoke-new-project"))smoke_new_project=1;
        else if(!strcmp(argv[i],"--smoke-ui-scale") && i+1<argc)smoke_ui_scale=atof(argv[++i]);
        else if(!strcmp(argv[i],"--smoke-popup") && i+1<argc)smoke_popup=atoi(argv[++i]);
        else if(!strcmp(argv[i],"--smoke-scene") && i+1<argc)smoke_scene=atoi(argv[++i]);
        else if(!strcmp(argv[i],"--visual-preview"))visual_preview=1;
        else if(!strcmp(argv[i],"--legacy-ui"))legacy_ui=1;
        else if(!strcmp(argv[i],"--version")) { printf("LibreLoop %s\n",LIBRELOOP_VERSION); return 0; }
    }
    if((smoke_mixer_offset || smoke_fx_scroll || smoke_fx_hidden || smoke_inserts || smoke_routing || smoke_fx_count) && !smoke)return 1;
    if(smoke_patterns && (!smoke || smoke_patterns<1 || smoke_patterns>PATTERNS))return 1;
    if(smoke_fx_count<0 || smoke_fx_count>EFFECT_SLOTS)return 1;
    if(!isfinite(smoke_mixer_offset) || !isfinite(smoke_fx_scroll) || smoke_inserts<0 || smoke_inserts>INSERTS)return 1;
    if((smoke_playing || smoke_piano_maximized) && !smoke)return 1;
    if((smoke_seconds || smoke_velocity || smoke_count || smoke_fill || smoke_workspace || smoke_rack_zoom || smoke_grid_menu || smoke_grid>=0) && !smoke)return 1;
    if(smoke_rack_zoom && (!isfinite(smoke_rack_zoom) || smoke_rack_zoom<.25f || smoke_rack_zoom>8))return 1;
    if(smoke_count<0 || smoke_count>4096)return 1;
    if(smoke_workspace<0 || smoke_workspace>9)return 1;
    if(smoke_grid_menu<0 || smoke_grid_menu>2 || smoke_grid>=GRID_CHOICES)return 1;
    if(smoke_scene<0 || smoke_scene>24 || (smoke_scene && !smoke))return 1;
    if(smoke_width && (!smoke || smoke_width<900 || smoke_height<562 || smoke_width>3840 || smoke_height>2160))return 1;
    if(smoke_new_project && !smoke)return 1;
    if(!isfinite(smoke_ui_scale) || (smoke_ui_scale && (!smoke || smoke_ui_scale<75 || smoke_ui_scale>200)))return 1;
    if(smoke)ui.ui_scale_preference=smoke_ui_scale/100;
    if(smoke_popup<0 || smoke_popup>14 || (smoke_popup && !smoke))return 1;
    if(profile_frames && (!smoke || profile_frames<100 || profile_frames>10000))return 1;
    if(ui.profile_flat && !profile_frames)return 1;
    double *profile_times=profile_frames?calloc(profile_frames,sizeof *profile_times):NULL;
    if(profile_frames && !profile_times)return 1;
    double profile_cpu=0;int profile_count=0;
    ui.studio_style=!legacy_ui && !smoke;
    if(visual_preview)ui.studio_style=1;
    if(ui.studio_style)ui.piano_zoom=2;
    resource_paths();
    if(smoke && (!host_directory_exists(ui.samples_path) || !host_file_exists(ui.font_path) || !host_file_exists(ui.logo_path))) {
        fprintf(stderr,"Smoke check failed: bundled samples, font or logo are missing.\n"); return 1;
    }
#ifdef __APPLE__
    /* Finder launches may start in /, where default saves cannot be written. */
    if(!smoke && !strcmp(host_get_working_directory(),"/")) {
        const char *home=getenv("HOME"); char directory[LL_PATH_CAPACITY];
        if(!home || snprintf(directory,sizeof directory,"%s/Music/LibreLoop",home)>=(int)sizeof directory
            || host_make_directory(directory)!=0 || !host_change_directory(directory)) {
            fprintf(stderr,"Cannot open a writable LibreLoop working directory.\n"); return 1;
        }
    }
#endif
    project_new(&ui.project); project_document_saved(&ui.document,&ui.project,NULL); history_checkpoint();
    for(int c=0;c<CHANNELS;c++) ui.sampler_applied[c]=ui.project.sampler[c];
    unsigned flags=HOST_FLAG_WINDOW_RESIZABLE;
#ifdef __APPLE__
    flags|=HOST_FLAG_WINDOW_HIGHDPI;
#endif
    /* On Linux, raylib 5.5's high-DPI flag rescales mouse/scissors separately
       from our 2D cameras. Keep window coordinates consistent; rasterize fonts
       using actual framebuffer density rather than the monitor's DPI setting. */
    host_set_config_flags(flags); host_init_window(smoke_width?smoke_width:ui.studio_style?1448:1200,smoke_height?smoke_height:ui.studio_style?1086:675,"LibreLoop " LIBRELOOP_VERSION " - pattern workstation");
    HostImage logo_image=host_load_image(ui.logo_path);
    if(logo_image.data) {
        host_set_window_icon(logo_image); host_unload_image(logo_image);
    }
    circles_init(); arcs_init(); cables_init();
    draw_palette_enable(host_draw(),ui.studio_style);
    if(ui.studio_style && !theme_draw_init(host_draw()))fprintf(stderr,"Desktop skin texture unavailable.\n");
    host_set_window_min_size(900,ui.studio_style?562:506);
    if(ui.studio_style && !smoke)platform_window_fit_display(host_window());
    host_set_target_f_p_s(profile_frames?0:60); host_set_exit_key(HOST_KEY_NULL);
    int audio_ok=audio_start(&ui.project,ui.samples);
    if(!audio_ok) snprintf(ui.status,sizeof ui.status,"Audio device unavailable. Editing and WAV export are available.");
    ui.app_window=host_window();
    browser_init(&ui.browser,ui.samples_path);if(!smoke && ui.studio_style)ui_scale_load(); midi_initialize(); theme_init(); if(ui.studio_style){theme_studio();ui.browser_width=238;ui.last_sidebar=238;} file_chooser_locations(ui.browser.config); if(smoke){new_project(smoke_new_project?0:1);smoke_scene_prepare(smoke_scene);while(ui.project.pattern_count<smoke_patterns)if(pattern_add(&ui.project)<0)return 1;if(smoke_playing){ui.playing=1;ui.reset=1;}} int frames=0,initialized=0;
    float studio_piano_width=0,studio_arrangement_width=0,last_ui_scale=0;
    double last_activity=host_get_time();
    while(!ui.document.quit) {
        if(host_window_should_close()) {

            if(!smoke) replace_project(REPLACE_QUIT); else break;
            if(ui.document.quit) break;
        }
        if(ui.sampler_job.busy && atomic_load(&ui.sampler_job.done)) last_activity=host_get_time();
        sampler_update();
        if(ui.edit_clipboard.kind==COPY_CLIPS && (ui.edit_clipboard.channels!=ui.project.channel_count || ui.edit_clipboard.patterns!=ui.project.pattern_count || ui.edit_clipboard.automations!=ui.project.automation_count)) ui.edit_clipboard.kind=COPY_EMPTY;
        DrawPoint wheel=host_get_mouse_wheel_move_v();
        ui.navigation_input=(NavigationInput){wheel.x,wheel.y,0,0};
#ifdef __APPLE__
        NavigationInput native=host_navigation();
        if(native.precise) ui.navigation_input=native;
        else ui.navigation_input.zoom=native.zoom;
#endif
        DrawPoint movement=host_get_mouse_delta();
        if(movement.x || movement.y || ui.navigation_input.x || ui.navigation_input.y || ui.navigation_input.zoom || host_is_window_resized()) last_activity=host_get_time();
        for(int key=32;key<=HOST_KEY_KB_MENU;key++) if(host_is_key_pressed(key) || host_is_key_released(key) || host_is_key_pressed_repeat(key)) last_activity=host_get_time();
        for(int b=0;b<8;b++) { ui.frame_clicks[b]=input_click(host_input(),b); if(ui.frame_clicks[b] || host_is_mouse_button_released(b))last_activity=host_get_time(); }
        float scale=ui_scale();
        if(scale!=last_ui_scale){studio_piano_width=studio_arrangement_width=0;last_ui_scale=scale;}
        int framebuffer_width,framebuffer_height,window_width,window_height;
        host_framebuffer_size(ui.app_window,&framebuffer_width,&framebuffer_height);
        host_window_size(ui.app_window,&window_width,&window_height);
        float raster_scale=scale*(window_width>0 && framebuffer_width>0?framebuffer_width/(float)window_width:1);
        (void)framebuffer_height; (void)window_height;
        fonts_update(raster_scale);
        float width=host_get_screen_width()/scale,height=host_get_screen_height()/scale;
        float sidebar=ui.browser_hidden?24:ui.browser_width;
        float workspace_top=ui.studio_style?60:42;
        float picker_top=ui.studio_style?fmaxf(workspace_top+160,height-340):height-24;
        ui.picker_bounds=(Rect){4,picker_top-57,sidebar-8,height-24-picker_top+57};
        Rect desktop={sidebar+6,workspace_top,width-sidebar-10,height-workspace_top-24};
        if(!initialized) {
            windows_init(&ui.windows,width,height);
            if(ui.studio_style) {
                studio_workspace_init(&ui.windows,width,height,sidebar);
                if(!smoke)workspace_load();
                /* Initial view includes the default C4 entry pitch even on a
                   short screen. Subsequent resizing never changes row zoom. */
                if(piano_visible_rows()<13)ui.piano_top=fmaxf(piano_min_top(),60+piano_visible_rows()*.5f);
            }
            if(smoke_scene && !ui.studio_style)windows_focus(&ui.windows,smoke_scene>=9 && smoke_scene<=11?3:4);
            if(smoke_scene==9 || smoke_scene==11 || smoke_scene==15 || smoke_scene==16 || smoke_scene==17 || smoke_scene==18 || smoke_scene==19 || smoke_scene==20 || smoke_scene==21 || smoke_scene==22 || smoke_scene==23 || smoke_scene==24){if(ui.windows.workspace.enabled)mixer_effect_open(ui.eq_bus,ui.eq_slot,0);else windows_focus(&ui.windows,5);}
            if(smoke_scene==24){int id=mixer_effect_window(ui.eq_bus,ui.eq_slot);if(id>=5)ui.dynamics_view[id]=1;}
            if(profile_frames && !ui.studio_style){ui.windows.editors[0].visible=0;windows_focus(&ui.windows,2);windows_focus(&ui.windows,3);windows_focus(&ui.windows,4);}
            if(smoke_popup==11)project_file_action(4);
            else if(smoke_popup==2)begin_rename();
            else if(smoke_popup==5)begin_number(&ui.project.bpm,30,300,120,"Tempo (BPM)");
            else if(smoke_popup){
                if(smoke_popup==3){snprintf(ui.folder_text,sizeof ui.folder_text,"%s",ui.browser.path);ui.rename_select_all=1;}
                if(smoke_popup==13)midi_refresh();
                open_popup(smoke_popup);
            }
            if(smoke_piano_maximized) {
                Editor *piano=&ui.windows.editors[2];piano->restore=piano->rect;
                piano->rect=desktop;piano->maximized=1;windows_focus(&ui.windows,2);
                if(ui.windows.workspace.enabled)studio_workspace_maximize(&ui.windows,2);
            }
            if(smoke_grid>=0)ui.piano_grid=ui.playlist_grid=smoke_grid;
            if(smoke_rack_zoom) {ui.rack_step_width[ui.pattern]=16*smoke_rack_zoom;windows_focus(&ui.windows,0);}
            if(smoke_grid_menu)open_context(24,smoke_grid_menu,(DrawPoint){480,180});
            if(smoke_workspace==1){ui.windows.workspace.mixer=.32f;}
            if(smoke_workspace==2){windows_focus(&ui.windows,0);}
            if(smoke_workspace==3){ui.mixer_selected=1;mixer_effect_open(1,0,0);}
            if(smoke_workspace==4){studio_workspace_float(&ui.windows,4);windows_focus(&ui.windows,4);}
            if(smoke_workspace==5 || smoke_workspace==6){ui.windows.workspace.editor_right=1;windows_focus(&ui.windows,smoke_workspace==5?0:2);}
            if(smoke_workspace==7){ui.mixer_selected=1;mixer_effect_add(1,0,EFFECT_CHORUS);mixer_effect_add(1,1,EFFECT_CHORUS);mixer_effect_add(1,2,EFFECT_EQ);}
            if(smoke_workspace==8){ui.pattern=0;ui.channel=ui.piano_channel=0;ui.pattern_hits[0]=1;windows_focus(&ui.windows,2);}
            if(smoke_workspace==9){windows_focus(&ui.windows,2);windows_focus(&ui.windows,4);}
            if(smoke_count){ui.record_mask=RECORD_NOTES;ui.typing_keys=1;ui.record_count_enabled=1;ui.record_count_steps=smoke_count;recording_start();}
            if(smoke_fill){ui.rack_fill_interval=3;begin_rack_fill(ui.channel);}
            initialized=1;
        }
        if(ui.studio_style) {
            ui.windows.workspace.mixer_fx_rows=ui.mixer_fx_hidden?-1:desktop_mixer_fx_rows();
            studio_workspace_layout(&ui.windows,width,height,sidebar);
            workspace_view_scale(&studio_arrangement_width,&studio_piano_width);
        }
        ui.mouse=host_get_mouse_position(); ui.mouse.x/=scale; ui.mouse.y/=scale; ui.reset=0; ui.popup_opened=0; ui.context_opened=0;
        host_set_mouse_cursor(HOST_MOUSE_CURSOR_DEFAULT);
        if(smoke && frames==0){ui.velocity_lane.height=smoke_velocity;ui.velocity_lane.collapsed=smoke_velocity<0;ui.position_seconds=smoke_seconds;ui.mixer_offset=smoke_mixer_offset;for(int bus=0;bus<=INSERTS;bus++)ui.mixer_fx_scroll[bus]=smoke_fx_scroll;ui.mixer_fx_hidden=smoke_fx_hidden;if(smoke_inserts)ui.project.insert_count=smoke_inserts;if(smoke_fx_count){for(int slot=0;slot<smoke_fx_count;slot++)ui.project.effect_type[1][slot]=slot%2?EFFECT_EQ:EFFECT_CHORUS;ui.mixer_selected=1;}if(smoke_routing){insert_send_add(&ui.project,1,2);insert_send_add(&ui.project,1,3);insert_send_add(&ui.project,2,3);ui.mixer_selected=1;}}
        if(ui.studio_style)ui.piano_top=fmaxf(piano_min_top(),fminf(127,ui.piano_top));
        desktop_velocity_resize();
        navigate_editors(scale);
        if(host_is_mouse_button_pressed(HOST_MOUSE_BUTTON_LEFT) || host_is_mouse_button_pressed(HOST_MOUSE_BUTTON_RIGHT)) arrangement_release(&ui.arrangement);
        if(ui.arrangement.gesture) {
            Rect r=desktop_editor_rect(1);
            arrangement_drag(&ui.arrangement,&ui.project,ui.arrangement.view_start+(ui.mouse.x-r.x-playlist_grid_x())/((r.w-playlist_grid_x()-24)/BARS*ui.arrangement.zoom),track_at(ui.track_scroll+ui.mouse.y-r.y-playlist_grid_top()));
            int held=ui.arrangement.gesture==ERASE_CLIPS?(host_is_mouse_button_down(HOST_MOUSE_BUTTON_RIGHT) || host_is_mouse_button_down(HOST_MOUSE_BUTTON_LEFT)):host_is_mouse_button_down(HOST_MOUSE_BUTTON_LEFT);
            if(!held) arrangement_release(&ui.arrangement);
        }
        if(ui.dynamics_drag && (!host_is_mouse_button_down(HOST_MOUSE_BUTTON_LEFT) || !host_is_window_focused() || !ui.windows.editors[ui.dynamics_drag_window].visible))ui.dynamics_drag=0;
        if(ui.fm_graph_drag>=0) {
            if(!host_is_mouse_button_down(HOST_MOUSE_BUTTON_LEFT) || host_is_mouse_button_pressed(HOST_MOUSE_BUTTON_LEFT) || ui.pattern_popup || ui.context_kind || !host_is_window_focused() ||
               ui.instrument_channel!=ui.fm_graph_channel || ui.project.instrument[ui.fm_graph_channel]!=INSTRUMENT_FM || !ui.windows.editors[4].visible) ui.fm_graph_drag=-1;
            else fm_graph_update();
        }
        if(ui.row_zoom_drag>=0) {
            if(!host_is_mouse_button_down(HOST_MOUSE_BUTTON_LEFT) || host_is_mouse_button_pressed(HOST_MOUSE_BUTTON_LEFT) || ui.pattern_popup || ui.context_kind || !host_is_window_focused()) ui.row_zoom_drag=-1;
            else { row_zoom_update(ui.mouse.y); host_set_mouse_cursor(HOST_MOUSE_CURSOR_RESIZE_NS); }
        }
        if(ui.playlist_pan) {
            ui.arrangement.view_start=fmaxf(0,ui.playlist_pan_start+(ui.mouse.x-ui.playlist_pan_x)*ui.playlist_pan_range);
            if(!host_is_mouse_button_down(HOST_MOUSE_BUTTON_LEFT) || host_is_mouse_button_pressed(HOST_MOUSE_BUTTON_LEFT)) ui.playlist_pan=0;
        }
        if(ui.playlist_vpan) {
            Rect r=desktop_editor_rect(1); float area=r.h-playlist_grid_top()-PLAYLIST_BOTTOM+14;
            float total=track_position(LANES),travel=area-timeline_thumb(area,area-14,total),maximum=fmaxf(0,total-(area-14));
            ui.track_scroll=fmaxf(0,fminf(maximum,ui.playlist_vpan_start+(ui.mouse.y-ui.playlist_vpan_y)/fmaxf(1,travel)*maximum));
            if(!host_is_mouse_button_down(HOST_MOUSE_BUTTON_LEFT) || host_is_mouse_button_pressed(HOST_MOUSE_BUTTON_LEFT)) ui.playlist_vpan=0;
        }
        if(ui.track_resize>=0) {
            ui.track_heights[ui.track_resize]=roundf(fmaxf(32,fminf(320,ui.track_resize_height+ui.mouse.y-ui.track_resize_y)))/ui.track_zoom;
            host_set_mouse_cursor(HOST_MOUSE_CURSOR_RESIZE_NS);
            if(!host_is_mouse_button_down(HOST_MOUSE_BUTTON_LEFT)) ui.track_resize=-1;
        }
        if(ui.fx_press)desktop_fx_drag_update();
        if(ui.mixer_pan && ui.studio_style && ui.windows.workspace.enabled)desktop_mixer_scrollbar_update();
        else if(ui.mixer_pan) {
            Rect r=ui.windows.editors[3].rect; float area=r.w-MIXER_PANEL-MIXER_LEFT-8; int visible=fmaxf(1,area/mixer_strip_width()-1);
            ui.mixer_scroll=fmaxf(0,fminf(INSERTS-visible,(ui.mouse.x-r.x-MIXER_LEFT-area*visible/INSERTS/2)/area*INSERTS));
            if(!host_is_mouse_button_down(HOST_MOUSE_BUTTON_LEFT) || host_is_mouse_button_pressed(HOST_MOUSE_BUTTON_LEFT)) ui.mixer_pan=0;
        }
        if(ui.cable_drag>0) {
            if(host_is_mouse_button_pressed(HOST_MOUSE_BUTTON_LEFT)) ui.cable_drag=-1;
            else if(!host_is_mouse_button_down(HOST_MOUSE_BUTTON_LEFT)) {
                int destination=mixer_input_at(ui.mouse);
                if(destination>=0) snprintf(ui.status,sizeof ui.status,(ui.studio_style?insert_send_add(&ui.project,ui.cable_drag,destination):insert_connect(&ui.project,ui.cable_drag,destination))?"Mixer cable connected.":"Connection blocked: it would create a feedback loop.");
                ui.cable_drag=-1;
            }
        }
        if(ui.rack_paint>=0) {
            if(host_is_mouse_button_pressed(HOST_MOUSE_BUTTON_LEFT) || host_is_mouse_button_pressed(HOST_MOUSE_BUTTON_RIGHT)) ui.rack_paint=-1;
            else {
                Rect r=ui.windows.editors[0].rect; paint_rack(r.w,ui.mouse.x-r.x,ui.mouse.y-r.y);
                if(!host_is_mouse_button_down(ui.rack_paint)) ui.rack_paint=-1;
            }
        }
        int modal=ui.pattern_popup!=0 || ui.context_kind!=0,dragging=ui.sample_drag[0]!=0;
        if(ui.pattern_hit_active) {
            if(modal || !host_is_window_focused() || !ui.windows.editors[2].visible || !host_is_mouse_button_down(ui.pattern_hit_erase?HOST_MOUSE_BUTTON_RIGHT:HOST_MOUSE_BUTTON_LEFT))ui.pattern_hit_active=0;
            else {Rect r=desktop_piano_rect();float cw=(r.w-84)/ui.piano_span[ui.pattern];
                if(ui.mouse.x>=r.x+60 && ui.mouse.x<r.x+r.w-24 && ui.mouse.y>=r.y+PIANO_GRID_TOP && ui.mouse.y<r.y+r.h-94)pattern_hit_update(fmaxf(0,ui.piano_pan[ui.pattern]+(ui.mouse.x-r.x-60)/cw));
            }
        }
        if(!host_is_mouse_button_down(HOST_MOUSE_BUTTON_LEFT) || host_is_mouse_button_pressed(HOST_MOUSE_BUTTON_LEFT) || modal) { ui.route_drag=-1; ui.control_drag=NULL; ui.note_drag=NULL; }
        ShortcutContext shortcut_context={.modal=modal,.captured=captured(),.dragging=dragging,
            .browser_focus=ui.browser_focus,.editor=ui.windows.focused,.piano_tool=ui.piano_tool};
#ifdef __APPLE__
        shortcut_context.mac=1;
#endif
        ShortcutActions shortcuts=shortcut_actions(host_input(),shortcut_context);
        for(int index=0;index<shortcuts.count;index++) {
            ShortcutAction action=shortcuts.actions[index];
            switch(action.kind) {
            case SHORTCUT_UNDO:undo_redo(-1);break;
            case SHORTCUT_REDO:undo_redo(1);break;
            case SHORTCUT_SAVE:project_file_action(2);break;
            case SHORTCUT_SAVE_AS:project_file_action(3);break;
            case SHORTCUT_OPEN:replace_project(3);break;
            case SHORTCUT_SELECTION:edit_selection(action.argument);break;
            case SHORTCUT_OCTAVE:piano_octave(action.argument);break;
            case SHORTCUT_NUDGE:
                if(action.argument==0)piano_shift(-1,0,"left one step");
                else if(action.argument==1)piano_shift(1,0,"right one step");
                else if(action.argument==2)piano_shift(0,1,"up one semitone");
                else piano_shift(0,-1,"down one semitone");break;
            case SHORTCUT_TRANSPORT:transport_toggle();break;
            }
        }
        if(ui.rack_hdrag) {
            ui.rack_view[ui.pattern]=fmaxf(0,ui.rack_hstart+(ui.mouse.x-ui.rack_hx)*ui.rack_hscale);
            if(!host_is_mouse_button_down(HOST_MOUSE_BUTTON_LEFT) || host_is_mouse_button_pressed(HOST_MOUSE_BUTTON_LEFT) || modal) ui.rack_hdrag=0;
        }
        if(ui.rack_vdrag) {
            ui.rack_scroll=fmaxf(0,ui.rack_vstart+(ui.mouse.y-ui.rack_vy)*ui.rack_vscale);
            if(!host_is_mouse_button_down(HOST_MOUSE_BUTTON_LEFT) || host_is_mouse_button_pressed(HOST_MOUSE_BUTTON_LEFT) || modal) ui.rack_vdrag=0;
        }
        if(ui.piano_key_drag) {
            Rect r=desktop_piano_rect();
            desktop_piano_key_update(ui.mouse.x-r.x,ui.mouse.y-r.y,modal || !host_is_mouse_button_down(HOST_MOUSE_BUTTON_LEFT) || host_is_mouse_button_pressed(HOST_MOUSE_BUTTON_LEFT) || !host_is_window_focused() || !ui.windows.editors[2].visible);
        }
        if(ui.marker_drag>=0) {
            int id=ui.marker_drag; Rect r=id==2?desktop_piano_rect():desktop_editor_rect(id);
            float gx=id==0?200:id==1?playlist_grid_x():60,width=r.w-(id==0?226:id==1?playlist_grid_x()+24:84);
            float span=id==0?width/desktop_rack_step_width():id==1?BARS/ui.arrangement.zoom*STEPS:ui.piano_span[ui.pattern];
            float start=id==0?ui.rack_view[ui.pattern]:id==1?ui.arrangement.view_start*STEPS:ui.piano_pan[ui.pattern];
            float q=id==0?1:grid_choice_interval(id==1?ui.playlist_grid:ui.piano_grid,width/span);
            int button=ui.ruler_loop_drag?HOST_MOUSE_BUTTON_RIGHT:HOST_MOUSE_BUTTON_LEFT;
            float at=fmaxf(0,snap_round(start+(ui.mouse.x-r.x-gx)/width*span,q));
            desktop_ruler_drag(at,modal,host_is_mouse_button_pressed(button),host_is_mouse_button_down(button));
        }
        if(!modal && !dragging && !captured() && host_is_mouse_button_pressed(HOST_MOUSE_BUTTON_LEFT) && ui.mouse.x>=sidebar && ui.mouse.x<sidebar+6 && ui.mouse.y>=workspace_top && ui.mouse.y<height-22) ui.browser_resize=1;
        if(ui.browser_resize) {
            ui.browser_hidden=ui.mouse.x<120;
            if(!ui.browser_hidden) ui.browser_width=fminf(fminf(600,width-460),ui.mouse.x);
            sidebar=ui.browser_hidden?24:ui.browser_width;
            desktop=(Rect){sidebar+6,workspace_top,width-sidebar-10,height-workspace_top-24};
            if(!host_is_mouse_button_down(HOST_MOUSE_BUTTON_LEFT)) ui.browser_resize=0;
        }
        if(ui.studio_style){ui.windows.workspace.mixer_fx_rows=ui.mixer_fx_hidden?-1:desktop_mixer_fx_rows();studio_workspace_layout(&ui.windows,width,height,sidebar);}
        if(sidebar!=ui.last_sidebar) {
            Editor *list=&ui.windows.editors[1];
            if(!ui.windows.workspace.enabled && fabsf(list->rect.x-ui.last_sidebar-6)<1 && fabsf(list->rect.w-(width-ui.last_sidebar-10))<1) list->rect=desktop;
            ui.last_sidebar=sidebar;
        }
        if(dragging) {
            if(fabsf(ui.mouse.x-ui.sample_origin.x)+fabsf(ui.mouse.y-ui.sample_origin.y)>5) ui.sample_moved=1;
            if(host_is_mouse_button_pressed(HOST_MOUSE_BUTTON_LEFT)) ui.sample_drag[0]=0;
            else if(host_is_mouse_button_released(HOST_MOUSE_BUTTON_LEFT)) {
                if(ui.sample_moved && !modal) drop_sample(ui.sample_drag);
                ui.sample_drag[0]=0;
            } else if(!host_is_mouse_button_down(HOST_MOUSE_BUTTON_LEFT) || host_is_key_pressed(HOST_KEY_ESCAPE)) ui.sample_drag[0]=0;
        }
        dragging=ui.sample_drag[0]!=0;
        if(host_is_mouse_button_pressed(HOST_MOUSE_BUTTON_LEFT)) ui.windows.grab=-1;
        if(!modal && host_is_mouse_button_pressed(HOST_MOUSE_BUTTON_LEFT)) ui.browser_focus=ui.mouse.x<sidebar && ui.mouse.y>=workspace_top && ui.mouse.y<height-22 && (!ui.studio_style || ui.mouse.y<picker_top);
        if(ui.control_drag)desktop_control_update(ui.control_horizontal?-host_get_mouse_delta().x:host_get_mouse_delta().y,scale,ui.mouse.y);
        if(ui.piano_scroll_drag) {
            ui.piano_pan[ui.pattern]=fmaxf(0,ui.piano_scroll_start+(ui.mouse.x-ui.piano_scroll_x)*ui.piano_scroll_range);
            if(!host_is_mouse_button_down(HOST_MOUSE_BUTTON_LEFT) || host_is_mouse_button_pressed(HOST_MOUSE_BUTTON_LEFT)) ui.piano_scroll_drag=0;
        }
        if(ui.piano_vdrag) {
            float area=desktop_piano_grid_rect().h-PIANO_GRID_PADDING+14;
            ui.piano_top=127-(int)fmaxf(0,fminf(127-piano_min_top(),ui.piano_vscroll_start+(ui.mouse.y-ui.piano_vscroll_y)/area*128));
            if(!host_is_mouse_button_down(HOST_MOUSE_BUTTON_LEFT) || host_is_mouse_button_pressed(HOST_MOUSE_BUTTON_LEFT)) ui.piano_vdrag=0;
        }
        if(ui.piano_gesture) {
            int right=ui.piano_gesture==PIANO_ERASE;
            if(modal || host_is_mouse_button_pressed(HOST_MOUSE_BUTTON_LEFT) || host_is_mouse_button_pressed(HOST_MOUSE_BUTTON_RIGHT)) ui.piano_gesture=PIANO_IDLE;
            else {
                Rect r=desktop_piano_rect(); float cw=(r.w-84)/ui.piano_span[ui.pattern],rh=(desktop_piano_grid_rect().h-PIANO_GRID_PADDING)/piano_visible_rows();
                desktop_piano_gesture_update(ui.piano_pan[ui.pattern]+(ui.mouse.x-r.x-60)/cw,ui.piano_top-(ui.mouse.y-r.y-PIANO_GRID_TOP)/rh,grid_choice_interval(ui.piano_grid,cw));
                if(!host_is_mouse_button_down(right?HOST_MOUSE_BUTTON_RIGHT:HOST_MOUSE_BUTTON_LEFT)) ui.piano_gesture=PIANO_IDLE;
            }
        }
        if(ui.note_drag) {
            Rect r=desktop_piano_rect(); float cw=(r.w-84)/ui.piano_span[ui.pattern],rh=(desktop_piano_grid_rect().h-PIANO_GRID_PADDING)/piano_visible_rows();
            desktop_piano_note_drag_update(ui.piano_pan[ui.pattern]+(ui.mouse.x-r.x-60)/cw,(ui.mouse.y-r.y-PIANO_GRID_TOP)/rh,grid_choice_interval(ui.piano_grid,cw));
        }
        if(ui.velocity_drag>=0) {
            int button=ui.velocity_drag?HOST_MOUSE_BUTTON_RIGHT:HOST_MOUSE_BUTTON_LEFT;
            if(modal || ui.pattern!=ui.velocity_pattern || ui.piano_channel!=ui.velocity_channel ||
               host_is_mouse_button_pressed(HOST_MOUSE_BUTTON_LEFT) || host_is_mouse_button_pressed(HOST_MOUSE_BUTTON_RIGHT)) ui.velocity_drag=-1;
            else {
                Rect r=desktop_piano_rect(); float cw=(r.w-84)/ui.piano_span[ui.pattern];
                DrawPoint point={ui.piano_pan[ui.pattern]+(ui.mouse.x-r.x-60)/cw,
                    fmaxf(1,fminf(127,(r.y+r.h-22-ui.mouse.y)/velocity_lane_scale(desktop_velocity_height())))};
                DrawPoint from=ui.velocity_drag?ui.velocity_from:ui.velocity_now;
                notes_velocity(ui.project.notes[ui.pattern][ui.piano_channel],ui.velocity_drag?ui.velocity_before:NULL,
                    from.x,point.x,from.y,point.y,3/cw);
                ui.velocity_now=point;
                if(!host_is_mouse_button_down(button)) ui.velocity_drag=-1;
            }
        }
        if(ui.route_drag>=0) {
            ui.project.route[ui.route_drag]=(int)fmaxf(0,fminf(ui.project.insert_count,ui.route_start+(int)((ui.route_y-ui.mouse.y)/3)));
            ui.mixer_selected=ui.project.route[ui.route_drag];
            if(ui.studio_style && ui.windows.workspace.enabled){
                Rect r=desktop_editor_rect(3);MixerView v=desktop_mixer_view(r.w,r.h);
                ui.mixer_offset=mixer_reveal_bus(v,ui.mixer_selected);
            } else {
            int visible=fmaxf(1,(ui.windows.editors[3].rect.w-MIXER_PANEL-MIXER_LEFT-8)/mixer_strip_width()-1);
            if(ui.mixer_selected>ui.mixer_scroll+visible) ui.mixer_scroll=ui.mixer_selected-visible;
            else if(ui.mixer_selected>0 && ui.mixer_selected<=ui.mixer_scroll) ui.mixer_scroll=ui.mixer_selected-1;
            }
            snprintf(ui.status,sizeof ui.status,"%s -> %s",ui.project.channel_names[ui.route_drag],mixer_name(ui.mixer_selected));
        }
        Rect filter_rect=ui.windows.editors[0].rect;
        if(!modal && !captured() && !dragging && windows_hit(&ui.windows,ui.mouse.x,ui.mouse.y)==0 && ui.mouse.x>=filter_rect.x+124 && ui.mouse.x<filter_rect.x+180 && ui.mouse.y>=filter_rect.y+2 && ui.mouse.y<filter_rect.y+TITLE-2 && host_is_mouse_button_pressed(HOST_MOUSE_BUTTON_LEFT)) {
            windows_focus(&ui.windows,0); open_context(6,0,(DrawPoint){filter_rect.x+124,filter_rect.y+TITLE}); modal=1;
        }
        if(!modal && (!captured() || (ui.windows.workspace.enabled && ui.windows.workspace.drag>=0)) && !dragging) windows_update(&ui.windows,desktop,ui.mouse.x,ui.mouse.y,host_is_mouse_button_pressed(HOST_MOUSE_BUTTON_LEFT),host_is_mouse_button_down(HOST_MOUSE_BUTTON_LEFT),host_get_time());
        else { if(ui.browser_resize) windows_update(&ui.windows,desktop,ui.mouse.x,ui.mouse.y,0,0,host_get_time()); ui.windows.owner=-1; ui.windows.grab=-1; }
        if(ui.windows.workspace.enabled) {
            workspace_view_scale(&studio_arrangement_width,&studio_piano_width);
            int split=ui.windows.workspace.drag>=0?ui.windows.workspace.drag:studio_workspace_split(&ui.windows,ui.mouse.x,ui.mouse.y);
            if(split>=0){host_set_mouse_cursor((split==4 || split==6)?HOST_MOUSE_CURSOR_RESIZE_EW:HOST_MOUSE_CURSOR_RESIZE_NS);snprintf(ui.status,sizeof ui.status,"Drag divider to resize panels; double-click restores the split");}
            if(modal || !host_is_window_focused()){ui.windows.workspace.drag=-1;}
        }
        if(ui.studio_style)ui.piano_top=fmaxf(piano_min_top(),fminf(127,ui.piano_top));
        if(!modal && !captured() && !dragging && desktop_picker_hit(ui.mouse.x,ui.mouse.y) &&
            (host_is_mouse_button_pressed(HOST_MOUSE_BUTTON_LEFT) || host_is_mouse_button_pressed(HOST_MOUSE_BUTTON_RIGHT))) {
            if(ui.windows.editors[1].visible)windows_focus(&ui.windows,1);ui.browser_focus=0;
        }
        int focused=windows_count(&ui.windows)-1;
        while(focused>0 && !ui.windows.editors[ui.windows.order[focused]].visible) focused--;
        int keyboard_editor=ui.windows.workspace.enabled?ui.windows.focused:ui.windows.order[focused];
        if(!modal && !dragging && !captured() && !ui.browser_focus && !ui.midi_take.active && keyboard_editor==1 && ui.windows.editors[1].visible && delete_pressed()) {
            for(int l=0;l<LANES;l++) for(int b=0;b<CLIPS;b++) if(ui.arrangement.selected[l][b]) { ui.project.clips[l][b]=0; ui.project.clip_steps[l][b]=0; }
            memset(ui.arrangement.selected,0,sizeof ui.arrangement.selected);
        }
        if(!modal && !dragging && !captured() && !ui.browser_focus && !ui.midi_take.active && keyboard_editor==2 && ui.windows.editors[2].visible && delete_pressed()) {
            for(int i=0;i<NOTES;i++) if(ui.note_selected[ui.pattern][ui.piano_channel][i]) ui.project.notes[ui.pattern][ui.piano_channel][i].velocity=0;
            memset(ui.note_selected[ui.pattern][ui.piano_channel],0,NOTES);
        }
        if(!modal && host_is_file_dropped()) {
            WindowDrops dropped=host_load_dropped_files();
            for(int i=0;i<dropped.count;i++) {
                const char *path=dropped.paths[i];
                if(host_is_file_extension(path,PROJECT_FILE_SUFFIX)) request_load_project(path);
                else drop_sample(path);
            }
            host_unload_dropped_files(dropped);
        }
        if(profile_frames && profile_motion) {
            ui.arrangement.view_start=(frames%160)*.03125f;
            ui.arrangement.zoom=1+(frames%80)*.025f;
            ui.piano_pan[ui.pattern]=(frames%160)*.125f;
            ui.piano_span[ui.pattern]=16+(frames%80)*.5f;
        }
        meters_update();
        spectrum_update();
        track_activity_update();
        if(ui.windows.editors[1].visible) previews_update(raster_scale);
        ui.knob_context=-1; ui.browser_progress=-1;
        double render_start=profile_frames?host_get_time():0;
        clock_t render_cpu_start=profile_frames?clock():0;
        host_begin_drawing(); host_clear_background(bg); host_begin_mode2_d((HostCamera){.zoom=scale});
        ui.input_enabled=!modal && !dragging && !captured() && ui.windows.grab<0 && ui.mouse.y<workspace_top;
        if(ui.studio_style && !ui.profile_flat)ui_panel(host_draw(),(DrawRect){0,0,width,workspace_top-2},panel);
        else ui_surface(host_draw(),(DrawRect){0,0,width,workspace_top-2},panel);
        Theme toolbar_theme=ui_theme;
        StudioToolbar toolbar=studio_toolbar_layout(width);
        if(ui.studio_style) {
            if(!ui.profile_flat)ui_bevel(host_draw(),(DrawRect){1,1,width-2,57},0);
        }
        const int compact_toolbar=ui.studio_style && toolbar.compact;
        const int menu_y=ui.studio_style?toolbar.menus[0].y:8,menu_h=ui.studio_style?toolbar.menus[0].h:22;
        const int ty=ui.studio_style?toolbar.stop.y:8,th=ui.studio_style?toolbar.stop.h:22,tw=ui.studio_style?toolbar.stop.w:24;
        const int cy=ty+th/2;
        if(!ui.studio_style)ui_surface(host_draw(),(DrawRect){8,menu_y,176,menu_h},cell);
        if(button("FILE",ui.studio_style?toolbar.menus[0].x:8,ui.studio_style?toolbar.menus[0].y:menu_y,ui.studio_style?toolbar.menus[0].w:44,menu_h,ui.pattern_popup==1)) open_popup(1);
        if(hover(ui.studio_style?toolbar.menus[0].x:8,ui.studio_style?toolbar.menus[0].y:menu_y,ui.studio_style?toolbar.menus[0].w:44,menu_h)) snprintf(ui.status,sizeof ui.status,"File: New, Demo, Save, Open, Export or Collect samples");
        if(button("EDIT",ui.studio_style?toolbar.menus[1].x:52,ui.studio_style?toolbar.menus[1].y:menu_y,ui.studio_style?toolbar.menus[1].w:44,menu_h,ui.pattern_popup==10)) open_popup(10);
        if(hover(ui.studio_style?toolbar.menus[1].x:52,ui.studio_style?toolbar.menus[1].y:menu_y,ui.studio_style?toolbar.menus[1].w:44,menu_h)) snprintf(ui.status,sizeof ui.status,"Edit: Undo, Redo, Cut, Copy, Paste and Select all");
        if(button("VIEW",ui.studio_style?toolbar.menus[2].x:96,ui.studio_style?toolbar.menus[2].y:menu_y,ui.studio_style?toolbar.menus[2].w:44,menu_h,ui.pattern_popup==7)) open_popup(7);
        if(hover(ui.studio_style?toolbar.menus[2].x:96,ui.studio_style?toolbar.menus[2].y:menu_y,ui.studio_style?toolbar.menus[2].w:44,menu_h)) snprintf(ui.status,sizeof ui.status,"View: MIDI recording settings");
        if(button("HELP",ui.studio_style?toolbar.menus[3].x:140,ui.studio_style?toolbar.menus[3].y:menu_y,ui.studio_style?toolbar.menus[3].w:44,menu_h,ui.pattern_popup==9)) open_popup(9);
        label(fit_text(host_text_format("%s%s",host_get_file_name(ui.document.path),project_dirty()?" *":""),ui.studio_style?(compact_toolbar?width-836:176):176,ui.studio_style?9:11),ui.studio_style?(compact_toolbar?828:8):8,ui.studio_style?(compact_toolbar?20:0):29,ui.studio_style?9:11,muted);
        const int gap=4,pitch_x=ui.studio_style?toolbar.pitch.x+11:184+gap+KNOB_RADIUS;
        const int volume_x=ui.studio_style?toolbar.volume.x+11:pitch_x+KNOB_RADIUS*2+gap;
        const int mode_x=ui.studio_style?toolbar.mode.x:volume_x+KNOB_RADIUS+gap,mode_w=ui.studio_style?toolbar.mode.w:52;
        const int play_x=ui.studio_style?toolbar.play.x:mode_x+52+gap;
        const int stop_x=ui.studio_style?toolbar.stop.x:play_x+24+gap;
        const int record_x=ui.studio_style?toolbar.record.x:stop_x+24+gap;
        const int metro_x=ui.studio_style?toolbar.metronome.x:record_x+24+3*22+gap;
        const int tempo_x=ui.studio_style?toolbar.tempo.x:metro_x+22+gap;
        const int tempo_w=ui.studio_style?toolbar.tempo.w:56;
        const int tempo_y=ui.studio_style?toolbar.tempo.y:ty,tempo_h=ui.studio_style?toolbar.tempo.h:th;
        const int position_x=ui.studio_style?toolbar.position.x:tempo_x+56+12;
        const int position_w=ui.studio_style?toolbar.position.w:88;
        const int position_y=ui.studio_style?toolbar.position.y:ty,position_h=ui.studio_style?toolbar.position.h:th;
        const int audio_x=ui.studio_style?toolbar.spectrum.x:position_x+88+12;
        const int analyzer_width=ui.studio_style?toolbar.spectrum.w:(int)fminf(108,fmaxf(0,width-audio_x-208));
        const int editors_x=ui.studio_style?toolbar.editors[0].x:audio_x+analyzer_width+gap+40+12;
        const int editor_pitch=ui.studio_style?toolbar.editors[1].x-toolbar.editors[0].x:24;
        const int follow_x=ui.studio_style?toolbar.follow.x:editors_x+4*editor_pitch;
        const int keys_x=ui.studio_style?toolbar.typing.x:follow_x+editor_pitch;
        const int small_y=ui.studio_style?toolbar.follow.y:ty,small_w=ui.studio_style?toolbar.follow.w:22,small_h=ui.studio_style?toolbar.follow.h:th;
        label("Pitch",pitch_x-text_width("Pitch",8)/2,ui.studio_style?40:30,8,muted);
        label("Vol",volume_x-text_width("Vol",8)/2,ui.studio_style?40:30,8,muted);
        knob_style(pitch_x,cy,&ui.project.master_pitch,-12,12,0,"Master pitch (semitones)",KNOB_CENTER);
        if(hover(pitch_x-11,cy-11,22,22) || ui.control_drag==&ui.project.master_pitch) snprintf(ui.status,sizeof ui.status,"Master pitch: %+.2f semitones (sample speed; right-click to enter)",ui.project.master_pitch);
        knob_style(volume_x,cy,&ui.output_volume,0,VOLUME_KNOB_MAX,1,"LibreLoop output volume",KNOB_VOLUME);
        if(hover(volume_x-11,cy-11,22,22) || ui.control_drag==&ui.output_volume) snprintf(ui.status,sizeof ui.status,"LibreLoop output volume: listening level; Master and WAV export unchanged");
        if(button("",mode_x,ty,mode_w,th,0) && !recording_active()) {
            if(ui.windows.workspace.enabled)desktop_playback_page(!ui.song);
            else {ui.song=!ui.song;ui.reset=1;}
        }
        host_draw_rectangle(mode_x,ty,mode_w,th/2,ui.song?cell:ui.studio_style?ui_theme.meter_mid:ui_theme.patt);
        host_draw_rectangle(mode_x,ty+th/2,mode_w,th/2,ui.song?accent:cell);
        if(ui.studio_style) {
            ui_bevel(host_draw(),(DrawRect){mode_x,ty,mode_w,th/2},!ui.song);
            ui_bevel(host_draw(),(DrawRect){mode_x,ty+th/2,mode_w,th/2},ui.song);
            host_draw_rectangle(mode_x+4,ty+(ui.song?th/2:0)+5,3,th/2-10,ui.song?ui_theme.meter_low:ui_theme.meter_mid);
        }
        label("PATT",mode_x+(mode_w-text_width("PATT",10))/2,ty+(th/2-(ui.studio_style?UI_TEXT_SIZE:10))/2,10,ui.song?muted:ui_theme.selected_text);
        label("SONG",mode_x+(mode_w-text_width("SONG",10))/2,ty+th/2+(th/2-(ui.studio_style?UI_TEXT_SIZE:10))/2,10,ui.song?ui_theme.selected_text:muted);
        if(hover(mode_x,ty,mode_w,th)) snprintf(ui.status,sizeof ui.status,ui.windows.workspace.enabled?"Click either half to switch PATT / SONG and Piano Roll / Arrangement":"Playback mode: click to switch Pattern (orange) / Song (green)");
        int transport_active=ui.playing || audio_preview_position(ui.audition)>=0 || audio_key_position(127,ui.instrument_channel)>=0;
        if(button(ui.studio_style?"":transport_active?"||":">",play_x,ty,tw,th,transport_active)) transport_toggle();
        if(ui.studio_style)icon_button_state(transport_active?ICON_PAUSE:ICON_PLAY,(DrawRect){play_x,ty,tw,th},ink,transport_active,1);
        if(hover(play_x,ty,tw,th)) snprintf(ui.status,sizeof ui.status,"Play from ruler / stop sources, preserving effect tails (Space)");
        if(button(ui.studio_style?"":"[]",stop_x,ty,tw,th,0)) transport_stop();
        if(ui.studio_style)icon_in_button(ICON_STOP,(DrawRect){stop_x,ty,tw,th},ink);
        if(hover(stop_x,ty,tw,th)) snprintf(ui.status,sizeof ui.status,"Stop all audio; press again while stopped to return the start marker to the beginning");
        if(button("",record_x,ty,tw,th,recording_active())) recording_start();
        int record_pressed=recording_active() || (hover(record_x,ty,tw,th) && host_is_mouse_button_down(HOST_MOUSE_BUTTON_LEFT));
        if(ui.studio_style)ui_lamp(host_draw(),(DrawPoint){record_x+tw/2+record_pressed,cy+record_pressed},fminf(14,tw*.34f),(DrawColor){255,33,47,255});
        else circle(record_x+tw/2,cy,7,ui_theme.meter_high);
        if(hover(record_x,ty,tw,th) && host_is_mouse_button_pressed(HOST_MOUSE_BUTTON_RIGHT)){midi_refresh();open_popup(13);}
        const int record_icons[]={ICON_WAVE,ICON_PIANO,ICON_AUTOMATION};
        const char *record_names[]={"Audio from armed mixer tracks","MIDI notes into a new pattern","MIDI / mouse control automation"};
        int record_input=ui.input_enabled;if(recording_active())ui.input_enabled=0;
        for(int i=0;i<3;i++){int xx=ui.studio_style?toolbar.record_types[i].x:record_x+24+i*22;
            int rw=ui.studio_style?toolbar.record_types[i].w:22;
            if(button_available("",xx,cy-rw/2,rw,rw,ui.record_mask&(1u<<i),!recording_active())){ui.record_mask^=1u<<i;midi_save_settings();}
            if(ui.studio_style)icon_button_state(record_icons[i],(DrawRect){xx,cy-rw/2,rw,rw},ui.record_mask&(1u<<i)?ui_theme.selected_text:muted,ui.record_mask&(1u<<i),!recording_active());
            else icon(record_icons[i],xx+rw/2,cy,17,ui.record_mask&(1u<<i)?ui_theme.selected_text:muted);
            if(hover(xx,cy-rw/2,rw,rw))snprintf(ui.status,sizeof ui.status,"Record %s: %s",record_names[i],ui.record_mask&(1u<<i)?"enabled":"disabled");
        }ui.input_enabled=record_input;
        if(hover(record_x,ty,tw,th)) snprintf(ui.status,sizeof ui.status,recording_active()?"Recording: click to finish the takes":"Record enabled types; right-click for MIDI inputs and recording settings");
        int metro_h=ui.studio_style?(th-2)/2:th;
        if(button("",metro_x,ty,ui.studio_style?tw:22,metro_h,ui.metronome)) { ui.metronome=!ui.metronome; audio_metronome(ui.metronome); }
        if(ui.studio_style)icon_button_state(ICON_METRO,(DrawRect){metro_x,ty,tw,metro_h},ui.metronome?ui_theme.selected_text:ink,ui.metronome,1);
        else icon(ICON_METRO,metro_x+11,ty+metro_h/2,20,ui.metronome?ui_theme.selected_text:ink);
        if(ui.studio_style) {
            int input=ui.input_enabled;if(recording_active())ui.input_enabled=0;
            if(button_available(host_text_format("IN %.0f",ui.record_count_steps),metro_x,ty+metro_h+2,tw,th-metro_h-2,ui.record_count_enabled,!recording_active())){ui.record_count_enabled^=1;midi_save_settings();}
            if(hover(metro_x,ty+metro_h+2,tw,th-metro_h-2)) {
                snprintf(ui.status,sizeof ui.status,"Count-in %s: %.0f sixteenth steps before recording; right-click to set steps",ui.record_count_enabled?"on":"off",ui.record_count_steps);
                if(host_is_mouse_button_pressed(HOST_MOUSE_BUTTON_RIGHT)){begin_number(&ui.record_count_steps,1,4096,16,"Count-in steps (sixteenth notes)");ui.number_integer=1;}
            }
            ui.input_enabled=input;
        }
        if(hover(metro_x,ty,ui.studio_style?tw:22,metro_h)) snprintf(ui.status,sizeof ui.status,"Metronome: %s | beat clicks during playback; first beat accented; excluded from WAV export",ui.metronome?"On":"Off");
        int tempo_clicked;
        if(ui.studio_style) {
            /* Numeric display keeps its dark face during hover/capture. */
            ui_display_well(host_draw(),(DrawRect){tempo_x,tempo_y,tempo_w,tempo_h});
            if(!ui.profile_flat)ui_bevel(host_draw(),(DrawRect){tempo_x,tempo_y,tempo_w,tempo_h},1);
            int over=hover(tempo_x,tempo_y,tempo_w,tempo_h);
            if(over || ui.control_drag==&ui.project.bpm) {
                host_set_mouse_cursor(HOST_MOUSE_CURSOR_RESIZE_NS);
                host_draw_rectangle_lines_ex((DrawRect){tempo_x,tempo_y,tempo_w,tempo_h},1,ui_theme.signal);
            }
            tempo_clicked=over && host_is_mouse_button_pressed(HOST_MOUSE_BUTTON_LEFT);
        } else tempo_clicked=drag_button(host_text_format("%.2f",ui.project.bpm),tempo_x,tempo_y,tempo_w,tempo_h,ui.control_drag==&ui.project.bpm);
        if(tempo_clicked && !recording_active())capture_control(&ui.project.bpm,30,300,0);
        if(hover(tempo_x,tempo_y,tempo_w,tempo_h) && !recording_active()) {
            ui.project.bpm=fmaxf(30,fminf(300,ui.project.bpm+host_get_mouse_wheel_move()));
            snprintf(ui.status,sizeof ui.status,"Tempo: drag up/down; right-click for Reset or Enter value");
            if(host_is_mouse_button_pressed(HOST_MOUSE_BUTTON_RIGHT)) control_menu(&ui.project.bpm,30,300,120,"Tempo (BPM)");
        }
        if(ui.studio_style)lcd_number(host_text_format("%.2f",ui.project.bpm),tempo_x+8,tempo_y+4,tempo_w-16,ui.studio_style && compact_toolbar?14:18,ui_theme.signal);
        uint64_t position_frames=audio_position();
        uint64_t step=(uint64_t)(position_frames/(RATE*60.0/ui.project.bpm/4));
        if(hover(position_x,position_y,position_w,position_h) && host_is_mouse_button_pressed(HOST_MOUSE_BUTTON_LEFT))ui.position_seconds^=1;
        if(ui.studio_style)ui_display_well(host_draw(),(DrawRect){position_x,position_y,position_w,position_h});
        else ui_surface(host_draw(),(DrawRect){position_x,position_y,position_w,position_h},bg);
        if(ui.studio_style && !ui.profile_flat)ui_bevel(host_draw(),(DrawRect){position_x,position_y,position_w,position_h},1);
        const char *position=host_text_format("%03llu:%02llu",(unsigned long long)(step/16+1),(unsigned long long)(step%16+1));
        char seconds_position[32];
        if(ui.position_seconds){if(ui.studio_style)transport_clock(seconds_position,sizeof seconds_position,position_frames,RATE);else transport_seconds(seconds_position,sizeof seconds_position,position_frames,RATE);position=seconds_position;}
        if(ui.record_count_in.active)position=host_text_format("%d",record_count_in_remaining(&ui.record_count_in,midi_input_time()));
        if(ui.studio_style && ui.position_seconds && !ui.record_count_in.active)lcd_time(position,position_x+10,position_y+6,position_w-20,ui_theme.signal);
        else if(ui.studio_style)lcd_number(position,position_x+10,position_y+6,position_w-20,compact_toolbar?14:18,ui_theme.signal);
        else label(position,position_x+10,position_y+3,18,ui_theme.signal);
        if(hover(position_x,position_y,position_w,position_h)) snprintf(ui.status,sizeof ui.status,ui.position_seconds?(ui.studio_style?"Playback position: minutes:seconds:centiseconds; click for bars and steps":"Playback position: seconds.milliseconds; click for bars and steps"):ui.studio_style?"Playback position: bar and sixteenth-note step; click for minutes:seconds:centiseconds":"Playback position: bar and sixteenth-note step");
        if(ui.studio_style) {
            label(ui.record_count_in.active?"COUNT-IN STEPS":ui.position_seconds?"MIN:SEC:CS":"BAR : STEP",position_x+8,position_y+position_h-UI_TEXT_SIZE-2,10,ui_theme.signal);
            label("TEMPO",tempo_x+12,tempo_y+tempo_h-UI_TEXT_SIZE-2,10,ui_theme.signal);
        }
        if(!ui.windows.workspace.enabled)for(int i=0;i<4;i++) { int id=i==1?2:i==2?1:i; editor_button(id,editors_x+i*editor_pitch); }
        if(button("",follow_x,small_y,small_w,small_h,ui.follow_playhead)) { ui.follow_playhead=!ui.follow_playhead; if(!ui.follow_playhead) { ui.arrangement.view_start=fmaxf(0,ui.arrangement.view_start); for(int p=0;p<PATTERNS;p++) ui.piano_pan[p]=fmaxf(0,ui.piano_pan[p]); } }
        if(ui.studio_style)icon_button_state(ICON_FOLLOW,(DrawRect){follow_x,small_y,small_w,small_h},ui.follow_playhead?ui_theme.selected_text:ink,ui.follow_playhead,1);
        else icon(ICON_FOLLOW,follow_x+small_w/2,cy,20,ui.follow_playhead?ui_theme.selected_text:ink);
        if(hover(follow_x,small_y,small_w,small_h)) snprintf(ui.status,sizeof ui.status,"Follow playhead: center playback in the Playlist (Song) or Piano Roll (PAT)");
        if(button("",keys_x,small_y,small_w,small_h,ui.typing_keys)) ui.typing_keys=!ui.typing_keys;
        if(ui.studio_style)icon_button_state(ICON_KEYS,(DrawRect){keys_x,small_y,small_w,small_h},ui.typing_keys?ui_theme.selected_text:ink,ui.typing_keys,1);
        else icon(ICON_KEYS,keys_x+small_w/2,cy,20,ui.typing_keys?ui_theme.selected_text:ink);
        if(hover(keys_x,small_y,small_w,small_h)) snprintf(ui.status,sizeof ui.status,"Typing piano: Z row C3, Q row C4; S/D/G/H/J and 2/3/5/6/7 sharps; disabled in Browser/text fields");
        if(ui.record_count_in.active && host_is_key_pressed(HOST_KEY_ESCAPE))recording_finish();
        recording_poll();midi_poll();
        ui.visual_step=ui.playing && !ui.reset?audio_visual_position()/(RATE*60.0/ui.project.bpm/4):playback_start(); automation_display_update();
        ui.picker_bounds.w=sidebar-8;
        int browser_hover=!ui.browser_hidden && ui.mouse.x<sidebar && ui.mouse.y>=workspace_top && ui.mouse.y<(ui.studio_style?picker_top:height-22);
        typing_piano(modal || dragging || browser_hover);
        if(analyzer_width>0) {
            host_draw_rectangle(audio_x,ty,analyzer_width,th,bg);
            spectrum_draw(&ui.master_spectrum,(DrawRect){audio_x+2,ty+1,analyzer_width-4,th-2},0);
            if(ui.studio_style){
                ui_bevel(host_draw(),(DrawRect){audio_x,ty,analyzer_width,th},1);

            }
            int mx=audio_x+analyzer_width+gap,meter_width=ui.studio_style?toolbar.levels.w:40,fill_width=meter_width-(ui.studio_style?28:4);
            host_draw_rectangle(mx,ty,meter_width,th,bg);
            for(int side=0;side<2;side++) {
                float level=fmaxf(0,fminf(1,(20*log10f(fmaxf(.001f,ui.meter_level[0][side]))+60)/60));
                if(ui.studio_style) {
                    int row=ty+3+side*18;
                    label(side?"R":"L",mx+3,row-3,UI_TEXT_SIZE,ui_theme.signal);
                    ui_display_well(host_draw(),(DrawRect){mx+14,row,fill_width,13});
                    ui_level_fill(host_draw(),(DrawRect){mx+14,row,fill_width,13},level,.8f,1,0);
                    float hold=fmaxf(0,fminf(1,(20*log10f(fmaxf(.001f,ui.meter_hold[0][side]))+60)/60));
                    ui_meter_marks(host_draw(),(DrawRect){mx+14,row,fill_width,13},hold,ui.meter_hold[0][side]>1,0);
                    for(int tick=0;tick<=5;tick++)host_draw_line(mx+14+tick*fill_width/5,row+14,mx+14+tick*fill_width/5,row+16,(DrawColor){90,126,100,255});
                } else host_draw_rectangle(mx+2,ty+2+side*(th/2),36*level,7,accent);
                if(!ui.studio_style && ui.meter_hold[0][side]>=1) host_draw_rectangle(mx+meter_width-4,ty+(ui.studio_style?3+side*18:2+side*(th/2)),2,ui.studio_style?8:7,ui_theme.meter_high);
            }
            if(ui.studio_style)ui_bevel(host_draw(),(DrawRect){mx,ty,meter_width,th},1);
            if(hover(audio_x,ty,analyzer_width+gap+meter_width,th)) snprintf(ui.status,sizeof ui.status,audio_ok?"Master frequency spectrum and stereo level · red indicates clipping":"Audio device unavailable");
        }
        ui_theme=toolbar_theme;
        Theme browser_theme=ui_theme;
        if(ui.studio_style){ui_theme.text=(DrawColor){227,234,232,255};ui_theme.secondary=(DrawColor){174,190,189,255};}
        ui.input_enabled=!modal && !ui.pattern_popup && !ui.context_kind && !dragging && !captured() && ui.windows.grab<0 && ui.mouse.x<sidebar+6 && ui.mouse.y>=workspace_top && (!ui.studio_style || ui.mouse.y<picker_top);
        if(ui.studio_style && !ui.profile_flat)ui_panel(host_draw(),(DrawRect){0,workspace_top,sidebar,height-workspace_top-24},ui_theme.browser);
        else host_draw_rectangle(0,workspace_top,sidebar,height-workspace_top-24,ui_theme.browser);
        if(!ui.browser_hidden) {
            if(hover(0,workspace_top,sidebar,height-workspace_top-24)) snprintf(ui.status,sizeof ui.status,"Browser: click to preview; drag to load | Up/Down or j/k: select | Left/Right or h/l: fold");
            if(ui.studio_style)ui_titlebar(host_draw(),(DrawRect){8,workspace_top+4,sidebar-16,STUDIO_TITLE},ui.browser_focus);
            label("Browser",ui.studio_style?16:10,workspace_top+(ui.studio_style?5:8),14,ui.studio_style?theme_foreground(ui_theme.title):ink);
            if(button(ui.studio_style?"+":"+ Add folder",ui.studio_style?sidebar-30:8,workspace_top+(ui.studio_style?7:34),ui.studio_style?18:sidebar-16,ui.studio_style?18:23,0)) { snprintf(ui.folder_text,sizeof ui.folder_text,"%s",ui.browser.path); ui.rename_select_all=1; open_popup(3); }
            if(ui.studio_style && hover(sidebar-30,workspace_top+7,18,18))snprintf(ui.status,sizeof ui.status,"Add a folder as a new Browser tree root");
            int fy=workspace_top+(ui.studio_style?34:66),rows=fmaxf(1,(picker_top-fy-74)/23);
            if(!modal && !ui.pattern_popup && !ui.context_kind && (ui.browser_focus || browser_hover) && !dragging && !captured() && !command_down()) {
                if(host_is_key_pressed(HOST_KEY_UP) || host_is_key_pressed(HOST_KEY_K)) audition_entry((int)fmaxf(0,ui.browser.selected-1));
                if(host_is_key_pressed(HOST_KEY_DOWN) || host_is_key_pressed(HOST_KEY_J)) audition_entry((int)fminf(ui.browser.items-1,ui.browser.selected+1));
                if(host_is_key_pressed(HOST_KEY_LEFT) || host_is_key_pressed(HOST_KEY_H)) browser_left(&ui.browser);
                if(host_is_key_pressed(HOST_KEY_RIGHT) || host_is_key_pressed(HOST_KEY_L)) browser_right(&ui.browser);
            }
            if(ui.browser.follow && ui.browser.selected>=0) {
                ui.browser.follow=0;
                if(ui.browser.selected<ui.browser.scroll) ui.browser.scroll=ui.browser.selected;
                if(ui.browser.selected>=ui.browser.scroll+rows) ui.browser.scroll=ui.browser.selected-rows+1;
            }
            if(hover(0,fy,sidebar,rows*23)) ui.browser.scroll-=(int)host_get_mouse_wheel_move()*3;
            ui.browser.scroll=(int)fmaxf(0,fminf(fmaxf(0,ui.browser.items-rows),ui.browser.scroll));
            host_begin_scissor_mode(0,fy*scale,(sidebar-4)*scale,rows*23*scale);
            for(int entry=ui.browser.scroll;entry<ui.browser.items && entry<ui.browser.scroll+rows;entry++) {
                BrowserNode *node=&ui.browser.nodes[entry]; const char *path=node->path;
                int y=fy+(entry-ui.browser.scroll)*23,indent=12+node->depth*14;
                int clicked=button_color("",8,y,sidebar-16,21,ui.browser.selected==entry,ui_theme.browser);
                DrawColor color=ui.browser.selected==entry?ui_theme.selected_text:ink;
                if(node->dir) {
                    if(node->open) host_draw_triangle((DrawPoint){indent,y+8},(DrawPoint){indent+4,y+13},(DrawPoint){indent+8,y+8},color);
                    else host_draw_triangle((DrawPoint){indent,y+6},(DrawPoint){indent,y+14},(DrawPoint){indent+6,y+10},color);
                }
                int name_x=indent+14;
                if(ui.studio_style) {
                    int ix=indent+12;
                    if(node->dir) {
                        ui_folder_icon(host_draw(),ix,y+6);
                    } else {
                        host_draw_rectangle(ix+1,y+5,10,13,(DrawColor){225,228,210,255});
                        host_draw_rectangle_lines(ix+1,y+5,10,13,(DrawColor){111,127,113,255});
                        for(int k=0;k<5;k++)host_draw_line(ix+3+k,y+11-(k%3),ix+3+k,y+11+(k%3),(DrawColor){32,108,102,255});
                    }
                    name_x=indent+30;
                }
                const char *name=host_get_file_name(path); if(!*name) name=path;
                label(fit_text(node->depth==0 && !strcmp(path,ui.browser.samples)?"LibreLoop samples":name,fmaxf(0,sidebar-name_x-10),13),name_x,y+4,13,color);
                if(hover(8,y,sidebar-16,21)) snprintf(ui.status,sizeof ui.status,"%.80s: %s | j/k: select; h/l: fold",path,node->dir?"click to expand/collapse":host_is_file_extension(path,PROJECT_FILE_SUFFIX)?"click to open project":"click to preview; drag to Rack or Playlist");
                if(clicked) {
                    browser_select(&ui.browser,entry);
                    if(node->dir) { if(!browser_toggle(&ui.browser,entry)) snprintf(ui.status,sizeof ui.status,"Folder unavailable: %.180s",path); break; }
                    if(host_is_file_extension(path,PROJECT_FILE_SUFFIX)) request_load_project(path);
                    else {
                        audition_entry(entry); snprintf(ui.sample_drag,sizeof ui.sample_drag,"%s",path);
                        ui.sample_origin=ui.mouse; ui.sample_moved=0;
                        ui.sample_offset=(DrawPoint){ui.mouse.x-8,ui.mouse.y-y}; ui.sample_drag_width=sidebar-16; ui.sample_text_offset=indent+6;
                    }
                }
            }
            host_end_scissor_mode();
            if(ui.browser.selected>=0 && ui.browser.selected<ui.browser.items && ui.audition.data && !strcmp(ui.browser.nodes[ui.browser.selected].path,ui.audition_path)) {
                draw_waveform((WaveDisplay){.sample=ui.audition,.wave=&ui.audition_wave,.origin=8,.width=sidebar-16},
                    (DrawRect){8,picker_top-74,sidebar-16,68},8,sidebar-8,ui_theme.waveform);
                if(hover(8,picker_top-74,sidebar-16,68)) {
                    snprintf(ui.status,sizeof ui.status,"Click waveform to replay %.100s | Up/Down or j/k: select; Left/Right or h/l: fold",host_get_file_name(ui.audition_path));
                    if(host_is_mouse_button_pressed(HOST_MOUSE_BUTTON_LEFT)) { ui.browser_focus=1; audio_preview(ui.audition); }
                }
                ui.browser_progress=audio_preview_position(ui.audition);
                if(ui.browser_progress>=0) {
                    float x=8+ui.browser_progress*(sidebar-16);
                    host_draw_line_ex((DrawPoint){x,picker_top-74},(DrawPoint){x,picker_top-6},1.5f,ink);
                }
            }
        }
        int resize_hover=hover(sidebar,workspace_top,6,height-workspace_top-24);
        if(!ui.studio_style)host_draw_rectangle(sidebar,workspace_top,6,height-workspace_top-24,ui.browser_resize || resize_hover?accent:cell);
        if(ui.studio_style)ui_divider_grip(host_draw(),(DrawRect){sidebar,workspace_top,6,height-workspace_top-24},1,ui.browser_resize || resize_hover);
        if(resize_hover || ui.browser_resize) { host_set_mouse_cursor(HOST_MOUSE_CURSOR_RESIZE_EW); snprintf(ui.status,sizeof ui.status,"Drag to resize Browser; below minimum width collapses it; drag outward to restore"); }
        ui_theme=browser_theme;
        if(ui.studio_style && !ui.browser_hidden) {
            ui_surface(host_draw(),(DrawRect){0,picker_top,sidebar,height-picker_top-24},panel);
            Rect r=ui.picker_bounds;DrawPoint pointer=ui.mouse;
            ui.input_enabled=!modal && !ui.pattern_popup && !ui.context_kind && !dragging && !ui.sample_drag[0] && !captured() && !ui.midi_take.active && ui.windows.grab<0 && desktop_picker_hit(pointer.x,pointer.y);
            host_begin_scissor_mode(0,picker_top*scale,sidebar*scale,(height-picker_top-24)*scale);
            ui.text_origin=(DrawPoint){r.x*scale,r.y*scale};
            host_begin_mode2_d((HostCamera){.offset=ui.text_origin,.zoom=scale});
            ui.mouse=(DrawPoint){pointer.x-r.x,pointer.y-r.y};
            playlist_picker(r,r.h);
            ui.mouse=pointer;ui.text_origin=(DrawPoint){0};
            host_end_scissor_mode();host_begin_mode2_d((HostCamera){.zoom=scale});
        }
        host_end_mode2_d();
        /* Freeze draw order: opening Piano Roll while drawing must not draw a window twice. */
        int order[WINDOW_CAPACITY]; memcpy(order,ui.windows.order,sizeof order);
        if(ui.windows.workspace.enabled){ui.input_enabled=!modal && !ui.pattern_popup && !ui.context_kind && !dragging && !captured();workspace_dividers();}
        for(int i=0;i<windows_count(&ui.windows);i++) { ui.input_enabled=!modal && !ui.pattern_popup && !ui.context_kind && !dragging && !ui.sample_drag[0] && !captured(); draw_editor(order[i],scale); }

        host_begin_mode2_d((HostCamera){.zoom=scale}); ui_surface(host_draw(),(DrawRect){0,height-22,width,22},panel); label(fit_text(ui.status,width-16,11),8,height-17,11,ink);
        draw_browser_drag();
        draw_picker_drag();
        draw_popup(); draw_context(); host_end_mode2_d();

        if(profile_frames) {
            draw_flush(host_draw());
            double elapsed=host_get_time()-render_start;
            if(frames>=40){profile_times[profile_count++]=elapsed;profile_cpu+=(clock()-render_cpu_start)/(double)CLOCKS_PER_SEC;}
        }
        if(!captured() && !recording_active()) history_checkpoint();
        if(ui.pattern_popup!=11 && ui.file_picker.entries) file_chooser_close(&ui.file_picker);
        sampler_queue();
        if(ui.recording_ui.active || ui.midi_take.active) { ui.project.bpm=ui.recording_ui.active?ui.recording_ui.bpm:ui.midi_take.bpm; ui.song=ui.playing=1; ui.reset=0; }
        flush_cursor(); audio_update(&ui.project,ui.playing,ui.song,ui.pattern,ui.reset,ui.output_volume,playback_start(),recording_active()?0:playback_loop()[0],recording_active()?0:playback_loop()[1]);
        if(audio_effect_memory_failed())snprintf(ui.status,sizeof ui.status,"Not enough memory for effect buffers; unprepared effects pass dry.");
        int animate=audio_active() || ui.mixer_decay_active || ui.spectrum_active || (ui.windows.editors[0].visible && ui.channel_decay_active) || ui.browser_progress>=0 || (ui.windows.editors[3].visible && (audio_active() || ui.mixer_decay_active)) || ui.record_count_in.active || ui.playing || captured() || ui.windows.grab>=0 || ui.popup_drag || ui.sample_drag[0] ||
            (ui.windows.editors[4].visible && audio_key_position(127,ui.instrument_channel)>=0);
        if(!smoke && !animate && host_get_time()-last_activity>.15) host_enable_event_waiting(); else host_disable_event_waiting();
        host_end_drawing();
        if(profile_frames) {
            frames++;
            if(profile_count==profile_frames){
                qsort(profile_times,profile_frames,sizeof *profile_times,profile_order);
                printf("{\"style\":\"%s\",\"motion\":%s,\"decorative_edges\":%s,\"width\":%d,\"height\":%d,\"frames\":%d,\"warmup\":40,\"audio_device_started\":%s,\"render_cpu_seconds\":%.6f,\"p50_ms\":%.6f,\"p99_ms\":%.6f,\"max_ms\":%.6f}\n",
                    ui.studio_style?"classic":"legacy",profile_motion?"true":"false",ui.profile_flat?"false":"true",host_get_screen_width(),host_get_screen_height(),profile_frames,audio_ok?"true":"false",profile_cpu,
                    profile_times[profile_frames/2]*1000,profile_times[(int)((profile_frames-1)*.99)]*1000,profile_times[profile_frames-1]*1000);
                break;
            }
        }
        else if(smoke && ++frames%10==0) {
            int stage=frames/10;
            host_take_screenshot(stage==1?"libreloop-smoke.png":host_text_format("libreloop-view-%d.png",stage-1));
            if(stage==4) {
                if(!project_save("libreloop-smoke.llp",&ui.project)) {fprintf(stderr,"Smoke project capture failed.\n");return 1;}
                break;
            }
            if(stage==1) windows_focus(&ui.windows,1);
            if(stage==2) windows_focus(&ui.windows,2);
            if(stage==3) windows_focus(&ui.windows,3);
        }
    }
    if(ui.studio_style && !smoke)workspace_save();
    free(profile_times);
    recording_finish();midi_panic();midi_input_wake(NULL);midi_input_close(); history_clear(&ui.edit_history); file_chooser_close(&ui.file_picker);
    if(ui.sampler_job.busy) { host_thread_join(ui.sampler_job.thread,NULL); sample_free(ui.sampler_job.input); sample_free(ui.sampler_job.result); }
    for(int pat=0;pat<PATTERNS;pat++) if(ui.previews[pat].image.id) host_unload_render_texture(ui.previews[pat].image);
    #ifdef __APPLE__

#endif
    text_fonts_close(host_fonts()); host_fonts_clear(); host_unload_texture(ui.circle_texture); host_unload_texture(ui.icons); host_unload_texture(ui.knob_arcs); host_unload_texture(ui.cable_texture);
    browser_close(&ui.browser); audio_close(); sample_free(ui.audition); free(ui.audition_wave.tree); theme_draw_close(); host_close_window();
    for(int c=0;c<CHANNELS;c++) { free(ui.audio_waves[c].tree); free(ui.sampler_views[c].wave.tree); sample_free(ui.samples[c]); sample_free(ui.originals[c]); }
    return 0;
}
