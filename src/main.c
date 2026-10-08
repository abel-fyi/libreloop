// SPDX-License-Identifier: GPL-3.0-only
#include "raylib.h"
#include "rlgl.h"
#include "audio.h"
#include "windows.h"
#include "browser.h"
#include "theme.h"
#include "arrangement.h"
#include "edit_clipboard.h"
#include "waveform.h"
#include "navigation.h"
#include "history.h"
#include "file_chooser.h"
#include "project_assets.h"
#include "project_document.h"
#include "text_fonts.h"
#include "recording.h"
#include "midi_input.h"
#include "atomic_file.h"
#include "preset.h"
#include "spectrum.h"
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <ctype.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stddef.h>
#include <unistd.h>

static char samples_path[PATH_MAX],font_path[PATH_MAX];
static int shortcut_down(void) {
#ifdef __APPLE__
    return IsKeyDown(KEY_LEFT_SUPER) || IsKeyDown(KEY_RIGHT_SUPER);
#else
    return IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL);
#endif
}
static int command_down(void) {
    return shortcut_down() || IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL)
        || IsKeyDown(KEY_LEFT_SUPER) || IsKeyDown(KEY_RIGHT_SUPER);
}
static int delete_pressed(void) {
#ifdef __APPLE__
    return IsKeyPressed(KEY_DELETE) || IsKeyPressed(KEY_BACKSPACE);
#else
    return IsKeyPressed(KEY_DELETE);
#endif
}
static void resource_paths(void) {
#ifdef __APPLE__
    snprintf(samples_path,sizeof samples_path,"%s../Resources/samples",GetApplicationDirectory());
    snprintf(font_path,sizeof font_path,"%s../Resources/fonts/LiberationSans-Regular.ttf",GetApplicationDirectory());
#else
    snprintf(samples_path,sizeof samples_path,"%s../share/libreloop/samples",GetApplicationDirectory());
    snprintf(font_path,sizeof font_path,"%s../share/libreloop/fonts/LiberationSans-Regular.ttf",GetApplicationDirectory());
    if(!DirectoryExists(samples_path) || !FileExists(font_path)) {
        snprintf(samples_path,sizeof samples_path,"%s",LIBRELOOP_SAMPLES);
        snprintf(font_path,sizeof font_path,"%s",LIBRELOOP_FONT);
    }
#endif
}

/* Keep presses even when release + press arrive in the same raylib frame.
   Chain raylib's callback so its normal button, touch and gesture state stays intact. */
static GLFWmousebuttonfun raylib_mouse_callback;
static unsigned pending_clicks[8],frame_clicks[8];
static void mouse_callback(GLFWwindow *window,int button,int action,int mods) {
    if(button>=0 && button<8 && action==GLFW_PRESS) pending_clicks[button]++;
    if(raylib_mouse_callback) raylib_mouse_callback(window,button,action,mods);
}
static int mouse_pressed(int button) { return button>=0 && button<8 && frame_clicks[button]; }
#define IsMouseButtonPressed mouse_pressed

static int requested_cursor,current_cursor;
static GLFWcursor *resize_cursor,*track_cursor,*hand_cursor;
static GLFWwindow *app_window;
static void request_cursor(int cursor) { requested_cursor=cursor; }
static void flush_cursor(void) {
    if(requested_cursor==current_cursor) return;
    glfwSetCursor(app_window,requested_cursor==MOUSE_CURSOR_RESIZE_EW?resize_cursor:requested_cursor==MOUSE_CURSOR_RESIZE_NS?track_cursor:requested_cursor==MOUSE_CURSOR_POINTING_HAND?hand_cursor:NULL);
    current_cursor=requested_cursor;
}
#define SetMouseCursor request_cursor

static Project project;
static History edit_history;
static Sample samples[CHANNELS],originals[CHANNELS];
static RecordingSession recording_ui;
static MidiTake midi_take;
enum { RECORD_AUDIO=1,RECORD_NOTES=2,RECORD_AUTOMATION=4 };
static unsigned record_mask=RECORD_AUDIO;
static int midi_record_target;
static int recording_active(void) {return recording_ui.active || midi_take.active;}
static MidiDevice midi_devices[64];static int midi_device_count,midi_connected,midi_scroll,midi_learning;
static char midi_selected[128],midi_selected_name[128],midi_settings_path[PATH_MAX];static double midi_refresh_time;
static ParameterTarget midi_learn_target;
typedef struct {int used,pressed,mchannel,pitch,target,velocity;} MidiHeld;
static MidiHeld midi_keys[32];static unsigned char midi_sustain[16];
static void midi_panic(void);static void midi_initialize(void);static void midi_poll(void);
static void midi_refresh(void);static void midi_select(int index);static void midi_save_settings(void);
static int midi_target(void);static void recording_start_audio(void);

static void recording_finish(void);
static void recording_poll(void);
static void recording_start(void);
static int recording_source(int source) {
    if(recording_ui.active) for(int i=0;i<recording_ui.count;i++) if(source==PATTERNS+recording_ui.takes[i].channel) return 1;
    return 0;
}
static int channel_missing(int c) {
    return c>=0 && c<project.channel_count && !originals[c].frames && project.paths[c][0] &&
        strcmp(project.paths[c],SAMPLE_EMPTY) && !recording_source(PATTERNS+c);
}
static const char *channel_caption(int c) {
    return channel_missing(c)?TextFormat("%s [missing]",project.channel_names[c]):project.channel_names[c];
}
static void sample_duration(Project *p,int c,Sample processed,Sample original) {
    if(original.frames || !p->paths[c][0] || !strcmp(p->paths[c],SAMPLE_EMPTY)) p->audio_seconds[c]=processed.frames/(float)RATE;
}
static Sampler sampler_applied[CHANNELS];
static unsigned sample_generation[CHANNELS],sampler_generation[CHANNELS],sample_epoch;
static struct {
    pthread_t thread; atomic_int done; int busy,channel,ok;
    unsigned generation,epoch; Sampler settings; Sample input,result;
} sampler_job;
static Sample sample_copy(Sample source) {
    Sample copy={0}; sample_clone(source,&copy); return copy;
}
static void *sampler_worker(void *unused) {
    (void)unused;
    sampler_job.ok=sample_process(sampler_job.input,sampler_job.settings,&sampler_job.result);
    atomic_store(&sampler_job.done,1); glfwPostEmptyEvent(); return NULL;
}
static void sampler_update(void) {
    if(sampler_job.busy && atomic_load(&sampler_job.done)) {
        pthread_join(sampler_job.thread,NULL); int c=sampler_job.channel;
        if(sampler_job.epoch==sample_epoch && c<project.channel_count && sampler_job.generation==sample_generation[c] && sampler_processing_equal(sampler_job.settings,project.sampler[c])) {
            if(sampler_job.ok) {
                Sample old=samples[c]; audio_sample(c,sampler_job.result); samples[c]=sampler_job.result; sampler_job.result=(Sample){0}; sample_free(old);
            } else project.sampler[c]=sampler_applied[c];
            sample_duration(&project,c,samples[c],originals[c]);
            sampler_applied[c]=project.sampler[c]; sampler_generation[c]=sample_generation[c];
        }
        sample_free(sampler_job.input); sample_free(sampler_job.result); sampler_job.busy=0;
    }
}

static int sampler_flush(void) {
    if(sampler_job.busy) {
        pthread_join(sampler_job.thread,NULL); sample_free(sampler_job.input); sample_free(sampler_job.result); sampler_job.busy=0;
    }
    for(int c=0;c<project.channel_count;c++) if(sampler_generation[c]!=sample_generation[c] || !sampler_processing_equal(sampler_applied[c],project.sampler[c])) {
        Sample next; if(!sample_process(originals[c],project.sampler[c],&next)) return 0;
        Sample old=samples[c]; audio_sample(c,next); samples[c]=next; sample_free(old);
        sample_duration(&project,c,samples[c],originals[c]);
        sampler_applied[c]=project.sampler[c]; sampler_generation[c]=sample_generation[c];
    }
    return 1;
}

#define bg ui_theme.background
#define panel ui_theme.surface
#define cell ui_theme.control
#define ink ui_theme.text
#define muted ui_theme.secondary
#define accent ui_theme.highlight
static Vector2 mouse;
static int pattern,channel,piano_channel,instrument_channel,playing,song,reset;
static int typing_keys=1;
static float output_volume=1;
static int metronome;
static int follow_playhead;
static double visual_step;
static Windows windows;
static int input_enabled=1;
static int picker_tab,picker_scroll,picker_drag=-1;
static int automation_selected=-1,automation_node=-1,automation_lane=-1,automation_clip=-1;
static int rename_automation=-1;
static float automation_grab_x;
static ParameterTarget automation_target;
static float *menu_value,menu_low,menu_high,menu_initial;
static char menu_name[96];
static Vector2 picker_origin,picker_offset;
static Arrangement arrangement={.source_steps=STEPS,.source_pattern=-1,.zoom=1};
static float rack_view[PATTERNS],rack_range[PATTERNS];
static int rack_hdrag; static float rack_hx,rack_hstart,rack_hscale;
#define RACK_STEP_WIDTH 16
#define RACK_TOP (TITLE+14)
#define MIXER_LEFT 62
#define MIXER_PANEL 160
static int playlist_pan,playlist_vpan;
static float track_scroll,playlist_vpan_y,playlist_vpan_start;
enum { PLAYLIST_GRID_TOP=96, PLAYLIST_BOTTOM=18, PIANO_GRID_TOP=86, PIANO_GRID_PADDING=180, EDITOR_SCROLLBAR=16, EDITOR_CORNER=16 };
static float track_heights[LANES],track_resize_y,track_resize_height;
static float track_zoom=1,row_zoom_y,row_zoom_start,row_zoom_anchor;
static int row_zoom_drag=-1;
static int selected_track=-1;
static int track_resize=-1;
static float track_height(int lane) { return fmaxf(28,fminf(640,(track_heights[lane]>0?track_heights[lane]:52)*track_zoom)); }
static float track_position(float lane) {
    float y=0; int l=fmaxf(0,fminf(LANES,floorf(lane)));
    for(int i=0;i<l;i++) y+=track_height(i);
    return y+(l<LANES?(lane-l)*track_height(l):0);
}
static float track_at(float position) {
    for(int l=0;l<LANES;l++) { float h=track_height(l); if(position<h) return l+position/h; position-=h; }
    return LANES;
}
static float playlist_pan_x,playlist_pan_start,playlist_pan_range;
static float playlist_start,piano_start[PATTERNS];
static int marker_drag=-1,ruler_loop_drag,stop_armed;
static int ruler_last_id=-1,ruler_last_pattern,ruler_last_button;
static double ruler_last_click=-1;
static Vector2 ruler_last_position;
static float song_loop[2],pattern_loop[PATTERNS][2],ruler_anchor;
static float *playback_loop(void) { return song?song_loop:pattern_loop[pattern]; }
static unsigned char piano_channels[PATTERNS][CHANNELS];
static int rack_vdrag;
static float rack_vy,rack_vstart,rack_vscale;
static int rack_filter; /* 0 All, 1 Audio, 2 Unsorted */
static const char *rack_filters[]={"All","Audio","Unsorted"};
static int rack_channels(int *rows) {
    int count=0;
    for(int c=0;c<project.channel_count;c++) if(!rack_filter || project.channel_audio[c]==(rack_filter==1)) { if(rows) rows[count]=c; count++; }
    return count;
}
static int rack_channel_at(int row) { int rows[CHANNELS],count=rack_channels(rows); return row>=0 && row<count?rows[row]:-1; }
static int rack_scroll,context_kind,context_target,context_opened,instrument_replace_scroll;
static Vector2 context_position;
static int pattern_popup,popup_opened,rename_pattern,rename_select_all,popup_drag;
static Vector2 popup_position[14],popup_offset;
static char rename_text[PATTERN_NAME],folder_text[PATH_MAX],number_text[64];
static float *number_target,number_low,number_high,number_default;
static const char *number_name;
static char device_names[64][128];
static int device_count,device_target,device_io,device_scroll;
static Browser browser;
static int route_drag=-1,route_start,mixer_selected=1,mixer_scroll,mixer_pan,cable_drag=-1;
static float route_y;
static Sample audition;
static double browser_progress=-1;
static char audition_path[PATH_MAX];
static Waveform audition_wave;
static int browser_focus=1,sample_moved,browser_hidden,browser_resize;
static float browser_width=174,last_sidebar=174;
static char sample_drag[PATH_MAX];
static Vector2 sample_origin,sample_offset;
static float sample_drag_width,sample_text_offset;
static float *control_drag,control_low,control_high,control_bottom,control_range;
static int control_fader,control_reverse,control_integer,control_logarithmic,number_integer;
static float control_raw;
static Note *note_drag;
static int moving_note,piano_scroll_drag,piano_vdrag;
static int piano_top=72;
static float piano_zoom=1;
static float piano_visible_rows(void) { return fmaxf(8,fminf(128,25/piano_zoom)); }
static int piano_min_top(void) { return (int)ceilf(piano_visible_rows())-1; }
static int piano_key_drag,piano_key=-1,piano_key_channel;
static float piano_scroll_x,piano_scroll_start,piano_scroll_range,piano_vscroll_y,piano_vscroll_start;
static Note note_before[NOTES];
static uint8_t note_selected[PATTERNS][CHANNELS][NOTES],note_selection_before[NOTES];
static uint8_t keyboard_notes[CHANNELS][128],playing_notes[CHANNELS][NOTES],playing_keys[CHANNELS][128];
static int piano_tool,piano_gesture,piano_additive;
enum { PIANO_IDLE, PIANO_BRUSH, PIANO_ERASE, PIANO_BOX };
static Vector2 piano_from,piano_now;
static Vector2 note_grab;
static float piano_note_length=2;
static EditClipboard edit_clipboard;
static float piano_span[PATTERNS]={16,16,16,16,16,16,16,16},piano_pan[PATTERNS],piano_range[PATTERNS];
static int velocity_drag=-1,velocity_pattern,velocity_channel; /* 0 paint, 1 line */
static Vector2 velocity_from,velocity_now;
static Note velocity_before[NOTES];
static int rack_paint=-1,rack_paint_channel=-1; static float rack_paint_step;
static int navigation_drag=-1,navigation_active;
static Vector2 navigation_last;
static NavigationInput navigation_input;
static float piano_scroll_remainder;
static int eq_drag=-1;
static int fm_dx_operator,fm_dx_page,fm_analog_tab,fm_dx_drag_base;
static int fm_tab,fm_motion_target,fm_graph_drag=-1,fm_graph_channel;
static Rectangle fm_graph_area;
static int captured(void) { return fm_graph_drag>=0 || row_zoom_drag>=0 || eq_drag>=0 || automation_node>=0 || picker_drag>=0 || navigation_drag>=0 || track_resize>=0 || rack_hdrag || rack_vdrag || piano_key_drag || control_drag || route_drag>=0 || note_drag || velocity_drag>=0 || arrangement.gesture || playlist_pan || browser_resize || rack_paint>=0 || cable_drag>0 || playlist_vpan || mixer_pan || piano_scroll_drag || piano_vdrag || piano_gesture || marker_drag>=0; }
static void sampler_queue(void) {
    if(recording_ui.active) return;
    sampler_update();
    if(sampler_job.busy || control_drag || arrangement.gesture==STRETCH_CLIP || pattern_popup==5) return;
    for(int c=0;c<project.channel_count;c++) if(sampler_generation[c]!=sample_generation[c] || !sampler_processing_equal(sampler_applied[c],project.sampler[c])) {
        Sample copy=sample_copy(originals[c]);
        if(originals[c].frames && !copy.data) return;
        sampler_job.input=copy; sampler_job.result=(Sample){0}; sampler_job.channel=c;
        sampler_job.settings=project.sampler[c]; sampler_job.epoch=sample_epoch; sampler_job.generation=sample_generation[c]; atomic_store(&sampler_job.done,0);
        if(pthread_create(&sampler_job.thread,NULL,sampler_worker,NULL)) { sample_free(copy); return; }
        sampler_job.busy=1; return;
    }
}
static char status[256]="Use the toolbar to open editors; drag their title bars to arrange them";
static ProjectDocument document={.path="project.hbt"};
static int help_scroll,relink_channel=-1;
static int project_dirty(void) { return project_document_dirty(&document,&project); }
static float ui_scale(void) { return fmaxf(1,fminf(GetScreenWidth()/1200.f,GetScreenHeight()/675.f)); }
static FileChooser file_picker;
static int preset_kind,preset_owner,preset_slot;
static int file_action,file_field,file_confirm,file_scroll_drag,file_last_row=-1;
static double file_last_click=-1;
static char file_directory[PATH_MAX],file_pending[PATH_MAX];
static float playback_start(void) {
    float start=song?playlist_start*STEPS:piano_start[pattern];
    float end=song?song_steps(&project):project.pattern_steps[pattern];
    return playing && !playback_loop()[1] && start>=end?0:start;
}
static void transport_toggle(void) { stop_armed=0; recording_finish(); int active=audio_stop(); playing=playing || active?0:1; reset=1; }
static void transport_stop(void) {
    int rewind=stop_armed && !playing;
    recording_finish(); audio_stop(); playing=0; reset=1;
    if(rewind) { playlist_start=0; if(!song) piano_start[pattern]=0; }
    stop_armed=1;
}
/* Rasterize TTF at each display size; never enlarge a small glyph atlas. */
static float font_scale;
static Vector2 text_origin;
static void icons_init(float scale);
static void fonts_update(float scale) {
    if(font_scale==scale) return;
    font_scale=scale; icons_init(scale); text_fonts_update(font_path,scale);
}
static int text_width(const char *text,int size) {
    return (int)ceilf(MeasureTextEx(text_font(text,size),text,text_font(text,size).baseSize,0).x/font_scale);
}
static float raster_position(float value,float origin) {
    return text_pixel_position(value,origin,ui_scale(),font_scale);
}
static void label_moving(const char *text,float x,float y,int size,Color color) {
    Font font=text_font(text,size);
    DrawTextEx(font,text,(Vector2){x,y},font.baseSize/font_scale,0,color);
}
static void label(const char *text,float x,float y,int size,Color color) {
    label_moving(text,raster_position(x,text_origin.x),raster_position(y,text_origin.y),size,color);
}
static void backspace(char *text) {
    size_t n=strlen(text); if(!n) return;
    do { n--; } while(n && ((unsigned char)text[n]&0xc0)==0x80);
    text[n]=0;
}
static const char *fit_text(const char *text,int width,int size) {
    static char fitted[PATH_MAX]; if(text!=fitted) snprintf(fitted,sizeof fitted,"%s",text);
    Font font=text_font(fitted,size); float line=0;
    /* Match MeasureTextEx with zero spacing, visiting each UTF-8 glyph once. */
    for(int i=0;fitted[i];) {
        int bytes,codepoint=GetCodepointNext(fitted+i,&bytes),glyph=GetGlyphIndex(font,codepoint);
        if(codepoint=='\n') line=0;
        else line+=font.glyphs[glyph].advanceX>0?font.glyphs[glyph].advanceX:font.recs[glyph].width+font.glyphs[glyph].offsetX;
        if(ceilf(line/font_scale)>width) { fitted[i]=0; break; }
        i+=bytes;
    }
    return fitted;
}
/* One shared alpha mask smooths small circles without window-wide MSAA. */
static Texture2D circle_texture;
static void circles_init(void) {
    Image image=GenImageColor(64,64,WHITE); Color *pixels=image.data;
    for(int y=0;y<64;y++) for(int x=0;x<64;x++) {
        float dx=x+.5f-32,dy=y+.5f-32;
        pixels[y*64+x].a=255*fmaxf(0,fminf(1,(32-sqrtf(dx*dx+dy*dy))/3));
    }
    circle_texture=LoadTextureFromImage(image); UnloadImage(image);
    SetTextureFilter(circle_texture,TEXTURE_FILTER_BILINEAR);
}
static void circle(float x,float y,float radius,Color color) {
    DrawTexturePro(circle_texture,(Rectangle){0,0,64,64},(Rectangle){x-radius,y-radius,radius*2,radius*2},(Vector2){0},0,color);
}
/* Monochrome symbols rasterized once at 4x, then reduced for smooth edges. */
enum { ICON_RACK,ICON_PLAYLIST,ICON_PIANO,ICON_MIXER,ICON_PENCIL,ICON_BRUSH,ICON_SELECT,ICON_METRO,ICON_PLAY,ICON_STOP,ICON_PAUSE,ICON_CLOSE,ICON_BACK,ICON_FOLLOW,ICON_KEYS,ICON_WAVE,ICON_AUTOMATION,ICON_CUT,ICON_STRETCH,ICON_ROW_ZOOM,ICON_COUNT };
static Texture2D icons;
static int icon_large,icon_small,icon_medium;
static void icons_init(float scale) {
    if(icons.id) UnloadTexture(icons);
    icon_large=fmaxf(1,roundf(28*scale)); icon_small=fmaxf(1,roundf(16*scale));
    icon_medium=fmaxf(1,roundf(20*scale));
    int high=icon_large*4;
    RenderTexture2D target=LoadRenderTexture(ICON_COUNT*high,high);
    BeginTextureMode(target); ClearBackground((Color){255,255,255,0});
    for(int id=0;id<ICON_COUNT;id++) {
        BeginMode2D((Camera2D){.offset={id*high,0},.zoom=high/28.f});
        if(id==ICON_RACK) for(int row=0;row<3;row++) {
            DrawRectangle(5,7+row*6,4,3,WHITE);
            for(int step=0;step<3;step++) DrawRectangle(12+step*4,7+row*6,3,3,WHITE);
        }
        if(id==ICON_PLAYLIST) for(int row=0;row<3;row++) {
            DrawLineEx((Vector2){5,8+row*6},(Vector2){23,8+row*6},1,Fade(WHITE,.45f));
            DrawRectangle(5+(row%2)*7,6+row*6,10,4,WHITE);
        }
        if(id==ICON_PIANO) {
            DrawRectangleLinesEx((Rectangle){4,6,20,16},1.5f,WHITE);
            for(int key=1;key<4;key++) DrawLineEx((Vector2){4+key*5,6},(Vector2){4+key*5,22},1,WHITE);
            for(int key=0;key<2;key++) DrawRectangle(7+key*5,6,4,9,WHITE);
            DrawRectangle(21,6,3,9,WHITE); /* Keep the partial F-sharp distinct from the border at small sizes. */
        }
        if(id==ICON_MIXER) for(int strip=0;strip<3;strip++) {
            int x=7+strip*7,y=9+strip*4;
            DrawLineEx((Vector2){x,5},(Vector2){x,23},1.5f,WHITE);
            DrawRectangle(x-3,y-2,6,4,WHITE);
        }
        if(id==ICON_PENCIL) {
            DrawLineEx((Vector2){8,20},(Vector2){21,7},3.5f,WHITE);
            DrawTriangle((Vector2){5,23},(Vector2){10,21},(Vector2){7,18},WHITE);
        }
        if(id==ICON_BRUSH) {
            DrawLineEx((Vector2){14,14},(Vector2){21,6},3,WHITE);
            DrawCircle(11,17,4,WHITE);
            DrawTriangle((Vector2){5,23},(Vector2){13,21},(Vector2){8,15},WHITE);
        }
        if(id==ICON_SELECT) for(int i=0;i<3;i++) {
            int p=6+i*6;
            DrawLineEx((Vector2){p,6},(Vector2){p+3,6},1.5f,WHITE);
            DrawLineEx((Vector2){p,22},(Vector2){p+3,22},1.5f,WHITE);
            DrawLineEx((Vector2){6,p},(Vector2){6,p+3},1.5f,WHITE);
            DrawLineEx((Vector2){22,p},(Vector2){22,p+3},1.5f,WHITE);
        }
        if(id==ICON_CUT) {
            DrawCircleLines(8,19,4,WHITE); DrawCircleLines(20,19,4,WHITE);
            DrawLineEx((Vector2){10,16},(Vector2){22,5},2,WHITE);
            DrawLineEx((Vector2){18,16},(Vector2){6,5},2,WHITE);
            DrawCircle(14,12,1.5f,WHITE);
        }
        if(id==ICON_STRETCH) {
            DrawLineEx((Vector2){5,14},(Vector2){23,14},2,WHITE);
            DrawLineEx((Vector2){5,14},(Vector2){10,9},2,WHITE);
            DrawLineEx((Vector2){5,14},(Vector2){10,19},2,WHITE);
            DrawLineEx((Vector2){23,14},(Vector2){18,9},2,WHITE);
            DrawLineEx((Vector2){23,14},(Vector2){18,19},2,WHITE);
        }
        if(id==ICON_ROW_ZOOM) {
            DrawLineEx((Vector2){14,5},(Vector2){14,23},1.75f,WHITE);
            DrawLineEx((Vector2){9,10},(Vector2){14,5},1.75f,WHITE);
            DrawLineEx((Vector2){19,10},(Vector2){14,5},1.75f,WHITE);
            DrawLineEx((Vector2){9,18},(Vector2){14,23},1.75f,WHITE);
            DrawLineEx((Vector2){19,18},(Vector2){14,23},1.75f,WHITE);
        }
        if(id==ICON_METRO) {
            DrawLineEx((Vector2){7,22},(Vector2){11,6},1.8f,WHITE);
            DrawLineEx((Vector2){11,6},(Vector2){16,22},1.8f,WHITE);
            DrawLineEx((Vector2){7,22},(Vector2){16,22},1.8f,WHITE);
            DrawLineEx((Vector2){12,19},(Vector2){21,8},1.8f,WHITE); DrawCircle(21,8,2,WHITE);
        }
        if(id==ICON_FOLLOW) {
            DrawLineEx((Vector2){7,5},(Vector2){7,23},1.5f,WHITE);
            DrawLineEx((Vector2){12,14},(Vector2){23,14},1.8f,WHITE);
            DrawLineEx((Vector2){18,9},(Vector2){23,14},1.8f,WHITE);
            DrawLineEx((Vector2){18,19},(Vector2){23,14},1.8f,WHITE);
        }
        if(id==ICON_KEYS) {
            DrawRectangleLinesEx((Rectangle){3,7,22,14},1.5f,WHITE);
            for(int row=0;row<2;row++) for(int key=0;key<5;key++) DrawRectangle(6+key*3.5f,10+row*4,2,2,WHITE);
            DrawRectangleRec((Rectangle){9,18,10,1.5f},WHITE);
        }
        if(id==ICON_WAVE) {
            Vector2 previous={4,14};
            for(int step=1;step<=48;step++) {
                float phase=step/48.f;
                Vector2 next={4+20*phase,14-7*sinf(phase*2*PI)};
                DrawLineEx(previous,next,2,WHITE); previous=next;
            }
        }
        if(id==ICON_AUTOMATION) { DrawLineEx((Vector2){5,21},(Vector2){14,7},2,WHITE); DrawLineEx((Vector2){14,7},(Vector2){23,17},2,WHITE); DrawCircle(5,21,3,WHITE); DrawCircle(14,7,3,WHITE); DrawCircle(23,17,3,WHITE); }
        if(id==ICON_PLAY) DrawTriangle((Vector2){9,6},(Vector2){9,22},(Vector2){22,14},WHITE);
        if(id==ICON_BACK) DrawTriangle((Vector2){19,6},(Vector2){6,14},(Vector2){19,22},WHITE);
        if(id==ICON_STOP) DrawRectangle(7,7,14,14,WHITE);
        if(id==ICON_PAUSE) { DrawRectangle(7,6,5,16,WHITE); DrawRectangle(16,6,5,16,WHITE); }
        if(id==ICON_CLOSE) {
            DrawLineEx((Vector2){7,7},(Vector2){21,21},2,WHITE);
            DrawLineEx((Vector2){7,21},(Vector2){21,7},2,WHITE);
        }
        EndMode2D();
    }
    EndTextureMode();
    Image source=LoadImageFromTexture(target.texture); ImageFlipVertical(&source);
    Image image=GenImageColor(ICON_COUNT*icon_large,icon_large+icon_medium+icon_small,BLANK);
    Color *input=source.data,*output=image.data;
    /* Exact area coverage avoids the blur of bicubic resize and a second filter. */
    for(int variant=0;variant<3;variant++) {
        int size=variant==2?icon_small:variant==1?icon_medium:icon_large,row=variant==2?icon_large+icon_medium:variant==1?icon_large:0;
        float ratio=high/(float)size;
        for(int id=0;id<ICON_COUNT;id++) for(int y=0;y<size;y++) for(int x=0;x<size;x++) {
            float left=x*ratio,top=y*ratio,right=(x+1)*ratio,bottom=(y+1)*ratio,alpha=0;
            for(int sy=floorf(top);sy<ceilf(bottom) && sy<high;sy++) for(int sx=floorf(left);sx<ceilf(right) && sx<high;sx++) {
                float weight=(fminf(right,sx+1)-fmaxf(left,sx))*(fminf(bottom,sy+1)-fmaxf(top,sy));
                alpha+=input[sy*source.width+id*high+sx].a*weight;
            }
            output[(row+y)*image.width+id*icon_large+x]=(Color){255,255,255,roundf(alpha/(ratio*ratio))};
        }
    }
    icons=LoadTextureFromImage(image); SetTextureFilter(icons,TEXTURE_FILTER_POINT);
    UnloadImage(source); UnloadImage(image); UnloadRenderTexture(target);
}
static void icon(int id,float x,float y,float size,Color color) {
    int variant=size<=16?2:size<=20?1:0,pixels=variant==2?icon_small:variant==1?icon_medium:icon_large;
    float left=raster_position(x-pixels/font_scale/2,text_origin.x);
    float top=raster_position(y-pixels/font_scale/2,text_origin.y);
    DrawTexturePro(icons,(Rectangle){id*icon_large,variant==2?icon_large+icon_medium:variant==1?icon_large:0,pixels,pixels},(Rectangle){left,top,pixels/font_scale,pixels/font_scale},(Vector2){0},0,color);
}
/* Cached value arcs keep their edges as smooth as the knob circles. */
static Texture2D knob_arcs;
static int knob_context=-1;
#define KNOB_RADIUS 10
#define SWING_RADIUS 8
static void arcs_init(void) {
    /* Swing horseshoe, centered full turn, and minimum-to-value full turn. */
    Image image=GenImageColor(1536,288,BLANK);
    Color *pixels=image.data;
    for(int bank=0;bank<3;bank++) for(int level=0;level<=128;level++) for(int y=0;y<32;y++) for(int x=0;x<32;x++) {
        float dx=x+.5f-16,dy=y+.5f-16,distance=sqrtf(dx*dx+dy*dy);
        float origin=bank==0?2.4f:bank==1?PI/2:-PI/2;
        float angle=atan2f(dy,dx)-origin; if(angle<0) angle+=2*PI;
        float end=level/128.f*(bank==0?4.6f:2*PI),start=bank==1?PI:0;
        float alpha=fmaxf(0,fminf(1,fminf(16-distance,distance-(16-48.f/KNOB_RADIUS))/1.4f));
        if(bank!=2 || level!=128) alpha*=fmaxf(0,fminf(1,fminf(angle-fminf(start,end),fmaxf(start,end)-angle)*distance));
        pixels[(level/16*32+y)*1536+bank*512+level%16*32+x]=(Color){255,255,255,255*alpha};
    }
    knob_arcs=LoadTextureFromImage(image); UnloadImage(image);
    SetTextureFilter(knob_arcs,TEXTURE_FILTER_BILINEAR);
}
/* A narrow alpha ramp antialiases the cable strip in one draw batch. */
static Texture2D cable_texture;
static void cables_init(void) {
    Image image=GenImageColor(1,16,WHITE); Color *pixels=image.data;
    for(int y=0;y<16;y++) pixels[y].a=255*fminf(1,(8-fabsf(y+.5f-8))/4);
    cable_texture=LoadTextureFromImage(image); UnloadImage(image);
    SetTextureFilter(cable_texture,TEXTURE_FILTER_BILINEAR);
}
static void cable(Vector2 points[4],Color color) {
    Vector2 previous[2]={{0}},normal={2,0};
    rlSetTexture(cable_texture.id); rlBegin(RL_QUADS); rlColor4ub(color.r,color.g,color.b,color.a);
    for(int i=0;i<=64;i++) {
        float t=i/64.f,u=1-t;
        Vector2 p=GetSplinePointBezierCubic(points[0],points[1],points[2],points[3],t);
        float dx=3*u*u*(points[1].x-points[0].x)+6*u*t*(points[2].x-points[1].x)+3*t*t*(points[3].x-points[2].x);
        float dy=3*u*u*(points[1].y-points[0].y)+6*u*t*(points[2].y-points[1].y)+3*t*t*(points[3].y-points[2].y);
        float length=hypotf(dx,dy); if(length>.001f) normal=(Vector2){-dy/length*2,dx/length*2};
        Vector2 edge[2]={{p.x+normal.x,p.y+normal.y},{p.x-normal.x,p.y-normal.y}};
        if(i) {
            rlTexCoord2f(.5f,0); rlVertex2f(previous[0].x,previous[0].y);
            rlTexCoord2f(.5f,0); rlVertex2f(edge[0].x,edge[0].y);
            rlTexCoord2f(.5f,1); rlVertex2f(edge[1].x,edge[1].y);
            rlTexCoord2f(.5f,1); rlVertex2f(previous[1].x,previous[1].y);
        }
        previous[0]=edge[0]; previous[1]=edge[1];
    }
    rlEnd(); rlSetTexture(0);
}
/* The cable's cached alpha strip also smooths straight automation segments. */
static void smooth_line(Vector2 start,Vector2 end,float width,Color color) {
    float dx=end.x-start.x,dy=end.y-start.y,length=hypotf(dx,dy);
    if(length<.001f) return;
    float radius=width*2/3;
    Vector2 normal={-dy/length*radius,dx/length*radius};
    rlSetTexture(cable_texture.id); rlBegin(RL_QUADS); rlColor4ub(color.r,color.g,color.b,color.a);
    rlTexCoord2f(.5f,0); rlVertex2f(start.x+normal.x,start.y+normal.y);
    rlTexCoord2f(.5f,0); rlVertex2f(end.x+normal.x,end.y+normal.y);
    rlTexCoord2f(.5f,1); rlVertex2f(end.x-normal.x,end.y-normal.y);
    rlTexCoord2f(.5f,1); rlVertex2f(start.x-normal.x,start.y-normal.y);
    rlEnd(); rlSetTexture(0);
}
static int hover(float x,float y,float w,float h) { return input_enabled && CheckCollisionPointRec(mouse,(Rectangle){x,y,w,h}); }
/* Small control symbols share one size and never depend on font glyphs. */
static int symbol(const char *text,int x,int y,Color c) {
    int id;
    if(!strcmp(text,"x")) id=ICON_CLOSE;
    else if(!strcmp(text,"[]")) id=ICON_STOP;
    else if(!strcmp(text,"||")) id=ICON_PAUSE;
    else if(!strcmp(text,">")) id=ICON_PLAY;
    else if(!strcmp(text,"<")) id=ICON_BACK;
    else return 0;
    icon(id,x,y,16,c);
    return 1;
}
static int button_color(const char *text,int x,int y,int w,int h,int active,Color base) {
    int over=hover(x,y,w,h);
    if(over) SetMouseCursor(MOUSE_CURSOR_POINTING_HAND);
    if(over && *text) {
        const char *description=text;
        if(!strcmp(text,"New")) description="Start an empty project with one unloaded Sampler";
        else if(!strcmp(text,"Demo")) description="Load the built-in drum and bass demo";
        else if(!strcmp(text,"Save")) description="Save the current project";
        else if(!strcmp(text,"Open...")) description="Choose a project to open";
        else if(!strcmp(text,"Export...")) description="Choose where to export the Playlist as a WAV file";
        else if(!strcmp(text,"HELP")) description="Show all current keybindings";
        else if(!strcmp(text,"Dark")) description="Use dark colors; saved automatically for next launch";
        else if(!strcmp(text,"Light")) description="Use light colors; saved automatically for next launch";
        else if(!strcmp(text,"+ Add folder")) description="Add a folder as a new Browser tree root";
        else if(!strcmp(text,"PAT") || !strcmp(text,"SONG")) description="Switch playback between the current pattern and Playlist";
        else if(!strcmp(text,"Rack")) description="Show/focus or hide the Channel Rack";
        else if(!strcmp(text,"List")) description="Show/focus or hide the Playlist";
        else if(!strcmp(text,"Piano")) description="Show/focus or hide the Piano Roll";
        else if(!strcmp(text,"Mix")) description="Show/focus or hide the Mixer";
        else if(!strcmp(text,"x")) description="Close this dialog";
        else if(!strcmp(text,"<")) description="Previous";
        else if(!strcmp(text,">")) description="Next";
        else if(!strcmp(text,"[]")) description="Stop playback and return to the beginning";
        else if(!strcmp(text,"||")) description="Pause playback";
        else if(!strcmp(text,"Reset")) description="Restore this control's default value";
        else if(!strcmp(text,"Apply")) description="Apply the entered value";
        else if(!strcmp(text,"Cancel")) description="Cancel without changing the value";
        snprintf(status,sizeof status,"%s",description);
    }
    ui_surface((Rectangle){x,y,w,h},active?(over?ui_theme.active_hover:accent):over?ui_theme.hover:base);
    if(w>52 || !symbol(text,x+w/2,y+h/2,active?ui_theme.selected_text:ink)) {
        int size=15; if(w<=60 && text_width(text,size)>w-8) size=14;
        const char *caption=fit_text(text,w-(w<=72?8:16),size);
        label(caption,x+((w<=72 || !strcmp(text,"+"))?(w-text_width(caption,size))/2:8),y+(h-size)/2,size,active?ui_theme.selected_text:ink);
    }
    return over && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
}
/* One light everywhere: mute with left-click, toggle solo group membership with right-click. */
static int button(const char *text,int x,int y,int w,int h,int active) {
    return button_color(text,x,y,w,h,active,cell);
}
static Color clip_foreground(Color fill);
static int drag_button(const char *text,int x,int y,int w,int h,int active) {
    int clicked=button(text,x,y,w,h,active);
    if(hover(x,y,w,h) || active || clicked) SetMouseCursor(MOUSE_CURSOR_RESIZE_NS);
    return clicked;
}
static int color_picker(int x,int y,int width,uint32_t selected) {
    int chosen=-1,spacing=width/COLOR_HUES;
    for(int i=0;i<COLOR_COUNT;i++) {
        unsigned rgb=pattern_palette[i]; Color color={rgb>>16,(rgb>>8)&255,rgb&255,255};
        Rectangle swatch={x+(i%COLOR_HUES)*spacing,y+(i/COLOR_HUES)*28,spacing-4,24};
        int over=hover(swatch.x,swatch.y,swatch.width,swatch.height);
        ui_surface(swatch,color);
        if(over || selected==rgb) DrawRectangleLinesEx((Rectangle){swatch.x+2,swatch.y+2,swatch.width-4,swatch.height-4},2,clip_foreground(color));
        if(over) { SetMouseCursor(MOUSE_CURSOR_POINTING_HAND); snprintf(status,sizeof status,"Choose color #%06X",rgb); if(IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) chosen=i; }
    }
    return chosen;
}
static int picker_button(int y,Color color,int selected) {
    int over=hover(4,y,112,50);
    if(over) SetMouseCursor(MOUSE_CURSOR_POINTING_HAND);
    ui_surface((Rectangle){4,y,112,50},color);
    if(over || selected) DrawRectangleLinesEx((Rectangle){4,y,112,50},2,accent);
    return over && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
}
static float automation_shown[AUTOMATIONS];
static uint8_t automation_visible[AUTOMATIONS];
static void automation_display_update(void) {
    memset(automation_visible,0,sizeof automation_visible);
    if(!playing || !song || !project.automation_count) return;
    int solo=solo_any(project.lane_mute,LANES);
    double held_end[AUTOMATIONS]; for(int a=0;a<project.automation_count;a++) held_end[a]=-INFINITY;
    for(int pass=0;pass<2;pass++) for(int l=0;l<LANES;l++) if(!(project.lane_mute[l]&1) && (!solo || (project.lane_mute[l]&2))) for(int b=0;b<CLIPS;b++) {
        int a=project.clips[l][b]-AUTOMATION_SOURCE-1; if(a<0 || a>=project.automation_count) continue;
        float start=project.clip_starts[l][b]*STEPS;
        float end=start+clip_length(&project,l,b);
        if(pass==0?(visual_step<end || end<held_end[a]):(visual_step<start || visual_step>=end)) continue;
        ParameterTarget target=project.automations[a].target;
        float normalized=automation_value(&project.automations[a],(pass==0?end:visual_step)-start+project.clip_offsets[l][b]);
        for(int i=0;i<project.automation_count;i++) {
            ParameterTarget t=project.automations[i].target;
            if(t.parameter==target.parameter && t.owner==target.owner && t.slot==target.slot) { automation_visible[i]=1; automation_shown[i]=normalized; if(pass==0) held_end[i]=end; }
        }
    }
}
static float displayed_value(const void *pointer,float manual) {
    if(!playing || !song || !project.automation_count) return manual;
    ParameterTarget target; float value,lo,hi;
    if(!parameter_from_pointer(&project,pointer,&target) || !parameter_info(&project,target,&value,&lo,&hi)) return manual;
    for(int a=0;a<project.automation_count;a++) {
        ParameterTarget t=project.automations[a].target;
        if(automation_visible[a] && t.parameter==target.parameter && t.owner==target.owner && t.slot==target.slot) {
            float shown=lo+(hi-lo)*automation_shown[a]; return target.parameter==PARAM_PITCH_RANGE?roundf(shown):shown;
        }
    }
    return manual;
}
static void open_context(int kind,int target,Vector2 position);
static void mute_light(float x,float y,uint8_t *states,int count,int selected,const char *name) {
    uint8_t state=states[selected]; if(displayed_value(&states[selected],!!(state&1))>=.5f) state|=1; else state&=~1; int solo=count?solo_any(states,count):0;
    int over=hover(x-8,y-8,16,16);
    if(over) SetMouseCursor(MOUSE_CURSOR_POINTING_HAND);
    int enabled=!(state&1) && (!solo || (state&2));
    Color color=enabled?(state&2?ui_theme.meter_mid:muted):cell;
    circle(x,y,6,over?ink:state&2?ui_theme.meter_mid:ui_theme.border); circle(x,y,5,color);
    if(over) {
        snprintf(status,sizeof status,"%s: %s | Left-click: mute/unmute; right-click: %s",name,state&1?"Muted":state&2?"Solo":solo?"Silenced by solo":"Enabled",states==project.lane_mute?"add/remove solo":"automation / solo menu");
        if(IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) { states[selected]^=1; input_enabled=0; }
        if(IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
            if(parameter_from_pointer(&project,&states[selected],&automation_target)) {
                snprintf(menu_name,sizeof menu_name,"Mute");
                Vector2 position=mouse; if(knob_context>=0) { position.x+=windows.editors[knob_context].rect.x; position.y+=windows.editors[knob_context].rect.y; }
                open_context(10,0,position); return;
            }
            if(count) solo_toggle(states,count,selected);
            else {
                for(int i=0;i<INSERTS;i++) project.insert_mute[i]&=~2;
                for(int c=0;c<CHANNELS;c++) project.mute[c]&=~2;
                for(int l=0;l<LANES;l++) project.lane_mute[l]&=~2;
            }
            input_enabled=0;
        }
    }
}
static int editor_button(int id,int x) {
    static const char *names[]={"Channel Rack","Playlist","Piano Roll","Mixer"};
    int front=0;
    for(int i=EDITORS-1;i>=0;i--) {
        int other=windows.order[i];
        if(windows.editors[other].visible && windows.editors[other].pinned==windows.editors[id].pinned) { front=other==id; break; }
    }
    int clicked=button("",x,8,22,22,0);
    if(clicked) { if(front) windows.editors[id].visible=0; else windows_focus(&windows,id); }
    icon(id,x+11,19,20,ink);
    if(hover(x,8,22,22)) snprintf(status,sizeof status,"%s: %s",names[id],front?"hide this window":"show / bring this window to the front");
    return clicked;
}
static void open_context(int kind,int target,Vector2 position) {
    context_kind=kind; context_target=target; context_position=position; context_opened=1; input_enabled=0;
}
static void add_instrument(int type);
static int preset_directory(char *out,size_t capacity,int kind) {
    const char *slash=strrchr(browser.config,'/');
    if(!slash) return 0;
    char root[PATH_MAX];
    if(snprintf(root,sizeof root,"%.*s/presets",(int)(slash-browser.config),browser.config)>=(int)sizeof root || MakeDirectory(root)) return 0;
    const char *device=kind==PRESET_FM?"FM Synth":kind==PRESET_EQ?"Equalizer":kind==PRESET_CHORUS?"Chorus":"Sampler";
    if(snprintf(out,capacity,"%s/%s",root,device)>=(int)capacity || MakeDirectory(out)) return 0;
    if(kind==PRESET_FM) for(int i=0;i<FM_FACTORY_COUNT;i++) {
        char factory[PATH_MAX];
        if(snprintf(factory,sizeof factory,"%s/%s.llpreset",out,fm_factory_name(i))<(int)sizeof factory && !FileExists(factory)) {
            DevicePreset preset={.kind=PRESET_FM,.fm=fm_factory(i)}; preset_save(factory,&preset);
        }
    }
    return 1;
}
static void preset_action(int kind,int owner,int slot,int save);
static void open_popup(int kind) {
    if(kind==2) rename_automation=-1;
    if(kind==4) help_scroll=0;
    pattern_popup=kind; popup_opened=1; popup_drag=0;
    popup_position[kind]=kind==7?(Vector2){60,34}:(Vector2){-1,-1};
}
static int rename_track=-1,rename_mixer=-1,rename_channel=-1;
static const char *mixer_name(int id) { return id?project.insert_names[id-1]:"Master"; }
static void begin_rename(void) {
    rename_track=rename_mixer=rename_channel=-1;
    rename_pattern=pattern; snprintf(rename_text,sizeof rename_text,"%s",project.pattern_names[pattern]);
    rename_select_all=1; open_popup(2);
}
static void begin_number(float *value,float low,float high,float initial,const char *name) {
    number_integer=0; number_target=value; number_low=low; number_high=high; number_default=initial; number_name=name;
    snprintf(number_text,sizeof number_text,"%.6g",*value); rename_select_all=1; open_popup(5);
}
static void control_menu(float *value,float low,float high,float initial,const char *name) {
    int automated=parameter_from_pointer(&project,value,&automation_target);
    menu_value=value; menu_low=low; menu_high=high; menu_initial=initial;
    snprintf(menu_name,sizeof menu_name,"%s",name);
    Vector2 position=mouse;
    if(knob_context>=0) { position.x+=windows.editors[knob_context].rect.x; position.y+=windows.editors[knob_context].rect.y; }
    open_context(automated?9:15,0,position);
}
static void create_automation(void) {
    float start=song_loop[1]>song_loop[0]?song_loop[0]/STEPS:floorf((playing && song?visual_step:playlist_start)/STEPS);
    float steps=song_loop[1]>song_loop[0]?song_loop[1]-song_loop[0]:fmaxf(STEPS,project.pattern_steps[pattern]);
    char name[PATTERN_NAME];
    unsigned id=automation_target.parameter,owner=automation_target.owner;
    const char *prefix=((id>=PARAM_FM_RATIO && id<=PARAM_FM_LAST) || (id>=PARAM_DX7_FIRST && id<=PARAM_DX7_LAST))?project.channel_names[owner]:id>=PARAM_CHORUS_RATE && id<=PARAM_EFFECT_MIX?(owner?project.insert_names[owner-1]:"Master"):id<=PARAM_CHANNEL_PITCH || id==PARAM_CHANNEL_MUTE || id==PARAM_PITCH_RANGE?project.channel_names[owner]:id<=PARAM_INSERT_WIDTH || id==PARAM_INSERT_MUTE?project.insert_names[owner]:"Master";
    const char *parameter=menu_name;
    if(!strncmp(parameter,"Insert ",7)) parameter+=7;
    else if(!strncmp(parameter,"Channel ",8)) parameter+=8;
    else if(!strncmp(parameter,"Master ",7)) parameter+=7;
    snprintf(name,sizeof name,"%.20s %.23s",prefix,parameter);
    int a=automation_create(&project,automation_target,name,steps);
    if(a<0) { snprintf(status,sizeof status,"Cannot create automation: %d clip limit.",AUTOMATIONS); return; }
    int lane,slot=-1;
    for(lane=0;lane<LANES;lane++) if((slot=arrangement_place(&project,lane,start,AUTOMATION_SOURCE+a,steps))>=0) break;
    if(slot<0) { automation_delete(&project,a); snprintf(status,sizeof status,"No free Playlist space for automation."); return; }
    automation_selected=a; picker_tab=2; picker_scroll=a; picker_drag=-1;
    arrangement.source_pattern=AUTOMATION_SOURCE+a; arrangement.source_steps=steps; arrangement.source_offset=0;
    memset(arrangement.selected,0,sizeof arrangement.selected); arrangement.selected[lane][slot]=1;
    arrangement.view_start=fmaxf(0,start); track_scroll=track_position(lane); browser_focus=0; windows_focus(&windows,1);
    snprintf(status,sizeof status,"Automation created: click the line to add points; drag points; right-click points to delete.");
}
static void text_input(char *text,size_t capacity) {
    if(shortcut_down() && IsKeyPressed(KEY_A)) rename_select_all=1;
    if(shortcut_down() && IsKeyPressed(KEY_V)) {
        const char *paste=GetClipboardText();
        if(paste) {
            if(rename_select_all) { text[0]=0; rename_select_all=0; }
            while(*paste) {
                int bytes,ch=GetCodepointNext(paste,&bytes); size_t n=strlen(text);
                if(ch<32 || ch==127 || n+bytes>=capacity) break;
                memcpy(text+n,paste,bytes); text[n+bytes]=0; paste+=bytes;
            }
        }
    }
    if(IsKeyPressed(KEY_BACKSPACE)) { if(rename_select_all) text[0]=0; else backspace(text); rename_select_all=0; }
    if(IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_RIGHT)) rename_select_all=0;
    int ch;
    while((ch=GetCharPressed())>0) {
        if(ch<32 || ch==127 || command_down()) continue;
        if(rename_select_all) { text[0]=0; rename_select_all=0; }
        int bytes; const char *utf8=CodepointToUTF8(ch,&bytes); size_t n=strlen(text);
        if(n+bytes<capacity) { memcpy(text+n,utf8,bytes); text[n+bytes]=0; }
    }
}
static const char *device_caption(const char *name) { return !*name?"None":!strcmp(name,"@default")?"Default device":name; }
static void device_button(int io,int x,int y,int width) {
    const char *selected=project.audio_io[mixer_selected][io];
    if(button_color(fit_text(TextFormat("%s: %s",io?"Out":"In",device_caption(selected)),width-30,13),x,y,width,22,0,ui_theme.browser) && !recording_active()) {
        device_target=mixer_selected; device_io=io; device_scroll=0;
        device_count=audio_devices(!io,device_names,64);
        open_popup(8);
        Rect r=windows.editors[3].rect;
        popup_position[8]=(Vector2){r.x+x,r.y+y+24};
    }
    if(hover(x,y,width,22)) snprintf(status,sizeof status,io?"Output device choice is saved; external-output routing is not implemented yet":"Input device for recording this mixer track; None records its internal audio");
    DrawTriangle((Vector2){x+width-16,y+8},(Vector2){x+width-12,y+14},(Vector2){x+width-8,y+8},muted);
}
static int load_project(const char *path);
static void new_project(int demo);
static void replace_project(int action);
static void replacement_saved(void);
static void request_load_project(const char *path);
static int history_checkpoint(void);
static void undo_redo(int direction);
static void edit_selection(int action);
static void piano_octave(int direction);
static void project_file_action(int action);
static void draw_file_picker(int x,int y,int w,int h,float scale);
static void help_group(int x,int *y,const char *title) {
    label(title,x,*y,13,accent); *y+=26;
}
static void help_binding(int x,int *y,const char *keys,const char *action) {
    label(keys,x,*y,11,ink); label(action,x+126,*y,11,muted); *y+=23;
}
static void draw_popup(void) {
    if(!pattern_popup) { popup_drag=0; return; }
    input_enabled=1;
    if(pattern_popup==10) {
        int count=windows.focused==2 && piano_tool==PENCIL?8:6;
        int x=168,y=34,w=210,h=8+count*26;
        if(IsKeyPressed(KEY_ESCAPE) || (!popup_opened && IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && !hover(x,y,w,h))) { pattern_popup=0; return; }
        ui_frame((Rectangle){x,y,w,h});
        const char *names[]={"Undo","Redo","Cut","Copy","Paste","Select all","Octave up","Octave down"};
        for(int i=0;i<count;i++) if(button(names[i],x+4,y+4+i*26,w-8,26,0) && !popup_opened) { pattern_popup=0; if(i<2) undo_redo(i?1:-1); else if(i<6) edit_selection(i-2); else piano_octave(i==6?1:-1); }
        return;
    }
    if(pattern_popup==1 || pattern_popup==9) {
        int file=pattern_popup==1,x=file?8:116,y=34,w=210,h=file?190:34;
        if(IsKeyPressed(KEY_ESCAPE) || (!popup_opened && IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && !hover(x,y,w,h))) { pattern_popup=0; return; }
        ui_frame((Rectangle){x,y,w,h});
        const char *items[]={"New","Demo","Save","Save As...","Open...","Export...","Collect samples and save..."};
        for(int i=0;i<(file?7:1);i++) if(button(file?items[i]:"Keybindings",x+4,y+4+i*26,w-8,24,0) && !popup_opened) {
            if(file) recording_finish();
            pattern_popup=0;
            if(!file) open_popup(4);
            else if(i==0) replace_project(1);
            else if(i==1) replace_project(2);
            else if(i==4) replace_project(3);
            else project_file_action(i);
            return;
        }
        return;
    }
    if(pattern_popup==8) {
        int visible=fminf(10,device_count+2),w=300,h=visible*24+8;
        float scale=ui_scale();
        int x=fmaxf(0,fminf(GetScreenWidth()/scale-w,popup_position[8].x)),y=fmaxf(42,fminf(GetScreenHeight()/scale-24-h,popup_position[8].y));
        if(IsKeyPressed(KEY_ESCAPE) || (!popup_opened && IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && !hover(x,y,w,h))) { pattern_popup=0; return; }
        ui_surface((Rectangle){x,y,w,h},ui_theme.browser);
        DrawRectangleLinesEx((Rectangle){x,y,w,h},1,ui_theme.border);
        if(hover(x,y,w,h)) device_scroll=fmaxf(0,fminf(device_count+2-visible,device_scroll-GetMouseWheelMove()));
        for(int row=0;row<visible;row++) {
            int i=row+device_scroll; const char *name=i==0?"":i==1?"@default":device_names[i-2];
            char *selected=project.audio_io[device_target][device_io];
            if(button_color(fit_text(device_caption(name),w-32,13),x+4,y+4+row*24,w-8,22,!strcmp(name,selected),ui_theme.browser) && !popup_opened) {
                snprintf(selected,128,"%s",name); pattern_popup=0;
                snprintf(status,sizeof status,device_io?"Output device choice saved; external-output routing is not implemented yet":"Input device saved; arm this mixer track and press Record");
            }
        }
        return;
    }
    if(pattern_popup==7) {
        int x=60,y=34,w=304,h=212;
        if(IsKeyPressed(KEY_ESCAPE) || (!popup_opened && IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && !hover(x,y,w,h))) { pattern_popup=0; return; }
        ui_frame((Rectangle){x,y,w,h});
        if(button("MIDI / Recording",x+6,y+180,w-12,25,0) && !popup_opened){midi_refresh();open_popup(13);return;}
        label("Appearance",x+10,y+9,12,muted);
        for(int light=0;light<2;light++) {
            if(button(light?"Light":"Dark",x+6,y+30+light*28,w-12,25,ui_theme.light==light) && !popup_opened) {
                int saved=theme_select(light); pattern_popup=0;
                snprintf(status,sizeof status,saved?"Theme saved":"Theme changed for this session; preferences could not be saved");
            }
        }
        label("Accent color",x+10,y+94,12,muted);
        unsigned selected=(unsigned)accent.r<<16 | (unsigned)accent.g<<8 | accent.b;
        int choice=color_picker(x+8,y+114,w-16,selected);
        if(choice>=0 && !popup_opened) {
            int saved=theme_accent(pattern_palette[choice]); pattern_popup=0;
            snprintf(status,sizeof status,saved?"Accent color saved":"Accent changed for this session; preferences could not be saved");
        }
        return;
    }
    if(pattern_popup==12) {
        float scale=ui_scale(); int w=400,h=152,x=(GetScreenWidth()/scale-w)/2,y=(GetScreenHeight()/scale-h)/2;
        ui_frame((Rectangle){x,y,w,h}); label("Save your changes?",x+16,y+16,16,ink);
        label(fit_text(GetFileName(document.path),w-32,13),x+16,y+48,13,muted);
        if(button("Save",x+16,y+100,100,28,1)) { pattern_popup=0; project_file_action(2); return; }
        if(button("Discard",x+124,y+100,120,28,0)) { int action=document.pending_action; document.pending_action=0; pattern_popup=0; if(action==4) load_project(document.replacement_path); else if(action==3) project_file_action(4); else if(action==REPLACE_QUIT) document.quit=1; else new_project(action==2); return; }
        if(button("Cancel",x+252,y+100,100,28,0) || IsKeyPressed(KEY_ESCAPE)) { document.pending_action=0; pattern_popup=0; }
        return;
    }
    int picker=pattern_popup==11,folder=pattern_popup==3,help=pattern_popup==4,number=pattern_popup==5,w=pattern_popup==13?460:picker?700:pattern_popup==6?320:help?660:number?360:folder?620:260,h=pattern_popup==13?390:picker?(file_confirm?180:500):help?645:116;
    float scale=ui_scale();
    if(help || picker || pattern_popup==13) h=fminf(h,GetScreenHeight()/scale-56);
    Vector2 *position=&popup_position[pattern_popup];
    if(position->x<0 || position->y<0) *position=(Vector2){(GetScreenWidth()/scale-w)/2,(GetScreenHeight()/scale-h)/2};
    if(popup_drag) {
        position->x=mouse.x-popup_offset.x; position->y=mouse.y-popup_offset.y;
        if(!IsMouseButtonDown(MOUSE_BUTTON_LEFT) || IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) popup_drag=0;
    }
    position->x=fmaxf(0,fminf(GetScreenWidth()/scale-w,position->x));
    position->y=fmaxf(0,fminf(GetScreenHeight()/scale-h,position->y));
    int x=position->x,y=position->y;
    ui_frame((Rectangle){x,y,w,h});
    ui_surface((Rectangle){x,y,w,TITLE},ui_theme.title_focus);
    if(button("x",x+w-24,y+1,20,16,0)) { if(pattern_popup==11) document.pending_action=0; pattern_popup=0; popup_drag=0; return; }
    if(hover(x,y,w-26,TITLE)) snprintf(status,sizeof status,"Drag this title bar to move the dialog");
    if(hover(x,y,w-26,TITLE) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) { popup_drag=1; popup_offset=(Vector2){mouse.x-x,mouse.y-y}; }
    if(IsKeyPressed(KEY_ESCAPE)) { if(pattern_popup==11) document.pending_action=0; pattern_popup=0; popup_drag=0; return; }
    if(!popup_drag && !popup_opened && IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && !hover(x,y,w,h)) { if(pattern_popup==11) document.pending_action=0; pattern_popup=0; return; }
    input_enabled=!popup_drag;
    if(picker) { draw_file_picker(x,y,w,h,scale); EndScissorMode(); return; }
    if(pattern_popup==13) {
        label("MIDI / Recording",x+8,y+3,13,ink);
        int enabled=input_enabled;if(recording_active())input_enabled=0;
        const char *modes[]={"Audio","Notes","Automation"};
        for(int i=0;i<3;i++)if(button(modes[i],x+12+i*145,y+30,140,28,record_mask&(1u<<i))){record_mask^=1u<<i;midi_save_settings();}
        int target=midi_target();label(fit_text(TextFormat("Keyboard plays: %s",target>=0 && target<project.channel_count?project.channel_names[target]:"Select an instrument"),w-24,12),x+12,y+68,12,ink);
        label("Select a channel in the Rack or Piano Roll to play it.",x+12,y+88,11,muted);
        if(button("Refresh inputs",x+12,y+112,140,24,0))midi_refresh();
        if(button("Clear MIDI links",x+160,y+112,140,24,0)){project.midi_binding_count=0;memset(project.midi_bindings,0,sizeof project.midi_bindings);midi_learning=0;}
        label(midi_connected?"Input connected":"Select an input (USB or virtual MIDI)",x+12,y+145,11,midi_connected?accent:muted);
        int visible=fmaxf(1,(h-180)/24);if(hover(x+12,y+168,w-24,h-180))midi_scroll=fmaxf(0,fminf(fmaxf(0,midi_device_count+1-visible),midi_scroll-GetMouseWheelMove()));
        for(int row=0;row<visible && row+midi_scroll<=midi_device_count;row++) {
            int i=row+midi_scroll-1;const char *name=i<0?"None":midi_devices[i].name;
            if(button(fit_text(name,w-40,12),x+12,y+168+row*24,w-24,22,i<0?!midi_selected[0]:!strcmp(midi_selected,midi_devices[i].id)))midi_select(i);
        }
        input_enabled=enabled;return;
    }
    if(pattern_popup==6) {
        label("Add instrument",x+8,y+3,13,ink);
        if(button("Sampler",x+12,y+30,w-24,30,0)) { add_instrument(INSTRUMENT_SAMPLER); pattern_popup=0; }
        if(button("FM Synth",x+12,y+66,w-24,30,0)) { add_instrument(INSTRUMENT_FM); pattern_popup=0; }
        return;
    }
    if(help) {
        label("Help - Keybindings",x+8,y+3,13,ink);
        int maximum=767-h;
        if(hover(x,y+TITLE,w,h-TITLE)) help_scroll-=GetMouseWheelMove()*36;
        help_scroll=fmaxf(0,fminf(maximum,help_scroll));
        BeginScissorMode(x*scale,(y+TITLE)*scale,w*scale,(h-TITLE-30)*scale);
        int left=x+18,right=x+348,ly=y+34-help_scroll,ry=ly;
        help_group(left,&ly,"Playlist & Piano Roll navigation");
        help_binding(left,&ly,"Wheel","Scroll vertically");
        help_binding(left,&ly,"Shift + wheel","Scroll horizontally");
#ifdef __APPLE__
        help_binding(left,&ly,"Cmd + wheel","Zoom around pointer");
        help_binding(left,&ly,"Two-finger scroll","Pan in both directions");
        help_binding(left,&ly,"Pinch","Zoom around pointer");
#else
        help_binding(left,&ly,"Ctrl + wheel","Zoom around pointer");
#endif
        help_binding(left,&ly,"Middle drag","Pan freely");
        ly+=12; help_group(left,&ly,"Transport & keyboard audition");
        help_binding(left,&ly,"Space","Play / stop from marker");
        help_binding(left,&ly,"Z row + S/D/G/H/J","Play notes from C3");
        help_binding(left,&ly,"Q row + number keys","Play notes from C4");
        ly+=12; help_group(left,&ly,"Editor windows");
        help_binding(left,&ly,"Drag title bar","Move window");
        help_binding(left,&ly,"Double-click title","Maximize / restore");
        ly+=12; help_group(left,&ly,"Playlist tools");
        help_binding(left,&ly,"Cut: click clip","Split at snapped position");
        help_binding(left,&ly,"Stretch: drag edge","Resample / stretch audio");
#ifdef __APPLE__
        help_binding(left,&ly,"Cmd + Z / Shift + Z","Undo / redo");
        help_binding(left,&ly,"Cmd + S / O","Save / open (Shift+S: As)");
#else
        help_binding(left,&ly,"Ctrl + Z / Shift + Z","Undo / redo (also Ctrl + Y)");
        help_binding(left,&ly,"Ctrl + S / O","Save / open (Shift+S: As)");
#endif
        ly+=12; help_group(left,&ly,"Piano Roll velocity lane");
        help_binding(left,&ly,"Left-drag","Paint note velocities");
        help_binding(left,&ly,"Right-drag","Draw a straight velocity ramp");
        ly+=12; help_group(left,&ly,"MIDI & recording");
        help_binding(left,&ly,"Record type icons","Audio / Notes / Automation");
        help_binding(left,&ly,"Right-click Record","Choose MIDI input / settings");
        help_binding(left,&ly,"Control right-click","MIDI Learn: move a CC knob");
        help_group(right,&ry,"Editing & selection");
#ifdef __APPLE__
        help_binding(right,&ry,"Cmd + drag","Temporarily box-select");
#else
        help_binding(right,&ry,"Ctrl + drag","Temporarily box-select");
#endif
        help_binding(right,&ry,"Shift-click","Add / remove selection");
        help_binding(right,&ry,"Shift-drag empty","Add selection rectangle");
        help_binding(right,&ry,"Shift + Left/Right","Move notes one step");
        help_binding(right,&ry,"Shift + Up/Down","Move notes one semitone");
#ifdef __APPLE__
        help_binding(right,&ry,"Cmd + C / X","Copy / cut selection");
        help_binding(right,&ry,"Cmd + V","Paste at start marker");
        help_binding(right,&ry,"Cmd + A","Select all notes / clips");
        help_binding(right,&ry,"Cmd + Up/Down (K/J)","Shift notes an octave");
#else
        help_binding(right,&ry,"Ctrl + C / X","Copy / cut selection");
        help_binding(right,&ry,"Ctrl + V","Paste at start marker");
        help_binding(right,&ry,"Ctrl + A","Select all notes / clips");
        help_binding(right,&ry,"Ctrl + Up/Down (K/J)","Shift notes an octave");
#endif
        help_binding(right,&ry,"Delete","Delete selected clips / notes");
        help_binding(right,&ry,"Right-drag","Erase clips / notes");
        ry+=12; help_group(right,&ry,"Automation");
        help_binding(right,&ry,"Right-click control","Create automation");
        help_binding(right,&ry,"Click / drag curve","Add / move a point");
        help_binding(right,&ry,"Right-click point","Delete point");
        ry+=12; help_group(right,&ry,"Text fields & dialogs");
#ifdef __APPLE__
        help_binding(right,&ry,"Cmd + A / V","Select all / paste");
#else
        help_binding(right,&ry,"Ctrl + A / V","Select all / paste");
#endif
        help_binding(right,&ry,"Enter / Escape","Apply / cancel");
        help_binding(right,&ry,"Backspace","Delete text");
        ry+=12; help_group(right,&ry,"Browser (hover or focus)");
        help_binding(right,&ry,"Up/Down or K/J","Select & preview");
        help_binding(right,&ry,"Left or H","Collapse / parent folder");
        help_binding(right,&ry,"Right or L","Expand / child folder");
        EndScissorMode();
        label(maximum>0?"Wheel to scroll keybindings":"Navigation follows the pointer. Audition requires Keys enabled.",x+18,y+h-24,11,muted);
        return;
    }
    char *text=number?number_text:folder?folder_text:rename_text; size_t capacity=number?sizeof number_text:folder?sizeof folder_text:sizeof rename_text;
    label(number?number_name:folder?"Add Browser folder":(rename_automation>=0?"Rename automation":rename_channel>=0?TextFormat("Rename channel %d",rename_channel+1):rename_mixer>=0?TextFormat("Rename insert %d",rename_mixer+1):rename_track>=0?TextFormat("Rename track %d",rename_track+1):TextFormat("Rename pattern %d",rename_pattern+1)),x+8,y+3,13,ink);
    DrawRectangle(x+8,y+32,w-16,28,bg); DrawRectangleLines(x+8,y+32,w-16,28,cell);
    if(rename_select_all) DrawRectangle(x+12,y+38,fminf(w-26,text_width(text,13)),16,cell);
    if(hover(x+8,y+32,w-16,28)) snprintf(status,sizeof status,number?"Enter an exact value using a decimal dot or comma":folder?"Enter the folder path to add as a Browser root":"Enter a name");
    if(hover(x+8,y+32,w-16,28) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) rename_select_all=0;
    text_input(text,capacity);
    label(fit_text(text,w-26,13),x+12,y+40,13,ink);
    label(number?TextFormat("%.6g to %.6g; decimal dot or comma",number_low,number_high):"Choose Apply or Cancel below",x+8,y+66,11,muted);
    int apply=button("Apply",x+8,y+84,76,24,1) || IsKeyPressed(KEY_ENTER);
    if(button("Cancel",x+92,y+84,76,24,0)) pattern_popup=0;
    if(number && button("Reset",x+176,y+84,76,24,0)) { *number_target=number_default; pattern_popup=0; }
    if(apply) {
        if(number) {
            char value[64],*end; snprintf(value,sizeof value,"%s",text);
            for(char *c=value;*c;c++) if(*c==',') *c='.';
            float parsed=strtof(value,&end); int parsed_any=end!=value; while(isspace((unsigned char)*end)) end++;
            if(!parsed_any || *end || !isfinite(parsed) || parsed<number_low || parsed>number_high || (number_integer && parsed!=roundf(parsed))) { snprintf(status,sizeof status,"Enter a number from %.6g to %.6g.",number_low,number_high); return; }
            *number_target=parsed;
        } else if(folder) {
            if(!browser_add(&browser,text)) { snprintf(status,sizeof status,"Cannot add folder: check its path, permissions or the eight-folder limit."); return; }
            snprintf(status,sizeof status,"Browser folder saved.");
        } else {
            size_t n=strlen(text); while(n && text[n-1]==' ') text[--n]=0;
            char *name=text; while(*name==' ') name++;
            if(!*name) { snprintf(status,sizeof status,"Name cannot be empty."); return; }
            snprintf(rename_automation>=0?project.automations[rename_automation].name:rename_channel>=0?project.channel_names[rename_channel]:rename_mixer>=0?project.insert_names[rename_mixer]:rename_track>=0?project.track_names[rename_track]:project.pattern_names[rename_pattern],PATTERN_NAME,"%s",name);
        }
        pattern_popup=0;
    }
}
static int import_sample(const char *path,int c) {
    recording_finish();
    char full[PATH_MAX]; Sample s;
    if(!realpath(path,full) || strlen(full)>=sizeof project.paths[c] || !sample_load(full,&s)) { snprintf(status,sizeof status,"Cannot load sample: unreadable, unsupported, too large or insufficient memory."); return 0; }
    Sample cooked;
    if(!sample_process(s,project.sampler[c],&cooked)) { sample_free(s); snprintf(status,sizeof status,"Sample processing failed; channel unchanged"); return 0; }
    if(project.instrument[c]==INSTRUMENT_FM) {
        for(int a=project.automation_count-1;a>=0;a--) if(project.automations[a].target.owner==(unsigned)c && ((project.automations[a].target.parameter>=PARAM_FM_RATIO && project.automations[a].target.parameter<=PARAM_FM_LAST) || (project.automations[a].target.parameter>=PARAM_DX7_FIRST && project.automations[a].target.parameter<=PARAM_DX7_LAST))) automation_delete(&project,a);
        project.instrument[c]=INSTRUMENT_SAMPLER;
    }
    Sample old=samples[c],raw=originals[c]; audio_sample(c,cooked); samples[c]=cooked; originals[c]=s; sample_free(old); sample_free(raw);
    sample_generation[c]++; sampler_generation[c]=sample_generation[c]; sampler_applied[c]=project.sampler[c];
    sample_duration(&project,c,samples[c],originals[c]);
    snprintf(project.paths[c],sizeof project.paths[c],"%s",full);
    memset(project.channel_names[c],0,PATTERN_NAME); snprintf(project.channel_names[c],PATTERN_NAME,"%s",GetFileNameWithoutExt(full));
    snprintf(status,sizeof status,"Loaded %.100s into %s",GetFileName(path),project.channel_names[c]); return 1;
}
static void audition_entry(int entry) {
    browser_select(&browser,entry);
    if(entry<0 || entry>=browser.items) return;
    const char *path=browser.nodes[entry].path;
    if(!IsFileExtension(path,".wav;.flac;.mp3") || DirectoryExists(path)) return;
    Sample next;
    if(!sample_load(path,&next)) { snprintf(status,sizeof status,"Cannot preview %.120s",GetFileName(path)); return; }
    audio_preview(next); sample_free(audition); audition=next;
    snprintf(audition_path,sizeof audition_path,"%s",path);
    waveform_build(&audition_wave,next);
    snprintf(status,sizeof status,"Preview: %.140s",GetFileName(path));
}
static void rack_reveal_last(void) {
    Editor *e=&windows.editors[0];
    int count=rack_channels(NULL);
    e->rect.h=fminf(GetScreenHeight()/ui_scale()-66,RACK_TOP+48+count*28);
    rack_scroll=fmaxf(0,count-(int)((e->rect.h-RACK_TOP-48)/28));
}
static void add_instrument(int type) {
    int c=project.channel_count;
    if(c>=CHANNELS) { snprintf(status,sizeof status,"Channel Rack limit: %d channels.",CHANNELS); return; }
    project.instrument[c]=type; project.fm[c]=fm_default();
    project.channel_pitch[c]=0; project.pitch_range[c]=2; project.channel_audio[c]=0; project.audio_seconds[c]=0; rack_filter=2; project.channel_count++; project.sampler[c]=(Sampler){.time=1,.length=1}; project.volume[c]=1; project.pan[c]=0; project.mute[c]=0; project.route[c]=0;
    snprintf(project.paths[c],sizeof project.paths[c],"%s",SAMPLE_EMPTY);
    memset(project.channel_names[c],0,PATTERN_NAME); snprintf(project.channel_names[c],PATTERN_NAME,"%s",type==INSTRUMENT_FM?"FM Synth":"Sampler");
    for(int pat=0;pat<PATTERNS;pat++) {
        memset(project.notes[pat][c],0,sizeof project.notes[pat][c]); memset(note_selected[pat][c],0,NOTES); piano_channels[pat][c]=0;
    }
    Sample old=samples[c],raw=originals[c]; originals[c]=samples[c]=(Sample){0}; audio_channels(&project,samples); sample_free(old); sample_free(raw);
    sample_generation[c]++; sampler_generation[c]=sample_generation[c]; sampler_applied[c]=project.sampler[c];
    channel=instrument_channel=c; browser_focus=0; rack_reveal_last(); windows_focus(&windows,4);
}
static void replace_instrument(int c,int type) {
    if(recording_ui.active) { snprintf(status,sizeof status,"Finish recording before replacing an instrument."); return; }
    if(!sampler_flush()) { snprintf(status,sizeof status,"Could not prepare the current sample; instrument unchanged."); return; }
    if(!channel_replace_instrument(&project,c,type)) {
        snprintf(status,sizeof status,"Audio clip channels use a Sampler. Replace an instrument in a pattern to use FM Synth."); return;
    }
    automation_selected=automation_node=-1; control_drag=NULL; menu_value=NULL;
    if(!strcmp(project.channel_names[c],"Sampler") || !strcmp(project.channel_names[c],"FM Synth"))
        snprintf(project.channel_names[c],PATTERN_NAME,"%s",type==INSTRUMENT_FM?"FM Synth":"Sampler");
    channel=instrument_channel=c; browser_focus=0; windows_focus(&windows,4);
    snprintf(status,sizeof status,"%s now uses %s; notes and routing kept across all patterns.",project.channel_names[c],type==INSTRUMENT_FM?"FM Synth":"Sampler");
}
static int pattern_instruments(int pat,int rows[CHANNELS]) {
    int count=0;
    if(pat<0 || pat>=project.pattern_count) return 0;
    for(int c=0;c<project.channel_count;c++) for(int n=0;n<NOTES;n++) if(project.notes[pat][c][n].velocity) { rows[count++]=c; break; }
    if(!count && channel>=0 && channel<project.channel_count) rows[count++]=channel;
    return count;
}
static Color source_rgb(uint32_t rgb) {
    Color color={rgb>>16,(rgb>>8)&255,rgb&255,255};
    return color;
}
/* Clip artwork follows its fill, independently of the surrounding theme. */
static Color clip_foreground(Color fill) {
    float channels[]={fill.r/255.f,fill.g/255.f,fill.b/255.f};
    for(int i=0;i<3;i++) channels[i]=channels[i]<=.04045f?channels[i]/12.92f:powf((channels[i]+.055f)/1.055f,2.4f);
    return .2126f*channels[0]+.7152f*channels[1]+.0722f*channels[2]>.179f?(Color){20,25,28,255}:(Color){248,249,250,255};
}
static Color audio_color(int c) {
    if(project.channel_colors[c]) return source_rgb(project.channel_colors[c]);
    float t=fminf(1,log1pf(samples[c].frames/(float)RATE)/log1pf(30));
    Color short_color={142,73,78,255},long_color={72,137,91,255};
    Color color={(unsigned char)(short_color.r+(long_color.r-short_color.r)*t),(unsigned char)(short_color.g+(long_color.g-short_color.g)*t),(unsigned char)(short_color.b+(long_color.b-short_color.b)*t),255};
    return source_rgb((uint32_t)color.r<<16 | (uint32_t)color.g<<8 | color.b);
}
typedef struct {
    Waveform wave;
    Sample crop;
    Sampler settings;
    unsigned offset,generation,epoch;
    int valid;
    float gain;
} SamplerView;
static SamplerView sampler_views[CHANNELS];
static SamplerView *sampler_view(int c) {
    SamplerView *view=&sampler_views[c]; Sample source=originals[c]; Sampler settings=project.sampler[c];
    int changed=!view->valid || view->generation!=sample_generation[c] || view->epoch!=sample_epoch;
    if(changed) waveform_build(&view->wave,source);
    if(changed || !sampler_processing_equal(view->settings,settings)) {
        unsigned offset=llround(source.frames*(double)settings.start);
        Sample crop={.data=source.data?source.data+(size_t)offset*sample_channels(source):NULL,.frames=llround((source.frames-offset)*(double)settings.length),.channels=source.channels};
        view->crop=sample_trim(crop,settings.trim);
        view->offset=source.data?(view->crop.data-source.data)/sample_channels(source):0;
        view->settings=settings; view->generation=sample_generation[c]; view->epoch=sample_epoch; view->valid=1;
        WavePeak peak=waveform_range(&view->wave,source,view->offset,view->offset+view->crop.frames);
        float maximum=fmaxf(-peak.low,peak.high);
        view->gain=(settings.flags&SAMPLE_NORMALIZE) && maximum>0?1/maximum:1;
    }
    return view;
}
static int sampler_live_preview(int c) {
    Sampler current=project.sampler[c],applied=sampler_applied[c];
    return !recording_source(PATTERNS+c) && !sampler_processing_equal(current,applied) && current.pitch==applied.pitch && current.stretch==applied.stretch;
}
static unsigned sampler_view_frames(int c) {
    unsigned frames=sampler_view(c)->crop.frames;
    return frames?fmax(1,llround(frames*(double)project.sampler[c].time)):0;
}
static WavePeak sampler_view_envelope(int c,double position,double width,unsigned frames) {
    SamplerView *view=sampler_view(c); unsigned count=view->crop.frames;
    double ratio=frames?count/(double)frames:0;
    position*=ratio; width*=ratio;
    if(view->settings.flags&SAMPLE_REVERSE) position=count-position;
    WavePeak peak=waveform_envelope_region(&view->wave,originals[c],view->offset,count,position,width);
    if(view->settings.flags&SAMPLE_POLARITY) { float low=peak.low; peak.low=-peak.high; peak.high=-low; }
    peak.low*=view->gain; peak.high*=view->gain; return peak;
}
static float audio_view_steps(int c) {
    float seconds=sampler_live_preview(c)?sampler_view_frames(c)/(float)RATE:project.audio_seconds[c];
    return seconds*audio_source_bpm(&project,c)/15/channel_speed(&project,c);
}
static float playlist_clip_length(int lane,int clip) {
    int c=project.clips[lane][clip]-PATTERNS-1;
    if(c>=0 && c<CHANNELS && !project.clip_steps[lane][clip] && sampler_live_preview(c)) {
        double remaining=fmax(0,sampler_view_frames(c)/(double)RATE-project.clip_offsets[lane][clip])*audio_source_bpm(&project,c)/15;
        AudioTimeline map; audio_timeline_init(&map,&project,c);
        return audio_timeline_duration(&map,project.clip_starts[lane][clip]*STEPS,remaining);
    }
    return clip_length(&project,lane,clip);
}
static Waveform audio_waves[CHANNELS];
static struct { Sample sample; Sampler settings; unsigned generation,epoch,revision; int valid; } audio_wave_states[CHANNELS];
static Waveform *processed_waveform(int c) {
    Sample sample=samples[c];
    if(!audio_wave_states[c].valid || audio_wave_states[c].sample.data!=sample.data || audio_wave_states[c].sample.frames!=sample.frames ||
       audio_wave_states[c].sample.channels!=sample.channels || audio_wave_states[c].generation!=sample_generation[c] ||
       audio_wave_states[c].epoch!=sample_epoch || !sampler_processing_equal(audio_wave_states[c].settings,sampler_applied[c])) {
        waveform_build(&audio_waves[c],sample);
        audio_wave_states[c].sample=sample; audio_wave_states[c].settings=sampler_applied[c];
        audio_wave_states[c].generation=sample_generation[c]; audio_wave_states[c].epoch=sample_epoch;
        audio_wave_states[c].revision++; audio_wave_states[c].valid=1;
    }
    return &audio_waves[c];
}
typedef struct {
    Sample sample; Waveform *wave;
    int channel,preview;
    const AudioTimeline *timeline;
    double start,offset,frames_per_step;
    float origin,width,pixels_per_step;
} WaveDisplay;
static double wave_source(const WaveDisplay *view,float x) {
    if(view->timeline) return view->offset+audio_timeline_source(view->timeline,view->start,
        view->start+fmax(0,x-view->origin)/view->pixels_per_step)*view->frames_per_step;
    return (x-view->origin)/view->width*view->sample.frames;
}
static void wave_quad(float x,float xx,float top,float next_top,float bottom,float next_bottom,float v,float vv) {
    rlTexCoord2f(.5f,v); rlVertex2f(x,top); rlTexCoord2f(.5f,vv); rlVertex2f(x,bottom);
    rlTexCoord2f(.5f,vv); rlVertex2f(xx,next_bottom); rlTexCoord2f(.5f,v); rlVertex2f(xx,next_top);
}
/* A continuous filled envelope with one physical pixel of edge antialiasing. */
static void draw_waveform(WaveDisplay view,Rectangle area,float left,float right,Color color) {
    if(!view.sample.frames || view.width<=0 || right<=left || area.height<=0) return;
    float step=1/ui_scale(),feather=.5f/font_scale,mid=area.y+area.height*.5f,amplitude=fmaxf(0,area.height*.5f-2/font_scale);
    float previous_x=left,previous_top=mid,previous_bottom=mid;
    int columns=ceilf((right-left)/step);
    rlSetTexture(cable_texture.id); rlBegin(RL_QUADS); rlColor4ub(color.r,color.g,color.b,color.a);
    for(int i=0;i<=columns;i++) {
        float x=fminf(right,left+i*step);
        double source=wave_source(&view,x),width=fabs(wave_source(&view,x+step)-source);
        WavePeak peak;
        if(view.preview) peak=sampler_view_envelope(view.channel,source,width,view.sample.frames);
        else peak=waveform_envelope(view.wave,view.sample,source,width);
        float top=mid-fmaxf(0,peak.high)*amplitude,bottom=mid-fminf(0,peak.low)*amplitude;
        top=fminf(top,mid-feather); bottom=fmaxf(bottom,mid+feather);
        if(i) {
            wave_quad(previous_x,x,previous_top-feather,top-feather,previous_top+feather,top+feather,0,.25f);
            wave_quad(previous_x,x,previous_top+feather,top+feather,previous_bottom-feather,bottom-feather,.5f,.5f);
            wave_quad(previous_x,x,previous_bottom-feather,bottom-feather,previous_bottom+feather,bottom+feather,.75f,1);
        }
        previous_x=x; previous_top=top; previous_bottom=bottom;
    }
    rlEnd(); rlSetTexture(0);
}
static void audio_waveform_at(int c,float x,float y,float left,float right,float pixels_per_step,float height,int lane,int clip) {
    Sample sample=samples[c]; Waveform *wave=&audio_waves[c]; int recording=0,preview=sampler_live_preview(c);
    if(preview) sample.frames=sampler_view_frames(c);
    if(recording_ui.active) for(int i=0;i<recording_ui.count;i++) if(recording_ui.takes[i].channel==c) {
        sample=recording_ui.takes[i].sample; wave=&recording_ui.takes[i].wave; recording=1; preview=0; break;
    }
    if(recording) lane=-1;
    if(!sample.frames) return;
    if(!recording && !preview) wave=processed_waveform(c);
    AudioTimeline map; double start=0,offset=0;
    float full=sample.frames/(float)RATE*audio_source_bpm(&project,c)/15/channel_speed(&project,c)*pixels_per_step;
    if(lane>=0) {
        audio_timeline_init(&map,&project,c); start=project.clip_starts[lane][clip]*STEPS; offset=project.clip_offsets[lane][clip]*RATE;
        full=audio_timeline_duration(&map,start,fmax(0,sample.frames-offset)/RATE*audio_source_bpm(&project,c)/15)*pixels_per_step;
    }
    WaveDisplay view={.sample=sample,.wave=wave,.channel=c,.preview=preview,.timeline=lane>=0?&map:NULL,
        .start=start,.offset=offset,.frames_per_step=RATE*15/audio_source_bpm(&project,c),.origin=x,.width=full,.pixels_per_step=pixels_per_step};
    draw_waveform(view,(Rectangle){x,y+14,full,height-14},left,fminf(right,x+full),clip_foreground(audio_color(c)));
}
static void audio_waveform(int c,float x,float y,float left,float right,float pixels_per_step,float height) {
    audio_waveform_at(c,x,y-14,left,right,pixels_per_step,height+14,-1,-1);
}
static void drop_sample(const char *path) {
    int target=windows_hit(&windows,mouse.x,mouse.y);
    if(target==1) {
        Rect r=windows.editors[1].rect;
        float gx=r.x+212,gy=r.y+PLAYLIST_GRID_TOP,gridw=r.w-236;
        int list=mouse.x>=r.x+4 && mouse.x<r.x+116 && mouse.y>=gy && mouse.y<r.y+r.h-66;
        if(!list && (mouse.x<gx || mouse.x>=gx+gridw || mouse.y<gy || mouse.y>=r.y+r.h-PLAYLIST_BOTTOM)) { snprintf(status,sizeof status,"Drop audio onto the Playlist grid or Audio list."); return; }
        if(!list) {
            float pixels=gridw/BARS*arrangement.zoom;
            float at=arrangement.view_start+(mouse.x-gx)/pixels;
            int lane=track_at(track_scroll+mouse.y-gy),hit=arrangement_hit(&project,lane,at);
            if(hit>=0 && AUDIO_SOURCE(project.clips[lane][hit]-1)) {
                int c=project.clips[lane][hit]-PATTERNS-1;
                if(import_sample(path,c)) {
                    channel=instrument_channel=c; browser_focus=0;
                    arrangement.source_pattern=PATTERNS+c; arrangement.source_steps=project.audio_seconds[c]; arrangement.source_offset=0;
                    memset(arrangement.selected,0,sizeof arrangement.selected); arrangement.selected[lane][hit]=1;
                    audio_channels(&project,samples); windows_focus(&windows,1);
                    snprintf(status,sizeof status,"Replaced sample with %.100s",project.channel_names[c]);
                }
                return;
            }
        }
        int c=project.channel_count;
        if(c>=CHANNELS) { snprintf(status,sizeof status,"Channel Rack limit: %d channels.",CHANNELS); return; }
        float pixels=gridw/BARS*arrangement.zoom,q=grid_interval(pixels/STEPS);
        float start=snap_floor((arrangement.view_start+(mouse.x-gx)/pixels)*STEPS,q)/STEPS;
        int lane=track_at(track_scroll+mouse.y-gy);
        int slot=-1;
        if(!list) {
            /* Check placement before decoding or creating a channel. */
            Project next=project; next.channel_count++; next.channel_audio[c]=1; next.audio_seconds[c]=.001f;
            slot=arrangement_place(&next,lane,start,PATTERNS+c,.001f*project.bpm/15);
            if(slot<0) { snprintf(status,sizeof status,"No space here: choose an empty part of the track."); return; }
        }
        if(!import_sample(path,c)) return;
        if(!list) {
            float length=samples[c].frames/(float)RATE*project.bpm/15;
            slot=arrangement_place(&project,lane,start,PATTERNS+c,length);
            if(slot<0) { /* Discard the import without adding a channel. */
                Sample empty={0}; audio_sample(c,empty); sample_free(samples[c]); sample_free(originals[c]); samples[c]=originals[c]=empty;
                project.paths[c][0]=0; snprintf(status,sizeof status,"Audio overlaps another clip: choose an empty part of the track."); return;
            }
        }
        uint32_t colors[CHANNELS]; int count=0;
        for(int i=0;i<project.channel_count;i++) if(project.channel_audio[i]) colors[count++]=project.channel_colors[i];
        project.channel_colors[c]=next_source_color(colors,count);
        project.channel_audio[c]=1; project.channel_count++;
        channel=instrument_channel=c; rack_filter=1; rack_scroll=0;
        arrangement.source_pattern=PATTERNS+c; arrangement.source_steps=project.audio_seconds[c]; arrangement.source_offset=0;
        if(!list) { memset(arrangement.selected,0,sizeof arrangement.selected); arrangement.selected[lane][slot]=1; }
        picker_tab=1; picker_scroll=count;
        browser_focus=0; audio_channels(&project,samples); windows_focus(&windows,1);
        snprintf(status,sizeof status,list?"Imported %.100s into the Audio list; drag it to the Playlist when ready.":"Placed %.100s; available in the Audio list and Rack Audio group.",project.channel_names[c]); return;
    }
    if(target==4 && mouse.y>=windows.editors[4].rect.y+TITLE) {
        if(import_sample(path,instrument_channel)) {
            channel=instrument_channel; browser_focus=0; audio_channels(&project,samples); windows_focus(&windows,4);
        }
        return;
    }
    Rect r=windows.editors[0].rect;
    if(windows_hit(&windows,mouse.x,mouse.y)!=0 || mouse.y<r.y+TITLE) {
        snprintf(status,sizeof status,"Drop onto the Playlist grid, a sampler, a Rack row, or empty Rack space."); return;
    }
    int c=rack_channel_at(rack_scroll+(mouse.y-r.y-RACK_TOP)/28);
    int visible=fmaxf(1,(r.h-RACK_TOP-48)/28);
    int replace=mouse.y>=r.y+RACK_TOP && mouse.y<r.y+RACK_TOP+visible*28 && c>=0 && c<project.channel_count;
    if(!replace) c=project.channel_count;
    if(c>=CHANNELS) { snprintf(status,sizeof status,"Channel Rack limit: %d channels.",CHANNELS); return; }
    if(import_sample(path,c)) {
        if(!replace) {
            project.channel_audio[c]=0; project.channel_count++; rack_filter=2;
            rack_reveal_last();
        }
        channel=c; browser_focus=0; windows_focus(&windows,0); audio_channels(&project,samples);
    }
}
static void delete_channel(int c) {
    if(edit_clipboard.kind==COPY_CLIPS) edit_clipboard.kind=COPY_EMPTY;
    recording_finish();
    automation_selected=automation_node=-1; picker_drag=-1;
    Sample old=samples[c],raw=originals[c];
    if(!channel_delete(&project,c)) return;
    sample_epoch++;
    for(int i=c;i<project.channel_count;i++) {
        samples[i]=samples[i+1]; originals[i]=originals[i+1]; sampler_applied[i]=sampler_applied[i+1];
        sample_generation[i]=sample_generation[i+1]; sampler_generation[i]=sampler_generation[i+1];
    }
    originals[project.channel_count]=(Sample){0};
    samples[project.channel_count]=(Sample){0};
    for(int pat=0;pat<PATTERNS;pat++) {
        memmove(note_selected[pat][c],note_selected[pat][c+1],(project.channel_count-c)*NOTES); memset(note_selected[pat][project.channel_count],0,NOTES);
        memmove(&piano_channels[pat][c],&piano_channels[pat][c+1],project.channel_count-c);
        piano_channels[pat][project.channel_count]=0;
    }
    channel=fmaxf(0,fminf(channel-(channel>c),project.channel_count-1));
    piano_channel=fmaxf(0,fminf(piano_channel-(piano_channel>c),project.channel_count-1));
    instrument_channel=fmaxf(0,fminf(instrument_channel-(instrument_channel>c),project.channel_count-1));
    arrangement.gesture=IDLE; arrangement.source_pattern=-1; memset(arrangement.selected,0,sizeof arrangement.selected);
    audio_channels(&project,samples); sample_free(old); sample_free(raw); reset=1;
}
static void install_project(Project next,Sample fresh[CHANNELS],Sample processed[CHANNELS],const char *filename) {
    recording_finish();midi_panic();midi_learning=0;
    sample_epoch++; audio_stop(); fm_graph_drag=-1;
    sample_free(audition); free(audition_wave.tree); audition_wave=(Waveform){0}; audition=(Sample){0}; audition_path[0]=0; browser_progress=-1;
    memset(keyboard_notes,0,sizeof keyboard_notes); piano_note_length=2; if(edit_clipboard.kind==COPY_CLIPS) edit_clipboard.kind=COPY_EMPTY;
    picker_tab=picker_scroll=0; picker_drag=-1; automation_selected=automation_node=automation_lane=automation_clip=-1; sample_drag[0]=0;
    rack_hdrag=rack_vdrag=playlist_pan=playlist_vpan=piano_scroll_drag=piano_vdrag=mixer_pan=0;
    piano_scroll_remainder=0; track_zoom=piano_zoom=1; row_zoom_drag=-1; memset(track_heights,0,sizeof track_heights);
    navigation_active=0; navigation_drag=-1; track_resize=-1; selected_track=-1; ruler_loop_drag=0; stop_armed=0; ruler_last_id=-1;
    visual_step=0;
    playing=0; pattern=(int)fminf(pattern,next.pattern_count-1); audio_update(&next,0,song,pattern,1,output_volume,0,0,0);
    for(int c=0;c<CHANNELS;c++) sample_duration(&next,c,processed[c],fresh[c]);
    audio_channels(&next,processed);
    for(int c=0;c<CHANNELS;c++) {
        Sample old=samples[c],raw=originals[c]; samples[c]=processed[c]; originals[c]=fresh[c]; sample_free(old); sample_free(raw);
        sample_generation[c]++; sampler_generation[c]=sample_generation[c]; sampler_applied[c]=next.sampler[c];
    }
    project=next; project_document_saved(&document,&project,filename);
    project.insert_count=INSERTS; rack_scroll=0; rack_filter=0; channel=fmaxf(0,fminf(channel,project.channel_count-1));
    piano_channel=instrument_channel=channel;
    for(int i=0;i<PATTERNS;i++) { piano_span[i]=fmaxf(STEPS,next.pattern_steps[i]); piano_pan[i]=0; piano_range[i]=0; }
    memset(note_selected,0,sizeof note_selected); piano_gesture=PIANO_IDLE; playlist_start=0; memset(piano_start,0,sizeof piano_start); memset(song_loop,0,sizeof song_loop); memset(pattern_loop,0,sizeof pattern_loop); marker_drag=-1;
    memset(piano_channels,0,sizeof piano_channels); mixer_selected=project.route[channel]; mixer_scroll=0;
    piano_key_drag=0; piano_key=-1;
    note_drag=NULL; velocity_drag=-1; control_drag=NULL; route_drag=-1; rack_paint=-1; cable_drag=-1;
    memset(&arrangement,0,sizeof arrangement); arrangement.source_pattern=-1; arrangement.source_steps=STEPS; arrangement.zoom=1; playlist_pan=0; playlist_vpan=0; track_scroll=0; memset(rack_view,0,sizeof rack_view); memset(rack_range,0,sizeof rack_range);
    project_document_saved(&document,&project,NULL);
    history_clear(&edit_history); history_checkpoint();
    snprintf(status,sizeof status,"Loaded %.150s",filename); reset=1;
}
static void new_project(int demo) {
    Project next; Sample fresh[CHANNELS]={0},processed[CHANNELS]={0};
    if(demo) { project_demo(&next); samples_default(fresh); }
    else project_new(&next);
    for(int c=0;c<CHANNELS;c++) {
        if((demo && c<4 && !fresh[c].data) || !sample_process(fresh[c],next.sampler[c],&processed[c])) {
            for(int i=0;i<CHANNELS;i++) { sample_free(fresh[i]); sample_free(processed[i]); }
            snprintf(status,sizeof status,"Project unchanged: could not prepare samples."); return;
        }
    }
    pattern=channel=0; song=demo; install_project(next,fresh,processed,demo?"demo.hbt":"project.hbt"); document.saved_on_disk=0;
    snprintf(status,sizeof status,demo?"Demo loaded: press Play to hear the eight-bar groove.":"New project: load a sample to get started.");
}
static int load_project(const char *path) {
    Project next; Sample fresh[CHANNELS]={0},processed[CHANNELS]={0}; int missing=0;
    if(strlen(path)>=sizeof document.path || !project_load_assets(path,&next,fresh,processed,sample_load,&missing)) {
        snprintf(status,sizeof status,"Project unchanged: invalid project, sample path too long or processing failed: %.100s",path); return 0;
    }
    char filename[PATH_MAX]; snprintf(filename,sizeof filename,"%s",path);
    install_project(next,fresh,processed,filename);
    if(missing) snprintf(status,sizeof status,"Loaded with %d missing samples. Right-click a channel and choose Relink sample, or drop its file onto the channel.",missing);
    return 1;
}
static void replace_project(int action) {
    recording_finish();
    if(project_document_request(&document,&project,action)) { open_popup(12); return; }
    if(action==4) load_project(document.replacement_path); else if(action==3) project_file_action(4); else if(action==REPLACE_QUIT) document.quit=1; else new_project(action==2);
}
static void request_load_project(const char *path) {
    if(strlen(path)>=sizeof document.replacement_path) { snprintf(status,sizeof status,"Project path is too long."); return; }
    snprintf(document.replacement_path,sizeof document.replacement_path,"%s",path); replace_project(4);
}
static void replacement_saved(void) {
    project_document_saved(&document,&project,NULL);
    int action=document.pending_action; document.pending_action=0;
    if(action==4) load_project(document.replacement_path); else if(action==3) project_file_action(4); else if(action==REPLACE_QUIT) document.quit=1; else if(action) new_project(action==2);
}
static void file_picker_sync(void) {
    snprintf(file_directory,sizeof file_directory,"%s",file_picker.directory);
    file_last_row=-1; file_last_click=-1; file_scroll_drag=0;
}
static void preset_action(int kind,int owner,int slot,int save) {
    recording_finish();
    preset_kind=kind; preset_owner=owner; preset_slot=slot;
    char initial[PATH_MAX],directory[PATH_MAX];
    if(!preset_directory(directory,sizeof directory,kind)) snprintf(directory,sizeof directory,"%s",GetWorkingDirectory());
    const char *name=kind==PRESET_FM?"FM Synth":kind==PRESET_CHORUS?"Chorus":kind==PRESET_EQ?"Equalizer":"Sampler";
    if(snprintf(initial,sizeof initial,"%s/%s.llpreset",directory,name)>=(int)sizeof initial || !file_chooser_begin_recent(&file_picker,initial,"llpreset",save,save?FILE_PRESET_SAVE:FILE_PRESET_OPEN)) {
        snprintf(status,sizeof status,"Cannot open preset chooser"); return;
    }
    file_action=save?8:9; file_field=save?2:0; file_confirm=0;
    file_picker_sync(); rename_select_all=save; open_popup(11);
}
static int preset_commit(const char *chosen,int save) {
    DevicePreset preset={.kind=preset_kind}; int owner=preset_owner,slot=preset_slot;
    if(preset_kind==PRESET_CHORUS || preset_kind==PRESET_EQ) {
        if(owner<0 || owner>project.insert_count || slot<0 || slot>=EFFECT_SLOTS || project.effect_type[owner][slot]!=(preset_kind==PRESET_EQ?EFFECT_EQ:EFFECT_CHORUS)) return 0;
        preset.eq=project.eq[owner][slot]; preset.chorus=project.chorus[owner][slot]; preset.mix=project.effect_mix[owner][slot];
    } else {
        if(owner<0 || owner>=project.channel_count || (preset_kind==PRESET_FM)!=(project.instrument[owner]==INSTRUMENT_FM)) return 0;
        preset.fm=project.fm[owner]; preset.sampler=project.sampler[owner];
        snprintf(preset.sample_path,sizeof preset.sample_path,"%s",project.paths[owner]);
    }
    if(save) {
        int ok=preset_save(chosen,&preset); snprintf(status,sizeof status,ok?"Preset saved: %.180s":"Cannot save preset: %.180s",chosen); return ok;
    }
    if(!preset_load(chosen,&preset) || preset.kind!=preset_kind) { snprintf(status,sizeof status,"Invalid preset or wrong device type"); return 0; }
    if(preset_kind==PRESET_FM) project.fm[owner]=preset.fm;
    else if(preset_kind==PRESET_EQ) { project.eq[owner][slot]=preset.eq; project.effect_mix[owner][slot]=preset.mix; project.effect_bypass[owner][slot]=0; }
    else if(preset_kind==PRESET_CHORUS) { project.chorus[owner][slot]=preset.chorus; project.effect_mix[owner][slot]=preset.mix; project.effect_bypass[owner][slot]=0; }
    else {
        Sample raw={0},processed={0}; int ok;
        if(!sampler_flush()) return 0;
        if(!strcmp(preset.sample_path,SAMPLE_EMPTY)) ok=1;
        else if(preset.sample_path[0]) ok=sample_load(preset.sample_path,&raw);
        else ok=sample_clone(originals[owner],&raw);
        if(!ok || !sample_process(raw,preset.sampler,&processed)) {
            sample_free(raw); sample_free(processed); snprintf(status,sizeof status,"Preset sample missing or processing failed; channel unchanged"); return 0;
        }
        Sample old=samples[owner],source=originals[owner];
        audio_sample(owner,processed); samples[owner]=processed; originals[owner]=raw;
        sample_free(old); sample_free(source); project.sampler[owner]=preset.sampler;
        if(preset.sample_path[0]) snprintf(project.paths[owner],sizeof project.paths[owner],"%s",preset.sample_path);
        sample_generation[owner]++; sampler_generation[owner]=sample_generation[owner]; sampler_applied[owner]=preset.sampler;
        sample_duration(&project,owner,processed,raw);
    }
    snprintf(status,sizeof status,"Loaded preset: %.150s",GetFileNameWithoutExt(chosen)); return 1;
}
static int project_file_commit(const char *chosen) {
    if(file_action==8 || file_action==9) return preset_commit(chosen,file_action==8);
    int ok;
    if(file_action==4) return load_project(chosen);
    if(file_action==7) {
        if(relink_channel<0 || relink_channel>=project.channel_count) return 0;
        char name[PATTERN_NAME]; memcpy(name,project.channel_names[relink_channel],sizeof name);
        int loaded=import_sample(chosen,relink_channel);
        if(loaded) memcpy(project.channel_names[relink_channel],name,sizeof name);
        return loaded;
    }
    if(file_action==5) {
        ok=sampler_flush() && export_wav(chosen,&project,samples);
        if(ok) snprintf(document.export_path,sizeof document.export_path,"%s",chosen);
        snprintf(status,sizeof status,ok?"Exported %.220s":"Export failed: %.220s",chosen); return ok;
    }
    ok=sampler_flush() && project_save_assets(chosen,&project,originals,file_action==6);
    if(ok) project_document_saved(&document,&project,chosen);
    snprintf(status,sizeof status,ok?"Saved %.220s":"Save failed: %.220s",chosen); return ok;
}
static void project_file_action(int action) {
    recording_finish();
    if(action==2 && document.saved_on_disk) {
        int ok=sampler_flush() && project_save_assets(document.path,&project,originals,0);
        snprintf(status,sizeof status,ok?"Saved %.220s":"Save failed: %.220s",document.path);
        if(ok) replacement_saved(); else if(document.pending_action) open_popup(12);
        return;
    }
    const char *current=action==7?project.paths[relink_channel]:action==5?(document.export_path[0]?document.export_path:"song.wav"):document.path;
    char initial[PATH_MAX];
    int length=current[0]=='/'?snprintf(initial,sizeof initial,"%s",current):snprintf(initial,sizeof initial,"%s/%s",GetWorkingDirectory(),current);
    if(length>=(int)sizeof initial) { snprintf(status,sizeof status,"File path is too long."); return; }
    if(!file_chooser_begin_recent(&file_picker,initial,action==7?"wav;flac;mp3":action==5?"wav":"hbt",action!=4 && action!=7,action==4?FILE_OPEN:action==5?FILE_EXPORT:action==7?FILE_SAMPLE:FILE_SAVE)) { snprintf(status,sizeof status,"%s",file_picker.error); return; }
    file_action=action; file_field=action==4 || action==7?0:2; file_confirm=0;
    file_picker_sync(); rename_select_all=file_field==2; open_popup(11);
}
static int file_picker_commit(void) {
    if(!project_file_commit(file_pending)) return 0;
    file_chooser_remember(&file_picker,file_pending); return 1;
}
static void file_picker_submit(void) {
    int result=file_chooser_path(&file_picker,file_pending,sizeof file_pending);
    if(result<0) return;
    if(result==0) { file_picker.name[0]=0; file_picker_sync(); return; }
    if(result==2) { file_confirm=1; file_field=0; rename_select_all=0; return; }
    if(file_picker_commit()) { pattern_popup=0; if(file_action==2 || file_action==3 || file_action==6) replacement_saved(); }
    else snprintf(file_picker.error,sizeof file_picker.error,"%.159s",status);
}
static void draw_file_picker(int x,int y,int w,int h,float scale) {
    FileChooser *c=&file_picker;
    label(file_action==8?"Save preset":file_action==9?"Load preset":file_action==4?"Open project":file_action==5?"Export WAV":file_action==6?"Collect samples and save":file_action==7?"Relink sample":file_action==3?"Save project as":"Save project",x+8,y+3,13,ink);
    if(file_confirm) {
        label("Replace this existing file?",x+16,y+56,15,ink);
        label(fit_text(file_pending,w-32,13),x+16,y+86,13,muted);
        if(button("Replace",x+16,y+126,100,26,1) || IsKeyPressed(KEY_ENTER)) {
            if(file_picker_commit()) { pattern_popup=0; if(file_action==2 || file_action==3 || file_action==6) replacement_saved(); }
            else { file_confirm=0; snprintf(c->error,sizeof c->error,"%.159s",status); }
        }
        if(button("Back",x+124,y+126,100,26,0)) file_confirm=0;
        return;
    }
    if(button("Up",x+12,y+30,44,26,0) && file_chooser_parent(c)) file_picker_sync();
    const char *home=getenv("HOME");
    if(button("Home",x+60,y+30,54,26,0) && home && file_chooser_folder(c,home)) file_picker_sync();
    if(button("Hidden",x+w-84,y+30,72,26,c->hidden)) {
        c->hidden=!c->hidden; if(file_chooser_folder(c,c->directory)) file_picker_sync();
    }
    Rectangle path_box={x+122,y+30,w-214,26};
    DrawRectangleRec(path_box,bg); DrawRectangleLinesEx(path_box,1,file_field==1?accent:cell);
    if(hover(path_box.x,path_box.y,path_box.width,path_box.height) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) { file_field=1; rename_select_all=1; }
    int path_submitted=file_field==1 && IsKeyPressed(KEY_ENTER);
    if(file_field==1) {
        text_input(file_directory,sizeof file_directory);
        if(IsKeyPressed(KEY_ENTER) && file_chooser_folder(c,file_directory)) { file_picker_sync(); file_field=0; rename_select_all=0; }
    }
    if(file_field==1 && rename_select_all) DrawRectangle(path_box.x+4,path_box.y+5,fminf(path_box.width-8,text_width(file_directory,12)),16,cell);
    label(fit_text(file_directory,path_box.width-8,12),path_box.x+4,path_box.y+6,12,ink);
    int list_y=y+66,list_h=h-174,rows=list_h/24,list_w=w-38;
    if(hover(x+12,list_y,w-24,list_h)) c->scroll-=GetMouseWheelMove()*3;
    if(file_field!=1 && file_field!=2) {
        if(IsKeyPressed(KEY_UP)) c->selected=c->selected<0?0:(int)fmaxf(0,c->selected-1);
        if(IsKeyPressed(KEY_DOWN)) c->selected=(int)fminf(c->count-1,c->selected+1);
        if(c->selected>=0 && c->selected<c->count) {
            if(c->selected<c->scroll) c->scroll=c->selected;
            if(c->selected>=c->scroll+rows) c->scroll=c->selected-rows+1;
            if(!c->entries[c->selected].directory) snprintf(c->name,sizeof c->name,"%s",c->entries[c->selected].name);
        }
    }
    int maximum=(int)fmaxf(0,c->count-rows); c->scroll=(int)fmaxf(0,fminf(maximum,c->scroll));
    DrawRectangle(x+12,list_y,w-24,list_h,bg);
    BeginScissorMode((x+12)*scale,list_y*scale,list_w*scale,list_h*scale);
    int activate=-1;
    for(int row=c->scroll;row<c->count && row<c->scroll+rows;row++) {
        FileEntry *entry=&c->entries[row]; int ry=list_y+(row-c->scroll)*24;
        if(button("",x+12,ry,list_w,24,c->selected==row)) {
            double now=GetTime(); activate=file_last_row==row && now-file_last_click<.35?row:-1;
            c->selected=row; file_field=0; rename_select_all=0; c->error[0]=0;
            if(!entry->directory) snprintf(c->name,sizeof c->name,"%s",entry->name);
            file_last_row=row; file_last_click=now;
        }
        if(entry->directory) { DrawRectangleLines(x+20,ry+8,12,9,c->selected==row?ui_theme.selected_text:muted); DrawRectangle(x+20,ry+5,6,3,c->selected==row?ui_theme.selected_text:muted); }
        label(fit_text(entry->name,list_w-40,13),x+40,ry+5,13,c->selected==row?ui_theme.selected_text:ink);
    }
    BeginScissorMode(x*scale,y*scale,w*scale,h*scale);
    float thumb=fminf(list_h,fmaxf(24,list_h*rows/(float)fmaxf(1,c->count))),travel=list_h-thumb;
    float thumb_y=list_y+(maximum?c->scroll/(float)maximum*travel:0);
    if(hover(x+w-22,list_y,10,list_h) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) file_scroll_drag=1;
    if(file_scroll_drag) {
        if(IsMouseButtonDown(MOUSE_BUTTON_LEFT)) c->scroll=(int)fmaxf(0,fminf(maximum,(mouse.y-list_y-thumb/2)/fmaxf(1,travel)*maximum));
        else file_scroll_drag=0;
    }
    ui_surface((Rectangle){x+w-22,thumb_y,8,thumb},file_scroll_drag?accent:muted);
    if(!c->count) label("No matching files in this folder",x+24,list_y+12,13,muted);
    int enter=IsKeyPressed(KEY_ENTER) && file_field!=1 && !path_submitted;
    if(activate>=0 || (enter && file_field==0 && c->selected>=0 && c->selected<c->count && c->entries[c->selected].directory)) {
        int selected=activate>=0?activate:c->selected;
        if(c->entries[selected].directory) {
            char path[PATH_MAX];
            if(snprintf(path,sizeof path,"%s/%s",c->directory,c->entries[selected].name)<(int)sizeof path && file_chooser_folder(c,path)) file_picker_sync();
        } else file_picker_submit();
        enter=0;
    }
    label(file_action==8 || file_action==9?"Device presets (*.llpreset)":file_action==7?"Audio samples (*.wav, *.flac, *.mp3)":file_action==5?"WAV audio (*.wav)":"LibreLoop projects (*.hbt)",x+12,y+h-102,11,muted);
    label("Filename",x+12,y+h-76,12,muted);
    Rectangle name_box={x+82,y+h-82,w-94,26};
    DrawRectangleRec(name_box,bg); DrawRectangleLinesEx(name_box,1,file_field==2?accent:cell);
    if(hover(name_box.x,name_box.y,name_box.width,name_box.height) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) { file_field=2; rename_select_all=1; }
    if(file_field==2) text_input(c->name,sizeof c->name);
    if(file_field==2 && rename_select_all) DrawRectangle(name_box.x+4,name_box.y+5,fminf(name_box.width-8,text_width(c->name,13)),16,cell);
    label(fit_text(c->name,name_box.width-8,13),name_box.x+4,name_box.y+6,13,ink);
    label(fit_text(c->error,w-210,11),x+12,y+h-30,11,ui_theme.meter_high);
    if(button("Cancel",x+w-188,y+h-38,80,26,0)) { document.pending_action=0; pattern_popup=0; snprintf(status,sizeof status,"Cancelled."); }
    if(button(file_action==4 || file_action==9?"Open":file_action==7?"Relink":file_action==5?"Export":"Save",x+w-100,y+h-38,88,26,1) || enter) file_picker_submit();
}
static void history_stamps(uint64_t stamps[CHANNELS]) {
    for(int c=0;c<CHANNELS;c++) stamps[c]=((uint64_t)sample_epoch<<32)|sample_generation[c];
}
static int history_checkpoint(void) {
    uint64_t stamps[CHANNELS]; history_stamps(stamps);
    if(!history_capture(&edit_history,&project,originals,stamps)) {
        snprintf(status,sizeof status,"Could not retain undo history: insufficient memory."); return 0;
    }
    return 1;
}
static void undo_redo(int direction) {
    if(recording_active() || captured()) { snprintf(status,sizeof status,"Finish the current edit or recording before undo/redo."); return; }
    if(!history_checkpoint()) return;
    uint8_t keep[CHANNELS]={0}; uint64_t current_stamps[CHANNELS]; history_stamps(current_stamps);
    Project next; Sample retained[CHANNELS],fresh[CHANNELS]={0},processed[CHANNELS]={0};
    if(!history_peek(&edit_history,direction,&next,retained)) { snprintf(status,sizeof status,direction<0?"Nothing to undo.":"Nothing to redo."); return; }
    for(int c=0;c<CHANNELS;c++) {
        if(history_source_matches(&edit_history,direction,c,originals[c],current_stamps[c]) && sampler_processing_equal(next.sampler[c],sampler_applied[c])) {
            keep[c]=1; fresh[c]=originals[c]; processed[c]=samples[c]; sample_duration(&next,c,samples[c],originals[c]); continue;
        }
        fresh[c]=sample_copy(retained[c]);
        if((retained[c].frames && !fresh[c].data) || !sample_process(fresh[c],next.sampler[c],&processed[c])) goto failed;
        sample_duration(&next,c,processed[c],fresh[c]);
    }
    if(sampler_job.busy) { pthread_join(sampler_job.thread,NULL); sample_free(sampler_job.input); sample_free(sampler_job.result); sampler_job.busy=0; }
    audio_channels(&next,processed);
    for(int c=0;c<CHANNELS;c++) {
        if(keep[c]) continue;
        sample_free(samples[c]); sample_free(originals[c]); samples[c]=processed[c]; originals[c]=fresh[c];
        sample_generation[c]++; sampler_generation[c]=sample_generation[c]; sampler_applied[c]=next.sampler[c];
    }
    project=next; history_step(&edit_history,direction);
    uint64_t stamps[CHANNELS]; history_stamps(stamps); history_rebind(&edit_history,originals,stamps);
    pattern=fmaxf(0,fminf(pattern,project.pattern_count-1)); channel=fmaxf(0,fminf(channel,project.channel_count-1));
    piano_channel=instrument_channel=channel; automation_selected=automation_node=-1; picker_drag=-1;
    note_drag=NULL; control_drag=NULL; menu_value=NULL; context_kind=0;
    memset(note_selected,0,sizeof note_selected); memset(arrangement.selected,0,sizeof arrangement.selected);
    arrangement.source_pattern=-1; arrangement.source_steps=STEPS; arrangement.source_offset=0;
    rack_scroll=0; picker_scroll=0; piano_gesture=PIANO_IDLE;
    snprintf(status,sizeof status,direction<0?"Undone.":"Redone."); return;
failed:
    for(int c=0;c<CHANNELS;c++) if(!keep[c]) { sample_free(fresh[c]); sample_free(processed[c]); }
    snprintf(status,sizeof status,"Undo/redo unchanged: could not prepare audio.");
}
static void capture_control(float *value,float low,float high,int fader) {
    ParameterTarget target;
    const ParameterDescriptor *descriptor=parameter_from_pointer(&project,value,&target)?parameter_descriptor(target.parameter):NULL;
    control_integer=descriptor && descriptor->kind==PARAMETER_INTEGER;
    control_logarithmic=descriptor && descriptor->kind==PARAMETER_LOGARITHMIC;
    control_raw=*value; control_drag=value;
    control_low=descriptor?descriptor->low:low; control_high=descriptor?descriptor->high:high;
    control_fader=fader; control_reverse=0; input_enabled=0;
}
enum { KNOB_NORMAL,KNOB_SWING,KNOB_PAN,KNOB_WIDTH,KNOB_CENTER,KNOB_VOLUME,KNOB_LOGARITHMIC,KNOB_FIXED_COARSE };
static void knob_style(int x,int y,float *value,float low,float high,float initial,const char *name,int style) {
    float size=style==KNOB_SWING?SWING_RADIUS:KNOB_RADIUS,shown=displayed_value(value,*value);
    int over=hover(x-size-1,y-size-1,size*2+2,size*2+2);
    int fixed_coarse=style==KNOB_FIXED_COARSE;
    if(fixed_coarse) {
        high=3; initial=(int)initial&3; shown=(int)roundf(shown)&3;
        /* Old DX files may store aliases 4..31. Canonicalize on interaction,
           preserving their audible frequency and leaving unopened files alone. */
        if(over && (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) || GetMouseWheelMove()))
            *value=(int)roundf(*value)&3;
    }
    if(over || control_drag==value) {
        if(style==KNOB_VOLUME) snprintf(status,sizeof status,"%s: %.4g (%.2f dB) | Dot marks 0 dB; drag or wheel; right-click for value / automation",name,shown,gain_db(shown));
        else snprintf(status,sizeof status,"%s: %.4g | Drag up/down or wheel; right-click for value / automation",name,shown);
    }
    if(over) {
        if(IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) control_menu(value,low,high,initial,name);
        if(IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) { capture_control(value,low,high,0); control_low=low; control_high=high; control_reverse=style==KNOB_WIDTH; }
        float wheel=GetMouseWheelMove();
        if(wheel) {
            ParameterTarget target;
            const ParameterDescriptor *info=parameter_from_pointer(&project,value,&target)?parameter_descriptor(target.parameter):NULL;
            int integer=info && info->kind==PARAMETER_INTEGER;
            float next=style==KNOB_LOGARITHMIC?*value*powf(high/low,wheel/20):*value+wheel*(integer?1:(high-low)/20)*(style==KNOB_WIDTH?-1:1);
            *value=fmaxf(low,fminf(high,integer?roundf(next):next));
        }
    }
    float radius=size,fraction=(shown-low)/(high-low);
    if(style==KNOB_LOGARITHMIC) fraction=logf(shown/low)/logf(high/low);
    if(style==KNOB_CENTER) fraction=shown<initial?.5f*(shown-low)/(initial-low):.5f+.5f*(shown-initial)/(high-initial);
    if(style==KNOB_WIDTH) fraction=1-fraction;
    if(style==KNOB_VOLUME) fraction=shown<=1?.75f*(shown):.75f+.25f*(shown-1)/(high-1);
    int centered=style==KNOB_PAN || style==KNOB_WIDTH || style==KNOB_CENTER;
    float origin=centered?PI/2:style==KNOB_SWING?2.4f:style==KNOB_VOLUME?PI/2:-PI/2;
    float angle=origin+fraction*(style==KNOB_SWING?4.6f:2*PI);
    Color color=style==KNOB_SWING || style==KNOB_CENTER?ui_theme.swing:style==KNOB_PAN?(fraction<.5f?ui_theme.pan_left:ui_theme.pan_right):style==KNOB_WIDTH?(fraction<.5f?ui_theme.stereo:ui_theme.mono):ui_theme.knob;
    if(style==KNOB_SWING) {
        DrawTexturePro(knob_arcs,(Rectangle){0,256,32,32},(Rectangle){x-radius,y-radius,radius*2,radius*2},(Vector2){0},0,ui_theme.border);
        circle(x,y,radius*.68f,cell);
    } else { circle(x,y,radius,ui_theme.border); circle(x,y,radius*.8f,cell); }
    int level=fmaxf(0,fminf(128,roundf(fraction*128)));
    float rotation=style==KNOB_SWING || centered?0:(origin+PI/2)*RAD2DEG;
    DrawTexturePro(knob_arcs,(Rectangle){(style==KNOB_SWING?0:centered?512:1024)+level%16*32,level/16*32,32,32},(Rectangle){x,y,radius*2,radius*2},(Vector2){radius,radius},rotation,color);
    if(style==KNOB_VOLUME) {
        float unity=origin+.75f*2*PI;
        circle(x+cosf(unity)*size*1.22f,y+sinf(unity)*size*1.22f,1.2f,muted);
    }
    circle(x+cosf(angle)*radius*.85f,y+sinf(angle)*radius*.85f,1.7f*size/KNOB_RADIUS,centered && level==64?muted:color);
}
static void knob(int x,int y,float *value,float low,float high,float initial,const char *name) {
    knob_style(x,y,value,low,high,initial,name,KNOB_NORMAL);
}
static void channel_route(int c,int x,int y) {
    if(drag_button(TextFormat("%d",project.route[c]),x,y,24,22,route_drag==c)) {
        route_drag=c; route_start=project.route[c]; route_y=mouse.y+windows.editors[knob_context].rect.y;
        channel=c; mixer_selected=project.route[c]; input_enabled=0;
    }
    if(hover(x,y,24,22)) snprintf(status,sizeof status,"%s mixer destination: drag up/down; 0 = Master",project.channel_names[c]);
}
static void channel_controls(int c,float width) {
    int x=width-276;
    mute_light(x,36,project.mute,project.channel_count,c,project.channel_names[c]);
    knob_style(x+28,36,&project.pan[c],-1,1,0,"Channel pan",KNOB_PAN);
    knob_style(x+58,36,&project.volume[c],0,VOLUME_KNOB_MAX,1,"Channel volume",KNOB_VOLUME);
    knob_style(x+94,36,&project.channel_pitch[c],-1,1,0,"Playback pitch",KNOB_CENTER);
    if(hover(x+83,25,22,22) || control_drag==&project.channel_pitch[c]) snprintf(status,sizeof status,"Playback pitch: %+.2f semitones | range ±%.0f semitones",project.channel_pitch[c]*project.pitch_range[c],project.pitch_range[c]);
    float *range=&project.pitch_range[c];
    if(drag_button(TextFormat("%.0f",*range),x+150,25,30,22,control_drag==range)) { capture_control(range,1,48,0); control_integer=1; }
    if(hover(x+150,25,30,22)) {
        *range=fmaxf(1,fminf(48,*range+roundf(GetMouseWheelMove())));
        if(IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) { control_menu(range,1,48,2,"Pitch range (semitones)"); }
        snprintf(status,sizeof status,"Pitch range: %.0f semitones | drag up/down, 1–48 whole semitones; right-click to enter",*range);
    }
    channel_route(c,x+205,25);
    label("PAN",x+18,53,10,muted); label("VOL",x+48,53,10,muted);
    label("PITCH",x+80,53,10,muted); label("RANGE",x+150,53,10,muted); label("ROUTE",x+196,53,10,muted);
}
static double pattern_playback_position(void) {
    if(!playing) return -1;
    if(!song) return visual_step;
    double latest=-1,position=-1; int solo=solo_any(project.lane_mute,LANES);
    for(int l=0;l<LANES;l++) if(!(project.lane_mute[l]&1) && (!solo || (project.lane_mute[l]&2)))
        for(int b=0;b<CLIPS;b++) if(project.clips[l][b]==pattern+1) {
            double start=project.clip_starts[l][b]*STEPS,local=visual_step-start;
            if(start>latest && local>=0 && local<clip_length(&project,l,b) && local+project.clip_offsets[l][b]<project.pattern_steps[pattern]) { latest=start; position=local+project.clip_offsets[l][b]; }
        }
    return position;
}
static float edit_steps(void) { return arrangement_edit_steps(&arrangement,&project,pattern); }
static void pattern_extend(float end) {
    if(end<=project.pattern_steps[pattern]) return;
    project.pattern_steps[pattern]=ceilf(end/STEPS)*STEPS;
    if(arrangement.source_pattern==pattern) arrangement.source_steps=project.pattern_steps[pattern];
}
static int rack_melodic(int c) {
    if(piano_channels[pattern][c]) return 1;
    for(int i=0;i<NOTES;i++) { Note n=project.notes[pattern][c][i]; if(n.velocity && (n.pitch!=60 || n.length>0)) return 1; }
    return 0;
}
static void paint_rack(float width,float x,float y) {
    int visible=fmaxf(1,(windows.editors[0].rect.h-RACK_TOP-48)/28);
    if(x<200 || x>=width-26 || y<RACK_TOP || y>=RACK_TOP+visible*28) { rack_paint_channel=-1; return; }
    int c=rack_channel_at(rack_scroll+(y-RACK_TOP)/28); float step=floorf(rack_view[pattern]+(x-200)/RACK_STEP_WIDTH);
    if(c<0 || c>=project.channel_count || rack_melodic(c)) { rack_paint_channel=-1; return; }
    float from=rack_paint_channel==c?rack_paint_step:step;
    for(int i=0;i<=fabsf(step-from) && i<NOTES;i++) {
        float start=fminf(from,step)+i;
        Note *n=note_at(&project,pattern,c,start,60);
        if(rack_paint==MOUSE_BUTTON_RIGHT) { if(n) n->velocity=0; }
        else if(!n) { pattern_extend(start+1); note_add(&project,pattern,c,start,60,0); }
    }
    rack_paint_channel=c; rack_paint_step=step;
}
static void timeline_ruler(int id,float start,float span,float gx,float y,float width,float q);
static float channel_flash[CHANNELS];
static uint8_t channel_active_ui[CHANNELS];
static int channel_decay_active;
static void rack(float width,float height) {
    float stepw=RACK_STEP_WIDTH,gridw=width-226,view=rack_view[pattern],span=gridw/stepw;
    rack_range[pattern]=fmaxf(rack_range[pattern],fmaxf(view+span*2,project.pattern_steps[pattern]+span));
    timeline_ruler(0,view,span,200,TITLE,gridw,1);
    int visible=fmaxf(1,(height-RACK_TOP-48)/28),rows[CHANNELS],count=rack_channels(rows);
    if(hover(0,RACK_TOP,width,height-RACK_TOP-18)) rack_scroll-=GetMouseWheelMove();
    rack_scroll=fmaxf(0,fminf(fmaxf(0,count-visible),rack_scroll));
    for(int index=rack_scroll;index<count && index<rack_scroll+visible;index++) {
        int c=rows[index],row=RACK_TOP+(index-rack_scroll)*28;
        mute_light(12,row+11,project.mute,project.channel_count,c,project.channel_names[c]);
        knob_style(36,row+11,&project.pan[c],-1,1,0,"Channel pan",KNOB_PAN); knob_style(62,row+11,&project.volume[c],0,VOLUME_KNOB_MAX,1,"Channel volume",KNOB_VOLUME);
        channel_route(c,78,row);
        if(button_color(fit_text(channel_caption(c),68,13),106,row,78,22,0,project.channel_audio[c]?audio_color(c):cell)) {
            channel=instrument_channel=c; windows_focus(&windows,4);
        }
        if(hover(106,row,78,22) && IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) { channel=c; open_context(2,c,(Vector2){mouse.x+windows.editors[0].rect.x,mouse.y+windows.editors[0].rect.y}); }
        if(hover(106,row,78,22)) snprintf(status,sizeof status,"%s: open instrument; right-click for Piano Roll, rename, mute or delete; Keys: Z/Q white, S/2 black",project.channel_names[c]);
        if(sample_drag[0] && sample_moved && mouse.y>=row && mouse.y<row+28 && mouse.x>=0 && mouse.x<width && windows_hit(&windows,mouse.x+windows.editors[0].rect.x,mouse.y+windows.editors[0].rect.y)==0) {
            DrawRectangleLinesEx((Rectangle){106,row,78,22},1,ui_theme.signal);
            snprintf(status,sizeof status,"Release to replace %s's sample",project.channel_names[c]);
        }
        float brightness=fmaxf(channel_active_ui[c]?.32f:0,channel_flash[c]);
        DrawRectangle(187,row,3,22,Fade(ui_theme.light?BLACK:WHITE,.08f+brightness*.92f));
        int melodic=rack_melodic(c);
        if(melodic) {
            DrawRectangle(200,row+2,gridw,20,bg);
            float active=fmaxf(0,fminf(gridw,(edit_steps()-view)*stepw));
            if(active<gridw) DrawRectangle(200+active,row+2,gridw-active,20,ui_theme.disabled);
            for(int i=0;i<NOTES;i++) {
                Note n=project.notes[pattern][c][i]; float start=200+(n.start-view)*stepw,end=start+fmaxf(2,(n.length?n.length:1)*stepw-2);
                if(n.velocity && end>=200 && start<200+gridw) DrawRectangle(fmaxf(200,start),row+3+fmaxf(0,fminf(16,(72-(int)n.pitch)*.65f)),fminf(200+gridw,end)-fmaxf(200,start),2,playing_notes[c][i]?ink:accent);
            }
            if(hover(200,row+2,gridw,20)) snprintf(status,sizeof status,"%s notes: click to open Piano Roll; right-click for channel options",project.channel_names[c]);
            if(hover(200,row+2,gridw,20)) {
                SetMouseCursor(MOUSE_CURSOR_POINTING_HAND);
                if(IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) { channel=piano_channel=c; piano_channels[pattern][c]=1; browser_focus=0; windows_focus(&windows,2); input_enabled=0; }
            }
            if(hover(200,row+2,gridw,20) && IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) { channel=c; open_context(2,c,(Vector2){mouse.x+windows.editors[0].rect.x,mouse.y+windows.editors[0].rect.y}); }
            continue;
        }
        for(int i=0;i<span+2;i++) {
            float step=floorf(view)+i,x=200+(step-view)*stepw,left=fmaxf(200,x),right=fminf(200+gridw,x+stepw-3);
            if(right<=left) continue;
            Note *n=note_at(&project,pattern,c,step,60); int available=step<edit_steps();
            ui_surface((Rectangle){left+1,row+2,fmaxf(1,right-left-2),18},!available?ui_theme.disabled:n?ui_theme.step_on[(int)floorf(step/4)%2]:fmodf(floorf(step/4),2)?ui_theme.step_alt:cell);
            if(n && playing_notes[c][n-project.notes[pattern][c]]) DrawRectangleLinesEx((Rectangle){left+1,row+2,fmaxf(1,right-left-2),18},1,accent);
            if(fmodf(step,STEPS)==0 && x>=200) DrawLine(x,row,x,row+22,muted);
            if(hover(left,row+2,right-left,18)) {
                snprintf(status,sizeof status,"%s step %.0f: left paints%s, right erases",project.channel_names[c],step+1,available?"":" and extends the pattern");
                if(IsMouseButtonPressed(MOUSE_BUTTON_LEFT) || IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
                    rack_paint=IsMouseButtonPressed(MOUSE_BUTTON_LEFT)?MOUSE_BUTTON_LEFT:MOUSE_BUTTON_RIGHT;
                    rack_paint_channel=-1; paint_rack(width,mouse.x,mouse.y); input_enabled=0;
                }
            }
        }
    }
    double position=pattern_playback_position();
    if(position>=0) {
        float x=200+(position-view)*stepw;
        if(x>=200 && x<=200+gridw)
            DrawLineEx((Vector2){x,RACK_TOP-4},(Vector2){x,RACK_TOP+fminf(visible,count-rack_scroll)*28-6},1.5f,ink);
    }
    int add_y=RACK_TOP+(int)fminf(visible,count-rack_scroll)*28;
    if(add_y+22<=height-18) {
        int dropping=sample_drag[0] && sample_moved && mouse.y>=add_y && mouse.y<height-18 && mouse.x>=0 && mouse.x<width && windows_hit(&windows,mouse.x+windows.editors[0].rect.x,mouse.y+windows.editors[0].rect.y)==0;
        if(button("+",106,add_y,78,22,0)) open_popup(6);
        if(dropping) {
            ui_surface((Rectangle){106,add_y,78,22},Fade(ui_theme.signal,.18f));
            DrawRectangleLinesEx((Rectangle){106,add_y,78,22},1,ui_theme.signal);
            snprintf(status,sizeof status,"Release to add a new sample channel");
        } else if(hover(106,add_y,78,22)) snprintf(status,sizeof status,"Add instrument: choose a plugin, or drop a sample here");
    }
    float area=visible*28,maximum=fmaxf(0,count-visible),thumb=fminf(area,fmaxf(24,area*visible/fmaxf(1,count))),travel=area-thumb;
    float y=RACK_TOP+(maximum?rack_scroll/maximum*travel:0);
    if(maximum>0) {
        DrawRectangle(width-12,RACK_TOP,8,area,bg); ui_surface((Rectangle){width-12,y,8,thumb},rack_vdrag || hover(width-14,RACK_TOP,14,area)?accent:muted);
        if(hover(width-14,RACK_TOP,14,area)) {
            snprintf(status,sizeof status,"Drag to scroll Rack channels; wheel over rows also scrolls");
            if(IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                if(mouse.y<y || mouse.y>y+thumb) rack_scroll=fmaxf(0,fminf(maximum,(mouse.y-RACK_TOP-thumb/2)/fmaxf(1,travel)*maximum));
                rack_vy=mouse.y+windows.editors[0].rect.y; rack_vstart=rack_scroll; rack_vscale=maximum/fmaxf(1,travel); rack_vdrag=1; input_enabled=0;
            }
        }
    }
    float total=rack_range[pattern],hthumb=timeline_thumb(gridw,span,total),htravel=gridw-hthumb,hmaximum=total-span;
    float hx=200+(hmaximum>0?rack_view[pattern]/hmaximum*htravel:0);
    DrawRectangle(200,height-16,gridw,8,bg); ui_surface((Rectangle){hx,height-16,hthumb,8},rack_hdrag || hover(200,height-19,gridw,14)?accent:muted);
    if(hover(200,height-19,gridw,14)) {
        snprintf(status,sizeof status,"Drag to scroll steps horizontally; grey steps extend the pattern when painted");
        if(IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            if(mouse.x<hx || mouse.x>hx+hthumb) rack_view[pattern]=fmaxf(0,fminf(hmaximum,(mouse.x-200-hthumb/2)/fmaxf(1,htravel)*hmaximum));
            rack_hx=mouse.x+windows.editors[0].rect.x; rack_hstart=rack_view[pattern]; rack_hscale=hmaximum/fmaxf(1,htravel); rack_hdrag=1; input_enabled=0;
        }
    }
}
static Color pattern_rgb(uint32_t rgb) {
    return source_rgb(rgb);
}
static Color pattern_color(int id) { return pattern_rgb(project.pattern_colors[id]); }
static void pattern_pitch_range(int pat,int *low,int *high) {
    *low=127; *high=0;
    for(int c=0;c<project.channel_count;c++) for(int n=0;n<NOTES;n++) {
        Note note=project.notes[pat][c][n]; if(!note.velocity || note.start>=project.pattern_steps[pat]) continue;
        *low=fminf(*low,note.pitch); *high=fmaxf(*high,note.pitch);
    }
}
/* One small transparent image per pattern, reused by all Playlist copies. */
static struct {
    RenderTexture2D image;
    Note notes[CHANNELS][NOTES];
    float steps; int channels,valid; Color color;
} previews[PATTERNS];
static void previews_update(float scale) {
    float pixels=(windows.editors[1].rect.w-236)*arrangement.zoom/(BARS*STEPS)*scale;
    for(int pat=0;pat<project.pattern_count;pat++) {
        Color content=clip_foreground(pattern_color(pat));
        float size=project.pattern_steps[pat]*pixels;
        previews[pat].valid=0;
        if(!isfinite(size) || size>2048) continue; /* Keep vector detail at extreme zoom. */
        int w=fmaxf(1,ceilf(size)),h=fmaxf(1,ceilf(32*scale));
        if(!previews[pat].image.id || previews[pat].image.texture.width!=w || previews[pat].image.texture.height!=h ||
           previews[pat].steps!=project.pattern_steps[pat] || previews[pat].channels!=project.channel_count ||
           memcmp(&previews[pat].color,&content,sizeof content) || memcmp(previews[pat].notes,project.notes[pat],sizeof previews[pat].notes)) {
            if(!previews[pat].image.id || previews[pat].image.texture.width!=w || previews[pat].image.texture.height!=h) {
                if(previews[pat].image.id) UnloadRenderTexture(previews[pat].image);
                previews[pat].image=LoadRenderTexture(w,h);
                SetTextureFilter(previews[pat].image.texture,TEXTURE_FILTER_BILINEAR);
                SetTextureWrap(previews[pat].image.texture,TEXTURE_WRAP_CLAMP);
            }
            if(!previews[pat].image.id) continue;
            memcpy(previews[pat].notes,project.notes[pat],sizeof previews[pat].notes);
            previews[pat].steps=project.pattern_steps[pat]; previews[pat].channels=project.channel_count; previews[pat].color=content;
            BeginTextureMode(previews[pat].image); ClearBackground(BLANK);
            int low,high; pattern_pitch_range(pat,&low,&high);
            float note_height=fmaxf(2,fminf(5,32.f/fmaxf(1,high-low+1)));
            for(int c=0;c<project.channel_count;c++) for(int n=0;n<NOTES;n++) {
                Note note=project.notes[pat][c][n]; if(!note.velocity || note.start>=project.pattern_steps[pat]) continue;
                float x=note.start*w/project.pattern_steps[pat]+scale;
                float fraction=high>low?(high-note.pitch)/(float)(high-low):.5f;
                float y=fraction*(32-note_height)*h/32;
                float length=fmaxf(scale,(note.length?note.length:1)*w/project.pattern_steps[pat]-2*scale);
                DrawRectangleRec((Rectangle){x,y,length,note_height*h/32},content);
            }
            EndTextureMode();
        }
        previews[pat].valid=1;
    }
}
static void clip_preview(int pat,float x,float y,float left,float right,float pixels,float steps,float height) {
    Color content=clip_foreground(pattern_color(pat));
    float full=project.pattern_steps[pat]*pixels;
    if(previews[pat].valid) {
        right=fminf(right,x+full);
        if(right>left) {
            Texture2D image=previews[pat].image.texture;
            DrawTexturePro(image,(Rectangle){(left-x)*image.width/full,0,(right-left)*image.width/full,-image.height},
                           (Rectangle){left,y+16,right-left,height-20},(Vector2){0},0,WHITE);
        }
    } else {
        int low,high; pattern_pitch_range(pat,&low,&high);
        float area=fmaxf(1,height-20),note_height=fmaxf(2,fminf(5,32.f/fmaxf(1,high-low+1)))*area/32;
        for(int c=0;c<project.channel_count;c++) for(int n=0;n<NOTES;n++) {
            Note note=project.notes[pat][c][n]; if(!note.velocity || note.start>=steps) continue;
            float fraction=high>low?(high-note.pitch)/(float)(high-low):.5f;
            float yy=y+16+fraction*(area-note_height);
            float nx=x+note.start*pixels+1,nw=fmaxf(1,fminf(note.length?note.length:1,steps-note.start)*pixels-2);
            if(nx<=right && nx+nw>=left) DrawRectangleRec((Rectangle){fmaxf(left,nx),yy,fmaxf(1,fminf(right,nx+nw)-fmaxf(left,nx)),note_height},content);
        }
    }
}
static void picker_notes(int p,float x,float y) {
    Color content=clip_foreground(pattern_color(p)); int low=127,high=0,count=0; float end=0;
    for(int c=0;c<project.channel_count;c++) for(int n=0;n<NOTES;n++) {
        Note note=project.notes[p][c][n]; if(!note.velocity || note.start>=project.pattern_steps[p]) continue;
        low=fminf(low,note.pitch); high=fmaxf(high,note.pitch); count++;
        end=fmaxf(end,fminf(project.pattern_steps[p],note.start+(note.length?note.length:1)));
    }
    if(!count) return;
    float span=fmaxf(STEPS,ceilf(end/STEPS)*STEPS),note_height=fmaxf(2,fminf(5,26.f/(high-low+1)));
    for(int c=0;c<project.channel_count;c++) for(int n=0;n<NOTES;n++) {
        Note note=project.notes[p][c][n]; if(!note.velocity || note.start>=project.pattern_steps[p]) continue;
        float px=x+4+104*note.start/span;
        float length=fmaxf(1,104*fminf(note.length?note.length:1,span-note.start)/span-1);
        float fraction=high>low?(high-note.pitch)/(float)(high-low):.5f;
        float py=y+20+fraction*(26-note_height);
        DrawRectangleRec((Rectangle){px,py,length,note_height},content);
    }
}
static Color source_color(int source) {
    return source>=AUTOMATION_SOURCE?pattern_rgb(project.automations[source-AUTOMATION_SOURCE].color):AUDIO_SOURCE(source)?audio_color(source-PATTERNS):pattern_color(source);
}
static const char *source_name(int source) {
    return source>=AUTOMATION_SOURCE?project.automations[source-AUTOMATION_SOURCE].name:AUDIO_SOURCE(source)?channel_caption(source-PATTERNS):project.pattern_names[source];
}
static void automation_curve(int a,float origin,float y,float pixels,float height,float left,float right,int nodes) {
    Color content=clip_foreground(source_color(AUTOMATION_SOURCE+a));
    const Automation *automation=&project.automations[a];
    float top=y+19,range=fmaxf(1,height-25);
    if(automation->count) {
        float first=origin+automation->points[0].step*pixels,last=origin+automation->points[automation->count-1].step*pixels;
        float first_y=top+(1-automation->points[0].value)*range,last_y=top+(1-automation->points[automation->count-1].value)*range;
        if(first>left) smooth_line((Vector2){left,first_y},(Vector2){fminf(right,first),first_y},1.5f,content);
        if(last<right) smooth_line((Vector2){fmaxf(left,last),last_y},(Vector2){right,last_y},1.5f,content);
    }
    for(int i=1;i<automation->count;i++) {
        AutomationPoint p=automation->points[i-1],q=automation->points[i];
        float x=origin+p.step*pixels,xx=origin+q.step*pixels;
        if(xx<left || x>right) continue;
        if(xx==x) {
            if(x>=left && x<=right) smooth_line((Vector2){x,top+(1-p.value)*range},(Vector2){x,top+(1-q.value)*range},1.5f,content);
            continue;
        }
        float l=fmaxf(left,x),r=fminf(right,xx);
        float lv=p.value+(q.value-p.value)*(l-x)/(xx-x),rv=p.value+(q.value-p.value)*(r-x)/(xx-x);
        smooth_line((Vector2){l,top+(1-lv)*range},(Vector2){r,top+(1-rv)*range},1.5f,content);
    }
    if(nodes) for(int i=0;i<automation->count;i++) {
        float x=origin+automation->points[i].step*pixels,yy=top+(1-automation->points[i].value)*range;
        if(x>=left && x<=right+2) { x=fmaxf(left+4,fminf(right-4,x)); circle(x,yy,4,content); circle(x,yy,2,source_color(AUTOMATION_SOURCE+a)); }
    }
}
static void draw_picker_drag(void) {
    if(picker_drag<0 || (fabsf(mouse.x-picker_origin.x)<3 && fabsf(mouse.y-picker_origin.y)<3)) return;
    int source=picker_drag,audio=AUDIO_SOURCE(source);
    float x=mouse.x-picker_offset.x,y=mouse.y-picker_offset.y,steps=clip_source_steps(&project,source);
    Color color=source_color(source);
    ui_surface((Rectangle){x,y,112,50},color);
    if(audio) audio_waveform(source-PATTERNS,x+4,y+20,x+4,x+108,104/fmaxf(.001f,steps),28);
    else if(source>=AUTOMATION_SOURCE) automation_curve(source-AUTOMATION_SOURCE,x+4,y,104/steps,50,x+4,x+108,0);
    else picker_notes(source,x,y);
    label(fit_text(source_name(source),100,12),x+4,y+4,12,clip_foreground(color));
    DrawRectangleLinesEx((Rectangle){x,y,112,50},2,accent);
}
static void draw_browser_drag(void) {
    if(!sample_drag[0] || !sample_moved) return;
    float x=mouse.x-sample_offset.x,y=mouse.y-sample_offset.y;
    ui_surface((Rectangle){x,y,sample_drag_width,21},accent);
    label(fit_text(GetFileName(sample_drag),fmaxf(0,sample_drag_width-sample_text_offset-2),13),x+sample_text_offset,y+4,13,ui_theme.selected_text);
}
/* Icons use raylib primitives, without a font or image dependency. */
static int tool_button(int tool,int active,int x,const char *tip) {
    int over=hover(x,28,20,20);
    if(over) SetMouseCursor(MOUSE_CURSOR_POINTING_HAND);
    Color c=active==tool?ui_theme.selected_text:ink;
    ui_surface((Rectangle){x,28,20,20},active==tool?accent:over?ui_theme.hover:cell);
    icon(tool==PENCIL?ICON_PENCIL:tool==BRUSH?ICON_BRUSH:tool==CUT?ICON_CUT:tool==STRETCH?ICON_STRETCH:ICON_SELECT,x+10,38,20,c);
    if(over) snprintf(status,sizeof status,"%s",tip);
    return over && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
}
static void row_zoom_update(float y) {
    float zoom=row_zoom_start*expf((row_zoom_y-y)/160);
    if(row_zoom_drag==1) { track_zoom=fmaxf(.4f,fminf(4,zoom)); track_scroll=track_position(row_zoom_anchor); }
    else if(row_zoom_drag==2) { piano_zoom=fmaxf(25.f/128,fminf(25.f/8,zoom)); piano_top=fmaxf(piano_min_top(),piano_top); piano_scroll_remainder=0; }
}
static void row_zoom_button(int id,float width,float top) {
    float x=width-4-EDITOR_CORNER;
    int over=hover(x,top,EDITOR_CORNER,EDITOR_CORNER),active=over || row_zoom_drag==id;
    ui_surface((Rectangle){x,top,EDITOR_CORNER,EDITOR_CORNER},row_zoom_drag==id?(over?ui_theme.active_hover:accent):over?ui_theme.hover:cell);
    if(over && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        row_zoom_drag=id; row_zoom_y=mouse.y+windows.editors[id].rect.y;
        row_zoom_start=id==1?track_zoom:piano_zoom; row_zoom_anchor=track_at(track_scroll); input_enabled=0;
    }
    icon(ICON_ROW_ZOOM,x+EDITOR_CORNER/2,top+EDITOR_CORNER/2,16,muted);
    if(active) { SetMouseCursor(MOUSE_CURSOR_RESIZE_NS); snprintf(status,sizeof status,"Vertical zoom: drag up for taller %s, down to show more",id==1?"tracks":"notes"); }
}
static void ruler_press(int id,int button,float at,double time,Vector2 position) {
    float *range=id==1?song_loop:pattern_loop[pattern];
    int twice=ruler_last_id==id && (id==1 || ruler_last_pattern==pattern) && ruler_last_button==button &&
        time-ruler_last_click<.3 && fabsf(position.x-ruler_last_position.x)<6 && fabsf(position.y-ruler_last_position.y)<6;
    ruler_last_id=id; ruler_last_pattern=pattern; ruler_last_button=button; ruler_last_click=time; ruler_last_position=position;
    stop_armed=0;
    if(song!=(id==1)) { song=id==1; reset=1; }
    if(twice) { range[0]=range[1]=0; marker_drag=-1; ruler_last_id=-1; return; }
    marker_drag=id; ruler_loop_drag=button==MOUSE_BUTTON_RIGHT;
    if(ruler_loop_drag) {
        ruler_anchor=range[1]>range[0]?(at<(range[0]+range[1])/2?range[1]:range[0]):at;
        range[0]=fminf(ruler_anchor,at); range[1]=fmaxf(ruler_anchor,at);
        if(range[1]>range[0]) { if(id==1) playlist_start=range[0]/STEPS; else piano_start[pattern]=range[0]; }
    } else if(id==1) playlist_start=at/STEPS; else piano_start[pattern]=at;
    if(!playing) reset=1;
}
/* All three editors use steps internally and the same ruler appearance. */
static void timeline_ruler(int id,float start,float span,float gx,float y,float width,float q) {
    float *range=id==1?song_loop:pattern_loop[pattern],point=id==1?playlist_start*STEPS:piano_start[pattern];
    float pixels=width/span;
    DrawRectangle(gx,y,width,14,ui_theme.browser);
    if(range[1]>range[0]) {
        float left=fmaxf(gx,gx+(range[0]-start)*pixels),right=fminf(gx+width,gx+(range[1]-start)*pixels);
        if(right>left) DrawRectangleRec((Rectangle){left,y,right-left,14},Fade(ui_theme.loop,.6f));
    }
    float spacing=timeline_grid_layout(pixels).labels;
    float label_width=text_width(TextFormat("%.0f",fmaxf(0,start+span)/STEPS+1),11)+8;
    while(spacing*pixels<label_width && spacing<1e30f) spacing*=2;
    for(int i=0;i<width/(spacing*pixels)+2;i++) {
        float step=(floorf(start/spacing)+i)*spacing,x=gx+(step-start)*pixels;
        if(step>=0 && x>=gx && x<gx+width) {
            const char *number=TextFormat("%.0f",step/STEPS+1);
            int major=fmodf(step,STEPS*4)==0,size=major?11:10;
            if(x+3+text_width(number,size)<=gx+width) label(number,x+3,y+1,size,major?ink:muted);
        }
    }
    if(hover(gx,y,width,14)) {
        snprintf(status,sizeof status,"Ruler: left-click/drag sets playback start; right-click/drag adjusts the nearest loop edge; double-click clears loop");
        int right=IsMouseButtonPressed(MOUSE_BUTTON_RIGHT);
        if(right || IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            float at=fmaxf(0,snap_round(start+(mouse.x-gx)/pixels,q));
            ruler_press(id,right?MOUSE_BUTTON_RIGHT:MOUSE_BUTTON_LEFT,at,GetTime(),mouse);
            input_enabled=0;
        }
    }
    float x=gx+(point-start)*pixels;
    if(x>=gx && x<=gx+width) DrawTriangle((Vector2){x-5,y+1},(Vector2){x,y+12},(Vector2){x+5,y+1},marker_drag==id?ink:ui_theme.signal);
}
static void timeline_grid(float start,float span,float gx,float y,float width,float height) {
    float pixels=width/span;
    TimelineGrid grid=timeline_grid_layout(pixels);
    /* Keep phrase shading anchored to bar 1, including while panning between bars. */
    float band=STEPS*4;
    if(grid.band_alpha>0) for(int i=0;i<span/band+2;i++) {
        float step=(floorf(start/band)+i)*band;
        if(step<0 || fmodf(step/band,2)==0) continue;
        float left=fmaxf(gx,gx+(step-start)*pixels),right=fminf(gx+width,gx+(step+band-start)*pixels);
        if(right>left) DrawRectangleRec((Rectangle){left,y,right-left,height},Fade(ui_theme.light?WHITE:BLACK,grid.band_alpha));
    }
    if(!grid.lines) return;
    for(int i=0;i<span/grid.lines+2;i++) {
        float step=(floorf(start/grid.lines)+i)*grid.lines,x=gx+(step-start)*pixels;
        int bar=fmodf(step,STEPS)==0,beat=fmodf(step,4)==0;
        if(step>=0 && x>=gx && x<=gx+width)
            DrawLineEx((Vector2){x,y},(Vector2){x,y+height},1,bar?ui_theme.grid_major:Fade(ui_theme.grid_minor,beat?1:.65f));
    }
}
static void follow_view(float *start,float span,double position) {
    if(!follow_playhead || !playing || navigation_active || captured() || windows.grab>=0) return;
    *start=position-span*.5;
}
static float track_flash[LANES];
static uint8_t track_active_ui[LANES];
static void track_activity_update(void) {
    audio_pattern_activity(pattern,playing_notes); memset(playing_keys,0,sizeof playing_keys);
    for(int c=0;c<project.channel_count;c++) for(int n=0;n<NOTES;n++) if(playing_notes[c][n] && project.notes[pattern][c][n].velocity) playing_keys[c][project.notes[pattern][c][n].pitch]=1;
    uint8_t channel_triggered[CHANNELS]; audio_channel_activity(channel_active_ui,channel_triggered);
    channel_decay_active=0;
    for(int c=0;c<CHANNELS;c++) {
        channel_flash[c]=channel_triggered[c]?1:channel_flash[c]*expf(-GetFrameTime()*20);
        channel_decay_active|=channel_active_ui[c] || channel_flash[c]>.005f;
    }
    uint8_t triggered[LANES]; audio_track_activity(track_active_ui,triggered);
    float decay=expf(-GetFrameTime()*20);
    for(int l=0;l<LANES;l++) {
        if(!playing || !song) { track_flash[l]=0; track_active_ui[l]=0; }
        else track_flash[l]=triggered[l]?1:track_flash[l]*decay;
    }
}
static int clip_edge(int lane,int clip,float bar,float barw) {
    if(clip<0) return 0;
    float left=(bar-project.clip_starts[lane][clip])*barw,right=clip_length(&project,lane,clip)/STEPS*barw-left;
    return fminf(left,right)<7?(left<right?-1:1):0;
}
static int picker_double_click(int source) {
    static double last_click=-1;
    static int last_source=-1;
    static unsigned epoch;
    static Vector2 position;
    double now=GetTime();
    int twice=epoch==sample_epoch && source==last_source && now-last_click<.35 &&
        fabsf(mouse.x-position.x)<5 && fabsf(mouse.y-position.y)<5;
    last_click=twice?-1:now; last_source=source; position=mouse; epoch=sample_epoch;
    if(twice) {
        if(source<PATTERNS) { rack_filter=0; rack_scroll=0; windows_focus(&windows,0); }
        else if(AUDIO_SOURCE(source)) { channel=instrument_channel=source-PATTERNS; windows_focus(&windows,4); }
        else automation_selected=source-AUTOMATION_SOURCE;
        picker_drag=-1; browser_focus=0; input_enabled=0;
    }
    return twice;
}
static void playlist(float width,float height,float scale) {
    static double last_click=-1;
    static int last_lane=-1,last_clip=-1;
    static Vector2 last_position;
    Rect rect=windows.editors[1].rect;
    int gx=212,gy=PLAYLIST_GRID_TOP; float gridw=width-gx-24,track_area=height-gy-PLAYLIST_BOTTOM,total_height=track_position(LANES);
    track_scroll=fmaxf(0,fminf(total_height-track_area,track_scroll));
    float span=BARS/arrangement.zoom,barw=gridw/span;
    if(song) follow_view(&arrangement.view_start,span,visual_step/STEPS);
    arrangement.range=fmaxf(arrangement.range,fmaxf(arrangement.view_start+span*2,song_steps(&project)/STEPS+span));
    arrangement.snap=grid_interval(barw/STEPS);
    float pointer_bar=arrangement.view_start+(mouse.x-gx)/barw;
    int divider=-1;
    if(hover(120,gy,gx-120,track_area)) {
        float pos=track_scroll+mouse.y-gy; int lane=fminf(LANES-1,fmaxf(0,track_at(pos)));
        if(pos-track_position(lane)<3 && lane>0) divider=lane-1;
        else if(track_position(lane+1)-pos<3) divider=lane;
        if(divider>=0) {
            SetMouseCursor(MOUSE_CURSOR_RESIZE_NS); snprintf(status,sizeof status,"Drag divider to resize Track %d",divider+1);
            if(IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) { track_resize=divider; track_resize_y=mouse.y+rect.y; track_resize_height=track_height(divider); input_enabled=0; }
        }
    }
    if(divider<0 && hover(gx,gy,gridw,track_area)) {
        int lane=track_at(track_scroll+mouse.y-gy),hit=arrangement_hit(&project,lane,pointer_bar);
        if(arrangement.tool!=CUT && !shortcut_down() && hit>=0 && !recording_source(project.clips[lane][hit]-1) && clip_edge(lane,hit,pointer_bar,barw)) SetMouseCursor(MOUSE_CURSOR_RESIZE_EW);
    }
    if(arrangement.gesture==SIZE_CLIP || arrangement.gesture==STRETCH_CLIP) SetMouseCursor(MOUSE_CURSOR_RESIZE_EW);
    if(track_resize>=0) SetMouseCursor(MOUSE_CURSOR_RESIZE_NS);
    const char *tips[]={"Pencil: place one clip or drag a clip to move it","Brush: drag to paint copies of the last clicked clip","Select: drag a rectangle, then drag the selected clips together","Cut: click to split a clip at the snap position","Stretch: drag an audio edge; uses the sampler Resample/Stretch mode"};
    for(int i=0;i<5;i++) if(tool_button(i,arrangement.tool,8+i*24,tips[i])) arrangement.tool=i;

    const char *picker_names[]={"Patterns","Audio clips","Automation"};
    const int picker_icons[]={ICON_PIANO,ICON_WAVE,ICON_AUTOMATION};
    for(int t=0;t<3;t++) {
        if(button("",4+t*38,57,36,23,picker_tab==t)) { picker_tab=t; picker_scroll=0; picker_drag=-1; if(t==0) { arrangement.source_pattern=pattern; arrangement.source_steps=project.pattern_steps[pattern]; arrangement.source_offset=0; } }
        icon(picker_icons[t],22+t*38,68,18,picker_tab==t?ui_theme.selected_text:ink);
        if(hover(4+t*38,57,36,23)) snprintf(status,sizeof status,"%s%s",picker_names[t],"");
    }
    label("Tracks",128,63,12,muted);
    int audio_ids[CHANNELS],audio_count=0;
    for(int c=0;c<project.channel_count;c++) if(project.channel_audio[c]) audio_ids[audio_count++]=c;
    int picker_count=picker_tab==0?project.pattern_count:picker_tab==1?audio_count:project.automation_count;
    int picker_visible=fmaxf(1,(height-66-gy)/52);
    if(hover(4,gy,112,height-66-gy)) picker_scroll-=GetMouseWheelMove();
    picker_scroll=fmaxf(0,fminf(fmaxf(0,picker_count-picker_visible),picker_scroll));
    for(int row=picker_scroll;row<picker_count;row++) {
        int p=picker_tab==1?audio_ids[row]:row;
        int y=gy+(row-picker_scroll)*52; if(y+50>height-66) break;
        if(picker_tab==2) {
            int source=AUTOMATION_SOURCE+p;
            if(picker_button(y,source_color(source),automation_selected==p)) {
                automation_selected=p; arrangement.source_pattern=source; arrangement.source_steps=project.automations[p].steps; arrangement.source_offset=0; picker_drag=source;
                picker_origin=(Vector2){mouse.x+rect.x,mouse.y+rect.y}; picker_offset=(Vector2){mouse.x-4,mouse.y-y};
            }
            label(fit_text(source_name(source),100,12),8,y+4,12,clip_foreground(source_color(source)));
            automation_curve(p,8,y,104/project.automations[p].steps,50,8,112,0);
            if(hover(4,y,112,50)) {
                snprintf(status,sizeof status,"Automation: select or drag to place; right-click for Rename, Color or Delete");
                if(IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) open_context(11,p,(Vector2){mouse.x+rect.x,mouse.y+rect.y});
            }
            continue;
        }
        if(picker_tab==1) {
            if(picker_button(y,audio_color(p),arrangement.source_pattern==PATTERNS+p) && !recording_source(PATTERNS+p)) {
                arrangement.source_pattern=PATTERNS+p; arrangement.source_steps=project.audio_seconds[p]; arrangement.source_offset=0; picker_drag=PATTERNS+p;
                picker_origin=(Vector2){mouse.x+rect.x,mouse.y+rect.y};
                picker_offset=(Vector2){mouse.x-4,mouse.y-y};
                picker_double_click(PATTERNS+p);
            }
            if(arrangement.source_pattern==PATTERNS+p) DrawRectangleLinesEx((Rectangle){4,y,112,50},2,accent);
            label(fit_text(project.channel_names[p],100,12),8,y+4,12,clip_foreground(audio_color(p)));
            audio_waveform(p,8,y+20,8,112,104/fmaxf(.001f,audio_view_steps(p)),28);
            if(hover(4,y,112,50)) {
                snprintf(status,sizeof status,"Audio clip: double-click for sampler; select or drag to place; right-click for Rename, Color or Delete");
                if(IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) open_context(7,p,(Vector2){mouse.x+rect.x,mouse.y+rect.y});
            }
            continue;
        }
        if(picker_button(y,pattern_color(p),pattern==p)) {
            if(pattern!=p) reset=1;
            pattern=p; arrangement.source_pattern=p; arrangement.source_steps=project.pattern_steps[p]; arrangement.source_offset=0; picker_drag=p;
            picker_origin=(Vector2){mouse.x+rect.x,mouse.y+rect.y};
            picker_offset=(Vector2){mouse.x-4,mouse.y-y};
            picker_double_click(p);
        }
        if(pattern==p) DrawRectangleLinesEx((Rectangle){4,y,112,50},2,accent);
        label(fit_text(project.pattern_names[p],100,12),8,y+4,12,clip_foreground(pattern_color(p)));
        picker_notes(p,4,y);
        if(hover(4,y,112,50)) {
            snprintf(status,sizeof status,"Pattern: double-click for Channel Rack; select or drag to place; right-click for Rename, Color or Delete");
            if(IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) { pattern=p; arrangement.source_pattern=p; arrangement.source_steps=project.pattern_steps[p]; reset=1; open_context(4,p,(Vector2){mouse.x+rect.x,mouse.y+rect.y}); }
        }
    }
    if(sample_drag[0] && sample_moved && CheckCollisionPointRec(mouse,(Rectangle){4,gy,112,height-gy-66}) && windows_hit(&windows,mouse.x+rect.x,mouse.y+rect.y)==1) {
        DrawRectangleLinesEx((Rectangle){4,gy,112,height-gy-66},2,ui_theme.signal);
        snprintf(status,sizeof status,"Release to import into the Audio list without placing a clip");
    }
    if(picker_tab==2 && !project.automation_count) { label("No automation",8,gy+8,12,muted); label("Right-click a control",8,gy+28,11,muted); }
    if(picker_tab==1 && !audio_count) { label("No audio clips",8,gy+8,12,muted); label("Drop on Playlist",8,gy+28,11,muted); }
    if(picker_tab==0 && button("+",4,height-62,112,20,0)) {
        if(project.pattern_count<PATTERNS) {
            pattern=project.pattern_count++; arrangement.source_pattern=pattern; arrangement.source_steps=STEPS; memset(project.notes[pattern],0,sizeof project.notes[pattern]);
            project.pattern_steps[pattern]=STEPS; memset(piano_channels[pattern],0,sizeof piano_channels[pattern]);
            for(int n=1;n<=PATTERNS+1;n++) {
                snprintf(project.pattern_names[pattern],PATTERN_NAME,"Pattern %d",n);
                int duplicate=0;
                for(int i=0;i<pattern;i++) if(!strcmp(project.pattern_names[i],project.pattern_names[pattern])) duplicate=1;
                if(!duplicate) break;
            }
            project.pattern_colors[pattern]=next_source_color(project.pattern_colors,pattern);
            rack_view[pattern]=rack_range[pattern]=0; reset=1;
            picker_scroll=fmaxf(0,project.pattern_count-picker_visible);
        } else snprintf(status,sizeof status,"This prototype supports %d patterns.",PATTERNS);
    }
    if(picker_tab==0 && hover(4,height-62,112,20) && project.pattern_count<PATTERNS) snprintf(status,sizeof status,"Add a new blank pattern");
    timeline_ruler(1,arrangement.view_start*STEPS,span*STEPS,gx,gy-14,gridw,arrangement.snap);
    for(int l=fmaxf(0,track_at(track_scroll));l<LANES;l++) {
        float y=gy+track_position(l)-track_scroll,rowh=track_height(l);
        if(y>=height-PLAYLIST_BOTTOM) break;
        BeginScissorMode(rect.x*scale,(rect.y+gy)*scale,gx*scale,track_area*scale);
        DrawRectangleRec((Rectangle){120,y,gx-122,rowh-1},cell); label(fit_text(project.track_names[l],gx-142,12),128,y+fmaxf(1,(rowh-12)/2),12,ink);
        float brightness=fmaxf(track_active_ui[l]?.32f:0,track_flash[l]);
        Rectangle activity={gx-9,y+1,8,rowh-2};
        DrawRectangleRec(activity,ui_theme.track);
        if(brightness>.005f) DrawRectangleRec(activity,Fade(ui_theme.light?BLACK:WHITE,brightness));
        if(selected_track==l) DrawRectangleLinesEx(activity,1,accent);
        mute_light(gx-18,y+rowh-10,project.lane_mute,LANES,l,TextFormat("Playlist track %d",l+1));
        BeginScissorMode((rect.x+gx)*scale,(rect.y+gy)*scale,gridw*scale,track_area*scale);
        DrawRectangleRec((Rectangle){gx,y,gridw,rowh},ui_theme.track);
        timeline_grid(arrangement.view_start*STEPS,span*STEPS,gx,y,gridw,rowh-1);
        for(int b=0;b<CLIPS;b++) if(project.clips[l][b]) {
            int pat=project.clips[l][b]-1; float x=gx+(project.clip_starts[l][b]-arrangement.view_start)*barw,w=playlist_clip_length(l,b)*barw/STEPS;
            if(w<=0 || x>gx+gridw || x+w<gx) continue;
            float left=fmaxf(gx,x),right=fminf(gx+gridw,x+w);
            Rectangle bounds={left,y,fmaxf(1,right-left),rowh-1};
            float top=fmaxf(gy,y),bottom=fminf(height-PLAYLIST_BOTTOM,y+rowh-1);
            DrawRectangleRec(bounds,source_color(pat)); DrawRectangleRec((Rectangle){left,y,bounds.width,14},Fade(clip_foreground(source_color(pat)),.06f));
            const char *name=source_name(pat); if(pat<PATTERNS && !strcmp(name,TextFormat("Pattern %d",pat+1))) name=TextFormat("P%d",pat+1);
            if(x>=gx) label(fit_text(name,fminf(gridw,w-9),10),x+3,y+2,10,clip_foreground(source_color(pat)));
            float clip_left=floorf((rect.x+left)*scale),clip_top=floorf((rect.y+top)*scale);
            /* Cached pattern previews already crop their geometry to the clip.
               Keep per-clip scissors for curves, waveforms and vector fallbacks. */
            int clip_scissor=pat>=PATTERNS || !previews[pat].valid;
            if(clip_scissor) BeginScissorMode(clip_left,clip_top,ceilf((rect.x+left+bounds.width)*scale)-clip_left,fmaxf(0,ceilf((rect.y+bottom)*scale)-clip_top));
            float offset=clip_offset_steps(&project,l,b),origin=x-offset*barw/STEPS;
            if(pat>=AUTOMATION_SOURCE) automation_curve(pat-AUTOMATION_SOURCE,origin,y,barw/STEPS,rowh,left,right,1);
            else if(AUDIO_SOURCE(pat)) audio_waveform_at(pat-PATTERNS,x,y,left,right,barw/STEPS,rowh,l,b);
            else clip_preview(pat,origin,y,left,right,barw/STEPS,clip_length(&project,l,b)+offset,rowh);
            if(clip_scissor) BeginScissorMode((rect.x+gx)*scale,(rect.y+gy)*scale,gridw*scale,track_area*scale);
            if(arrangement.gesture==MOVE_CLIPS?arrangement.moved[l][b]:arrangement.selected[l][b]) DrawRectangleLinesEx(bounds,2,ui_theme.signal);
        }
        BeginScissorMode(rect.x*scale,rect.y*scale,rect.w*scale,rect.h*scale);
    }
    BeginScissorMode((rect.x+gx)*scale,(rect.y+gy)*scale,gridw*scale,track_area*scale);
    for(int l=fmaxf(0,track_at(track_scroll));l<LANES;l++) {
        float y=gy+track_position(l)-track_scroll,rowh=track_height(l);
        if(y>=height-PLAYLIST_BOTTOM) break;
        float bottom=roundf((y+rowh)*scale+text_origin.y);
        DrawRectangleRec((Rectangle){gx,(bottom-1-text_origin.y)/scale,gridw,1/scale},ui_theme.grid_major);
        BeginScissorMode((rect.x+120)*scale,(rect.y+gy)*scale,(gx-120)*scale,track_area*scale);
        DrawRectangleRec((Rectangle){120,(bottom-1-text_origin.y)/scale,gx-120,1/scale},divider==l || track_resize==l?accent:ui_theme.border);
        BeginScissorMode((rect.x+gx)*scale,(rect.y+gy)*scale,gridw*scale,track_area*scale);
    }
    BeginScissorMode(rect.x*scale,rect.y*scale,rect.w*scale,rect.h*scale);
    if(picker_drag>=0 && IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
        if(CheckCollisionPointRec(mouse,(Rectangle){gx,gy,gridw,track_area}) && windows_hit(&windows,mouse.x+rect.x,mouse.y+rect.y)==1) {
            float start=snap_floor(pointer_bar*STEPS,arrangement.snap)/STEPS;
            arrangement_place(&project,track_at(track_scroll+mouse.y-gy),start,picker_drag,clip_source_steps(&project,picker_drag));
        }
        picker_drag=-1;
    }
    /* Curve editing owns the body; the name strip keeps normal clip gestures. */
    if(automation_node>=0) {
        if(automation_selected<0 || automation_selected>=project.automation_count || automation_lane<0 || automation_clip<0 || project.clips[automation_lane][automation_clip]!=AUTOMATION_SOURCE+automation_selected+1) automation_node=-1;
        else {
            Automation *a=&project.automations[automation_selected];
            float y=gy+track_position(automation_lane)-track_scroll,rowh=track_height(automation_lane);
            float origin=gx+(project.clip_starts[automation_lane][automation_clip]-arrangement.view_start)*barw-clip_offset_steps(&project,automation_lane,automation_clip)*barw/STEPS;
            if(IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
                float step=(mouse.x-automation_grab_x-origin)/barw*STEPS;
                step=snap_round(step,arrangement.snap);
                int lane=automation_lane,clip=automation_clip;
                float offset=project.clip_offsets[lane][clip],length=clip_length(&project,lane,clip);
                step=fmaxf(offset-project.clip_starts[lane][clip]*STEPS,step);
                float value=fmaxf(0,fminf(1,1-(mouse.y-y-19)/fmaxf(1,rowh-25)));
                int moved=automation_move_point(&project,automation_selected,automation_node,step,value);
                if(moved>=0) {
                    automation_node=moved; step=a->points[moved].step; offset=project.clip_offsets[lane][clip];
                    if(step<offset) { float delta=offset-step; project.clip_starts[lane][clip]-=delta/STEPS; project.clip_offsets[lane][clip]=step; length+=delta; }
                    else if(step>offset+length) length=step-offset;
                    project.clip_steps[lane][clip]=length;
                    arrangement.source_pattern=AUTOMATION_SOURCE+automation_selected; arrangement.source_steps=length; arrangement.source_offset=project.clip_offsets[lane][clip];
                }
            } else automation_node=-1;
        }
    }
    if(automation_node<0 && arrangement.tool==PENCIL && !shortcut_down() && !IsKeyDown(KEY_LEFT_SHIFT) && !IsKeyDown(KEY_RIGHT_SHIFT) && hover(gx,gy,gridw,track_area)) {
        int lane=track_at(track_scroll+mouse.y-gy),slot=arrangement_hit(&project,lane,pointer_bar);
        float y=gy+track_position(lane)-track_scroll,rowh=track_height(lane);
        if(slot>=0 && project.clips[lane][slot]>AUTOMATION_SOURCE && mouse.y>=y+15) {
            int index=project.clips[lane][slot]-AUTOMATION_SOURCE-1; Automation *a=&project.automations[index];
            float origin=gx+(project.clip_starts[lane][slot]-arrangement.view_start)*barw-clip_offset_steps(&project,lane,slot)*barw/STEPS;
            int hit=-1; float nearest=49;
            for(int n=0;n<a->count;n++) {
                float dx=mouse.x-origin-a->points[n].step*barw/STEPS,dy=mouse.y-y-19-(1-a->points[n].value)*fmaxf(1,rowh-25),distance=dx*dx+dy*dy;
                if(distance<nearest) { hit=n; nearest=distance; }
            }
            snprintf(status,sizeof status,"Automation: click to add a point; drag points; right-click a point to delete; drag the title to move the clip");
            if(hit>=0 || !clip_edge(lane,slot,pointer_bar,barw)) {
                if(IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                    automation_selected=index;
                    if(hit<0) {
                                float step=(mouse.x-origin)/barw*STEPS;
                        step=snap_round(step,arrangement.snap);
                        hit=automation_point(a,fmaxf(0,fminf(a->steps,step)),fmaxf(0,fminf(1,1-(mouse.y-y-19)/fmaxf(1,rowh-25))));
                    }
                    automation_node=hit; automation_lane=lane; automation_clip=slot;
                    if(hit>=0) automation_grab_x=mouse.x-origin-a->points[hit].step*barw/STEPS;
                    memset(arrangement.selected,0,sizeof arrangement.selected); arrangement.selected[lane][slot]=1; input_enabled=0;
                }
                if(IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
                    if(hit>0 && hit<a->count-1) { memmove(&a->points[hit],&a->points[hit+1],(a->count-hit-1)*sizeof a->points[0]); a->count--; }
                    input_enabled=0;
                }
            }
        }
    }
    if(divider<0 && hover(120,gy,gx+gridw-120,track_area)) {
        float bx=mouse.x<gx?-1:arrangement.view_start+(mouse.x-gx)/barw,ly=track_at(track_scroll+mouse.y-gy); int hit=arrangement_hit(&project,(int)ly,bx);
        if(bx<0 && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) selected_track=(int)ly;
        int live=hit>=0 && recording_source(project.clips[(int)ly][hit]-1);
        int edge=shortcut_down() || live || arrangement.tool==CUT?0:clip_edge((int)ly,hit,bx,barw);
        if(edge) { SetMouseCursor(MOUSE_CURSOR_RESIZE_EW); snprintf(status,sizeof status,"%.100s · %.2f s | Drag edge to resize",source_name(project.clips[(int)ly][hit]-1),playlist_clip_length((int)ly,hit)*15/project.bpm); }
        else if(mouse.x<gx) snprintf(status,sizeof status,"Track %d: drag to select tracks; right-click to rename",(int)ly+1);
        else if(hit>=0) {
            int source=project.clips[(int)ly][hit]-1;
            snprintf(status,sizeof status,"%.100s · %.2f s | %s",source_name(source),playlist_clip_length((int)ly,hit)*15/project.bpm,source>=AUTOMATION_SOURCE?"Drag title to move; body to edit":AUDIO_SOURCE(source)?"Double-click: Sampler; drag: move; right-click: erase":"Double-click: Channel Rack; drag: move; right-click: erase");
        }
        else snprintf(status,sizeof status,"Playlist: %s; Shift adds to selection, right-drag erases",arrangement.tool==PENCIL?"place or move a clip":arrangement.tool==BRUSH?"paint copies of the current pattern":"select and move clips");
        if(mouse.x>=gx && arrangement.tool==CUT) snprintf(status,sizeof status,"Cut: click a clip to split at the snap position");
        if(mouse.x>=gx && arrangement.tool==STRETCH) snprintf(status,sizeof status,"Stretch: drag either audio edge; Resample changes pitch, Stretch preserves pitch");
        if(mouse.x<gx && IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
            rename_channel=rename_mixer=-1; rename_track=(int)ly; snprintf(rename_text,sizeof rename_text,"%s",project.track_names[rename_track]); rename_select_all=1; open_popup(2); input_enabled=0; return;
        }
        if(!live && mouse.x>=gx && hit>=0 && IsMouseButtonPressed(MOUSE_BUTTON_RIGHT) &&
           mouse.y<gy+track_position((int)ly)-track_scroll+14) {
            int source=project.clips[(int)ly][hit]-1;
            if(source<PATTERNS) { pattern=source; open_context(4,source,(Vector2){mouse.x+rect.x,mouse.y+rect.y}); }
            else if(AUDIO_SOURCE(source)) open_context(7,source-PATTERNS,(Vector2){mouse.x+rect.x,mouse.y+rect.y});
            else open_context(11,source-AUTOMATION_SOURCE,(Vector2){mouse.x+rect.x,mouse.y+rect.y});
            arrangement.gesture=IDLE; last_click=-1; return;
        }
        if(shortcut_down() && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            arrangement_select_press(&arrangement,bx,ly,IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT));
            last_click=-1; input_enabled=0;
        } else if(!live && !recording_source(arrangement.source_pattern) && (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) || IsMouseButtonPressed(MOUSE_BUTTON_RIGHT))) {
            int plain_left=arrangement.tool!=CUT && arrangement.tool!=STRETCH && IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && !IsKeyDown(KEY_LEFT_SHIFT) && !IsKeyDown(KEY_RIGHT_SHIFT) && hit>=0 && (!edge || project.clips[(int)ly][hit]>PATTERNS);
            double now=GetTime();
            if(plain_left && last_lane==(int)ly && last_clip==hit && now-last_click<.35 && fabsf(mouse.x-last_position.x)<5 && fabsf(mouse.y-last_position.y)<5) {
                int source=project.clips[(int)ly][hit]-1;
                if(source<PATTERNS) { pattern=source; reset=1; }
                else if(AUDIO_SOURCE(source)) { channel=instrument_channel=source-PATTERNS; rack_filter=1; rack_scroll=0; }
                else automation_selected=source-AUTOMATION_SOURCE;
                arrangement.gesture=IDLE; windows_focus(&windows,source>=AUTOMATION_SOURCE?1:AUDIO_SOURCE(source)?4:0); last_click=-1;
                input_enabled=0; return;
            }
            last_click=plain_left?now:-1; last_lane=(int)ly; last_clip=hit; last_position=mouse;
            arrangement_press(&arrangement,&project,bx,ly,IsMouseButtonPressed(MOUSE_BUTTON_RIGHT),edge,pattern,IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT));
            if(arrangement.source_pattern>=0 && arrangement.source_pattern<PATTERNS && arrangement.source_pattern!=pattern) { pattern=arrangement.source_pattern; reset=1; }
            input_enabled=0;
        }
    }
    BeginScissorMode((rect.x+gx)*scale,(rect.y+gy)*scale,gridw*scale,(height-gy-PLAYLIST_BOTTOM)*scale);
    if(arrangement.tool==CUT && !shortcut_down() && !IsKeyDown(KEY_LEFT_SHIFT) && !IsKeyDown(KEY_RIGHT_SHIFT) && hover(gx,gy,gridw,track_area)) {
        int lane=track_at(track_scroll+mouse.y-gy),hit=arrangement_hit(&project,lane,pointer_bar);
        if(hit>=0 && !recording_source(project.clips[lane][hit]-1)) {
            float cut=snap_floor(pointer_bar*STEPS+.00001f,arrangement.snap);
            float start=project.clip_starts[lane][hit]*STEPS,end=start+clip_length(&project,lane,hit);
            if(cut>start+.0001f && cut<end-.0001f) {
                float x=gx+(cut/STEPS-arrangement.view_start)*barw,y=gy+track_position(lane)-track_scroll;
                x=roundf((rect.x+x)*scale)/scale-rect.x;
                DrawRectangleRec((Rectangle){x,y,1/scale,track_height(lane)-1},ui_theme.signal);
            }
        }
    }
    if(picker_drag>=0 && CheckCollisionPointRec(mouse,(Rectangle){gx,gy,gridw,track_area}) && windows_hit(&windows,mouse.x+rect.x,mouse.y+rect.y)==1) {
        float start=snap_floor(pointer_bar*STEPS,arrangement.snap)/STEPS;
        int lane=track_at(track_scroll+mouse.y-gy);
        Rectangle ghost={gx+(start-arrangement.view_start)*barw,gy+track_position(lane)-track_scroll,fmaxf(4,clip_source_steps(&project,picker_drag)*barw/STEPS),track_height(lane)-1};
        DrawRectangleRec(ghost,Fade(ui_theme.signal,.2f)); DrawRectangleLinesEx(ghost,1,ui_theme.signal);
        snprintf(status,sizeof status,"Release to place %s",source_name(picker_drag));
    }
    if(sample_drag[0] && sample_moved && mouse.x>=gx && mouse.x<gx+gridw && mouse.y>=gy && mouse.y<height-PLAYLIST_BOTTOM && windows_hit(&windows,mouse.x+rect.x,mouse.y+rect.y)==1) {
        float start=snap_floor(pointer_bar*STEPS,arrangement.snap)/STEPS;
        float length=audition.frames/(float)RATE*project.bpm/15;
        int lane=fminf(LANES-1,fmaxf(0,track_at(track_scroll+mouse.y-gy)));
        Rectangle ghost={gx+(start-arrangement.view_start)*barw,gy+track_position(lane)-track_scroll,fmaxf(4,length*barw/STEPS),track_height(lane)-1};
        int hit=arrangement_hit(&project,lane,pointer_bar);
        if(hit>=0 && AUDIO_SOURCE(project.clips[lane][hit]-1)) {
            ghost.x=gx+(project.clip_starts[lane][hit]-arrangement.view_start)*barw;
            ghost.width=fmaxf(4,playlist_clip_length(lane,hit)*barw/STEPS);
            snprintf(status,sizeof status,"Release to replace %s's sample",source_name(project.clips[lane][hit]-1));
            DrawRectangleLinesEx(ghost,1,ui_theme.signal);
        } else {
            DrawRectangleRec(ghost,Fade(ui_theme.signal,.2f)); DrawRectangleLinesEx(ghost,1,ui_theme.signal);
        }
    }
    if(arrangement.gesture==BOX_SELECT) {
        float x=gx+(fminf(arrangement.x,arrangement.now_x)-arrangement.view_start)*barw,y=gy+track_position(fminf(arrangement.y,arrangement.now_y))-track_scroll;
        Rectangle box={x,y,fabsf(arrangement.x-arrangement.now_x)*barw,fabsf(track_position(arrangement.y)-track_position(arrangement.now_y))};
        DrawRectangleRec(box,Fade(ui_theme.signal,.15f)); DrawRectangleLinesEx(box,1,ui_theme.signal);
    }
    BeginScissorMode(rect.x*scale,rect.y*scale,rect.w*scale,rect.h*scale);
    if(playing && song) { float x=gx+(visual_step/STEPS-arrangement.view_start)*barw; if(x>=gx && x<=gx+gridw) DrawLineEx((Vector2){x,gy},(Vector2){x,height-PLAYLIST_BOTTOM},1.5f,ui_theme.signal); }
    float rail_top=gy-14,rail_height=track_area+14;
    float vthumb=timeline_thumb(rail_height,track_area,total_height),travel_y=rail_height-vthumb,maximum_y=fmaxf(0,total_height-track_area),vy=rail_top+(maximum_y>0?track_scroll/maximum_y*travel_y:0);
    DrawRectangleRec((Rectangle){width-4-(EDITOR_CORNER+EDITOR_SCROLLBAR)/2,rail_top,EDITOR_SCROLLBAR,rail_height},bg); ui_surface((Rectangle){width-4-(EDITOR_CORNER+EDITOR_SCROLLBAR)/2,vy,EDITOR_SCROLLBAR,fmaxf(12,vthumb)},playlist_vpan || hover(width-4-(EDITOR_CORNER+EDITOR_SCROLLBAR)/2,rail_top,EDITOR_SCROLLBAR,rail_height)?accent:muted);
    if(hover(width-4-(EDITOR_CORNER+EDITOR_SCROLLBAR)/2,rail_top,EDITOR_SCROLLBAR,rail_height)) snprintf(status,sizeof status,"Drag to scroll through the 100 Playlist tracks");
    if(hover(width-4-(EDITOR_CORNER+EDITOR_SCROLLBAR)/2,rail_top,EDITOR_SCROLLBAR,rail_height) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        if(mouse.y<vy || mouse.y>vy+vthumb) track_scroll=fmaxf(0,fminf(maximum_y,(mouse.y-rail_top-vthumb/2)/fmaxf(1,travel_y)*maximum_y));
        playlist_vpan=1; playlist_vpan_y=mouse.y+rect.y; playlist_vpan_start=track_scroll;
    }
    float rail_width=width-4-EDITOR_CORNER-gx,thumbw=timeline_thumb(rail_width,span,arrangement.range),travel=rail_width-thumbw,maximum=arrangement.range-span,thumbx=gx+(maximum>0?fmaxf(0,arrangement.view_start)/maximum*travel:0);
    DrawRectangleRec((Rectangle){gx,gy-14-(EDITOR_CORNER+EDITOR_SCROLLBAR)/2,rail_width,EDITOR_SCROLLBAR},bg); ui_surface((Rectangle){thumbx,gy-14-(EDITOR_CORNER+EDITOR_SCROLLBAR)/2,thumbw,EDITOR_SCROLLBAR},playlist_pan || hover(gx,gy-14-(EDITOR_CORNER+EDITOR_SCROLLBAR)/2,rail_width,EDITOR_SCROLLBAR)?accent:muted);
    if(hover(gx,gy-14-(EDITOR_CORNER+EDITOR_SCROLLBAR)/2,rail_width,EDITOR_SCROLLBAR)) snprintf(status,sizeof status,"Drag to scroll along the Playlist timeline");
    if(hover(gx,gy-14-(EDITOR_CORNER+EDITOR_SCROLLBAR)/2,rail_width,EDITOR_SCROLLBAR) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        if(mouse.x<thumbx || mouse.x>thumbx+thumbw) arrangement.view_start=fmaxf(0,fminf(maximum,(mouse.x-gx-thumbw/2)/fmaxf(1,travel)*maximum));
        playlist_pan=1; playlist_pan_x=mouse.x+rect.x; playlist_pan_start=arrangement.view_start; playlist_pan_range=maximum/fmaxf(1,travel);
    }
    row_zoom_button(1,width,gy-14-EDITOR_CORNER);

}
/* Edit actions target the focused editor; text dialogs retain their own clipboard. */
static void edit_selection(int action) {
    if(midi_take.active){snprintf(status,sizeof status,"Finish the MIDI take before editing notes or clips");return;}
    int id=windows.focused;
    if((id!=1 && id!=2) || !windows.editors[id].visible || browser_focus || captured()) { snprintf(status,sizeof status,"Focus the Piano Roll or Arrangement to edit its selection"); return; }
    if(action==3) {
        if(id==2) for(int i=0;i<NOTES;i++) note_selected[pattern][piano_channel][i]=!!project.notes[pattern][piano_channel][i].velocity;
        else for(int l=0;l<LANES;l++) for(int b=0;b<CLIPS;b++) arrangement.selected[l][b]=!!project.clips[l][b];
        snprintf(status,sizeof status,"Selected all %s",id==2?"notes":"clips"); return;
    }
    int result;
    if(action<2) {
        result=id==2?clipboard_copy_notes(&edit_clipboard,&project,pattern,piano_channel,note_selected[pattern][piano_channel]):clipboard_copy_clips(&edit_clipboard,&project,(const uint8_t (*)[CLIPS])arrangement.selected);
        if(result && action==0) {
            if(!history_checkpoint()) return;
            if(id==2) { for(int i=0;i<NOTES;i++) if(note_selected[pattern][piano_channel][i]) project.notes[pattern][piano_channel][i].velocity=0; memset(note_selected[pattern][piano_channel],0,NOTES); }
            else { for(int l=0;l<LANES;l++) for(int b=0;b<CLIPS;b++) if(arrangement.selected[l][b]) { project.clips[l][b]=0; project.clip_steps[l][b]=0; } memset(arrangement.selected,0,sizeof arrangement.selected); }
        }
        snprintf(status,sizeof status,result?"%s %d %s":"Nothing selected",action==0?"Cut":"Copied",result,id==2?"notes":"clips"); return;
    }
    if(!history_checkpoint()) return;
    result=id==2?clipboard_paste_notes(&edit_clipboard,&project,pattern,piano_channel,piano_start[pattern],note_selected[pattern][piano_channel]):clipboard_paste_clips(&edit_clipboard,&project,selected_track>=0?selected_track:edit_clipboard.lane,playlist_start,arrangement.selected);
    if(result>0) { if(id==2) { piano_channels[pattern][piano_channel]=1; if(arrangement.source_pattern==pattern) arrangement.source_steps=project.pattern_steps[pattern]; } snprintf(status,sizeof status,"Pasted %d %s at the start marker",result,id==2?"notes":"clips"); }
    else snprintf(status,sizeof status,"%s",result==-1?"Clip sources changed; copy the selection again":result==-2?"No room for the entire selection":result==-3?"Paste position is occupied; move the start marker to empty space":result==-4?"Selection does not fit at this destination":"Copy a selection from this editor first");
}
static void piano_shift(float dx,int dy,const char *action) {
    if(windows.focused!=2 || !windows.editors[2].visible || captured() || browser_focus || midi_take.active) return;
    uint8_t *selected=note_selected[pattern][piano_channel]; int count=0,low=127,high=0;
    float end=edit_steps();
    for(int i=0;i<NOTES;i++) if(selected[i] && project.notes[pattern][piano_channel][i].velocity) {
        Note n=project.notes[pattern][piano_channel][i]; int pitch=n.pitch+dy; count++;
        low=fminf(low,pitch); high=fmaxf(high,pitch);
        if(pitch<0 || pitch>127 || n.start+dx<0) { snprintf(status,sizeof status,"Move stopped at the note or timeline limit"); return; }
        end=fmaxf(end,n.start+dx+(n.length?n.length:1));
    }
    if(!count) { snprintf(status,sizeof status,"Select notes to move"); return; }
    if(!history_checkpoint()) return;
    Note before[NOTES]; memcpy(before,project.notes[pattern][piano_channel],sizeof before);
    float previous=project.pattern_steps[pattern];
    project.pattern_steps[pattern]=end>previous?ceilf(end/STEPS)*STEPS:previous;
    if(!notes_move(&project,pattern,piano_channel,before,selected,dx,dy,project.pattern_steps[pattern])) {
        project.pattern_steps[pattern]=previous;
        snprintf(status,sizeof status,"Move stopped at the note or timeline limit"); return;
    }
    if(arrangement.source_pattern==pattern) arrangement.source_steps=project.pattern_steps[pattern];
    if(dy) {
        if(high>piano_top) piano_top=high;
        else if(low<piano_top-piano_min_top()) piano_top=low+piano_min_top();
        piano_top=fmaxf(piano_min_top(),fminf(127,piano_top));
    }
    snprintf(status,sizeof status,"Moved %d notes %s",count,action);
}
static void piano_octave(int direction) {
    if(piano_tool==PENCIL) piano_shift(0,direction*12,direction>0?"up an octave":"down an octave");
}
static int piano_inactive(float step) { float end=edit_steps(); return step>=end && step<ceilf(end/STEPS)*STEPS; }
static void piano_extend(float end) { pattern_extend(end); }
static void piano_gesture_update(float step,float pitch,float q) {
    int channel=piano_channel;
    uint8_t *selected=note_selected[pattern][channel];
    if(piano_gesture==PIANO_BOX) {
        float left=fminf(piano_from.x,step),right=fmaxf(piano_from.x,step),low=fminf(piano_from.y,pitch),high=fmaxf(piano_from.y,pitch);
        for(int i=0;i<NOTES;i++) {
            Note n=project.notes[pattern][channel][i];
            selected[i]=(piano_additive && note_selection_before[i]) || (n.velocity && n.start<edit_steps() && n.start<=right && n.start+(n.length?n.length:1)>left && n.pitch>=low && n.pitch-1<=high);
        }
    } else {
        float dx=step-piano_now.x,dy=pitch-piano_now.y,spacing=piano_gesture==PIANO_BRUSH?fmaxf(piano_note_length,q):q>0?q:1;
        int count=fminf(NOTES*2,ceilf(fmaxf(fabsf(dx)/spacing,fabsf(dy))*2)); if(count<1) count=1;
        for(int j=0;j<=count;j++) {
            float t=j/(float)count,at=piano_now.x+dx*t; int key=ceilf(piano_now.y+dy*t);
            if(at<0 || piano_inactive(at) || key<0 || key>127) continue;
            if(piano_gesture==PIANO_ERASE) {
                /* Hit the gesture's original stack so holding the button or
                   repeated stroke samples cannot peel off underlying notes. */
                int i=note_hit(note_before,at,key);
                if(i>=0) { project.notes[pattern][channel][i].velocity=0; selected[i]=0; }
            } else {
                float start=piano_from.x+floorf((at-piano_from.x)/spacing)*spacing;
                piano_extend(start+piano_note_length);
                int exists=note_at(&project,pattern,channel,start,key)!=NULL;
                Note *n=exists?NULL:note_add(&project,pattern,channel,start,key,piano_note_length);
                if(n) { audio_note(channel,*n); piano_channels[pattern][channel]=1; }
            }
        }
    }
    piano_now=(Vector2){step,pitch};
}
static void piano_note_drag_update(float step,float row,float q) {
    int channel=piano_channel;
    if(moving_note) {
        uint8_t *selected=note_selected[pattern][channel];
        float dx=snap_round(step-note_grab.x,q),low=0,high=0; int dy=roundf(note_grab.y-row),bottom=0,top=127,first=1;
        for(int i=0;i<NOTES;i++) if(selected[i]) {
            Note n=note_before[i]; float end=n.start+(n.length?n.length:1);
            if(first || n.start<low) low=n.start;
            if(first || end>high) high=end;
            if(first || n.pitch<bottom) bottom=n.pitch;
            if(first || n.pitch>top) top=n.pitch;
            first=0;
        }
        dx=fmaxf(-low,dx); piano_extend(high+dx); dy=fmaxf(-bottom,fminf(127-top,dy));
        int old_pitch=note_drag->pitch;
        notes_move(&project,pattern,channel,note_before,selected,dx,dy,edit_steps());
        if(note_drag->pitch!=old_pitch) audio_note(channel,*note_drag);
    } else {
        float delta=snap_round(step-note_grab.x,q);
        float end=notes_resize(&project,pattern,channel,note_before,note_selected[pattern][channel],delta,q>0?q:.01f);
        piano_extend(end); piano_note_length=note_drag->length;
    }
}
static int piano_black_key(int pitch) {
    int pc=pitch%12; return pc==1 || pc==3 || pc==6 || pc==8 || pc==10;
}
static Rectangle piano_key_rect(int pitch,float rh) {
    float center=PIANO_GRID_TOP+(piano_top-pitch+.5f)*rh;
    if(piano_black_key(pitch)) return (Rectangle){4,center-rh*.5f,34,rh};
    float top=center-rh*(piano_black_key(pitch+1)?1:.5f);
    float bottom=center+rh*(piano_black_key(pitch-1)?1:.5f);
    top=fmaxf(PIANO_GRID_TOP,top); bottom=fminf(PIANO_GRID_TOP+rh*piano_visible_rows(),bottom);
    return (Rectangle){4,top,54,fmaxf(0,bottom-top)};
}
static int piano_key_at(float x,float y,float rh) {
    if(x<4 || x>=58 || y<PIANO_GRID_TOP || y>=PIANO_GRID_TOP+rh*piano_visible_rows()) return -1;
    float row=(y-PIANO_GRID_TOP)/rh; int pitch=piano_top-(int)floorf(row);
    if(piano_black_key(pitch) && x>=38) pitch+=row-floorf(row)<.5f?1:-1;
    return pitch>=0 && pitch<=127?pitch:-1;
}
static void piano_key_update(float x,float y,int cancel) {
    Rect r=windows.editors[2].rect; float rh=(r.h-PIANO_GRID_PADDING)/piano_visible_rows();
    int pitch=piano_key_at(x,y,rh);
    if(cancel || pitch<0 || pitch>127) pitch=-1;
    if(pitch!=piano_key) {
        if(piano_key>=0) audio_key(96+piano_key%25,piano_key_channel,piano_key,0);
        piano_key=pitch;
        if(pitch>=0) audio_key(96+pitch%25,piano_key_channel,pitch,1);
    }
    if(cancel) piano_key_drag=0;
}
static void piano(float width,float height) {
    int channel=piano_channel;
    int gx=60,gy=PIANO_GRID_TOP; float gridw=width-84,rh=(height-PIANO_GRID_PADDING)/piano_visible_rows();
    float extent=piano_span[pattern];
    Rect r=windows.editors[2].rect; float scale=ui_scale();
    const char *tips[]={"Pencil: draw one note; drag note bodies to move; edges to resize","Brush: drag to paint notes on the visible grid","Select: drag a rectangle; drag selected notes together; Shift adds; Delete deletes"};
    for(int i=0;i<3;i++) if(tool_button(i,piano_tool,8+i*24,tips[i])) piano_tool=i;
    label(fit_text(TextFormat("%s / %s",project.channel_names[channel],project.pattern_names[pattern]),width-158,12),88,30,12,muted);
    int grid_over=hover(gx,gy,gridw,rh*piano_visible_rows());
    extent=piano_span[pattern];
    double playback=pattern_playback_position();
    if(playback>=0) follow_view(&piano_pan[pattern],extent,playback);
    float cw=gridw/extent,view=piano_pan[pattern],q=grid_interval(cw),end=edit_steps();
    piano_range[pattern]=fmaxf(piano_range[pattern],fmaxf(view+extent*2,end+extent));
    float partial=ceilf(end/STEPS)*STEPS;
    if(hover(4,gy,gx-6,rh*piano_visible_rows())) {
        snprintf(status,sizeof status,"Piano keys: click to play; hold and drag up/down to audition pitches");
        if(IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            piano_key_drag=1; piano_key_channel=channel; piano_key_update(mouse.x,mouse.y,0); input_enabled=0;
        }
    }
    DrawRectangleRec((Rectangle){4,gy,gx-6,rh*piano_visible_rows()},ui_theme.piano_white);
    BeginScissorMode((r.x+4)*scale,(r.y+gy)*scale,54*scale,(height-PIANO_GRID_PADDING)*scale);
    int hovered_key=input_enabled?piano_key_at(mouse.x,mouse.y,rh):-1;
    /* Draw full white keys first; black keys cover their overlapping left ends. */
    for(int black=0;black<2;black++) for(int row=-1;row<=(int)ceilf(piano_visible_rows());row++) {
        int pitch=piano_top-row;
        if(pitch<0 || pitch>127 || piano_black_key(pitch)!=black) continue;
        Rectangle key=piano_key_rect(pitch,rh);
        float bottom=fminf(gy+rh*piano_visible_rows(),key.y+key.height);
        key.y=fmaxf(gy,key.y); key.height=fmaxf(0,bottom-key.y);
        if(key.height<=0) continue;
        int active=keyboard_notes[channel][pitch] || playing_keys[channel][pitch] || (piano_key==pitch && piano_key_channel==channel);
        Color base=black?ui_theme.piano_black:pitch%12==0?ui_theme.piano_c:ui_theme.piano_white;
        DrawRectangleRec(key,active?accent:base);
        if(!active && hovered_key==pitch) DrawRectangleRec(key,Fade(accent,.22f));
        if(!black) DrawLineEx((Vector2){key.x,key.y+key.height},(Vector2){key.x+key.width,key.y+key.height},1,Fade(ui_theme.piano_black,.15f));
        if(pitch%12==0) {
            const char *name=TextFormat("C%d",pitch/12-1);
            label(name,gx-5-text_width(name,10),fmaxf(gy,fminf(gy+rh*piano_visible_rows()-10,gy+(row+.5f)*rh-5)),10,active?ui_theme.selected_text:ui_theme.piano_black);
        }
    }
    BeginScissorMode((r.x+gx)*scale,(r.y+gy)*scale,gridw*scale,rh*piano_visible_rows()*scale);
    for(int row=0;row<(int)ceilf(piano_visible_rows());row++) {
        int pc=(piano_top-row)%12,black=pc==1 || pc==3 || pc==6 || pc==8 || pc==10;
        float y=gy+row*rh;
        DrawRectangleRec((Rectangle){gx,y,gridw,rh},ui_theme.piano_row[black]);
        int pitch=piano_top-row;
        if(pitch>=0 && pitch<=127 && (keyboard_notes[channel][pitch] || playing_keys[channel][pitch] ||
           (piano_key==pitch && piano_key_channel==channel)))
            DrawRectangleRec((Rectangle){gx,y,gridw,rh},Fade(accent,.15f));
        DrawLineEx((Vector2){gx,y},(Vector2){gx+gridw,y},1,Fade(ui_theme.grid_minor,.45f));
        if(partial>end) { float left=fmaxf(gx,gx+(end-view)*cw),right=fminf(gx+gridw,gx+(partial-view)*cw); if(right>left) DrawRectangleRec((Rectangle){left,y,right-left,rh-1},ui_theme.disabled); }
        if(pc==0) DrawLineEx((Vector2){gx,y+rh},(Vector2){gx+gridw,y+rh},1,ui_theme.grid_major);
    }
    timeline_grid(view,extent,gx,gy,gridw,rh*piano_visible_rows());
    Note *hit=NULL;
    for(int i=0;i<NOTES;i++) {
        Note *n=&project.notes[pattern][channel][i];
        if(!n->velocity || n->pitch<piano_top-(int)ceilf(piano_visible_rows())+1 || n->pitch>piano_top || n->start>=end || n->start+(n->length?n->length:1)<=view) continue;
        float note_end=fminf(n->start+(n->length?n->length:1),end);
        float x=gx+(n->start-view)*cw,y=gy+(piano_top-n->pitch)*rh,w=(note_end-n->start)*cw;
        float left=fmaxf(gx,x),right=fminf(gx+gridw,x+w);
        if(right<=left) continue;
        float inset=fminf(1,rh*.15f),note_height=rh-2*inset;
        if(note_selected[pattern][channel][i]) DrawRectangleLinesEx((Rectangle){left,y,right-left,rh},fminf(1,rh*.25f),ink);
        DrawRectangleRec((Rectangle){left+1,y+inset,fmaxf(1,right-left-2),note_height},n==note_drag || note_selected[pattern][channel][i]?ui_theme.note_drag:ui_theme.note);
        DrawRectangleLinesEx((Rectangle){left+1,y+inset,fmaxf(1,right-left-2),note_height},fminf(1,rh*.25f),Fade(ui_theme.signal,.45f));
        int edge=grid_over && !shortcut_down() && hover(x+w-7,y,7,rh);
        if(x+w>=gx && x+w<=gx+gridw) DrawRectangleRec((Rectangle){x+w-5,y+inset,3,note_height},edge || (n==note_drag && !moving_note)?ink:Fade(bg,.35f));
        if(edge || (n==note_drag && !moving_note)) SetMouseCursor(MOUSE_CURSOR_RESIZE_EW);
        if(grid_over && hover(x,y,w,rh)) hit=n;
    }
    if(grid_over) {
        float start=snap_floor(view+(mouse.x-gx)/cw,q);
        snprintf(status,sizeof status,"Piano Roll: %s | Shift: add selection | Right-drag: erase | Delete: delete selected notes | Wheel: zoom",piano_tool==PENCIL?"draw or move notes":piano_tool==BRUSH?"paint notes":"select or move notes");
        if(piano_inactive(start)) snprintf(status,sizeof status,"Inactive steps: extend the Playlist clip to enable them");
        else if(IsMouseButtonPressed(MOUSE_BUTTON_LEFT) || IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
            uint8_t *selected=note_selected[pattern][channel];
            if(hit && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) piano_note_length=hit->length?hit->length:2;
            Vector2 point={view+(mouse.x-gx)/cw,piano_top-(mouse.y-gy)/rh};
            int shift=IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT),group=0;
            for(int i=0;i<NOTES;i++) group+=selected[i]!=0;
            piano_from=piano_now=point; piano_additive=shift; memcpy(note_selection_before,selected,NOTES);
            if(IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
                memcpy(note_before,project.notes[pattern][channel],sizeof note_before);
                piano_gesture=PIANO_ERASE;
            }
            else if(shortcut_down()) {
                if(!shift) memset(selected,0,NOTES);
                piano_gesture=PIANO_BOX;
            } else if(shift && hit) selected[hit-project.notes[pattern][channel]]^=1;
            else if((shift || piano_tool==SELECT) && !hit) {
                if(!shift) memset(selected,0,NOTES);
                piano_gesture=PIANO_BOX;
            } else if(piano_tool==BRUSH && !(hit && selected[hit-project.notes[pattern][channel]] && group>1)) piano_gesture=PIANO_BRUSH;
            else {
                moving_note=0;
                if(hit) {
                    float edge=gx+(fminf(hit->start+(hit->length?hit->length:1),end)-view)*cw;
                    note_drag=hit; moving_note=mouse.x<edge-7;
                    if(!selected[hit-project.notes[pattern][channel]]) { memset(selected,0,NOTES); selected[hit-project.notes[pattern][channel]]=1; }
                    memcpy(note_before,project.notes[pattern][channel],sizeof note_before);
                    note_grab=(Vector2){point.x,(mouse.y-gy)/rh};
                } else {
                    memset(selected,0,NOTES);
                    piano_extend(start+piano_note_length);
                    note_drag=note_add(&project,pattern,channel,start,ceilf(point.y),piano_note_length);
                    if(note_drag) {
                        selected[note_drag-project.notes[pattern][channel]]=1; moving_note=1;
                        memcpy(note_before,project.notes[pattern][channel],sizeof note_before);
                        note_grab=(Vector2){point.x,(mouse.y-gy)/rh};
                    }
                }
                if(!note_drag) snprintf(status,sizeof status,"No room here: remove a note (limit %d).",NOTES);
                else { audio_note(channel,*note_drag); piano_channels[pattern][channel]=1; }
            }
            if(piano_gesture==PIANO_BRUSH) piano_from.x=start;
            if(piano_gesture) piano_gesture_update(point.x,point.y,q);
            input_enabled=0;
        }
    }
    if(piano_gesture==PIANO_BOX) {
        float left=fminf(piano_from.x,piano_now.x),high=fmaxf(piano_from.y,piano_now.y);
        Rectangle box={gx+(left-view)*cw,gy+(piano_top-high)*rh,fabsf(piano_from.x-piano_now.x)*cw,fabsf(piano_from.y-piano_now.y)*rh};
        DrawRectangleRec(box,Fade(ui_theme.signal,.15f)); DrawRectangleLinesEx(box,1,ui_theme.signal);
    }

    BeginScissorMode(r.x*scale,r.y*scale,r.w*scale,r.h*scale);
    int vy=height-84; label("Velocity",6,vy+4,10,muted);
    BeginScissorMode((r.x+gx)*scale,(r.y+vy)*scale,gridw*scale,62*scale);
    DrawRectangle(gx,vy,gridw,62,ui_theme.piano_row[0]);
    for(int line=1;line<5;line++) DrawLine(gx,vy+line*62/5,gx+gridw,vy+line*62/5,Fade(ui_theme.grid_minor,.6f));
    timeline_grid(view,extent,gx,vy,gridw,62);
    if(partial>end) { float left=fmaxf(gx,gx+(end-view)*cw),right=fminf(gx+gridw,gx+(partial-view)*cw); if(right>left) DrawRectangle(left,vy,right-left,62,ui_theme.disabled); }
    for(int i=0;i<NOTES;i++) {
        Note n=project.notes[pattern][channel][i]; if(!n.velocity || n.start>=end) continue;
        float x=gx+(n.start-view)*cw;
        if(x<gx-10 || x>gx+gridw) continue;
        float top=vy+62-n.velocity*.48f;
        Color color=note_selected[pattern][channel][i]?ui_theme.note_drag:ui_theme.note;
        DrawRectangleRec((Rectangle){x,top,2,n.velocity*.48f},color);
        circle(x+1,top,2.5f,color); circle(x+1,top,1.25f,ui_theme.piano_row[0]);
    }
    if(hover(gx,vy,gridw,62)) {
        snprintf(status,sizeof status,"Velocity: left-drag paints; right-drag draws a straight ramp");
        if(IsMouseButtonPressed(MOUSE_BUTTON_LEFT) || IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
            velocity_drag=IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)?1:0;
            velocity_pattern=pattern; velocity_channel=channel;
            velocity_from=velocity_now=(Vector2){view+(mouse.x-gx)/cw,fmaxf(1,fminf(127,(vy+62-mouse.y)/.48f))};
            memcpy(velocity_before,project.notes[pattern][channel],sizeof velocity_before);
            notes_velocity(project.notes[pattern][channel],velocity_drag?velocity_before:NULL,
                velocity_from.x,velocity_from.x,velocity_from.y,velocity_from.y,3/cw);
            input_enabled=0;
        }
    }
    if(velocity_drag==1 && velocity_pattern==pattern && velocity_channel==channel)
        DrawLineEx((Vector2){gx+(velocity_from.x-view)*cw,vy+62-velocity_from.y*.48f},
            (Vector2){gx+(velocity_now.x-view)*cw,vy+62-velocity_now.y*.48f},1,accent);
    BeginScissorMode(r.x*scale,r.y*scale,r.w*scale,r.h*scale);
    float rail_top=gy-14,varea=rh*piano_visible_rows()+14,vthumb=varea*piano_visible_rows()/128,vypos=rail_top+(127-piano_top)/128.f*varea;
    DrawRectangleRec((Rectangle){width-4-(EDITOR_CORNER+EDITOR_SCROLLBAR)/2,rail_top,EDITOR_SCROLLBAR,varea},bg);
    ui_surface((Rectangle){width-4-(EDITOR_CORNER+EDITOR_SCROLLBAR)/2,vypos,EDITOR_SCROLLBAR,vthumb},piano_vdrag || hover(width-4-(EDITOR_CORNER+EDITOR_SCROLLBAR)/2,rail_top,EDITOR_SCROLLBAR,varea)?accent:muted);
    if(hover(width-4-(EDITOR_CORNER+EDITOR_SCROLLBAR)/2,rail_top,EDITOR_SCROLLBAR,varea)) {
        snprintf(status,sizeof status,"Drag to scroll higher or lower pitches");
        if(IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            if(mouse.y<vypos || mouse.y>vypos+vthumb) piano_top=127-(int)fmaxf(0,fminf(127-piano_min_top(),(mouse.y-rail_top-vthumb/2)/varea*128));
            piano_vscroll_y=mouse.y+r.y; piano_vscroll_start=127-piano_top; piano_vdrag=1; input_enabled=0;
        }
    }
    float rail_width=width-4-EDITOR_CORNER-gx,thumb=timeline_thumb(rail_width,extent,piano_range[pattern]),travel=rail_width-thumb,maximum=piano_range[pattern]-extent,x=gx+(maximum>0?fmaxf(0,piano_pan[pattern])/maximum*travel:0);
    DrawRectangleRec((Rectangle){gx,gy-14-(EDITOR_CORNER+EDITOR_SCROLLBAR)/2,rail_width,EDITOR_SCROLLBAR},bg); ui_surface((Rectangle){x,gy-14-(EDITOR_CORNER+EDITOR_SCROLLBAR)/2,thumb,EDITOR_SCROLLBAR},piano_scroll_drag || hover(gx,gy-14-(EDITOR_CORNER+EDITOR_SCROLLBAR)/2,rail_width,EDITOR_SCROLLBAR)?accent:muted);
    if(hover(gx,gy-14-(EDITOR_CORNER+EDITOR_SCROLLBAR)/2,rail_width,EDITOR_SCROLLBAR)) {
        snprintf(status,sizeof status,"Drag to pan the zoomed Piano Roll; wheel over notes to zoom; the timeline grows as you navigate");
        if(IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            if(mouse.x<x || mouse.x>x+thumb) piano_pan[pattern]=fmaxf(0,fminf(maximum,(mouse.x-gx-thumb/2)/fmaxf(1,travel)*maximum));
            piano_scroll_x=mouse.x+r.x; piano_scroll_start=piano_pan[pattern]; piano_scroll_range=maximum/fmaxf(1,travel); piano_scroll_drag=1; input_enabled=0;
        }
    }
    row_zoom_button(2,width,gy-14-EDITOR_CORNER);
    timeline_ruler(2,view,extent,gx,gy-14,gridw,q);
    if(playback>=0) {
        float x=gx+(playback-view)*cw;
        if(x>=gx && x<=gx+gridw) DrawLineEx((Vector2){x,gy},(Vector2){x,gy+rh*piano_visible_rows()},1.5f,ui_theme.signal);
    }

}
static int mixer_input_at(Vector2 point) {
    Rect r=windows.editors[3].rect; if(!windows.editors[3].visible) return -1;
    int capacity=fmaxf(1,(r.w-MIXER_PANEL-MIXER_LEFT-8)/51-1),visible=fminf(project.insert_count,capacity);
    for(int col=0;col<=visible;col++) if(CheckCollisionPointRec(point,(Rectangle){r.x+MIXER_LEFT+col*51+7,r.y+r.h-46,12,12})) return col?col+mixer_scroll:0;
    return -1;
}
static float meter_level[INSERTS+1][2],meter_hold[INSERTS+1][2];
static double meter_until[INSERTS+1][2],meter_time;
static uint8_t record_armed[INSERTS+1];
static void recording_finish(void) {
    if(midi_take.active) {
        int failed=midi_take.failed;audio_record_mode(0);midi_take_finish(&midi_take,&project,midi_input_time());midi_panic();
        pattern=fmaxf(0,fminf(pattern,project.pattern_count-1));
        snprintf(status,sizeof status,failed?"MIDI recording stopped at the note or track limit; captured take kept":"MIDI take saved in the Playlist");
    }
    if(!recording_ui.active) return;
    int ok=recording_finish_writers(&recording_ui,&project);
    for(int i=0;i<recording_ui.count;i++) {
        RecordingTake *take=&recording_ui.takes[i]; int c=take->channel;
        unsigned committed=recording_writer_frames(take->writer);
        recording_writer_free(take->writer,0); take->writer=NULL;
        if(!take->sample.frames && committed) {
            /* A mapping failure must never delete successfully written audio. */
            project.audio_seconds[c]=committed/(float)RATE;
            if(take->clip>=0) project.clip_steps[take->lane][take->clip]=0;
            snprintf(project.paths[c],sizeof project.paths[c],"%s",take->path); ok=0;
        } else if(!take->sample.frames) {
            if(take->clip>=0) project.clips[take->lane][take->clip]=0;
            project.audio_seconds[c]=0;
            snprintf(project.paths[c],sizeof project.paths[c],"%s",SAMPLE_EMPTY); remove(take->path);
            sample_free(take->sample);
        } else {
            if(take->clip>=0) project.clip_steps[take->lane][take->clip]=0;
            Sample processed;
            if(!sample_process(take->sample,project.sampler[c],&processed)) {
                processed=take->sample; take->sample=(Sample){0}; ok=0;
            }
            originals[c]=take->sample; samples[c]=processed; audio_sample(c,processed);
            sampler_applied[c]=project.sampler[c]; sample_generation[c]++; sampler_generation[c]=sample_generation[c];
            project.audio_seconds[c]=processed.frames/(float)RATE;
            snprintf(project.paths[c],sizeof project.paths[c],"%s",take->path);
        }
        free(take->wave.tree); take->wave=(Waveform){0}; take->sample=(Sample){0};
    }
    snprintf(status,sizeof status,ok?"Recording saved: %d audio clips in recordings/":"Recording stopped after a device, buffer, memory or disk error; captured audio kept in recordings/",recording_ui.count);
    recording_ui.count=0;
}
static void recording_poll(void) {
    if(recording_ui.active && (!recording_refresh(&recording_ui,&project) || audio_record_failed() || !playing)) recording_finish();
}
static void recording_start_audio(void) {
    if(recording_ui.active) { recording_finish(); return; }
    int buses[CHANNELS],count=0;
    for(int id=0;id<=project.insert_count;id++) if(record_armed[id]) {
        if(count>=CHANNELS-project.channel_count) { snprintf(status,sizeof status,"Not enough free Channel Rack slots for all armed tracks."); return; }
        buses[count++]=id;
    }
    if(!count) { snprintf(status,sizeof status,"Arm a mixer track with its red recording button first."); return; }
    if(!sampler_flush()) { snprintf(status,sizeof status,"Wait for sample processing before recording."); return; }
    if(MakeDirectory("recordings")!=0) { snprintf(status,sizeof status,"Cannot create recordings/ in the working directory."); return; }
    char directory[PATH_MAX]; if(!realpath("recordings",directory)) { snprintf(status,sizeof status,"Cannot open recordings/ in the working directory."); return; }
    float start=playing && song?audio_visual_position()/(RATE*60.0/project.bpm/4):playlist_start*STEPS;
    Project next=project;
    if(!recording_prepare(&recording_ui,&next,buses,count,start,directory)) goto failed;
    char error[256]={0};
    /* Begin Song playback at the recording origin, without looping the existing arrangement. */
    for(int i=0;i<count;i++) audio_sample(recording_ui.takes[i].channel,(Sample){0});
    if(!audio_record_start(&next,buses,count,start,output_volume,error)) {
        audio_channels(&project,samples);
        audio_update(&project,playing,song,pattern,1,output_volume,playback_start(),playback_loop()[0],playback_loop()[1]);
        snprintf(status,sizeof status,"%s",error); goto cleanup;
    }
    if(!recording_start_workers(&recording_ui)) {
        audio_record_end(); audio_channels(&project,samples);
        audio_update(&project,playing,song,pattern,1,output_volume,playback_start(),playback_loop()[0],playback_loop()[1]);
        snprintf(status,sizeof status,"Could not start the recording writer."); goto cleanup;
    }
    for(int i=0;i<count;i++) {
        int c=recording_ui.takes[i].channel; sample_free(samples[c]); sample_free(originals[c]);
        samples[c]=originals[c]=(Sample){0}; sampler_applied[c]=next.sampler[c];
    }
    project=next; recording_ui.active=1; recording_ui.bpm=project.bpm; recording_ui.start=start;
    playing=song=1; reset=0; playlist_start=start/STEPS; memset(song_loop,0,sizeof song_loop);
    memset(arrangement.selected,0,sizeof arrangement.selected);
    for(int i=0;i<count;i++) { RecordingTake *take=&recording_ui.takes[i]; arrangement.selected[take->lane][take->clip]=1; }
    picker_tab=1; picker_scroll=0; channel=instrument_channel=recording_ui.takes[0].channel;
    arrangement.source_pattern=-1; arrangement.source_offset=0;
    windows_focus(&windows,1); browser_focus=0;
    snprintf(status,sizeof status,"Recording %d armed mixer tracks; Stop or Record finishes the takes.",count); return;
failed:
    snprintf(status,sizeof status,"Cannot prepare recording: free Playlist tracks, memory or writable disk space required.");
cleanup:
    recording_cancel(&recording_ui);
}
static int midi_target(void) { return midi_take.active?midi_take.channel:recording_ui.active?midi_record_target:windows.focused==2?piano_channel:windows.focused==4?instrument_channel:channel; }
static void midi_panic(void) {
    for(int i=0;i<32;i++)if(midi_keys[i].used) {audio_key_velocity(64+i,midi_keys[i].target,midi_keys[i].pitch,0);if(keyboard_notes[midi_keys[i].target][midi_keys[i].pitch])keyboard_notes[midi_keys[i].target][midi_keys[i].pitch]--;if(midi_take.active && (record_mask&RECORD_NOTES))midi_take_note(&midi_take,&project,midi_keys[i].mchannel,midi_keys[i].pitch,0,midi_input_time());midi_keys[i].used=0;}
    memset(midi_sustain,0,sizeof midi_sustain);
}
static void midi_save_settings(void) {
    AtomicFile out;if(!midi_settings_path[0] || !atomic_file_open(&out,midi_settings_path))return;
    fprintf(out.file,"%u\n%s\n%s\n",record_mask,midi_selected,midi_selected_name);atomic_file_commit(&out);
}
static void midi_refresh(void) {
    midi_device_count=midi_input_devices(midi_devices,64);int found=-1;
    for(int i=0;i<midi_device_count;i++)if(!strcmp(midi_devices[i].id,midi_selected)) {
#ifndef __APPLE__
        if(midi_selected_name[0] && strcmp(midi_devices[i].name,midi_selected_name))continue;
#endif
        found=i;break;
    }
#ifndef __APPLE__
    if(found<0 && midi_selected_name[0]) {
        int matches=0;for(int i=0;i<midi_device_count;i++)if(!strcmp(midi_devices[i].name,midi_selected_name)){found=i;matches++;}
        if(matches!=1)found=-1;
    }
#endif
    if(midi_connected && (found<0 || strcmp(midi_selected,midi_devices[found].id))) {
        midi_panic();midi_input_close();midi_connected=0;char error[256];midi_input_open("",error);
        snprintf(status,sizeof status,"MIDI input disconnected; held notes released");
    }
    if(found>=0 && !midi_connected) {
        snprintf(midi_selected,sizeof midi_selected,"%s",midi_devices[found].id);
        char error[256];midi_connected=midi_input_open(midi_selected,error);
        if(!midi_connected)snprintf(status,sizeof status,"%s",error);
    }
}
static void midi_select(int index) {
    if(recording_active())return;midi_panic();midi_input_close();midi_connected=0;
    snprintf(midi_selected,sizeof midi_selected,"%s",index<0?"":midi_devices[index].id);
    snprintf(midi_selected_name,sizeof midi_selected_name,"%s",index<0?"":midi_devices[index].name);
    if(index>=0){char error[256];midi_connected=midi_input_open(midi_selected,error);if(midi_connected)record_mask|=RECORD_NOTES;else snprintf(status,sizeof status,"%s",error);}
    if(index<0){char error[256];midi_input_open("",error);}
    midi_save_settings();
}
static void midi_initialize(void) {
    const char *slash=strrchr(browser.config,'/');if(slash)snprintf(midi_settings_path,sizeof midi_settings_path,"%.*s/midi.txt",(int)(slash-browser.config),browser.config);
    FILE *f=midi_settings_path[0]?fopen(midi_settings_path,"r"):NULL;
    if(f){unsigned mask;if(fscanf(f,"%u\n",&mask)==1){record_mask=mask&7;if(fgets(midi_selected,sizeof midi_selected,f))midi_selected[strcspn(midi_selected,"\r\n")]=0;if(fgets(midi_selected_name,sizeof midi_selected_name,f))midi_selected_name[strcspn(midi_selected_name,"\r\n")]=0;}fclose(f);}
    midi_input_wake(glfwPostEmptyEvent);char error[256];midi_input_open("",error);midi_refresh();
}
static void midi_release_key(int slot,double time) {
    if(!midi_keys[slot].used)return;
    int target=midi_keys[slot].target,pitch=midi_keys[slot].pitch;
    audio_key_velocity(64+slot,target,pitch,0);if(keyboard_notes[target][pitch])keyboard_notes[target][pitch]--;
    if(midi_take.active && (record_mask&RECORD_NOTES))midi_take_note(&midi_take,&project,midi_keys[slot].mchannel,pitch,0,time);
    midi_keys[slot].used=0;
}
static void midi_control(ParameterTarget target,float normalized,double time) {
    float value,lo,hi;if(!parameter_info(&project,target,&value,&lo,&hi))return;
    const ParameterDescriptor *d=parameter_descriptor(target.parameter);
    if(target.parameter==PARAM_MASTER_VOLUME || target.parameter==PARAM_INSERT_VOLUME)normalized=fader_gain(normalized)/hi;
    if(d && d->kind==PARAMETER_LOGARITHMIC && lo>0)normalized=(lo*powf(hi/lo,normalized)-lo)/(hi-lo);
    if(target.parameter>=PARAM_DX7_FIRST && target.parameter<PARAM_DX7_FIRST+126 &&
       (target.parameter-PARAM_DX7_FIRST)%21==18 && project.fm[target.owner].dx7.value[target.parameter-PARAM_DX7_FIRST-1])normalized=roundf(normalized*3)/31;
    if(midi_take.active && (record_mask&RECORD_AUTOMATION))midi_take_control(&midi_take,&project,target,normalized,time);
    parameter_write(&project,target,normalized);
}
static void midi_poll(void) {
    double now=midi_input_time();
    if(midi_input_changed() || (midi_selected[0] && now-midi_refresh_time>2)){midi_refresh_time=now;midi_refresh();}
    if(midi_learning && IsKeyPressed(KEY_ESCAPE)){midi_learning=0;snprintf(status,sizeof status,"MIDI Learn cancelled");}
    int input_error=midi_input_failed();
    if(input_error){for(int i=0;i<32;i++)midi_release_key(i,now);midi_input_flush();
        if(input_error==2){midi_input_close();midi_connected=0;char error[256];midi_input_open("",error);}
        snprintf(status,sizeof status,"MIDI input lost events; held notes released");}
    MidiEvent e;
    while(midi_input_poll(&e)) {
        int type=e.status&0xf0,mchannel=e.status&15;
        if(type==0x90 || type==0x80) {
            int slot=-1;for(int i=0;i<32;i++)if(midi_keys[i].used && midi_keys[i].mchannel==mchannel && midi_keys[i].pitch==e.data1){slot=i;break;}
            if(type==0x80 || !e.data2) {if(slot>=0){midi_keys[slot].pressed=0;if(!midi_sustain[mchannel])midi_release_key(slot,e.time);}continue;}
            if(slot>=0)midi_release_key(slot,e.time);
            if(slot<0)for(int i=0;i<32;i++)if(!midi_keys[i].used){slot=i;break;}
            if(slot<0){slot=0;midi_release_key(slot,e.time);}
            int target=midi_target();if(target<0 || target>=project.channel_count)continue;
            midi_keys[slot]=(MidiHeld){1,1,mchannel,e.data1,target,e.data2};
            audio_key_velocity(64+slot,target,e.data1,e.data2);keyboard_notes[target][e.data1]++;
            if(midi_take.active && (record_mask&RECORD_NOTES))midi_take_note(&midi_take,&project,mchannel,e.data1,e.data2,e.time);
        } else if(type==0xb0) {
            if(e.data1>=120){for(int i=0;i<32;i++)if(midi_keys[i].used && midi_keys[i].mchannel==mchannel)midi_release_key(i,e.time);midi_sustain[mchannel]=0;continue;}
            if(e.data1==64){midi_sustain[mchannel]=e.data2>=64;if(!midi_sustain[mchannel])for(int i=0;i<32;i++)if(midi_keys[i].used && midi_keys[i].mchannel==mchannel && !midi_keys[i].pressed)midi_release_key(i,e.time);}
            if(midi_learning){int ok=midi_bind(&project,mchannel,e.data1,midi_learn_target);midi_learning=0;snprintf(status,sizeof status,ok?"MIDI controller %d linked; mapping saved with project":"MIDI link could not be added",e.data1);}
            for(int i=0;i<project.midi_binding_count;i++){MidiBinding b=project.midi_bindings[i];if(b.channel==(unsigned)mchannel && b.controller==e.data1)midi_control(b.target,e.data2/127.f,e.time);}
        } else if(type==0xe0) {
            int target=midi_target();int value=e.data1|(e.data2<<7);
            float pitch=value>=8192?(value-8192)/8191.f:(value-8192)/8192.f;
            midi_control((ParameterTarget){PARAM_CHANNEL_PITCH,target,0},(pitch+1)*.5f,e.time);
        }
    }
    if(midi_take.active) {
        project.bpm=midi_take.bpm;midi_take_update(&midi_take,&project,now);
        if((record_mask&RECORD_AUTOMATION) && control_drag){ParameterTarget target;float value,lo,hi;
            if(parameter_from_pointer(&project,control_drag,&target) && parameter_info(&project,target,&value,&lo,&hi))midi_take_control(&midi_take,&project,target,(value-lo)/(hi-lo),now);}
        if(midi_take.failed || !playing)recording_finish();
    }
}
static void recording_start(void) {
    if(recording_active()){recording_finish();return;}
    midi_poll();
    if(!record_mask){snprintf(status,sizeof status,"Enable Audio, Notes or Automation recording");return;}
    if((record_mask&RECORD_NOTES) && !midi_connected){snprintf(status,sizeof status,"Choose a MIDI input in View > MIDI / Recording");open_popup(13);return;}
    int target=midi_target();midi_record_target=target;float origin=playing && song?audio_visual_position()/(RATE*15/project.bpm):playlist_start*STEPS;
    if(!history_checkpoint())return;
    if(record_mask&(RECORD_NOTES|RECORD_AUTOMATION)) {
        if(!midi_take_begin(&midi_take,&project,target,!!(record_mask&RECORD_NOTES),origin,midi_input_time())){snprintf(status,sizeof status,"MIDI recording needs a free pattern and Playlist track");return;}
    }
    int armed=0;for(int i=0;i<=project.insert_count;i++)armed+=record_armed[i]!=0;
    if((record_mask&RECORD_AUDIO) && armed){recording_start_audio();if(!recording_ui.active){if(midi_take.active)midi_take_finish(&midi_take,&project,midi_input_time());return;}}
    if(!recording_ui.active && !midi_take.active){snprintf(status,sizeof status,"Arm a mixer track to record audio");return;}
    if(midi_take.active){
        audio_record_mode(1);double now=midi_input_time();
        if(recording_ui.active)midi_take.time=now-fmax(0,audio_visual_position()/(RATE*15/project.bpm)-origin)*15/project.bpm;
        playlist_start=origin/STEPS;
        if(!playing || !song){playing=song=1;reset=1;}
        else if(!recording_ui.active)reset=0;
        if(!recording_ui.active){audio_update(&project,1,1,pattern,reset,output_volume,origin,0,0);reset=0;}
        if(midi_take.pattern>=0){pattern=midi_take.pattern;piano_channel=target;piano_channels[pattern][target]=1;}
        for(int i=0;i<32;i++)if(midi_keys[i].used && midi_keys[i].target==target && (record_mask&RECORD_NOTES))midi_take_note(&midi_take,&project,midi_keys[i].mchannel,midi_keys[i].pitch,midi_keys[i].velocity,midi_take.time);
        memset(song_loop,0,sizeof song_loop);windows_focus(&windows,2);browser_focus=0;
        snprintf(status,sizeof status,"Recording MIDI to %s; Stop finishes the take",project.channel_names[target]);
    }
}
static int mixer_decay_active,effect_slot;
static int eq_bus,eq_slot,eq_band;
static Spectrum master_spectrum,eq_spectrum;
static int spectrum_active;
static void meters_update(void) {
    float peaks[INSERTS+1][2]; audio_meters(peaks);
    double now=GetTime(),elapsed=fmax(0,now-meter_time); meter_time=now;
    float release=expf(-elapsed/.18),peak_release=expf(-elapsed/.45); mixer_decay_active=0;
    for(int id=0;id<=INSERTS;id++) for(int side=0;side<2;side++) {
        meter_level[id][side]=fmaxf(peaks[id][side],meter_level[id][side]*release);
        if(peaks[id][side]>=meter_hold[id][side] && peaks[id][side]>.00001f) { meter_hold[id][side]=peaks[id][side]; meter_until[id][side]=now+.5; }
        else if(now>meter_until[id][side]) meter_hold[id][side]*=peak_release;
        mixer_decay_active|=meter_hold[id][side]>.001f || meter_level[id][side]>.001f;
    }
}
static void spectrum_update(void) {
    static int initialized,last_bus=-1;
    if(!initialized) { spectrum_reset(&master_spectrum); spectrum_reset(&eq_spectrum); initialized=1; }
    int bus=windows.editors[5].visible?eq_bus:-1;
    audio_spectrum_bus(bus);
    float pcm[8192*2]; unsigned n;
    n=audio_spectrum_read(0,pcm,8192); spectrum_push(&master_spectrum,pcm,n);
    n=audio_spectrum_read(1,pcm,8192);
    if(bus!=last_bus) { spectrum_reset(&eq_spectrum); last_bus=bus; }
    else spectrum_push(&eq_spectrum,pcm,n);
    spectrum_active=0;
    for(int i=0;i<SPECTRUM_BINS;i++) spectrum_active|=master_spectrum.db[i]>-78 || (bus>=0 && eq_spectrum.db[i]>-78);
}
static void spectrum_draw(const Spectrum *s,Rectangle r,int colored) {
    for(int i=0;i<SPECTRUM_BINS;i++) {
        float height=fmaxf(0,fminf(1,(s->db[i]+78)/78))*r.height;
        Color color=colored?ColorFromHSV(260-240.f*i/(SPECTRUM_BINS-1),.45f,ui_theme.light?.55f:.8f):accent;
        float x=r.x+r.width*i/SPECTRUM_BINS,w=r.width/SPECTRUM_BINS;
        DrawRectangleRec((Rectangle){x,r.y+r.height-height,w+.1f,height},color);
    }
}
static float eq_x(float frequency,Rectangle r) { return r.x+logf(frequency/20)/logf(1000)*r.width; }
static float eq_y(float gain,Rectangle r) { return r.y+r.height*(.5f-gain/36); }
static void eq_editor(float width,float height) {
    if(eq_bus<0 || eq_bus>project.insert_count || eq_slot<0 || eq_slot>=EFFECT_SLOTS || project.effect_type[eq_bus][eq_slot]!=EFFECT_EQ) {
        eq_drag=-1; label("Select an Equalizer in the Mixer.",16,40,14,muted); return;
    }
    EQSettings *settings=&project.eq[eq_bus][eq_slot],shown=*settings;
    for(int i=0;i<EQ_BANDS;i++) {
        shown.bands[i].frequency=displayed_value(&settings->bands[i].frequency,settings->bands[i].frequency);
        shown.bands[i].gain=displayed_value(&settings->bands[i].gain,settings->bands[i].gain);
        shown.bands[i].q=displayed_value(&settings->bands[i].q,settings->bands[i].q);
    }
    label(fit_text(TextFormat("%s · Slot %d",eq_bus?project.insert_names[eq_bus-1]:"Master",eq_slot+1),width-254,12),12,30,12,muted);
    if(button("Bypass",width-226,26,70,24,project.effect_bypass[eq_bus][eq_slot])) project.effect_bypass[eq_bus][eq_slot]^=1;
    if(button("Load",width-152,26,66,24,0)) preset_action(PRESET_EQ,eq_bus,eq_slot,0);
    if(button("Save",width-82,26,70,24,0)) preset_action(PRESET_EQ,eq_bus,eq_slot,1);
    Rectangle graph={42,64,width-58,height-196};
    DrawRectangleRec(graph,bg); spectrum_draw(&eq_spectrum,graph,1);
    const float frequencies[]={20,100,1000,10000,20000};
    const char *names[]={"20","100","1k","10k","20k"};
    for(int i=0;i<5;i++) {
        float x=eq_x(frequencies[i],graph);
        DrawLineEx((Vector2){x,graph.y},(Vector2){x,graph.y+graph.height},1,ui_theme.grid_minor);
        label(names[i],fminf(width-32,fmaxf(32,x-text_width(names[i],10)/2)),graph.y+graph.height+5,10,muted);
    }
    for(int db=-18;db<=18;db+=6) {
        float y=eq_y(db,graph);
        DrawLineEx((Vector2){graph.x,y},(Vector2){graph.x+graph.width,y},1,db?ui_theme.grid_minor:muted);
        label(TextFormat("%+d",db),8,y-5,10,muted);
    }
    static EQSettings drawn; static float response[257]; static int cached;
    if(!cached || memcmp(&drawn,&shown,sizeof drawn)) {
        for(int i=0;i<=256;i++) response[i]=equalizer_response(shown,spectrum_frequency(i/256.f));
        drawn=shown; cached=1;
    }
    Vector2 last={0};
    for(int x=0;x<=256;x++) {
        float db=response[x];
        Vector2 next={graph.x+graph.width*x/256,eq_y(fmaxf(-18,fminf(18,db)),graph)};
        if(x) smooth_line(last,next,1.8f,ink); last=next;
    }
    for(int i=0;i<EQ_BANDS;i++) {
        EQBand b=shown.bands[i]; Vector2 p={eq_x(b.frequency,graph),eq_y(b.shape==EQ_LOW_CUT || b.shape==EQ_HIGH_CUT?0:b.gain,graph)};
        int over=hover(p.x-10,p.y-10,20,20);
        circle(p.x,p.y,eq_band==i?8:6,b.shape==EQ_OFF?muted:accent);
        label(TextFormat("%d",i+1),p.x-text_width(TextFormat("%d",i+1),10)/2,p.y-5,10,ui_theme.selected_text);
        if(over) {
            snprintf(status,sizeof status,"EQ band %d: %.0f Hz, %+.1f dB | drag to tune; wheel changes Q; right-click to reset",i+1,b.frequency,b.gain);
            if(IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) { eq_band=i; eq_drag=i; }
            float wheel=GetMouseWheelMove();
            if(wheel) settings->bands[i].q=fmaxf(.2f,fminf(10,settings->bands[i].q+wheel*.1f));
            if(IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
                eq_band=i; open_context(16,i,(Vector2){mouse.x+windows.editors[5].rect.x,mouse.y+windows.editors[5].rect.y}); input_enabled=0;
            }
        }
    }
    if(eq_drag>=0) {
        if(IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
            EQBand *b=&settings->bands[eq_drag];
            b->frequency=spectrum_frequency(fmaxf(0,fminf(1,(mouse.x-graph.x)/graph.width)));
            if(b->shape!=EQ_LOW_CUT && b->shape!=EQ_HIGH_CUT) b->gain=fmaxf(-18,fminf(18,(.5f-(mouse.y-graph.y)/graph.height)*36));
        } else eq_drag=-1;
    }
    label("Spectrum: -78 to 0 dBFS",graph.x,graph.y+graph.height+22,10,muted);
    float y=height-55;
    for(int i=0;i<EQ_BANDS;i++) if(button(TextFormat("%d",i+1),12+i*28,y-34,24,22,eq_band==i)) eq_band=i;
    EQBand *b=&settings->bands[eq_band];
    if(button(equalizer_shape_name(b->shape),216,y-34,152,22,0)) open_context(14,eq_band,(Vector2){mouse.x+windows.editors[5].rect.x,mouse.y+windows.editors[5].rect.y});
    if(hover(216,y-34,152,22)) snprintf(status,sizeof status,"Band shape: Bell, shelves, cuts or Off");
    knob_style(92,y+4,&b->frequency,20,20000,equalizer_default().bands[eq_band].frequency,"EQ frequency (Hz)",KNOB_LOGARITHMIC);
    int cut=b->shape==EQ_LOW_CUT || b->shape==EQ_HIGH_CUT;
    if(cut) label("12 dB/oct",174,y,11,muted);
    else knob_style(200,y+4,&b->gain,-18,18,0,"EQ gain (dB)",KNOB_CENTER);
    knob(310,y+4,&b->q,.2f,10,equalizer_default().bands[eq_band].q,"EQ Q (bandwidth)");
    label(TextFormat("%.0f Hz",b->frequency),66,y+24,11,muted);
    label(cut?"Slope":TextFormat("%+.1f dB",b->gain),178,y+24,11,muted);
    label(TextFormat("Q %.2f",b->q),288,y+24,11,muted);
    knob(width-44,y+4,&project.effect_mix[eq_bus][eq_slot],0,1,1,"Equalizer mix");
    label("Mix",width-54,y+24,11,muted);
}

static float meter_height(float level) { return level>.001f?fader_position(level):0; }
static void meter_draw(int id,int x,int top,int bottom,int bar_width) {
    int height=bottom-top,spacing=bar_width+2;
    float orange=meter_height(powf(10,-12.f/20)),zero=meter_height(1);
    DrawRectangle(x-1,top,spacing*2,height,bg);
    for(int side=0;side<2;side++) {
        int left=x+side*spacing; float level=meter_height(meter_level[id][side]),peak=meter_height(meter_hold[id][side]);
        if(level>0) DrawRectangle(left,bottom-level*height,bar_width,level*height,ui_theme.meter_low);
        if(level>orange) DrawRectangle(left,bottom-level*height,bar_width,(level-orange)*height,ui_theme.meter_mid);
        if(level>zero) DrawRectangle(left,bottom-level*height,bar_width,(level-zero)*height,ui_theme.meter_high);
        if(peak>0) DrawRectangle(left-1,fmaxf(top,bottom-peak*height),bar_width+2,1,meter_hold[id][side]>1?ui_theme.meter_high:ink);
    }
    if(hover(x-3,top,spacing*2+4,height)) snprintf(status,sizeof status,"%s output | Peak L %.1f dBFS, R %.1f dBFS | Red above 0 dBFS; markers hold 0.5 s",mixer_name(id),gain_db(meter_hold[id][0]),gain_db(meter_hold[id][1]));
}
static void selected_meter(float height) {
    int top=TITLE+28,bottom=height-34;
    label("dBFS",8,TITLE+6,11,muted);
    meter_draw(mixer_selected,32,top,bottom,7);
    const int ticks[]={6,0,-6,-12,-24,-36,-48,-60};
    for(unsigned i=0;i<sizeof ticks/sizeof *ticks;i++) {
        float y=bottom-meter_height(powf(10,ticks[i]/20.f))*(bottom-top);
        const char *number=TextFormat("%d",ticks[i]);
        label(number,16-text_width(number,10)/2,fmaxf(top,fminf(bottom-10,y-5)),10,muted);
        DrawLine(29,y,31,y,muted);
    }
    float peak=fmaxf(meter_hold[mixer_selected][0],meter_hold[mixer_selected][1]);
    label(peak?TextFormat("%.1f",gain_db(peak)):"-inf",8,height-29,11,peak>1?ui_theme.meter_high:ink);
    if(hover(4,TITLE,54,height-TITLE-18)) snprintf(status,sizeof status,"Selected %s | Peak L %.1f dBFS, R %.1f dBFS",mixer_name(mixer_selected),gain_db(meter_hold[mixer_selected][0]),gain_db(meter_hold[mixer_selected][1]));
}
static void mixer(float width,float height) {
    selected_meter(height);
    int capacity=fmaxf(1,(width-MIXER_PANEL-MIXER_LEFT-8)/51-1),visible=project.insert_count<capacity?project.insert_count:capacity;
    mixer_scroll=(int)fmaxf(0,fminf(fmaxf(0,project.insert_count-visible),mixer_scroll));
    if(hover(0,TITLE,width-220,height-TITLE) && mouse.y>TITLE+60) {
        mixer_scroll=(int)fmaxf(0,fminf(fmaxf(0,project.insert_count-visible),mixer_scroll-GetMouseWheelMove()));
    }
    float fader_top=TITLE+76,fader_bottom=height-74;
    for(int col=0;col<=visible;col++) {
        int id=col?col+mixer_scroll:0,x=MIXER_LEFT+col*51;
        float *v=id?&project.insert_volume[id-1]:&project.master;
        int over=hover(x,TITLE+4,48,height-TITLE-12);
        ui_surface((Rectangle){x,TITLE+4,48,height-TITLE-12},mixer_selected==id?ui_theme.title_focus:cell);
        if(over) snprintf(status,sizeof status,"%s: click to select this mixer channel",mixer_name(id));
        if(over && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) mixer_selected=id;
        DrawRectangle(x+3,TITLE+7,42,17,mixer_selected==id?ui_theme.title_focus:cell);
        label(id?TextFormat("%d",id):"Master",x+5,TITLE+9,id?12:10,ink);
        if(id) {
            label(fit_text(project.insert_names[id-1],42,10),x+3,TITLE+26,10,muted);
            if(hover(x+3,TITLE+25,42,16)) {
                snprintf(status,sizeof status,"%s: right-click to rename",project.insert_names[id-1]);
                if(IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
                    rename_channel=rename_track=-1; rename_mixer=id-1; mixer_selected=id;
                    snprintf(rename_text,sizeof rename_text,"%s",project.insert_names[id-1]); rename_select_all=1; open_popup(2); input_enabled=0;
                }
            }
        }
        int input_over=hover(x+7,height-46,12,12) || (cable_drag>0 && CheckCollisionPointRec(mouse,(Rectangle){x+7,height-46,12,12}));
        DrawRectangleLines(x+9,height-44,8,8,input_over?ui_theme.signal:muted);

        if(input_over) snprintf(status,sizeof status,"%s input: drop an insert output cable here",mixer_name(id));
        if(over && mouse.y<TITLE+24 && IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) { mixer_selected=id; open_context(3,id,(Vector2){mouse.x+windows.editors[3].rect.x,mouse.y+windows.editors[3].rect.y}); }
        if(over && mouse.y<TITLE+24) snprintf(status,sizeof status,"%s: right-click for mixer channel actions",mixer_name(id));
        mute_light(x+11,TITLE+52,id?project.insert_mute:&project.master_mute,id?project.insert_count:0,id?id-1:0,mixer_name(id));
        if(id) knob_style(x+35,TITLE+52,&project.insert_pan[id-1],-1,1,0,"Insert pan",KNOB_PAN);
        meter_draw(id,x+8,fader_top,fader_bottom,3);
        DrawRectangle(x+33,fader_top,3,fader_bottom-fader_top,bg);
        if(hover(x+23,fader_top-4,22,fader_bottom-fader_top+8) && IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) control_menu(v,0,MIXER_GAIN_MAX,1,id?"Insert volume":"Master volume");
        if(hover(x+23,fader_top-4,22,fader_bottom-fader_top+8) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            capture_control(v,0,MIXER_GAIN_MAX,1); control_range=fader_bottom-fader_top;
            /* Anchor the drag to the current handle position rather than the click position. */
            control_bottom=mouse.y+windows.editors[3].rect.y+fader_position(*v)*control_range;
        }
        if(hover(x+23,fader_top-4,22,fader_bottom-fader_top+8) || control_drag==v) SetMouseCursor(MOUSE_CURSOR_RESIZE_NS);
        if(hover(x+23,fader_top-4,22,fader_bottom-fader_top+8) || control_drag==v) snprintf(status,sizeof status,"%s fader: %.1f dB | Mark = 0 dB; right-click for value / automation",mixer_name(id),gain_db(*v));
        DrawLine(x+25,fader_bottom-fader_position(1)*(fader_bottom-fader_top),x+44,fader_bottom-fader_position(1)*(fader_bottom-fader_top),muted);
        int fy=fader_bottom-fader_position(displayed_value(v,*v))*(fader_bottom-fader_top);
        ui_surface((Rectangle){x+26,fy-9,14,18},mixer_selected==id?accent:ui_theme.piano_white);
        DrawLine(x+28,fy,x+38,fy,bg);
        knob_style(x+33,height-59,id?&project.insert_width[id-1]:&project.master_width,0,2,1,"Stereo width (left wider, center unchanged, right mono)",KNOB_WIDTH);
        int arm_over=hover(x+4,height-67,16,16);
        circle(x+12,height-59,6,arm_over || record_armed[id]?ui_theme.meter_high:ui_theme.border);
        circle(x+12,height-59,5,record_armed[id]?ui_theme.meter_high:cell);
        if(arm_over) {
            snprintf(status,sizeof status,"%s record arm: %s | Top-bar Record captures this track's post-fader audio",mixer_name(id),record_armed[id]?"On":"Off");
            if(IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && !recording_active()) { record_armed[id]^=1; input_enabled=0; }
        }
        if(id) {

            int output_over=hover(x+29,height-46,12,12);
            circle(x+35,height-40,5,output_over || cable_drag==id?accent:muted);
            circle(x+35,height-40,2,project.insert_output[id-1]==255?bg:accent);
            if(output_over) {
                snprintf(status,sizeof status,"Insert %d output: drag to an input; right-click to unplug",id);
                if(IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) insert_connect(&project,id,255);
                if(IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) { cable_drag=id; mixer_selected=id; input_enabled=0; }
            }
        }
    }
    if(mixer_selected>0) {
        int col=mixer_selected-mixer_scroll,destination=project.insert_output[mixer_selected-1];
        int target=destination?destination-mixer_scroll:0;
        if(col>=1 && col<=visible && (cable_drag==mixer_selected || (destination!=255 && target>=0 && target<=visible && (!destination || target>=1)))) {
            Vector2 start={MIXER_LEFT+col*51+35,height-40},end=cable_drag==mixer_selected?mouse:(Vector2){MIXER_LEFT+target*51+13,height-40};
            Vector2 points[]={start,{start.x,height-21},{end.x,height-21},end};
            cable(points,Fade(ui_theme.signal,.8f));
        }
    }
    int fx=width-MIXER_PANEL;
    DrawRectangle(fx,TITLE,MIXER_PANEL,height-TITLE,ui_theme.mixer);
    label("Recording input",fx+8,TITLE+8,11,muted);
    device_button(0,fx+8,TITLE+26,MIXER_PANEL-16);
    label("Effects",fx+8,TITLE+70,12,ink);
    for(int slot=0;slot<EFFECT_SLOTS;slot++) {
        int x=fx+8+(slot%5)*29,y=TITLE+90+(slot/5)*24;
        if(button(TextFormat("%d%s",slot+1,project.effect_type[mixer_selected][slot]?"·":""),x,y,27,22,effect_slot==slot)) effect_slot=slot;
        if(hover(x,y,27,22)) snprintf(status,sizeof status,"Effect slot %d: %s",slot+1,project.effect_type[mixer_selected][slot]==EFFECT_EQ?"Equalizer":project.effect_type[mixer_selected][slot]?"Chorus":"Empty");
    }
    int bus=mixer_selected,slot=effect_slot;
    if(project.effect_type[bus][slot]==EFFECT_EMPTY) {
        if(button("+ Chorus",fx+8,TITLE+148,MIXER_PANEL-16,25,0)) {
            project.effect_type[bus][slot]=EFFECT_CHORUS; project.chorus[bus][slot]=chorus_default();
            project.effect_mix[bus][slot]=.5f; project.effect_bypass[bus][slot]=0;
        }
        if(button("+ Equalizer",fx+8,TITLE+177,MIXER_PANEL-16,25,0)) {
            project.effect_type[bus][slot]=EFFECT_EQ; project.eq[bus][slot]=equalizer_default(); project.effect_mix[bus][slot]=1; project.effect_bypass[bus][slot]=0;
            eq_bus=bus; eq_slot=slot; eq_drag=-1; windows_focus(&windows,5);
        }
    } else {
        label(project.effect_type[bus][slot]==EFFECT_EQ?"Equalizer":"Chorus",fx+8,TITLE+148,12,ink);
        if(button("x",fx+MIXER_PANEL-28,TITLE+144,20,22,0)) {
            project.effect_type[bus][slot]=EFFECT_EMPTY;
            for(int a=project.automation_count-1;a>=0;a--) {
                ParameterTarget t=project.automations[a].target;
                if(((t.parameter>=PARAM_CHORUS_RATE && t.parameter<=PARAM_EFFECT_MIX) || (t.parameter>=PARAM_EQ_FIRST && t.parameter<=PARAM_EQ_LAST)) && t.owner==(unsigned)bus && t.slot==(unsigned)slot) automation_delete(&project,a);
            }
            control_drag=NULL; automation_selected=automation_node=-1; picker_drag=-1;
        } else if(project.effect_type[bus][slot]==EFFECT_EQ) {
            if(button("Open Equalizer",fx+8,TITLE+177,MIXER_PANEL-16,25,0)) { eq_bus=bus; eq_slot=slot; eq_drag=-1; windows_focus(&windows,5); }
            if(button("Bypass",fx+8,TITLE+225,MIXER_PANEL-16,24,project.effect_bypass[bus][slot])) project.effect_bypass[bus][slot]^=1;
            if(button("Load",fx+8,TITLE+253,70,24,0)) preset_action(PRESET_EQ,bus,slot,0);
            if(button("Save",fx+82,TITLE+253,70,24,0)) preset_action(PRESET_EQ,bus,slot,1);
        } else {
            ChorusSettings *settings=&project.chorus[bus][slot];
            knob(fx+27,TITLE+183,&settings->rate,.05f,5,.513f,"Chorus rate (Hz)");
            knob(fx+78,TITLE+183,&settings->depth,0,8,1.85f,"Chorus depth (ms)");
            knob(fx+129,TITLE+183,&project.effect_mix[bus][slot],0,1,.5f,"Chorus mix");
            label("Rate",fx+15,TITLE+203,10,muted); label("Depth",fx+64,TITLE+203,10,muted); label("Mix",fx+120,TITLE+203,10,muted);
            if(button("Bypass",fx+8,TITLE+225,MIXER_PANEL-16,24,project.effect_bypass[bus][slot])) project.effect_bypass[bus][slot]^=1;
            if(button("Load",fx+8,TITLE+253,70,24,0)) preset_action(PRESET_CHORUS,bus,slot,0);
            if(button("Save",fx+82,TITLE+253,70,24,0)) preset_action(PRESET_CHORUS,bus,slot,1);
        }
    }
    float area=fx-MIXER_LEFT-8,thumb=area*visible/INSERTS,pos=MIXER_LEFT+area*mixer_scroll/INSERTS;
    DrawRectangle(MIXER_LEFT,height-17,area,9,bg); ui_surface((Rectangle){pos,height-17,fmaxf(12,thumb),9},mixer_pan || hover(MIXER_LEFT,height-20,area,14)?accent:muted);
    if(hover(MIXER_LEFT,height-20,area,14)) {
        snprintf(status,sizeof status,"Mixer: wheel over inserts or drag this scrollbar to browse all 100 inserts");
        if(IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) { mixer_pan=1; mixer_scroll=fmaxf(0,fminf(INSERTS-visible,(mouse.x-MIXER_LEFT-thumb/2)/area*INSERTS)); input_enabled=0; }
    }
}
static void delete_pattern(int target) {
    if(edit_clipboard.kind==COPY_CLIPS) edit_clipboard.kind=COPY_EMPTY;
    int old_count=project.pattern_count;
    if(!pattern_delete(&project,target)) return;
    for(int i=target;i<old_count-1;i++) {
        rack_view[i]=rack_view[i+1]; rack_range[i]=rack_range[i+1]; piano_start[i]=piano_start[i+1];
        piano_span[i]=piano_span[i+1]; piano_pan[i]=piano_pan[i+1]; piano_range[i]=piano_range[i+1];
        memcpy(pattern_loop[i],pattern_loop[i+1],sizeof pattern_loop[i]);
        memcpy(piano_channels[i],piano_channels[i+1],sizeof piano_channels[i]);
        memcpy(note_selected[i],note_selected[i+1],sizeof note_selected[i]);
    }
    int last=old_count>1?project.pattern_count:0;
    rack_view[last]=rack_range[last]=piano_start[last]=piano_pan[last]=piano_range[last]=0; piano_span[last]=STEPS;
    memset(pattern_loop[last],0,sizeof pattern_loop[last]); memset(piano_channels[last],0,sizeof piano_channels[last]); memset(note_selected[last],0,sizeof note_selected[last]);
    pattern=fminf(target,project.pattern_count-1); playing=0; reset=1; note_drag=NULL; piano_gesture=moving_note=0;
    arrangement.gesture=IDLE; arrangement.source_pattern=-1;
    memset(arrangement.selected,0,sizeof arrangement.selected);
    snprintf(status,sizeof status,"Deleted pattern and its Playlist clips%s",old_count==1?"; kept one blank pattern":"");
}
static void draw_context(void) {
    if(!context_kind) return;
    float scale=ui_scale();
    int instrument_rows[CHANNELS],instrument_count=context_kind==17?pattern_instruments(context_target,instrument_rows):0;
    int w=context_kind==14 || context_kind==16?160:(context_kind==5 || context_kind==8 || context_kind==12)?304:228,h=context_kind==9?133:context_kind==21?8+25*FM_FACTORY_COUNT:context_kind==22?83:context_kind==23?108:context_kind==19?108:context_kind==20?58:context_kind==17?8+25*(int)fminf(8,instrument_count):context_kind==18?58:context_kind==4?108:context_kind==16?32:context_kind==15?58:context_kind==14?8+EQ_SHAPES*25:context_kind==13?58:context_kind==2?158:context_kind==7?133:(context_kind==5 || context_kind==8 || context_kind==12)?76:context_kind==3 && !context_target?32:82,x=fmaxf(0,fminf(GetScreenWidth()/scale-w,context_position.x)),y=fmaxf(0,fminf(GetScreenHeight()/scale-h,context_position.y));
    input_enabled=1;
    if(IsKeyPressed(KEY_ESCAPE) || (!context_opened && IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && !hover(x,y,w,h))) { context_kind=0; return; }
    ui_frame((Rectangle){x,y,w,h});
    int target=context_target,kind=context_kind;
    if(kind==21 || kind==22 || kind==23) {
        if(target<0 || target>=project.channel_count || project.instrument[target]!=INSTRUMENT_FM) { context_kind=0; return; }
        const char *engines[]={"Custom FM","Six-operator FM","Analog"},*waves[]={"Saw","Pulse","Triangle","Sine"};
        int count=kind==21?FM_FACTORY_COUNT:kind==22?3:4;
        for(int i=0;i<count;i++) if(button(kind==21?fm_factory_name(i):kind==22?engines[i]:waves[i],x+4,y+4+i*25,w-8,24,0)) {
            if(kind==21) project.fm[target]=fm_factory(i);
            else if(kind==22) project.fm[target].engine=i;
            else project.fm[target].analog.wave=i;
            fm_graph_drag=-1; context_kind=0; return;
        }
    } else if(kind==19 || kind==20) {
        if(target<0 || target>=project.channel_count || project.instrument[target]!=INSTRUMENT_FM) { context_kind=0; return; }
        const char *shapes[]={"Sine","Triangle","Saw","Square"},*routing[]={"Body + Attack","Attack into Body"};
        float *value=kind==19?&project.fm[target].lfo_shape:&project.fm[target].routing;
        for(int i=0;i<(kind==19?4:2);i++) if(button(kind==19?shapes[i]:routing[i],x+4,y+4+i*25,w-8,24,*value==i)) { *value=i; context_kind=0; return; }
    } else if(kind==17) {
        int visible=(int)fminf(8,instrument_count);
        if(hover(x,y,w,h)) instrument_replace_scroll=(int)fmaxf(0,fminf(instrument_count-visible,instrument_replace_scroll-GetMouseWheelMove()));
        for(int row=0;row<visible;row++) {
            int c=instrument_rows[row+instrument_replace_scroll];
            if(button(fit_text(TextFormat("Replace %s...",project.channel_names[c]),w-16,13),x+4,y+4+row*25,w-8,24,0)) {
                open_context(18,c,context_position); return;
            }
        }
        if(hover(x,y,w,h)) snprintf(status,sizeof status,"Choose the pattern's instrument to replace; the change applies across all patterns. Scroll for more instruments.");
    } else if(kind==18) {
        if(target<0 || target>=project.channel_count) { context_kind=0; return; }
        for(int type=INSTRUMENT_SAMPLER;type<=INSTRUMENT_FM;type++) {
            int enabled=input_enabled;
            if(type==INSTRUMENT_FM && project.channel_audio[target]) input_enabled=0;
            if(button(type==INSTRUMENT_FM?"FM Synth":"Sampler",x+4,y+4+type*25,w-8,24,project.instrument[target]==type)) {
                context_kind=0; replace_instrument(target,type); return;
            }
            input_enabled=enabled;
            if(hover(x+4,y+4+type*25,w-8,24)) snprintf(status,sizeof status,
                type==INSTRUMENT_FM && project.channel_audio[target]?"Audio clips require a Sampler; FM Synth replaces pattern instruments":"Replace this channel's instrument; keep notes, sample settings, name and mixer routing across all patterns");
        }
    } else if(kind==16) {
        if(eq_bus>project.insert_count || project.effect_type[eq_bus][eq_slot]!=EFFECT_EQ || target<0 || target>=EQ_BANDS) { context_kind=0; return; }
        if(button("Reset",x+4,y+4,w-8,24,0)) { project.eq[eq_bus][eq_slot].bands[target]=equalizer_default().bands[target]; context_kind=0; }
        if(hover(x+4,y+4,w-8,24)) snprintf(status,sizeof status,"Reset band %d frequency, gain, Q and shape to defaults",target+1);
    } else if(kind==14) {
        if(eq_bus>project.insert_count || project.effect_type[eq_bus][eq_slot]!=EFFECT_EQ || target<0 || target>=EQ_BANDS) { context_kind=0; return; }
        EQBand *band=&project.eq[eq_bus][eq_slot].bands[target];
        for(int shape=0;shape<EQ_SHAPES;shape++) if(button(equalizer_shape_name(shape),x+4,y+4+shape*25,w-8,24,band->shape==(unsigned)shape)) { band->shape=shape; context_kind=0; }
    } else if(kind==13) {
        for(int mode=0;mode<2;mode++) if(button(mode?"Stretch":"Resample",x+4,y+4+mode*25,w-8,24,project.sampler[target].stretch==mode)) { project.sampler[target].stretch=mode; context_kind=0; }
    } else if(kind==9 || kind==15) {
        if(kind==9) {
            if(button("MIDI Learn",x+4,y+79,w-8,24,0)){midi_learn_target=automation_target;midi_learning=1;context_kind=0;snprintf(status,sizeof status,"Move a MIDI CC knob or fader to link %s; Escape cancels",menu_name);}
            if(button("Clear MIDI link",x+4,y+104,w-8,24,0)){midi_unbind(&project,automation_target);context_kind=0;}
        }
        if(button("Reset",x+4,y+4,w-8,24,0)) { *menu_value=menu_initial; context_kind=0; }
        if(hover(x+4,y+4,w-8,24)) snprintf(status,sizeof status,"Reset %s to %.6g",menu_name,menu_initial);
        if(button("Enter value",x+4,y+29,w-8,24,0)) {
            begin_number(menu_value,menu_low,menu_high,menu_initial,menu_name);
            const ParameterDescriptor *descriptor=kind==9?parameter_descriptor(automation_target.parameter):NULL;
            number_integer=descriptor && descriptor->kind==PARAMETER_INTEGER; context_kind=0;
        }
        if(kind==9 && button("Create automation",x+4,y+54,w-8,24,0)) { create_automation(); context_kind=0; }
    } else if(kind==10) {
        if(button("Create automation",x+4,y+4,w-8,24,0)) { create_automation(); context_kind=0; }
        if(button(automation_target.parameter==PARAM_MASTER_MUTE?"Clear all solos":"Toggle solo",x+4,y+29,w-8,24,0)) {
            if(automation_target.parameter==PARAM_CHANNEL_MUTE) solo_toggle(project.mute,project.channel_count,automation_target.owner);
            else if(automation_target.parameter==PARAM_INSERT_MUTE) solo_toggle(project.insert_mute,project.insert_count,automation_target.owner);
            else { for(int c=0;c<CHANNELS;c++) project.mute[c]&=~2; for(int i=0;i<INSERTS;i++) project.insert_mute[i]&=~2; for(int l=0;l<LANES;l++) project.lane_mute[l]&=~2; }
            context_kind=0;
        }
    } else if(kind==11) {
        if(button("Rename",x+4,y+4,w-8,24,0)) { rename_track=rename_mixer=rename_channel=-1; snprintf(rename_text,sizeof rename_text,"%s",project.automations[target].name); rename_select_all=1; open_popup(2); rename_automation=target; context_kind=0; }
        if(button("Color",x+4,y+29,w-8,24,0)) { context_kind=12; context_opened=1; }
        if(button("Delete",x+4,y+54,w-8,24,0)) { automation_delete(&project,target); automation_selected=-1; automation_node=-1; picker_drag=-1; arrangement.gesture=IDLE; arrangement.source_pattern=-1; memset(arrangement.selected,0,sizeof arrangement.selected); context_kind=0; }
    } else if(kind==6) {
        for(int i=0;i<3;i++) if(button(rack_filters[i],x+4,y+4+i*25,w-8,24,rack_filter==i)) { rack_filter=i; rack_scroll=0; context_kind=0; }
    } else if(kind==5 || kind==8 || kind==12) {
        uint32_t *color=kind==12?&project.automations[target].color:kind==8?&project.channel_colors[target]:&project.pattern_colors[target];
        int choice=color_picker(x+8,y+10,w-16,*color);
        if(choice>=0) { *color=pattern_palette[choice]; context_kind=0; }
    } else if(kind==7) {
        if(button("Rename",x+4,y+4,w-8,24,0)) { rename_track=rename_mixer=-1; rename_channel=target; snprintf(rename_text,sizeof rename_text,"%s",project.channel_names[target]); rename_select_all=1; open_popup(2); context_kind=0; }
        if(button("Color",x+4,y+29,w-8,24,0)) { context_kind=8; context_opened=1; }
        if(button("Delete",x+4,y+54,w-8,24,0)) { picker_drag=-1; delete_channel(target); context_kind=0; }
        if(button(project.instrument[target]==INSTRUMENT_FM?"Replace with sample...":"Relink sample...",x+4,y+79,w-8,24,0)) { relink_channel=target; context_kind=0; project_file_action(7); }
        if(button("Replace instrument...",x+4,y+104,w-8,24,0)) { open_context(18,target,context_position); return; }
        if(hover(x+4,y+54,w-8,24)) snprintf(status,sizeof status,"Delete this Audio channel and all its Playlist clips; the source file stays on disk");
    } else if(kind==4) {
        if(button("Rename",x+4,y+4,w-8,24,0)) { pattern=target; begin_rename(); context_kind=0; }
        if(button("Color",x+4,y+29,w-8,24,0)) { context_kind=5; context_opened=1; }
        if(button("Delete",x+4,y+54,w-8,24,0)) { delete_pattern(target); context_kind=0; }
        if(button("Replace instrument...",x+4,y+79,w-8,24,0)) { instrument_replace_scroll=0; open_context(17,target,context_position); return; }
        if(hover(x+4,y+54,w-8,24)) snprintf(status,sizeof status,"Delete this pattern and all its Playlist clips; the last pattern becomes blank");
    } else if(kind==1) {
        if(button("Stay on top",x+4,y+4,w-8,24,windows.editors[target].pinned)) { windows_pin(&windows,target); context_kind=0; }
        if(button(windows.editors[target].maximized?"Restore":"Maximize",x+4,y+29,w-8,24,0)) {
            Editor *e=&windows.editors[target]; if(e->maximized) { e->rect=e->restore; e->maximized=0; } else { e->restore=e->rect; e->maximized=1; } context_kind=0;
        }
        if(button("Hide window",x+4,y+54,w-8,24,0)) { windows.editors[target].visible=0; context_kind=0; }
    } else if(kind==2) {
        if(button("Go to Piano Roll",x+4,y+4,w-8,24,0)) { channel=piano_channel=target; piano_channels[pattern][target]=1; windows_focus(&windows,2); context_kind=0; }
        if(button("Mute channel",x+4,y+29,w-8,24,project.mute[target]&1)) { project.mute[target]^=1; context_kind=0; }
        if(button("Rename",x+4,y+54,w-8,24,0)) { rename_track=rename_mixer=-1; rename_channel=target; snprintf(rename_text,sizeof rename_text,"%s",project.channel_names[target]); rename_select_all=1; open_popup(2); context_kind=0; }
        if(button("Delete channel",x+4,y+79,w-8,24,0)) { delete_channel(target); context_kind=0; }
        if(button(project.instrument[target]==INSTRUMENT_FM?"Replace with sample...":"Relink sample...",x+4,y+104,w-8,24,0)) { relink_channel=target; context_kind=0; project_file_action(7); }
        if(button("Replace instrument...",x+4,y+129,w-8,24,0)) { open_context(18,target,context_position); return; }
    } else if(!target) {
        if(button("Reset Master volume",x+4,y+4,w-8,24,0)) { project.master=1; context_kind=0; }
    } else {
        if(button("Mute insert",x+4,y+4,w-8,24,target && (project.insert_mute[target-1]&1))) { if(target) project.insert_mute[target-1]^=1; context_kind=0; }
        if(button("Solo insert",x+4,y+29,w-8,24,target && (project.insert_mute[target-1]&2))) { if(target) solo_toggle(project.insert_mute,project.insert_count,target-1); context_kind=0; }
        if(button("Reset mixer channel",x+4,y+54,w-8,24,0)) { if(target) insert_reset(&project,target); else project.master=1; context_kind=0; }
    }
}
static void typing_piano(int blocked) {
    /* GLFW key tokens describe physical US positions, including Spanish - and +. */
    static const int keys[]={KEY_Z,KEY_S,KEY_X,KEY_D,KEY_C,KEY_V,KEY_G,KEY_B,KEY_H,KEY_N,KEY_J,KEY_M,KEY_COMMA,KEY_L,KEY_PERIOD,KEY_SEMICOLON,KEY_SLASH,
        KEY_Q,KEY_TWO,KEY_W,KEY_THREE,KEY_E,KEY_R,KEY_FIVE,KEY_T,KEY_SIX,KEY_Y,KEY_SEVEN,KEY_U,KEY_I,KEY_NINE,KEY_O,KEY_ZERO,KEY_P,KEY_LEFT_BRACKET,KEY_EQUAL,KEY_RIGHT_BRACKET};
    static unsigned char held[sizeof keys/sizeof *keys];
    int enabled=typing_keys && !blocked && !browser_focus && IsWindowFocused() && !command_down() && !IsKeyDown(KEY_LEFT_ALT) && !IsKeyDown(KEY_RIGHT_ALT);
    for(unsigned i=0;i<sizeof keys/sizeof *keys;i++) {
        int pitch=i<17?48+i:60+i-17;
        if(held[i] && (!enabled || !IsKeyDown(keys[i]))) {
            if(keyboard_notes[held[i]-1][pitch]) keyboard_notes[held[i]-1][pitch]--;
            audio_key(i,channel,pitch,0); held[i]=0;
        }
        if(enabled && !held[i] && IsKeyPressed(keys[i])) {
            audio_key(i,channel,pitch,1); held[i]=channel+1; keyboard_notes[channel][pitch]++;
        }
    }
}
static int sampler_toggle(int x,int y,int width,const char *name,int active) {
    int over=hover(x,y,width,22);
    if(over) SetMouseCursor(MOUSE_CURSOR_POINTING_HAND);
    if(over) ui_surface((Rectangle){x,y,width,22},ui_theme.hover);
    circle(x+9,y+11,7,over?ink:ui_theme.border); circle(x+9,y+11,5,cell);
    if(active) circle(x+9,y+11,3,accent);
    label(name,x+24,y+4,13,ink);
    return over && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
}
static void sampler_switch(int x,int y,int width,const char *name,uint8_t *flags,int bit) {
    if(sampler_toggle(x,y,width,name,*flags&bit)) *flags^=bit;
}
static const unsigned fm_envelope_ids[3][4]={
    {PARAM_FM_ATTACK,PARAM_FM_DECAY,PARAM_FM_SUSTAIN,PARAM_FM_RELEASE},
    {PARAM_FM_MOD_ATTACK,PARAM_FM_MOD_DECAY,PARAM_FM_MOD_SUSTAIN,PARAM_FM_MOD_RELEASE},
    {PARAM_FM_ATTACK_ATTACK,PARAM_FM_ATTACK_DECAY,PARAM_FM_ATTACK_SUSTAIN,PARAM_FM_ATTACK_RELEASE}
};
static void fm_graph_update_at(float x,float y) {
    if(fm_graph_channel<0 || fm_graph_channel>=project.channel_count) return;
    FMSettings *s=&project.fm[fm_graph_channel];
    if(fm_graph_drag>=4) {
        int stage=fm_graph_drag-4; float slot=(fm_graph_area.width-16)/4;
        float position=fmaxf(0,fminf(1,(x-fm_graph_area.x-8-slot*(stage+.1f))/(slot*.8f)));
        s->dx7.value[fm_dx_drag_base+stage]=roundf((1-position)*99);
        s->dx7.value[fm_dx_drag_base+4+stage]=roundf(fmaxf(0,fminf(99,(1-(y-fm_graph_area.y-6)/fmaxf(1,fm_graph_area.height-12))*99)));
        return;
    }
    if(fm_graph_drag==3) {
        float position=fmaxf(0,fminf(1,(x-fm_graph_area.x-8)/(fm_graph_area.width-16)));
        s->lfo_rate=.1f*powf(120,position);
        float amount=fmaxf(0,fminf(1,(fm_graph_area.y+fm_graph_area.height-22-y)/(fm_graph_area.height-34)));
        if(fm_motion_target) s->tremolo=amount; else s->vibrato=amount*100;
        return;
    }
    int field=fm_graph_drag==0?0:fm_graph_drag==1?1:3;
    const ParameterDescriptor *info=parameter_descriptor(fm_envelope_ids[fm_tab][field]);
    float slot=(fm_graph_area.width-24)/3;
    float position=fmaxf(0,fminf(1,(x-fm_graph_area.x-8-slot*fm_graph_drag)/(.9f*slot)));
    float value=expm1f(position*log1pf(info->high*100))/100;
    *(float *)fm_parameter_pointer(s,info->id)=fmaxf(info->low,fminf(info->high,value));
    if(fm_graph_drag==1) {
        float sustain=fmaxf(0,fminf(1,(fm_graph_area.y+fm_graph_area.height-22-y)/(fm_graph_area.height-34)));
        *(float *)fm_parameter_pointer(s,fm_envelope_ids[fm_tab][2])=sustain;
    }
}
static void fm_graph_update(void) {
    Rect r=windows.editors[4].rect; fm_graph_update_at(mouse.x-r.x,mouse.y-r.y);
}
static void fm_control(FMSettings *s,unsigned id,const char *caption,int x,int y) {
    const ParameterDescriptor *info=parameter_descriptor(id);
    float *value=(float *)fm_parameter_pointer(s,id);
    int logarithmic=info->kind==PARAMETER_LOGARITHMIC ||
        (info->low>0 && (!strcmp(caption,"Attack") || !strcmp(caption,"Decay") || !strcmp(caption,"Release")));
    int fixed_coarse=id>=PARAM_DX7_FIRST && id<PARAM_DX7_FIRST+126 &&
        (id-PARAM_DX7_FIRST)%21==18 && s->dx7.value[id-PARAM_DX7_FIRST-1];
    knob_style(x,y,value,info->low,info->high,info->initial,info->name,
        fixed_coarse?KNOB_FIXED_COARSE:logarithmic?KNOB_LOGARITHMIC:info->low<0?KNOB_CENTER:KNOB_NORMAL);
    if(control_drag==value && logarithmic) control_logarithmic=1;
    label(caption,x-text_width(caption,11)/2,y+17,11,muted);
    float display=displayed_value(value,*value);
    const char *shown;
    if(id>=PARAM_DX7_FIRST && id<=PARAM_DX7_LAST) {
        int index=id-PARAM_DX7_FIRST,field=index%21;
        if(index>=DX7_NATIVE_PARAMETERS) shown=TextFormat("%.1f%%",display);
        else if(index<126 && field==18) {
            int base=index-field;
            double fine=s->dx7.value[base+19]/100;
            if(s->dx7.value[base+17]) {
                double frequency=pow(10,((int)display&3)+fine);
                shown=frequency>=1000?TextFormat("%.2f kHz",frequency/1000):
                    TextFormat(frequency<10?"%.2f Hz":"%.0f Hz",frequency);
            } else shown=TextFormat("%.2fx",fmaxf(.5f,display)*(1+fine));
        }
        else if(index<126 && field==8) { static const char *notes[]={"C","C#","D","D#","E","F","F#","G","G#","A","A#","B"}; int key=(int)display+17; shown=TextFormat("%s%d",notes[key%12],key/12-1); }
        else if(index<126 && (field==11 || field==12)) { static const char *curves[]={"-Lin","-Exp","+Exp","+Lin"}; shown=curves[(int)display]; }
        else if(index==136 || index==141) shown=display>=.5f?"On":"Off";
        else if(index==142) { static const char *waves[]={"Triangle","Saw down","Saw up","Square","Sine","S&H"}; shown=waves[(int)display]; }
        else if(index<126 && field==20) shown=TextFormat("%+.0f",display-7);
        else if(index==134) shown=TextFormat("%.0f",display+1);
        else if(index==144) shown=TextFormat("%+.0f st",display-24);
        else shown=TextFormat("%.0f",display);
    }
    else if(id==PARAM_FM_FILTER_CUTOFF) shown=TextFormat(display>=1000?"%.2g kHz":"%.0f Hz",display>=1000?display/1000:display);
    else if(id==PARAM_FM_CARRIER_RATIO || id==PARAM_FM_ATTACK_RATIO) shown=TextFormat("%.2gx",display);
    else if(id==PARAM_FM_BODY_PITCH) shown=TextFormat("%+.1f st",display);
    else if(id==PARAM_FM_CARRIER_DETUNE || id==PARAM_FM_BODY_DETUNE || id==PARAM_FM_ATTACK_DETUNE) shown=TextFormat("%+.0f ct",display);
    else if(id==PARAM_FM_ATTACK || id==PARAM_FM_DECAY || id==PARAM_FM_RELEASE || id==PARAM_FM_MOD_ATTACK || id==PARAM_FM_MOD_DECAY || id==PARAM_FM_MOD_RELEASE ||
            id==PARAM_FM_ATTACK_ATTACK || id==PARAM_FM_ATTACK_DECAY || id==PARAM_FM_ATTACK_RELEASE || id==PARAM_FM_LFO_FADE)
        shown=display<1?TextFormat("%.0f ms",display*1000):TextFormat("%.2g s",display);
    else shown=TextFormat("%.2g",display);
    label(shown,x-text_width(shown,10)/2,y+31,10,ink);
    if(id>=PARAM_DX7_FIRST+DX7_NATIVE_PARAMETERS && id<=PARAM_DX7_LAST &&
       (hover(x-16,y-16,32,32) || control_drag==value))
        snprintf(status,sizeof status,"Key tracking: 0%% fixed, 50%% half an octave per octave, 100%% follows notes. Anchor: middle C (C4).");
}
static void fm_envelope_graph(FMSettings *s,Rectangle plot) {
    DrawRectangleRec(plot,bg);
    float slot=(plot.width-24)/3,base=plot.y+plot.height-22,range=plot.height-34;
    float times[3]={*fm_parameter_pointer(s,fm_envelope_ids[fm_tab][0]),*fm_parameter_pointer(s,fm_envelope_ids[fm_tab][1]),*fm_parameter_pointer(s,fm_envelope_ids[fm_tab][3])};
    int fields[]={0,1,3}; Vector2 handles[3];
    float sustain=*fm_parameter_pointer(s,fm_envelope_ids[fm_tab][2]);
    for(int i=0;i<3;i++) {
        const ParameterDescriptor *info=parameter_descriptor(fm_envelope_ids[fm_tab][fields[i]]);
        float t=log1pf(times[i]*100)/log1pf(info->high*100);
        handles[i]=(Vector2){plot.x+8+slot*i+.9f*slot*t,i==0?base-range:i==1?base-sustain*range:base};
        DrawLineEx((Vector2){plot.x+8+slot*i,plot.y+8},(Vector2){plot.x+8+slot*i,base},1,ui_theme.grid_minor);
    }
    smooth_line((Vector2){plot.x+8,base},handles[0],1.5f,accent);
    Vector2 previous=handles[0];
    for(int i=1;i<=40;i++) {
        float t=i/40.f,level=fm_tab?sustain+(1-sustain)*expf(-4.6051702f*t):1-(1-sustain)*t;
        Vector2 point={handles[0].x+(handles[1].x-handles[0].x)*t,base-level*range};
        smooth_line(previous,point,1.5f,accent); previous=point;
    }
    smooth_line(previous,handles[1],1.5f,accent);
    Vector2 release={plot.x+8+slot*2,handles[1].y};
    smooth_line(handles[1],release,1.5f,accent); smooth_line(release,handles[2],1.5f,accent);
    const char *captions[]={"Attack","Decay / sustain","Release"};
    for(int i=0;i<3;i++) {
        circle(handles[i].x,handles[i].y,4,ink); circle(handles[i].x,handles[i].y,2,accent);
        label(captions[i],plot.x+8+slot*i,base+7,10,muted);
        if(hover(handles[i].x-7,handles[i].y-7,14,14)) {
            SetMouseCursor(MOUSE_CURSOR_POINTING_HAND);
            if(IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) { fm_graph_drag=i; fm_graph_channel=instrument_channel; fm_graph_area=plot; input_enabled=0; }
        }
    }
    if(hover(plot.x,plot.y,plot.width,plot.height)) snprintf(status,sizeof status,"Drag envelope points: left/right changes time; the middle point also changes sustain. Knobs offer exact entry and automation.");
}
static void fm_legacy_editor(float width,float height) {
    int c=instrument_channel; FMSettings *s=&project.fm[c];
    label(fit_text(project.channel_names[c],width-298,14),12,30,14,ink); channel_controls(c,width);
    label("Custom three-operator FM",12,56,11,muted);
    const char *tabs[]={"Sound","Body","Attack","Motion"}; float tabw=(width-24)/4;
    for(int i=0;i<4;i++) if(button(tabs[i],12+i*tabw,78,tabw-3,26,fm_tab==i)) fm_tab=i;
    float column=(width-32)/4;
    const unsigned head[4][4]={
        {PARAM_FM_CARRIER_RATIO,PARAM_FM_CARRIER_DETUNE,PARAM_FM_VELOCITY,0},
        {PARAM_FM_BODY_PITCH,PARAM_FM_BODY_DETUNE,PARAM_FM_RATIO,PARAM_FM_DEPTH},
        {PARAM_FM_ATTACK_RATIO,PARAM_FM_ATTACK_DETUNE,PARAM_FM_ATTACK_DEPTH,PARAM_FM_VELOCITY},
        {PARAM_FM_LFO_RATE,PARAM_FM_VIBRATO,PARAM_FM_TREMOLO,PARAM_FM_LFO_FADE}
    };
    const char *captions[4][4]={{"Pitch","Fine","Velocity",""},{"Pitch","Fine","Harmonic","FM amount"},{"Pitch","Fine","FM amount","Velocity"},{"Speed","Pitch motion","Volume motion","Fade in"}};
    const char *descriptions[]={"Audible oscillator and volume envelope","Long-lived tone modulation","Short, bright attack modulation","A separate, per-note LFO for pitch and volume"};
    label(descriptions[fm_tab],20,113,12,muted);
    for(int i=0;i<4;i++) if(head[fm_tab][i]) fm_control(s,head[fm_tab][i],captions[fm_tab][i],20+column*(i+.5f),139);
    if(fm_tab==0) {
        int x=20+column*3;
        if(button(s->routing?"Stacked":"Body + Attack",x,131,column-8,24,0)) open_context(20,c,(Vector2){mouse.x+windows.editors[4].rect.x,mouse.y+windows.editors[4].rect.y});
        if(hover(x,131,column-8,24) && IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) control_menu(&s->routing,0,1,0,"FM routing");
        label("Routing",x+8,159,11,muted);
    }
    if(fm_tab<3) {
        const char *names[]={"Attack","Decay","Sustain","Release"};
        label("Envelope",20,184,12,muted);
        for(int i=0;i<4;i++) fm_control(s,fm_envelope_ids[fm_tab][i],names[i],20+column*(i+.5f),207);
        fm_envelope_graph(s,(Rectangle){12,252,width-24,height-294});
    } else {
        const char *shapes[]={"Sine","Triangle","Saw","Square"};
        if(button(shapes[(int)s->lfo_shape],12,184,112,24,0)) open_context(19,c,(Vector2){mouse.x+windows.editors[4].rect.x,mouse.y+windows.editors[4].rect.y});
        if(hover(12,184,112,24) && IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) control_menu(&s->lfo_shape,0,3,0,"LFO shape");
        if(button("Pitch",132,184,72,24,!fm_motion_target)) fm_motion_target=0;
        if(button("Volume",208,184,80,24,fm_motion_target)) fm_motion_target=1;
        Rectangle plot={12,216,width-24,height-258}; DrawRectangleRec(plot,bg);
        float base=plot.y+plot.height-22,range=plot.height-34,mid=plot.y+(plot.height-22)/2;
        float amount=fm_motion_target?s->tremolo:s->vibrato/100;
        Vector2 previous={plot.x+8,mid};
        for(int i=0;i<(int)plot.width-16;i++) {
            float t=i/(plot.width-16)*s->lfo_rate*.5f;
            Vector2 point={plot.x+8+i,mid-fm_lfo_value(t,(int)s->lfo_shape)*range*.42f};
            if(i) smooth_line(previous,point,1.5f,Fade(accent,.35f+amount*.65f));
            previous=point;
        }
        Vector2 handle={plot.x+8+logf(s->lfo_rate/.1f)/logf(120)*(plot.width-16),base-amount*range};
        circle(handle.x,handle.y,5,ink); circle(handle.x,handle.y,3,accent);
        label("Drag: speed / amount",plot.x+8,base+7,10,muted);
        if(hover(plot.x,plot.y,plot.width,plot.height)) {
            SetMouseCursor(MOUSE_CURSOR_POINTING_HAND);
            snprintf(status,sizeof status,"Drag the LFO graph horizontally for speed, vertically for %s amount; choose a waveform above",fm_motion_target?"volume":"pitch");
            if(IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) { fm_graph_drag=3; fm_graph_channel=c; fm_graph_area=plot; fm_graph_update_at(mouse.x,mouse.y); input_enabled=0; }
        }
    }
    if(button("Factory",12,height-34,98,24,0)) open_context(21,c,(Vector2){mouse.x+windows.editors[4].rect.x,mouse.y+windows.editors[4].rect.y});
    if(button("Load",114,height-34,64,24,0)) preset_action(PRESET_FM,c,0,0);
    if(button("Save",182,height-34,64,24,0)) preset_action(PRESET_FM,c,0,1);
    double active=audio_key_position(127,c);
    if(button(active>=0?"Release C4":"Play C4",width-112,height-34,100,24,active>=0)) { channel=c; audio_key(127,c,60,active<0); }
}
static const char *fm_sound_name(const FMSettings *s) {
    for(int i=0;i<FM_FACTORY_COUNT;i++) { FMSettings factory=fm_factory(i); if(!memcmp(s,&factory,sizeof factory)) return fm_factory_name(i); }
    return "Edited sound";
}
static void fm_footer(int c,float width,float height) {
    if(button("Factory",12,height-34,98,24,0)) open_context(21,c,(Vector2){mouse.x+windows.editors[4].rect.x,mouse.y+windows.editors[4].rect.y});
    if(button("Load",114,height-34,64,24,0)) preset_action(PRESET_FM,c,0,0);
    if(button("Save",182,height-34,64,24,0)) preset_action(PRESET_FM,c,0,1);
    int active=audio_key_position(127,c)>=0;
    if(button(active?"Release C4":"Play C4",width-112,height-34,100,24,active)) { channel=c; audio_key(127,c,60,!active); }
}
static void fm_controls_row(FMSettings *s,const unsigned *ids,const char *const *names,int count,float width,int y) {
    float column=(width-32)/count;
    for(int i=0;i<count;i++) fm_control(s,ids[i],names[i],20+column*(i+.5f),y);
}
static void dx7_envelope_graph(FMSettings *s,int base,Rectangle plot) {
    DrawRectangleRec(plot,bg); float width=(plot.width-16)/4,height=fmaxf(1,plot.height-12);
    Vector2 previous={plot.x+8,plot.y+height};
    for(int i=0;i<4;i++) {
        float rate=s->dx7.value[base+i],level=s->dx7.value[base+4+i];
        Vector2 point={plot.x+8+width*(i+.1f+.8f*(1-rate/99)),plot.y+6+height*(1-level/99)};
        smooth_line(previous,point,1.5f,accent); circle(point.x,point.y,3,ink); previous=point;
        if(hover(point.x-7,point.y-7,14,14)) {
            SetMouseCursor(MOUSE_CURSOR_POINTING_HAND); snprintf(status,sizeof status,"Drag stage %d: left/right changes rate, up/down changes level",i+1);
            if(IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) { fm_graph_drag=4+i; fm_graph_channel=instrument_channel; fm_graph_area=plot; fm_dx_drag_base=base; input_enabled=0; }
        }
    }
}
static void fm_dx_editor(float width,float height) {
    int c=instrument_channel; FMSettings *s=&project.fm[c];
    label(fit_text(project.channel_names[c],width-298,14),12,30,14,ink); channel_controls(c,width);
    label(fit_text(fm_sound_name(s),width-160,11),12,56,11,muted);
    float tab=(width-24)/7;
    for(int i=0;i<7;i++) if(button(i==6?"Global":TextFormat("Op %d",i+1),12+i*tab,78,tab-3,26,fm_dx_operator==i)) fm_dx_operator=i;
    if(fm_dx_operator<6) {
        int base=(5-fm_dx_operator)*21;
        label(TextFormat("Operator %d",fm_dx_operator+1),20,113,12,muted);
        unsigned head[]={PARAM_DX7_FIRST+base+16,PARAM_DX7_FIRST+base+18,PARAM_DX7_FIRST+base+19,PARAM_DX7_FIRST+base+20,PARAM_DX7_FIRST+base+15,PARAM_DX7_FIRST+DX7_NATIVE_PARAMETERS+base/21};
        const char *head_names[]={"Output","Coarse","Fine","Detune","Velocity","Tracking"}; fm_controls_row(s,head,head_names,6,width,136);
        if(button("Envelope",12,184,100,24,!fm_dx_page)) fm_dx_page=0;
        if(button("Keyboard",116,184,100,24,fm_dx_page)) fm_dx_page=1;
        float *mode=&s->dx7.value[base+17];
        if(button(*mode?"Hz at C4":"Ratio",width-112,184,100,24,0)) {
            *mode=1-*mode; s->dx7.value[DX7_NATIVE_PARAMETERS+base/21]=*mode?0:100;
            if(*mode) s->dx7.value[base+18]=(int)s->dx7.value[base+18]&3;
        }
        if(hover(width-112,184,100,24) && IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) control_menu(mode,0,1,0,"Operator frequency mode");
        if(!fm_dx_page) {
            unsigned rates[4],levels[4]; for(int i=0;i<4;i++) { rates[i]=PARAM_DX7_FIRST+base+i; levels[i]=PARAM_DX7_FIRST+base+4+i; }
            const char *rnames[]={"Rate 1","Rate 2","Rate 3","Release rate"},*lnames[]={"Level 1","Level 2","Sustain level","Release level"};
            fm_controls_row(s,rates,rnames,4,width,226); fm_controls_row(s,levels,lnames,4,width,292);
            dx7_envelope_graph(s,base,(Rectangle){12,336,width-24,height-378});
        } else {
            const int fields[]={8,9,10,14,11,12,13,15}; const char *names[]={"Breakpoint","Left depth","Right depth","Amp motion","Left curve","Right curve","Key rate","Velocity"};
            for(int row=0;row<2;row++) { unsigned ids[4]; for(int i=0;i<4;i++) ids[i]=PARAM_DX7_FIRST+base+fields[row*4+i]; fm_controls_row(s,ids,names+row*4,4,width,226+row*66); }
            label("Curves: 0 -linear, 1 -exponential, 2 +exponential, 3 +linear",20,344,11,muted);
        }
    } else {
        unsigned head[]={PARAM_DX7_FIRST+134,PARAM_DX7_FIRST+135,PARAM_DX7_FIRST+144,PARAM_DX7_FIRST+136};
        const char *names[]={"Algorithm","Feedback","Transpose","Osc sync"}; label("Routing and tuning",20,113,12,muted); fm_controls_row(s,head,names,4,width,136);
        if(button("Motion",12,184,100,24,!fm_dx_page)) fm_dx_page=0;
        if(button("Pitch envelope",116,184,140,24,fm_dx_page)) fm_dx_page=1;
        if(!fm_dx_page) {
            unsigned a[]={PARAM_DX7_FIRST+137,PARAM_DX7_FIRST+138,PARAM_DX7_FIRST+139,PARAM_DX7_FIRST+140},b[]={PARAM_DX7_FIRST+142,PARAM_DX7_FIRST+141,PARAM_DX7_FIRST+143};
            const char *an[]={"Speed","Delay","Pitch amount","Volume amount"},*bn[]={"Waveform","Key sync","Pitch sense"};
            fm_controls_row(s,a,an,4,width,226); fm_controls_row(s,b,bn,3,width,292);
            label("Waves: triangle, saw down, saw up, square, sine, sample & hold",20,344,11,muted);
        } else {
            unsigned rates[4],levels[4]; for(int i=0;i<4;i++) { rates[i]=PARAM_DX7_FIRST+126+i; levels[i]=PARAM_DX7_FIRST+130+i; }
            const char *rn[]={"Rate 1","Rate 2","Rate 3","Release rate"},*ln[]={"Level 1","Level 2","Sustain level","Release level"};
            fm_controls_row(s,rates,rn,4,width,226); fm_controls_row(s,levels,ln,4,width,292);
            dx7_envelope_graph(s,126,(Rectangle){12,336,width-24,height-378});
        }
    }
    fm_footer(c,width,height);
}
static void fm_analog_editor(float width,float height) {
    int c=instrument_channel; FMSettings *s=&project.fm[c];
    label(fit_text(project.channel_names[c],width-298,14),12,30,14,ink); channel_controls(c,width);
    label(fit_text(fm_sound_name(s),width-160,11),12,56,11,muted);
    const char *tabs[]={"Sound","Amplitude","Filter","Motion"}; float tab=(width-24)/4;
    for(int i=0;i<4;i++) if(button(tabs[i],12+i*tab,78,tab-3,26,fm_analog_tab==i)) fm_analog_tab=i;
    if(fm_analog_tab==0) {
        unsigned a[]={PARAM_FM_CARRIER_RATIO,PARAM_FM_ANALOG_DETUNE,PARAM_FM_ANALOG_MIX,PARAM_FM_ANALOG_SUB,PARAM_FM_ANALOG_NOISE},b[]={PARAM_FM_ANALOG_PULSE,PARAM_FM_ANALOG_PWM,PARAM_FM_ANALOG_CHORUS,PARAM_FM_VELOCITY};
        const char *an[]={"Pitch","Detune","Osc 2","Sub","Noise"},*bn[]={"Pulse width","PWM","Chorus","Velocity"};
        label("Oscillators",20,113,12,muted); fm_controls_row(s,a,an,5,width,136);
        const char *waves[]={"Saw","Pulse","Triangle","Sine"};
        if(button(waves[(int)s->analog.wave],12,184,112,24,0)) open_context(23,c,(Vector2){mouse.x+windows.editors[4].rect.x,mouse.y+windows.editors[4].rect.y});
        if(hover(12,184,112,24) && IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) control_menu(&s->analog.wave,0,3,0,"Oscillator waveform");
        fm_controls_row(s,b,bn,4,width,226); label("PWM moves the Pulse shape. Chorus spreads the sound in stereo.",20,296,11,muted);
    } else if(fm_analog_tab==1 || fm_analog_tab==2) {
        fm_tab=fm_analog_tab==1?0:1;
        if(fm_analog_tab==2) {
            unsigned a[]={PARAM_FM_FILTER_CUTOFF,PARAM_FM_FILTER_RESONANCE,PARAM_FM_FILTER_ENV,PARAM_FM_VELOCITY}; const char *names[]={"Cutoff","Resonance","Envelope","Velocity"};
            label("Filter",20,113,12,muted); fm_controls_row(s,a,names,4,width,136);
        } else label("Volume envelope",20,113,12,muted);
        unsigned ids[4]; for(int i=0;i<4;i++) ids[i]=fm_envelope_ids[fm_tab][i]; const char *names[]={"Attack","Decay","Sustain","Release"};
        fm_controls_row(s,ids,names,4,width,207); fm_envelope_graph(s,(Rectangle){12,252,width-24,height-294});
    } else {
        unsigned ids[]={PARAM_FM_LFO_RATE,PARAM_FM_VIBRATO,PARAM_FM_TREMOLO,PARAM_FM_LFO_FADE}; const char *names[]={"Speed","Pitch motion","Volume motion","Fade in"};
        label("Per-note modulation",20,113,12,muted); fm_controls_row(s,ids,names,4,width,136);
        const char *waves[]={"Sine","Triangle","Saw","Square"};
        if(button(waves[(int)s->lfo_shape],12,184,112,24,0)) open_context(19,c,(Vector2){mouse.x+windows.editors[4].rect.x,mouse.y+windows.editors[4].rect.y});
        if(hover(12,184,112,24) && IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) control_menu(&s->lfo_shape,0,3,0,"LFO shape");
        label("Pitch motion adds vibrato; volume motion adds tremolo. PWM follows this LFO.",20,228,11,muted);
    }
    fm_footer(c,width,height);
}
static void fm_editor(float width,float height) {
    FMSettings *s=&project.fm[instrument_channel];
    if(s->engine==1) fm_dx_editor(width,height); else if(s->engine==2) fm_analog_editor(width,height); else fm_legacy_editor(width,height);
    const char *engines[]={"Custom FM","Six-op FM","Analog"};
    if(button(engines[(int)s->engine],width-132,52,120,22,0)) open_context(22,instrument_channel,(Vector2){mouse.x+windows.editors[4].rect.x,mouse.y+windows.editors[4].rect.y});
    if(hover(width-132,52,120,22)) snprintf(status,sizeof status,"Choose the synthesis engine; factory presets select their own engine");
}

static void sampler(float width,float height) {
    int c=instrument_channel;
    Sampler *settings=&project.sampler[c];
    label(fit_text(channel_caption(c),width-298,14),12,30,14,ink);
    channel_controls(c,width);
    label(fit_text(!originals[c].frames?"Drop a sample from the Browser":project.paths[c][0]?GetFileName(project.paths[c]):"Built-in sample",width-298,12),12,54,12,muted);
    float right=width/2+6,groupw=width/2-18;
    label("Sample effects",20,82,12,muted);
    sampler_switch(20,100,groupw-16,"Normalize",&settings->flags,SAMPLE_NORMALIZE);
    sampler_switch(20,128,groupw-16,"Reverse",&settings->flags,SAMPLE_REVERSE);
    sampler_switch(20,156,groupw-16,"Reverse polarity",&settings->flags,SAMPLE_POLARITY);
    if(hover(20,100,groupw-16,22)) snprintf(status,sizeof status,"Normalize: bring the processed sample peak to full scale");
    if(hover(20,128,groupw-16,22)) snprintf(status,sizeof status,"Reverse: play the cropped sample backwards");
    if(hover(20,156,groupw-16,22)) snprintf(status,sizeof status,"Polarity: flip the waveform vertically");
    label("Sample processing",right+8,82,12,muted);
    knob_style(right+28,108,&settings->pitch,-12,12,0,"Process pitch (semitones)",KNOB_CENTER);
    knob_style(right+84,108,&settings->time,.25f,4,1,"Time multiplier",KNOB_CENTER);
    label("Pitch shift",right+4,128,11,muted); label("Time",right+71,128,11,muted);
    if(button(settings->stretch?"Stretch":"Resample",right+116,98,groupw-128,22,0)) open_context(13,c,(Vector2){mouse.x+windows.editors[4].rect.x,mouse.y+windows.editors[4].rect.y});
    DrawTriangle((Vector2){right+groupw-23,107},(Vector2){right+groupw-19,112},(Vector2){right+groupw-15,107},muted);
    label("Mode",right+116,128,11,muted);
    if(sampler_toggle(right+8,146,groupw-16,"Fit to tempo",settings->fit_bpm>0)) {
        settings->fit_bpm=settings->fit_bpm?0:project.bpm;
    }
    if(hover(right+8,146,groupw-16,22)) snprintf(status,sizeof status,"Fit to tempo: keep audio fixed on the grid when BPM changes; uses Resample/Stretch mode");
    if(hover(right+116,98,groupw-128,22)) snprintf(status,sizeof status,"Time mode: Resample changes speed/pitch; Stretch preserves pitch; click to choose");
    label("Presets",20,190,11,muted);
    if(button("Load",20,210,(groupw-20)/2,24,0)) preset_action(PRESET_SAMPLER,c,0,0);
    if(button("Save",24+(groupw-20)/2,210,(groupw-20)/2,24,0)) preset_action(PRESET_SAMPLER,c,0,1);
    label("Sample region",right+8,182,12,muted);
    knob(right+28,212,&settings->start,0,1,0,"Start");
    knob(right+84,212,&settings->length,0,1,1,"Length");
    label("Start",right+14,230,11,muted); label("Length",right+67,230,11,muted);
    knob(right+156,212,&settings->trim,0,1,0,"Trim quiet beginning and end (threshold)");
    label("Trim",right+143,230,11,muted);
    int pending=!sampler_processing_equal(*settings,sampler_applied[c]);
    int live_preview=sampler_live_preview(c);
    float duration=live_preview?sampler_view_frames(c)/(float)RATE:samples[c].frames/(float)RATE;

    if(settings->fit_bpm) duration*=settings->fit_bpm/project.bpm;

    Sample sample=samples[c];
    if(live_preview) sample.frames=sampler_view_frames(c);
    float area=height-276;
    Rectangle display={12,252,width-24,area};
    DrawRectangleRec(display,bg);
    draw_waveform((WaveDisplay){.sample=sample,.wave=live_preview?NULL:processed_waveform(c),.channel=c,.preview=live_preview,
        .origin=12,.width=width-24},display,12,width-12,ui_theme.waveform);
    double progress=audio_key_position(127,c);
    if(hover(12,252,width-24,area)) {
        snprintf(status,sizeof status,"Sample waveform: click to %s; drop a Browser sample here to load it",progress>=0?"stop playback":"play from the beginning");
        if(samples[c].frames && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) { channel=c; audio_key(127,c,60,progress<0); }
    }
    if(progress>=0 && !live_preview) {
        float x=12+progress*(width-24);
        DrawLineEx((Vector2){x,252},(Vector2){x,252+area},1.5f,ink);
    }
    label("0 s",16,254,10,muted);
    label(TextFormat("%.2f s%s",duration,pending?" *":""),width-78,254,10,muted);
    if(live_preview && hover(12,252,width-24,area)) snprintf(status,sizeof status,"Live envelope preview; processed pitch/stretch detail appears when ready; audio changes after release");
    if(sample_drag[0] && sample_moved) DrawRectangleLines(12,252,width-24,area,ui_theme.signal);

}
static void draw_editor(int id,float scale) {
    Editor *e=&windows.editors[id]; if(!e->visible) return;
    knob_context=id;
    if(id==4) { e->minh=project.instrument[instrument_channel]==INSTRUMENT_FM?420:300; e->rect.h=fmaxf(e->minh,e->rect.h); }
    Rect r=e->rect; int top=EDITORS-1;
    while(top>0 && !windows.editors[windows.order[top]].visible) top--;
    int focused=(windows.editors[windows.focused].visible?windows.focused:windows.order[top])==id;
    BeginScissorMode((int)(r.x*scale),(int)(r.y*scale),(int)(r.w*scale),(int)(r.h*scale));
    text_origin=(Vector2){r.x*scale,r.y*scale};
    BeginMode2D((Camera2D){.offset=text_origin,.zoom=scale});
    ui_surface((Rectangle){0,0,r.w,r.h},id==0?ui_theme.rack:id==3?ui_theme.mixer:panel);
    ui_surface((Rectangle){0,0,r.w,TITLE},focused?ui_theme.title_focus:ui_theme.title);
    const char *titles[]={"Channel Rack","Arrangement","Piano Roll","Mixer","Sampler","Equalizer"};
    label(TextFormat("%s%s",id==4 && project.instrument[instrument_channel]==INSTRUMENT_FM?"FM Synth":titles[id],e->pinned?" (on top)":""),8,3,13,ink);
    int rack_title_knob=id==0 && ((mouse.x>=r.x+r.w-72-SWING_RADIUS-1 && mouse.x<r.x+r.w-72+SWING_RADIUS+1) || (mouse.x>=r.x+124 && mouse.x<r.x+180));
    if(input_enabled && !rack_title_knob && windows_hit(&windows,mouse.x,mouse.y)==id && mouse.y<r.y+TITLE && IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) open_context(1,id,mouse);
    int chrome=input_enabled && !rack_title_knob && windows_hit(&windows,mouse.x,mouse.y)==id && mouse.y<r.y+TITLE;
    if(chrome && mouse.x>=r.x+r.w-54) {
        int slot=fminf(2,(mouse.x-r.x-r.w+54)/18);
        DrawRectangle(r.w-54+slot*18,1,17,TITLE-2,ui_theme.hover);
        snprintf(status,sizeof status,"%s window",slot==0?"Minimize":slot==1?e->maximized?"Restore":"Maximize":"Hide");
    } else if(chrome) snprintf(status,sizeof status,"Drag title bar to move; double-click to maximize/restore; right-click for window options");
    DrawLineEx((Vector2){r.w-49,9},(Vector2){r.w-41,9},2,ink);
    int mx=r.w-27;
    if(e->maximized) { DrawRectangleLines(mx-2,4,7,7,ink); DrawRectangle(mx-4,6,7,7,cell); DrawRectangleLines(mx-4,6,7,7,ink); }
    else DrawRectangleLines(mx-4,5,8,8,ink);
    symbol("x",r.w-9,9,ink);
    input_enabled=input_enabled && windows.owner==id && windows.grab<0;
    Vector2 global=mouse; mouse.x-=r.x; mouse.y-=r.y;
    if(id==0) {
        button("",124,2,56,TITLE-4,0);
        label(rack_filters[rack_filter],124+(56-text_width(rack_filters[rack_filter],11))/2,3,11,ink);
        if(hover(124,2,56,TITLE-4)) snprintf(status,sizeof status,"Channel filter: All, Audio (Playlist drops), Unsorted (Rack samples)");
        knob_style(r.w-72,TITLE/2,&project.swing,0,1,0,"Swing",KNOB_SWING);
        if(hover(r.w-72-SWING_RADIUS-1,TITLE/2-SWING_RADIUS-1,2*SWING_RADIUS+2,2*SWING_RADIUS+2) || control_drag==&project.swing) snprintf(status,sizeof status,"Swing: %.0f%% | delays alternate steps; drag or wheel; right-click to enter",project.swing*100);
    }
    input_enabled=input_enabled && mouse.y>=TITLE && !(mouse.x>r.w-12 && mouse.y>r.h-12);
    if(midi_take.active && id<=2) input_enabled=0;
    if(id==0) rack(r.w,r.h); else if(id==1) playlist(r.w,r.h,scale); else if(id==2) piano(r.w,r.h); else if(id==3) mixer(r.w,r.h); else if(id==5) eq_editor(r.w,r.h); else if(project.instrument[instrument_channel]==INSTRUMENT_FM) fm_editor(r.w,r.h); else sampler(r.w,r.h);
    mouse=global;
    DrawLine(r.w-9,r.h-2,r.w-2,r.h-9,muted);
    EndMode2D(); EndScissorMode(); text_origin=(Vector2){0};
}
/* Apply navigation once to the frontmost editor, before drawing/editing. */
static void navigate_editors(float scale) {
    navigation_active=0;
    int blocked=pattern_popup || context_kind || sample_drag[0] || !IsWindowFocused();
    if(navigation_drag>=0 && (blocked || !IsMouseButtonDown(MOUSE_BUTTON_MIDDLE))) navigation_drag=-1;
    int id=navigation_drag>=0?navigation_drag:windows_hit(&windows,mouse.x,mouse.y);
    if(blocked || (id!=1 && id!=2) || (navigation_drag<0 && captured()) || windows.grab>=0) return;
    Rect r=windows.editors[id].rect;
    float gx=id==1?212:60,gy=id==1?PLAYLIST_GRID_TOP:PIANO_GRID_TOP,gridw=r.w-gx-24;
    float area=id==1?r.h-gy-PLAYLIST_BOTTOM:r.h-PIANO_GRID_PADDING;
    Vector2 local={mouse.x-r.x,mouse.y-r.y};
    if(navigation_drag<0 && (local.x<(id==1?120:4) || local.x>=r.w-24 || local.y<gy || local.y>=gy+area)) return;
    int middle=navigation_drag>=0;
    if(!middle && IsMouseButtonPressed(MOUSE_BUTTON_MIDDLE)) {
        windows_focus(&windows,id); navigation_drag=id; navigation_last=mouse; middle=1;
    }
    NavigationMotion motion={0};
    if(middle) {
        motion.x=navigation_last.x-mouse.x; motion.y=navigation_last.y-mouse.y;
        navigation_last=mouse; SetMouseCursor(MOUSE_CURSOR_RESIZE_ALL);
    } else motion=navigation_motion(navigation_input,IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT),shortcut_down(),scale);
    if(motion.x || motion.y || motion.zoom || middle) { browser_focus=0; navigation_active=1; }
    if(id==1) {
        float span=BARS/arrangement.zoom;
        arrangement.view_start=fmaxf(0,arrangement.view_start+motion.x*span/gridw);
        if(motion.zoom) arrangement_zoom(&arrangement,motion.zoom,fmaxf(0,fminf(1,(local.x-gx)/gridw)));
        track_scroll=fmaxf(0,fminf(fmaxf(0,track_position(LANES)-area),track_scroll+motion.y));
    } else {
        piano_pan[pattern]=fmaxf(0,piano_pan[pattern]+motion.x*piano_span[pattern]/gridw);
        if(motion.zoom && local.x>=gx) timeline_zoom(&piano_span[pattern],&piano_pan[pattern],motion.zoom,fmaxf(0,fminf(1,(local.x-gx)/gridw)),STEPS);
        piano_scroll_remainder+=motion.y/(area/piano_visible_rows());
        int rows=(int)piano_scroll_remainder;
        piano_top=fmaxf(piano_min_top(),fminf(127,piano_top-rows)); piano_scroll_remainder-=rows;
        if(piano_top==piano_min_top() || piano_top==127) piano_scroll_remainder=0;
    }
}

int main(int argc,char **argv) {
    int smoke=0;
    for(int i=1;i<argc;i++) if(!strcmp(argv[i],"--smoke")) smoke=1;
    resource_paths();
    if(smoke && (!DirectoryExists(samples_path) || !FileExists(font_path))) {
        fprintf(stderr,"Smoke check failed: bundled samples or font are missing.\n"); return 1;
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
    project_new(&project); project_document_saved(&document,&project,NULL); history_checkpoint();
    for(int c=0;c<CHANNELS;c++) sampler_applied[c]=project.sampler[c];
    unsigned flags=FLAG_WINDOW_RESIZABLE;
#ifdef __APPLE__
    flags|=FLAG_WINDOW_HIGHDPI;
#endif
    /* On Linux, raylib 5.5's high-DPI flag rescales mouse/scissors separately
       from our 2D cameras. Keep window coordinates consistent; rasterize fonts
       using actual framebuffer density rather than the monitor's DPI setting. */
    SetConfigFlags(flags); InitWindow(1200,675,"LibreLoop - pattern workstation");
    circles_init(); arcs_init(); cables_init();
    SetWindowMinSize(900,506); SetTargetFPS(60); SetExitKey(KEY_NULL);
    int audio_ok=audio_start(&project,samples);
    if(!audio_ok) snprintf(status,sizeof status,"Audio device unavailable. Editing and WAV export are available.");
    app_window=glfwGetCurrentContext();
    raylib_mouse_callback=glfwSetMouseButtonCallback(app_window,mouse_callback);
#ifdef __APPLE__
    macos_navigation_init(app_window);
#endif
    resize_cursor=glfwCreateStandardCursor(GLFW_HRESIZE_CURSOR);
    track_cursor=glfwCreateStandardCursor(GLFW_VRESIZE_CURSOR);
    hand_cursor=glfwCreateStandardCursor(GLFW_HAND_CURSOR);
    browser_init(&browser,samples_path); midi_initialize(); theme_init(browser.config); file_chooser_locations(browser.config); if(smoke) new_project(1); int frames=0,initialized=0; double last_activity=GetTime();
    while(!document.quit) {
        if(WindowShouldClose()) {
            glfwSetWindowShouldClose(app_window,GLFW_FALSE);
            if(!smoke) replace_project(REPLACE_QUIT); else break;
            if(document.quit) break;
        }
        if(sampler_job.busy && atomic_load(&sampler_job.done)) last_activity=GetTime();
        sampler_update();
        if(edit_clipboard.kind==COPY_CLIPS && (edit_clipboard.channels!=project.channel_count || edit_clipboard.patterns!=project.pattern_count || edit_clipboard.automations!=project.automation_count)) edit_clipboard.kind=COPY_EMPTY;
        Vector2 wheel=GetMouseWheelMoveV();
        navigation_input=(NavigationInput){wheel.x,wheel.y,0,0};
#ifdef __APPLE__
        NavigationInput native=macos_navigation_poll();
        if(native.precise) navigation_input=native;
        else navigation_input.zoom=native.zoom;
#endif
        Vector2 movement=GetMouseDelta();
        if(movement.x || movement.y || navigation_input.x || navigation_input.y || navigation_input.zoom || IsWindowResized()) last_activity=GetTime();
        for(int key=32;key<=KEY_KB_MENU;key++) if(IsKeyPressed(key) || IsKeyReleased(key) || IsKeyPressedRepeat(key)) last_activity=GetTime();
        for(int b=0;b<8;b++) if(pending_clicks[b] || IsMouseButtonReleased(b)) last_activity=GetTime();
        for(int b=0;b<8;b++) { frame_clicks[b]=pending_clicks[b]>0; if(frame_clicks[b]) pending_clicks[b]--; }
        float scale=ui_scale();
        int framebuffer_width,framebuffer_height,window_width,window_height;
        glfwGetFramebufferSize(app_window,&framebuffer_width,&framebuffer_height);
        glfwGetWindowSize(app_window,&window_width,&window_height);
        float raster_scale=scale*(window_width>0 && framebuffer_width>0?framebuffer_width/(float)window_width:1);
        (void)framebuffer_height; (void)window_height;
        fonts_update(raster_scale);
        float width=GetScreenWidth()/scale,height=GetScreenHeight()/scale;
        float sidebar=browser_hidden?24:browser_width;
        Rect desktop={sidebar+6,42,width-sidebar-10,height-66};
        if(!initialized) { windows_init(&windows,width,height); initialized=1; }
        mouse=GetMousePosition(); mouse.x/=scale; mouse.y/=scale; reset=0; popup_opened=0; context_opened=0;
        SetMouseCursor(MOUSE_CURSOR_DEFAULT);
        navigate_editors(scale);
        if(IsMouseButtonPressed(MOUSE_BUTTON_LEFT) || IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) arrangement_release(&arrangement);
        if(arrangement.gesture) {
            Rect r=windows.editors[1].rect;
            arrangement_drag(&arrangement,&project,arrangement.view_start+(mouse.x-r.x-212)/((r.w-236)/BARS*arrangement.zoom),track_at(track_scroll+mouse.y-r.y-PLAYLIST_GRID_TOP));
            int held=arrangement.gesture==ERASE_CLIPS?(IsMouseButtonDown(MOUSE_BUTTON_RIGHT) || IsMouseButtonDown(MOUSE_BUTTON_LEFT)):IsMouseButtonDown(MOUSE_BUTTON_LEFT);
            if(!held) arrangement_release(&arrangement);
        }
        if(fm_graph_drag>=0) {
            if(!IsMouseButtonDown(MOUSE_BUTTON_LEFT) || IsMouseButtonPressed(MOUSE_BUTTON_LEFT) || pattern_popup || context_kind || !IsWindowFocused() ||
               instrument_channel!=fm_graph_channel || project.instrument[fm_graph_channel]!=INSTRUMENT_FM || !windows.editors[4].visible) fm_graph_drag=-1;
            else fm_graph_update();
        }
        if(row_zoom_drag>=0) {
            if(!IsMouseButtonDown(MOUSE_BUTTON_LEFT) || IsMouseButtonPressed(MOUSE_BUTTON_LEFT) || pattern_popup || context_kind || !IsWindowFocused()) row_zoom_drag=-1;
            else { row_zoom_update(mouse.y); SetMouseCursor(MOUSE_CURSOR_RESIZE_NS); }
        }
        if(playlist_pan) {
            arrangement.view_start=fmaxf(0,playlist_pan_start+(mouse.x-playlist_pan_x)*playlist_pan_range);
            if(!IsMouseButtonDown(MOUSE_BUTTON_LEFT) || IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) playlist_pan=0;
        }
        if(playlist_vpan) {
            Rect r=windows.editors[1].rect; float area=r.h-PLAYLIST_GRID_TOP-PLAYLIST_BOTTOM+14;
            float total=track_position(LANES),travel=area-timeline_thumb(area,area-14,total),maximum=fmaxf(0,total-(area-14));
            track_scroll=fmaxf(0,fminf(maximum,playlist_vpan_start+(mouse.y-playlist_vpan_y)/fmaxf(1,travel)*maximum));
            if(!IsMouseButtonDown(MOUSE_BUTTON_LEFT) || IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) playlist_vpan=0;
        }
        if(track_resize>=0) {
            track_heights[track_resize]=roundf(fmaxf(32,fminf(320,track_resize_height+mouse.y-track_resize_y)))/track_zoom;
            SetMouseCursor(MOUSE_CURSOR_RESIZE_NS);
            if(!IsMouseButtonDown(MOUSE_BUTTON_LEFT)) track_resize=-1;
        }
        if(mixer_pan) {
            Rect r=windows.editors[3].rect; float area=r.w-MIXER_PANEL-MIXER_LEFT-8; int visible=fmaxf(1,area/51-1);
            mixer_scroll=fmaxf(0,fminf(INSERTS-visible,(mouse.x-r.x-MIXER_LEFT-area*visible/INSERTS/2)/area*INSERTS));
            if(!IsMouseButtonDown(MOUSE_BUTTON_LEFT) || IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) mixer_pan=0;
        }
        if(cable_drag>0) {
            if(IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) cable_drag=-1;
            else if(!IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
                int destination=mixer_input_at(mouse);
                if(destination>=0) snprintf(status,sizeof status,insert_connect(&project,cable_drag,destination)?"Mixer cable connected.":"Connection blocked: it would create a feedback loop.");
                cable_drag=-1;
            }
        }
        if(rack_paint>=0) {
            if(IsMouseButtonPressed(MOUSE_BUTTON_LEFT) || IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) rack_paint=-1;
            else {
                Rect r=windows.editors[0].rect; paint_rack(r.w,mouse.x-r.x,mouse.y-r.y);
                if(!IsMouseButtonDown(rack_paint)) rack_paint=-1;
            }
        }
        int modal=pattern_popup!=0 || context_kind!=0,dragging=sample_drag[0]!=0;
        if(!IsMouseButtonDown(MOUSE_BUTTON_LEFT) || IsMouseButtonPressed(MOUSE_BUTTON_LEFT) || modal) { route_drag=-1; control_drag=NULL; note_drag=NULL; }
        if(!modal && !captured() && shortcut_down() && IsKeyPressed(KEY_Z)) undo_redo(IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT)?1:-1);
#ifndef __APPLE__
        if(!modal && !captured() && shortcut_down() && IsKeyPressed(KEY_Y)) undo_redo(1);
#endif
        if(!modal && !captured() && !dragging && shortcut_down() && IsKeyPressed(KEY_S)) project_file_action(IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT)?3:2);
        if(!modal && !captured() && !dragging && shortcut_down() && IsKeyPressed(KEY_O)) replace_project(3);
        if(!modal && !captured() && !dragging && !browser_focus && shortcut_down()) {
            if(windows.focused==2 && piano_tool==PENCIL && (IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_K))) piano_octave(1);
            else if(windows.focused==2 && piano_tool==PENCIL && (IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_J))) piano_octave(-1);
            else if(IsKeyPressed(KEY_X)) edit_selection(0);
            else if(IsKeyPressed(KEY_C)) edit_selection(1);
            else if(IsKeyPressed(KEY_V)) edit_selection(2);
            else if(IsKeyPressed(KEY_A)) edit_selection(3);
        }
        if(!modal && !captured() && !dragging && !browser_focus && !shortcut_down() &&
           !IsKeyDown(KEY_LEFT_ALT) && !IsKeyDown(KEY_RIGHT_ALT) &&
           (IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT))) {
            if(IsKeyPressed(KEY_LEFT) || IsKeyPressedRepeat(KEY_LEFT)) piano_shift(-1,0,"left one step");
            else if(IsKeyPressed(KEY_RIGHT) || IsKeyPressedRepeat(KEY_RIGHT)) piano_shift(1,0,"right one step");
            else if(IsKeyPressed(KEY_UP) || IsKeyPressedRepeat(KEY_UP)) piano_shift(0,1,"up one semitone");
            else if(IsKeyPressed(KEY_DOWN) || IsKeyPressedRepeat(KEY_DOWN)) piano_shift(0,-1,"down one semitone");
        }
        if(!modal && IsKeyPressed(KEY_SPACE)) transport_toggle();
        if(rack_hdrag) {
            rack_view[pattern]=fmaxf(0,rack_hstart+(mouse.x-rack_hx)*rack_hscale);
            if(!IsMouseButtonDown(MOUSE_BUTTON_LEFT) || IsMouseButtonPressed(MOUSE_BUTTON_LEFT) || modal) rack_hdrag=0;
        }
        if(rack_vdrag) {
            rack_scroll=fmaxf(0,rack_vstart+(mouse.y-rack_vy)*rack_vscale);
            if(!IsMouseButtonDown(MOUSE_BUTTON_LEFT) || IsMouseButtonPressed(MOUSE_BUTTON_LEFT) || modal) rack_vdrag=0;
        }
        if(piano_key_drag) {
            Rect r=windows.editors[2].rect;
            piano_key_update(mouse.x-r.x,mouse.y-r.y,modal || !IsMouseButtonDown(MOUSE_BUTTON_LEFT) || IsMouseButtonPressed(MOUSE_BUTTON_LEFT) || !IsWindowFocused() || !windows.editors[2].visible);
        }
        if(marker_drag>=0) {
            int id=marker_drag; Rect r=windows.editors[id].rect;
            float gx=id==0?200:id==1?212:60,width=r.w-(id==0?226:id==1?236:84);
            float span=id==0?width/RACK_STEP_WIDTH:id==1?BARS/arrangement.zoom*STEPS:piano_span[pattern];
            float start=id==0?rack_view[pattern]:id==1?arrangement.view_start*STEPS:piano_pan[pattern];
            float q=id==0?1:grid_interval(width/span);
            int button=ruler_loop_drag?MOUSE_BUTTON_RIGHT:MOUSE_BUTTON_LEFT;
            if(modal || IsMouseButtonPressed(button)) marker_drag=-1;
            else {
                float at=fmaxf(0,snap_round(start+(mouse.x-r.x-gx)/width*span,q));
                if(ruler_loop_drag) {
                    float *range=id==1?song_loop:pattern_loop[pattern];
                    range[0]=fminf(ruler_anchor,at); range[1]=fmaxf(ruler_anchor,at);
                    if(range[1]>range[0]) { if(id==1) playlist_start=range[0]/STEPS; else piano_start[pattern]=range[0]; }
                } else if(id==1) playlist_start=at/STEPS; else piano_start[pattern]=at;
                if(!playing) reset=1;
                if(!IsMouseButtonDown(button)) marker_drag=-1;
            }
        }
        if(!modal && !dragging && !captured() && IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && mouse.x>=sidebar && mouse.x<sidebar+6 && mouse.y>=42 && mouse.y<height-22) browser_resize=1;
        if(browser_resize) {
            browser_hidden=mouse.x<120;
            if(!browser_hidden) browser_width=fminf(fminf(600,width-460),mouse.x);
            sidebar=browser_hidden?24:browser_width;
            desktop=(Rect){sidebar+6,42,width-sidebar-10,height-66};
            if(!IsMouseButtonDown(MOUSE_BUTTON_LEFT)) browser_resize=0;
        }
        if(sidebar!=last_sidebar) {
            Editor *list=&windows.editors[1];
            if(fabsf(list->rect.x-last_sidebar-6)<1 && fabsf(list->rect.w-(width-last_sidebar-10))<1) list->rect=desktop;
            last_sidebar=sidebar;
        }
        if(dragging) {
            if(fabsf(mouse.x-sample_origin.x)+fabsf(mouse.y-sample_origin.y)>5) sample_moved=1;
            if(IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) sample_drag[0]=0;
            else if(IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
                if(sample_moved && !modal) drop_sample(sample_drag);
                sample_drag[0]=0;
            } else if(!IsMouseButtonDown(MOUSE_BUTTON_LEFT) || IsKeyPressed(KEY_ESCAPE)) sample_drag[0]=0;
        }
        dragging=sample_drag[0]!=0;
        if(IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) windows.grab=-1;
        if(!modal && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) browser_focus=mouse.x<sidebar && mouse.y>=42 && mouse.y<height-22;
        if(control_drag) {
            float value=control_fader?fader_gain(fmaxf(0,fminf(1,(control_bottom-mouse.y)/control_range))):*control_drag-GetMouseDelta().y/scale*(control_reverse?-1:1)*(control_drag==&project.bpm?.25f:(control_high-control_low)/160);
            if(control_logarithmic) value=*control_drag*powf(control_high/control_low,-GetMouseDelta().y/scale/160);
            if(control_integer) { control_raw=fmaxf(control_low,fminf(control_high,control_raw-GetMouseDelta().y/scale*(control_high-control_low)/160)); value=roundf(control_raw); }
            *control_drag=fmaxf(control_low,fminf(control_high,value));
        }
        if(piano_scroll_drag) {
            piano_pan[pattern]=fmaxf(0,piano_scroll_start+(mouse.x-piano_scroll_x)*piano_scroll_range);
            if(!IsMouseButtonDown(MOUSE_BUTTON_LEFT) || IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) piano_scroll_drag=0;
        }
        if(piano_vdrag) {
            Rect r=windows.editors[2].rect; float area=r.h-PIANO_GRID_PADDING+14;
            piano_top=127-(int)fmaxf(0,fminf(127-piano_min_top(),piano_vscroll_start+(mouse.y-piano_vscroll_y)/area*128));
            if(!IsMouseButtonDown(MOUSE_BUTTON_LEFT) || IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) piano_vdrag=0;
        }
        if(piano_gesture) {
            int right=piano_gesture==PIANO_ERASE;
            if(modal || IsMouseButtonPressed(MOUSE_BUTTON_LEFT) || IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) piano_gesture=PIANO_IDLE;
            else {
                Rect r=windows.editors[2].rect; float cw=(r.w-84)/piano_span[pattern],rh=(r.h-PIANO_GRID_PADDING)/piano_visible_rows();
                piano_gesture_update(piano_pan[pattern]+(mouse.x-r.x-60)/cw,piano_top-(mouse.y-r.y-PIANO_GRID_TOP)/rh,grid_interval(cw));
                if(!IsMouseButtonDown(right?MOUSE_BUTTON_RIGHT:MOUSE_BUTTON_LEFT)) piano_gesture=PIANO_IDLE;
            }
        }
        if(note_drag) {
            Rect r=windows.editors[2].rect; float cw=(r.w-84)/piano_span[pattern],rh=(r.h-PIANO_GRID_PADDING)/piano_visible_rows();
            piano_note_drag_update(piano_pan[pattern]+(mouse.x-r.x-60)/cw,(mouse.y-r.y-PIANO_GRID_TOP)/rh,grid_interval(cw));
        }
        if(velocity_drag>=0) {
            int button=velocity_drag?MOUSE_BUTTON_RIGHT:MOUSE_BUTTON_LEFT;
            if(modal || pattern!=velocity_pattern || piano_channel!=velocity_channel ||
               IsMouseButtonPressed(MOUSE_BUTTON_LEFT) || IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) velocity_drag=-1;
            else {
                Rect r=windows.editors[2].rect; float cw=(r.w-84)/piano_span[pattern];
                Vector2 point={piano_pan[pattern]+(mouse.x-r.x-60)/cw,
                    fmaxf(1,fminf(127,(r.y+r.h-22-mouse.y)/.48f))};
                Vector2 from=velocity_drag?velocity_from:velocity_now;
                notes_velocity(project.notes[pattern][piano_channel],velocity_drag?velocity_before:NULL,
                    from.x,point.x,from.y,point.y,3/cw);
                velocity_now=point;
                if(!IsMouseButtonDown(button)) velocity_drag=-1;
            }
        }
        if(route_drag>=0) {
            project.route[route_drag]=(int)fmaxf(0,fminf(project.insert_count,route_start+(int)((route_y-mouse.y)/3)));
            mixer_selected=project.route[route_drag];
            int visible=fmaxf(1,(windows.editors[3].rect.w-MIXER_PANEL-MIXER_LEFT-8)/51-1);
            if(mixer_selected>mixer_scroll+visible) mixer_scroll=mixer_selected-visible;
            else if(mixer_selected>0 && mixer_selected<=mixer_scroll) mixer_scroll=mixer_selected-1;
            snprintf(status,sizeof status,"%s -> %s",project.channel_names[route_drag],mixer_name(mixer_selected));
        }
        Rect filter_rect=windows.editors[0].rect;
        if(!modal && !captured() && !dragging && windows_hit(&windows,mouse.x,mouse.y)==0 && mouse.x>=filter_rect.x+124 && mouse.x<filter_rect.x+180 && mouse.y>=filter_rect.y+2 && mouse.y<filter_rect.y+TITLE-2 && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            windows_focus(&windows,0); open_context(6,0,(Vector2){filter_rect.x+124,filter_rect.y+TITLE}); modal=1;
        }
        if(!modal && !captured() && !dragging) windows_update(&windows,desktop,mouse.x,mouse.y,IsMouseButtonPressed(MOUSE_BUTTON_LEFT),IsMouseButtonDown(MOUSE_BUTTON_LEFT),GetTime());
        else { if(browser_resize) windows_update(&windows,desktop,mouse.x,mouse.y,0,0,GetTime()); windows.owner=-1; windows.grab=-1; }
        int focused=EDITORS-1;
        while(focused>0 && !windows.editors[windows.order[focused]].visible) focused--;
        if(!modal && !dragging && !captured() && !browser_focus && !midi_take.active && windows.order[focused]==1 && windows.editors[1].visible && delete_pressed()) {
            for(int l=0;l<LANES;l++) for(int b=0;b<CLIPS;b++) if(arrangement.selected[l][b]) { project.clips[l][b]=0; project.clip_steps[l][b]=0; }
            memset(arrangement.selected,0,sizeof arrangement.selected);
        }
        if(!modal && !dragging && !captured() && !browser_focus && !midi_take.active && windows.order[focused]==2 && windows.editors[2].visible && delete_pressed()) {
            for(int i=0;i<NOTES;i++) if(note_selected[pattern][piano_channel][i]) project.notes[pattern][piano_channel][i].velocity=0;
            memset(note_selected[pattern][piano_channel],0,NOTES);
        }
        if(!modal && IsFileDropped()) {
            FilePathList dropped=LoadDroppedFiles();
            for(unsigned i=0;i<dropped.count;i++) {
                const char *path=dropped.paths[i];
                if(IsFileExtension(path,".hbt")) request_load_project(path);
                else drop_sample(path);
            }
            UnloadDroppedFiles(dropped);
        }
        meters_update();
        spectrum_update();
        track_activity_update();
        if(windows.editors[1].visible) previews_update(raster_scale);
        knob_context=-1; browser_progress=-1;
        BeginDrawing(); ClearBackground(bg); BeginMode2D((Camera2D){.zoom=scale});
        input_enabled=!modal && !dragging && !captured() && windows.grab<0 && mouse.y<42;
        ui_surface((Rectangle){0,0,width,40},panel);
        ui_surface((Rectangle){8,8,196,22},cell);
        if(button("FILE",8,8,52,22,pattern_popup==1)) open_popup(1);
        if(hover(8,8,48,22)) snprintf(status,sizeof status,"File: New, Demo, Save, Open, Export or Collect samples");
        if(button("VIEW",60,8,56,22,pattern_popup==7)) open_popup(7);
        if(hover(60,8,52,22)) snprintf(status,sizeof status,"View: Dark / Light themes and accent color");
        if(button("HELP",116,8,52,22,pattern_popup==9)) open_popup(9);
        if(button("EDIT",168,8,36,22,pattern_popup==10)) open_popup(10);
        if(hover(168,8,36,22)) snprintf(status,sizeof status,"Edit: Undo, Redo, Cut, Copy, Paste and Select all");
        label(fit_text(TextFormat("%s%s",GetFileName(document.path),project_dirty()?" *":""),196,11),8,29,11,muted);
        const int gap=4,pitch_x=204+gap+KNOB_RADIUS,volume_x=pitch_x+KNOB_RADIUS*2+gap;
        const int mode_x=volume_x+KNOB_RADIUS+gap,play_x=mode_x+52+gap,stop_x=play_x+24+gap,record_x=stop_x+24+gap;
        const int metro_x=record_x+24+3*22+gap,tempo_x=metro_x+22+gap,position_x=tempo_x+56+gap,editors_x=position_x+88+gap;
        const int follow_x=editors_x+4*(22+gap),keys_x=follow_x+22+gap,audio_x=keys_x+22+gap;
        knob_style(pitch_x,19,&project.master_pitch,-12,12,0,"Master pitch (semitones)",KNOB_CENTER);
        if(hover(pitch_x-11,8,22,22) || control_drag==&project.master_pitch) snprintf(status,sizeof status,"Master pitch: %+.2f semitones (sample speed; right-click to enter)",project.master_pitch);
        knob_style(volume_x,19,&output_volume,0,VOLUME_KNOB_MAX,1,"LibreLoop output volume",KNOB_VOLUME);
        if(hover(volume_x-11,8,22,22) || control_drag==&output_volume) snprintf(status,sizeof status,"LibreLoop output volume: listening level; Master and WAV export unchanged");
        if(button("",mode_x,8,52,22,0) && !recording_active()) { song=!song; reset=1; }
        DrawRectangle(mode_x,8,52,11,song?cell:ui_theme.patt);
        DrawRectangle(mode_x,19,52,11,song?accent:cell);
        label("PATT",mode_x+(52-text_width("PATT",10))/2,8,10,song?muted:ui_theme.selected_text);
        label("SONG",mode_x+(52-text_width("SONG",10))/2,19,10,song?ui_theme.selected_text:muted);
        if(hover(mode_x,8,52,22)) snprintf(status,sizeof status,"Playback mode: click to switch Pattern (orange) / Song (green)");
        int transport_active=playing || audio_preview_position(audition)>=0 || audio_key_position(127,instrument_channel)>=0;
        if(button(transport_active?"||":">",play_x,8,24,22,transport_active)) transport_toggle();
        if(hover(play_x,8,24,22)) snprintf(status,sizeof status,"Play from ruler / stop all audio, including previews (Space)");
        if(button("[]",stop_x,8,24,22,0)) transport_stop();
        if(hover(stop_x,8,24,22)) snprintf(status,sizeof status,"Stop all audio; press again while stopped to return the start marker to the beginning");
        if(button("",record_x,8,24,22,recording_active())) recording_start();
        circle(record_x+12,19,7,ui_theme.meter_high);
        if(hover(record_x,8,24,22) && IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)){midi_refresh();open_popup(13);}
        const int record_icons[]={ICON_WAVE,ICON_PIANO,ICON_AUTOMATION};
        const char *record_names[]={"Audio from armed mixer tracks","MIDI notes into a new pattern","MIDI / mouse control automation"};
        int record_input=input_enabled;if(recording_active())input_enabled=0;
        for(int i=0;i<3;i++){int xx=record_x+24+i*22;
            if(button("",xx,8,22,22,record_mask&(1u<<i))){record_mask^=1u<<i;midi_save_settings();}
            icon(record_icons[i],xx+11,19,17,record_mask&(1u<<i)?ui_theme.selected_text:muted);
            if(hover(xx,8,22,22))snprintf(status,sizeof status,"Record %s: %s",record_names[i],record_mask&(1u<<i)?"enabled":"disabled");
        }input_enabled=record_input;
        if(hover(record_x,8,24,22)) snprintf(status,sizeof status,recording_active()?"Recording: click to finish the takes":"Record enabled types; right-click for MIDI inputs and recording settings");
        if(button("",metro_x,8,22,22,metronome)) { metronome=!metronome; audio_metronome(metronome); }
        icon(ICON_METRO,metro_x+11,19,20,metronome?ui_theme.selected_text:ink);
        if(hover(metro_x,8,22,22)) snprintf(status,sizeof status,"Metronome: %s | beat clicks during playback; first beat accented; excluded from WAV export",metronome?"On":"Off");
        if(drag_button(TextFormat("%.2f",project.bpm),tempo_x,8,56,22,control_drag==&project.bpm) && !recording_active()) capture_control(&project.bpm,30,300,0);
        if(hover(tempo_x,8,56,22) && !recording_active()) {
            project.bpm=fmaxf(30,fminf(300,project.bpm+GetMouseWheelMove()));
            snprintf(status,sizeof status,"Tempo: drag up/down; right-click for Reset or Enter value");
            if(IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) control_menu(&project.bpm,30,300,120,"Tempo (BPM)");
        }
        uint64_t step=(uint64_t)(audio_position()/(RATE*60.0/project.bpm/4));
        ui_surface((Rectangle){position_x,8,88,22},bg);
        label(TextFormat("%03llu:%02llu",(unsigned long long)(step/16+1),(unsigned long long)(step%16+1)),position_x+10,10,18,ui_theme.signal);
        if(hover(position_x,8,88,22)) snprintf(status,sizeof status,"Playback position: bar and sixteenth-note step");
        for(int i=0;i<4;i++) { int id=i==1?2:i==2?1:i; editor_button(id,editors_x+i*(22+gap)); }
        if(button("",follow_x,8,22,22,follow_playhead)) { follow_playhead=!follow_playhead; if(!follow_playhead) { arrangement.view_start=fmaxf(0,arrangement.view_start); for(int p=0;p<PATTERNS;p++) piano_pan[p]=fmaxf(0,piano_pan[p]); } }
        icon(ICON_FOLLOW,follow_x+11,19,20,follow_playhead?ui_theme.selected_text:ink);
        if(hover(follow_x,8,22,22)) snprintf(status,sizeof status,"Follow playhead: center playback in the Playlist (Song) or Piano Roll (PAT)");
        if(button("",keys_x,8,22,22,typing_keys)) typing_keys=!typing_keys;
        icon(ICON_KEYS,keys_x+11,19,20,typing_keys?ui_theme.selected_text:ink);
        if(hover(keys_x,8,22,22)) snprintf(status,sizeof status,"Typing piano: Z row C3, Q row C4; S/D/G/H/J and 2/3/5/6/7 sharps; disabled in Browser/text fields");
        recording_poll();midi_poll();
        visual_step=playing && !reset?audio_visual_position()/(RATE*60.0/project.bpm/4):playback_start(); automation_display_update();
        int browser_hover=!browser_hidden && mouse.x<sidebar && mouse.y>=42 && mouse.y<height-22;
        typing_piano(modal || dragging || browser_hover);
        float analyzer_width=fminf(108,fmaxf(0,width-audio_x-72));
        if(analyzer_width>=28) {
            DrawRectangle(audio_x,8,analyzer_width,22,bg);
            spectrum_draw(&master_spectrum,(Rectangle){audio_x+2,9,analyzer_width-4,20},0);
            int mx=audio_x+analyzer_width+gap; DrawRectangle(mx,8,60,22,bg);
            for(int side=0;side<2;side++) {
                float level=fmaxf(0,fminf(1,(20*log10f(fmaxf(.001f,meter_level[0][side]))+60)/60));
                DrawRectangle(mx+2,10+side*10,56*level,7,accent);
                if(meter_hold[0][side]>=1) DrawRectangle(mx+56,10+side*10,2,7,ui_theme.meter_high);
            }
            if(hover(audio_x,8,analyzer_width+gap+60,22)) snprintf(status,sizeof status,audio_ok?"Master frequency spectrum and stereo level · red indicates clipping":"Audio device unavailable");
        }
        input_enabled=!modal && !pattern_popup && !context_kind && !dragging && !captured() && windows.grab<0 && mouse.x<sidebar+6 && mouse.y>=42;
        DrawRectangle(0,42,sidebar,height-66,ui_theme.browser);
        if(!browser_hidden) {
            if(hover(0,42,sidebar,height-66)) snprintf(status,sizeof status,"Browser: click to preview; drag to load | Up/Down or j/k: select | Left/Right or h/l: fold");
            label("Browser",10,50,14,ink);
            if(button("+ Add folder",8,76,sidebar-16,23,0)) { snprintf(folder_text,sizeof folder_text,"%s",browser.path); rename_select_all=1; open_popup(3); }
            int fy=108,rows=fmaxf(1,(height-fy-98)/23);
            if(!modal && !pattern_popup && !context_kind && (browser_focus || browser_hover) && !dragging && !captured() && !command_down()) {
                if(IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_K)) audition_entry((int)fmaxf(0,browser.selected-1));
                if(IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_J)) audition_entry((int)fminf(browser.items-1,browser.selected+1));
                if(IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_H)) browser_left(&browser);
                if(IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_L)) browser_right(&browser);
            }
            if(browser.follow && browser.selected>=0) {
                browser.follow=0;
                if(browser.selected<browser.scroll) browser.scroll=browser.selected;
                if(browser.selected>=browser.scroll+rows) browser.scroll=browser.selected-rows+1;
            }
            if(hover(0,fy,sidebar,rows*23)) browser.scroll-=(int)GetMouseWheelMove()*3;
            browser.scroll=(int)fmaxf(0,fminf(fmaxf(0,browser.items-rows),browser.scroll));
            BeginScissorMode(0,fy*scale,(sidebar-4)*scale,rows*23*scale);
            for(int entry=browser.scroll;entry<browser.items && entry<browser.scroll+rows;entry++) {
                BrowserNode *node=&browser.nodes[entry]; const char *path=node->path;
                int y=fy+(entry-browser.scroll)*23,indent=12+node->depth*14;
                int clicked=button_color("",8,y,sidebar-16,21,browser.selected==entry,ui_theme.browser);
                Color color=browser.selected==entry?ui_theme.selected_text:ink;
                if(node->dir) {
                    if(node->open) DrawTriangle((Vector2){indent,y+8},(Vector2){indent+4,y+13},(Vector2){indent+8,y+8},color);
                    else DrawTriangle((Vector2){indent,y+6},(Vector2){indent,y+14},(Vector2){indent+6,y+10},color);
                }
                const char *name=GetFileName(path); if(!*name) name=path;
                label(fit_text(node->depth==0 && !strcmp(path,browser.samples)?"LibreLoop samples":name,fmaxf(0,sidebar-indent-24),13),indent+14,y+4,13,color);
                if(hover(8,y,sidebar-16,21)) snprintf(status,sizeof status,"%.80s: %s | j/k: select; h/l: fold",path,node->dir?"click to expand/collapse":IsFileExtension(path,".hbt")?"click to open project":"click to preview; drag to Rack or Playlist");
                if(clicked) {
                    browser_select(&browser,entry);
                    if(node->dir) { if(!browser_toggle(&browser,entry)) snprintf(status,sizeof status,"Folder unavailable: %.180s",path); break; }
                    if(IsFileExtension(path,".hbt")) request_load_project(path);
                    else {
                        audition_entry(entry); snprintf(sample_drag,sizeof sample_drag,"%s",path);
                        sample_origin=mouse; sample_moved=0;
                        sample_offset=(Vector2){mouse.x-8,mouse.y-y}; sample_drag_width=sidebar-16; sample_text_offset=indent+6;
                    }
                }
            }
            EndScissorMode();
            if(browser.selected>=0 && browser.selected<browser.items && audition.data && !strcmp(browser.nodes[browser.selected].path,audition_path)) {
                draw_waveform((WaveDisplay){.sample=audition,.wave=&audition_wave,.origin=8,.width=sidebar-16},
                    (Rectangle){8,height-98,sidebar-16,68},8,sidebar-8,ui_theme.waveform);
                if(hover(8,height-98,sidebar-16,68)) {
                    snprintf(status,sizeof status,"Click waveform to replay %.100s | Up/Down or j/k: select; Left/Right or h/l: fold",GetFileName(audition_path));
                    if(IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) { browser_focus=1; audio_preview(audition); }
                }
                browser_progress=audio_preview_position(audition);
                if(browser_progress>=0) {
                    float x=8+browser_progress*(sidebar-16);
                    DrawLineEx((Vector2){x,height-98},(Vector2){x,height-30},1.5f,ink);
                }
            }
        }
        int resize_hover=hover(sidebar,42,6,height-66);
        DrawRectangle(sidebar,42,6,height-66,browser_resize || resize_hover?accent:cell);
        if(resize_hover || browser_resize) { SetMouseCursor(MOUSE_CURSOR_RESIZE_EW); snprintf(status,sizeof status,"Drag to resize Browser; below minimum width collapses it; drag outward to restore"); }
        EndMode2D();
        /* Freeze draw order: opening Piano Roll while drawing must not draw a window twice. */
        int order[EDITORS]; memcpy(order,windows.order,sizeof order);
        for(int i=0;i<EDITORS;i++) { input_enabled=!modal && !pattern_popup && !context_kind && !dragging && !sample_drag[0] && !captured(); draw_editor(order[i],scale); }

        BeginMode2D((Camera2D){.zoom=scale}); DrawRectangle(0,height-22,width,22,panel); label(fit_text(status,width-16,11),8,height-17,11,ink);
        draw_browser_drag();
        draw_picker_drag();
        draw_popup(); draw_context(); EndMode2D();

        if(!captured() && !recording_active()) history_checkpoint();
        if(pattern_popup!=11 && file_picker.entries) file_chooser_close(&file_picker);
        sampler_queue();
        if(recording_active()) { project.bpm=recording_ui.active?recording_ui.bpm:midi_take.bpm; song=playing=1; reset=0; }
        flush_cursor(); audio_update(&project,playing,song,pattern,reset,output_volume,playback_start(),recording_active()?0:playback_loop()[0],recording_active()?0:playback_loop()[1]);
        int animate=audio_active() || mixer_decay_active || spectrum_active || (windows.editors[0].visible && channel_decay_active) || browser_progress>=0 || (windows.editors[3].visible && (audio_active() || mixer_decay_active)) || playing || captured() || windows.grab>=0 || popup_drag || sample_drag[0] ||
            (windows.editors[4].visible && audio_key_position(127,instrument_channel)>=0);
        if(!smoke && !animate && GetTime()-last_activity>.15) EnableEventWaiting(); else DisableEventWaiting();
        EndDrawing();
        if(smoke && ++frames%10==0) {
            int stage=frames/10;
            TakeScreenshot(stage==1?"libreloop-smoke.png":TextFormat("libreloop-view-%d.png",stage-1));
            if(stage==4) break;
            if(stage==1) windows_focus(&windows,1);
            if(stage==2) windows_focus(&windows,2);
            if(stage==3) windows_focus(&windows,3);
        }
    }
    recording_finish();midi_panic();midi_input_wake(NULL);midi_input_close(); history_clear(&edit_history); file_chooser_close(&file_picker);
    if(sampler_job.busy) { pthread_join(sampler_job.thread,NULL); sample_free(sampler_job.input); sample_free(sampler_job.result); }
    for(int pat=0;pat<PATTERNS;pat++) if(previews[pat].image.id) UnloadRenderTexture(previews[pat].image);
    #ifdef __APPLE__
    macos_navigation_close();
#endif
    text_fonts_close(); UnloadTexture(circle_texture); UnloadTexture(icons); UnloadTexture(knob_arcs); UnloadTexture(cable_texture);
    browser_close(&browser); audio_close(); sample_free(audition); free(audition_wave.tree); if(resize_cursor) glfwDestroyCursor(resize_cursor); if(track_cursor) glfwDestroyCursor(track_cursor); if(hand_cursor) glfwDestroyCursor(hand_cursor); CloseWindow();
    for(int c=0;c<CHANNELS;c++) { free(audio_waves[c].tree); free(sampler_views[c].wave.tree); sample_free(samples[c]); sample_free(originals[c]); }
    return 0;
}
