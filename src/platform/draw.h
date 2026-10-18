// SPDX-License-Identifier: GPL-3.0-only
#ifndef LIBRELOOP_DRAW_H
#define LIBRELOOP_DRAW_H
#include <stdint.h>
#include <stddef.h>
typedef struct DrawContext DrawContext;
typedef struct DrawTexture DrawTexture;
typedef struct DrawFont DrawFont;
typedef struct FontLayout FontLayout;
typedef struct { float x,y; } DrawPoint;
typedef struct { float x,y,width,height; } DrawRect;
typedef struct { uint8_t r,g,b,a; } DrawColor;
typedef struct { float x,y,z; } DrawHSV;
typedef struct { DrawPoint point; float u,v; DrawColor color; } DrawVertex;
/* Only the SDL platform implementation provides the native renderer pointer. */
DrawContext *draw_create(void *native_renderer);
void draw_destroy(DrawContext *draw);
void draw_transform(DrawContext *draw,float scale,float x,float y);
void draw_pattern_density(DrawContext *draw,float framebuffer_density);
int draw_pattern_pixel_size(const DrawContext *draw);
void draw_clear(DrawContext *draw,DrawColor color);
void draw_rect(DrawContext *draw,DrawRect rect,DrawColor color);
void draw_border(DrawContext *draw,DrawRect rect,float thickness,DrawColor color);
void draw_line(DrawContext *draw,DrawPoint from,DrawPoint to,float thickness,DrawColor color);
void draw_hairline(DrawContext *draw,DrawPoint from,DrawPoint to,DrawColor color);
void draw_clip(DrawContext *draw,const DrawRect *physical_rect);
DrawTexture *draw_texture_create(DrawContext *draw,int width,int height,const void *rgba,int target);
void draw_texture_destroy(DrawTexture *texture);
void draw_texture_filter(DrawTexture *texture,int linear);
void draw_texture(DrawContext *draw,DrawTexture *texture,DrawRect source,DrawRect destination,DrawColor color);
/* Fixed physical-pixel size and target-origin phase, independent of UI transform. */
void draw_texture_tiled_pixels(DrawContext *draw,DrawTexture *texture,DrawRect source,DrawRect destination,DrawColor color);
void draw_texture_pro(DrawContext *draw,DrawTexture *texture,DrawRect source,DrawRect destination,DrawPoint origin,float rotation,DrawColor color);
void draw_triangles(DrawContext *draw,DrawTexture *texture,const DrawVertex *vertices,int count);
void draw_target(DrawContext *draw,DrawTexture *texture);
int draw_read_rgba(DrawContext *draw,void *rgba,int width,int height);
void draw_flush(DrawContext *draw);
int draw_present(DrawContext *draw);
DrawFont *draw_font_create(DrawContext *draw,const unsigned char *data,size_t bytes,int size,const int *codepoints,int count);
DrawFont *draw_font_create_em(DrawContext *draw,const unsigned char *data,size_t bytes,int size,const int *codepoints,int count);
DrawFont *draw_font_bitmap(DrawContext *draw);
void draw_font_destroy(DrawFont *font);
void draw_font_filter(DrawFont *font,int linear);
const FontLayout *draw_font_layout(const DrawFont *font);
DrawPoint draw_measure(const DrawFont *font,const char *text,float size,float spacing);
void draw_text(DrawContext *draw,const DrawFont *font,const char *text,DrawPoint position,float size,float spacing,DrawColor color);
DrawColor draw_color_fade(DrawColor color,float alpha);
DrawColor draw_color_blend(DrawColor destination,DrawColor source,DrawColor tint);
DrawHSV draw_color_hsv(DrawColor color);
DrawColor draw_color_from_hsv(float hue,float saturation,float value);
#endif
