// SPDX-License-Identifier: GPL-3.0-only
#include "icons.h"
#include <SDL3/SDL.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#define CHECK(x) do { if(!(x)){fprintf(stderr,"Classic icons line %d: %s\n",__LINE__,#x);return 1;} } while(0)
int main(void) {
    CHECK(icon_action("FILE")==ICON_FILE && icon_action("EDIT")==ICON_PENCIL);
    CHECK(icon_action("VIEW")==ICON_VIEW && icon_action("HELP")==ICON_HELP);
    CHECK(icon_action("My bass preset")==-1 && icon_action("110.00")==-1);
    CHECK(SDL_Init(0));SDL_Surface *s=SDL_CreateSurface(2048,512,SDL_PIXELFORMAT_RGBA32);CHECK(s);
    SDL_Renderer *renderer=SDL_CreateSoftwareRenderer(s);CHECK(renderer);DrawContext *draw=draw_create(renderer);CHECK(draw);
    const float scales[]={1,1.25f,1.5f,2,3};
    for(unsigned d=0;d<sizeof scales/sizeof *scales;d++) {
        UIIcons icons={0};CHECK(icons_build_classic(draw,scales[d],&icons));
        int width=ICON_CLASSIC_COUNT*icons.large,height=icons.large+icons.medium+icons.small;
        DrawTexture *target=draw_texture_create(draw,width,height,NULL,1);CHECK(target);
        draw_target(draw,target);draw_transform(draw,1,0,0);draw_clear(draw,(DrawColor){0,0,0,0});
        draw_texture(draw,icons.texture,(DrawRect){0,0,width,height},(DrawRect){0,0,width,height},(DrawColor){255,255,255,255});
        unsigned char *pixels=malloc((size_t)width*height*4);CHECK(pixels);CHECK(draw_read_rgba(draw,pixels,width,height));
        for(int v=0;v<3;v++) {
            int size=v==0?icons.large:v==1?icons.medium:icons.small,row=v==0?0:v==1?icons.large:icons.large+icons.medium;
            for(int id=0;id<ICON_CLASSIC_COUNT;id++) {
                int ink=0,clear=0,color=0;
                for(int y=0;y<size;y++)for(int x=0;x<size;x++) {
                    const unsigned char *p=pixels+((row+y)*width+id*icons.large+x)*4;
                    CHECK(p[3]==0 || p[3]==255); /* No smoothing/halos at fractional DPI. */
                    if(p[3]){ink++;if(p[0]!=p[1] || p[1]!=p[2])color++;}else clear++;
                }
                CHECK(ink>0 && clear>0);if(id!=ICON_CLOSE && id!=ICON_SELECT)CHECK(color>0);
            }
        }
        free(pixels);draw_target(draw,NULL);draw_texture_destroy(target);icons_destroy(&icons);
    }
    draw_destroy(draw);SDL_DestroyRenderer(renderer);SDL_DestroySurface(s);SDL_Quit();
    puts("All classic sprites retain colour, transparency and hard edges at five DPI scales.");return 0;
}
