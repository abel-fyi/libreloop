// SPDX-License-Identifier: GPL-3.0-only
#include "font_layout.h"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <assert.h>
#include <math.h>
#include <stdio.h>
static GlyphRect bounds;static float padding;
static void check(void *unused,FontQuad q) {
    (void)unused;GlyphRect r=q.destination;
    assert(r.x+padding>=bounds.x-.001f && r.y+padding>=bounds.y-.001f);
    assert(r.x+r.width-padding<=bounds.x+bounds.width+.001f);
    assert(r.y+r.height-padding<=bounds.y+bounds.height+.001f);
}
int main(void) {
    RasterGlyph glyphs[13]={0};GlyphRect rects[13]={0};
    for(int density=1;density<=2;density++) {
        for(int i=0;i<13;i++) {glyphs[i].value=i==0?'C':i==1?'-':i==2?'?':'0'+i-3;glyphs[i].offset_x=density;glyphs[i].offset_y=3*density;glyphs[i].advance=7*density;rects[i].width=6*density;rects[i].height=9*density;}
        FontLayout font={12*density,13,4,glyphs,rects};
        float heights[]={1,2,6.8f,12,24,100};
        for(int octave=-1;octave<=9;octave++)for(unsigned h=0;h<sizeof heights/sizeof *heights;h++) {
            char name[8];snprintf(name,sizeof name,"C%d",octave);bounds=(GlyphRect){40,86.5f,15,heights[h]};
            FontPoint position;float size;assert(font_fit_ink(&font,name,bounds,12,&position,&size));
            assert(size>0 && size<=12);padding=4*size/font.size;
            font_text_quads(&font,name,position,size,0,check,0);
        }
    }
    return 0;
}
