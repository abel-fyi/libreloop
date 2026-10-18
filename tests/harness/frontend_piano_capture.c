// SPDX-License-Identifier: GPL-3.0-only
// Frozen original piano drawing function with drawing-only calls disabled.
#include "../../src/ui/frontend/ui_internal.h"
const char *host_text_format(const char *format,...){(void)format;return "";}
UiState ui;
Theme ui_theme;
DesktopSkin ui_skin;
static int test_left,test_right,test_shift,test_shortcut;
static FILE *previews,*audition;
static int test_button(int button){return button==HOST_MOUSE_BUTTON_LEFT?test_left:button==HOST_MOUSE_BUTTON_RIGHT?test_right:0;}
static int test_key(int key){return (key==HOST_KEY_LEFT_SHIFT || key==HOST_KEY_RIGHT_SHIFT)?test_shift:0;}
#undef host_is_mouse_button_pressed
#define host_is_mouse_button_pressed test_button
#define host_is_key_down test_key
#undef host_draw_rectangle_rec
#define host_draw_rectangle_rec(...) ((void)0)
#undef host_draw_rectangle_lines_ex
#define host_draw_rectangle_lines_ex(...) ((void)0)
#undef host_draw_line_ex
#define host_draw_line_ex(...) ((void)0)
#undef host_draw_line
#define host_draw_line(...) ((void)0)
#undef host_draw_rectangle
#define host_draw_rectangle(...) ((void)0)
#undef host_begin_scissor_mode
#define host_begin_scissor_mode(...) ((void)0)
#undef host_set_mouse_cursor
#define host_set_mouse_cursor(...) ((void)0)
#undef label
#define label_key(...) ((void)0)
#define label(...) ((void)0)
#undef circle
#define circle(...) ((void)0)
#undef ui_surface
#define ui_surface(...) ((void)0)
#define ui_gradient(...) ((void)0)
#define ui_graph_dots(...) ((void)0)
#define ui_divider_grip(...) ((void)0)
#define ui_note_velocity_color(...) ((DrawColor){0})
#define ui_scrollbar(...) ((void)0)
#undef row_zoom_button
#define row_zoom_button(...) ((void)0)
#undef timeline_ruler
#define timeline_ruler(...) ((void)0)
#define timeline_grid_choice(...) ((void)0)
#undef timeline_grid
#define timeline_grid(...) ((void)0)
#undef follow_view
#define follow_view(...) ((void)0)
/* New workspace toolbar is outside these frozen legacy interaction captures. */
#define button(...) 0
#define grid_selector(...) ((void)0)
#define begin_rack_fill(...) ((void)0)
#define knob_style(...) ((void)0)
#ifndef LIBRELOOP_CAPTURE_REAL_HISTORY
int history_checkpoint(void){return 1;}
#endif

int tool_button(int t,int a,int x,const char *tip){(void)t;(void)a;(void)x;(void)tip;return 0;}
int shortcut_down(void){return test_shortcut;}
int hover(float x,float y,float w,float h){return ui.input_enabled && ui.mouse.x>=x && ui.mouse.x<x+w && ui.mouse.y>=y && ui.mouse.y<y+h;}
float ui_scale(void){return 1;}
int text_width(const char *text,int size){(void)text;(void)size;return 0;}
double pattern_playback_position(void){return -1;}
float edit_steps(void){return ui.project.pattern_steps[ui.pattern];}
void audio_note(int c,Note n){fprintf(previews,"%d %d %d %.9g %.9g\n",c,n.pitch,n.velocity,n.start,n.length);}
void audio_key(int k,int c,int p,int down){fprintf(audition,"%d %d %d %d\n",k,c,p,down);}
void pattern_extend(float end) {
    if(end<=ui.project.pattern_steps[ui.pattern]) return;
    ui.project.pattern_steps[ui.pattern]=ceilf(end/STEPS)*STEPS;
    if(ui.arrangement.source_pattern==ui.pattern) ui.arrangement.source_steps=ui.project.pattern_steps[ui.pattern];
}

