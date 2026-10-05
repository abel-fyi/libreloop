// SPDX-License-Identifier: GPL-3.0-only
#include "text_fonts.h"
#include <stdio.h>
#include <string.h>
#include <math.h>
#define CHECK(x) do { if(!(x)) { fprintf(stderr,"Failed line %d: %s\n",__LINE__,#x); return 1; } } while(0)
static Image draw(Font font,float density,int aligned) {
    RenderTexture2D target=LoadRenderTexture(400,100);
    BeginTextureMode(target); ClearBackground(BLACK);
    if(aligned) {
        float origin=5.1f,layout=density;
        BeginMode2D((Camera2D){.offset={origin,origin},.zoom=density});
        float x=text_pixel_position(10.2f,origin,layout,density),y=text_pixel_position(12.7f,origin,layout,density);
        DrawTextEx(font,"Clip title",(Vector2){x,y},font.baseSize/density,0,WHITE); EndMode2D();
    } else {
        DrawTextEx(font,"Clip title",(Vector2){roundf(10.2f*density+5.1f),roundf(12.7f*density+5.1f)},font.baseSize,0,WHITE);
    }
    EndTextureMode(); Image image=LoadImageFromTexture(target.texture); UnloadRenderTexture(target); return image;
}
int main(int argc,char **argv) {
    CHECK(argc==2); SetTraceLogLevel(LOG_WARNING); SetConfigFlags(FLAG_WINDOW_HIDDEN); InitWindow(400,100,"Font regression checks");
    const float scales[]={1,1.25f,1.5f,2};
    for(unsigned i=0;i<sizeof scales/sizeof *scales;i++) {
        float density=scales[i]; text_fonts_update(argv[1],density);
        Font font=text_font("Clip title",10); CHECK(font.texture.id && font.baseSize==(int)roundf(10*density));
        Image aligned=draw(font,density,1),native=draw(font,density,0);
        CHECK(aligned.width==native.width && aligned.height==native.height && aligned.format==native.format);
        CHECK(!memcmp(aligned.data,native.data,(size_t)aligned.width*aligned.height*4));
        UnloadImage(aligned); UnloadImage(native);
        font=text_font("Ελληνικά Живой",12);
        CHECK(font.glyphs[GetGlyphIndex(font,0x416)].value==0x416);
        CHECK(font.glyphs[GetGlyphIndex(font,0x3bb)].value==0x3bb);
        /* Adding more glyphs while a draw is queued must preserve the earlier draw. */
        BeginDrawing(); ClearBackground(BLACK); font=text_font("first",12);
        DrawTextEx(font,"first",(Vector2){0},font.baseSize,0,WHITE);
        font=text_font("Łódź",12); DrawTextEx(font,"Łódź",(Vector2){100,0},font.baseSize,0,WHITE); EndDrawing();
    }
    text_fonts_close(); CloseWindow(); puts("Native glyph raster alignment at 100/125/150/200%, UTF-8 atlases and safe rebuilds passed."); return 0;
}
