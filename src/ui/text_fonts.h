// SPDX-License-Identifier: GPL-3.0-only
#ifndef LIBRELOOP_TEXT_FONTS_H
#define LIBRELOOP_TEXT_FONTS_H
#include "draw.h"
enum { UI_TEXT_SIZE=16 };
typedef struct TextFonts TextFonts;
TextFonts *text_fonts_create(DrawContext *draw);
/* Opt in to 23..28px caption atlases; default legacy fallback stays at 22. */
void text_fonts_large_captions(TextFonts *cache);
/* Preserve atlas metrics while sampling its pixels without extra smoothing. */
void text_fonts_pixel_sampling(TextFonts *cache);
void text_fonts_em_metrics(TextFonts *cache);
void text_fonts_close(TextFonts *cache);
void text_fonts_update(TextFonts *cache,const char *path,float scale);
DrawFont *text_font(TextFonts *cache,const char *text,int size);
float text_font_width(TextFonts *cache,const char *text,int size);
float text_pixel_position(float value,float origin,float layout_scale,float raster_scale);
void text_fit(TextFonts *cache,char *out,size_t capacity,const char *text,float width,int size);
int text_button_size(TextFonts *cache,const char *text,float width,float height);
#endif