#include "../../src/ui/frontend/ui_piano.c"
int main(int argc,char **argv) {
    if(argc!=2)return 1;int mode=atoi(argv[1]);if(mode<0 || mode>29)return 1;
    project_new(&ui.project);ui.project.insert_count=INSERTS; /* Frozen reference project geometry. */ui.project.pattern_steps[0]=17.25f;
    ui.piano_note_length=1.5f;ui.piano_zoom=1;ui.piano_top=72;ui.piano_span[0]=32;ui.velocity_drag=-1;
    ui.windows.editors[2].rect=(Rect){0,0,484,580};ui.input_enabled=1;
    ui.project.notes[0][0][0]=(Note){60,100,4,2};ui.project.notes[0][0][1]=(Note){60,70,4,1};ui.project.notes[0][0][2]=(Note){64,80,8,1.25f};
    float step=10.25f,pitch=64.5f;test_left=1;
    if(mode>=1 && mode<=5){step=4.25f;pitch=59.5f;}
    if(mode==2)step=4.9f;
    if(mode==3)step=4.44f;
    if(mode==4){test_shift=1;ui.note_selected[0][0][1]=1;}
    if(mode==5){test_shortcut=1;ui.note_selected[0][0][0]=1;}
    if(mode==6){test_shift=1;ui.note_selected[0][0][0]=1;}
    if(mode==7){ui.piano_tool=SELECT;ui.note_selected[0][0][0]=1;}
    if(mode==8 || mode==9){ui.piano_tool=BRUSH;step=4.25f;pitch=59.5f;ui.note_selected[0][0][1]=1;if(mode==9)ui.note_selected[0][0][0]=1;}
    if(mode==10 || mode==11){test_right=1;test_left=mode==11;step=4.25f;pitch=59.5f;}
    if(mode==12)step=24.25f;
    if(mode==13)step=33;
    if(mode==14)for(int i=3;i<NOTES;i++)ui.project.notes[0][0][i]=(Note){40,20,0,1};
    if(mode==15){ui.piano_span[0]=64;step=35.25f;}
    if(mode==20){ui.project.notes[0][0][0]=(Note){60,100,17,4};ui.project.notes[0][0][1].velocity=0;step=17.1f;pitch=59.5f;}
    if(mode==21){ui.piano_pan[0]=8;step=.25f;pitch=63.5f;}
    ui.mouse=(DrawPoint){60+step*(400/ui.piano_span[0]),86+(ui.piano_top-pitch)*16};
    if(mode==16)ui.mouse=(DrawPoint){150,64};
    if(mode==17)ui.mouse=(DrawPoint){400,64};
    if(mode==18)ui.mouse=(DrawPoint){472,80};
    if(mode==19)ui.mouse=(DrawPoint){472,480};
    if(mode>=22)ui.piano_key=-1;
    if(mode==22 || mode==23)ui.mouse=(DrawPoint){mode==22?50:20,mode==22?286:266};
    if(mode>=24){ui.mouse=(DrawPoint){111,mode==25?510:520};test_right=mode==25 || mode==27;test_left=!test_right;}
    previews=fopen("previews.txt","w");audition=fopen("audition.txt","w");piano(484,580);
    if(mode==22 || mode==23){desktop_piano_key_update(20,266,0);desktop_piano_key_update(50,266,0);desktop_piano_key_update(58,266,0);desktop_piano_key_update(50,286,1);}
    if(mode>=24) {
        test_left=test_right=0;ui.mouse=(DrawPoint){164,530};
        int modal=mode==26;int new_press=mode==27;int held=mode!=29;
        if(mode==28)ui.pattern=1;
        if(modal || ui.pattern!=ui.velocity_pattern || ui.piano_channel!=ui.velocity_channel || new_press)ui.velocity_drag=-1;
        else {
            Rect r=ui.windows.editors[2].rect;float cw=(r.w-84)/ui.piano_span[ui.pattern];
            DrawPoint point={ui.piano_pan[ui.pattern]+(ui.mouse.x-r.x-60)/cw,fmaxf(1,fminf(127,(r.y+r.h-22-ui.mouse.y)/.48f))};
            DrawPoint from=ui.velocity_drag?ui.velocity_from:ui.velocity_now;
            notes_velocity(ui.project.notes[ui.pattern][ui.piano_channel],ui.velocity_drag?ui.velocity_before:NULL,from.x,point.x,from.y,point.y,3/cw);
            ui.velocity_now=point;if(!held)ui.velocity_drag=-1;
        }
    }
    fclose(previews);fclose(audition);
    if(!project_save("expected.llp",&ui.project))return 2;
    FILE *selection=fopen("selection.bin","wb"),*state=fopen("state.txt","w");
    fwrite(ui.note_selected[0][0],1,NOTES,selection);fwrite(ui.note_selection_before,1,NOTES,selection);fwrite(ui.note_before,sizeof(Note),NOTES,selection);fclose(selection);
    fprintf(state,"%d %d %d %d %d %d %.9g %.9g %.9g %.9g %.9g %.9g %.9g\n",ui.input_enabled,ui.piano_gesture,ui.piano_additive,ui.moving_note,ui.piano_channels[0][0],ui.note_drag?(int)(ui.note_drag-ui.project.notes[0][0]):-1,ui.piano_note_length,ui.piano_from.x,ui.piano_from.y,ui.piano_now.x,ui.piano_now.y,ui.note_grab.x,ui.note_grab.y);fclose(state);
    FILE *scroll=fopen("scrollbar.txt","w");fprintf(scroll,"%d %d %d %.9g %.9g %.9g %.9g %.9g %.9g\n",ui.piano_top,ui.piano_vdrag,ui.piano_scroll_drag,ui.piano_pan[0],ui.piano_vscroll_y,ui.piano_vscroll_start,ui.piano_scroll_x,ui.piano_scroll_start,ui.piano_scroll_range);fclose(scroll);
    FILE *controls=fopen("controls.txt","w"),*velocity=fopen("velocity-before.bin","wb");
    fprintf(controls,"%d %d %d %d %d %d %.9g %.9g %.9g %.9g\n",ui.piano_key,ui.piano_key_channel,ui.piano_key_drag,ui.velocity_drag,ui.velocity_pattern,ui.velocity_channel,ui.velocity_from.x,ui.velocity_from.y,ui.velocity_now.x,ui.velocity_now.y);fclose(controls);
    fwrite(ui.velocity_before,sizeof(Note),NOTES,velocity);fclose(velocity);return 0;
}
