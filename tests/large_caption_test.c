// SPDX-License-Identifier: GPL-3.0-only
#include "text_fonts.h"
#include "font_layout.h"
#include <SDL3/SDL.h>
#include <math.h>
#include <stdio.h>
#define CHECK(x) do {if(!(x)){fprintf(stderr,"%d: %s\n",__LINE__,#x);return 1;}} while(0)
typedef struct {float top,bottom;int count;} Ink;
static void ink(void *context,FontQuad q) {
    Ink *i=context;i->top=fminf(i->top,q.destination.y);
    i->bottom=fmaxf(i->bottom,q.destination.y+q.destination.height);i->count++;
}
int main(int argc,char **argv) {
    CHECK((argc==2 || argc==3) && SDL_Init(0));
    SDL_Surface *surface=SDL_CreateSurface(80,64,SDL_PIXELFORMAT_RGBA32);CHECK(surface);
    SDL_Renderer *renderer=SDL_CreateSoftwareRenderer(surface);CHECK(renderer);
    DrawContext *draw=draw_create(renderer);CHECK(draw);
    TextFonts *fonts=text_fonts_create(draw);CHECK(fonts);
    text_fonts_update(fonts,argv[1],1);
    CHECK(draw_font_layout(text_font(fonts,"Arrangement",23))->size==10);
    text_fonts_large_captions(fonts);
    float original_width=text_font_width(fonts,"Arrangement",16);
    text_fonts_pixel_sampling(fonts);
    CHECK(text_font_width(fonts,"Arrangement",16)==original_width);
    if(argc==3)text_fonts_em_metrics(fonts);
    const float scales[]={1,1.25f,1.5f,2,3};
    for(unsigned s=0;s<sizeof scales/sizeof *scales;s++) {
        text_fonts_update(fonts,argv[1],scales[s]);
        const FontLayout *caption=draw_font_layout(text_font(fonts,"FM Synth · FM Synth",24));
        CHECK(caption->size==(int)roundf(24*scales[s]));
        Ink bounds={.top=1000};
        font_text_quads(caption,"FM Synth · FM Synth",(FontPoint){8,3},24,0,ink,&bounds);
        CHECK(bounds.count>10 && bounds.top>=0 && bounds.bottom<=32);
        Ink compact={.top=1000};
        font_text_quads(draw_font_layout(text_font(fonts,"Arrangement",16)),"Arrangement",(FontPoint){8,3},16,0,ink,&compact);
        if(compact.top<0 || compact.bottom>24)fprintf(stderr,"compact scale %.2f: %.3f..%.3f\n",scales[s],compact.top,compact.bottom);
        CHECK(compact.count>5 && compact.top>=0 && compact.bottom<=24);
        CHECK(text_font_width(fonts,"Arrangement",24)>text_font_width(fonts,"Arrangement",18)*1.15f);
        if(argc==3) {
            CHECK(fabsf(text_font_width(fonts,"MMMM",16)-32)<.001f);
            CHECK(text_font_width(fonts,"Save As...",16)<=272-36);
            CHECK(text_font_width(fonts,"Collect samples and save...",16)<=272-36);
        }
        CHECK(text_font_width(fonts,"Tracking",16)<((argc==3?480:376)-32)/6.f);
        CHECK(text_font_width(fonts,"Release rate",16)<((argc==3?480:376)-32)/4.f);
        CHECK(text_font_width(fonts,"Sustain level",16)<((argc==3?480:376)-32)/4.f);
    }
    text_fonts_close(fonts);draw_destroy(draw);SDL_DestroyRenderer(renderer);SDL_DestroySurface(surface);SDL_Quit();
    puts("Large captions use real font atlases and fit 32-unit titles at all tested densities; legacy fallback remains opt-in.");
    return 0;
}
