// SPDX-License-Identifier: GPL-3.0-only
#ifndef TEXT_FONTS_H
#define TEXT_FONTS_H
#include "raylib.h"
/* Rebuild at physical pixel density; extend each atlas for encountered UTF-8 text. */
void text_fonts_update(const char *path,float raster_scale);
Font text_font(const char *text,int size);
void text_fonts_close(void);
float text_pixel_position(float value,float origin,float layout_scale,float raster_scale);
#endif
