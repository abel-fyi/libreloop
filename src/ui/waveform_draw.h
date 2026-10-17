// SPDX-License-Identifier: GPL-3.0-only
#ifndef LIBRELOOP_WAVEFORM_DRAW_H
#define LIBRELOOP_WAVEFORM_DRAW_H
#include "render.h"
#include "waveform.h"
typedef struct {
    Sample sample; const Waveform *wave;
    int channel,preview;
    const AudioTimeline *timeline;
    double start,offset,frames_per_step;
    float origin,width,pixels_per_step;
} WaveDrawing;
typedef WavePeak (*WavePreviewEnvelope)(void *context,int channel,double position,double width,unsigned frames);
typedef struct { DrawPoint point; float u,v; } WaveVertex;
double wave_drawing_source(const WaveDrawing *view,float x);
/* Emit original quads in their original order; draw integration triangulates
   only at the renderer boundary. Useful for original geometry comparison. */
void wave_drawing_quads(WaveDrawing view,DrawRect area,float left,float right,float layout_scale,float raster_scale,
    WavePreviewEnvelope preview,void *preview_context,void (*emit)(void *,const WaveVertex quad[4]),void *emit_context);
void waveform_draw(UIRender *render,WaveDrawing view,DrawRect area,float left,float right,float layout_scale,float raster_scale,
    DrawColor color,WavePreviewEnvelope preview,void *context);
DrawColor waveform_source_color(uint32_t rgb);
DrawColor waveform_audio_color(uint32_t rgb,unsigned frames);
#endif
