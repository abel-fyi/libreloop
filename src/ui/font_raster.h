// SPDX-License-Identifier: GPL-3.0-only
#ifndef LIBRELOOP_FONT_RASTER_H
#define LIBRELOOP_FONT_RASTER_H
#include <stddef.h>
typedef struct {
    int value,offset_x,offset_y,advance,width,height;
    unsigned char *pixels;
} RasterGlyph;
/* UI/startup only. Raw grayscale masks and integer metrics; no renderer handle. */
RasterGlyph *font_rasterize(const unsigned char *data,size_t bytes,int pixel_size,const int *codepoints,int count);
RasterGlyph *font_rasterize_em(const unsigned char *data,size_t bytes,int pixel_size,const int *codepoints,int count);
void font_raster_free(RasterGlyph *glyphs,int count);
#endif
