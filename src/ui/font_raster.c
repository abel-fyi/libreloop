// SPDX-License-Identifier: GPL-3.0-only
/* Default TTF raster path adapted from raylib5.5 LoadFontData.
 * Original copyright2013–2024 Ramon Santamaria; zlib notice retained in
 * docs/licenses/raylib.txt. No raylib code/library/header is linked.
 * Uses the exact original stb_truetype header and font; no kerning/shaping added.
 */
#include "font_raster.h"
#include <stdlib.h>
#define STB_TRUETYPE_IMPLEMENTATION
#define STBTT_STATIC
#include "stb_truetype.h"
static RasterGlyph *rasterize(const unsigned char *data,size_t bytes,int pixel_size,const int *codepoints,int count,int em) {
    if(!data || !bytes || pixel_size<=0 || !codepoints || count<=0) return NULL;
    stbtt_fontinfo font={0}; if(!stbtt_InitFont(&font,data,0)) return NULL;
    float scale=em?stbtt_ScaleForMappingEmToPixels(&font,(float)pixel_size):stbtt_ScaleForPixelHeight(&font,(float)pixel_size);
    int ascent,descent,line_gap;stbtt_GetFontVMetrics(&font,&ascent,&descent,&line_gap);
    if(em)stbtt_GetFontVMetricsOS2(&font,&ascent,&descent,&line_gap);
    RasterGlyph *glyphs=calloc((size_t)count,sizeof *glyphs);if(!glyphs)return NULL;
    for(int i=0;i<count;i++) {
        RasterGlyph *g=&glyphs[i];int width=0,height=0;g->value=codepoints[i];
        int index=stbtt_FindGlyphIndex(&font,g->value);
        if(index>0) {
            g->pixels=stbtt_GetCodepointBitmap(&font,scale,scale,g->value,&width,&height,&g->offset_x,&g->offset_y);
            if(g->pixels) {
                stbtt_GetCodepointHMetrics(&font,g->value,&g->advance,NULL);
                g->advance=(int)((float)g->advance*scale);
                g->width=width;g->height=height;
                g->offset_y+=(int)((float)ascent*scale);
            }
            if(g->value==32) {
                stbtt_GetCodepointHMetrics(&font,g->value,&g->advance,NULL);
                g->advance=(int)((float)g->advance*scale);
                g->pixels=calloc((size_t)g->advance*pixel_size,2);
                g->width=g->advance;g->height=pixel_size;
            }
        }
    }
    return glyphs;
}
RasterGlyph *font_rasterize(const unsigned char *data,size_t bytes,int size,const int *codepoints,int count){return rasterize(data,bytes,size,codepoints,count,0);}
RasterGlyph *font_rasterize_em(const unsigned char *data,size_t bytes,int size,const int *codepoints,int count){return rasterize(data,bytes,size,codepoints,count,1);}
void font_raster_free(RasterGlyph *glyphs,int count) {
    if(!glyphs)return;
    for(int i=0;i<count;i++)stbtt_FreeBitmap(glyphs[i].pixels,NULL);
    free(glyphs);
}
