// SPDX-License-Identifier: GPL-3.0-only
#include "raylib.h"
#include "audio.h"
#include "windows.h"
#include "browser.h"
#include "arrangement.h"
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
static GLFWcursor *resize_cursor;
static void request_cursor(int cursor) { requested_cursor=cursor; }
static void flush_cursor(void) {
    if(requested_cursor==current_cursor) return;
    glfwSetCursor(GetWindowHandle(),requested_cursor==MOUSE_CURSOR_RESIZE_EW?resize_cursor:NULL);
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
    if(source.frames) { copy.data=malloc(source.frames*sizeof(float)); if(copy.data) { memcpy(copy.data,source.data,source.frames*sizeof(float)); copy.frames=source.frames; } }
    return copy;
}
static void *sampler_worker(void *unused) {
    (void)unused;
    sampler_job.ok=sample_process(sampler_job.input,sampler_job.settings,&sampler_job.result);
    atomic_store(&sampler_job.done,1); return NULL;
}
static void sampler_update(void) {
    if(sampler_job.busy && atomic_load(&sampler_job.done)) {
        pthread_join(sampler_job.thread,NULL); int c=sampler_job.channel;
        if(sampler_job.epoch==sample_epoch && c<project.channel_count && sampler_job.generation==sample_generation[c] && sampler_equal(sampler_job.settings,project.sampler[c])) {
            if(sampler_job.ok) {
                Sample old=samples[c]; audio_sample(c,sampler_job.result); samples[c]=sampler_job.result; sampler_job.result=(Sample){0}; free(old.data);
            } else project.sampler[c]=sampler_applied[c];
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
        sampler_applied[c]=project.sampler[c]; sampler_generation[c]=sample_generation[c];
    }
    return 1;
}

static Color bg={24,27,31,255},panel={34,39,45,255},cell={48,55,63,255},ink={218,224,230,255},muted={139,151,163,255},accent={100,198,171,255};
static Vector2 mouse;
static int pattern,channel,piano_channel,instrument_channel,playing,song,reset;
static int typing_keys=1;
static float output_volume=1;
static int follow_playhead;
static double visual_step;
static Windows windows;
static int input_enabled=1;
static Arrangement arrangement={.source_steps=STEPS,.source_pattern=-1,.zoom=1};
static float rack_view[PATTERNS],rack_range[PATTERNS];
static int rack_hdrag; static float rack_hx,rack_hstart,rack_hscale;
#define RACK_STEP_WIDTH 16
static int playlist_pan,playlist_vpan;
static float track_scroll,playlist_vpan_y,playlist_vpan_start;
static float playlist_pan_x,playlist_pan_start,playlist_pan_range;
static float playlist_start,piano_start[PATTERNS];
static int marker_drag=-1;
static unsigned char piano_channels[PATTERNS][CHANNELS];
static int rack_vdrag;
static float rack_vy,rack_vstart,rack_vscale;
static int rack_scroll,context_kind,context_target,context_opened;
static Vector2 context_position;
static int pattern_popup,popup_opened,rename_pattern,rename_select_all,popup_drag;
static Vector2 popup_position[7],popup_offset;
static char rename_text[PATTERN_NAME],folder_text[PATH_MAX],number_text[64];
static float *number_target,number_low,number_high,number_default;
static const char *number_name;
static Browser browser;
static int route_drag=-1,route_start,mixer_selected=1,mixer_scroll,mixer_pan,cable_drag=-1;
static float route_y;
static Sample audition;
static char audition_path[PATH_MAX];
static float audition_low[128],audition_high[128];
static int browser_focus=1,sample_moved,browser_hidden,browser_resize;
static float browser_width=174,last_sidebar=174;
static char sample_drag[PATH_MAX];
static Vector2 sample_origin;
static float *control_drag,control_low,control_high,control_bottom,control_range;
static int control_fader;
static Note *note_drag;
static int moving_note,piano_scroll_drag,piano_vdrag;
static int piano_top=72;
static int piano_key_drag,piano_key=-1,piano_key_channel;
static float piano_scroll_x,piano_scroll_start,piano_scroll_range,piano_vscroll_y,piano_vscroll_start;
static Note note_before[NOTES];
static uint8_t note_selected[PATTERNS][CHANNELS][NOTES],note_selection_before[NOTES];
static int piano_tool,piano_gesture,piano_additive;
enum { PIANO_IDLE, PIANO_BRUSH, PIANO_ERASE, PIANO_BOX };
static Vector2 piano_from,piano_now;
static Vector2 note_grab;
static float piano_span[PATTERNS]={16,16,16,16,16,16,16,16},piano_pan[PATTERNS],piano_range[PATTERNS];
static float velocity_drag=-1;
static int snap_mode[2]={SNAP_STEP,SNAP_STEP},snap_menu,snap_opened;
static Vector2 snap_position;
static int rack_paint=-1,rack_paint_channel=-1; static float rack_paint_step;
static int captured(void) { return rack_hdrag || rack_vdrag || piano_key_drag || control_drag || route_drag>=0 || note_drag || velocity_drag>=0 || arrangement.gesture || playlist_pan || browser_resize || rack_paint>=0 || cable_drag>0 || playlist_vpan || mixer_pan || piano_scroll_drag || piano_vdrag || piano_gesture || marker_drag>=0; }
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
static float playback_start(void) { return song?playlist_start*STEPS:piano_start[pattern]; }
static void transport_toggle(void) { playing=!playing; reset=1; }
static void label(const char *text,int x,int y,int size,Color color) { DrawText(text,x,y,size,color); }
static void backspace(char *text) {
    size_t n=strlen(text); if(!n) return;
    do { n--; } while(n && ((unsigned char)text[n]&0xc0)==0x80);
    text[n]=0;
}
static const char *fit_text(const char *text,int width,int size) {
    static char fitted[PATH_MAX]; if(text!=fitted) snprintf(fitted,sizeof fitted,"%s",text);
    while(fitted[0] && MeasureText(fitted,size)>width) backspace(fitted);
    return fitted;
}
static int hover(float x,float y,float w,float h) { return input_enabled && CheckCollisionPointRec(mouse,(Rectangle){x,y,w,h}); }
/* Small control symbols share one size and never depend on font glyphs. */
static void zoom_label(float percent,int x,int y) { label(fit_text(TextFormat(percent<1?"%.2f%%":"%.0f%%",percent),56,11),x,y,11,muted); }
static int symbol(const char *text,int x,int y,Color c) {
    if(!strcmp(text,"x")) { DrawLine(x-3,y-3,x+3,y+3,c); DrawLine(x-3,y+3,x+3,y-3,c); }
    else if(!strcmp(text,"[]")) DrawRectangle(x-4,y-4,8,8,c);
    else if(!strcmp(text,"||")) { DrawRectangle(x-4,y-5,3,10,c); DrawRectangle(x+1,y-5,3,10,c); }
    else if(!strcmp(text,">")) DrawTriangle((Vector2){x-3,y-5},(Vector2){x-3,y+5},(Vector2){x+4,y},c);
    else if(!strcmp(text,"<")) DrawTriangle((Vector2){x+3,y-5},(Vector2){x-4,y},(Vector2){x+3,y+5},c);
    else return 0;
    return 1;
}
static int button(const char *text,int x,int y,int w,int h,int active) {
    int over=hover(x,y,w,h);
    if(over && *text) {
        const char *description=text;
        if(!strcmp(text,"Save")) description="Save the current project";
        else if(!strcmp(text,"Open")) description="Reload the current project file";
        else if(!strcmp(text,"WAV")) description="Export the Playlist to song.wav";
        else if(!strcmp(text,"Help")) description="Show all current keybindings";
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
    DrawRectangle(x,y,w,h,active?(over?(Color){122,215,190,255}:accent):over?(Color){65,75,85,255}:cell);
    if(w>52 || !symbol(text,x+w/2,y+h/2,active?bg:ink)) {
        int size=13; const char *caption=fit_text(text,w-(w<=52?8:16),size);
        label(caption,x+(w<=52?(w-MeasureText(caption,size))/2:8),y+(h-size)/2,size,active?bg:ink);
    }
    return over && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
}
static int editor_button(int id,int x) {
    static const char *names[]={"Channel Rack","Playlist","Piano Roll","Mixer"};
    int front=0;
    for(int i=EDITORS-1;i>=0;i--) {
        int other=windows.order[i];
        if(windows.editors[other].visible && windows.editors[other].pinned==windows.editors[id].pinned) { front=other==id; break; }
    }
    int clicked=button("",x,5,28,28,0);
    if(clicked) { if(front) windows.editors[id].visible=0; else windows_focus(&windows,id); }
    Color c=ink,cut=hover(x,5,28,28)?(Color){65,75,85,255}:cell; int left=x+4;
    if(id==0) for(int row=0;row<3;row++) {
        DrawRectangle(left,11+row*6,4,3,c);
        for(int step=0;step<4;step++) DrawRectangle(left+7+step*4,11+row*6,2,3,c);
    }
    if(id==1) for(int row=0;row<3;row++) {
        DrawLine(left,11+row*6,left+19,11+row*6,Fade(c,.4f));
        DrawRectangle(left+(row%2)*7,12+row*6,12,3,c);
    }
    if(id==2) {
        DrawRectangle(left,10,21,18,c);
        for(int key=1;key<7;key++) DrawLine(left+key*3,10,left+key*3,27,cut);
        for(int key=0;key<6;key++) if(key!=2) DrawRectangle(left+key*3+2,10,2,10,cut);
    }
    if(id==3) for(int strip=0;strip<3;strip++) {
        int xx=left+3+strip*7; DrawLine(xx,10,xx,27,c);
        DrawRectangle(xx-2,12+strip*4,5,3,c);
    }
    if(hover(x,5,28,28)) snprintf(status,sizeof status,"%s: %s",names[id],front?"hide this window":"show / bring this window to the front");
    return clicked;
}
static void open_context(int kind,int target,Vector2 position) {
    context_kind=kind; context_target=target; context_position=position; context_opened=1; input_enabled=0;
}
static void add_sampler(void);
static void open_popup(int kind) {
    pattern_popup=kind; popup_opened=1; popup_drag=0;
    popup_position[kind]=kind==1?(Vector2){632,38}:(Vector2){-1,-1};
}
static void begin_rename(void) {
    rename_pattern=pattern; snprintf(rename_text,sizeof rename_text,"%s",project.pattern_names[pattern]);
    rename_select_all=1; open_popup(2);
}
static void begin_number(float *value,float low,float high,float initial,const char *name) {
    number_target=value; number_low=low; number_high=high; number_default=initial; number_name=name;
    snprintf(number_text,sizeof number_text,"%.6g",*value); rename_select_all=1; open_popup(5);
}
static void text_input(char *text,size_t capacity) {
    if(IsKeyDown(KEY_LEFT_CONTROL) && IsKeyPressed(KEY_A)) rename_select_all=1;
    if(IsKeyDown(KEY_LEFT_CONTROL) && IsKeyPressed(KEY_V)) {
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
        if(ch<32 || ch==127 || IsKeyDown(KEY_LEFT_CONTROL)) continue;
        if(rename_select_all) { text[0]=0; rename_select_all=0; }
        int bytes; const char *utf8=CodepointToUTF8(ch,&bytes); size_t n=strlen(text);
        if(n+bytes<capacity) { memcpy(text+n,utf8,bytes); text[n+bytes]=0; }
    }
}
static void draw_popup(void) {
    if(!pattern_popup) { popup_drag=0; return; }
    input_enabled=1;
    int folder=pattern_popup==3,help=pattern_popup==4,number=pattern_popup==5,w=pattern_popup==6?320:help?580:number?360:folder?620:260,h=help?376:pattern_popup==1?project.pattern_count*24+84:116;
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
    DrawRectangle(x+3,y+3,w,h,Fade(BLACK,.5f)); DrawRectangle(x,y,w,h,panel); DrawRectangleLines(x,y,w,h,muted);
    DrawRectangle(x,y,w,TITLE,cell);
    if(button("x",x+w-24,y+3,20,18,0)) { pattern_popup=0; popup_drag=0; return; }
    if(hover(x,y,w-26,TITLE)) snprintf(status,sizeof status,"Drag this title bar to move the dialog");
    if(hover(x,y,w-26,TITLE) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) { popup_drag=1; popup_offset=(Vector2){mouse.x-x,mouse.y-y}; }
    if(IsKeyPressed(KEY_ESCAPE)) { pattern_popup=0; popup_drag=0; return; }
    if(!popup_drag && !popup_opened && IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && !hover(x,y,w,h)) { pattern_popup=0; return; }
    input_enabled=!popup_drag;
    if(pattern_popup==6) {
        label("Add instrument",x+8,y+6,13,ink);
        if(button("Sampler",x+12,y+38,w-24,30,0)) { add_sampler(); pattern_popup=0; }
        return;
    }
    if(help) {
        label("Help - Keybindings",x+8,y+6,13,ink);
        const char *bindings[]={
            "Space - Play / stop from the ruler start marker",
            "Keys on: Z row - C3 white keys; S/D/G/H/J - black keys",
            "Keys on: Q row - C4 white keys; 2/3/5/6/7 - black keys",
            "Delete / Supr - Delete selected clips / notes (focused editor)",
            "Shift-click - Add/remove a clip or note from selection",
            "Shift-drag empty grid - Add a selection rectangle",
            "Up/Down or k/j - Browser selection and sample preview",
            "Up/Down or k/j over pattern selector - Change pattern",
            "Left/h - Collapse Browser folder or select its parent",
            "Right/l - Expand Browser folder or select its first child",
            "Escape - Close a dialog or cancel a sample drag",
            "Enter - Apply a name, folder path or numeric value",
            "Ctrl+A / Ctrl+V - Select all / paste in a text field",
            "Backspace - Delete text; Left/Right - Clear text selection"
        };
        for(unsigned i=0;i<sizeof bindings/sizeof *bindings;i++) label(bindings[i],x+12,y+34+i*24,13,ink);
        return;
    }
    if(pattern_popup==1) {
        label("Patterns",x+8,y+6,13,ink);
        for(int p=0;p<project.pattern_count;p++) {
            if(button(TextFormat("%d  %s",p+1,fit_text(project.pattern_names[p],w-42,13)),x+4,y+28+p*24,w-8,23,pattern==p)) {
                pattern=p; reset=1; pattern_popup=0;
            }
        }
        if(button("Rename selected pattern...",x+4,y+32+project.pattern_count*24,w-8,23,0)) begin_rename();
        if(button("+ New pattern",x+4,y+56+project.pattern_count*24,w-8,23,0)) {
            if(project.pattern_count<PATTERNS) {
                pattern=project.pattern_count++; memset(project.notes[pattern],0,sizeof project.notes[pattern]);
                project.pattern_steps[pattern]=STEPS; memset(piano_channels[pattern],0,sizeof piano_channels[pattern]);
                rack_view[pattern]=0; reset=1; pattern_popup=0;
            } else snprintf(status,sizeof status,"This prototype supports %d patterns.",PATTERNS);
        }
        return;
    }
    char *text=number?number_text:folder?folder_text:rename_text; size_t capacity=number?sizeof number_text:folder?sizeof folder_text:sizeof rename_text;
    label(number?number_name:folder?"Add Browser folder":TextFormat("Rename pattern %d",rename_pattern+1),x+8,y+6,13,ink);
    DrawRectangle(x+8,y+32,w-16,28,bg); DrawRectangleLines(x+8,y+32,w-16,28,cell);
    if(rename_select_all) DrawRectangle(x+12,y+38,fminf(w-26,MeasureText(text,13)),16,cell);
    if(hover(x+8,y+32,w-16,28)) snprintf(status,sizeof status,number?"Enter an exact value using a decimal dot or comma":folder?"Enter the folder path to add as a Browser root":"Enter the pattern name");
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
            if(!parsed_any || *end || !isfinite(parsed) || parsed<number_low || parsed>number_high) { snprintf(status,sizeof status,"Enter a number from %.6g to %.6g.",number_low,number_high); return; }
            *number_target=parsed;
        } else if(folder) {
            if(!browser_add(&browser,text)) { snprintf(status,sizeof status,"Cannot add folder: check its path, permissions or the eight-folder limit."); return; }
            snprintf(status,sizeof status,"Browser folder saved.");
        } else {
            size_t n=strlen(text); while(n && text[n-1]==' ') text[--n]=0;
            char *name=text; while(*name==' ') name++;
            if(!*name) { snprintf(status,sizeof status,"Pattern name cannot be empty."); return; }
            snprintf(project.pattern_names[rename_pattern],PATTERN_NAME,"%s",name);
        }
        pattern_popup=0;
    }
}
static int import_sample(const char *path,int c) {
    char full[PATH_MAX]; Sample s;
    if(!realpath(path,full) || strlen(full)>=sizeof project.paths[c] || !sample_load(full,&s)) { snprintf(status,sizeof status,"Cannot load sample (maximum 60 seconds)."); return 0; }
    Sample cooked;
    if(!sample_process(s,project.sampler[c],&cooked)) { free(s.data); snprintf(status,sizeof status,"Sample processing failed; channel unchanged"); return 0; }
    Sample old=samples[c],raw=originals[c]; audio_sample(c,cooked); samples[c]=cooked; originals[c]=s; free(old.data); free(raw.data);
    sample_generation[c]++; sampler_generation[c]=sample_generation[c]; sampler_applied[c]=project.sampler[c];
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
            audition_low[i]=fminf(audition_low[i],next.data[j]);
            audition_high[i]=fmaxf(audition_high[i],next.data[j]);
        }
    }
    snprintf(status,sizeof status,"Preview: %.140s",GetFileName(path));
}
static void rack_reveal_last(void) {
    Editor *e=&windows.editors[0];
    e->rect.h=fminf(GetScreenHeight()/fminf(GetScreenWidth()/1200.f,GetScreenHeight()/675.f)-66,92+project.channel_count*28);
    rack_scroll=fmaxf(0,project.channel_count-(int)((e->rect.h-92)/28));
}
static void add_sampler(void) {
    int c=project.channel_count;
    if(c>=CHANNELS) { snprintf(status,sizeof status,"Channel Rack limit: %d channels.",CHANNELS); return; }
    project.channel_count++; project.sampler[c]=(Sampler){.time=1,.length=1}; project.volume[c]=.7f; project.pan[c]=0; project.mute[c]=0; project.route[c]=0;
    snprintf(project.paths[c],sizeof project.paths[c],"%s",SAMPLE_EMPTY);
    memset(project.channel_names[c],0,PATTERN_NAME); snprintf(project.channel_names[c],PATTERN_NAME,"Sampler");
    for(int pat=0;pat<PATTERNS;pat++) {
        memset(project.notes[pat][c],0,sizeof project.notes[pat][c]); memset(note_selected[pat][c],0,NOTES); piano_channels[pat][c]=0;
    }
    Sample old=samples[c],raw=originals[c]; originals[c]=samples[c]=(Sample){0}; audio_channels(&project,samples); free(old.data); free(raw.data);
    sample_generation[c]++; sampler_generation[c]=sample_generation[c]; sampler_applied[c]=project.sampler[c];
    channel=instrument_channel=c; browser_focus=0; rack_reveal_last(); windows_focus(&windows,4);
}
static void drop_sample(const char *path) {
    int target=windows_hit(&windows,mouse.x,mouse.y);
    if(target==4 && mouse.y>=windows.editors[4].rect.y+TITLE) {
        if(import_sample(path,instrument_channel)) {
            channel=instrument_channel; browser_focus=0; audio_channels(&project,samples); windows_focus(&windows,4);
        }
        return;
    }
    Rect r=windows.editors[0].rect;
    if(windows_hit(&windows,mouse.x,mouse.y)!=0 || mouse.y<r.y+TITLE) {
        snprintf(status,sizeof status,"Drop onto a sampler to load it, a Rack row to replace it, or empty Rack space to add a channel."); return;
    }
    int c=rack_scroll+(mouse.y-r.y-52)/28;
    int visible=fmaxf(1,(r.h-92)/28);
    int replace=mouse.y>=r.y+52 && mouse.y<r.y+52+visible*28 && c<project.channel_count;
    if(!replace) c=project.channel_count;
    if(c>=CHANNELS) { snprintf(status,sizeof status,"Channel Rack limit: %d channels.",CHANNELS); return; }
    if(import_sample(path,c)) {
        if(!replace) {
            project.channel_count++;
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
    audio_channels(&project,samples); free(old.data); free(raw.data); reset=1;
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
    sample_epoch++;
    playing=0; pattern=(int)fminf(pattern,next.pattern_count-1); audio_update(&next,0,song,pattern,1,output_volume,0);
    for(int c=0;c<CHANNELS;c++) {
        Sample old=samples[c],raw=originals[c]; audio_sample(c,processed[c]); samples[c]=processed[c]; originals[c]=fresh[c]; free(old.data); free(raw.data);
        sample_generation[c]++; sampler_generation[c]=sample_generation[c]; sampler_applied[c]=next.sampler[c];
    }
    project=next; memcpy(project_path,filename,strlen(filename)+1);
    project.insert_count=INSERTS; rack_scroll=0; channel=fmaxf(0,fminf(channel,project.channel_count-1));
    piano_channel=instrument_channel=channel;
    for(int i=0;i<PATTERNS;i++) { piano_span[i]=fmaxf(STEPS,next.pattern_steps[i]); piano_pan[i]=0; piano_range[i]=0; }
    memset(note_selected,0,sizeof note_selected); piano_gesture=PIANO_IDLE; playlist_start=0; memset(piano_start,0,sizeof piano_start); marker_drag=-1;
    memset(piano_channels,0,sizeof piano_channels); mixer_selected=project.route[channel]; mixer_scroll=0;
    piano_key_drag=0; piano_key=-1;
    note_drag=NULL; velocity_drag=-1; control_drag=NULL; route_drag=-1; rack_paint=-1; cable_drag=-1;
    memset(&arrangement,0,sizeof arrangement); arrangement.source_pattern=-1; arrangement.source_steps=STEPS; arrangement.zoom=1; playlist_pan=0; playlist_vpan=0; track_scroll=0; memset(rack_view,0,sizeof rack_view); memset(rack_range,0,sizeof rack_range);
    snprintf(status,sizeof status,"Loaded %.150s",filename); reset=1; return 1;
failed:
    for(int c=0;c<CHANNELS;c++) { free(fresh[c].data); free(processed[c].data); }
    snprintf(status,sizeof status,"Project unchanged: a referenced sample is missing or invalid."); return 0;
}
static void capture_control(float *value,float low,float high,int fader) {
    control_drag=value; control_low=low; control_high=high; control_fader=fader; input_enabled=0;
}
static void knob(int x,int y,float *value,float low,float high,float initial,const char *name) {
    int over=hover(x-9,y-9,18,18);
    if(over || control_drag==value) snprintf(status,sizeof status,"%s: %.4g | Drag up/down or wheel; right-click for number entry",name,*value);
    if(over) {
        if(IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) begin_number(value,low,high,initial,name);
        if(IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) capture_control(value,low,high,0);
        *value=fmaxf(low,fminf(high,*value+GetMouseWheelMove()*(high-low)/20));
    }
    float angle=2.4f+(*value-low)/(high-low)*4.6f;
    DrawCircleLines(x,y,6,control_drag==value || over?accent:muted);
    DrawLine(x,y,x+cosf(angle)*5,y+sinf(angle)*5,accent);
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
    int visible=fmaxf(1,(windows.editors[0].rect.h-92)/28);
    if(x<200 || x>=width-26 || y<52 || y>=52+visible*28) { rack_paint_channel=-1; return; }
    int c=rack_scroll+(y-52)/28; float step=floorf(rack_view[pattern]+(x-200)/RACK_STEP_WIDTH);
    if(c>=project.channel_count || rack_melodic(c)) { rack_paint_channel=-1; return; }
    float from=rack_paint_channel==c?rack_paint_step:step;
    for(int i=0;i<=fabsf(step-from) && i<NOTES;i++) {
        float start=fminf(from,step)+i;
        Note *n=note_at(&project,pattern,c,start,60);
        if(rack_paint==MOUSE_BUTTON_RIGHT) { if(n) n->velocity=0; }
        else if(!n) { pattern_extend(start+1); note_add(&project,pattern,c,start,60,0); }
    }
    rack_paint_channel=c; rack_paint_step=step;
}
static void rack(float width,float height) {
    label(fit_text(project.pattern_names[pattern],width-170,12),10,32,12,muted);
    float stepw=RACK_STEP_WIDTH,gridw=width-226,view=rack_view[pattern],span=gridw/stepw;
    rack_range[pattern]=fmaxf(rack_range[pattern],fmaxf(view+span*2,project.pattern_steps[pattern]+span));
    for(int i=0;i<span/STEPS+2;i++) {
        float bar=(floorf(view/STEPS)+i)*STEPS,x=200+(bar-view)*stepw;
        if(x>=200 && x<width-26) label(TextFormat("%.0f",bar/STEPS+1),x+2,32,11,muted);
    }
    int visible=fmaxf(1,(height-92)/28);
    if(hover(0,48,width,height-66)) rack_scroll-=GetMouseWheelMove();
    rack_scroll=fmaxf(0,fminf(fmaxf(0,project.channel_count-visible),rack_scroll));
    for(int c=rack_scroll;c<project.channel_count && c<rack_scroll+visible;c++) {
        int row=52+(c-rack_scroll)*28;
        if(hover(6,row,12,22) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) project.mute[c]^=1;
        DrawCircle(12,row+11,3,project.mute[c]?muted:accent);
        if(hover(6,row,12,22)) snprintf(status,sizeof status,"%s: mute/unmute this channel",project.channel_names[c]);
        knob(28,row+11,&project.pan[c],-1,1,0,"Channel pan"); knob(46,row+11,&project.volume[c],0,1,.7f,"Channel volume");
        if(button(TextFormat("%d",project.route[c]),58,row,24,22,route_drag==c)) {
            route_drag=c; route_start=project.route[c]; route_y=mouse.y+windows.editors[0].rect.y;
            channel=c; mixer_selected=project.route[c]; input_enabled=0;
        }
        if(hover(58,row,24,22)) snprintf(status,sizeof status,"%s mixer destination: drag up/down; 0 = Master",project.channel_names[c]);
        if(button(fit_text(project.channel_names[c],80,13),84,row,96,22,channel==c)) {
            channel=instrument_channel=c; windows_focus(&windows,4);
        }
        if(hover(84,row,96,22) && IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) { channel=c; open_context(2,c,(Vector2){mouse.x+windows.editors[0].rect.x,mouse.y+windows.editors[0].rect.y}); }
        if(hover(84,row,96,22)) snprintf(status,sizeof status,"%s: open instrument; right-click for Piano Roll, mute or delete; Keys: Z/Q white, S/2 black",project.channel_names[c]);
        DrawRectangle(187,row+2,3,18,channel==c?accent:muted);
        int melodic=rack_melodic(c);
        if(melodic) {
            DrawRectangle(200,row+2,gridw,20,bg);
            float active=fmaxf(0,fminf(gridw,(edit_steps()-view)*stepw));
            if(active<gridw) DrawRectangle(200+active,row+2,gridw-active,20,(Color){40,43,46,255});
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
            DrawRectangle(left,row+2,right-left,18,!available?(Color){40,43,46,255}:n?accent:fmodf(floorf(step/4),2)?cell:(Color){58,64,71,255});
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
    int add_y=52+(int)fminf(visible,project.channel_count-rack_scroll)*28;
    if(add_y+22<=height-18) {
        if(sample_drag[0] && sample_moved && mouse.y>=add_y && windows_hit(&windows,mouse.x+windows.editors[0].rect.x,mouse.y+windows.editors[0].rect.y)==0) {
            DrawRectangle(84,add_y,width-100,22,Fade(accent,.18f)); DrawRectangleLines(84,add_y,width-100,22,Fade(accent,.5f));
            label("Drop to add channel",92,add_y+5,11,accent);
        } else if(button("+",84,add_y,96,22,0)) open_popup(6);
        if(hover(84,add_y,width-100,22)) snprintf(status,sizeof status,"Add instrument: click + to choose a plugin, or drop a sample here");
    }
    float area=visible*28,maximum=fmaxf(0,project.channel_count-visible),thumb=fminf(area,fmaxf(24,area*visible/fmaxf(1,project.channel_count))),travel=area-thumb;
    float y=52+(maximum?rack_scroll/maximum*travel:0);
    DrawRectangle(width-12,52,8,area,bg); DrawRectangle(width-12,y,8,thumb,rack_vdrag || hover(width-14,52,14,area)?accent:muted);
    if(hover(width-14,52,14,area)) {
        snprintf(status,sizeof status,"Drag to scroll Rack channels; wheel over rows also scrolls");
        if(IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            if(mouse.y<y || mouse.y>y+thumb) rack_scroll=fmaxf(0,fminf(maximum,(mouse.y-52-thumb/2)/fmaxf(1,travel)*maximum));
            rack_vy=mouse.y+windows.editors[0].rect.y; rack_vstart=rack_scroll; rack_vscale=maximum/fmaxf(1,travel); rack_vdrag=1; input_enabled=0;
        }
    }
    float total=rack_range[pattern],hthumb=timeline_thumb(gridw,span,total),htravel=gridw-hthumb,hmaximum=total-span;
    float hx=200+(hmaximum>0?rack_view[pattern]/hmaximum*htravel:0);
    DrawRectangle(200,height-16,gridw,8,bg); DrawRectangle(hx,height-16,hthumb,8,rack_hdrag || hover(200,height-19,gridw,14)?accent:muted);
    if(hover(200,height-19,gridw,14)) {
        snprintf(status,sizeof status,"Drag to scroll steps horizontally; grey steps extend the pattern when painted");
        if(IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            if(mouse.x<hx || mouse.x>hx+hthumb) rack_view[pattern]=fmaxf(0,fminf(hmaximum,(mouse.x-200-hthumb/2)/fmaxf(1,htravel)*hmaximum));
            rack_hx=mouse.x+windows.editors[0].rect.x; rack_hstart=rack_view[pattern]; rack_hscale=hmaximum/fmaxf(1,htravel); rack_hdrag=1; input_enabled=0;
        }
    }
}
static Color pattern_color(int id) { return id%2?(Color){137,91,80,255}:(Color){113,86,137,255}; }
/* Icons use raylib primitives, without a font or image dependency. */
static int tool_button(int tool,int active,int x,const char *tip) {
    int over=hover(x,28,28,26); Color c=active==tool?bg:ink;
    DrawRectangle(x,28,28,26,active==tool?accent:over?muted:cell);
    if(tool==PENCIL) { DrawLineEx((Vector2){x+8,46},(Vector2){x+20,34},4,c); DrawTriangle((Vector2){x+5,49},(Vector2){x+10,47},(Vector2){x+7,44},c); }
    if(tool==BRUSH) { DrawLineEx((Vector2){x+15,41},(Vector2){x+22,33},3,c); DrawRectangle(x+7,41,10,7,c); DrawLine(x+6,49,x+16,49,c); }
    if(tool==SELECT) { for(int i=0;i<16;i+=5) { DrawLine(x+6+i,34,x+9+i,34,c); DrawLine(x+6+i,48,x+9+i,48,c); DrawLine(x+6,34+i,x+6,37+i,c); DrawLine(x+22,34+i,x+22,37+i,c); } }
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
    DrawRectangle(x+3,y+3,144,SNAP_COUNT*24+8,Fade(BLACK,.5f)); DrawRectangle(x,y,144,SNAP_COUNT*24+8,panel); DrawRectangleLines(x,y,144,SNAP_COUNT*24+8,muted);
    int id=snap_menu-1;
    for(int i=0;i<SNAP_COUNT;i++) if(button(snap_names[i],x+4,y+4+i*24,136,24,snap_mode[id]==i) && !snap_opened) { snap_mode[id]=i; snap_menu=0; break; }
}
static void timeline_marker(int id,float start,float span,float gx,float y,float width,float height,float q) {
    float *point=id==1?&playlist_start:&piano_start[pattern];
    if(hover(gx,y,width,height)) {
        snprintf(status,sizeof status,"Click or drag the ruler to set playback start; Space starts/stops here; wheel zoom focuses on this marker");
        if(IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) { if(song!=(id==1)) { song=id==1; reset=1; } *point=fmaxf(0,roundf((start+(mouse.x-gx)/width*span)/q)*q); marker_drag=id; if(!playing) reset=1; input_enabled=0; }
    }
    float x=gx+(*point-start)/span*width;
    if(x>=gx && x<=gx+width) {
        DrawTriangle((Vector2){x-5,y+2},(Vector2){x,y+height-2},(Vector2){x+5,y+2},marker_drag==id || hover(x-6,y,12,height)?ink:accent);
    }
}
static void timeline_grid(float start,float span,float snap,float gx,float y,float width,float height) {
    float pixels=width/span,spacing=snap*fmaxf(1,ceilf(6/(snap*pixels)));
    for(int i=0;i<width/(spacing*pixels)+2;i++) {
        float step=(floorf(start/spacing)+i)*spacing,x=gx+(step-start)*pixels;
        int bar=fabsf(step/STEPS-roundf(step/STEPS))<.0001f,beat=fabsf(step/4-roundf(step/4))<.0001f;
        if(x>=gx && x<=gx+width) DrawLineEx((Vector2){x,y},(Vector2){x,y+height},1,Fade(muted,bar?.45f:(beat?.12f:.06f)*fminf(1,spacing*pixels/12)));
    }
}
static void follow_view(float *start,float span,double position) {
    if(!follow_playhead || !playing || captured() || windows.grab>=0) return;
    if(position<*start || position>*start+span) *start=fmax(0,position-span*.25);
    else if(position>*start+span*.7)
        *start+=(position-span*.7-*start)*(1-expf(-12*GetFrameTime()));
}
static void playlist(float width,float height,float scale) {
    static double last_click=-1;
    static int last_lane=-1,last_clip=-1;
    static Vector2 last_position;
    Rect rect=windows.editors[1].rect;
    int gx=92,gy=82,rowh=52; float gridw=width-gx-24,track_area=height-gy-42;
    track_scroll=fmaxf(0,fminf(LANES-track_area/rowh,track_scroll));
    if(hover(gx,62,gridw,height-104)) {
        float span=BARS/arrangement.zoom,wheel=GetMouseWheelMove();
        if(wheel && (playlist_start<arrangement.view_start || playlist_start>arrangement.view_start+span)) arrangement.view_start=fmaxf(0,playlist_start-span*.5f);
        arrangement_zoom(&arrangement,wheel,(playlist_start-arrangement.view_start)/span);
    }
    float span=BARS/arrangement.zoom,barw=gridw/span;
    if(song) follow_view(&arrangement.view_start,span,visual_step/STEPS);
    arrangement.range=fmaxf(arrangement.range,fmaxf(arrangement.view_start+span*2,song_steps(&project)/STEPS+span));
    float ruler_step=fmaxf(1,powf(2,ceilf(log2f(48/barw))));
    arrangement.snap=snap_interval(snap_mode[0],barw/STEPS);
    float pointer_bar=arrangement.view_start+(mouse.x-gx)/barw;
    int edge_lane=-1,edge_bar=-1;
    if(hover(gx,gy,gridw,fminf(rowh*LANES,height-42-gy))) {
        int lane=track_scroll+(mouse.y-gy)/rowh,hit=arrangement_hit(&project,lane,pointer_bar);
        if(hit>=0 && mouse.x>=gx+(project.clip_starts[lane][hit]-arrangement.view_start+clip_length(&project,lane,hit)/(float)STEPS)*barw-7) { edge_lane=lane; edge_bar=hit; SetMouseCursor(MOUSE_CURSOR_RESIZE_EW); }
    }
    if(arrangement.gesture==SIZE_CLIP) { edge_lane=arrangement.lane; edge_bar=arrangement.bar; SetMouseCursor(MOUSE_CURSOR_RESIZE_EW); }
    const char *tips[]={"Pencil: place one clip or drag a clip to move it","Brush: drag to paint copies of the last clicked clip","Select: drag a rectangle, then drag the selected clips together"};
    for(int i=0;i<3;i++) if(tool_button(i,arrangement.tool,8+i*32,tips[i])) arrangement.tool=i;
    snap_button(0,width-184);
    label(fit_text(project.pattern_names[pattern],width-310,12),118,35,12,muted);
    label("Tracks",8,63,12,muted);
    zoom_label(arrangement.zoom*100,width-62,35);
    BeginScissorMode((rect.x+gx)*scale,(rect.y+60)*scale,gridw*scale,20*scale);
    for(int i=0;i<gridw/(ruler_step*barw)+2;i++) { float b=(floorf(arrangement.view_start/ruler_step)+i)*ruler_step,x=gx+(b-arrangement.view_start)*barw; if(i==0 || (x>=gx && x<gx+gridw)) label(TextFormat("%.0f",i==0 && x<gx?floorf(arrangement.view_start)+1:b+1),fmaxf(gx,x)+8,64,11,muted); }
    BeginScissorMode(rect.x*scale,rect.y*scale,rect.w*scale,rect.h*scale);
    for(int l=(int)track_scroll;l<LANES;l++) {
        int y=gy+(l-track_scroll)*rowh,selected=0;
        if(y>=height-42) break;
        for(int b=0;b<CLIPS;b++) if(arrangement.selected[l][b]) selected=1;
        BeginScissorMode(rect.x*scale,(rect.y+gy)*scale,gx*scale,track_area*scale);
        DrawRectangle(0,y,gx-2,rowh-1,selected?Fade(accent,.35f):cell); label(TextFormat("Track %d",l+1),8,y+8,12,ink);
        BeginScissorMode((rect.x+gx)*scale,(rect.y+gy)*scale,gridw*scale,track_area*scale);
        DrawRectangle(gx,y,gridw,rowh-1,(Color){39,45,52,255});
        timeline_grid(arrangement.view_start*STEPS,span*STEPS,arrangement.snap,gx,y,gridw,rowh-1);
        for(int b=0;b<CLIPS;b++) if(project.clips[l][b]) {
            int pat=project.clips[l][b]-1; float x=gx+(project.clip_starts[l][b]-arrangement.view_start)*barw,w=clip_length(&project,l,b)*barw/STEPS;
            if(x>gx+gridw || x+w<gx) continue;
            float left=fmaxf(gx,x),right=fminf(gx+gridw,x+w-1);
            DrawRectangleRec((Rectangle){left,y,fmaxf(1,right-left),rowh-1},pattern_color(pat)); DrawRectangleRec((Rectangle){left,y,fmaxf(1,right-left),14},Fade(WHITE,.08f));
            const char *name=project.pattern_names[pat]; if(!strcmp(name,TextFormat("Pattern %d",pat+1))) name=TextFormat("P%d",pat+1);
            if(x>=gx) label(fit_text(name,fminf(gridw,w-9),10),x+3,y+2,10,ink);
            /* Preview uses the timeline scale, leaving extended space empty. */
            float visible_steps=clip_length(&project,l,b);
            for(int c=0;c<project.channel_count;c++) for(int n=0;n<NOTES;n++) {
                Note note=project.notes[pat][c][n]; if(!note.velocity || note.start>=visible_steps) continue;
                float yy=y+19+c*6+fmaxf(-3,fminf(3,(60-(int)note.pitch)*.3f));
                float nx=x+note.start*barw/STEPS+1,nw=fmaxf(1,fminf(note.length?note.length:1,visible_steps-note.start)*barw/STEPS-2);
                if(nx<=right && nx+nw>=left) DrawRectangle(fmaxf(left,nx),yy,fmaxf(1,fminf(right,nx+nw)-fmaxf(left,nx)),2,ink);
            }
            if(x+w>=gx && x+w<=gx+gridw) DrawRectangle(x+w-5,y+15,3,rowh-18,edge_lane==l && edge_bar==b?accent:Fade(ink,.45f));
            if(arrangement.gesture==MOVE_CLIPS?arrangement.moved[l][b]:arrangement.selected[l][b]) DrawRectangleLinesEx((Rectangle){left+1,y+1,fmaxf(1,right-left-2),rowh-3},2,accent);
        }
        BeginScissorMode(rect.x*scale,rect.y*scale,rect.w*scale,rect.h*scale);
    }
    if(hover(0,gy,gx+gridw,fminf(rowh*LANES,height-42-gy))) {
        float bx=mouse.x<gx?-1:arrangement.view_start+(mouse.x-gx)/barw,ly=track_scroll+(mouse.y-gy)/rowh; int hit=arrangement_hit(&project,(int)ly,bx);
        int edge=hit>=0 && mouse.x>=gx+(project.clip_starts[(int)ly][hit]-arrangement.view_start+clip_length(&project,(int)ly,hit)/(float)STEPS)*barw-7;
        if(edge) { SetMouseCursor(MOUSE_CURSOR_RESIZE_EW); snprintf(status,sizeof status,"Drag this clip edge to resize it"); }
        else if(mouse.x<gx) snprintf(status,sizeof status,"Track %d: drag to select tracks; right scrollbar: scroll",(int)ly+1);
        else if(hit>=0) snprintf(status,sizeof status,"Pattern clip: double-click to open Channel Rack; drag to move; right-click to erase");
        else snprintf(status,sizeof status,"Playlist: %s; Shift adds to selection, right-drag erases",arrangement.tool==PENCIL?"place or move a clip":arrangement.tool==BRUSH?"paint copies of the current pattern":"select and move clips");
        if(IsMouseButtonPressed(MOUSE_BUTTON_LEFT) || IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
            int plain_left=IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && !IsKeyDown(KEY_LEFT_SHIFT) && !IsKeyDown(KEY_RIGHT_SHIFT) && !edge && hit>=0;
            double now=GetTime();
            if(plain_left && last_lane==(int)ly && last_clip==hit && now-last_click<.35 && fabsf(mouse.x-last_position.x)<5 && fabsf(mouse.y-last_position.y)<5) {
                pattern=project.clips[(int)ly][hit]-1; reset=1;
                arrangement.gesture=IDLE; windows_focus(&windows,0); last_click=-1;
                input_enabled=0; return;
            }
            last_click=plain_left?now:-1; last_lane=(int)ly; last_clip=hit; last_position=mouse;
            arrangement_press(&arrangement,&project,bx,ly,IsMouseButtonPressed(MOUSE_BUTTON_RIGHT),edge,pattern,IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT));
            if(arrangement.source_pattern>=0 && arrangement.source_pattern!=pattern) { pattern=arrangement.source_pattern; reset=1; }
            input_enabled=0;
        }
    }
    BeginScissorMode((rect.x+gx)*scale,(rect.y+gy)*scale,gridw*scale,(height-gy-42)*scale);
    if(arrangement.gesture==BOX_SELECT) {
        float x=gx+(fminf(arrangement.x,arrangement.now_x)-arrangement.view_start)*barw,y=gy+(fminf(arrangement.y,arrangement.now_y)-track_scroll)*rowh;
        Rectangle box={x,y,fabsf(arrangement.x-arrangement.now_x)*barw,fabsf(arrangement.y-arrangement.now_y)*rowh};
        DrawRectangleRec(box,Fade(accent,.15f)); DrawRectangleLinesEx(box,1,accent);
    }
    BeginScissorMode(rect.x*scale,rect.y*scale,rect.w*scale,rect.h*scale);
    if(playing && song) { float x=gx+(visual_step/STEPS-arrangement.view_start)*barw; if(x>=gx && x<=gx+gridw) DrawLineEx((Vector2){x,gy},(Vector2){x,height-42},1.5f,accent); }
    float vthumb=track_area*track_area/(LANES*rowh),vy=gy+track_scroll/(float)LANES*track_area;
    DrawRectangle(width-18,gy,12,track_area,bg); DrawRectangle(width-18,vy,12,fmaxf(12,vthumb),playlist_vpan || hover(width-18,gy,12,track_area)?accent:muted);
    if(hover(width-18,gy,12,track_area)) snprintf(status,sizeof status,"Drag to scroll through the 100 Playlist tracks");
    if(hover(width-18,gy,12,track_area) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        if(mouse.y<vy || mouse.y>vy+fmaxf(12,vthumb)) track_scroll=fmaxf(0,fminf(LANES-track_area/rowh,(mouse.y-gy-vthumb/2)/track_area*LANES));
        playlist_vpan=1; playlist_vpan_y=mouse.y+rect.y; playlist_vpan_start=track_scroll;
    }
    timeline_marker(1,arrangement.view_start,span,gx,60,gridw,20,arrangement.snap/STEPS);
    float thumbw=timeline_thumb(gridw,span,arrangement.range),travel=gridw-thumbw,maximum=arrangement.range-span,thumbx=gx+(maximum>0?arrangement.view_start/maximum*travel:0);
    DrawRectangle(gx,height-37,gridw,13,bg); DrawRectangle(thumbx,height-37,thumbw,13,playlist_pan || hover(gx,height-37,gridw,13)?accent:muted);
    if(hover(gx,height-37,gridw,13)) snprintf(status,sizeof status,"Drag to scroll along the Playlist timeline");
    if(hover(gx,height-37,gridw,13) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        if(mouse.x<thumbx || mouse.x>thumbx+thumbw) arrangement.view_start=fmaxf(0,fminf(maximum,(mouse.x-gx-thumbw/2)/fmaxf(1,travel)*maximum));
        playlist_pan=1; playlist_pan_x=mouse.x+rect.x; playlist_pan_start=arrangement.view_start; playlist_pan_range=maximum/fmaxf(1,travel);
    }
    label(fit_text("Wheel: zoom | Scrollbars: pan | Shift: add selection | Right-drag: erase | Supr: delete",width-24,11),8,height-17,11,muted);

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
    Rect r=windows.editors[2].rect; float rh=(r.h-162)/25;
    int pitch=x>=4 && x<58 && y>=68 && y<68+rh*25?piano_top-(int)((y-68)/rh):-1;
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
    int gx=60,gy=68; float gridw=width-84,rh=(height-162)/25;
    float extent=piano_span[pattern];
    Rect r=windows.editors[2].rect; float scale=fminf(GetScreenWidth()/1200.f,GetScreenHeight()/675.f);
    const char *tips[]={"Pencil: draw one note; drag note bodies to move; edges to resize","Brush: drag to paint notes on the snap grid","Select: drag a rectangle; drag selected notes together; Shift adds; Supr deletes"};
    for(int i=0;i<3;i++) if(tool_button(i,piano_tool,8+i*32,tips[i])) piano_tool=i;
    label(fit_text(TextFormat("%s / %s",project.channel_names[channel],project.pattern_names[pattern]),width-344,12),112,30,12,muted);
    zoom_label(STEPS/piano_span[pattern]*100,width-205,32);
    snap_button(1,width-120);
    int grid_over=hover(gx,gy,gridw,rh*25);
    float wheel=grid_over?GetMouseWheelMove():0;
    if(wheel) {
        if(piano_start[pattern]<piano_pan[pattern] || piano_start[pattern]>piano_pan[pattern]+piano_span[pattern]) piano_pan[pattern]=fmaxf(0,piano_start[pattern]-piano_span[pattern]*.5f);
        timeline_zoom(&piano_span[pattern],&piano_pan[pattern],wheel,(piano_start[pattern]-piano_pan[pattern])/piano_span[pattern],STEPS);
    }
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
    DrawRectangle(4,gy,gx-6,rh*25,(Color){178,184,189,255});
    for(int row=0;row<25;row++) {
        int pitch=piano_top-row,pc=pitch%12,y=gy+row*rh;
        if(pc==1 || pc==3 || pc==6 || pc==8 || pc==10) DrawRectangle(4,y,34,rh-1,bg);
        else DrawLine(4,y,gx-2,y,muted);
        if(piano_key==pitch && piano_key_channel==channel) DrawRectangle(4,y,gx-6,rh-1,accent);
        else if(hover(4,y,gx-6,rh-1)) DrawRectangle(4,y,gx-6,rh-1,Fade(accent,.22f));
        if(pc==0) label(TextFormat("C%d",pitch/12-1),10,y+1,10,bg);
    }
    BeginScissorMode((r.x+gx)*scale,(r.y+gy)*scale,gridw*scale,rh*25*scale);
    for(int row=0;row<25;row++) {
        int pc=(piano_top-row)%12,y=gy+row*rh,black=pc==1 || pc==3 || pc==6 || pc==8 || pc==10;
        DrawRectangle(gx,y,gridw,rh-1,black?bg:cell);
        if(partial>end) { float left=fmaxf(gx,gx+(end-view)*cw),right=fminf(gx+gridw,gx+(partial-view)*cw); if(right>left) DrawRectangle(left,y,right-left,rh-1,(Color){40,43,46,255}); }
        if(pc==0) DrawLine(gx,y,gx+gridw,y,Fade(muted,.4f));
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
        DrawRectangle(left+1,y+1,fmaxf(1,right-left-2),fmaxf(2,rh-2),n==note_drag?(Color){169,231,208,255}:accent);
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
        DrawRectangleRec(box,Fade(accent,.15f)); DrawRectangleLinesEx(box,1,accent);
    }

    int vy=height-84; label("Velocity",6,vy+4,10,muted);
    BeginScissorMode((r.x+gx)*scale,(r.y+vy)*scale,gridw*scale,62*scale);
    DrawRectangle(gx,vy,gridw,62,cell);
    if(partial>end) { float left=fmaxf(gx,gx+(end-view)*cw),right=fminf(gx+gridw,gx+(partial-view)*cw); if(right>left) DrawRectangle(left,vy,right-left,62,(Color){40,43,46,255}); }
    for(int i=0;i<NOTES;i++) {
        Note n=project.notes[pattern][channel][i]; if(!n.velocity || n.start>=end) continue;
        float x=gx+(n.start-view)*cw;
        if(x<gx-10 || x>gx+gridw) continue;
        DrawRectangle(x+1,vy+62-n.velocity*.48f,fmaxf(2,fminf(cw*q-2,10)),n.velocity*.48f,accent);
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
    DrawRectangle(width-18,vypos,12,vthumb,piano_vdrag || hover(width-18,gy,12,varea)?accent:muted);
    if(hover(width-18,gy,12,varea)) {
        snprintf(status,sizeof status,"Drag to scroll higher or lower pitches");
        if(IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            if(mouse.y<vypos || mouse.y>vypos+vthumb) piano_top=127-(int)fmaxf(0,fminf(103,(mouse.y-gy-vthumb/2)/varea*128));
            piano_vscroll_y=mouse.y+r.y; piano_vscroll_start=127-piano_top; piano_vdrag=1; input_enabled=0;
        }
    }
    float thumb=timeline_thumb(gridw,extent,piano_range[pattern]),travel=gridw-thumb,maximum=piano_range[pattern]-extent,x=gx+(maximum>0?piano_pan[pattern]/maximum*travel:0);
    DrawRectangle(gx,height-18,gridw,8,bg); DrawRectangle(x,height-18,thumb,8,piano_scroll_drag || hover(gx,height-21,gridw,14)?accent:muted);
    if(hover(gx,height-21,gridw,14)) {
        snprintf(status,sizeof status,"Drag to pan the zoomed Piano Roll; wheel over notes to zoom; the timeline grows as you navigate");
        if(IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            if(mouse.x<x || mouse.x>x+thumb) piano_pan[pattern]=fmaxf(0,fminf(maximum,(mouse.x-gx-thumb/2)/fmaxf(1,travel)*maximum));
            piano_scroll_x=mouse.x+r.x; piano_scroll_start=piano_pan[pattern]; piano_scroll_range=maximum/fmaxf(1,travel); piano_scroll_drag=1; input_enabled=0;
        }
    }
    DrawRectangle(gx,54,gridw,14,cell);
    float ruler_step=fmaxf(1,powf(2,ceilf(log2f(48/(STEPS*cw)))));
    for(int i=0;i<gridw/(ruler_step*STEPS*cw)+2;i++) {
        float b=(floorf(view/STEPS/ruler_step)+i)*ruler_step,x=gx+(b*STEPS-view)*cw;
        if(i==0 || (x>=gx && x<gx+gridw)) label(TextFormat("%.0f",i==0 && x<gx?floorf(view/STEPS)+1:b+1),fmaxf(gx,x)+8,55,11,ink);
        if(STEPS*cw>=48) for(int beat=1;beat<4;beat++) { float bx=x+beat*4*cw; if(bx>=gx && bx<=gx+gridw) DrawLine(bx,gy-5,bx,gy,muted); }
    }
    if(playing && !song) {
        float x=gx+(visual_step-view)*cw;
        if(x>=gx && x<=gx+gridw) DrawLineEx((Vector2){x,gy},(Vector2){x,gy+rh*25},1.5f,accent);
    }
    timeline_marker(2,view,extent,gx,54,gridw,14,q);

}
static int mixer_input_at(Vector2 point) {
    Rect r=windows.editors[3].rect; if(!windows.editors[3].visible) return -1;
    int capacity=fmaxf(1,(r.w-236)/51-1),visible=fminf(project.insert_count,capacity);
    for(int col=0;col<=visible;col++) if(CheckCollisionPointRec(point,(Rectangle){r.x+8+col*51+7,r.y+r.h-46,12,12})) return col?col+mixer_scroll:0;
    return -1;
}
static void mixer(float width,float height) {
    int capacity=fmaxf(1,(width-236)/51-1),visible=project.insert_count<capacity?project.insert_count:capacity;
    mixer_scroll=(int)fmaxf(0,fminf(fmaxf(0,project.insert_count-visible),mixer_scroll));
    if(hover(0,TITLE,width-220,height-TITLE) && mouse.y>TITLE+60) {
        mixer_scroll=(int)fmaxf(0,fminf(fmaxf(0,project.insert_count-visible),mixer_scroll-GetMouseWheelMove()));
    }
    float fader_top=TITLE+78,fader_bottom=height-74;
    for(int col=0;col<=visible;col++) {
        int id=col?col+mixer_scroll:0,x=8+col*51;
        float *v=id?&project.insert_volume[id-1]:&project.master;
        int over=hover(x,TITLE+4,48,height-TITLE-12);
        DrawRectangle(x,TITLE+4,48,height-TITLE-12,mixer_selected==id?(Color){59,72,76,255}:cell);
        if(over) snprintf(status,sizeof status,"%s: click to select this mixer channel",id?TextFormat("Insert %d",id):"Master");
        if(over && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) mixer_selected=id;
        label(id?TextFormat("%d",id):"Master",x+5,TITLE+9,id?12:10,ink);
        int input_over=hover(x+7,height-46,12,12) || (cable_drag>0 && CheckCollisionPointRec(mouse,(Rectangle){x+7,height-46,12,12}));
        DrawRectangleLines(x+9,height-44,8,8,input_over?accent:muted);
        label("IN",x+7,height-60,10,muted);
        if(input_over) snprintf(status,sizeof status,"%s input: drop an insert output cable here",id?TextFormat("Insert %d",id):"Master");
        if(over && mouse.y<TITLE+27 && IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) { mixer_selected=id; open_context(3,id,(Vector2){mouse.x+windows.editors[3].rect.x,mouse.y+windows.editors[3].rect.y}); }
        if(over && mouse.y<TITLE+27) snprintf(status,sizeof status,"%s: right-click for mixer channel actions",id?TextFormat("Insert %d",id):"Master");
        if(id) knob(x+25,TITLE+34,&project.insert_pan[id-1],-1,1,0,"Insert pan");
        DrawRectangle(x+24,fader_top,3,fader_bottom-fader_top,bg);
        if(hover(x+8,fader_top-4,36,fader_bottom-fader_top+8) && IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) begin_number(v,0,1,id?1:.7f,id?"Insert volume":"Master volume");
        if(hover(x+8,fader_top-4,36,fader_bottom-fader_top+8) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            *v=fmaxf(0,fminf(1,(fader_bottom-mouse.y)/(fader_bottom-fader_top)));
            capture_control(v,0,1,1); control_bottom=fader_bottom+windows.editors[3].rect.y; control_range=fader_bottom-fader_top;
        }
        if(hover(x+8,fader_top-4,36,fader_bottom-fader_top+8) || control_drag==v) snprintf(status,sizeof status,"%s volume: drag fader or right-click to enter a value",id?TextFormat("Insert %d",id):"Master");
        int fy=fader_bottom-*v*(fader_bottom-fader_top); DrawRectangle(x+16,fy-4,20,8,accent);
        if(id) {
            DrawRectangle(x+1,TITLE+48,46,22,bg);
            if(button("M",x+2,TITLE+49,21,20,project.insert_mute[id-1]&1)) project.insert_mute[id-1]^=1;
            if(hover(x+2,TITLE+49,21,20)) snprintf(status,sizeof status,"Insert %d: mute/unmute",id);
            if(button("S",x+25,TITLE+49,21,20,project.insert_mute[id-1]&2)) project.insert_mute[id-1]^=2;
            if(hover(x+25,TITLE+49,21,20)) snprintf(status,sizeof status,"Insert %d: solo/unsolo (multiple solos allowed)",id);
        }
        if(id) {
            label("OUT",x+26,height-60,10,muted);
            int output_over=hover(x+29,height-46,12,12);
            DrawCircle(x+35,height-40,5,output_over || cable_drag==id?accent:muted);
            DrawCircle(x+35,height-40,2,project.insert_output[id-1]==255?bg:accent);
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
            Vector2 start={8+col*51+35,height-40},end=cable_drag==mixer_selected?mouse:(Vector2){8+target*51+13,height-40};
            Vector2 points[]={start,{start.x,height-21},{end.x,height-21},end};
            DrawSplineBezierCubic(points,4,2,Fade(accent,.8f));
        }
    }
    int fx=width-220;
    DrawRectangle(fx,TITLE,220,height-TITLE,bg); DrawLine(fx,TITLE,fx,height,cell);
    label(fit_text(mixer_selected?TextFormat("Insert %d - Effects",mixer_selected):"Master - Effects",196,13),fx+12,TITLE+10,13,ink);
    int slot_h=fmaxf(12,fminf(24,(height-TITLE-48)/10));
    for(int slot=0;slot<10;slot++) {
        int y=TITLE+34+slot*slot_h;
        DrawRectangle(fx+8,y,204,slot_h-2,cell);
        label(TextFormat("%d",slot+1),fx+14,y+(slot_h-10)/2,10,muted);
        label("Empty",fx+42,y+(slot_h-10)/2,10,muted);
        if(hover(fx+8,y,204,slot_h-2)) snprintf(status,sizeof status,"Effect slot %d: empty (effects/plugins are not implemented yet)",slot+1);
    }
    float area=fx-16,thumb=area*visible/INSERTS,pos=8+area*mixer_scroll/INSERTS;
    DrawRectangle(8,height-17,area,9,bg); DrawRectangle(pos,height-17,fmaxf(12,thumb),9,mixer_pan || hover(8,height-20,area,14)?accent:muted);
    if(hover(8,height-20,area,14)) {
        snprintf(status,sizeof status,"Mixer: wheel over inserts or drag this scrollbar to browse all 100 inserts");
        if(IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) { mixer_pan=1; mixer_scroll=fmaxf(0,fminf(INSERTS-visible,(mouse.x-8-thumb/2)/area*INSERTS)); input_enabled=0; }
    }
}
static void draw_context(void) {
    if(!context_kind) return;
    float scale=fminf(GetScreenWidth()/1200.f,GetScreenHeight()/675.f);
    int w=228,h=context_kind==3 && !context_target?32:82,x=fmaxf(0,fminf(GetScreenWidth()/scale-w,context_position.x)),y=fmaxf(0,fminf(GetScreenHeight()/scale-h,context_position.y));
    input_enabled=1;
    if(IsKeyPressed(KEY_ESCAPE) || (!context_opened && IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && !hover(x,y,w,h))) { context_kind=0; return; }
    DrawRectangle(x+3,y+3,w,h,Fade(BLACK,.5f)); DrawRectangle(x,y,w,h,panel); DrawRectangleLines(x,y,w,h,muted);
    int target=context_target,kind=context_kind;
    if(kind==1) {
        if(button("Stay on top",x+4,y+4,w-8,24,windows.editors[target].pinned)) { windows_pin(&windows,target); context_kind=0; }
        if(button(windows.editors[target].maximized?"Restore":"Maximize",x+4,y+29,w-8,24,0)) {
            Editor *e=&windows.editors[target]; if(e->maximized) { e->rect=e->restore; e->maximized=0; } else { e->restore=e->rect; e->maximized=1; } context_kind=0;
        }
        if(button("Hide window",x+4,y+54,w-8,24,0)) { windows.editors[target].visible=0; context_kind=0; }
    } else if(kind==2) {
        if(button("Go to Piano Roll",x+4,y+4,w-8,24,0)) { channel=piano_channel=target; piano_channels[pattern][target]=1; windows_focus(&windows,2); context_kind=0; }
        if(button("Mute channel",x+4,y+29,w-8,24,project.mute[target])) { project.mute[target]^=1; context_kind=0; }
        if(button("Delete channel",x+4,y+54,w-8,24,0)) { delete_channel(target); context_kind=0; }
    } else if(!target) {
        if(button("Reset Master volume",x+4,y+4,w-8,24,0)) { project.master=.7f; context_kind=0; }
    } else {
        if(button("Mute insert",x+4,y+4,w-8,24,target && (project.insert_mute[target-1]&1))) { if(target) project.insert_mute[target-1]^=1; context_kind=0; }
        if(button("Solo insert",x+4,y+29,w-8,24,target && (project.insert_mute[target-1]&2))) { if(target) project.insert_mute[target-1]^=2; context_kind=0; }
        if(button("Reset mixer channel",x+4,y+54,w-8,24,0)) { if(target) insert_reset(&project,target); else project.master=.7f; context_kind=0; }
    }
}
static void typing_piano(int blocked) {
    /* GLFW key tokens describe physical US positions, including Spanish - and +. */
    static const int keys[]={KEY_Z,KEY_S,KEY_X,KEY_D,KEY_C,KEY_V,KEY_G,KEY_B,KEY_H,KEY_N,KEY_J,KEY_M,KEY_COMMA,KEY_L,KEY_PERIOD,KEY_SEMICOLON,KEY_SLASH,
        KEY_Q,KEY_TWO,KEY_W,KEY_THREE,KEY_E,KEY_R,KEY_FIVE,KEY_T,KEY_SIX,KEY_Y,KEY_SEVEN,KEY_U,KEY_I,KEY_NINE,KEY_O,KEY_ZERO,KEY_P,KEY_LEFT_BRACKET,KEY_EQUAL,KEY_RIGHT_BRACKET};
    static unsigned char held[sizeof keys/sizeof *keys];
    int enabled=typing_keys && !blocked && !browser_focus && IsWindowFocused() && !IsKeyDown(KEY_LEFT_CONTROL) && !IsKeyDown(KEY_RIGHT_CONTROL) && !IsKeyDown(KEY_LEFT_ALT) && !IsKeyDown(KEY_RIGHT_ALT);
    for(unsigned i=0;i<sizeof keys/sizeof *keys;i++) {
        int pitch=i<17?48+i:60+i-17;
        if(held[i] && (!enabled || !IsKeyDown(keys[i]))) { audio_key(i,channel,pitch,0); held[i]=0; }
        if(enabled && IsKeyPressed(keys[i])) { audio_key(i,channel,pitch,1); held[i]=1; }
    }
}
static void sampler(float width,float height) {
    int c=instrument_channel;
    label(project.channel_names[c],12,36,18,ink);
    label(fit_text(!originals[c].frames?"Drop a sample from the Browser":project.paths[c][0]?GetFileName(project.paths[c]):"Built-in sample",width-24,12),12,62,12,muted);
    Sampler *settings=&project.sampler[c];
    knob(30,102,&project.volume[c],0,1,.7f,"Sampler volume"); label("Volume",50,96,12,muted);
    knob(148,102,&project.pan[c],-1,1,0,"Sampler pan"); label("Pan",168,96,12,muted);
    knob(266,102,&settings->pitch,-12,12,0,"Pitch (semitones, duration unchanged)"); label(TextFormat("Pitch %+.1f",settings->pitch),286,96,12,muted);
    knob(384,102,&settings->time,.25f,4,1,"Time multiplier (1 = original duration)"); label(TextFormat("Time %.2fx",settings->time),404,96,12,muted);
    if(button("Normalize",12,132,102,26,settings->flags&SAMPLE_NORMALIZE)) settings->flags^=SAMPLE_NORMALIZE;
    if(hover(12,132,102,26)) snprintf(status,sizeof status,"Normalize: scale the processed sample peak to 100%%; channel/output volume remain separate");
    if(button("Reverse",122,132,84,26,settings->flags&SAMPLE_REVERSE)) settings->flags^=SAMPLE_REVERSE;
    if(hover(122,132,84,26)) snprintf(status,sizeof status,"Reverse: play the cropped sample backwards");
    if(button("Polarity",214,132,100,26,settings->flags&SAMPLE_POLARITY)) settings->flags^=SAMPLE_POLARITY;
    if(hover(214,132,100,26)) snprintf(status,sizeof status,"Reverse polarity: flip the waveform vertically, leaving duration unchanged");
    if(button(settings->stretch?"Mode: Stretch":"Mode: Resample",324,132,width-336,26,0)) settings->stretch^=1;
    if(hover(324,132,width-336,26)) snprintf(status,sizeof status,"Time mode: Resample changes speed/pitch like vinyl; Stretch preserves pitch; click to switch");
    knob(30,190,&settings->start,0,1,0,"Start (0 to 1 of the original sample)"); label(TextFormat("Start %.0f%%",settings->start*100),50,184,12,muted);
    knob(148,190,&settings->length,0,1,1,"Length (0 to 1 of the remaining sample)"); label(TextFormat("Length %.0f%%",settings->length*100),168,184,12,muted);
    int pending=!sampler_equal(*settings,sampler_applied[c]);
    Sampler applied=sampler_applied[c];
    int live_preview=pending && settings->pitch==applied.pitch && settings->time==applied.time && settings->stretch==applied.stretch;
    unsigned offset=llround(originals[c].frames*(double)settings->start);
    unsigned cropped=llround((originals[c].frames-offset)*(double)settings->length);
    float duration=live_preview?cropped*settings->time/RATE:samples[c].frames/(float)RATE;
    label(TextFormat("%.2f s / %s",duration,live_preview?"Preview":pending?"Processing...":"Sample"),286,184,12,pending?accent:muted);
    static float peaks[512][2]; static const float *cached; static unsigned cached_frames;
    Sample sample=samples[c];
    if(cached!=sample.data || cached_frames!=sample.frames) {
        cached=sample.data; cached_frames=sample.frames;
        for(unsigned i=0;i<512;i++) {
            peaks[i][0]=peaks[i][1]=0;
            for(unsigned f=(uint64_t)i*sample.frames/512;f<(uint64_t)(i+1)*sample.frames/512;f++) {
                peaks[i][0]=fminf(peaks[i][0],sample.data[f]); peaks[i][1]=fmaxf(peaks[i][1],sample.data[f]);
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
                    source_peaks[i][0]=fminf(source_peaks[i][0],source.data[f]);
                    source_peaks[i][1]=fmaxf(source_peaks[i][1],source.data[f]);
                }
            }
        }
        float peak=0;
        for(unsigned i=0;i<512;i++) {
            unsigned bin=settings->flags&SAMPLE_REVERSE?511-i:i;
            unsigned begin=offset+(uint64_t)bin*cropped/512,end=offset+(uint64_t)(bin+1)*cropped/512;
            float low=0,high=0;
            if(cropped<8192) {
                for(unsigned f=begin;f<end;f++) { low=fminf(low,source.data[f]); high=fmaxf(high,source.data[f]); }
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
    float area=height-254,mid=222+area/2;
    DrawRectangle(12,222,width-24,area,bg); DrawLine(12,mid,width-12,mid,cell);
    for(int i=0;i<512;i++) {
        float x=12+i*(width-24)/512;
        DrawLineEx((Vector2){x,mid-display_peaks[i][1]*area*.45f},(Vector2){x,mid-display_peaks[i][0]*area*.45f},1,accent);
    }
    if(hover(12,222,width-24,area)) {
        snprintf(status,sizeof status,"Sample waveform: click to play from the beginning; drop a Browser sample here to load it");
        if(samples[c].frames && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) { channel=c; audio_key(127,c,60,1); }
    }
    double progress=audio_key_position(127,c);
    if(progress>=0 && !live_preview) {
        float x=12+progress*(width-24);
        DrawLineEx((Vector2){x,222},(Vector2){x,222+area},1.5f,ink);
    }
    label("0 s",16,224,10,muted);
    label(TextFormat("%.2f s",duration),width-66,224,10,muted);
    if(live_preview && hover(12,222,width-24,area)) snprintf(status,sizeof status,"Live envelope preview; processed pitch/stretch detail appears when ready; audio changes after release");
    if(sample_drag[0] && sample_moved) DrawRectangleLines(12,222,width-24,area,accent);
    label("Click waveform to play | Drop sample to load",12,height-22,11,muted);
}
static void draw_editor(int id,float scale) {
    Editor *e=&windows.editors[id]; if(!e->visible) return;
    Rect r=e->rect; int top=EDITORS-1;
    while(top>0 && !windows.editors[windows.order[top]].visible) top--;
    int focused=windows.order[top]==id;
    BeginScissorMode((int)(r.x*scale),(int)(r.y*scale),(int)(r.w*scale),(int)(r.h*scale));
    BeginMode2D((Camera2D){.offset={r.x*scale,r.y*scale},.zoom=scale});
    DrawRectangle(0,0,r.w,r.h,panel); DrawRectangle(0,0,r.w,TITLE,focused?(Color){63,74,80,255}:cell);
    const char *titles[]={"Channel Rack","Playlist - Arrangement","Piano Roll","Mixer","Sampler"};
    label(TextFormat("%s%s",titles[id],e->pinned?" (on top)":""),8,6,13,ink);
    if(input_enabled && windows_hit(&windows,mouse.x,mouse.y)==id && mouse.y<r.y+TITLE && IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) open_context(1,id,mouse);
    int chrome=input_enabled && windows_hit(&windows,mouse.x,mouse.y)==id && mouse.y<r.y+TITLE;
    if(chrome && mouse.x>=r.x+r.w-44) DrawRectangle(mouse.x>=r.x+r.w-22?r.w-22:r.w-44,2,20,20,(Color){78,91,99,255});
    int mx=r.w-33;
    if(e->maximized) { DrawRectangleLines(mx-2,6,8,8,ink); DrawRectangle(mx-5,9,8,8,cell); DrawRectangleLines(mx-5,9,8,8,ink); }
    else DrawRectangleLines(mx-4,7,9,9,ink);
    symbol("x",r.w-11,12,ink);
    if(windows_hit(&windows,mouse.x,mouse.y)==id) {
        if(mouse.y<r.y+TITLE) snprintf(status,sizeof status,mouse.x>=r.x+r.w-22?"Hide this editor":mouse.x>=r.x+r.w-44?"Maximize or restore this editor":"Drag to move this editor; right-click for window actions");
        else if(mouse.x>r.x+r.w-12 && mouse.y>r.y+r.h-12) snprintf(status,sizeof status,"Drag this corner to resize the editor");
    }
    input_enabled=input_enabled && windows.owner==id && windows.grab<0;
    Vector2 global=mouse; mouse.x-=r.x; mouse.y-=r.y;
    input_enabled=input_enabled && mouse.y>=TITLE && !(mouse.x>r.w-12 && mouse.y>r.h-12);
    if(id==0) rack(r.w,r.h); else if(id==1) playlist(r.w,r.h,scale); else if(id==2) piano(r.w,r.h); else if(id==3) mixer(r.w,r.h); else sampler(r.w,r.h);
    mouse=global;
    DrawLine(r.w-9,r.h-2,r.w-2,r.h-9,muted); DrawRectangleLines(0,0,r.w,r.h,focused?muted:cell);
    EndMode2D(); EndScissorMode();
}
int main(int argc,char **argv) {
    int smoke=0;
    for(int i=1;i<argc;i++) if(!strcmp(argv[i],"--smoke")) smoke=1;
    project_default(&project); samples_default(originals);
    for(int c=0;c<CHANNELS;c++) { samples[c]=sample_copy(originals[c]); sampler_applied[c]=project.sampler[c]; }
    for(int c=0;c<4;c++) {
        const char *path=TextFormat("%s/%s/%s.wav",LIBRELOOP_SAMPLES,c==3?"melodic":"drums",project.channel_names[c]);
        if(FileExists(path)) snprintf(project.paths[c],sizeof project.paths[c],"%s",path);
    }
    for(int c=0;c<4;c++) if(!samples[c].data) { fprintf(stderr,"Out of memory\n"); return 1; }
    int audio_ok=audio_start(&project,samples);
    if(!audio_ok) snprintf(status,sizeof status,"Audio device unavailable. Editing and WAV export are available.");
    SetConfigFlags(FLAG_WINDOW_RESIZABLE); InitWindow(1200,675,"LibreLoop - Linux pattern workstation");
    SetWindowMinSize(900,506); SetTargetFPS(60); SetExitKey(KEY_NULL);
    raylib_mouse_callback=glfwSetMouseButtonCallback(GetWindowHandle(),mouse_callback);
    resize_cursor=glfwCreateStandardCursor(GLFW_HRESIZE_CURSOR);
    browser_init(&browser,LIBRELOOP_SAMPLES); int frames=0,initialized=0;
    while(!WindowShouldClose()) {
        for(int b=0;b<8;b++) { frame_clicks[b]=pending_clicks[b]>0; if(frame_clicks[b]) pending_clicks[b]--; }
        float scale=fminf(GetScreenWidth()/1200.f,GetScreenHeight()/675.f);
        float width=GetScreenWidth()/scale,height=GetScreenHeight()/scale;
        float sidebar=browser_hidden?24:browser_width;
        Rect desktop={sidebar+6,42,width-sidebar-10,height-66};
        if(!initialized) { windows_init(&windows,width,height); initialized=1; }
        mouse=GetMousePosition(); mouse.x/=scale; mouse.y/=scale; reset=0; popup_opened=0; context_opened=0; snap_opened=0;
        SetMouseCursor(MOUSE_CURSOR_DEFAULT);
        if(IsMouseButtonPressed(MOUSE_BUTTON_LEFT) || IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) arrangement_release(&arrangement);
        if(arrangement.gesture) {
            Rect r=windows.editors[1].rect;
            arrangement_drag(&arrangement,&project,arrangement.view_start+(mouse.x-r.x-92)/((r.w-116)/BARS*arrangement.zoom),track_scroll+(mouse.y-r.y-82)/52);
            int held=arrangement.gesture==ERASE_CLIPS?(IsMouseButtonDown(MOUSE_BUTTON_RIGHT) || IsMouseButtonDown(MOUSE_BUTTON_LEFT)):IsMouseButtonDown(MOUSE_BUTTON_LEFT);
            if(!held) arrangement_release(&arrangement);
        }
        if(playlist_pan) {
            arrangement.view_start=fmaxf(0,playlist_pan_start+(mouse.x-playlist_pan_x)*playlist_pan_range);
            if(!IsMouseButtonDown(MOUSE_BUTTON_LEFT) || IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) playlist_pan=0;
        }
        if(playlist_vpan) {
            Rect r=windows.editors[1].rect; float area=r.h-124;
            track_scroll=fmaxf(0,fminf(LANES-area/52,playlist_vpan_start+(mouse.y-playlist_vpan_y)/area*LANES));
            if(!IsMouseButtonDown(MOUSE_BUTTON_LEFT) || IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) playlist_vpan=0;
        }
        if(mixer_pan) {
            Rect r=windows.editors[3].rect; float area=r.w-236; int visible=fmaxf(1,area/51-1);
            mixer_scroll=fmaxf(0,fminf(INSERTS-visible,(mouse.x-r.x-8-area*visible/INSERTS/2)/area*INSERTS));
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
            Rect r=windows.editors[marker_drag].rect;
            float gx=marker_drag==1?92:60,width=r.w-(marker_drag==1?116:84),span=marker_drag==1?BARS/arrangement.zoom:piano_span[pattern],start=marker_drag==1?arrangement.view_start:piano_pan[pattern];
            float q=snap_interval(snap_mode[marker_drag-1],width/span/(marker_drag==1?STEPS:1))/(marker_drag==1?STEPS:1);
            if(IsMouseButtonPressed(MOUSE_BUTTON_LEFT) || modal) marker_drag=-1;
            else {
                float *point=marker_drag==1?&playlist_start:&piano_start[pattern];
                *point=fmaxf(0,roundf((start+(mouse.x-r.x-gx)/width*span)/q)*q);
                if(!playing) reset=1;
                if(!IsMouseButtonDown(MOUSE_BUTTON_LEFT)) marker_drag=-1;
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
            float value=control_fader?(control_bottom-mouse.y)/control_range:*control_drag-GetMouseDelta().y/scale*(control_drag==&project.bpm?.25f:(control_high-control_low)/160);
            *control_drag=fmaxf(control_low,fminf(control_high,value));
        }
        if(piano_scroll_drag) {
            piano_pan[pattern]=fmaxf(0,piano_scroll_start+(mouse.x-piano_scroll_x)*piano_scroll_range);
            if(!IsMouseButtonDown(MOUSE_BUTTON_LEFT) || IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) piano_scroll_drag=0;
        }
        if(piano_vdrag) {
            Rect r=windows.editors[2].rect; float area=r.h-162;
            piano_top=127-(int)fmaxf(0,fminf(103,piano_vscroll_start+(mouse.y-piano_vscroll_y)/area*128));
            if(!IsMouseButtonDown(MOUSE_BUTTON_LEFT) || IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) piano_vdrag=0;
        }
        if(piano_gesture) {
            int right=piano_gesture==PIANO_ERASE;
            if(modal || IsMouseButtonPressed(MOUSE_BUTTON_LEFT) || IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) piano_gesture=PIANO_IDLE;
            else {
                Rect r=windows.editors[2].rect; float cw=(r.w-84)/piano_span[pattern],rh=(r.h-162)/25;
                piano_gesture_update(piano_pan[pattern]+(mouse.x-r.x-60)/cw,piano_top-(mouse.y-r.y-68)/rh,snap_interval(snap_mode[1],cw));
                if(!IsMouseButtonDown(right?MOUSE_BUTTON_RIGHT:MOUSE_BUTTON_LEFT)) piano_gesture=PIANO_IDLE;
            }
        }
        if(note_drag) {
            Rect r=windows.editors[2].rect; float cw=(r.w-84)/piano_span[pattern],rh=(r.h-162)/25,q=snap_interval(snap_mode[1],cw);
            float step=piano_pan[pattern]+(mouse.x-r.x-60)/cw;
            if(moving_note) {
                uint8_t *selected=note_selected[pattern][piano_channel];
                float dx=roundf((step-note_grab.x)/q)*q,low=0,high=0; int dy=roundf(note_grab.y-(mouse.y-r.y-68)/rh),bottom=0,top=127,first=1;
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
            int visible=fmaxf(1,(windows.editors[3].rect.w-236)/51-1);
            if(mixer_selected>mixer_scroll+visible) mixer_scroll=mixer_selected-visible;
            else if(mixer_selected>0 && mixer_selected<=mixer_scroll) mixer_scroll=mixer_selected-1;
            snprintf(status,sizeof status,"%s -> %s",project.channel_names[route_drag],mixer_selected?TextFormat("Insert %d",mixer_selected):"Master");
        }
        if(!modal && !captured() && !dragging) windows_update(&windows,desktop,mouse.x,mouse.y,IsMouseButtonPressed(MOUSE_BUTTON_LEFT),IsMouseButtonDown(MOUSE_BUTTON_LEFT));
        else { if(browser_resize) windows_update(&windows,desktop,mouse.x,mouse.y,0,0); windows.owner=-1; windows.grab=-1; }
        int focused=EDITORS-1;
        while(focused>0 && !windows.editors[windows.order[focused]].visible) focused--;
        if(!modal && !dragging && !captured() && !browser_focus && windows.order[focused]==1 && windows.editors[1].visible && IsKeyPressed(KEY_DELETE)) {
            for(int l=0;l<LANES;l++) for(int b=0;b<CLIPS;b++) if(arrangement.selected[l][b]) { project.clips[l][b]=0; project.clip_steps[l][b]=0; }
            memset(arrangement.selected,0,sizeof arrangement.selected);
        }
        if(!modal && !dragging && !captured() && !browser_focus && windows.order[focused]==2 && windows.editors[2].visible && IsKeyPressed(KEY_DELETE)) {
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
        BeginDrawing(); ClearBackground(bg); BeginMode2D((Camera2D){.zoom=scale});
        input_enabled=!modal && !dragging && !captured() && windows.grab<0 && mouse.y<42;
        DrawRectangle(0,0,width,40,panel);
        int save=button("Save",8,5,48,28,0);
        if(save) snprintf(status,sizeof status,project_save(project_path,&project)?"Saved %.150s":"Save failed: %.150s",project_path);
        if(button("Open",60,5,48,28,0)) load_project(project_path);
        if(button("WAV",112,5,44,28,0)) snprintf(status,sizeof status,(sampler_flush() && export_wav("song.wav",&project,samples))?"Exported song.wav (Playlist, 48 kHz stereo)":"WAV export failed");
        label("LibreLoop",166,9,14,muted);
        label(GetFileName(project_path),8,29,10,muted);
        DrawRectangle(244,5,48,28,cell);
        knob(258,18,&project.master_pitch,-12,12,0,"Master pitch (semitones)");
        if(hover(249,9,18,18) || control_drag==&project.master_pitch) snprintf(status,sizeof status,"Master pitch: %+.2f semitones (sample speed; right-click to enter)",project.master_pitch);
        knob(278,18,&output_volume,0,1,1,"LibreLoop output volume");
        if(hover(269,9,18,18) || control_drag==&output_volume) snprintf(status,sizeof status,"LibreLoop output volume: listening level; Master and WAV export unchanged");
        if(button("",300,5,52,28,0)) { song=!song; reset=1; }
        DrawRectangle(301,song?19:6,50,13,song?accent:(Color){218,161,91,255});
        label("PATT",311,7,10,song?muted:bg); label("SONG",311,20,10,song?bg:muted);
        if(hover(300,5,52,28)) snprintf(status,sizeof status,"Playback mode: click to switch Pattern (orange) / Song (green)");
        if(button(playing?"||":">",356,5,30,28,playing)) transport_toggle();
        if(hover(356,5,30,28)) snprintf(status,sizeof status,"Play/stop from the ruler start marker (Space)");
        if(button("[]",390,5,30,28,0)) { playing=0; reset=1; }
        if(button(TextFormat("%.2f",project.bpm),428,5,84,28,control_drag==&project.bpm)) capture_control(&project.bpm,30,300,0);
        if(hover(428,5,84,28)) {
            project.bpm=fmaxf(30,fminf(300,project.bpm+GetMouseWheelMove()));
            snprintf(status,sizeof status,"Tempo: drag up/down; right-click to enter a value");
            if(IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) begin_number(&project.bpm,30,300,120,"Tempo (BPM)");
        }
        uint64_t step=(uint64_t)(audio_position()/(RATE*60.0/project.bpm/4));
        DrawRectangle(520,5,104,28,bg); DrawRectangleLines(520,5,104,28,cell);
        label(TextFormat("%03llu:%02llu",(unsigned long long)(step/16+1),(unsigned long long)(step%16+1)),530,9,22,accent);
        if(hover(520,5,104,28)) snprintf(status,sizeof status,"Playback position: bar and sixteenth-note step");
        if(button(fit_text(project.pattern_names[pattern],130,13),632,5,152,28,1)) open_popup(1);
        DrawTriangle((Vector2){771,16},(Vector2){775,21},(Vector2){779,16},bg);
        int pattern_hover=CheckCollisionPointRec(mouse,(Rectangle){632,5,152,28});
        if(pattern_hover && (pattern_popup==0 || pattern_popup==1) && !dragging && !captured() && windows.grab<0) {
            snprintf(status,sizeof status,"Pattern: click to select/create; right-click to rename; Up/Down or k/j: change pattern");
            int direction=(IsKeyPressed(KEY_DOWN)||IsKeyPressed(KEY_J))-(IsKeyPressed(KEY_UP)||IsKeyPressed(KEY_K));
            if(direction) { pattern=(pattern+project.pattern_count+direction)%project.pattern_count; reset=1; if(!pattern_popup) open_popup(1); }
            if(IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) begin_rename();
        }
        for(int i=0;i<4;i++) { int id=i==1?2:i==2?1:i; editor_button(id,792+i*32); }
        if(button("Help",924,5,52,28,0)) { open_popup(4); }
        if(button("Follow",984,5,64,28,follow_playhead)) follow_playhead=!follow_playhead;
        if(hover(984,5,64,28)) snprintf(status,sizeof status,"Follow playhead: keep playback in view in the Playlist (Song) or Piano Roll (PAT)");
        if(button("Keys",1056,5,56,28,typing_keys)) typing_keys=!typing_keys;
        if(hover(1056,5,56,28)) snprintf(status,sizeof status,"Typing piano: Z row C3, Q row C4; S/D/G/H/J and 2/3/5/6/7 sharps; disabled in Browser/text fields");
        visual_step=playing && !reset?audio_visual_position()/(RATE*60.0/project.bpm/4):playback_start();
        typing_piano(modal || dragging);
        DrawRectangle(width-78,5,70,28,bg);
        label(audio_ok?"Audio":"Offline",width-70,13,12,audio_ok?accent:muted);
        if(hover(width-78,5,70,28)) snprintf(status,sizeof status,audio_ok?"Audio device is running":"Audio device unavailable");
        input_enabled=!modal && !pattern_popup && !context_kind && !snap_menu && !dragging && !captured() && windows.grab<0 && mouse.x<sidebar+6 && mouse.y>=42;
        DrawRectangle(0,42,sidebar,height-66,panel);
        if(!browser_hidden) {
            label("Browser",10,50,14,ink);
            if(button("+ Add folder",8,76,sidebar-16,23,0)) { snprintf(folder_text,sizeof folder_text,"%s",browser.path); rename_select_all=1; open_popup(3); }
            int fy=108,rows=fmaxf(1,(height-fy-148)/23);
            if(!modal && !pattern_popup && !context_kind && !snap_menu && browser_focus && !dragging && !captured() && !IsKeyDown(KEY_LEFT_CONTROL)) {
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
                Color color=browser.selected==entry?bg:ink;
                if(node->dir) {
                    if(node->open) DrawTriangle((Vector2){indent,y+8},(Vector2){indent+4,y+13},(Vector2){indent+8,y+8},color);
                    else DrawTriangle((Vector2){indent,y+6},(Vector2){indent,y+14},(Vector2){indent+6,y+10},color);
                }
                const char *name=GetFileName(path); if(!*name) name=path;
                label(fit_text(node->depth==0 && !strcmp(path,browser.samples)?"LibreLoop samples":name,fmaxf(0,sidebar-indent-24),13),indent+14,y+4,13,color);
                if(hover(8,y,sidebar-16,21)) snprintf(status,sizeof status,"%.80s: %s | j/k: select; h/l: fold",path,node->dir?"click to expand/collapse":IsFileExtension(path,".hbt")?"click to open project":"click to preview; drag onto a Rack channel to load");
                if(clicked) {
                    browser_select(&browser,entry);
                    if(node->dir) { if(!browser_toggle(&browser,entry)) snprintf(status,sizeof status,"Folder unavailable: %.180s",path); break; }
                    if(IsFileExtension(path,".hbt")) load_project(path);
                    else {
                        audition_entry(entry); snprintf(sample_drag,sizeof sample_drag,"%s",path);
                        sample_origin=mouse; sample_moved=0;
                    }
                }
            }
            EndScissorMode();
            DrawRectangle(8,height-137,sidebar-16,68,bg);
            if(browser.selected>=0 && browser.selected<browser.items && audition.data && !strcmp(browser.nodes[browser.selected].path,audition_path)) {
                for(int i=0;i<128;i++) {
                    float x=9+i*(sidebar-18)/128.0f;
                    DrawLine(x,height-103-audition_high[i]*29,x,height-103-audition_low[i]*29,accent);
                }
                if(hover(8,height-137,sidebar-16,68)) snprintf(status,sizeof status,"Selected sample waveform: %.120s",GetFileName(audition_path));
            }
            label(fit_text("Click: preview / drag: load",sidebar-16,10),8,height-59,10,muted); label(fit_text("j/k: select  h/l: fold",sidebar-16,10),8,height-43,10,muted);
        }
        int resize_hover=hover(sidebar,42,6,height-66);
        DrawRectangle(sidebar,42,6,height-66,browser_resize || resize_hover?accent:cell);
        if(resize_hover || browser_resize) { SetMouseCursor(MOUSE_CURSOR_RESIZE_EW); snprintf(status,sizeof status,"Drag to resize Browser; below minimum width collapses it; drag outward to restore"); }
        EndMode2D();
        /* Freeze draw order: opening Piano Roll while drawing must not draw a window twice. */
        int order[EDITORS]; memcpy(order,windows.order,sizeof order);
        for(int i=0;i<EDITORS;i++) { input_enabled=!modal && !pattern_popup && !context_kind && !snap_menu && !dragging && !sample_drag[0] && !captured(); draw_editor(order[i],scale); }
        BeginMode2D((Camera2D){.zoom=scale}); DrawRectangle(0,height-22,width,22,panel); label(fit_text(status,width-16,11),8,height-17,11,ink);
        if(sample_drag[0] && sample_moved) {
            DrawRectangle(mouse.x+12,mouse.y+8,164,24,cell);
            label(fit_text(GetFileName(sample_drag),148,12),mouse.x+20,mouse.y+14,12,ink);
        }
        draw_popup(); draw_context(); draw_snap(); EndMode2D();
        sampler_queue();
        flush_cursor(); EndDrawing(); audio_update(&project,playing,song,pattern,reset,output_volume,playback_start());
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
    browser_close(&browser); audio_close(); free(audition.data); if(resize_cursor) glfwDestroyCursor(resize_cursor); CloseWindow();
    for(int c=0;c<CHANNELS;c++) { free(samples[c].data); free(originals[c].data); }
    return 0;
}
