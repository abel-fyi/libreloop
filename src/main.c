// SPDX-License-Identifier: GPL-3.0-only
#include "raylib.h"
#include "rlgl.h"
#include "audio.h"
#include "windows.h"
#include "browser.h"
#include "theme.h"
#include "arrangement.h"
#include "waveform.h"
#include "navigation.h"
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
    snprintf(samples_path,sizeof samples_path,"%s",LIBRELOOP_SAMPLES);
    snprintf(font_path,sizeof font_path,"%s",LIBRELOOP_FONT);
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
static GLFWcursor *resize_cursor,*track_cursor;
static GLFWwindow *app_window;
static void request_cursor(int cursor) { requested_cursor=cursor; }
static void flush_cursor(void) {
    if(requested_cursor==current_cursor) return;
    glfwSetCursor(app_window,requested_cursor==MOUSE_CURSOR_RESIZE_EW?resize_cursor:requested_cursor==MOUSE_CURSOR_RESIZE_NS?track_cursor:NULL);
    current_cursor=requested_cursor;
}
#define SetMouseCursor request_cursor

static Project project;
static Sample samples[CHANNELS],originals[CHANNELS];
static Sampler sampler_applied[CHANNELS];
static unsigned sample_generation[CHANNELS],sampler_generation[CHANNELS],sample_epoch;
static struct {
    pthread_t thread; atomic_int done; int busy,channel,ok;
    unsigned generation,epoch; Sampler settings; Sample input,result;
} sampler_job;
static Sample sample_copy(Sample source) {
    Sample copy={0};
    if(source.frames) { copy.data=malloc((size_t)source.frames*sample_channels(source)*sizeof(float)); if(copy.data) { memcpy(copy.data,source.data,(size_t)source.frames*sample_channels(source)*sizeof(float)); copy.frames=source.frames; copy.channels=source.channels; } }
    return copy;
}
static void *sampler_worker(void *unused) {
    (void)unused;
    sampler_job.ok=sample_process(sampler_job.input,sampler_job.settings,&sampler_job.result);
    atomic_store(&sampler_job.done,1); glfwPostEmptyEvent(); return NULL;
}
static void sampler_update(void) {
    if(sampler_job.busy && atomic_load(&sampler_job.done)) {
        pthread_join(sampler_job.thread,NULL); int c=sampler_job.channel;
        if(sampler_job.epoch==sample_epoch && c<project.channel_count && sampler_job.generation==sample_generation[c] && sampler_equal(sampler_job.settings,project.sampler[c])) {
            if(sampler_job.ok) {
                Sample old=samples[c]; audio_sample(c,sampler_job.result); samples[c]=sampler_job.result; sampler_job.result=(Sample){0}; free(old.data);
            } else project.sampler[c]=sampler_applied[c];
            project.audio_seconds[c]=samples[c].frames/(float)RATE;
            sampler_applied[c]=project.sampler[c]; sampler_generation[c]=sample_generation[c];
        }
        free(sampler_job.input.data); free(sampler_job.result.data); sampler_job.busy=0;
    }
}

