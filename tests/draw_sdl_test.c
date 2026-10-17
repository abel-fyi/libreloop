// SPDX-License-Identifier: GPL-3.0-only
#include "draw.h"
#include <SDL3/SDL.h>
#include <stdio.h>
#include <string.h>
#define CHECK(x) do { if(!(x)) { fprintf(stderr,"Draw check line%d: %s (%s)\n",__LINE__,#x,SDL_GetError());return 1; } } while(0)
static int pixel(const unsigned char *p,int x,int y,DrawColor c) {
    const unsigned char *v=p+(y*64+x)*4;return v[0]==c.r && v[1]==c.g && v[2]==c.b && v[3]==c.a;
}
int main(void) {
    CHECK(SDL_Init(0));SDL_Surface *surface=SDL_CreateSurface(64,64,SDL_PIXELFORMAT_RGBA32);CHECK(surface);
    SDL_Renderer *renderer=SDL_CreateSoftwareRenderer(surface);CHECK(renderer);
    DrawContext *draw=draw_create(renderer);CHECK(draw);
    DrawColor bg={9,7,15,255},white={255,255,255,255},cyan={0,200,220,255};unsigned char pixels[64*64*4];
    draw_clear(draw,bg);draw_transform(draw,2,4,6);draw_rect(draw,(DrawRect){2,3,10,8},cyan);
    draw_transform(draw,1,0,0);DrawRect clip={16,16,8,8};draw_clip(draw,&clip);draw_rect(draw,(DrawRect){0,0,64,64},white);draw_clip(draw,NULL);
    CHECK(draw_read_rgba(draw,pixels,64,64));CHECK(pixel(pixels,0,0,bg));CHECK(pixel(pixels,10,14,cyan));CHECK(pixel(pixels,18,18,white));CHECK(pixel(pixels,24,24,cyan));
    unsigned char rgba[]={255,0,0,255,0,255,0,255,0,0,255,255,255,255,255,255};
    DrawTexture *texture=draw_texture_create(draw,2,2,rgba,0);CHECK(texture);draw_texture_filter(texture,0);
    draw_clear(draw,bg);draw_texture(draw,texture,(DrawRect){0,0,2,2},(DrawRect){4,4,8,8},white);
    CHECK(draw_read_rgba(draw,pixels,64,64));CHECK(pixel(pixels,5,5,(DrawColor){255,0,0,255}));CHECK(pixel(pixels,10,5,(DrawColor){0,255,0,255}));CHECK(pixel(pixels,5,10,(DrawColor){0,0,255,255}));
    draw_clear(draw,bg);draw_texture_pro(draw,texture,(DrawRect){0,0,2,2},(DrawRect){32,32,8,8},(DrawPoint){4,4},90,white);
    CHECK(draw_read_rgba(draw,pixels,64,64));CHECK(pixel(pixels,34,30,(DrawColor){255,0,0,255}));CHECK(pixel(pixels,34,34,(DrawColor){0,255,0,255}));CHECK(pixel(pixels,30,30,(DrawColor){0,0,255,255}));CHECK(pixel(pixels,30,34,white));
    DrawTexture *target=draw_texture_create(draw,16,16,NULL,1);CHECK(target);
    draw_target(draw,target);draw_clear(draw,cyan);draw_target(draw,NULL);draw_clear(draw,bg);
    draw_texture(draw,target,(DrawRect){0,0,16,16},(DrawRect){0,0,16,16},white);
    draw_line(draw,(DrawPoint){30,30},(DrawPoint){50,30},4,white);
    CHECK(draw_read_rgba(draw,pixels,64,64));CHECK(pixel(pixels,8,8,cyan));CHECK(pixel(pixels,40,30,white));CHECK(pixel(pixels,40,35,bg));
    DrawFont *font=draw_font_bitmap(draw);CHECK(font);
    DrawPoint measured=draw_measure(font,"A",10,0);CHECK(measured.x==6 && measured.y==10);
    draw_clear(draw,bg);draw_text(draw,font,"A",(DrawPoint){4,4},10,0,white);
    CHECK(draw_read_rgba(draw,pixels,64,64));
    int lit=0;for(int y=4;y<14;y++)for(int x=4;x<10;x++)if(pixel(pixels,x,y,white))lit++;
    CHECK(lit>0);CHECK(pixel(pixels,3,4,bg));CHECK(pixel(pixels,10,4,bg));
    draw_font_destroy(font);
    draw_texture_destroy(target);draw_texture_destroy(texture);draw_destroy(draw);SDL_DestroyRenderer(renderer);SDL_DestroySurface(surface);SDL_Quit();
    puts("SDL2D transform, clipping, nearest textures, target restore and thick geometry passed");return 0;
}
