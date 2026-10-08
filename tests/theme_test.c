// SPDX-License-Identifier: GPL-3.0-only
#include "theme.h"
#include <stdio.h>
#include <string.h>
#include <math.h>
#define CHECK(x) do { if(!(x)) { fprintf(stderr,"Failed line %d: %s\n",__LINE__,#x); return 1; } } while(0)
static float luminance(Color c) {
    float v[]={c.r/255.f,c.g/255.f,c.b/255.f};
    for(int i=0;i<3;i++) v[i]=v[i]<=.04045f?v[i]/12.92f:powf((v[i]+.055f)/1.055f,2.4f);
    return .2126f*v[0]+.7152f*v[1]+.0722f*v[2];
}
static float contrast(Color a,Color b) { float x=luminance(a),y=luminance(b); return (fmaxf(x,y)+.05f)/(fminf(x,y)+.05f); }
static int same(Color a,Color b) { return a.r==b.r && a.g==b.g && a.b==b.b && a.a==b.a; }
static int check_render(void) {
    RenderTexture2D target=LoadRenderTexture(80,64);
    BeginTextureMode(target); ClearBackground(ui_theme.surface);
    ui_fader_handle((Rectangle){10,10,14,18},0); ui_fader_handle((Rectangle){40,10,14,18},1);
    EndTextureMode(); Image image=LoadImageFromTexture(target.texture);
    int mark=0,selected=0;
    for(int y=0;y<64;y++) for(int x=12;x<23;x++) mark+=same(GetImageColor(image,x,y),ui_theme.fader_mark);
    for(int y=0;y<64;y++) for(int x=42;x<53;x++) selected+=same(GetImageColor(image,x,y),ui_theme.selected_text);
    UnloadImage(image); UnloadRenderTexture(target); return mark>=8 && selected>=8;
}
int main(int argc,char **argv) {
    int render=argc>1 && !strcmp(argv[1],"--render");
    if(render) { SetTraceLogLevel(LOG_WARNING); SetConfigFlags(FLAG_WINDOW_HIDDEN); InitWindow(80,64,"Theme checks"); }
    theme_init();
    CHECK(same(ui_theme.background,(Color){9,7,15,255}));
    CHECK(contrast(ui_theme.selected_text,ui_theme.highlight)>=4.5f);
    CHECK(contrast(theme_foreground(ui_theme.active_hover),ui_theme.active_hover)>=4.5f);
    Color surfaces[]={ui_theme.surface,ui_theme.browser,ui_theme.title,ui_theme.title_focus};
    for(unsigned i=0;i<sizeof surfaces/sizeof *surfaces;i++) {
        CHECK(contrast(ui_theme.text,surfaces[i])>=4.5f);
        CHECK(contrast(ui_theme.secondary,surfaces[i])>=4.5f);
    }
    CHECK(contrast(ui_theme.note,ui_theme.piano_row[0])>=3);
    CHECK(contrast(ui_theme.note,ui_theme.piano_row[1])>=3);
    CHECK(contrast(ui_theme.waveform,ui_theme.background)>=3);
    CHECK(contrast(theme_foreground(ui_theme.hover),ui_theme.hover)>=4.5f);
    CHECK(same(ui_theme.highlight,(Color){179,154,221,255}));
    CHECK(same(ui_theme.hover,ui_theme.highlight));
    CHECK(contrast(ui_theme.border,ui_theme.control)>=3);
    CHECK(contrast(ui_theme.knob_track,ui_theme.control)>=3);
    float delta=fabsf(ColorToHSV(ui_theme.pan_left).x-ColorToHSV(ui_theme.pan_right).x);
    CHECK(fminf(delta,360-delta)>=90);
    Color hints[]={ui_theme.pan_left,ui_theme.pan_right,ui_theme.stereo,ui_theme.mono};
    for(unsigned i=0;i<sizeof hints/sizeof *hints;i++) {
        Color hint=ColorAlphaBlend(ui_theme.knob_track,Fade(hints[i],.35f),WHITE);
        CHECK(contrast(hint,ui_theme.control)>=3 && contrast(hint,ui_theme.surface)>=3);
    }
    if(render) CHECK(check_render());
    CHECK(contrast(ui_theme.fader_mark,ui_theme.fader)>=4.5f);
    CHECK(contrast(ui_theme.text,ui_theme.control)>=4.5f && contrast(ui_theme.secondary,ui_theme.control)>=4.5f);
    Color indicators[]={ui_theme.pan_left,ui_theme.pan_right,ui_theme.stereo,ui_theme.mono};
    for(unsigned i=0;i<sizeof indicators/sizeof *indicators;i++) CHECK(contrast(indicators[i],ui_theme.control)>=3);
    Color meters[]={ui_theme.meter_low,ui_theme.meter_mid,ui_theme.meter_high};
    for(unsigned i=0;i<sizeof meters/sizeof *meters;i++) CHECK(contrast(meters[i],ui_theme.background)>=3);
    if(render) CloseWindow();
    puts("Fader marks, directional indicators and palette contrast passed."); return 0;
}
