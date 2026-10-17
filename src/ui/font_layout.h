// SPDX-License-Identifier: GPL-3.0-only
#ifndef LIBRELOOP_FONT_LAYOUT_H
#define LIBRELOOP_FONT_LAYOUT_H
#include "font_atlas.h"
typedef struct FontLayout {
    int size,count,padding;
    const RasterGlyph *glyphs;
    const GlyphRect *rects;
} FontLayout;
typedef struct { float x,y; } FontPoint;
typedef struct { GlyphRect source,destination; } FontQuad;
int font_fit_ink(const FontLayout *font,const char *text,GlyphRect bounds,float maximum,FontPoint *position,float *size);
int font_glyph_index(const FontLayout *font,int codepoint);
FontPoint font_measure(const FontLayout *font,const char *text,float size,float spacing);
void font_text_quads(const FontLayout *font,const char *text,FontPoint position,
    float size,float spacing,void (*emit)(void *,FontQuad),void *context);
#endif
