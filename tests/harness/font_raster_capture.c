// SPDX-License-Identifier: GPL-3.0-only
#include "font_raster.h"
#include "font_atlas.h"
#include <stdio.h>
#include <stdlib.h>
int main(int argc,char **argv) {
    if(argc!=3)return 1;int size=atoi(argv[2]);FILE *file=fopen(argv[1],"rb");if(!file)return 2;
    if(fseek(file,0,SEEK_END))return 2;long bytes=ftell(file);if(bytes<=0 || fseek(file,0,SEEK_SET))return 2;
    unsigned char *font=malloc((size_t)bytes);if(!font || fread(font,1,(size_t)bytes,file)!=(size_t)bytes)return 2;fclose(file);
    int codepoints[228];for(int i=0;i<224;i++)codepoints[i]=32+i;
    codepoints[224]=0x3bb;codepoints[225]=0x416;codepoints[226]=0x1f3b9;codepoints[227]=0x10ffff;
    RasterGlyph *glyphs=font_rasterize(font,(size_t)bytes,size,codepoints,228);if(!glyphs)return 3;
    FILE *out=fopen("glyphs.bin","wb"),*metrics=fopen("metrics.txt","w");if(!out || !metrics)return 4;
    for(int i=0;i<228;i++) {
        RasterGlyph g=glyphs[i];
        fprintf(metrics,"%d %d %d %d %d %d\n",g.value,g.offset_x,g.offset_y,g.advance,g.width,g.height);
        if(g.width*g.height)fwrite(g.pixels,1,g.width*g.height,out);
    }
    fclose(out);fclose(metrics);
    FontAtlas atlas;if(!font_atlas_build(&atlas,glyphs,228,size,4))return 4;
    FILE *image=fopen("atlas.bin","wb"),*locations=fopen("atlas-recs.txt","w");if(!image || !locations)return 4;
    fwrite(atlas.gray_alpha,2,(size_t)atlas.width*atlas.height,image);fclose(image);
    fprintf(locations,"%d %d\n",atlas.width,atlas.height);
    for(int i=0;i<228;i++)fprintf(locations,"%.9g %.9g %.9g %.9g\n",atlas.rects[i].x,atlas.rects[i].y,atlas.rects[i].width,atlas.rects[i].height);
    fclose(locations);font_atlas_free(&atlas);
    font_raster_free(glyphs,228);free(font);return 0;
}
