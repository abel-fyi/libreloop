// SPDX-License-Identifier: GPL-3.0-only
/* Draw-only frozen knob_style geometry. Shown value is supplied from the
   original pre-interaction snapshot; this function never mutates a parameter. */
#include "controls_draw.h"
#include "theme.h"
#include <math.h>
#define PI 3.14159265358979323846f
#define RAD2DEG (180.0f/PI)
#define KNOB_RADIUS 10
static void arc(UIRender *render,DrawRect source,DrawRect destination,DrawPoint origin,float rotation,DrawColor color) {
    int bank=(int)source.x/512,level=((int)source.y/32)*16+((int)source.x%512)/32;
    ui_render_arc_pro(render,bank,level,destination,origin,rotation,color);
}
void knob_draw(UIRender *render,const KnobDrawing *state) {
    int x=state->x,y=state->y,style=state->style,over=state->hovered;
    float low=state->low,high=state->high,initial=state->initial,shown=state->shown,size=style==CONTROL_SWING?8:10;
    if(style==CONTROL_FIXED_COARSE){high=3;initial=(int)initial&3;shown=(int)roundf(shown)&3;}
    float radius=size,fraction=(shown-low)/(high-low);
    if(style==CONTROL_LOGARITHMIC) fraction=logf(shown/low)/logf(high/low);
    if(style==CONTROL_CENTER) fraction=shown<initial?.5f*(shown-low)/(initial-low):.5f+.5f*(shown-initial)/(high-initial);
    if(style==CONTROL_WIDTH) fraction=1-fraction;
    if(style==CONTROL_VOLUME) fraction=shown<=1?.75f*(shown):.75f+.25f*(shown-1)/(high-1);
    int centered=style==CONTROL_PAN || style==CONTROL_WIDTH || style==CONTROL_CENTER;
    float origin=centered?PI/2:style==CONTROL_SWING?2.4f:style==CONTROL_VOLUME?PI/2:-PI/2;
    float angle=origin+fraction*(style==CONTROL_SWING?4.6f:2*PI);
    DrawColor color=style==CONTROL_SWING || style==CONTROL_CENTER?ui_theme.swing:style==CONTROL_PAN?(fraction<.5f?ui_theme.pan_left:ui_theme.pan_right):style==CONTROL_WIDTH?(fraction<.5f?ui_theme.stereo:ui_theme.mono):ui_theme.knob;
    if((over || state->dragging) && (style==CONTROL_NORMAL || style==CONTROL_VOLUME || style==CONTROL_LOGARITHMIC || style==CONTROL_FIXED_COARSE)) color=ui_theme.highlight;
    if(style==CONTROL_SWING) {
        arc(render,(DrawRect){0,256,32,32},(DrawRect){x-radius,y-radius,radius*2,radius*2},(DrawPoint){0},0,ui_theme.knob_track);
        ui_render_circle(render,x,y,radius*.68f,ui_theme.control);
    } else { ui_render_circle(render,x,y,radius,ui_theme.knob_track); ui_render_circle(render,x,y,radius*.8f,ui_theme.control); }
    if(style==CONTROL_PAN || style==CONTROL_WIDTH) {
        DrawColor left=style==CONTROL_PAN?ui_theme.pan_left:ui_theme.stereo;
        DrawColor right=style==CONTROL_PAN?ui_theme.pan_right:ui_theme.mono;
        float opacity=.35f;
        DrawRect ring={x,y,radius*2,radius*2}; DrawPoint center={radius,radius};
        arc(render,(DrawRect){512,0,32,32},ring,center,0,draw_color_fade(left,opacity));
        arc(render,(DrawRect){512,256,32,32},ring,center,0,draw_color_fade(right,opacity));
    }
    int level=fmaxf(0,fminf(128,roundf(fraction*128)));
    float rotation=style==CONTROL_SWING || centered?0:(origin+PI/2)*RAD2DEG;
    arc(render,(DrawRect){(style==CONTROL_SWING?0:centered?512:1024)+level%16*32,level/16*32,32,32},(DrawRect){x,y,radius*2,radius*2},(DrawPoint){radius,radius},rotation,color);
    if(style==CONTROL_VOLUME) {
        float unity=origin+.75f*2*PI;
        ui_render_circle(render,x+cosf(unity)*size*1.22f,y+sinf(unity)*size*1.22f,1.2f,ui_theme.secondary);
    }
    DrawPoint pointer={x+cosf(angle)*radius*.85f,y+sinf(angle)*radius*.85f};
    float pointer_scale=size/KNOB_RADIUS;
    ui_render_circle(render,pointer.x,pointer.y,2.3f*pointer_scale,(DrawColor){35,24,49,255});
    ui_render_circle(render,pointer.x,pointer.y,1.7f*pointer_scale,((DrawColor){255,255,255,255}));
}