static int sampler_flush(void) {
    if(sampler_job.busy) {
        pthread_join(sampler_job.thread,NULL); free(sampler_job.input.data); free(sampler_job.result.data); sampler_job.busy=0;
    }
    for(int c=0;c<project.channel_count;c++) if(sampler_generation[c]!=sample_generation[c] || !sampler_equal(sampler_applied[c],project.sampler[c])) {
        Sample next; if(!sample_process(originals[c],project.sampler[c],&next)) return 0;
        Sample old=samples[c]; audio_sample(c,next); samples[c]=next; free(old.data);
        project.audio_seconds[c]=samples[c].frames/(float)RATE;
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
static Vector2 picker_origin,picker_offset;
static Arrangement arrangement={.source_steps=STEPS,.source_pattern=-1,.zoom=1};
static float rack_view[PATTERNS],rack_range[PATTERNS];
static int rack_hdrag; static float rack_hx,rack_hstart,rack_hscale;
#define RACK_STEP_WIDTH 16
#define RACK_TOP (TITLE+14)
#define MIXER_LEFT 62
static int playlist_pan,playlist_vpan;
static float track_scroll,playlist_vpan_y,playlist_vpan_start;
static float track_heights[LANES],track_resize_y,track_resize_height;
static int track_resize=-1;
static float track_height(int lane) { return track_heights[lane]>0?track_heights[lane]:52; }
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
static int marker_drag=-1,ruler_loop_drag;
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
static int rack_scroll,context_kind,context_target,context_opened;
static Vector2 context_position;
static int pattern_popup,popup_opened,rename_pattern,rename_select_all,popup_drag;
static Vector2 popup_position[10],popup_offset;
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
static float audition_low[128],audition_high[128];
static int browser_focus=1,sample_moved,browser_hidden,browser_resize;
static float browser_width=174,last_sidebar=174;
static char sample_drag[PATH_MAX];
static Vector2 sample_origin,sample_offset;
static float sample_drag_width,sample_text_offset;
static float *control_drag,control_low,control_high,control_bottom,control_range;
static int control_fader,control_reverse,control_integer,number_integer;
static float control_raw;
static Note *note_drag;
static int moving_note,piano_scroll_drag,piano_vdrag;
static int piano_top=72;
static int piano_key_drag,piano_key=-1,piano_key_channel;
static float piano_scroll_x,piano_scroll_start,piano_scroll_range,piano_vscroll_y,piano_vscroll_start;
static Note note_before[NOTES];
static uint8_t note_selected[PATTERNS][CHANNELS][NOTES],note_selection_before[NOTES];
static uint8_t keyboard_notes[CHANNELS][128];
static int piano_tool,piano_gesture,piano_additive;
enum { PIANO_IDLE, PIANO_BRUSH, PIANO_ERASE, PIANO_BOX };
static Vector2 piano_from,piano_now;
static Vector2 note_grab;
static float piano_span[PATTERNS]={16,16,16,16,16,16,16,16},piano_pan[PATTERNS],piano_range[PATTERNS];
static float velocity_drag=-1;
static int snap_mode[2]={SNAP_STEP,SNAP_STEP},snap_menu,snap_opened;
static Vector2 snap_position;
static int rack_paint=-1,rack_paint_channel=-1; static float rack_paint_step;
static int navigation_drag=-1,navigation_active;
static Vector2 navigation_last;
static NavigationInput navigation_input;
static float piano_scroll_remainder;
static int captured(void) { return picker_drag>=0 || navigation_drag>=0 || track_resize>=0 || rack_hdrag || rack_vdrag || piano_key_drag || control_drag || route_drag>=0 || note_drag || velocity_drag>=0 || arrangement.gesture || playlist_pan || browser_resize || rack_paint>=0 || cable_drag>0 || playlist_vpan || mixer_pan || piano_scroll_drag || piano_vdrag || piano_gesture || marker_drag>=0; }
static void sampler_queue(void) {
    sampler_update();
    if(sampler_job.busy || control_drag || pattern_popup==5) return;
    for(int c=0;c<project.channel_count;c++) if(sampler_generation[c]!=sample_generation[c] || !sampler_equal(sampler_applied[c],project.sampler[c])) {
        Sample copy=sample_copy(originals[c]);
        if(originals[c].frames && !copy.data) return;
        sampler_job.input=copy; sampler_job.result=(Sample){0}; sampler_job.channel=c;
        sampler_job.settings=project.sampler[c]; sampler_job.epoch=sample_epoch; sampler_job.generation=sample_generation[c]; atomic_store(&sampler_job.done,0);
        if(pthread_create(&sampler_job.thread,NULL,sampler_worker,NULL)) { free(copy.data); return; }
        sampler_job.busy=1; return;
    }
}
static char status[256]="Use the toolbar to open editors; drag their title bars to arrange them";
static char project_path[1024]="project.hbt";
static float playback_start(void) {
    float start=song?playlist_start*STEPS:piano_start[pattern];
    float end=song?song_steps(&project):project.pattern_steps[pattern];
    return playing && !playback_loop()[1] && start>=end?0:start;
}
static void transport_toggle(void) { int active=audio_stop(); playing=playing || active?0:1; reset=1; }
/* Rasterize TTF at each display size; never enlarge a small glyph atlas. */
static Font ui_fonts[23];
static float font_scale;
static Vector2 text_origin;
static void fonts_close(void) {
    for(int size=10;size<=22;size++) if(ui_fonts[size].texture.id && ui_fonts[size].texture.id!=GetFontDefault().texture.id) UnloadFont(ui_fonts[size]);
    memset(ui_fonts,0,sizeof ui_fonts);
}
static void icons_init(float scale);
static void fonts_update(float scale) {
    if(font_scale==scale) return;
    fonts_close(); font_scale=scale; icons_init(scale);
    int glyphs[224]; for(int i=0;i<224;i++) glyphs[i]=32+i;
    for(int size=10;size<=22;size++) {
        ui_fonts[size]=LoadFontEx(font_path,fmaxf(1,roundf(size*scale)),glyphs,224);
        if(!ui_fonts[size].texture.id) ui_fonts[size]=GetFontDefault();
        SetTextureFilter(ui_fonts[size].texture,TEXTURE_FILTER_POINT);
    }
}
static int text_width(const char *text,int size) {
    return (int)ceilf(MeasureTextEx(ui_fonts[size],text,ui_fonts[size].baseSize,0).x/font_scale);
}
static void label(const char *text,int x,int y,int size,Color color) {
    DrawTextEx(ui_fonts[size],text,(Vector2){(roundf(x*font_scale+text_origin.x)-text_origin.x)/font_scale,(roundf(y*font_scale+text_origin.y)-text_origin.y)/font_scale},ui_fonts[size].baseSize/font_scale,0,color);
}
static void backspace(char *text) {
    size_t n=strlen(text); if(!n) return;
    do { n--; } while(n && ((unsigned char)text[n]&0xc0)==0x80);
    text[n]=0;
}
static const char *fit_text(const char *text,int width,int size) {
    static char fitted[PATH_MAX]; if(text!=fitted) snprintf(fitted,sizeof fitted,"%s",text);
    while(fitted[0] && text_width(fitted,size)>width) backspace(fitted);
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
enum { ICON_RACK,ICON_PLAYLIST,ICON_PIANO,ICON_MIXER,ICON_PENCIL,ICON_BRUSH,ICON_SELECT,ICON_METRO,ICON_PLAY,ICON_STOP,ICON_PAUSE,ICON_CLOSE,ICON_BACK,ICON_FOLLOW,ICON_KEYS,ICON_WAVE,ICON_AUTOMATION,ICON_COUNT };
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
        if(id==ICON_WAVE) for(int x=5;x<24;x+=3) DrawLineEx((Vector2){x,14-3-(x%3+1)*2},(Vector2){x,14+3+(x%5)},2,WHITE);
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
    float left=(roundf((x-pixels/font_scale/2)*font_scale+text_origin.x)-text_origin.x)/font_scale;
    float top=(roundf((y-pixels/font_scale/2)*font_scale+text_origin.y)-text_origin.y)/font_scale;
    DrawTexturePro(icons,(Rectangle){id*icon_large,variant==2?icon_large+icon_medium:variant==1?icon_large:0,pixels,pixels},(Rectangle){left,top,pixels/font_scale,pixels/font_scale},(Vector2){0},0,color);
}
/* Cached value arcs keep their edges as smooth as the knob circles. */
static Texture2D knob_arcs;
static int knob_context=-1,knob_animating;
#define KNOB_RADIUS 10
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
static int hover(float x,float y,float w,float h) { return input_enabled && CheckCollisionPointRec(mouse,(Rectangle){x,y,w,h}); }
/* Small control symbols share one size and never depend on font glyphs. */
static void zoom_label(float percent,int x,int y) { label(fit_text(TextFormat(percent<1?"%.2f%%":"%.0f%%",percent),56,11),x,y,11,muted); }
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
    if(over && *text) {
        const char *description=text;
        if(!strcmp(text,"New")) description="Start an empty project with one unloaded Sampler";
        else if(!strcmp(text,"Demo")) description="Load the built-in drum and bass demo";
        else if(!strcmp(text,"Save")) description="Save the current project";
        else if(!strcmp(text,"Open")) description="Reload the current project file";
        else if(!strcmp(text,"Export")) description="Export the Playlist to song.wav";
        else if(!strcmp(text,"HELP")) description="Show all current keybindings";
        else if(!strcmp(text,"Dark")) description="Use dark colors; saved automatically for next launch";
        else if(!strcmp(text,"Light")) description="Use light colors; saved automatically for next launch";
        else if(!strncmp(text,"Transparency:",13)) description="Toggle see-through window surfaces; saved automatically for next launch";
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
static int picker_button(int y,Color color,int selected) {
    int over=hover(4,y,112,50);
    ui_surface((Rectangle){4,y,112,50},color);
    if(over || selected) DrawRectangleLinesEx((Rectangle){4,y,112,50},2,accent);
    return over && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
}
static void mute_light(int x,int y,uint8_t *states,int count,int selected,const char *name) {
    uint8_t state=states[selected]; int solo=count?solo_any(states,count):0;
    int over=hover(x-8,y-8,16,16),enabled=!(state&1) && (!solo || (state&2));
    Color color=enabled?(state&2?ui_theme.meter_mid:ui_theme.meter_low):cell;
    circle(x,y,6,over?ink:state&2?ui_theme.meter_mid:ui_theme.border); circle(x,y,5,color);
    if(over) {
        snprintf(status,sizeof status,"%s: %s | Left-click: mute/unmute; right-click: %s",name,state&1?"Muted":state&2?"Solo":solo?"Silenced by solo":"Enabled",count?"add/remove solo":"clear all solos");
        if(IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) { states[selected]^=1; input_enabled=0; }
        if(IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
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
static void add_sampler(void);
static void open_popup(int kind) {
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
    if(button_color(fit_text(TextFormat("%s: %s",io?"Out":"In",device_caption(selected)),width-30,13),x,y,width,22,0,ui_theme.browser)) {
        device_target=mixer_selected; device_io=io; device_scroll=0;
        device_count=audio_devices(!io,device_names,64);
        open_popup(8);
        Rect r=windows.editors[3].rect;
        popup_position[8]=(Vector2){r.x+x,r.y+y+24};
    }
    if(hover(x,y,width,22)) snprintf(status,sizeof status,"Audio %s device: saved per mixer track; capture and external-device routing are not implemented yet",io?"output":"input");
    DrawTriangle((Vector2){x+width-16,y+8},(Vector2){x+width-12,y+14},(Vector2){x+width-8,y+8},muted);
}
static int load_project(const char *path);
static void new_project(int demo);
static void help_group(int x,int *y,const char *title) {
    label(title,x,*y,13,accent); *y+=26;
}
static void help_binding(int x,int *y,const char *keys,const char *action) {
    label(keys,x,*y,11,ink); label(action,x+126,*y,11,muted); *y+=23;
}
static void draw_popup(void) {
    if(!pattern_popup) { popup_drag=0; return; }
    input_enabled=1;
    if(pattern_popup==1 || pattern_popup==9) {
        int file=pattern_popup==1,x=file?8:116,y=34,w=180,h=file?138:34;
        if(IsKeyPressed(KEY_ESCAPE) || (!popup_opened && IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && !hover(x,y,w,h))) { pattern_popup=0; return; }
        ui_frame((Rectangle){x,y,w,h});
        const char *items[]={"New","Demo","Save","Open","Export"};
        for(int i=0;i<(file?5:1);i++) if(button(file?items[i]:"Keybindings",x+4,y+4+i*26,w-8,24,0) && !popup_opened) {
            pattern_popup=0;
            if(!file) open_popup(4);
            else if(i==0) new_project(0);
            else if(i==1) new_project(1);
            else if(i==2) snprintf(status,sizeof status,project_save(project_path,&project)?"Saved %.150s":"Save failed: %.150s",project_path);
            else if(i==3) load_project(project_path);
            else snprintf(status,sizeof status,(sampler_flush() && export_wav("song.wav",&project,samples))?"Exported song.wav (Playlist, 48 kHz stereo)":"Export failed");
            return;
        }
        return;
    }
    if(pattern_popup==8) {
        int visible=fminf(10,device_count+2),w=300,h=visible*24+8;
        float scale=fminf(GetScreenWidth()/1200.f,GetScreenHeight()/675.f);
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
                snprintf(status,sizeof status,"Device choice saved in project; capture / external-device routing will use it when implemented");
            }
        }
        return;
    }
    if(pattern_popup==7) {
        int x=60,y=34,w=192,h=126;
        if(IsKeyPressed(KEY_ESCAPE) || (!popup_opened && IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && !hover(x,y,w,h))) { pattern_popup=0; return; }
        ui_frame((Rectangle){x,y,w,h});
        label("Appearance",x+10,y+9,12,muted);
        for(int light=0;light<2;light++) {
            if(button(light?"Light":"Dark",x+6,y+30+light*28,w-12,25,ui_theme.light==light) && !popup_opened) {
                int saved=theme_select(light); pattern_popup=0;
                snprintf(status,sizeof status,saved?"Theme saved":"Theme changed for this session; preferences could not be saved");
            }
        }
        if(button(ui_theme.transparent?"Transparency: On":"Transparency: Off",x+6,y+90,w-12,25,ui_theme.transparent) && !popup_opened) {
            int saved=theme_transparency(!ui_theme.transparent); pattern_popup=0;
            snprintf(status,sizeof status,saved?"Transparency preference saved":"Transparency changed for this session; preferences could not be saved");
        }
        return;
    }
    int folder=pattern_popup==3,help=pattern_popup==4,number=pattern_popup==5,w=pattern_popup==6?320:help?660:number?360:folder?620:260,h=help?420:116;
    float scale=fminf(GetScreenWidth()/1200.f,GetScreenHeight()/675.f);
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
    ui_surface((Rectangle){x,y,w,TITLE},ui_glass(ui_theme.title_focus,90));
    if(button("x",x+w-24,y+1,20,16,0)) { pattern_popup=0; popup_drag=0; return; }
    if(hover(x,y,w-26,TITLE)) snprintf(status,sizeof status,"Drag this title bar to move the dialog");
    if(hover(x,y,w-26,TITLE) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) { popup_drag=1; popup_offset=(Vector2){mouse.x-x,mouse.y-y}; }
    if(IsKeyPressed(KEY_ESCAPE)) { pattern_popup=0; popup_drag=0; return; }
    if(!popup_drag && !popup_opened && IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && !hover(x,y,w,h)) { pattern_popup=0; return; }
    input_enabled=!popup_drag;
    if(pattern_popup==6) {
        label("Add instrument",x+8,y+3,13,ink);
        if(button("Sampler",x+12,y+38,w-24,30,0)) { add_sampler(); pattern_popup=0; }
        return;
    }
    if(help) {
        label("Help - Keybindings",x+8,y+3,13,ink);
        int left=x+18,right=x+348,ly=y+34,ry=ly;
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
        help_group(right,&ry,"Editing & selection");
        help_binding(right,&ry,"Shift-click","Add / remove selection");
        help_binding(right,&ry,"Shift-drag empty","Add selection rectangle");
        help_binding(right,&ry,"Delete","Delete selected clips / notes");
        help_binding(right,&ry,"Right-drag","Erase clips / notes");
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
        label("Navigation applies to the editor under the pointer. Audition requires Keys enabled.",x+18,y+h-24,11,muted);
        return;
    }
    char *text=number?number_text:folder?folder_text:rename_text; size_t capacity=number?sizeof number_text:folder?sizeof folder_text:sizeof rename_text;
    label(number?number_name:folder?"Add Browser folder":(rename_channel>=0?TextFormat("Rename channel %d",rename_channel+1):rename_mixer>=0?TextFormat("Rename insert %d",rename_mixer+1):rename_track>=0?TextFormat("Rename track %d",rename_track+1):TextFormat("Rename pattern %d",rename_pattern+1)),x+8,y+3,13,ink);
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
            snprintf(rename_channel>=0?project.channel_names[rename_channel]:rename_mixer>=0?project.insert_names[rename_mixer]:rename_track>=0?project.track_names[rename_track]:project.pattern_names[rename_pattern],PATTERN_NAME,"%s",name);
        }
        pattern_popup=0;
    }
}
static int import_sample(const char *path,int c) {
    char full[PATH_MAX]; Sample s;
    if(!realpath(path,full) || strlen(full)>=sizeof project.paths[c] || !sample_load(full,&s)) { snprintf(status,sizeof status,"Cannot load sample: unreadable, unsupported, too large or insufficient memory."); return 0; }
    Sample cooked;
    if(!sample_process(s,project.sampler[c],&cooked)) { free(s.data); snprintf(status,sizeof status,"Sample processing failed; channel unchanged"); return 0; }
    Sample old=samples[c],raw=originals[c]; audio_sample(c,cooked); samples[c]=cooked; originals[c]=s; free(old.data); free(raw.data);
    sample_generation[c]++; sampler_generation[c]=sample_generation[c]; sampler_applied[c]=project.sampler[c];
    project.audio_seconds[c]=samples[c].frames/(float)RATE;
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
    audio_preview(next); free(audition.data); audition=next;
    snprintf(audition_path,sizeof audition_path,"%s",path);
    for(int i=0;i<128;i++) {
        audition_low[i]=audition_high[i]=0;
        for(size_t j=(size_t)i*next.frames/128;j<(size_t)(i+1)*next.frames/128;j++) {
            audition_low[i]=fminf(audition_low[i],fminf(sample_at(next,j,0),sample_at(next,j,1)));
            audition_high[i]=fmaxf(audition_high[i],fmaxf(sample_at(next,j,0),sample_at(next,j,1)));
        }
    }
    snprintf(status,sizeof status,"Preview: %.140s",GetFileName(path));
}
static void rack_reveal_last(void) {
    Editor *e=&windows.editors[0];
    int count=rack_channels(NULL);
    e->rect.h=fminf(GetScreenHeight()/fminf(GetScreenWidth()/1200.f,GetScreenHeight()/675.f)-66,RACK_TOP+20+count*28);
    rack_scroll=fmaxf(0,count-(int)((e->rect.h-RACK_TOP-20)/28));
}
static void add_sampler(void) {
    int c=project.channel_count;
    if(c>=CHANNELS) { snprintf(status,sizeof status,"Channel Rack limit: %d channels.",CHANNELS); return; }
    project.channel_pitch[c]=0; project.pitch_range[c]=2; project.channel_audio[c]=0; project.audio_seconds[c]=0; rack_filter=2; project.channel_count++; project.sampler[c]=(Sampler){.time=1,.length=1}; project.volume[c]=1; project.pan[c]=0; project.mute[c]=0; project.route[c]=0;
    snprintf(project.paths[c],sizeof project.paths[c],"%s",SAMPLE_EMPTY);
    memset(project.channel_names[c],0,PATTERN_NAME); snprintf(project.channel_names[c],PATTERN_NAME,"Sampler");
    for(int pat=0;pat<PATTERNS;pat++) {
        memset(project.notes[pat][c],0,sizeof project.notes[pat][c]); memset(note_selected[pat][c],0,NOTES); piano_channels[pat][c]=0;
    }
    Sample old=samples[c],raw=originals[c]; originals[c]=samples[c]=(Sample){0}; audio_channels(&project,samples); free(old.data); free(raw.data);
    sample_generation[c]++; sampler_generation[c]=sample_generation[c]; sampler_applied[c]=project.sampler[c];
    channel=instrument_channel=c; browser_focus=0; rack_reveal_last(); windows_focus(&windows,4);
}
static uint32_t next_source_color(const uint32_t *colors,int count) {
    for(int color=0;color<PATTERNS;color++) {
        int used=0;
        for(int i=0;i<count;i++) if(colors[i]==pattern_palette[color]) used=1;
        if(!used) return pattern_palette[color];
    }
    return pattern_palette[count%PATTERNS];
}
static Color audio_color(int c) {
    if(project.channel_colors[c]) { unsigned rgb=project.channel_colors[c]; return (Color){rgb>>16,(rgb>>8)&255,rgb&255,255}; }
    float t=fminf(1,log1pf(samples[c].frames/(float)RATE)/log1pf(30));
    Color short_color={142,73,78,255},long_color={72,137,91,255};
    Color color={(unsigned char)(short_color.r+(long_color.r-short_color.r)*t),(unsigned char)(short_color.g+(long_color.g-short_color.g)*t),(unsigned char)(short_color.b+(long_color.b-short_color.b)*t),255};
    return color;
}
static Waveform audio_waves[CHANNELS];
static void audio_waveform(int c,float x,float y,float left,float right,float pixels_per_step,float height) {
    static unsigned generations[CHANNELS],frames[CHANNELS],epochs[CHANNELS];
    static Sampler settings[CHANNELS];
    Sample sample=samples[c]; if(!sample.frames) return;
    if(generations[c]!=sample_generation[c] || frames[c]!=sample.frames || epochs[c]!=sample_epoch || !sampler_equal(settings[c],sampler_applied[c])) {
        waveform_build(&audio_waves[c],sample);
        generations[c]=sample_generation[c]; frames[c]=sample.frames; epochs[c]=sample_epoch; settings[c]=sampler_applied[c];
    }
    float full=sample.frames/(float)RATE*project.bpm/15/channel_speed(&project,c)*pixels_per_step;
    if(x>=left && x<=right) DrawLineEx((Vector2){x,y+14},(Vector2){x,y+height-1},1,Fade(ink,.6f));
    if(x+full>=left && x+full<=right+1) DrawLineEx((Vector2){x+full-1,y+14},(Vector2){x+full-1,y+height-1},1,Fade(ink,.6f));
    right=fminf(right,x+full);
    for(int px=(int)ceilf(left);px<right;px++) {
        unsigned first=fmax(0,floor((px-(double)x)/full*sample.frames)),last=fmin(sample.frames,ceil((px+1-(double)x)/full*sample.frames));
        WavePeak peak=waveform_range(&audio_waves[c],sample,first,last);
        float mid=y+14+(height-14)/2,amplitude=(height-18)/2;
        DrawLineEx((Vector2){px,mid-peak.high*amplitude},(Vector2){px,mid-peak.low*amplitude},1,ui_theme.waveform);
    }
}
static void drop_sample(const char *path) {
    int target=windows_hit(&windows,mouse.x,mouse.y);
    if(target==1) {
        Rect r=windows.editors[1].rect;
        float gx=r.x+212,gy=r.y+82,gridw=r.w-236;
        int list=mouse.x>=r.x+4 && mouse.x<r.x+116 && mouse.y>=gy && mouse.y<r.y+r.h-42;
        if(!list && (mouse.x<gx || mouse.x>=gx+gridw || mouse.y<gy || mouse.y>=r.y+r.h-42)) { snprintf(status,sizeof status,"Drop audio onto the Playlist grid or Audio list."); return; }
        int c=project.channel_count;
        if(c>=CHANNELS) { snprintf(status,sizeof status,"Channel Rack limit: %d channels.",CHANNELS); return; }
        float pixels=gridw/BARS*arrangement.zoom,q=snap_interval(snap_mode[0],pixels/STEPS);
        float start=floorf((arrangement.view_start+(mouse.x-gx)/pixels)*STEPS/q)*q/STEPS;
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
                Sample empty={0}; audio_sample(c,empty); free(samples[c].data); free(originals[c].data); samples[c]=originals[c]=empty;
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
    int visible=fmaxf(1,(r.h-RACK_TOP-20)/28);
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
    audio_channels(&project,samples); free(old.data); free(raw.data); reset=1;
}
static void install_project(Project next,Sample fresh[CHANNELS],Sample processed[CHANNELS],const char *filename) {
    sample_epoch++; audio_stop();
    free(audition.data); audition=(Sample){0}; audition_path[0]=0; browser_progress=-1;
    memset(keyboard_notes,0,sizeof keyboard_notes);
    picker_tab=picker_scroll=0; picker_drag=-1; sample_drag[0]=0;
    rack_hdrag=rack_vdrag=playlist_pan=playlist_vpan=piano_scroll_drag=piano_vdrag=mixer_pan=0;
    piano_scroll_remainder=0; memset(track_heights,0,sizeof track_heights);
    navigation_active=0; navigation_drag=-1; track_resize=-1; ruler_loop_drag=0;
    visual_step=0;
    playing=0; pattern=(int)fminf(pattern,next.pattern_count-1); audio_update(&next,0,song,pattern,1,output_volume,0,0,0);
    for(int c=0;c<CHANNELS;c++) next.audio_seconds[c]=processed[c].frames/(float)RATE;
    audio_channels(&next,processed);
    for(int c=0;c<CHANNELS;c++) {
        Sample old=samples[c],raw=originals[c]; samples[c]=processed[c]; originals[c]=fresh[c]; free(old.data); free(raw.data);
        sample_generation[c]++; sampler_generation[c]=sample_generation[c]; sampler_applied[c]=next.sampler[c];
    }
    project=next; memcpy(project_path,filename,strlen(filename)+1);
    project.insert_count=INSERTS; rack_scroll=0; rack_filter=0; channel=fmaxf(0,fminf(channel,project.channel_count-1));
    piano_channel=instrument_channel=channel;
    for(int i=0;i<PATTERNS;i++) { piano_span[i]=fmaxf(STEPS,next.pattern_steps[i]); piano_pan[i]=0; piano_range[i]=0; }
    memset(note_selected,0,sizeof note_selected); piano_gesture=PIANO_IDLE; playlist_start=0; memset(piano_start,0,sizeof piano_start); memset(song_loop,0,sizeof song_loop); memset(pattern_loop,0,sizeof pattern_loop); marker_drag=-1;
    memset(piano_channels,0,sizeof piano_channels); mixer_selected=project.route[channel]; mixer_scroll=0;
    piano_key_drag=0; piano_key=-1;
    note_drag=NULL; velocity_drag=-1; control_drag=NULL; route_drag=-1; rack_paint=-1; cable_drag=-1;
    memset(&arrangement,0,sizeof arrangement); arrangement.source_pattern=-1; arrangement.source_steps=STEPS; arrangement.zoom=1; playlist_pan=0; playlist_vpan=0; track_scroll=0; memset(rack_view,0,sizeof rack_view); memset(rack_range,0,sizeof rack_range);
    snprintf(status,sizeof status,"Loaded %.150s",filename); reset=1;
}
static void new_project(int demo) {
    Project next; Sample fresh[CHANNELS]={0},processed[CHANNELS]={0};
    if(demo) { project_demo(&next); samples_default(fresh); }
    else project_new(&next);
    for(int c=0;c<CHANNELS;c++) {
        if((demo && c<4 && !fresh[c].data) || !sample_process(fresh[c],next.sampler[c],&processed[c])) {
            for(int i=0;i<CHANNELS;i++) { free(fresh[i].data); free(processed[i].data); }
            snprintf(status,sizeof status,"Project unchanged: could not prepare samples."); return;
        }
    }
    pattern=channel=0; song=demo; install_project(next,fresh,processed,demo?"demo.hbt":"project.hbt");
    snprintf(status,sizeof status,demo?"Demo loaded: press Play to hear the eight-bar groove.":"New project: load a sample to get started.");
}
static int load_project(const char *path) {
    Project next; Sample fresh[CHANNELS]={0},processed[CHANNELS]={0};
    if(strlen(path)>=sizeof project_path || !project_load(path,&next)) { snprintf(status,sizeof status,"Invalid project: %.120s",path); return 0; }
    char filename[sizeof project_path]; snprintf(filename,sizeof filename,"%s",path);
    samples_default(fresh);
    for(int c=0;c<CHANNELS;c++) {
        if(c<4 && !fresh[c].data) goto failed;
        if(!strcmp(next.paths[c],SAMPLE_EMPTY)) { free(fresh[c].data); fresh[c]=(Sample){0}; }
        else if(next.paths[c][0]) { Sample s; if(!sample_load(next.paths[c],&s)) goto failed; free(fresh[c].data); fresh[c]=s; }
    }
    for(int c=0;c<CHANNELS;c++) if(!sample_process(fresh[c],next.sampler[c],&processed[c])) goto failed;
    install_project(next,fresh,processed,filename); return 1;
failed:
    for(int c=0;c<CHANNELS;c++) { free(fresh[c].data); free(processed[c].data); }
    snprintf(status,sizeof status,"Project unchanged: a referenced sample is missing or invalid."); return 0;
}
static void capture_control(float *value,float low,float high,int fader) {
    control_integer=0; control_raw=*value; control_drag=value; control_low=low; control_high=high; control_fader=fader; control_reverse=0; input_enabled=0;
}
enum { KNOB_NORMAL,KNOB_SWING,KNOB_PAN,KNOB_WIDTH,KNOB_CENTER,KNOB_WET,KNOB_VOLUME };
static void knob_style(int x,int y,float *value,float low,float high,float initial,const char *name,int style) {
    float size=KNOB_RADIUS;
    int over=hover(x-size-1,y-size-1,size*2+2,size*2+2);
    if(over || control_drag==value) {
        if(style==KNOB_VOLUME) snprintf(status,sizeof status,"%s: %.4g (%.2f dB) | Dot marks 0 dB; drag or wheel; right-click for number entry",name,*value,gain_db(*value));
        else snprintf(status,sizeof status,"%s: %.4g | Drag up/down or wheel; right-click for number entry",name,*value);
    }
    if(over) {
        if(IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) begin_number(value,low,high,initial,name);
        if(IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) { capture_control(value,low,high,0); control_reverse=style==KNOB_WIDTH; }
        *value=fmaxf(low,fminf(high,*value+GetMouseWheelMove()*(high-low)/20*(style==KNOB_WIDTH?-1:1)));
    }
    static struct { float *value; int context; float hover; } states[2048];
    static int count;
    int i=0; while(i<count && (states[i].value!=value || states[i].context!=knob_context)) i++;
    if(i==count && count<2048) { states[i].value=value; states[i].context=knob_context; count++; }
    float shrink=0;
    if(i<count) {
        float target=over || control_drag==value;
        states[i].hover+=(target-states[i].hover)*(1-expf(-24*fminf(GetFrameTime(),.05f)));
        if(fabsf(states[i].hover-target)<.001f) states[i].hover=target;
        else knob_animating=1;
        shrink=states[i].hover;
    }
    float radius=size*(1-.08f*shrink),fraction=(*value-low)/(high-low);
    if(style==KNOB_CENTER) fraction=*value<initial?.5f*(*value-low)/(initial-low):.5f+.5f*(*value-initial)/(high-initial);
    if(style==KNOB_WIDTH) fraction=1-fraction;
    if(style==KNOB_VOLUME) fraction=*value<=1?.75f*(*value):.75f+.25f*(*value-1)/(high-1);
    int centered=style==KNOB_PAN || style==KNOB_WIDTH || style==KNOB_CENTER;
    float origin=centered?PI/2:style==KNOB_SWING?2.4f:style==KNOB_VOLUME?PI/2:-PI/2;
    float angle=origin+fraction*(style==KNOB_SWING?4.6f:2*PI);
    int arc_row=0;
    Color color=style==KNOB_WET?muted:style==KNOB_SWING || style==KNOB_CENTER?ui_theme.swing:style==KNOB_PAN?(fraction<.5f?ui_theme.pan_left:ui_theme.pan_right):style==KNOB_WIDTH?(fraction<.5f?ui_theme.stereo:ui_theme.mono):ui_theme.knob;
    if(style==KNOB_SWING) {
        DrawTexturePro(knob_arcs,(Rectangle){0,256,32,32},(Rectangle){x-radius,y-radius,radius*2,radius*2},(Vector2){0},0,ui_theme.border);
        circle(x,y,radius*.68f,cell);
    } else { circle(x,y,radius,ui_theme.border); circle(x,y,radius*.8f,cell); }
    int level=fmaxf(0,fminf(128,roundf(fraction*128)));
    float rotation=style==KNOB_SWING || centered?0:(origin+PI/2)*RAD2DEG;
    DrawTexturePro(knob_arcs,(Rectangle){(style==KNOB_SWING?0:centered?512:1024)+level%16*32,arc_row+level/16*32,32,32},(Rectangle){x,y,radius*2,radius*2},(Vector2){radius,radius},rotation,color);
    if(style==KNOB_VOLUME) {
        float unity=origin+.75f*2*PI;
        circle(x+cosf(unity)*size*1.22f,y+sinf(unity)*size*1.22f,1.2f,muted);
    }
    if(style!=KNOB_WET) circle(x+cosf(angle)*radius*.85f,y+sinf(angle)*radius*.85f,1.7f,centered && level==64?muted:color);
}
static void knob(int x,int y,float *value,float low,float high,float initial,const char *name) {
    knob_style(x,y,value,low,high,initial,name,KNOB_NORMAL);
}
static void channel_route(int c,int x,int y) {
    if(button(TextFormat("%d",project.route[c]),x,y,24,22,route_drag==c)) {
        route_drag=c; route_start=project.route[c]; route_y=mouse.y+windows.editors[knob_context].rect.y;
        channel=c; mixer_selected=project.route[c]; input_enabled=0;
    }
    if(hover(x,y,24,22)) snprintf(status,sizeof status,"%s mixer destination: drag up/down; 0 = Master",project.channel_names[c]);
}
static void channel_controls(int c,float width) {
    int x=width-220;
    mute_light(x,36,project.mute,project.channel_count,c,project.channel_names[c]);
    knob_style(x+28,36,&project.pan[c],-1,1,0,"Channel pan",KNOB_PAN);
    knob_style(x+58,36,&project.volume[c],0,VOLUME_KNOB_MAX,1,"Channel volume",KNOB_VOLUME);
    knob_style(x+94,36,&project.channel_pitch[c],-1,1,0,"Channel pitch",KNOB_CENTER);
    if(hover(x+83,25,22,22) || control_drag==&project.channel_pitch[c]) snprintf(status,sizeof status,"Channel pitch: %+.2f semitones | range ±%.0f semitones",project.channel_pitch[c]*project.pitch_range[c],project.pitch_range[c]);
    float *range=&project.pitch_range[c];
    if(button(TextFormat("%.0f",*range),x+111,25,30,22,control_drag==range)) { capture_control(range,1,48,0); control_integer=1; }
    if(hover(x+111,25,30,22)) {
        *range=fmaxf(1,fminf(48,*range+roundf(GetMouseWheelMove())));
        if(IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) { begin_number(range,1,48,2,"Pitch range (semitones)"); number_integer=1; }
        snprintf(status,sizeof status,"Pitch range: %.0f semitones | drag up/down, 1–48 whole semitones; right-click to enter",*range);
    }
    channel_route(c,x+161,25);
    label("PAN",x+18,53,10,muted); label("VOL",x+48,53,10,muted);
    label("PITCH",x+82,53,10,muted); label("RANGE",x+111,53,10,muted); label("MIX",x+162,53,10,muted);
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
    int visible=fmaxf(1,(windows.editors[0].rect.h-RACK_TOP-20)/28);
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
static void rack(float width,float height) {
    float stepw=RACK_STEP_WIDTH,gridw=width-226,view=rack_view[pattern],span=gridw/stepw;
    rack_range[pattern]=fmaxf(rack_range[pattern],fmaxf(view+span*2,project.pattern_steps[pattern]+span));
    timeline_ruler(0,view,span,200,TITLE,gridw,1);
    int visible=fmaxf(1,(height-RACK_TOP-20)/28),rows[CHANNELS],count=rack_channels(rows);
    if(hover(0,RACK_TOP,width,height-RACK_TOP-18)) rack_scroll-=GetMouseWheelMove();
    rack_scroll=fmaxf(0,fminf(fmaxf(0,count-visible),rack_scroll));
    for(int index=rack_scroll;index<count && index<rack_scroll+visible;index++) {
        int c=rows[index],row=RACK_TOP+(index-rack_scroll)*28;
        mute_light(12,row+11,project.mute,project.channel_count,c,project.channel_names[c]);
        knob_style(36,row+11,&project.pan[c],-1,1,0,"Channel pan",KNOB_PAN); knob_style(62,row+11,&project.volume[c],0,VOLUME_KNOB_MAX,1,"Channel volume",KNOB_VOLUME);
        channel_route(c,78,row);
        if(button_color(fit_text(project.channel_names[c],68,13),106,row,78,22,0,project.channel_audio[c]?audio_color(c):cell)) {
            channel=instrument_channel=c; windows_focus(&windows,4);
        }
        if(hover(106,row,78,22) && IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) { channel=c; open_context(2,c,(Vector2){mouse.x+windows.editors[0].rect.x,mouse.y+windows.editors[0].rect.y}); }
        if(hover(106,row,78,22)) snprintf(status,sizeof status,"%s: open instrument; right-click for Piano Roll, rename, mute or delete; Keys: Z/Q white, S/2 black",project.channel_names[c]);
        DrawRectangle(187,row+2,3,18,channel==c?accent:muted);
        int melodic=rack_melodic(c);
        if(melodic) {
            DrawRectangle(200,row+2,gridw,20,bg);
            float active=fmaxf(0,fminf(gridw,(edit_steps()-view)*stepw));
            if(active<gridw) DrawRectangle(200+active,row+2,gridw-active,20,ui_theme.disabled);
            for(int i=0;i<NOTES;i++) {
                Note n=project.notes[pattern][c][i]; float start=200+(n.start-view)*stepw,end=start+fmaxf(2,(n.length?n.length:1)*stepw-2);
                if(n.velocity && end>=200 && start<200+gridw) DrawRectangle(fmaxf(200,start),row+3+fmaxf(0,fminf(16,(72-(int)n.pitch)*.65f)),fminf(200+gridw,end)-fmaxf(200,start),2,accent);
            }
            if(hover(200,row+2,gridw,20)) snprintf(status,sizeof status,"%s notes: right-click for Piano Roll",project.channel_names[c]);
            if(hover(200,row+2,gridw,20) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) channel=c;
            if(hover(200,row+2,gridw,20) && IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) { channel=c; open_context(2,c,(Vector2){mouse.x+windows.editors[0].rect.x,mouse.y+windows.editors[0].rect.y}); }
            continue;
        }
        for(int i=0;i<span+2;i++) {
            float step=floorf(view)+i,x=200+(step-view)*stepw,left=fmaxf(200,x),right=fminf(200+gridw,x+stepw-3);
            if(right<=left) continue;
            Note *n=note_at(&project,pattern,c,step,60); int available=step<edit_steps();
            ui_surface((Rectangle){left+1,row+2,fmaxf(1,right-left-2),18},!available?ui_theme.disabled:n?ui_theme.step_on[(int)floorf(step/4)%2]:fmodf(floorf(step/4),2)?ui_theme.step_alt:cell);
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
    double position=playing && !song?visual_step:-1;
    if(playing && song) {
        double latest=-1; int solo=solo_any(project.lane_mute,LANES);
        for(int l=0;l<LANES;l++) if(!(project.lane_mute[l]&1) && (!solo || (project.lane_mute[l]&2)))
            for(int b=0;b<CLIPS;b++) if(project.clips[l][b]==pattern+1) {
                double start=project.clip_starts[l][b]*STEPS,local=visual_step-start;
                if(start>latest && local>=0 && local<clip_length(&project,l,b) && local+project.clip_offsets[l][b]<project.pattern_steps[pattern]) { latest=start; position=local+project.clip_offsets[l][b]; }
            }
    }
    if(position>=0) {
        float x=200+(position-view)*stepw;
        if(x>=200 && x<=200+gridw)
            DrawLineEx((Vector2){x,RACK_TOP-4},(Vector2){x,RACK_TOP+fminf(visible,count-rack_scroll)*28-6},1.5f,ink);
    }
    int add_y=RACK_TOP+(int)fminf(visible,count-rack_scroll)*28;
    if(add_y+22<=height-18) {
        if(sample_drag[0] && sample_moved && mouse.y>=add_y && windows_hit(&windows,mouse.x+windows.editors[0].rect.x,mouse.y+windows.editors[0].rect.y)==0) {
            DrawRectangle(84,add_y,width-100,22,Fade(ui_theme.signal,.18f)); DrawRectangleLines(84,add_y,width-100,22,Fade(ui_theme.signal,.5f));
            label("Drop to add channel",92,add_y+5,11,ui_theme.signal);
        }
        if(hover(84,add_y,width-100,22)) snprintf(status,sizeof status,"Add instrument: click + to choose a plugin, or drop a sample here");
    }
    float area=visible*28,maximum=fmaxf(0,count-visible),thumb=fminf(area,fmaxf(24,area*visible/fmaxf(1,count))),travel=area-thumb;
    float y=RACK_TOP+(maximum?rack_scroll/maximum*travel:0);
    DrawRectangle(width-12,RACK_TOP,8,area,bg); ui_surface((Rectangle){width-12,y,8,thumb},rack_vdrag || hover(width-14,RACK_TOP,14,area)?accent:muted);
    if(hover(width-14,RACK_TOP,14,area)) {
        snprintf(status,sizeof status,"Drag to scroll Rack channels; wheel over rows also scrolls");
        if(IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            if(mouse.y<y || mouse.y>y+thumb) rack_scroll=fmaxf(0,fminf(maximum,(mouse.y-RACK_TOP-thumb/2)/fmaxf(1,travel)*maximum));
            rack_vy=mouse.y+windows.editors[0].rect.y; rack_vstart=rack_scroll; rack_vscale=maximum/fmaxf(1,travel); rack_vdrag=1; input_enabled=0;
        }
    }
    if(button("+",106,height-20,20,18,0)) open_popup(6);
    if(hover(106,height-20,20,18)) snprintf(status,sizeof status,"Add instrument: choose a plugin");
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
    Color color={rgb>>16,(rgb>>8)&255,rgb&255,255};
    if(ui_theme.light) { color.r+=(255-color.r)*.45f; color.g+=(255-color.g)*.45f; color.b+=(255-color.b)*.45f; }
    return color;
}
static Color pattern_color(int id) { return pattern_rgb(project.pattern_colors[id]); }
/* One small transparent image per pattern, reused by all Playlist copies. */
static struct {
    RenderTexture2D image;
    Note notes[CHANNELS][NOTES];
    float steps; int channels,valid; Color color;
} previews[PATTERNS];
static void previews_update(float scale) {
    float pixels=(windows.editors[1].rect.w-236)*arrangement.zoom/(BARS*STEPS)*scale;
    for(int pat=0;pat<project.pattern_count;pat++) {
        float size=project.pattern_steps[pat]*pixels;
        previews[pat].valid=0;
        if(!isfinite(size) || size>2048) continue; /* Keep vector detail at extreme zoom. */
        int w=fmaxf(1,ceilf(size)),h=fmaxf(1,ceilf(32*scale));
        if(previews[pat].image.texture.width!=w || previews[pat].image.texture.height!=h ||
           previews[pat].steps!=project.pattern_steps[pat] || previews[pat].channels!=project.channel_count ||
           memcmp(&previews[pat].color,&ink,sizeof ink) || memcmp(previews[pat].notes,project.notes[pat],sizeof previews[pat].notes)) {
            if(previews[pat].image.id) UnloadRenderTexture(previews[pat].image);
            previews[pat].image=LoadRenderTexture(w,h);
            if(!previews[pat].image.id) continue;
            memcpy(previews[pat].notes,project.notes[pat],sizeof previews[pat].notes);
            previews[pat].steps=project.pattern_steps[pat]; previews[pat].channels=project.channel_count; previews[pat].color=ink;
            BeginTextureMode(previews[pat].image); ClearBackground(BLANK);
            float spacing=fminf(6,26/fmaxf(1,project.channel_count-1));
            for(int c=0;c<project.channel_count;c++) for(int n=0;n<NOTES;n++) {
                Note note=project.notes[pat][c][n]; if(!note.velocity) continue;
                float x=note.start*w/project.pattern_steps[pat]+scale;
                float y=(3+c*spacing+fmaxf(-3,fminf(3,(60-(int)note.pitch)*.3f)))*h/32;
                float length=fmaxf(scale,(note.length?note.length:1)*w/project.pattern_steps[pat]-2*scale);
                DrawRectangleRec((Rectangle){x,y,length,2*h/32.f},ink);
            }
            EndTextureMode();
        }
        previews[pat].valid=1;
    }
}
static void clip_preview(int pat,float x,float y,float left,float right,float pixels,float steps,float height) {
    float full=project.pattern_steps[pat]*pixels;
    if(previews[pat].valid) {
        right=fminf(right,x+full);
        if(right>left) {
            Texture2D image=previews[pat].image.texture;
            DrawTexturePro(image,(Rectangle){(left-x)*image.width/full,image.height,(right-left)*image.width/full,-image.height},
                           (Rectangle){left,y+16,right-left,height-20},(Vector2){0},0,WHITE);
        }
    } else {
        float spacing=fminf(6,26/fmaxf(1,project.channel_count-1));
        for(int c=0;c<project.channel_count;c++) for(int n=0;n<NOTES;n++) {
            Note note=project.notes[pat][c][n]; if(!note.velocity || note.start>=steps) continue;
            float yy=y+19+c*spacing+fmaxf(-3,fminf(3,(60-(int)note.pitch)*.3f));
            float nx=x+note.start*pixels+1,nw=fmaxf(1,fminf(note.length?note.length:1,steps-note.start)*pixels-2);
            if(nx<=right && nx+nw>=left) DrawRectangleRec((Rectangle){fmaxf(left,nx),yy,fmaxf(1,fminf(right,nx+nw)-fmaxf(left,nx)),2},ink);
        }
    }
}
static void picker_notes(int p,float x,float y) {
    for(int c=0;c<project.channel_count;c++) for(int n=0;n<NOTES;n++) {
        Note note=project.notes[p][c][n]; if(!note.velocity || note.start>=project.pattern_steps[p]) continue;
        float px=x+4+104*note.start/project.pattern_steps[p];
        float length=fmaxf(1,104*fminf(note.length?note.length:1,project.pattern_steps[p]-note.start)/project.pattern_steps[p]-1);
        float py=y+23+c*18.f/fmaxf(1,project.channel_count-1)+fmaxf(-2,fminf(2,(60-(int)note.pitch)*.2f));
        DrawRectangleRec((Rectangle){px,py,length,2},ink);
    }
}
static void draw_picker_drag(void) {
    if(picker_drag<0 || (fabsf(mouse.x-picker_origin.x)<3 && fabsf(mouse.y-picker_origin.y)<3)) return;
    int source=picker_drag,audio=source>=PATTERNS;
    float x=mouse.x-picker_offset.x,y=mouse.y-picker_offset.y,steps=clip_source_steps(&project,source);
    Color color=audio?audio_color(source-PATTERNS):pattern_color(source);
    ui_surface((Rectangle){x,y,112,50},color);
    if(audio) audio_waveform(source-PATTERNS,x+4,y+20,x+4,x+108,104/fmaxf(.001f,steps),28);
    else picker_notes(source,x,y);
    label(fit_text(audio?project.channel_names[source-PATTERNS]:project.pattern_names[source],100,12),x+4,y+4,12,ink);
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
    int over=hover(x,28,20,20); Color c=active==tool?ui_theme.selected_text:ink;
    ui_surface((Rectangle){x,28,20,20},active==tool?accent:over?ui_theme.hover:cell);
    icon(tool==PENCIL?ICON_PENCIL:tool==BRUSH?ICON_BRUSH:ICON_SELECT,x+10,38,20,c);
    if(over) snprintf(status,sizeof status,"%s",tip);
    return over && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
}
static void snap_button(int id,float x) {
    if(button(TextFormat("Snap: %s",snap_names[snap_mode[id]]),x,28,112,20,snap_menu==id+1)) {
        snap_menu=id+1; snap_opened=1;
        Rect r=windows.editors[id?2:1].rect; snap_position=(Vector2){r.x+x,r.y+50}; input_enabled=0;
    }
    if(hover(x,28,112,20)) snprintf(status,sizeof status,"Snap: choose the timing grid; Auto adjusts to the zoom level");
}
static void draw_snap(void) {
    if(!snap_menu) return;
    float scale=fminf(GetScreenWidth()/1200.f,GetScreenHeight()/675.f);
    float x=fminf(snap_position.x,GetScreenWidth()/scale-144),y=fminf(snap_position.y,GetScreenHeight()/scale-22-SNAP_COUNT*24-8);
    input_enabled=1;
    if(IsKeyPressed(KEY_ESCAPE) || (!snap_opened && IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && !hover(x,y,144,SNAP_COUNT*24+8))) { snap_menu=0; return; }
    ui_frame((Rectangle){x,y,144,SNAP_COUNT*24+8});
    int id=snap_menu-1;
    for(int i=0;i<SNAP_COUNT;i++) if(button(snap_names[i],x+4,y+4+i*24,136,24,snap_mode[id]==i) && !snap_opened) { snap_mode[id]=i; snap_menu=0; break; }
}
/* All three editors use steps internally and the same ruler appearance. */
static void timeline_ruler(int id,float start,float span,float gx,float y,float width,float q) {
    float *range=id==1?song_loop:pattern_loop[pattern],point=id==1?playlist_start*STEPS:piano_start[pattern];
    float pixels=width/span;
    DrawRectangle(gx,y,width,14,cell);
    if(range[1]>range[0]) {
        float left=fmaxf(gx,gx+(range[0]-start)*pixels),right=fminf(gx+width,gx+(range[1]-start)*pixels);
        if(right>left) DrawRectangle(left,y,right-left,14,Fade(ui_theme.loop,ui_theme.light?.35f:.6f));
    }
    float spacing=STEPS*fmaxf(1,powf(2,ceilf(log2f(48/(STEPS*pixels)))));
    for(int i=0;i<width/(spacing*pixels)+2;i++) {
        float step=(floorf(start/spacing)+i)*spacing,x=gx+(step-start)*pixels;
        if(step>=0 && x>=gx && x<gx+width-24) { DrawLine(x,y+10,x,y+14,muted); label(TextFormat("%.0f",step/STEPS+1),x+8,y+1,11,ink); }
    }
    if(hover(gx,y,width,14)) {
        snprintf(status,sizeof status,"Ruler: left-click/drag sets playback start; right-drag selects a loop; right-click clears loop; Space plays/stops");
        int right=IsMouseButtonPressed(MOUSE_BUTTON_RIGHT);
        if(right || IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            if(song!=(id==1)) { song=id==1; reset=1; }
            float at=fmaxf(0,roundf((start+(mouse.x-gx)/pixels)/q)*q);
            marker_drag=id; ruler_loop_drag=right; ruler_anchor=at;
            if(right) range[0]=range[1]=0;
            else if(id==1) playlist_start=at/STEPS; else piano_start[pattern]=at;
            if(!playing) reset=1;
            input_enabled=0;
        }
    }
    float x=gx+(point-start)*pixels;
    if(x>=gx && x<=gx+width) DrawTriangle((Vector2){x-5,y+1},(Vector2){x,y+12},(Vector2){x+5,y+1},marker_drag==id?ink:ui_theme.signal);
}
static void timeline_grid(float start,float span,float snap,float gx,float y,float width,float height) {
    float pixels=width/span,spacing=snap*fmaxf(1,ceilf(6/(snap*pixels)));
    for(int i=0;i<width/(spacing*pixels)+2;i++) {
        float step=(floorf(start/spacing)+i)*spacing,x=gx+(step-start)*pixels;
        int bar=fabsf(step/STEPS-roundf(step/STEPS))<.0001f,beat=fabsf(step/4-roundf(step/4))<.0001f;
        if(step>=0 && x>=gx && x<=gx+width) DrawLineEx((Vector2){x,y},(Vector2){x,y+height},1,bar?ui_theme.grid_major:Fade(ui_theme.grid_minor,(beat?1:.6f)*fminf(1,spacing*pixels/12)));
    }
}
static void follow_view(float *start,float span,double position) {
    if(!follow_playhead || !playing || navigation_active || captured() || windows.grab>=0) return;
    *start=position-span*.5;
}
static float track_flash[LANES];
static uint8_t track_active_ui[LANES];
static void track_activity_update(void) {
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
        else { channel=instrument_channel=source-PATTERNS; windows_focus(&windows,4); }
        picker_drag=-1; browser_focus=0; input_enabled=0;
    }
    return twice;
}
static void playlist(float width,float height,float scale) {
    static double last_click=-1;
    static int last_lane=-1,last_clip=-1;
    static Vector2 last_position;
    Rect rect=windows.editors[1].rect;
    int gx=212,gy=82; float gridw=width-gx-24,track_area=height-gy-42,total_height=track_position(LANES);
    track_scroll=fmaxf(0,fminf(total_height-track_area,track_scroll));
    float span=BARS/arrangement.zoom,barw=gridw/span;
    if(song) follow_view(&arrangement.view_start,span,visual_step/STEPS);
    arrangement.range=fmaxf(arrangement.range,fmaxf(arrangement.view_start+span*2,song_steps(&project)/STEPS+span));
    arrangement.snap=snap_interval(snap_mode[0],barw/STEPS);
    float pointer_bar=arrangement.view_start+(mouse.x-gx)/barw;
    int edge_lane=-1,edge_bar=-1,edge_side=0,divider=-1;
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
        if((edge_side=clip_edge(lane,hit,pointer_bar,barw))) { edge_lane=lane; edge_bar=hit; SetMouseCursor(MOUSE_CURSOR_RESIZE_EW); }
    }
    if(arrangement.gesture==SIZE_CLIP) { edge_lane=arrangement.lane; edge_bar=arrangement.bar; edge_side=arrangement.edge; SetMouseCursor(MOUSE_CURSOR_RESIZE_EW); }
    if(track_resize>=0) SetMouseCursor(MOUSE_CURSOR_RESIZE_NS);
    const char *tips[]={"Pencil: place one clip or drag a clip to move it","Brush: drag to paint copies of the last clicked clip","Select: drag a rectangle, then drag the selected clips together"};
    for(int i=0;i<3;i++) if(tool_button(i,arrangement.tool,8+i*24,tips[i])) arrangement.tool=i;
    snap_button(0,width-184);

    const char *picker_names[]={"Patterns","Audio clips","Automation"};
    const int picker_icons[]={ICON_PIANO,ICON_WAVE,ICON_AUTOMATION};
    for(int t=0;t<3;t++) {
        if(button("",4+t*38,57,36,23,picker_tab==t)) { picker_tab=t; picker_scroll=0; picker_drag=-1; if(t==0) { arrangement.source_pattern=pattern; arrangement.source_steps=project.pattern_steps[pattern]; arrangement.source_offset=0; } }
        icon(picker_icons[t],22+t*38,68,18,picker_tab==t?ui_theme.selected_text:ink);
        if(hover(4+t*38,57,36,23)) snprintf(status,sizeof status,"%s%s",picker_names[t],t==2?" (coming later)":"");
    }
    label("Tracks",128,63,12,muted);
    int audio_ids[CHANNELS],audio_count=0;
    for(int c=0;c<project.channel_count;c++) if(project.channel_audio[c]) audio_ids[audio_count++]=c;
    int picker_count=picker_tab==0?project.pattern_count:picker_tab==1?audio_count:0;
    int picker_visible=fmaxf(1,(height-66-gy)/52);
    if(hover(4,gy,112,height-66-gy)) picker_scroll-=GetMouseWheelMove();
    picker_scroll=fmaxf(0,fminf(fmaxf(0,picker_count-picker_visible),picker_scroll));
    for(int row=picker_scroll;row<picker_count;row++) {
        int p=picker_tab==0?row:audio_ids[row];
        int y=gy+(row-picker_scroll)*52; if(y+50>height-66) break;
        if(picker_tab==1) {
            if(picker_button(y,audio_color(p),arrangement.source_pattern==PATTERNS+p)) {
                arrangement.source_pattern=PATTERNS+p; arrangement.source_steps=project.audio_seconds[p]; arrangement.source_offset=0; picker_drag=PATTERNS+p;
                picker_origin=(Vector2){mouse.x+rect.x,mouse.y+rect.y};
                picker_offset=(Vector2){mouse.x-4,mouse.y-y};
                picker_double_click(PATTERNS+p);
            }
            if(arrangement.source_pattern==PATTERNS+p) DrawRectangleLinesEx((Rectangle){4,y,112,50},2,accent);
            label(fit_text(project.channel_names[p],100,12),8,y+4,12,ink);
            audio_waveform(p,8,y+20,8,112,104/fmaxf(.001f,clip_source_steps(&project,PATTERNS+p)),28);
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
        label(fit_text(project.pattern_names[p],100,12),8,y+4,12,ink);
        picker_notes(p,4,y);
        if(hover(4,y,112,50)) {
            snprintf(status,sizeof status,"Pattern: double-click for Channel Rack; select or drag to place; right-click for Rename, Color or Delete");
            if(IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) { pattern=p; arrangement.source_pattern=p; arrangement.source_steps=project.pattern_steps[p]; reset=1; open_context(4,p,(Vector2){mouse.x+rect.x,mouse.y+rect.y}); }
        }
    }
    if(sample_drag[0] && sample_moved && CheckCollisionPointRec(mouse,(Rectangle){4,gy,112,height-gy-42}) && windows_hit(&windows,mouse.x+rect.x,mouse.y+rect.y)==1) {
        DrawRectangleLinesEx((Rectangle){4,gy,112,height-gy-42},2,ui_theme.signal);
        snprintf(status,sizeof status,"Release to import into the Audio list without placing a clip");
    }
    if(picker_tab==2) label("Coming later",8,gy+8,12,muted);
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
    zoom_label(arrangement.zoom*100,width-62,35);
    timeline_ruler(1,arrangement.view_start*STEPS,span*STEPS,gx,68,gridw,arrangement.snap);
    for(int l=fmaxf(0,track_at(track_scroll));l<LANES;l++) {
        float y=gy+track_position(l)-track_scroll,rowh=track_height(l); int selected=0;
        if(y>=height-42) break;
        for(int b=0;b<CLIPS;b++) if(arrangement.selected[l][b]) selected=1;
        BeginScissorMode(rect.x*scale,(rect.y+gy)*scale,gx*scale,track_area*scale);
        DrawRectangle(120,y,gx-122,rowh-1,selected?Fade(ui_theme.signal,.35f):cell); label(fit_text(project.track_names[l],gx-132,12),128,y+8,12,ink);
        float brightness=fmaxf(track_active_ui[l]?.32f:0,track_flash[l]);
        if(brightness>.005f) DrawRectangle(gx-5,y+1,4,rowh-2,(Color){255,255,255,(unsigned char)(brightness*255)});
        mute_light(gx-12,y+rowh-10,project.lane_mute,LANES,l,TextFormat("Playlist track %d",l+1));
        BeginScissorMode((rect.x+gx)*scale,(rect.y+gy)*scale,gridw*scale,track_area*scale);
        DrawRectangle(gx,y,gridw,rowh,ui_glass(ui_theme.track,48));
        timeline_grid(arrangement.view_start*STEPS,span*STEPS,arrangement.snap,gx,y,gridw,rowh-1);
        for(int b=0;b<CLIPS;b++) if(project.clips[l][b]) {
            int pat=project.clips[l][b]-1; float x=gx+(project.clip_starts[l][b]-arrangement.view_start)*barw,w=clip_length(&project,l,b)*barw/STEPS;
            if(w<=0 || x>gx+gridw || x+w<gx) continue;
            float left=fmaxf(gx,x),right=fminf(gx+gridw,x+w-1);
            DrawRectangleRec((Rectangle){left,y,fmaxf(1,right-left),rowh-1},pat>=PATTERNS?audio_color(pat-PATTERNS):pattern_color(pat)); DrawRectangleRec((Rectangle){left,y,fmaxf(1,right-left),14},Fade(WHITE,.08f));
            const char *name=pat>=PATTERNS?project.channel_names[pat-PATTERNS]:project.pattern_names[pat]; if(pat<PATTERNS && !strcmp(name,TextFormat("Pattern %d",pat+1))) name=TextFormat("P%d",pat+1);
            if(x>=gx) label(fit_text(name,fminf(gridw,w-9),10),x+3,y+2,10,ink);
            float offset=clip_offset_steps(&project,l,b),origin=x-offset*barw/STEPS;
            if(pat>=PATTERNS) audio_waveform(pat-PATTERNS,origin,y,left,right,barw/STEPS,rowh);
            else clip_preview(pat,origin,y,left,right,barw/STEPS,clip_length(&project,l,b)+offset,rowh);
            if(x>=gx && x<=gx+gridw) DrawRectangle(x+2,y+15,3,rowh-18,edge_lane==l && edge_bar==b && edge_side<0?accent:Fade(ink,.45f));
            if(x+w>=gx && x+w<=gx+gridw) DrawRectangle(x+w-5,y+15,3,rowh-18,edge_lane==l && edge_bar==b && edge_side>0?accent:Fade(ink,.45f));
            if(arrangement.gesture==MOVE_CLIPS?arrangement.moved[l][b]:arrangement.selected[l][b]) DrawRectangleLinesEx((Rectangle){left+1,y+1,fmaxf(1,right-left-2),rowh-3},2,ui_theme.signal);
        }
        BeginScissorMode(rect.x*scale,rect.y*scale,rect.w*scale,rect.h*scale);
    }
    BeginScissorMode((rect.x+gx)*scale,(rect.y+gy)*scale,gridw*scale,track_area*scale);
    for(int l=fmaxf(0,track_at(track_scroll));l<LANES;l++) {
        float y=gy+track_position(l)-track_scroll,rowh=track_height(l);
        if(y>=height-42) break;
        float bottom=roundf((y+rowh)*scale+text_origin.y);
        DrawRectangleRec((Rectangle){gx,(bottom-1-text_origin.y)/scale,gridw,1/scale},ui_theme.border);
        BeginScissorMode((rect.x+120)*scale,(rect.y+gy)*scale,(gx-120)*scale,track_area*scale);
        DrawRectangleRec((Rectangle){120,(bottom-1-text_origin.y)/scale,gx-120,1/scale},divider==l || track_resize==l?accent:ui_theme.border);
        BeginScissorMode((rect.x+gx)*scale,(rect.y+gy)*scale,gridw*scale,track_area*scale);
    }
    BeginScissorMode(rect.x*scale,rect.y*scale,rect.w*scale,rect.h*scale);
    if(picker_drag>=0 && IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
        if(CheckCollisionPointRec(mouse,(Rectangle){gx,gy,gridw,track_area}) && windows_hit(&windows,mouse.x+rect.x,mouse.y+rect.y)==1) {
            float start=floorf(pointer_bar*STEPS/arrangement.snap)*arrangement.snap/STEPS;
            arrangement_place(&project,track_at(track_scroll+mouse.y-gy),start,picker_drag,clip_source_steps(&project,picker_drag));
        }
        picker_drag=-1;
    }
    if(divider<0 && hover(120,gy,gx+gridw-120,track_area)) {
        float bx=mouse.x<gx?-1:arrangement.view_start+(mouse.x-gx)/barw,ly=track_at(track_scroll+mouse.y-gy); int hit=arrangement_hit(&project,(int)ly,bx);
        int edge=clip_edge((int)ly,hit,bx,barw);
        if(edge) { SetMouseCursor(MOUSE_CURSOR_RESIZE_EW); snprintf(status,sizeof status,"Drag this clip edge to resize it"); }
        else if(mouse.x<gx) snprintf(status,sizeof status,"Track %d: drag to select tracks; right-click to rename",(int)ly+1);
        else if(hit>=0) snprintf(status,sizeof status,project.clips[(int)ly][hit]>PATTERNS?"Audio clip: double-click to open sampler; drag to move; right-click to erase":"Pattern clip: double-click to open Channel Rack; drag to move; right-click to erase");
        else snprintf(status,sizeof status,"Playlist: %s; Shift adds to selection, right-drag erases",arrangement.tool==PENCIL?"place or move a clip":arrangement.tool==BRUSH?"paint copies of the current pattern":"select and move clips");
        if(mouse.x<gx && IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
            rename_channel=rename_mixer=-1; rename_track=(int)ly; snprintf(rename_text,sizeof rename_text,"%s",project.track_names[rename_track]); rename_select_all=1; open_popup(2); input_enabled=0; return;
        }
        if(IsMouseButtonPressed(MOUSE_BUTTON_LEFT) || IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
            int plain_left=IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && !IsKeyDown(KEY_LEFT_SHIFT) && !IsKeyDown(KEY_RIGHT_SHIFT) && hit>=0 && (!edge || project.clips[(int)ly][hit]>PATTERNS);
            double now=GetTime();
            if(plain_left && last_lane==(int)ly && last_clip==hit && now-last_click<.35 && fabsf(mouse.x-last_position.x)<5 && fabsf(mouse.y-last_position.y)<5) {
                int source=project.clips[(int)ly][hit]-1;
                if(source<PATTERNS) { pattern=source; reset=1; }
                else { channel=instrument_channel=source-PATTERNS; rack_filter=1; rack_scroll=0; }
                arrangement.gesture=IDLE; windows_focus(&windows,source>=PATTERNS?4:0); last_click=-1;
                input_enabled=0; return;
            }
            last_click=plain_left?now:-1; last_lane=(int)ly; last_clip=hit; last_position=mouse;
            arrangement_press(&arrangement,&project,bx,ly,IsMouseButtonPressed(MOUSE_BUTTON_RIGHT),edge,pattern,IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT));
            if(arrangement.source_pattern>=0 && arrangement.source_pattern<PATTERNS && arrangement.source_pattern!=pattern) { pattern=arrangement.source_pattern; reset=1; }
            input_enabled=0;
        }
    }
    BeginScissorMode((rect.x+gx)*scale,(rect.y+gy)*scale,gridw*scale,(height-gy-42)*scale);
    if(picker_drag>=0 && CheckCollisionPointRec(mouse,(Rectangle){gx,gy,gridw,track_area}) && windows_hit(&windows,mouse.x+rect.x,mouse.y+rect.y)==1) {
        float start=floorf(pointer_bar*STEPS/arrangement.snap)*arrangement.snap/STEPS;
        int lane=track_at(track_scroll+mouse.y-gy);
        Rectangle ghost={gx+(start-arrangement.view_start)*barw,gy+track_position(lane)-track_scroll,fmaxf(4,clip_source_steps(&project,picker_drag)*barw/STEPS),track_height(lane)-1};
        DrawRectangleRec(ghost,Fade(ui_theme.signal,.2f)); DrawRectangleLinesEx(ghost,1,ui_theme.signal);
        snprintf(status,sizeof status,"Release to place %s",picker_drag<PATTERNS?project.pattern_names[picker_drag]:project.channel_names[picker_drag-PATTERNS]);
    }
    if(sample_drag[0] && sample_moved && mouse.x>=gx && mouse.x<gx+gridw && mouse.y>=gy && mouse.y<height-42 && windows_hit(&windows,mouse.x+rect.x,mouse.y+rect.y)==1) {
        float start=floorf(pointer_bar*STEPS/arrangement.snap)*arrangement.snap/STEPS;
        float length=audition.frames/(float)RATE*project.bpm/15;
        int lane=fminf(LANES-1,fmaxf(0,track_at(track_scroll+mouse.y-gy)));
        Rectangle ghost={gx+(start-arrangement.view_start)*barw,gy+track_position(lane)-track_scroll,fmaxf(4,length*barw/STEPS),track_height(lane)-1};
        DrawRectangleRec(ghost,Fade(ui_theme.signal,.2f)); DrawRectangleLinesEx(ghost,1,ui_theme.signal);
    }
    if(arrangement.gesture==BOX_SELECT) {
        float x=gx+(fminf(arrangement.x,arrangement.now_x)-arrangement.view_start)*barw,y=gy+track_position(fminf(arrangement.y,arrangement.now_y))-track_scroll;
        Rectangle box={x,y,fabsf(arrangement.x-arrangement.now_x)*barw,fabsf(track_position(arrangement.y)-track_position(arrangement.now_y))};
        DrawRectangleRec(box,Fade(ui_theme.signal,.15f)); DrawRectangleLinesEx(box,1,ui_theme.signal);
    }
    BeginScissorMode(rect.x*scale,rect.y*scale,rect.w*scale,rect.h*scale);
    if(playing && song) { float x=gx+(visual_step/STEPS-arrangement.view_start)*barw; if(x>=gx && x<=gx+gridw) DrawLineEx((Vector2){x,gy},(Vector2){x,height-42},1.5f,ui_theme.signal); }
    float vthumb=timeline_thumb(track_area,track_area,total_height),travel_y=track_area-vthumb,maximum_y=fmaxf(0,total_height-track_area),vy=gy+(maximum_y>0?track_scroll/maximum_y*travel_y:0);
    DrawRectangle(width-18,gy,12,track_area,bg); ui_surface((Rectangle){width-18,vy,12,fmaxf(12,vthumb)},playlist_vpan || hover(width-18,gy,12,track_area)?accent:muted);
    if(hover(width-18,gy,12,track_area)) snprintf(status,sizeof status,"Drag to scroll through the 100 Playlist tracks");
    if(hover(width-18,gy,12,track_area) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        if(mouse.y<vy || mouse.y>vy+vthumb) track_scroll=fmaxf(0,fminf(maximum_y,(mouse.y-gy-vthumb/2)/fmaxf(1,travel_y)*maximum_y));
        playlist_vpan=1; playlist_vpan_y=mouse.y+rect.y; playlist_vpan_start=track_scroll;
    }
    float thumbw=timeline_thumb(gridw,span,arrangement.range),travel=gridw-thumbw,maximum=arrangement.range-span,thumbx=gx+(maximum>0?fmaxf(0,arrangement.view_start)/maximum*travel:0);
    DrawRectangle(gx,height-37,gridw,13,bg); ui_surface((Rectangle){thumbx,height-37,thumbw,13},playlist_pan || hover(gx,height-37,gridw,13)?accent:muted);
    if(hover(gx,height-37,gridw,13)) snprintf(status,sizeof status,"Drag to scroll along the Playlist timeline");
    if(hover(gx,height-37,gridw,13) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        if(mouse.x<thumbx || mouse.x>thumbx+thumbw) arrangement.view_start=fmaxf(0,fminf(maximum,(mouse.x-gx-thumbw/2)/fmaxf(1,travel)*maximum));
        playlist_pan=1; playlist_pan_x=mouse.x+rect.x; playlist_pan_start=arrangement.view_start; playlist_pan_range=maximum/fmaxf(1,travel);
    }
    label(fit_text("Wheel: scroll | Cmd/Ctrl+wheel: zoom | Shift: horizontal / add selection | Right-drag: erase | Delete: remove",width-24,11),8,height-17,11,muted);

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
        float dx=step-piano_now.x,dy=pitch-piano_now.y;
        int count=fminf(NOTES*2,ceilf(fmaxf(fabsf(dx)/q,fabsf(dy))*2)); if(count<1) count=1;
        for(int j=0;j<=count;j++) {
            float t=j/(float)count,at=piano_now.x+dx*t; int key=ceilf(piano_now.y+dy*t);
            if(at<0 || piano_inactive(at) || key<0 || key>127) continue;
            if(piano_gesture==PIANO_ERASE) {
                for(int i=0;i<NOTES;i++) {
                    Note *n=&project.notes[pattern][channel][i];
                    if(n->velocity && n->pitch==key && n->start<=at && n->start+(n->length?n->length:1)>at) { n->velocity=0; selected[i]=0; }
                }
            } else {
                float start=floorf(at/q)*q;
                piano_extend(start+q);
                int exists=note_at(&project,pattern,channel,start,key)!=NULL;
                Note *n=note_add(&project,pattern,channel,start,key,fminf(q,edit_steps()-start));
                if(n && !exists) { audio_note(channel,*n); piano_channels[pattern][channel]=1; }
            }
        }
    }
    piano_now=(Vector2){step,pitch};
}
static void piano_key_update(float x,float y,int cancel) {
    Rect r=windows.editors[2].rect; float rh=(r.h-166)/25;
    int pitch=x>=4 && x<58 && y>=72 && y<72+rh*25?piano_top-(int)((y-72)/rh):-1;
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
    int gx=60,gy=72; float gridw=width-84,rh=(height-166)/25;
    float extent=piano_span[pattern];
    Rect r=windows.editors[2].rect; float scale=fminf(GetScreenWidth()/1200.f,GetScreenHeight()/675.f);
    const char *tips[]={"Pencil: draw one note; drag note bodies to move; edges to resize","Brush: drag to paint notes on the snap grid","Select: drag a rectangle; drag selected notes together; Shift adds; Supr deletes"};
    for(int i=0;i<3;i++) if(tool_button(i,piano_tool,8+i*24,tips[i])) piano_tool=i;
    label(fit_text(TextFormat("%s / %s",project.channel_names[channel],project.pattern_names[pattern]),width-344,12),88,30,12,muted);
    zoom_label(STEPS/piano_span[pattern]*100,width-205,32);
    snap_button(1,width-120);
    int grid_over=hover(gx,gy,gridw,rh*25);
    extent=piano_span[pattern];
    if(!song) follow_view(&piano_pan[pattern],extent,visual_step);
    float cw=gridw/extent,view=piano_pan[pattern],q=snap_interval(snap_mode[1],cw),end=edit_steps();
    piano_range[pattern]=fmaxf(piano_range[pattern],fmaxf(view+extent*2,end+extent));
    float partial=ceilf(end/STEPS)*STEPS;
    if(hover(4,gy,gx-6,rh*25)) {
        snprintf(status,sizeof status,"Piano keys: click to play; hold and drag up/down to audition pitches");
        if(IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            piano_key_drag=1; piano_key_channel=channel; piano_key_update(mouse.x,mouse.y,0); input_enabled=0;
        }
    }
    DrawRectangle(4,gy,gx-6,rh*25,ui_theme.piano_white);
    for(int row=0;row<25;row++) {
        int pitch=piano_top-row,pc=pitch%12,y=gy+row*rh;
        if(pc==1 || pc==3 || pc==6 || pc==8 || pc==10) DrawRectangle(4,y,34,rh-1,ui_theme.selected_text);
        else DrawLine(4,y,gx-2,y,muted);
        if(keyboard_notes[channel][pitch] || (piano_key==pitch && piano_key_channel==channel)) DrawRectangle(4,y,gx-6,rh-1,accent);
        else if(hover(4,y,gx-6,rh-1)) DrawRectangle(4,y,gx-6,rh-1,Fade(accent,.22f));
        if(pc==0) label(TextFormat("C%d",pitch/12-1),10,y+1,10,ui_theme.selected_text);
    }
    BeginScissorMode((r.x+gx)*scale,(r.y+gy)*scale,gridw*scale,rh*25*scale);
    for(int row=0;row<25;row++) {
        int pc=(piano_top-row)%12,y=gy+row*rh,black=pc==1 || pc==3 || pc==6 || pc==8 || pc==10;
        DrawRectangle(gx,y,gridw,rh,ui_glass(black?bg:cell,48));
        DrawLine(gx,y,gx+gridw,y,ui_theme.grid_minor);
        if(partial>end) { float left=fmaxf(gx,gx+(end-view)*cw),right=fminf(gx+gridw,gx+(partial-view)*cw); if(right>left) DrawRectangle(left,y,right-left,rh-1,ui_theme.disabled); }
        if(pc==0) DrawLine(gx,y,gx+gridw,y,ui_theme.grid_major);
    }
    timeline_grid(view,extent,q,gx,gy,gridw,rh*25);
    Note *hit=NULL;
    for(int i=0;i<NOTES;i++) {
        Note *n=&project.notes[pattern][channel][i];
        if(!n->velocity || n->pitch<piano_top-24 || n->pitch>piano_top || n->start>=end || n->start+(n->length?n->length:1)<=view) continue;
        float note_end=fminf(n->start+(n->length?n->length:1),end);
        float x=gx+(n->start-view)*cw,y=gy+(piano_top-n->pitch)*rh,w=(note_end-n->start)*cw;
        float left=fmaxf(gx,x),right=fminf(gx+gridw,x+w);
        if(right<=left) continue;
        if(note_selected[pattern][channel][i]) DrawRectangleLinesEx((Rectangle){left,y,right-left,rh},1,ink);
        DrawRectangle(left+1,y+1,fmaxf(1,right-left-2),fmaxf(2,rh-2),n==note_drag || keyboard_notes[channel][n->pitch]?ui_theme.note_drag:ui_theme.note);
        DrawRectangleLinesEx((Rectangle){left+1,y+1,fmaxf(1,right-left-2),fmaxf(2,rh-2)},1,Fade(ui_theme.signal,.45f));
        int edge=grid_over && hover(x+w-7,y,7,rh);
        if(x+w>=gx && x+w<=gx+gridw) DrawRectangle(x+w-5,y+1,3,fmaxf(2,rh-2),edge || (n==note_drag && !moving_note)?ink:Fade(bg,.35f));
        if(edge || (n==note_drag && !moving_note)) SetMouseCursor(MOUSE_CURSOR_RESIZE_EW);
        if(grid_over && hover(x,y,w,rh)) hit=n;
    }
    if(grid_over) {
        float start=floorf((view+(mouse.x-gx)/cw)/q)*q;
        snprintf(status,sizeof status,"Piano Roll: %s | Shift: add selection | Right-drag: erase | Supr: delete selected notes | Wheel: zoom",piano_tool==PENCIL?"draw or move notes":piano_tool==BRUSH?"paint notes":"select or move notes");
        if(piano_inactive(start)) snprintf(status,sizeof status,"Inactive steps: extend the Playlist clip to enable them");
        else if(IsMouseButtonPressed(MOUSE_BUTTON_LEFT) || IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
            uint8_t *selected=note_selected[pattern][channel];
            Vector2 point={view+(mouse.x-gx)/cw,piano_top-(mouse.y-gy)/rh};
            int shift=IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT),group=0;
            for(int i=0;i<NOTES;i++) group+=selected[i]!=0;
            piano_from=piano_now=point; piano_additive=shift; memcpy(note_selection_before,selected,NOTES);
            if(IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) piano_gesture=PIANO_ERASE;
            else if(shift && hit) selected[hit-project.notes[pattern][channel]]^=1;
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
                    piano_extend(start+q);
                    note_drag=note_add(&project,pattern,channel,start,ceilf(point.y),q);
                }
                if(!note_drag) snprintf(status,sizeof status,"No room here: remove a note (limit %d).",NOTES);
                else { audio_note(channel,*note_drag); piano_channels[pattern][channel]=1; }
            }
            if(piano_gesture) piano_gesture_update(point.x,point.y,q);
            input_enabled=0;
        }
    }
    if(piano_gesture==PIANO_BOX) {
        float left=fminf(piano_from.x,piano_now.x),high=fmaxf(piano_from.y,piano_now.y);
        Rectangle box={gx+(left-view)*cw,gy+(piano_top-high)*rh,fabsf(piano_from.x-piano_now.x)*cw,fabsf(piano_from.y-piano_now.y)*rh};
        DrawRectangleRec(box,Fade(ui_theme.signal,.15f)); DrawRectangleLinesEx(box,1,ui_theme.signal);
    }

    int vy=height-84; label("Velocity",6,vy+4,10,muted);
    BeginScissorMode((r.x+gx)*scale,(r.y+vy)*scale,gridw*scale,62*scale);
    DrawRectangle(gx,vy,gridw,62,cell);
    if(partial>end) { float left=fmaxf(gx,gx+(end-view)*cw),right=fminf(gx+gridw,gx+(partial-view)*cw); if(right>left) DrawRectangle(left,vy,right-left,62,ui_theme.disabled); }
    for(int i=0;i<NOTES;i++) {
        Note n=project.notes[pattern][channel][i]; if(!n.velocity || n.start>=end) continue;
        float x=gx+(n.start-view)*cw;
        if(x<gx-10 || x>gx+gridw) continue;
        DrawRectangle(x+1,vy+62-n.velocity*.48f,fmaxf(2,fminf(cw*q-2,10)),n.velocity*.48f,ui_theme.note);
        if(hover(gx,vy,gridw,62) && hover(x,vy,fmaxf(5,cw*q),62)) {
            snprintf(status,sizeof status,"Velocity: drag to set loudness of notes at this position");
            if(IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                velocity_drag=n.start; input_enabled=0;
                int value=fmaxf(1,fminf(127,(vy+62-mouse.y)/.48f));
                for(int j=0;j<NOTES;j++) { Note *other=&project.notes[pattern][channel][j]; if(other->velocity && fabsf(other->start-n.start)<.00001f) other->velocity=value; }
            }
        }
    }
    BeginScissorMode(r.x*scale,r.y*scale,r.w*scale,r.h*scale);
    float varea=rh*25,vthumb=varea*25/128,vypos=gy+(127-piano_top)/128.f*varea;
    DrawRectangle(width-18,gy,12,varea,bg);
    ui_surface((Rectangle){width-18,vypos,12,vthumb},piano_vdrag || hover(width-18,gy,12,varea)?accent:muted);
    if(hover(width-18,gy,12,varea)) {
        snprintf(status,sizeof status,"Drag to scroll higher or lower pitches");
        if(IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            if(mouse.y<vypos || mouse.y>vypos+vthumb) piano_top=127-(int)fmaxf(0,fminf(103,(mouse.y-gy-vthumb/2)/varea*128));
            piano_vscroll_y=mouse.y+r.y; piano_vscroll_start=127-piano_top; piano_vdrag=1; input_enabled=0;
        }
    }
    float thumb=timeline_thumb(gridw,extent,piano_range[pattern]),travel=gridw-thumb,maximum=piano_range[pattern]-extent,x=gx+(maximum>0?fmaxf(0,piano_pan[pattern])/maximum*travel:0);
    DrawRectangle(gx,height-18,gridw,8,bg); ui_surface((Rectangle){x,height-18,thumb,8},piano_scroll_drag || hover(gx,height-21,gridw,14)?accent:muted);
    if(hover(gx,height-21,gridw,14)) {
        snprintf(status,sizeof status,"Drag to pan the zoomed Piano Roll; wheel over notes to zoom; the timeline grows as you navigate");
        if(IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            if(mouse.x<x || mouse.x>x+thumb) piano_pan[pattern]=fmaxf(0,fminf(maximum,(mouse.x-gx-thumb/2)/fmaxf(1,travel)*maximum));
            piano_scroll_x=mouse.x+r.x; piano_scroll_start=piano_pan[pattern]; piano_scroll_range=maximum/fmaxf(1,travel); piano_scroll_drag=1; input_enabled=0;
        }
    }
    timeline_ruler(2,view,extent,gx,58,gridw,q);
    if(playing && !song) {
        float x=gx+(visual_step-view)*cw;
        if(x>=gx && x<=gx+gridw) DrawLineEx((Vector2){x,gy},(Vector2){x,gy+rh*25},1.5f,ui_theme.signal);
    }

}
static int mixer_input_at(Vector2 point) {
    Rect r=windows.editors[3].rect; if(!windows.editors[3].visible) return -1;
    int capacity=fmaxf(1,(r.w-220-MIXER_LEFT-8)/51-1),visible=fminf(project.insert_count,capacity);
    for(int col=0;col<=visible;col++) if(CheckCollisionPointRec(point,(Rectangle){r.x+MIXER_LEFT+col*51+7,r.y+r.h-46,12,12})) return col?col+mixer_scroll:0;
    return -1;
}
static float meter_level[INSERTS+1][2],meter_hold[INSERTS+1][2];
static double meter_until[INSERTS+1][2],meter_time;
static uint8_t record_armed[INSERTS+1];
static int mixer_decay_active,mixer_was_visible;
static void meters_update(void) {
    float peaks[INSERTS+1][2]; audio_meters(peaks);
    double now=GetTime(),elapsed=fmax(0,now-meter_time); meter_time=now;
    if(!mixer_was_visible) { memset(meter_level,0,sizeof meter_level); memset(meter_hold,0,sizeof meter_hold); memset(peaks,0,sizeof peaks); }
    float release=expf(-elapsed/.18),peak_release=expf(-elapsed/.45); mixer_decay_active=0;
    for(int id=0;id<=INSERTS;id++) for(int side=0;side<2;side++) {
        meter_level[id][side]=fmaxf(peaks[id][side],meter_level[id][side]*release);
        if(peaks[id][side]>=meter_hold[id][side] && peaks[id][side]>.00001f) { meter_hold[id][side]=peaks[id][side]; meter_until[id][side]=now+.5; }
        else if(now>meter_until[id][side]) meter_hold[id][side]*=peak_release;
        mixer_decay_active|=meter_hold[id][side]>.001f || meter_level[id][side]>.001f;
    }
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
    meters_update(); selected_meter(height);
    int capacity=fmaxf(1,(width-220-MIXER_LEFT-8)/51-1),visible=project.insert_count<capacity?project.insert_count:capacity;
    mixer_scroll=(int)fmaxf(0,fminf(fmaxf(0,project.insert_count-visible),mixer_scroll));
    if(hover(0,TITLE,width-220,height-TITLE) && mouse.y>TITLE+60) {
        mixer_scroll=(int)fmaxf(0,fminf(fmaxf(0,project.insert_count-visible),mixer_scroll-GetMouseWheelMove()));
    }
    float fader_top=TITLE+76,fader_bottom=height-74;
    for(int col=0;col<=visible;col++) {
        int id=col?col+mixer_scroll:0,x=MIXER_LEFT+col*51;
        float *v=id?&project.insert_volume[id-1]:&project.master;
        int over=hover(x,TITLE+4,48,height-TITLE-12);
        ui_surface((Rectangle){x,TITLE+4,48,height-TITLE-12},ui_glass(mixer_selected==id?ui_theme.title_focus:cell,70));
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
        if(hover(x+23,fader_top-4,22,fader_bottom-fader_top+8) && IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) begin_number(v,0,MIXER_GAIN_MAX,1,id?"Insert volume":"Master volume");
        if(hover(x+23,fader_top-4,22,fader_bottom-fader_top+8) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            *v=fader_gain(fmaxf(0,fminf(1,(fader_bottom-mouse.y)/(fader_bottom-fader_top))));
            capture_control(v,0,MIXER_GAIN_MAX,1); control_bottom=fader_bottom+windows.editors[3].rect.y; control_range=fader_bottom-fader_top;
        }
        if(hover(x+23,fader_top-4,22,fader_bottom-fader_top+8) || control_drag==v) snprintf(status,sizeof status,"%s fader: %.1f dB | Mark = 0 dB; right-click for linear gain (1 = 0 dB)",mixer_name(id),gain_db(*v));
        DrawLine(x+25,fader_bottom-fader_position(1)*(fader_bottom-fader_top),x+44,fader_bottom-fader_position(1)*(fader_bottom-fader_top),muted);
        int fy=fader_bottom-fader_position(*v)*(fader_bottom-fader_top);
        ui_surface((Rectangle){x+26,fy-9,14,18},mixer_selected==id?accent:ui_theme.piano_white);
        DrawLine(x+28,fy,x+38,fy,bg);
        knob_style(x+33,height-59,id?&project.insert_width[id-1]:&project.master_width,0,2,1,"Stereo width (left wider, center unchanged, right mono)",KNOB_WIDTH);
        int arm_over=hover(x+4,height-67,16,16);
        circle(x+12,height-59,6,arm_over || record_armed[id]?ui_theme.meter_high:ui_theme.border);
        circle(x+12,height-59,5,record_armed[id]?ui_theme.meter_high:cell);
        if(arm_over) {
            snprintf(status,sizeof status,"%s record arm: %s | Indicator only; recording is not implemented",mixer_name(id),record_armed[id]?"On":"Off");
            if(IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) { record_armed[id]^=1; input_enabled=0; }
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
    int fx=width-220;
    DrawRectangle(fx,TITLE,220,height-TITLE,ui_glass(ui_theme.effects,80)); DrawLine(fx,TITLE,fx,height,cell);
    device_button(0,fx+8,TITLE+6,204);
    int slot_h=fmaxf(20,fminf(24,(height-TITLE-80)/10));
    for(int slot=0;slot<10;slot++) {
        int y=TITLE+34+slot*slot_h,cy=y+slot_h/2;
        ui_surface((Rectangle){fx+8,y,204,slot_h},hover(fx+8,y,204,slot_h)?ui_theme.hover:cell);
        label(TextFormat("Slot %d",slot+1),fx+16,cy-6,12,muted);
        if(hover(fx+8,y,204,slot_h)) snprintf(status,sizeof status,"Slot %d: empty; wet/dry and bypass are reserved for future effects",slot+1);
        knob_style(fx+178,cy,&project.effect_mix[mixer_selected][slot],0,1,1,TextFormat("Slot %d wet/dry",slot+1),KNOB_WET);
        int on=!project.effect_bypass[mixer_selected][slot];
        circle(fx+202,cy,6,ui_theme.border); circle(fx+202,cy,4,on?ui_theme.meter_low:muted);
        if(hover(fx+193,cy-9,18,18)) {
            snprintf(status,sizeof status,"Slot %d: %s; click to %s (empty slot)",slot+1,on?"enabled":"bypassed",on?"bypass":"enable");
            if(IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) project.effect_bypass[mixer_selected][slot]^=1;
        }
    }
    /* Draw separators last, snapped to one physical pixel at every UI scale. */
    float left=roundf((fx+8)*font_scale+text_origin.x),right=roundf((fx+212)*font_scale+text_origin.x);
    for(int slot=0;slot<10;slot++) {
        float bottom=roundf((TITLE+34+(slot+1)*slot_h)*font_scale+text_origin.y);
        DrawRectangleRec((Rectangle){(left-text_origin.x)/font_scale,(bottom-1-text_origin.y)/font_scale,(right-left)/font_scale,1/font_scale},bg);
    }
    device_button(1,fx+8,height-42,204);
    float area=fx-MIXER_LEFT-8,thumb=area*visible/INSERTS,pos=MIXER_LEFT+area*mixer_scroll/INSERTS;
    DrawRectangle(MIXER_LEFT,height-17,area,9,bg); ui_surface((Rectangle){pos,height-17,fmaxf(12,thumb),9},mixer_pan || hover(MIXER_LEFT,height-20,area,14)?accent:muted);
    if(hover(MIXER_LEFT,height-20,area,14)) {
        snprintf(status,sizeof status,"Mixer: wheel over inserts or drag this scrollbar to browse all 100 inserts");
        if(IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) { mixer_pan=1; mixer_scroll=fmaxf(0,fminf(INSERTS-visible,(mouse.x-MIXER_LEFT-thumb/2)/area*INSERTS)); input_enabled=0; }
    }
}
static void delete_pattern(int target) {
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
    float scale=fminf(GetScreenWidth()/1200.f,GetScreenHeight()/675.f);
    int w=228,h=context_kind==2?108:(context_kind==5 || context_kind==8)?76:context_kind==3 && !context_target?32:82,x=fmaxf(0,fminf(GetScreenWidth()/scale-w,context_position.x)),y=fmaxf(0,fminf(GetScreenHeight()/scale-h,context_position.y));
    input_enabled=1;
    if(IsKeyPressed(KEY_ESCAPE) || (!context_opened && IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && !hover(x,y,w,h))) { context_kind=0; return; }
    ui_frame((Rectangle){x,y,w,h});
    int target=context_target,kind=context_kind;
    if(kind==6) {
        for(int i=0;i<3;i++) if(button(rack_filters[i],x+4,y+4+i*25,w-8,24,rack_filter==i)) { rack_filter=i; rack_scroll=0; context_kind=0; }
    } else if(kind==5 || kind==8) {
        uint32_t *color=kind==8?&project.channel_colors[target]:&project.pattern_colors[target];
        for(int i=0;i<PATTERNS;i++) {
            int bx=x+6+(i%4)*54,by=y+6+(i/4)*32;
            if(button_color("",bx,by,48,26,0,pattern_rgb(pattern_palette[i]))) { *color=pattern_palette[i]; context_kind=0; }
            if(*color==pattern_palette[i]) DrawRectangleLinesEx((Rectangle){bx,by,48,26},2,accent);
            if(hover(bx,by,48,26)) snprintf(status,sizeof status,"Choose this source color");
        }
    } else if(kind==7) {
        if(button("Rename",x+4,y+4,w-8,24,0)) { rename_track=rename_mixer=-1; rename_channel=target; snprintf(rename_text,sizeof rename_text,"%s",project.channel_names[target]); rename_select_all=1; open_popup(2); context_kind=0; }
        if(button("Color",x+4,y+29,w-8,24,0)) { context_kind=8; context_opened=1; }
        if(button("Delete",x+4,y+54,w-8,24,0)) { picker_drag=-1; delete_channel(target); context_kind=0; }
        if(hover(x+4,y+54,w-8,24)) snprintf(status,sizeof status,"Delete this Audio channel and all its Playlist clips; the source file stays on disk");
    } else if(kind==4) {
        if(button("Rename",x+4,y+4,w-8,24,0)) { pattern=target; begin_rename(); context_kind=0; }
        if(button("Color",x+4,y+29,w-8,24,0)) { context_kind=5; context_opened=1; }
        if(button("Delete",x+4,y+54,w-8,24,0)) { delete_pattern(target); context_kind=0; }
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
static void sampler_switch(int x,int y,int width,const char *name,uint8_t *flags,int bit) {
    int over=hover(x,y,width,22),active=*flags&bit;
    if(over) ui_surface((Rectangle){x,y,width,22},ui_theme.hover);
    circle(x+9,y+11,7,over?ink:ui_theme.border); circle(x+9,y+11,5,cell);
    if(active) circle(x+9,y+11,3,accent);
    label(name,x+24,y+4,13,ink);
    if(over && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) *flags^=bit;
}
static void sampler(float width,float height) {
    int c=instrument_channel;
    Sampler *settings=&project.sampler[c];
    label(fit_text(project.channel_names[c],width-240,14),12,30,14,ink);
    channel_controls(c,width);
    label(fit_text(!originals[c].frames?"Drop a sample from the Browser":project.paths[c][0]?GetFileName(project.paths[c]):"Built-in sample",width-240,12),12,54,12,muted);
    float right=width/2+6,groupw=width/2-18;
    Rectangle groups[]={{12,78,groupw,116},{right,78,groupw,68},{right,152,groupw,46}};
    for(int i=0;i<3;i++) {
        ui_surface(groups[i],ui_glass(cell,40));
        DrawRectangleLinesEx(groups[i],1,Fade(ui_theme.border,.45f));
    }
    label("Precomputed effects",20,82,12,muted);
    sampler_switch(20,100,groupw-16,"Normalize",&settings->flags,SAMPLE_NORMALIZE);
    sampler_switch(20,128,groupw-16,"Reverse",&settings->flags,SAMPLE_REVERSE);
    sampler_switch(20,156,groupw-16,"Reverse polarity",&settings->flags,SAMPLE_POLARITY);
    if(hover(20,100,groupw-16,22)) snprintf(status,sizeof status,"Normalize: bring the processed sample peak to full scale");
    if(hover(20,128,groupw-16,22)) snprintf(status,sizeof status,"Reverse: play the cropped sample backwards");
    if(hover(20,156,groupw-16,22)) snprintf(status,sizeof status,"Polarity: flip the waveform vertically");
    label("Time stretching",right+8,82,12,muted);
    knob_style(right+28,108,&settings->pitch,-12,12,0,"Pitch (semitones)",KNOB_CENTER);
    knob_style(right+84,108,&settings->time,.25f,4,1,"Time multiplier",KNOB_CENTER);
    label("Pitch",right+14,128,11,muted); label("Time",right+71,128,11,muted);
    if(button(settings->stretch?"Stretch":"Resample",right+116,98,groupw-128,22,0)) settings->stretch^=1;
    label("Mode",right+116,128,11,muted);
    if(hover(right+116,98,groupw-128,22)) snprintf(status,sizeof status,"Time mode: Resample changes speed/pitch; Stretch preserves pitch; click to switch");
    label("Trim",right+8,165,12,muted);
    knob(right+88,168,&settings->start,0,1,0,"Start");
    knob(right+136,168,&settings->length,0,1,1,"Length");
    label("Start",right+74,184,11,muted); label("Length",right+119,184,11,muted);
    knob(right+206,168,&settings->trim,0,1,0,"Trim quiet tail (threshold)");
    label("Trim",right+193,184,11,muted);
    int pending=!sampler_equal(*settings,sampler_applied[c]);
    Sampler applied=sampler_applied[c];
    int live_preview=pending && settings->pitch==applied.pitch && settings->time==applied.time && settings->stretch==applied.stretch;
    unsigned offset=llround(originals[c].frames*(double)settings->start);
    unsigned cropped=llround((originals[c].frames-offset)*(double)settings->length);
    cropped=sample_trim_end((Sample){originals[c].data?originals[c].data+(size_t)offset*sample_channels(originals[c]):NULL,cropped,originals[c].channels},settings->trim);
    float duration=live_preview?cropped*settings->time/RATE:samples[c].frames/(float)RATE;

    static float peaks[512][2]; static const float *cached; static unsigned cached_frames;
    Sample sample=samples[c];
    if(cached!=sample.data || cached_frames!=sample.frames) {
        cached=sample.data; cached_frames=sample.frames;
        for(unsigned i=0;i<512;i++) {
            peaks[i][0]=peaks[i][1]=0;
            for(unsigned f=(uint64_t)i*sample.frames/512;f<(uint64_t)(i+1)*sample.frames/512;f++) {
                peaks[i][0]=fminf(peaks[i][0],fminf(sample_at(sample,f,0),sample_at(sample,f,1))); peaks[i][1]=fmaxf(peaks[i][1],fmaxf(sample_at(sample,f,0),sample_at(sample,f,1)));
            }
        }
    }
    /* Preview the source envelope cheaply; audio processing still waits for release. */
    float preview[512][2];
    if(live_preview) {
        static float source_peaks[4096][2];
        static uint64_t epoch; static unsigned generation; static int source_channel=-1;
        Sample source=originals[c];
        if(source_channel!=c || epoch!=sample_epoch || generation!=sample_generation[c]) {
            source_channel=c; epoch=sample_epoch; generation=sample_generation[c];
            for(unsigned i=0;i<4096;i++) {
                source_peaks[i][0]=source_peaks[i][1]=0;
                for(unsigned f=(uint64_t)i*source.frames/4096;f<(uint64_t)(i+1)*source.frames/4096;f++) {
                    source_peaks[i][0]=fminf(source_peaks[i][0],fminf(sample_at(source,f,0),sample_at(source,f,1)));
                    source_peaks[i][1]=fmaxf(source_peaks[i][1],fmaxf(sample_at(source,f,0),sample_at(source,f,1)));
                }
            }
        }
        float peak=0;
        for(unsigned i=0;i<512;i++) {
            unsigned bin=settings->flags&SAMPLE_REVERSE?511-i:i;
            unsigned begin=offset+(uint64_t)bin*cropped/512,end=offset+(uint64_t)(bin+1)*cropped/512;
            float low=0,high=0;
            if(cropped<8192) {
                for(unsigned f=begin;f<end;f++) { low=fminf(low,fminf(sample_at(source,f,0),sample_at(source,f,1))); high=fmaxf(high,fmaxf(sample_at(source,f,0),sample_at(source,f,1))); }
            } else {
                unsigned first=(uint64_t)begin*4096/source.frames,last=((uint64_t)end*4096+source.frames-1)/source.frames;
                for(unsigned j=first;j<last && j<4096;j++) { low=fminf(low,source_peaks[j][0]); high=fmaxf(high,source_peaks[j][1]); }
            }
            preview[i][0]=settings->flags&SAMPLE_POLARITY?-high:low;
            preview[i][1]=settings->flags&SAMPLE_POLARITY?-low:high;
            peak=fmaxf(peak,fmaxf(-low,high));
        }
        if((settings->flags&SAMPLE_NORMALIZE) && peak>0) for(int i=0;i<512;i++) { preview[i][0]/=peak; preview[i][1]/=peak; }
    }
    float (*display_peaks)[2]=live_preview?preview:peaks;
    float area=height-234,mid=202+area/2;
    DrawRectangle(12,202,width-24,area,bg); DrawLine(12,mid,width-12,mid,cell);
    for(int i=0;i<512;i++) {
        float x=12+i*(width-24)/512;
        DrawLineEx((Vector2){x,mid-display_peaks[i][1]*area*.45f},(Vector2){x,mid-display_peaks[i][0]*area*.45f},1,ui_theme.waveform);
    }
    double progress=audio_key_position(127,c);
    if(hover(12,202,width-24,area)) {
        snprintf(status,sizeof status,"Sample waveform: click to %s; drop a Browser sample here to load it",progress>=0?"stop playback":"play from the beginning");
        if(samples[c].frames && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) { channel=c; audio_key(127,c,60,progress<0); }
    }
    if(progress>=0 && !live_preview) {
        float x=12+progress*(width-24);
        DrawLineEx((Vector2){x,202},(Vector2){x,202+area},1.5f,ink);
    }
    label("0 s",16,204,10,muted);
    label(TextFormat("%.2f s%s",duration,pending?" *":""),width-78,204,10,muted);
    if(live_preview && hover(12,202,width-24,area)) snprintf(status,sizeof status,"Live envelope preview; processed pitch/stretch detail appears when ready; audio changes after release");
    if(sample_drag[0] && sample_moved) DrawRectangleLines(12,202,width-24,area,ui_theme.signal);
    label("Click waveform to play / stop | Drop sample to load",12,height-22,11,muted);
}
static void draw_editor(int id,float scale) {
    Editor *e=&windows.editors[id]; if(!e->visible) return;
    knob_context=id;
    Rect r=e->rect; int top=EDITORS-1;
    while(top>0 && !windows.editors[windows.order[top]].visible) top--;
    int focused=(windows.editors[windows.focused].visible?windows.focused:windows.order[top])==id;
    BeginScissorMode((int)(r.x*scale),(int)(r.y*scale),(int)(r.w*scale),(int)(r.h*scale));
    text_origin=(Vector2){r.x*scale,r.y*scale};
    BeginMode2D((Camera2D){.offset=text_origin,.zoom=scale});
    ui_surface((Rectangle){0,0,r.w,r.h},ui_glass(id==0?ui_theme.rack:id==3?ui_theme.mixer:panel,160));
    ui_surface((Rectangle){0,0,r.w,TITLE},ui_glass(focused?ui_theme.title_focus:ui_theme.title,90));
    const char *titles[]={"Channel Rack","Playlist - Arrangement","Piano Roll","Mixer","Sampler"};
    label(TextFormat("%s%s",titles[id],e->pinned?" (on top)":""),8,3,13,ink);
    int rack_title_knob=id==0 && ((mouse.x>=r.x+r.w-90 && mouse.x<r.x+r.w-54) || (mouse.x>=r.x+124 && mouse.x<r.x+194));
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
        button(rack_filters[rack_filter],124,1,70,TITLE-2,0);
        if(hover(124,0,70,TITLE)) snprintf(status,sizeof status,"Channel filter: All, Audio (Playlist drops), Unsorted (Rack samples)");
        knob_style(r.w-72,TITLE/2,&project.swing,0,1,0,"Swing",KNOB_SWING);
        if(hover(r.w-81,0,18,TITLE) || control_drag==&project.swing) snprintf(status,sizeof status,"Swing: %.0f%% | delays alternate steps; drag or wheel; right-click to enter",project.swing*100);
    }
    input_enabled=input_enabled && mouse.y>=TITLE && !(mouse.x>r.w-12 && mouse.y>r.h-12);
    if(id==0) rack(r.w,r.h); else if(id==1) playlist(r.w,r.h,scale); else if(id==2) piano(r.w,r.h); else if(id==3) mixer(r.w,r.h); else sampler(r.w,r.h);
    mouse=global;
    Color border=ui_theme.border;
    if(focused) { border.r+=(ink.r-border.r)*.6f; border.g+=(ink.g-border.g)*.6f; border.b+=(ink.b-border.b)*.6f; }
    DrawLine(r.w-9,r.h-2,r.w-2,r.h-9,muted); DrawRectangleLinesEx((Rectangle){0,0,r.w,r.h},2,focused?border:Fade(border,.6f));
    EndMode2D(); EndScissorMode(); text_origin=(Vector2){0};
}
/* Apply navigation once to the frontmost editor, before drawing/editing. */
static void navigate_editors(float scale) {
    navigation_active=0;
    int blocked=pattern_popup || context_kind || snap_menu || sample_drag[0] || !IsWindowFocused();
    if(navigation_drag>=0 && (blocked || !IsMouseButtonDown(MOUSE_BUTTON_MIDDLE))) navigation_drag=-1;
    int id=navigation_drag>=0?navigation_drag:windows_hit(&windows,mouse.x,mouse.y);
    if(blocked || (id!=1 && id!=2) || (navigation_drag<0 && captured()) || windows.grab>=0) return;
    Rect r=windows.editors[id].rect;
    float gx=id==1?212:60,gy=id==1?82:72,gridw=r.w-gx-24;
    float area=id==1?r.h-gy-42:r.h-166;
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
        if(motion.zoom && local.x>=gx) arrangement_zoom(&arrangement,motion.zoom,fmaxf(0,fminf(1,(local.x-gx)/gridw)));
        track_scroll=fmaxf(0,fminf(fmaxf(0,track_position(LANES)-area),track_scroll+motion.y));
    } else {
        piano_pan[pattern]=fmaxf(0,piano_pan[pattern]+motion.x*piano_span[pattern]/gridw);
        if(motion.zoom && local.x>=gx) timeline_zoom(&piano_span[pattern],&piano_pan[pattern],motion.zoom,fmaxf(0,fminf(1,(local.x-gx)/gridw)),STEPS);
        piano_scroll_remainder+=motion.y/(area/25);
        int rows=(int)piano_scroll_remainder;
        piano_top=fmaxf(24,fminf(127,piano_top-rows)); piano_scroll_remainder-=rows;
        if(piano_top==24 || piano_top==127) piano_scroll_remainder=0;
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
    project_new(&project);
    for(int c=0;c<CHANNELS;c++) sampler_applied[c]=project.sampler[c];
    int audio_ok=audio_start(&project,samples);
    if(!audio_ok) snprintf(status,sizeof status,"Audio device unavailable. Editing and WAV export are available.");
    unsigned flags=FLAG_WINDOW_RESIZABLE;
#ifdef __APPLE__
    flags|=FLAG_WINDOW_HIGHDPI;
#endif
    SetConfigFlags(flags); InitWindow(1200,675,"LibreLoop - pattern workstation");
    circles_init(); arcs_init(); cables_init();
    SetWindowMinSize(900,506); SetTargetFPS(60); SetExitKey(KEY_NULL);
    app_window=glfwGetCurrentContext();
    raylib_mouse_callback=glfwSetMouseButtonCallback(app_window,mouse_callback);
#ifdef __APPLE__
    macos_navigation_init(app_window);
#endif
    resize_cursor=glfwCreateStandardCursor(GLFW_HRESIZE_CURSOR);
    track_cursor=glfwCreateStandardCursor(GLFW_VRESIZE_CURSOR);
    browser_init(&browser,samples_path); theme_init(browser.config); int frames=0,initialized=0; double last_activity=GetTime();
    while(!WindowShouldClose()) {
        if(sampler_job.busy && atomic_load(&sampler_job.done)) last_activity=GetTime();
        sampler_update();
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
        float scale=fminf(GetScreenWidth()/1200.f,GetScreenHeight()/675.f);
        float raster_scale=scale;
#ifdef __APPLE__
        raster_scale*=GetWindowScaleDPI().x;
#endif
        fonts_update(raster_scale);
        float width=GetScreenWidth()/scale,height=GetScreenHeight()/scale;
        float sidebar=browser_hidden?24:browser_width;
        Rect desktop={sidebar+6,42,width-sidebar-10,height-66};
        if(!initialized) { windows_init(&windows,width,height); initialized=1; }
        mouse=GetMousePosition(); mouse.x/=scale; mouse.y/=scale; reset=0; popup_opened=0; context_opened=0; snap_opened=0;
        SetMouseCursor(MOUSE_CURSOR_DEFAULT);
        navigate_editors(scale);
        if(IsMouseButtonPressed(MOUSE_BUTTON_LEFT) || IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) arrangement_release(&arrangement);
        if(arrangement.gesture) {
            Rect r=windows.editors[1].rect;
            arrangement_drag(&arrangement,&project,arrangement.view_start+(mouse.x-r.x-212)/((r.w-236)/BARS*arrangement.zoom),track_at(track_scroll+mouse.y-r.y-82));
            int held=arrangement.gesture==ERASE_CLIPS?(IsMouseButtonDown(MOUSE_BUTTON_RIGHT) || IsMouseButtonDown(MOUSE_BUTTON_LEFT)):IsMouseButtonDown(MOUSE_BUTTON_LEFT);
            if(!held) arrangement_release(&arrangement);
        }
        if(playlist_pan) {
            arrangement.view_start=fmaxf(0,playlist_pan_start+(mouse.x-playlist_pan_x)*playlist_pan_range);
            if(!IsMouseButtonDown(MOUSE_BUTTON_LEFT) || IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) playlist_pan=0;
        }
        if(playlist_vpan) {
            Rect r=windows.editors[1].rect; float area=r.h-124;
            float total=track_position(LANES),travel=area-timeline_thumb(area,area,total),maximum=fmaxf(0,total-area);
            track_scroll=fmaxf(0,fminf(maximum,playlist_vpan_start+(mouse.y-playlist_vpan_y)/fmaxf(1,travel)*maximum));
            if(!IsMouseButtonDown(MOUSE_BUTTON_LEFT) || IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) playlist_vpan=0;
        }
        if(track_resize>=0) {
            track_heights[track_resize]=roundf(fmaxf(32,fminf(320,track_resize_height+mouse.y-track_resize_y)));
            SetMouseCursor(MOUSE_CURSOR_RESIZE_NS);
            if(!IsMouseButtonDown(MOUSE_BUTTON_LEFT)) track_resize=-1;
        }
        if(mixer_pan) {
            Rect r=windows.editors[3].rect; float area=r.w-220-MIXER_LEFT-8; int visible=fmaxf(1,area/51-1);
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
        int modal=pattern_popup!=0 || context_kind!=0 || snap_menu!=0,dragging=sample_drag[0]!=0;
        if(!IsMouseButtonDown(MOUSE_BUTTON_LEFT) || IsMouseButtonPressed(MOUSE_BUTTON_LEFT) || modal) { route_drag=-1; control_drag=NULL; note_drag=NULL; velocity_drag=-1; }
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
            float q=id==0?1:snap_interval(snap_mode[id-1],width/span);
            int button=ruler_loop_drag?MOUSE_BUTTON_RIGHT:MOUSE_BUTTON_LEFT;
            if(modal || IsMouseButtonPressed(button)) marker_drag=-1;
            else {
                float at=fmaxf(0,roundf((start+(mouse.x-r.x-gx)/width*span)/q)*q);
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
            if(control_integer) { control_raw=fmaxf(control_low,fminf(control_high,control_raw-GetMouseDelta().y/scale*(control_high-control_low)/160)); value=roundf(control_raw); }
            *control_drag=fmaxf(control_low,fminf(control_high,value));
        }
        if(piano_scroll_drag) {
            piano_pan[pattern]=fmaxf(0,piano_scroll_start+(mouse.x-piano_scroll_x)*piano_scroll_range);
            if(!IsMouseButtonDown(MOUSE_BUTTON_LEFT) || IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) piano_scroll_drag=0;
        }
        if(piano_vdrag) {
            Rect r=windows.editors[2].rect; float area=r.h-166;
            piano_top=127-(int)fmaxf(0,fminf(103,piano_vscroll_start+(mouse.y-piano_vscroll_y)/area*128));
            if(!IsMouseButtonDown(MOUSE_BUTTON_LEFT) || IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) piano_vdrag=0;
        }
        if(piano_gesture) {
            int right=piano_gesture==PIANO_ERASE;
            if(modal || IsMouseButtonPressed(MOUSE_BUTTON_LEFT) || IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) piano_gesture=PIANO_IDLE;
            else {
                Rect r=windows.editors[2].rect; float cw=(r.w-84)/piano_span[pattern],rh=(r.h-166)/25;
                piano_gesture_update(piano_pan[pattern]+(mouse.x-r.x-60)/cw,piano_top-(mouse.y-r.y-72)/rh,snap_interval(snap_mode[1],cw));
                if(!IsMouseButtonDown(right?MOUSE_BUTTON_RIGHT:MOUSE_BUTTON_LEFT)) piano_gesture=PIANO_IDLE;
            }
        }
        if(note_drag) {
            Rect r=windows.editors[2].rect; float cw=(r.w-84)/piano_span[pattern],rh=(r.h-166)/25,q=snap_interval(snap_mode[1],cw);
            float step=piano_pan[pattern]+(mouse.x-r.x-60)/cw;
            if(moving_note) {
                uint8_t *selected=note_selected[pattern][piano_channel];
                float dx=roundf((step-note_grab.x)/q)*q,low=0,high=0; int dy=roundf(note_grab.y-(mouse.y-r.y-72)/rh),bottom=0,top=127,first=1;
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
                notes_move(&project,pattern,piano_channel,note_before,selected,dx,dy,edit_steps());
                if(note_drag->pitch!=old_pitch) audio_note(piano_channel,*note_drag);
            } else { float length=fmaxf(q,roundf((step-note_drag->start)/q)*q); piano_extend(note_drag->start+length); note_drag->length=length; }
        }
        if(velocity_drag>=0) {
            Rect r=windows.editors[2].rect;
            int velocity=fmaxf(1,fminf(127,(r.y+r.h-22-mouse.y)/.48f));
            for(int i=0;i<NOTES;i++) { Note *n=&project.notes[pattern][piano_channel][i]; if(n->velocity && fabsf(n->start-velocity_drag)<.00001f) n->velocity=velocity; }
        }
        if(route_drag>=0) {
            project.route[route_drag]=(int)fmaxf(0,fminf(project.insert_count,route_start+(int)((route_y-mouse.y)/3)));
            mixer_selected=project.route[route_drag];
            int visible=fmaxf(1,(windows.editors[3].rect.w-220-MIXER_LEFT-8)/51-1);
            if(mixer_selected>mixer_scroll+visible) mixer_scroll=mixer_selected-visible;
            else if(mixer_selected>0 && mixer_selected<=mixer_scroll) mixer_scroll=mixer_selected-1;
            snprintf(status,sizeof status,"%s -> %s",project.channel_names[route_drag],mixer_name(mixer_selected));
        }
        Rect filter_rect=windows.editors[0].rect;
        if(!modal && !captured() && !dragging && windows_hit(&windows,mouse.x,mouse.y)==0 && mouse.x>=filter_rect.x+124 && mouse.x<filter_rect.x+194 && mouse.y>=filter_rect.y && mouse.y<filter_rect.y+TITLE && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            windows_focus(&windows,0); open_context(6,0,(Vector2){filter_rect.x+124,filter_rect.y+TITLE}); modal=1;
        }
        if(!modal && !captured() && !dragging) windows_update(&windows,desktop,mouse.x,mouse.y,IsMouseButtonPressed(MOUSE_BUTTON_LEFT),IsMouseButtonDown(MOUSE_BUTTON_LEFT),GetTime());
        else { if(browser_resize) windows_update(&windows,desktop,mouse.x,mouse.y,0,0,GetTime()); windows.owner=-1; windows.grab=-1; }
        int focused=EDITORS-1;
        while(focused>0 && !windows.editors[windows.order[focused]].visible) focused--;
        if(!modal && !dragging && !captured() && !browser_focus && windows.order[focused]==1 && windows.editors[1].visible && delete_pressed()) {
            for(int l=0;l<LANES;l++) for(int b=0;b<CLIPS;b++) if(arrangement.selected[l][b]) { project.clips[l][b]=0; project.clip_steps[l][b]=0; }
            memset(arrangement.selected,0,sizeof arrangement.selected);
        }
        if(!modal && !dragging && !captured() && !browser_focus && windows.order[focused]==2 && windows.editors[2].visible && delete_pressed()) {
            for(int i=0;i<NOTES;i++) if(note_selected[pattern][piano_channel][i]) project.notes[pattern][piano_channel][i].velocity=0;
            memset(note_selected[pattern][piano_channel],0,NOTES);
        }
        if(!modal && IsFileDropped()) {
            FilePathList dropped=LoadDroppedFiles();
            for(unsigned i=0;i<dropped.count;i++) {
                const char *path=dropped.paths[i];
                if(IsFileExtension(path,".hbt")) load_project(path);
                else drop_sample(path);
            }
            UnloadDroppedFiles(dropped);
        }
        track_activity_update();
        if(windows.editors[1].visible) previews_update(scale);
        knob_context=-1; knob_animating=0; browser_progress=-1;
        BeginDrawing(); ClearBackground(bg); BeginMode2D((Camera2D){.zoom=scale});
        input_enabled=!modal && !dragging && !captured() && windows.grab<0 && mouse.y<42;
        ui_surface((Rectangle){0,0,width,40},panel);
        ui_surface((Rectangle){8,8,160,22},cell);
        if(button("FILE",8,8,52,22,pattern_popup==1)) open_popup(1);
        if(hover(8,8,48,22)) snprintf(status,sizeof status,"File: New, Demo, Save, Open or Export");
        if(button("VIEW",60,8,56,22,pattern_popup==7)) open_popup(7);
        if(hover(60,8,52,22)) snprintf(status,sizeof status,"View: theme and transparency settings");
        if(button("HELP",116,8,52,22,pattern_popup==9)) open_popup(9);
        label(GetFileName(project_path),8,29,10,muted);
        ui_surface((Rectangle){208,8,52,22},cell);
        knob_style(222,18,&project.master_pitch,-12,12,0,"Master pitch (semitones)",KNOB_CENTER);
        if(hover(211,7,22,22) || control_drag==&project.master_pitch) snprintf(status,sizeof status,"Master pitch: %+.2f semitones (sample speed; right-click to enter)",project.master_pitch);
        knob_style(248,18,&output_volume,0,VOLUME_KNOB_MAX,1,"LibreLoop output volume",KNOB_VOLUME);
        if(hover(237,7,22,22) || control_drag==&output_volume) snprintf(status,sizeof status,"LibreLoop output volume: listening level; Master and WAV export unchanged");
        if(button("",264,5,52,28,0)) { song=!song; reset=1; }
        DrawRectangle(265,song?19:6,50,13,song?accent:ui_theme.patt);
        label("PATT",275,7,10,song?muted:ui_theme.selected_text); label("SONG",275,20,10,song?ui_theme.selected_text:muted);
        if(hover(264,5,52,28)) snprintf(status,sizeof status,"Playback mode: click to switch Pattern (orange) / Song (green)");
        int transport_active=playing || audio_preview_position(audition)>=0 || audio_key_position(127,instrument_channel)>=0;
        if(button(transport_active?"||":">",324,8,24,22,transport_active)) transport_toggle();
        if(hover(324,8,24,22)) snprintf(status,sizeof status,"Play from ruler / stop all audio, including previews (Space)");
        if(button("[]",352,8,24,22,0)) { audio_stop(); playing=0; reset=1; }
        if(button("",388,8,22,22,metronome)) { metronome=!metronome; audio_metronome(metronome); }
        icon(ICON_METRO,399,19,20,metronome?ui_theme.selected_text:ink);
        if(hover(388,8,22,22)) snprintf(status,sizeof status,"Metronome: %s | beat clicks during playback; first beat accented; excluded from WAV export",metronome?"On":"Off");
        if(button(TextFormat("%.2f",project.bpm),418,8,56,22,control_drag==&project.bpm)) capture_control(&project.bpm,30,300,0);
        if(hover(418,8,56,22)) {
            project.bpm=fmaxf(30,fminf(300,project.bpm+GetMouseWheelMove()));
            snprintf(status,sizeof status,"Tempo: drag up/down; right-click to enter a value");
            if(IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) begin_number(&project.bpm,30,300,120,"Tempo (BPM)");
        }
        uint64_t step=(uint64_t)(audio_position()/(RATE*60.0/project.bpm/4));
        ui_surface((Rectangle){478,8,88,22},bg);
        label(TextFormat("%03llu:%02llu",(unsigned long long)(step/16+1),(unsigned long long)(step%16+1)),488,10,18,ui_theme.signal);
        if(hover(478,8,88,22)) snprintf(status,sizeof status,"Playback position: bar and sixteenth-note step");
        for(int i=0;i<4;i++) { int id=i==1?2:i==2?1:i; editor_button(id,574+i*26); }
        if(button("",686,8,22,22,follow_playhead)) { follow_playhead=!follow_playhead; if(!follow_playhead) { arrangement.view_start=fmaxf(0,arrangement.view_start); for(int p=0;p<PATTERNS;p++) piano_pan[p]=fmaxf(0,piano_pan[p]); } }
        icon(ICON_FOLLOW,697,19,20,follow_playhead?ui_theme.selected_text:ink);
        if(hover(686,8,22,22)) snprintf(status,sizeof status,"Follow playhead: center playback in the Playlist (Song) or Piano Roll (PAT)");
        if(button("",712,8,22,22,typing_keys)) typing_keys=!typing_keys;
        icon(ICON_KEYS,723,19,20,typing_keys?ui_theme.selected_text:ink);
        if(hover(712,8,22,22)) snprintf(status,sizeof status,"Typing piano: Z row C3, Q row C4; S/D/G/H/J and 2/3/5/6/7 sharps; disabled in Browser/text fields");
        visual_step=playing && !reset?audio_visual_position()/(RATE*60.0/project.bpm/4):playback_start();
        int browser_hover=!browser_hidden && mouse.x<sidebar && mouse.y>=42 && mouse.y<height-22;
        typing_piano(modal || dragging || browser_hover);
        ui_surface((Rectangle){width-68,8,56,22},bg);
        label(audio_ok?"Audio":"Offline",width-56,13,12,audio_ok?ui_theme.signal:muted);
        if(hover(width-68,8,56,22)) snprintf(status,sizeof status,audio_ok?"Audio device is running":"Audio device unavailable");
        input_enabled=!modal && !pattern_popup && !context_kind && !snap_menu && !dragging && !captured() && windows.grab<0 && mouse.x<sidebar+6 && mouse.y>=42;
        DrawRectangle(0,42,sidebar,height-66,ui_theme.browser);
        if(!browser_hidden) {
            if(hover(0,42,sidebar,height-66)) snprintf(status,sizeof status,"Browser: click to preview; drag to load | Up/Down or j/k: select | Left/Right or h/l: fold");
            label("Browser",10,50,14,ink);
            if(button("+ Add folder",8,76,sidebar-16,23,0)) { snprintf(folder_text,sizeof folder_text,"%s",browser.path); rename_select_all=1; open_popup(3); }
            int fy=108,rows=fmaxf(1,(height-fy-98)/23);
            if(!modal && !pattern_popup && !context_kind && !snap_menu && (browser_focus || browser_hover) && !dragging && !captured() && !command_down()) {
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
                int clicked=button("",8,y,sidebar-16,21,browser.selected==entry);
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
                    if(IsFileExtension(path,".hbt")) load_project(path);
                    else {
                        audition_entry(entry); snprintf(sample_drag,sizeof sample_drag,"%s",path);
                        sample_origin=mouse; sample_moved=0;
                        sample_offset=(Vector2){mouse.x-8,mouse.y-y}; sample_drag_width=sidebar-16; sample_text_offset=indent+6;
                    }
                }
            }
            EndScissorMode();
            if(browser.selected>=0 && browser.selected<browser.items && audition.data && !strcmp(browser.nodes[browser.selected].path,audition_path)) {
                for(int i=0;i<128;i++) {
                    float x=9+i*(sidebar-18)/128.0f;
                    DrawLine(x,height-64-audition_high[i]*29,x,height-64-audition_low[i]*29,ui_theme.signal);
                }
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
        for(int i=0;i<EDITORS;i++) { input_enabled=!modal && !pattern_popup && !context_kind && !snap_menu && !dragging && !sample_drag[0] && !captured(); draw_editor(order[i],scale); }
        mixer_was_visible=windows.editors[3].visible;
        BeginMode2D((Camera2D){.zoom=scale}); DrawRectangle(0,height-22,width,22,panel); label(fit_text(status,width-16,11),8,height-17,11,ink);
        draw_browser_drag();
        draw_picker_drag();
        draw_popup(); draw_context(); draw_snap(); EndMode2D();
        sampler_queue();
        flush_cursor(); audio_update(&project,playing,song,pattern,reset,output_volume,playback_start(),playback_loop()[0],playback_loop()[1]);
        int animate=browser_progress>=0 || knob_animating || (windows.editors[3].visible && (audio_active() || mixer_decay_active)) || playing || captured() || windows.grab>=0 || popup_drag || sample_drag[0] ||
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
    if(sampler_job.busy) { pthread_join(sampler_job.thread,NULL); free(sampler_job.input.data); free(sampler_job.result.data); }
    for(int pat=0;pat<PATTERNS;pat++) if(previews[pat].image.id) UnloadRenderTexture(previews[pat].image);
    #ifdef __APPLE__
    macos_navigation_close();
#endif
    fonts_close(); UnloadTexture(circle_texture); UnloadTexture(icons); UnloadTexture(knob_arcs); UnloadTexture(cable_texture);
    browser_close(&browser); audio_close(); free(audition.data); if(resize_cursor) glfwDestroyCursor(resize_cursor); if(track_cursor) glfwDestroyCursor(track_cursor); CloseWindow();
    for(int c=0;c<CHANNELS;c++) { free(audio_waves[c].tree); free(samples[c].data); free(originals[c].data); }
    return 0;
}
