// SPDX-License-Identifier: GPL-3.0-only
#include "draw.h"
#include "draw_cpu_stats.h"
#include <SDL3/SDL.h>
#include <stdio.h>
#include <string.h>
#define CHECK(x) do{if(!(x)){fprintf(stderr,"CPU raster check%d: %s\n",__LINE__,#x);return 1;}}while(0)
int main(void) {
    CHECK(SDL_Init(0));SDL_Surface *s=SDL_CreateSurface(32,32,SDL_PIXELFORMAT_RGBA32);CHECK(s);
    SDL_Renderer *r=SDL_CreateSoftwareRenderer(s);CHECK(r);DrawContext *d=draw_create(r);CHECK(d);
    DrawColor pixels[32*32];draw_clear(d,(DrawColor){0,0,0,255});
    DrawColor tint={255,255,255,128};DrawVertex q[]={{{2,2},0,0,tint},{{12,2},0,0,tint},{{12,12},0,0,tint},{{2,2},0,0,tint},{{12,12},0,0,tint},{{2,12},0,0,tint}};
    draw_triangles(d,NULL,q,6);CHECK(draw_read_rgba(d,pixels,32,32));
    for(int y=2;y<12;y++)for(int x=2;x<12;x++)CHECK(pixels[y*32+x].r==128 && pixels[y*32+x].a==255);
    CHECK(pixels[1*32+1].r==0);CHECK(draw_present(d));DrawCpuStats first=draw_cpu_stats(d);CHECK(first.uploads==1 && first.uploaded_pixels==1024);
    draw_clear(d,(DrawColor){0,0,0,255});draw_triangles(d,NULL,q,6);CHECK(draw_present(d));DrawCpuStats same=draw_cpu_stats(d);CHECK(same.uploads==first.uploads);
    DrawRect clip={20,20,2,2};draw_clip(d,&clip);draw_rect(d,(DrawRect){0,0,32,32},(DrawColor){10,30,50,255});draw_clip(d,NULL);
    CHECK(draw_present(d));DrawCpuStats changed=draw_cpu_stats(d);CHECK(changed.uploaded_pixels==first.uploaded_pixels+4);
    CHECK(draw_read_rgba(d,pixels,32,32));CHECK(pixels[20*32+20].g==30 && pixels[19*32+20].g==0);
    DrawColor colors[]={{255,0,0,255},{0,255,0,255},{0,0,255,255},{255,255,255,255}};
    DrawTexture *tex=draw_texture_create(d,2,2,colors,0);CHECK(tex);DrawTexture *target=draw_texture_create(d,4,4,NULL,1);CHECK(target);
    draw_target(d,target);draw_clear(d,(DrawColor){0,0,0,0});draw_texture(d,tex,(DrawRect){0,0,2,2},(DrawRect){0,0,4,4},(DrawColor){255,255,255,255});
    DrawColor result[16];CHECK(draw_read_rgba(d,result,4,4));CHECK(result[0].r==255 && result[3].g==255 && result[12].b==255);
    draw_target(d,NULL);draw_texture_destroy(target);draw_texture_destroy(tex);draw_destroy(d);SDL_DestroyRenderer(r);SDL_DestroySurface(s);SDL_Quit();
    puts("CPU clipping, shared triangle edge, targets, textures and final-frame dirty uploads checked");return 0;
}
