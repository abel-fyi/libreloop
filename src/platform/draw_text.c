// SPDX-License-Identifier: GPL-3.0-only
#include "draw.h"
#include "font_bitmap.h"
#include "font_layout.h"
#include <stdlib.h>
struct DrawFont { FontLayout layout; FontAtlas atlas; RasterGlyph *glyphs; DrawTexture *texture; };
static DrawFont *upload(DrawContext *draw,DrawFont *font) {
    size_t count=(size_t)font->atlas.width*font->atlas.height;
    unsigned char *rgba=malloc(count*4);
    if(!rgba){draw_font_destroy(font);return NULL;}
    for(size_t i=0;i<count;i++) {
        rgba[i*4]=rgba[i*4+1]=rgba[i*4+2]=font->atlas.gray_alpha[i*2];
        rgba[i*4+3]=font->atlas.gray_alpha[i*2+1];
    }
    font->texture=draw_texture_create(draw,font->atlas.width,font->atlas.height,rgba,0);free(rgba);
    if(!font->texture){draw_font_destroy(font);return NULL;}
    return font;
}
static DrawFont *create(DrawContext *draw,const unsigned char *data,size_t bytes,int size,const int *codepoints,int count,int em) {
    DrawFont *font=calloc(1,sizeof *font);if(!font)return NULL;
    font->layout=(FontLayout){size,count,4,NULL,NULL};
    font->glyphs=em?font_rasterize_em(data,bytes,size,codepoints,count):font_rasterize(data,bytes,size,codepoints,count);
    if(!font->glyphs || !font_atlas_build(&font->atlas,font->glyphs,count,size,4)){draw_font_destroy(font);return NULL;}
    font->layout.glyphs=font->glyphs;font->layout.rects=font->atlas.rects;
    font=upload(draw,font);if(font)draw_font_filter(font,1);return font;
}
DrawFont *draw_font_create(DrawContext *draw,const unsigned char *data,size_t bytes,int size,const int *codepoints,int count){return create(draw,data,bytes,size,codepoints,count,0);}
DrawFont *draw_font_create_em(DrawContext *draw,const unsigned char *data,size_t bytes,int size,const int *codepoints,int count){return create(draw,data,bytes,size,codepoints,count,1);}
DrawFont *draw_font_bitmap(DrawContext *draw) {
    DrawFont *font=calloc(1,sizeof *font);if(!font)return NULL;
    font->layout=(FontLayout){10,224,0,NULL,NULL};
    if(!font_bitmap_build(&font->atlas,&font->glyphs)){free(font);return NULL;}
    font->layout.glyphs=font->glyphs;font->layout.rects=font->atlas.rects;
    return upload(draw,font);
}
void draw_font_destroy(DrawFont *font) {
    if(!font)return;
    draw_texture_destroy(font->texture);font_raster_free(font->glyphs,font->layout.count);
    font_atlas_free(&font->atlas);free(font);
}
void draw_font_filter(DrawFont *font,int linear) { if(font)draw_texture_filter(font->texture,linear); }
const FontLayout *draw_font_layout(const DrawFont *font) { return font?&font->layout:NULL; }
DrawPoint draw_measure(const DrawFont *font,const char *text,float size,float spacing) {
    if(!font)return (DrawPoint){0};FontPoint p=font_measure(&font->layout,text,size,spacing);return (DrawPoint){p.x,p.y};
}
typedef struct { DrawContext *draw; const DrawFont *font; DrawColor color; } TextDraw;
static void emit(void *context,FontQuad quad) {
    TextDraw *text=context;GlyphRect s=quad.source,d=quad.destination;
    draw_texture(text->draw,text->font->texture,(DrawRect){s.x,s.y,s.width,s.height},(DrawRect){d.x,d.y,d.width,d.height},text->color);
}
void draw_text(DrawContext *draw,const DrawFont *font,const char *text,DrawPoint position,float size,float spacing,DrawColor color) {
    if(!font)return;TextDraw context={draw,font,color};
    font_text_quads(&font->layout,text,(FontPoint){position.x,position.y},size,spacing,emit,&context);
}
