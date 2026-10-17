// SPDX-License-Identifier: GPL-3.0-only
#include "font_bitmap.h"
#include "font_layout.h"
#include <stdio.h>
#include <stdlib.h>
static FILE *quads;
static void emit(void *unused,FontQuad q) {
    (void)unused;GlyphRect s=q.source,d=q.destination;
    fprintf(quads,"%.9g %.9g %.9g %.9g %.9g %.9g %.9g %.9g\n",s.x,s.y,s.width,s.height,d.x,d.y,d.width,d.height);
}
#define FACE FontLayout
#define POINT FontPoint
#define MEASURE(f,t,s,p) font_measure(&(f),t,s,p)
#define DRAW(f,t,s,p) font_text_quads(&(f),t,(FontPoint){13.25f,-4.5f},s,p,emit,NULL)

static void capture_layout(FACE font,const char *tag) {
    const char *texts[]={"","A","AV j","\n","line\nshort\nlast","\t A \t","caf\xc3\xa9 \xce\xbb \xd0\x96","\xf0\x9f\x8e\xb9 missing","bad\xc0\xaf\xff","\xe2\x82"};
    const float sizes[]={8,10,15.5f,23},spacings[]={0,1.25f};
    for(int t=0;t<10;t++)for(int s=0;s<4;s++)for(int p=0;p<2;p++) {
        POINT m=MEASURE(font,texts[t],sizes[s],spacings[p]);
        fprintf(quads,"%s %d %d %d %.9g %.9g\n",tag,t,s,p,m.x,m.y);
        DRAW(font,texts[t],sizes[s],spacings[p]);
    }
}

int main(int argc,char **argv) {
    if(argc!=2)return 1;RasterGlyph *glyphs;FontAtlas atlas;if(!font_bitmap_build(&atlas,&glyphs))return 2;
    FILE *out=fopen("bitmap-atlas.bin","wb");fwrite(atlas.gray_alpha,2,atlas.width*atlas.height,out);fclose(out);
    FILE *metrics=fopen("bitmap-metrics.txt","w");
    for(int i=0;i<224;i++) {
        RasterGlyph g=glyphs[i];GlyphRect r=atlas.rects[i];
        fprintf(metrics,"%d %d %d %d %.9g %.9g %.9g %.9g\n",g.value,g.offset_x,g.offset_y,g.advance,r.x,r.y,r.width,r.height);
    }fclose(metrics);quads=fopen("layout.txt","w");FontLayout f={10,224,0,glyphs,atlas.rects};capture_layout(f,"bitmap");
    font_atlas_free(&atlas);font_raster_free(glyphs,224);
    FILE *file=fopen(argv[1],"rb");if(!file || fseek(file,0,SEEK_END))return 2;long bytes=ftell(file);if(bytes<=0 || fseek(file,0,SEEK_SET))return 2;
    unsigned char *data=malloc((size_t)bytes);if(!data || fread(data,1,(size_t)bytes,file)!=(size_t)bytes)return 2;fclose(file);
    int cps[226];for(int i=0;i<224;i++)cps[i]=32+i;cps[224]=0x3bb;cps[225]=0x416;
    glyphs=font_rasterize(data,(size_t)bytes,19,cps,226);if(!glyphs || !font_atlas_build(&atlas,glyphs,226,19,4))return 2;
    f=(FontLayout){19,226,4,glyphs,atlas.rects};capture_layout(f,"ttf");fclose(quads);
    font_atlas_free(&atlas);font_raster_free(glyphs,226);free(data);return 0;
}
