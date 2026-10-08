// SPDX-License-Identifier: GPL-3.0-only
#ifndef TEXT_FONTS_H
#define TEXT_FONTS_H
#include "raylib.h"
#include <stddef.h>
/* Rebuild at physical pixel density; extend each atlas for encountered UTF-8 text. */
void text_fonts_update(const char *path,float raster_scale);
Font text_font(const char *text,int size);
void text_fonts_close(void);
/* Logical widths include visible glyph overhangs and use DrawTextEx advances. */
float text_font_width(const char *text,int size);
void text_fit(char *out,size_t capacity,const char *text,float width,int size);
int text_button_size(const char *text,float width,float height);
float text_pixel_position(float value,float origin,float layout_scale,float raster_scale);
#endif
