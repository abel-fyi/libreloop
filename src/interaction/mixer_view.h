// SPDX-License-Identifier: GPL-3.0-only
#ifndef LIBRELOOP_MIXER_VIEW_H
#define LIBRELOOP_MIXER_VIEW_H
#include <math.h>
enum { STUDIO_INSERTS=99, STUDIO_FX_SLOTS=9, MIXER_STRIDE=112 };
typedef struct {
    float width,height,master_x,insert_x,viewport,maximum,offset;
    float fx_top,fx_height,header,controls,fader_top,fader_bottom,route;
    int count,fx_rows;
} MixerView;
static inline MixerView mixer_view(float width,float height,int count,float offset,int fx_hidden) {
    MixerView v={.width=width,.height=height,.master_x=42,.insert_x=158,.fx_top=48,.count=count};
    v.viewport=fmaxf(1,width-v.insert_x-4);
    v.maximum=fmaxf(0,count*MIXER_STRIDE-v.viewport);
    v.offset=fmaxf(0,fminf(v.maximum,offset));
    v.fx_rows=fx_hidden?0:(int)fmaxf(1,fminf(3,floorf((height-202)/22)));
    v.fx_height=v.fx_rows*22;v.header=v.fx_top+v.fx_height+4;
    v.controls=v.header+32;v.fader_top=v.header+48;
    v.fader_bottom=fmaxf(v.fader_top+12,height-66);v.route=height-31;
    return v;
}
/* Motion is already normalized to logical units by navigation_motion(). */
static inline float mixer_scroll_motion(MixerView v,float dx,float dy,int shift,float x,float y,float *fx_scroll) {
    if(x<0 || x>=v.width || y<v.fx_top || y>=v.height-18)return v.offset;
    if(y<v.fx_top+v.fx_height && !shift)*fx_scroll+=dy;
    else if(y>v.controls+14 || (y>=v.header && y<v.header+20))dx+=dy;
    return x>=v.insert_x?fmaxf(0,fminf(v.maximum,v.offset+dx)):v.offset;
}
static inline float mixer_bus_x(MixerView v,int bus) {return bus?v.insert_x+(bus-1)*MIXER_STRIDE-v.offset:v.master_x;}
static inline int mixer_bus_visible(MixerView v,int bus) {
    float x=mixer_bus_x(v,bus);return !bus || (bus<=v.count && x+MIXER_STRIDE>v.insert_x && x<v.insert_x+v.viewport);
}
static inline int mixer_port_visible(MixerView v,int bus,float local_x) {
    float x=mixer_bus_x(v,bus)+local_x;
    return bus==0 || (bus>0 && bus<=v.count && x>=v.insert_x && x<v.insert_x+v.viewport);
}
static inline float mixer_reveal_bus(MixerView v,int bus) {
    if(bus<=0 || bus>v.count)return v.offset;
    float left=(bus-1)*MIXER_STRIDE,right=left+MIXER_STRIDE;
    return fmaxf(0,fminf(v.maximum,left<v.offset?left:right>v.offset+v.viewport?right-v.viewport:v.offset));
}
static inline float mixer_thumb_width(MixerView v) {return fminf(v.viewport,fmaxf(24,v.viewport*v.viewport/fmaxf(1,v.count*MIXER_STRIDE)));}
static inline float mixer_thumb_x(MixerView v) {return v.insert_x+(v.maximum>0?v.offset/v.maximum*(v.viewport-mixer_thumb_width(v)):0);}
static inline float mixer_scroll_from_thumb(MixerView v,float pointer,float grab) {
    float travel=v.viewport-mixer_thumb_width(v);
    return travel>0?fmaxf(0,fminf(v.maximum,(pointer-v.insert_x-grab)/travel*v.maximum)):0;
}
static inline int mixer_route_hit(MixerView v,float x,float y) {
    if(y<v.route || y>=v.route+12)return -1;
    if(x>=v.master_x+7 && x<v.master_x+19)return 0;
    if(x<v.insert_x || x>=v.insert_x+v.viewport)return -1;
    int bus=1+(int)floorf((x-v.insert_x+v.offset)/MIXER_STRIDE);
    float local=x-mixer_bus_x(v,bus);
    return bus>0 && bus<=v.count && local>=7 && local<19?bus:-1;
}
#endif
