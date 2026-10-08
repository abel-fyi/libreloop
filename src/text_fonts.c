// SPDX-License-Identifier: GPL-3.0-only
#include "text_fonts.h"
#include "rlgl.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
typedef struct { Font font; int *codepoints,count,capacity,dirty; } TextFont;
static TextFont fonts[23];
static char font_path[PATH_MAX];
static float density;
static void unload(Font font) {
    if(font.texture.id && font.texture.id!=GetFontDefault().texture.id) UnloadFont(font);
}
void text_fonts_close(void) {
    rlDrawRenderBatchActive();
    for(int i=10;i<=22;i++) { unload(fonts[i].font); free(fonts[i].codepoints); }
    memset(fonts,0,sizeof fonts); density=0;
}
void text_fonts_update(const char *path,float scale) {
    if(density==scale && !strcmp(font_path,path)) return;
    rlDrawRenderBatchActive();
    for(int i=10;i<=22;i++) { unload(fonts[i].font); fonts[i].font=(Font){0}; fonts[i].dirty=1; }
    density=scale; snprintf(font_path,sizeof font_path,"%s",path);
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
Font text_font(const char *text,int size) {
    if(size<10 || size>22 || density<=0) return GetFontDefault();
    TextFont *atlas=&fonts[size];
    if(!atlas->count) for(int i=32;i<256;i++) if(!add(atlas,i)) break;
    for(int i=0;text && text[i];) {
        int bytes=1,codepoint=GetCodepointNext(text+i,&bytes); if(bytes<1) bytes=1;
        if(!add(atlas,codepoint)) break;
        i+=bytes;
    }
    if(atlas->dirty) {
        /* Draw commands can still reference the old texture during this frame. */
        rlDrawRenderBatchActive(); unload(atlas->font);
        atlas->font=LoadFontEx(font_path,(int)fmaxf(1,roundf(size*density)),atlas->codepoints,atlas->count);
        if(!atlas->font.texture.id) atlas->font=GetFontDefault();
        SetTextureFilter(atlas->font.texture,TEXTURE_FILTER_BILINEAR); atlas->dirty=0;
    }
    return atlas->font;
}

float text_pixel_position(float value,float origin,float layout_scale,float raster_scale) {
    float pixels=origin*raster_scale/layout_scale;
    return (roundf(value*raster_scale+pixels)-pixels)/raster_scale;
}

static float advance(Font font,int glyph) {
    return font.glyphs[glyph].advanceX!=0?font.glyphs[glyph].advanceX:font.recs[glyph].width;
}
static float glyph_width(Font font,int glyph,float *position,float *left,float *right) {
    *left=fminf(*left,*position+font.glyphs[glyph].offsetX);
    *right=fmaxf(*right,*position+font.glyphs[glyph].offsetX+font.recs[glyph].width);
    *position+=advance(font,glyph);
    return fmaxf(*position,*right)-*left;
}
float text_font_width(const char *text,int size) {
    Font font=text_font(text,size); float position=0,left=0,right=0,maximum=0;
    for(int i=0;text && text[i];) {
        int bytes,codepoint=GetCodepointNext(text+i,&bytes); if(bytes<1) bytes=1;
        if(codepoint=='\n') position=left=right=0;
        else maximum=fmaxf(maximum,glyph_width(font,GetGlyphIndex(font,codepoint),&position,&left,&right));
        i+=bytes;
    }
    return density>0?maximum/density:maximum;
}
void text_fit(char *out,size_t capacity,const char *text,float width,int size) {
    if(!capacity) return;
    if(!text || width<=0) { out[0]=0; return; }
    Font font=text_font(text,size); float position=0,left=0,right=0;
    size_t used=0;
    for(size_t i=0;text[i];) {
        int bytes,codepoint=GetCodepointNext(text+i,&bytes); if(bytes<1) bytes=1;
        if(used+(size_t)bytes>=capacity) break;
        if(codepoint=='\n') position=left=right=0;
        else {
            float pixels=glyph_width(font,GetGlyphIndex(font,codepoint),&position,&left,&right);
            if(ceilf(density>0?pixels/density:pixels)>width) break;
        }
        memmove(out+used,text+i,(size_t)bytes); used+=(size_t)bytes; i+=(size_t)bytes;
    }
    out[used]=0;
}
int text_button_size(const char *text,float width,float height) {
    int preferred=(int)fmaxf(10,fminf(15,height-4));
    for(int size=preferred;size>=10;size--) if(ceilf(text_font_width(text,size))<=width) return size;
    /* Genuinely long data labels still truncate at a readable size. */
    return preferred;
}
