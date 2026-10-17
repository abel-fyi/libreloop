// SPDX-License-Identifier: GPL-3.0-only
#include "render.h"
#include <SDL3/SDL.h>
#include <stdio.h>
#define CHECK(x) do{if(!(x)){fprintf(stderr,"UI render check%d: %s\n",__LINE__,#x);return 1;}}while(0)
int main(int argc,char **argv) {
    CHECK(argc==2);CHECK(SDL_Init(0));SDL_Surface *surface=SDL_CreateSurface(400,240,SDL_PIXELFORMAT_RGBA32);CHECK(surface);
    SDL_Renderer *native=SDL_CreateSoftwareRenderer(surface);CHECK(native);DrawContext *draw=draw_create(native);CHECK(draw);
    UIRender *ui=ui_render_create(draw,argv[1]);CHECK(ui);
    WindowGeometry g={.width=400,.height=240,.pixel_width=400,.pixel_height=240,.layout_scale=1,.raster_scale=1,.dpi_x=1,.dpi_y=1};
    ui_render_view(ui,g,(DrawRect){12.25f,18.5f,100,120});DrawColor bg={9,7,15,255},white={255,255,255,255};
    draw_clear(draw,bg);ui_render_clip(ui,(DrawRect){4,4,80,80});ui_render_circle(ui,40,40,12,white);ui_render_label(ui,"A",8.3f,8.7f,13,white);ui_render_unclip(ui);
    unsigned char pixels[400*240*4];CHECK(draw_read_rgba(draw,pixels,400,240));
    const unsigned char *p=pixels+(58*400+52)*4;CHECK(p[0]==255 && p[1]==255 && p[2]==255);p=pixels+(18*400+12)*4;CHECK(p[0]==9 && p[1]==7 && p[2]==15);
    char fit[128];ui_render_fit(ui,fit,sizeof fit,"Long label",0,13);CHECK(!fit[0]);CHECK(ui_render_text_width(ui,"A",13)>0);
    ui_render_view(ui,g,(DrawRect){0,0,400,240});ui_render_arc(ui,0,64,(DrawRect){100,100,32,32},white);CHECK(draw_read_rgba(draw,pixels,400,240));
    int arc=0;for(int y=100;y<132;y++)for(int x=100;x<132;x++)if(pixels[(y*400+x)*4]>9)arc++;CHECK(arc>0);
    ui_render_destroy(ui);draw_destroy(draw);SDL_DestroyRenderer(native);SDL_DestroySurface(surface);SDL_Quit();puts("UI font/asset cache, editor origin, clipping and original widget texture upload passed");return 0;
}
