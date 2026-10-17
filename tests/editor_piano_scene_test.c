// SPDX-License-Identifier: GPL-3.0-only
#include "editor_draw.h"
#include "piano_draw.h"
#include "theme.h"
#include <SDL3/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CHECK(x) do{if(!(x)){fprintf(stderr,"Desktop scene check%d: %s\n",__LINE__,#x);return 1;}}while(0)
static Project project;
int main(int argc,char **argv){CHECK(argc==2);CHECK(SDL_Init(0));SDL_Surface *surface=SDL_CreateSurface(1200,675,SDL_PIXELFORMAT_RGBA32);CHECK(surface);SDL_Renderer *native=SDL_CreateSoftwareRenderer(surface);CHECK(native);DrawContext *draw=draw_create(native);CHECK(draw);UIRender *render=ui_render_create(draw,argv[1]);CHECK(render);theme_init();project_new(&project);
    Windows windows;windows_init(&windows,1200,675);windows.editors[2].visible=1;windows_focus(&windows,2);Rect r=windows.editors[2].rect;
    PianoViewport view={r.w,r.h,16,0,1,72};Note before[NOTES]={0},velocity_before[NOTES]={0};uint8_t selected[NOTES]={0},selection_before[NOTES]={0};PianoGesture gesture={.selected=selected,.before=before,.selection_before=selection_before};PianoKeys keys={.pitch=-1};PianoVelocity velocity={.ramp=-1,.before=velocity_before};PianoScrollbars bars={0};
    PianoDrawing piano={.project=&project,.view=&view,.gesture=&gesture,.keys=&keys,.velocity=&velocity,.bars=&bars,.hovered_key=-1,.hovered_tool=-1,.range=32,.playback=-1};WindowGeometry geometry={.width=1200,.height=675,.pixel_width=1200,.pixel_height=675,.layout_scale=1,.raster_scale=1,.dpi_x=1,.dpi_y=1};EditorDrawing editor={&windows,2,-1,0};
    draw_clear(draw,ui_theme.background);editor_draw_begin(render,geometry,&editor);piano_draw_toolbar(render,&piano);piano_draw_content(render,&piano);editor_draw_end(render,&editor);
    unsigned char *pixels=malloc(1200*675*4);CHECK(pixels);CHECK(draw_read_rgba(draw,pixels,1200,675));const unsigned char *p=pixels+((128+6)*1200+320+400)*4;CHECK(p[0]==ui_theme.title_focus.r && p[1]==ui_theme.title_focus.g && p[2]==ui_theme.title_focus.b);
    WindowChromeHit hit=windows_chrome_hit(&windows,2,r.x+r.w-10,r.y+9,1);CHECK(hit.title && hit.slot==2);CHECK(windows_chrome_hit(&windows,2,r.x+r.w-10,r.y+18,1).slot==-1);CHECK(windows_chrome_hit(&windows,2,r.x+r.w-10,r.y+9,0).slot==-1);
    ui_render_destroy(render);draw_destroy(draw);SDL_DestroyRenderer(native);SDL_DestroySurface(surface);SDL_Quit();free(pixels);puts("Original editor rectangle/chrome composed with readonly piano drawing");return 0;}
