// SPDX-License-Identifier: GPL-3.0-only
/* Adapted from pinned raylib5.5 GetGlyphIndex/MeasureTextEx/DrawTextEx/
   DrawTextCodepoint. Copyright Ramon Santamaria; zlib notice retained in
   docs/licenses/raylib.txt. Preserve the original 2px line spacing. */
#include "font_layout.h"
#include "utf8.h"
#include <string.h>
#include <math.h>
int font_glyph_index(const FontLayout *font,int codepoint) {
    int index=0,fallback=0;
    for(int i=0;i<font->count;i++) {
        if(font->glyphs[i].value==63)fallback=i;
        if(font->glyphs[i].value==codepoint){index=i;break;}
    }
    if(index==0 && font->glyphs[0].value!=codepoint)index=fallback;
    return index;
}
FontPoint font_measure(const FontLayout *font,const char *text,float size,float spacing) {
    FontPoint result={0};if(!font || !text || !text[0])return result;
    int length=(int)strlen(text),longest_count=0,count=0;
    float width=0,longest_width=0,height=size,scale=size/(float)font->size;
    for(int i=0;i<length;) {
        count++;int bytes=0,cp=utf8_next(text+i,&bytes),index=font_glyph_index(font,cp);i+=bytes;
        if(cp!='\n') {
            if(font->glyphs[index].advance>0)width+=font->glyphs[index].advance;
            else width+=font->rects[index].width+font->glyphs[index].offset_x;
        } else {
            if(longest_width<width)longest_width=width;
            count=0;width=0;height+=size+2;
        }
        if(longest_count<count)longest_count=count;
    }
    if(longest_width<width)longest_width=width;
    result.x=longest_width*scale+(float)((longest_count-1)*spacing);result.y=height;return result;
}
void font_text_quads(const FontLayout *font,const char *text,FontPoint position,
    float size,float spacing,void (*emit)(void *,FontQuad),void *context) {
    if(!font || !text || !emit)return;
    int length=(int)strlen(text);float y=0,x=0,scale=size/font->size;
    for(int i=0;i<length;) {
        int bytes=0,cp=utf8_next(text+i,&bytes),index=font_glyph_index(font,cp);
        if(cp=='\n'){y+=size+2;x=0;}else {
            RasterGlyph g=font->glyphs[index];GlyphRect r=font->rects[index];
            if(cp!=' ' && cp!='\t') {
                FontQuad quad={
                    {r.x-(float)font->padding,r.y-(float)font->padding,
                     r.width+2.0f*font->padding,r.height+2.0f*font->padding},
                    {position.x+x+g.offset_x*scale-(float)font->padding*scale,
                     position.y+y+g.offset_y*scale-(float)font->padding*scale,
                     (r.width+2.0f*font->padding)*scale,(r.height+2.0f*font->padding)*scale}};
                emit(context,quad);
            }
            if(g.advance==0)x+=(float)r.width*scale+spacing;
            else x+=(float)g.advance*scale+spacing;
        }
        i+=bytes;
    }
}

/* Fit visible glyph ink, excluding transparent atlas padding. */
int font_fit_ink(const FontLayout *font,const char *text,GlyphRect b,float maximum,FontPoint *position,float *size) {
    if(!font || !text || !*text || b.width<=0 || b.height<=0)return 0;
    float x=0,left=1e9f,top=1e9f,right=-1e9f,bottom=-1e9f;
    for(int i=0;text[i];) {
        int bytes=0,cp=utf8_next(text+i,&bytes);if(cp=='\n')return 0;i+=bytes;
        int index=font_glyph_index(font,cp);RasterGlyph g=font->glyphs[index];GlyphRect r=font->rects[index];
        left=fminf(left,x+g.offset_x);top=fminf(top,g.offset_y);
        right=fmaxf(right,x+g.offset_x+r.width);bottom=fmaxf(bottom,g.offset_y+r.height);
        x+=g.advance?g.advance:r.width;
    }
    if(right<=left || bottom<=top)return 0;
    float scale=fminf(maximum/font->size,fminf(b.width/(right-left),b.height/(bottom-top)));
    *size=scale*font->size;*position=(FontPoint){b.x+b.width-right*scale,b.y+(b.height-(bottom-top)*scale)/2-top*scale};return 1;
}
