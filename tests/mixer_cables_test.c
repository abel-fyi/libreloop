// SPDX-License-Identifier: GPL-3.0-only
#include "mixer_cables.h"
#include <stdio.h>
#include <string.h>
#define CHECK(x) do{if(!(x)){fprintf(stderr,"%d: %s\n",__LINE__,#x);return 1;}}while(0)
static int check_paths(MixerCable *wires,int count,int lanes,float port,float bottom) {
    for(int i=0;i<count;i++) {
        float x[4],y[4];mixer_cable_path(wires[i],lanes,port,bottom,x,y);
        CHECK(x[0]==wires[i].start && x[3]==wires[i].end && y[0]==port && y[3]==port);
        CHECK(y[1]==y[2] && y[1]>port && y[1]<=bottom);
        for(int j=0;j<i;j++) {
            float other_x[4],other_y[4];mixer_cable_path(wires[j],lanes,port,bottom,other_x,other_y);
            float left=fmaxf(fminf(x[1],x[2]),fminf(other_x[1],other_x[2]));
            float right=fminf(fmaxf(x[1],x[2]),fmaxf(other_x[1],other_x[2]));
            CHECK(right<=left || y[1]!=other_y[1]); /* No coincident horizontal runs. */
            if(x[0]==other_x[0])CHECK((x[1]-x[0])*(other_y[1]-port)!=(other_x[1]-other_x[0])*(y[1]-port));
            if(x[3]==other_x[3])CHECK((x[2]-x[3])*(other_y[2]-port)!=(other_x[2]-other_x[3])*(y[2]-port));
        }
    }
    return 0;
}
int main(void) {
    MixerCable wires[100],shuffled[100];
    for(int count=1;count<=100;count++) {
        for(int i=0;i<count;i++)wires[i]=(MixerCable){.source=1,.destination=i+2,.start=200,.end=300+i*112.f};
        int lanes=mixer_cable_lanes(wires,count);CHECK(lanes==count);
        CHECK(!check_paths(wires,count,lanes,100,129));
        CHECK(!check_paths(wires,count,lanes,100,105));
        for(int i=0;i<count;i++)shuffled[count-i-1]=wires[i];
        CHECK(mixer_cable_lanes(shuffled,count)==lanes && !memcmp(wires,shuffled,count*sizeof *wires));
        for(int i=0;i<count;i++)wires[i]=(MixerCable){.source=i+1,.destination=0,.start=200+i*112.f,.end=55};
        lanes=mixer_cable_lanes(wires,count);CHECK(lanes==count);CHECK(!check_paths(wires,count,lanes,100,129));
    }
    MixerCable disjoint[]={{.start=10,.end=100},{.start=200,.end=120},{.start=220,.end=300}};
    CHECK(mixer_cable_lanes(disjoint,3)==1);CHECK(!check_paths(disjoint,3,1,100,129));
    MixerCable mixed[]={{.source=1,.destination=3,.start=200,.end=400},{.source=2,.destination=3,.start=320,.end=400},{.source=3,.destination=0,.start=422,.end=55}};
    CHECK(mixer_cable_lanes(mixed,3)==3);CHECK(!check_paths(mixed,3,3,100,129));
    for(int height=220;height<=700;height+=13) {
        MixerView v=mixer_view_rows(1200,height,99,0,0,1);
        CHECK(v.cable_space>=0 && v.cable_space<=24);
        CHECK(v.route+12<=height-19 && v.fader_bottom<=v.route-20);
    }
    puts("Distinct fan-out/fan-in paths, deterministic lanes, nonoverlapping runs, disjoint lane reuse and routing bounds passed.");return 0;
}
