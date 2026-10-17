// SPDX-License-Identifier: GPL-3.0-only
/* Original ui_waveform.c source/envelope/quad math; no project/input mutation. */
#include "waveform_draw.h"
#include "theme.h"
#include <math.h>
double wave_drawing_source(const WaveDrawing *view,float x) {
    if(view->timeline)return view->offset+audio_timeline_source(view->timeline,view->start,
        view->start+fmax(0,x-view->origin)/view->pixels_per_step)*view->frames_per_step;
    return (x-view->origin)/view->width*view->sample.frames;
}
static void quad(void (*emit)(void *,const WaveVertex[4]),void *context,float x,float xx,float top,float next_top,float bottom,float next_bottom,float v,float vv) {
    WaveVertex vertices[]={{{x,top},.5f,v},{{x,bottom},.5f,vv},{{xx,next_bottom},.5f,vv},{{xx,next_top},.5f,v}};emit(context,vertices);
}
void wave_drawing_quads(WaveDrawing view,DrawRect area,float left,float right,float layout_scale,float raster_scale,
    WavePreviewEnvelope preview,void *preview_context,void (*emit)(void *,const WaveVertex[4]),void *emit_context) {
    if(!view.sample.frames || view.width<=0 || right<=left || area.height<=0)return;
    float step=1/layout_scale,feather=.5f/raster_scale,mid=area.y+area.height*.5f,amplitude=fmaxf(0,area.height*.5f-2/raster_scale);
    float previous_x=left,previous_top=mid,previous_bottom=mid;int columns=ceilf((right-left)/step);
    for(int i=0;i<=columns;i++) {
        float x=fminf(right,left+i*step);double source=wave_drawing_source(&view,x),width=fabs(wave_drawing_source(&view,x+step)-source);WavePeak peak;
        if(view.preview)peak=preview(preview_context,view.channel,source,width,view.sample.frames);
        else peak=waveform_envelope(view.wave,view.sample,source,width);
        float top=mid-fmaxf(0,peak.high)*amplitude,bottom=mid-fminf(0,peak.low)*amplitude;
        top=fminf(top,mid-feather);bottom=fmaxf(bottom,mid+feather);
        if(i) {
            quad(emit,emit_context,previous_x,x,previous_top-feather,top-feather,previous_top+feather,top+feather,0,.25f);
            quad(emit,emit_context,previous_x,x,previous_top+feather,top+feather,previous_bottom-feather,bottom-feather,.5f,.5f);
            quad(emit,emit_context,previous_x,x,previous_bottom-feather,bottom-feather,previous_bottom+feather,bottom+feather,.75f,1);
        }
        previous_x=x;previous_top=top;previous_bottom=bottom;
    }
}
typedef struct { DrawContext *draw;DrawTexture *texture;DrawColor color; } WaveDraw;
static void emit(void *context,const WaveVertex quad[4]) {
    WaveDraw *draw=context;DrawVertex triangles[6];const int order[]={0,1,2,0,2,3};
    for(int i=0;i<6;i++){WaveVertex v=quad[order[i]];triangles[i]=(DrawVertex){v.point,v.u,v.v,draw->color};}
    draw_triangles(draw->draw,draw->texture,triangles,6);
}
void waveform_draw(UIRender *render,WaveDrawing view,DrawRect area,float left,float right,float layout_scale,float raster_scale,
    DrawColor color,WavePreviewEnvelope preview,void *context) {
    WaveDraw draw={ui_render_draw(render),ui_render_cable_texture(render),color};
    wave_drawing_quads(view,area,left,right,layout_scale,raster_scale,preview,context,emit,&draw);
}
DrawColor waveform_source_color(uint32_t rgb) {
    DrawColor color={rgb>>16,(rgb>>8)&255,rgb&255,255};DrawHSV hsv=draw_color_hsv(color);
    return draw_color_from_hsv(hsv.x,fminf(hsv.y,.25f),hsv.z);
}
DrawColor waveform_audio_color(uint32_t rgb,unsigned frames) {
    if(rgb)return waveform_source_color(rgb);
    float t=fminf(1,log1pf(frames/(float)RATE)/log1pf(30));return draw_color_from_hsv(320-235*t,.25f,.98f);
}
