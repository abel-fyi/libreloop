// SPDX-License-Identifier: GPL-3.0-only
#ifndef LIBRELOOP_FONT_ATLAS_H
#define LIBRELOOP_FONT_ATLAS_H
#include "font_raster.h"
typedef struct { float x,y,width,height; } GlyphRect;
typedef struct { int width,height; unsigned char *gray_alpha; GlyphRect *rects; } FontAtlas;
int font_atlas_build(FontAtlas *atlas,const RasterGlyph *glyphs,int count,int pixel_size,int padding);
void font_atlas_free(FontAtlas *atlas);
#endif
