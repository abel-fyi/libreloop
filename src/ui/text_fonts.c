// SPDX-License-Identifier: GPL-3.0-only
#include "text_fonts.h"
#include "file_io.h"
#include "font_layout.h"
#include "utf8.h"
#include "path.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct { DrawFont *font; int *codepoints,count,capacity,dirty; } TextFont;
struct TextFonts {
    DrawContext *draw;
    DrawFont *fallback;
    TextFont fonts[29];
    int maximum;
    int pixel_sampling,em_metrics;
    char font_path[LL_PATH_CAPACITY];
    float density;
};
static void unload(TextFonts *cache,DrawFont *font) {
    if(font && font!=cache->fallback)draw_font_destroy(font);
}
TextFonts *text_fonts_create(DrawContext *draw) {
    TextFonts *cache=calloc(1,sizeof *cache);if(!cache)return NULL;
    cache->draw=draw;cache->maximum=22;cache->fallback=draw_font_bitmap(draw);
    if(!cache->fallback){free(cache);return NULL;}return cache;
}
void text_fonts_close(TextFonts *cache) {
    if(!cache)return;
    draw_flush(cache->draw);
    for(int i=10;i<=28;i++){unload(cache,cache->fonts[i].font);free(cache->fonts[i].codepoints);}
    draw_font_destroy(cache->fallback);free(cache);
}
void text_fonts_update(TextFonts *cache,const char *path,float scale) {
    if(cache->density==scale && !strcmp(cache->font_path,path))return;
    draw_flush(cache->draw);
    for(int i=10;i<=28;i++){unload(cache,cache->fonts[i].font);cache->fonts[i].font=NULL;cache->fonts[i].dirty=1;}
    cache->density=scale;snprintf(cache->font_path,sizeof cache->font_path,"%s",path);
}
void text_fonts_large_captions(TextFonts *cache) { if(cache)cache->maximum=28; }
void text_fonts_pixel_sampling(TextFonts *cache) {
    if(!cache || cache->pixel_sampling)return;
    draw_flush(cache->draw);cache->pixel_sampling=1;
    for(int i=10;i<=28;i++)if(cache->fonts[i].font)draw_font_filter(cache->fonts[i].font,0);
}
void text_fonts_em_metrics(TextFonts *cache) {
    if(!cache || cache->em_metrics)return;cache->em_metrics=1;
    for(int i=10;i<=28;i++)cache->fonts[i].dirty=1;
}
static DrawFont *load_font(TextFonts *cache,int size,const int *codepoints,int count) {
    FILE *file=platform_file_open(cache->font_path,"rb");if(!file)return NULL;
    if(fseek(file,0,SEEK_END)){fclose(file);return NULL;}
    long length=ftell(file);if(length<=0 || fseek(file,0,SEEK_SET)){fclose(file);return NULL;}
    unsigned char *data=malloc((size_t)length);if(!data){fclose(file);return NULL;}
    if(fread(data,1,(size_t)length,file)!=(size_t)length){free(data);fclose(file);return NULL;}
    fclose(file);DrawFont *font=cache->em_metrics?draw_font_create_em(cache->draw,data,(size_t)length,size,codepoints,count):draw_font_create(cache->draw,data,(size_t)length,size,codepoints,count);free(data);return font;
}
static int add(TextFont *atlas,int codepoint) {
    if(codepoint<32 || codepoint>0x10ffff) return 1;
    for(int i=0;i<atlas->count;i++) if(atlas->codepoints[i]==codepoint) return 1;
    if(atlas->count==atlas->capacity) {
        int next=atlas->capacity?atlas->capacity*2:256;
        int *grown=realloc(atlas->codepoints,(size_t)next*sizeof *grown); if(!grown) return 0;
        atlas->codepoints=grown; atlas->capacity=next;
    }
    atlas->codepoints[atlas->count++]=codepoint; atlas->dirty=1; return 1;
}
DrawFont *text_font(TextFonts *cache,const char *text,int size) {
    if(size<10 || size>cache->maximum || cache->density<=0) return cache->fallback;
    TextFont *atlas=&cache->fonts[size];
    if(!atlas->count) for(int i=32;i<256;i++) if(!add(atlas,i)) break;
    for(int i=0;text && text[i];) {
        int bytes=1,codepoint=utf8_next(text+i,&bytes); if(bytes<1) bytes=1;
        if(!add(atlas,codepoint)) break;
        i+=bytes;
    }
    if(atlas->dirty) {
        /* Draw commands can still reference the old texture during this frame. */
        draw_flush(cache->draw); unload(cache,atlas->font);
        atlas->font=load_font(cache,(int)fmaxf(1,roundf(size*cache->density)),atlas->codepoints,atlas->count);
        if(!atlas->font) atlas->font=cache->fallback;
        draw_font_filter(atlas->font,!cache->pixel_sampling); atlas->dirty=0;
    }
    return atlas->font;
}

float text_pixel_position(float value,float origin,float layout_scale,float raster_scale) {
    float pixels=origin*raster_scale/layout_scale;
    return (roundf(value*raster_scale+pixels)-pixels)/raster_scale;
}

static float advance(const FontLayout *font,int glyph) {
    return font->glyphs[glyph].advance!=0?font->glyphs[glyph].advance:font->rects[glyph].width;
}
static float glyph_width(const FontLayout *font,int glyph,float *position,float *left,float *right) {
    *left=fminf(*left,*position+font->glyphs[glyph].offset_x);
    *right=fmaxf(*right,*position+font->glyphs[glyph].offset_x+font->rects[glyph].width);
    *position+=advance(font,glyph);
    return fmaxf(*position,*right)-*left;
}
float text_font_width(TextFonts *cache,const char *text,int size) {
    const FontLayout *font=draw_font_layout(text_font(cache,text,size)); float position=0,left=0,right=0,maximum=0;
    for(int i=0;text && text[i];) {
        int bytes,codepoint=utf8_next(text+i,&bytes); if(bytes<1) bytes=1;
        if(codepoint=='\n') position=left=right=0;
        else maximum=fmaxf(maximum,glyph_width(font,font_glyph_index(font,codepoint),&position,&left,&right));
        i+=bytes;
    }
    return cache->density>0?maximum/cache->density:maximum;
}
void text_fit(TextFonts *cache,char *out,size_t capacity,const char *text,float width,int size) {
    if(!capacity) return;
    if(!text || width<=0) { out[0]=0; return; }
    const FontLayout *font=draw_font_layout(text_font(cache,text,size)); float position=0,left=0,right=0;
    size_t used=0;
    for(size_t i=0;text[i];) {
        int bytes,codepoint=utf8_next(text+i,&bytes); if(bytes<1) bytes=1;
        if(used+(size_t)bytes>=capacity) break;
        if(codepoint=='\n') position=left=right=0;
        else {
            float pixels=glyph_width(font,font_glyph_index(font,codepoint),&position,&left,&right);
            if(ceilf(cache->density>0?pixels/cache->density:pixels)>width) break;
        }
        memmove(out+used,text+i,(size_t)bytes); used+=(size_t)bytes; i+=(size_t)bytes;
    }
    out[used]=0;
}
int text_button_size(TextFonts *cache,const char *text,float width,float height) {
    int preferred=(int)fmaxf(10,fminf(15,height-4));
    for(int size=preferred;size>=10;size--) if(ceilf(text_font_width(cache,text,size))<=width) return size;
    /* Genuinely long data labels still truncate at a readable size. */
    return preferred;
}
