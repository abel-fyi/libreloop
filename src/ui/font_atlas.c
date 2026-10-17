// SPDX-License-Identifier: GPL-3.0-only
/* Basic atlas packing adapted from pinned raylib5.5 GenImageFontAtlas;
   copyright Ramon Santamaria, zlib notice docs/licenses/raylib.txt.
   Exact size heuristic, padding, wrap threshold and white corner retained. */
#include "font_atlas.h"
#include <stdlib.h>
#include <math.h>
#include <string.h>
int font_atlas_build(FontAtlas *a,const RasterGlyph *glyphs,int count,int size,int padding) {
    *a=(FontAtlas){0};if(!glyphs || count<=0 || size<=0 || padding<0)return 0;
    GlyphRect *recs=malloc((size_t)count*sizeof *recs);if(!recs)return 0;
    int total_width=0;
    for(int i=0;i<count;i++)total_width+=glyphs[i].width+2*padding;
    int padded_size=size+2*padding;
    float total_area=total_width*padded_size*1.2f;
    float minimum=sqrtf(total_area);
    int image_size=(int)powf(2,ceilf(logf(minimum)/logf(2)));
    a->width=image_size;a->height=total_area<((image_size*image_size)/2)?image_size/2:image_size;
    if(a->width<=0 || a->height<=0){free(recs);*a=(FontAtlas){0};return 0;}
    unsigned char *gray=calloc(1,(size_t)a->width*a->height);if(!gray){free(recs);*a=(FontAtlas){0};return 0;}
    int x=padding,y=padding;
    for(int i=0;i<count;i++) {
        if(x>=(a->width-glyphs[i].width-2*padding)) {
            x=padding;y+=size+2*padding;
            if(y>(a->height-size-padding)) {
                /* Same original overflow branch; ordinary UI glyph sets are
                   covered by original reference fixtures, not re-packed. */
                for(int j=i+1;j<count;j++)recs[j]=(GlyphRect){0};
                break;
            }
        }
        for(int row=0;row<glyphs[i].height;row++)for(int col=0;col<glyphs[i].width;col++)
            gray[(y+row)*a->width+x+col]=glyphs[i].pixels[row*glyphs[i].width+col];
        recs[i]=(GlyphRect){(float)x,(float)y,(float)glyphs[i].width,(float)glyphs[i].height};
        x+=glyphs[i].width+2*padding;
    }
    for(int i=0,k=a->width*a->height-1;i<3;i++,k-=a->width){gray[k]=255;gray[k-1]=255;gray[k-2]=255;}
    unsigned char *pixels=malloc((size_t)a->width*a->height*2);
    if(!pixels){free(gray);free(recs);*a=(FontAtlas){0};return 0;}
    for(int i=0,k=0;i<a->width*a->height;i++,k+=2){pixels[k]=255;pixels[k+1]=gray[i];}
    free(gray);a->gray_alpha=pixels;a->rects=recs;return 1;
}
void font_atlas_free(FontAtlas *atlas) { free(atlas->gray_alpha);free(atlas->rects);*atlas=(FontAtlas){0}; }
