// SPDX-License-Identifier: GPL-3.0-only
#ifndef LIBRELOOP_MIXER_CABLES_H
#define LIBRELOOP_MIXER_CABLES_H
#include "mixer_view.h"
/* Concrete routing-row layout. No renderer, project or platform dependency. */
typedef struct {int source,destination,lane;float start,end;} MixerCable;
static inline int mixer_cable_lanes(MixerCable *wires,int count) {
    /* Stable geometry order keeps lanes deterministic across graph enumeration. */
    for(int i=1;i<count;i++) {
        MixerCable item=wires[i];float left=fminf(item.start,item.end),right=fmaxf(item.start,item.end);int at=i;
        while(at>0) {
            MixerCable prev=wires[at-1];float pl=fminf(prev.start,prev.end),pr=fmaxf(prev.start,prev.end);
            if(pl<left || (pl==left && (pr<right || (pr==right && (prev.source<item.source || (prev.source==item.source && prev.destination<=item.destination))))))break;
            wires[at]=prev;at--;
        }
        wires[at]=item;
    }
    int lanes=0;
    for(int i=0;i<count;i++) {
        float left=fminf(wires[i].start,wires[i].end);int lane=0;
        for(;;lane++) {
            int clear=1;
            for(int previous=0;previous<i;previous++)if(wires[previous].lane==lane && fmaxf(wires[previous].start,wires[previous].end)+6>=left){clear=0;break;}
            if(clear)break;
        }
        wires[i].lane=lane;if(lane>=lanes)lanes=lane+1;
    }
    return lanes;
}
static inline void mixer_cable_path(MixerCable wire,int lanes,float port_y,float bottom,float x[4],float y[4]) {
    float depth=fmaxf(1,bottom-port_y),lead=fminf(6,depth/(lanes+1)),lane_y=port_y+lead+(lanes>1?wire.lane*(depth-lead)/(lanes-1):0);
    float span=fabsf(wire.end-wire.start),fan=fminf(span*.25f,8+wire.lane*2),sign=wire.end>=wire.start?1:-1;
    x[0]=wire.start;x[1]=wire.start+sign*fan;x[2]=wire.end-sign*fan;x[3]=wire.end;
    y[0]=y[3]=port_y;y[1]=y[2]=lane_y;
}
#endif
