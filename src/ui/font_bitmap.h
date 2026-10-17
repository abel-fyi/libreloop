// SPDX-License-Identifier: GPL-3.0-only
#ifndef LIBRELOOP_FONT_BITMAP_H
#define LIBRELOOP_FONT_BITMAP_H
#include "font_atlas.h"
/* Existing 10px/224-glyph bitmap fallback, not a new font. */
int font_bitmap_build(FontAtlas *atlas,RasterGlyph **glyphs);
#endif
