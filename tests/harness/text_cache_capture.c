// SPDX-License-Identifier: GPL-3.0-only
#include "text_fonts.h"
#include "font_layout.h"
#include <SDL3/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static TextFonts *cache;
static FILE *out;
#define UPDATE(p,s) text_fonts_update(cache,p,s)
#define GET_FONT(t,s) do {const FontLayout *f=draw_font_layout(text_font(cache,t,s));fprintf(out,"font %d %d\n",f->size,f->count);}while(0)
#define WIDTH(t,s) text_font_width(cache,t,s)
#define FIT(o,c,t,w,s) text_fit(cache,o,c,t,w,s)
#define BUTTON(t,w,h) text_button_size(cache,t,w,h)

static void capture(const char *path) {
    const char *texts[]={"","AV j","A\nB","\xc3\xa9\xce\xbb\xd0\x96","\xf0\x9f\x8e\xb9","tab\t stop","\xe2\x82","long label with numbers 12345"};
    const int sizes[]={8,10,13,15,22,23};const float scales[]={1,1.25f,2,2,0,1.5f,1.5f};
    const float widths[]={0,1,15,37.5f,120};const size_t capacities[]={1,7,256};
    for(int stage=0;stage<7;stage++) {
        UPDATE(stage==5?"/libreloop-missing-font.ttf":path,scales[stage]);
        for(int t=0;t<8;t++)for(int s=0;s<6;s++) {
            GET_FONT(texts[t],sizes[s]);
            fprintf(out,"case %d %d %d %.9g\n",stage,t,s,WIDTH(texts[t],sizes[s]));
            for(int w=0;w<5;w++)for(int c=0;c<3;c++) {
                char fit[256];memset(fit,0x5a,sizeof fit);FIT(fit,capacities[c],texts[t],widths[w],sizes[s]);
                fprintf(out,"fit %d %d ",w,c);for(size_t i=0;i<strlen(fit);i++)fprintf(out,"%02x",(unsigned char)fit[i]);fprintf(out,"\n");
            }
            fprintf(out,"button %d %d %d\n",BUTTON(texts[t],16,13),BUTTON(texts[t],45,19),BUTTON(texts[t],120,22));
        }
    }
    for(int i=-30;i<=30;i++)for(int s=1;s<=3;s++) {
        float value=i*.13f,origin=-7.25f,layout=(float)s,raster=layout*1.5f;
        fprintf(out,"pixel %d %d %.9g\n",i,s,text_pixel_position(value,origin,layout,raster));
    }
}

int main(int argc,char **argv) {
    if(argc!=2 || !SDL_Init(0))return 1;SDL_Surface *surface=SDL_CreateSurface(64,64,SDL_PIXELFORMAT_RGBA32);if(!surface)return 2;
    SDL_Renderer *renderer=SDL_CreateSoftwareRenderer(surface);if(!renderer)return 2;DrawContext *draw=draw_create(renderer);if(!draw)return 2;
    cache=text_fonts_create(draw);if(!cache)return 2;out=fopen("cache.txt","wb");if(!out)return 2;
    capture(argv[1]);fclose(out);text_fonts_close(cache);draw_destroy(draw);SDL_DestroyRenderer(renderer);SDL_DestroySurface(surface);SDL_Quit();return 0;
}
