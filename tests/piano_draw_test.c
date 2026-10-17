// SPDX-License-Identifier: GPL-3.0-only
#include "piano_draw.h"
#include "theme.h"
#include <SDL3/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CHECK(x) do{if(!(x)){fprintf(stderr,"Piano drawing check%d: %s\n",__LINE__,#x);return 1;}}while(0)
static int pixel(const unsigned char *pixels,int x,int y,DrawColor color){const unsigned char *p=pixels+(y*512+x)*4;return p[0]==color.r && p[1]==color.g && p[2]==color.b && p[3]==color.a;}
static Project project;
int main(int argc,char **argv) {
    CHECK(argc==2);CHECK(SDL_Init(0));SDL_Surface *surface=SDL_CreateSurface(512,640,SDL_PIXELFORMAT_RGBA32);CHECK(surface);SDL_Renderer *native=SDL_CreateSoftwareRenderer(surface);CHECK(native);
    DrawContext *draw=draw_create(native);CHECK(draw);UIRender *render=ui_render_create(draw,argv[1]);CHECK(render);theme_init();project_new(&project);project.pattern_steps[0]=17.25f;
    project.notes[0][0][0]=(Note){60,100,4,2};project.notes[0][0][1]=(Note){64,80,8,1.25f};
    Project *before=malloc(sizeof *before);CHECK(before);memcpy(before,&project,sizeof project);
    Note notes_before[NOTES]={0},velocity_before[NOTES]={0};uint8_t selected[NOTES]={1},selection_before[NOTES]={0},edges[NOTES]={0};
    PianoGesture gesture={.pattern=0,.channel=0,.selected=selected,.before=notes_before,.selection_before=selection_before};PianoViewport view={484,580,32,0,1,72};PianoKeys keys={.pitch=-1};PianoVelocity velocity={.ramp=-1,.before=velocity_before};PianoScrollbars bars={0};
    PianoDrawing state={.project=&project,.view=&view,.gesture=&gesture,.keys=&keys,.velocity=&velocity,.bars=&bars,.edges=edges,.hovered_key=-1,.range=64,.playback=-1};
    WindowGeometry geometry={.width=512,.height=640,.pixel_width=512,.pixel_height=640,.layout_scale=1,.raster_scale=1,.dpi_x=1,.dpi_y=1};
    state.hovered_tool=-1;ui_render_view(render,geometry,(DrawRect){0,0,484,580});draw_clear(draw,ui_theme.background);piano_draw_toolbar(render,&state);piano_draw_content(render,&state);
    unsigned char *pixels=malloc(512*640*4);CHECK(pixels);CHECK(draw_read_rgba(draw,pixels,512,640));
    CHECK(pixel(pixels,20,286,ui_theme.piano_c));CHECK(pixel(pixels,115,284,ui_theme.note_drag));CHECK(pixel(pixels,163,220,ui_theme.note));CHECK(pixel(pixels,300,91,ui_theme.disabled));CHECK(pixel(pixels,500,620,ui_theme.background));
    CHECK(!memcmp(before,&project,sizeof project));CHECK(selected[0]==1 && selected[1]==0);CHECK(gesture.kind==PIANO_IDLE && keys.pitch==-1 && velocity.ramp==-1);
    piano_note_edges(&project,0,0,&view,(PianoPoint){133,286},1,0,edges);CHECK(edges[0]);piano_note_edges(&project,0,0,&view,(PianoPoint){133,286},1,1,edges);CHECK(!edges[0]);
    ui_render_destroy(render);draw_destroy(draw);SDL_DestroyRenderer(native);SDL_DestroySurface(surface);SDL_Quit();free(before);free(pixels);puts("Original piano content draws with project/selection unchanged and pure edge hit testing");return 0;
}
